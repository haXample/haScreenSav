// haScreensav - Screensaver for windows 10, 11, ...
// haScrBrowse.cpp - C++ Developer source file.
// (c)2025 by helmut altmann

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#include <sys\types.h> // For _open( , , S_IWRITE) needed for VC 2010
#include <sys\stat.h>  // For filesize
#include <fcntl.h>     // Console
#include <stdlib.h>
#include <stdio.h>                        
#include <io.h>                                                        
#include <iostream>
#include <conio.h>

#include <shlwapi.h>  // Library shlwapi.lib for PathFileExistsA
#include <commctrl.h> // Library Comctl32.lib
#include <commdlg.h>
#include <winuser.h>
#include <time.h>

#include <string.h>                                                 
#include <string>     // sprintf, etc.
#include <tchar.h>     
#include <strsafe.h>  // <strsafe.h> must be included after <tchar.h>

#include <windows.h>
#include <scrnsave.h>

#include <shlobj.h>    // For browsing directory info
#include <unknwn.h>    // For browsing directory info

#include "hascreensav.h"

using namespace std;

int _cbtFolderFlag = FALSE; // CBT Hook title/button string control

TCHAR pathDisplay[MAX_PATH+1] = _T("");

BROWSEINFO bi = {0};                  // Global
LPITEMIDLIST pidl = NULL;             // Global
LPITEMIDLIST pidlPathSave = NULL;     // Global
PCUIDLIST_ABSOLUTE pidlSave = NULL;   // Global

char szPathSave[MAX_PATH+1];

// Extern variables and functions
extern TCHAR szhaScrFilename[];
extern TCHAR* pszhaScrDfltFilename;
extern TCHAR* pszhaScrFilename;

extern HWND hwnd;

extern void errchk(char*, int);

// Forward declaration of functions included in this code module:
int CBTMessageBox(HWND, LPCTSTR, LPCTSTR, UINT);
LRESULT CALLBACK CBTProc(INT, WPARAM, LPARAM);

//-----------------------------------------------------------------------------
//
//                           DoRootFolder
//
BOOL DoRootFolder(WCHAR *rootFolder)
  {
  ULONG         chEaten;  
  ULONG         dwAttributes;  
  IShellFolder* pDesktopFolder;
    
  if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))  
    {  
    // Get PIDL for root folder  
    pDesktopFolder->ParseDisplayName(NULL, NULL, rootFolder, &chEaten, &pidlPathSave, &dwAttributes);  
    pDesktopFolder->Release();  
    }  

  // Store PIDL for root folder in BROWSEINFO  
  bi.pidlRoot = pidlPathSave;  
  //bi.lParam = NULL;  
  return TRUE;  
  } // DoRootFolder  
 
//-----------------------------------------------------------------------------
//
//                       CustomMessageBox
//
// ... usage example 
// int msgID = CustomMessageBox(m_hWnd, _T("TEXT"), _T("Caption"), MB_OKCANCEL, IDI_ICON1);
//
int CustomMessageBox(HWND _hwnd, char* lpText,  char* lpCaption,
                                 UINT uType, UINT uIconResID)
   {
   MSGBOXPARAMS mbp;

   mbp.cbSize             = sizeof(MSGBOXPARAMS);
   mbp.hwndOwner          = _hwnd;                       // hMain
   mbp.hInstance          = GetModuleHandle(NULL);
   mbp.lpszText           = lpText;                      // Text within window
   mbp.lpszCaption        = lpCaption;                   // Text in window header
   mbp.dwStyle            = uType | MB_USERICON;         // Set Butoons and custom icon style
   mbp.dwLanguageId       = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
   mbp.lpfnMsgBoxCallback = NULL;
   mbp.dwContextHelpId    = 0;
   mbp.lpszIcon           = MAKEINTRESOURCE(uIconResID); // Custom icon ID (haScreensav.rc)
   
   return MessageBoxIndirect(&mbp); // Returns button choice (e.g. msgID = IDYES, IDNO)
   } // CustomMessageBox

//-----------------------------------------------------------------------------
//
//                      - Centered Message Boxes -
//                         CBTSHBrowseForFolder
//                         CBTCustomMessageBox
//                         CBTMessageBox
//
// Message boxes are normally placed in the center of the desktop window.
// It is often desirable to create messages boxes over the parent app window.
// This cannot be done by using the standard Win32 API call:
// 
//  PIDLIST_ABSOLUTE CBTSHBrowseForFolder(BROWSEINFO bi);
//  int CustomMessageBox(HWND, LPCTSTR, LPCTSTR, UINT, UINT uIconResID);
//  int MessageBox(HWND, LPCTSTR, LPCTSTR, UINT);
// 
// The code below centers a message box over a parent window by using a
// CBT (computer-based training) hook. All message box calls are made with
// a substitute function called 'CBTMessageBox', which inserts the CBT hook
// prior to activating a message box.
// 
// The CBTProc function processes a thread-specific hook.
// The needed notification code is HCBT_ACTIVATE, which is issued whenever
// a new window is about to be activated (made visible).
// The wParam holds the forthcoming window handle value.
// The parent window handle is found using the GetForegroundWindow() function.
// 
// Centering is done by finding the center point of the parent window
// and by calculating the upper left corner starting point of the message box.
// The starting point will be adjusted if the message box dimensions
// exceed the desktop window region.
// 
// The CBT hook remains active until the necessary HCBT_ACTIVATE code is issued.
// Because several other CBT codes may be waiting in the hook queue
// when the SetWindowsHookEx call is made, it is useful to have the CBTProc
// continue the hook chain if another code is issued first.
// The CallNextHookEx function helps to ensure proper
// chaining of hooks issued by other apps.
// 
// The CBT hook process can be used to center any window to another.
// Just declare the hHook=SetWindowsHookEx function before activating a window.
//
//  Substitute
//      CBTSHBrowseForFolder(BROWSEINFO); 
//      CBTCustomMessageBox(HWND, LPCTSTR, LPCTSTR, UINT, UINT uIconResID);
//      CBTMessageBox(HWND, LPCTSTR, LPCTSTR, UINT);
//  instead of
//      SHBrowseForFolder(&bi);
//      CustomMessageBox(HWND, LPCTSTR, LPCTSTR, UINT, UINT uIconResID);
//      MessageBox(HWND, LPCTSTR, LPCTSTR, UINT);
//  to produce a centered message box.
//
HHOOK _hHook;   // Declare the hook handle as global

PIDLIST_ABSOLUTE CBTSHBrowseForFolder(BROWSEINFO bi)
  {
  _hHook = SetWindowsHookEx(WH_CBT, &CBTProc, 0, GetCurrentThreadId());
  return SHBrowseForFolder(&bi);
  }

int CBTCustomMessageBox(HWND _hwnd, char* lpText, char* lpCaption,
                        UINT uType, UINT uIconResID)
  {
  _hHook = SetWindowsHookEx(WH_CBT, &CBTProc, 0, GetCurrentThreadId());
  return CustomMessageBox(_hwnd, lpText, lpCaption, uType, uIconResID);
  }

int CBTMessageBox(HWND _hwnd, char* lpText, char* lpCaption, UINT uType)
  {
  _hHook = SetWindowsHookEx(WH_CBT, &CBTProc, 0, GetCurrentThreadId());
  return MessageBox(_hwnd, lpText, lpCaption, uType);
  }

//-----------------------------------------------------------------------------
//
//                           CBTProc (Thread)
//
// Change the text of the [Yes], [No], [Cancel] and [Make new folder] buttons
// The text should be always meaningful English.
// Center the MessageBox within the parent window.
// Keep the MessageBox within the Desktop limits.
//
// ID=0x3746 = [Make new folder] button is undocumented (but very useful here).
//
#define IDNEWFOLDER 0x3746 // Microsoft system specific undocumented ID

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
  {
  HWND  hParentWnd, hChildWnd;    // MessageBox is "child"
  RECT  rParent, rChild, rDesktop;
  POINT pCenter, cStart;
  int   cWidth, cHeight;

  // Notification that a child window is about to be activated.
  // Child window handle is wParam
  if (nCode == HCBT_ACTIVATE)
    {
    // Set window handles
    hParentWnd = GetForegroundWindow();
    hChildWnd  = (HWND)wParam;

    // Check if GetCurrentThreadId() is SHBrowseForFolder()
    // Change the text of the buttons: [YES], [NO], [CANCEL], [Make new folder], ...
    // Buttons' text should be always in English.
    SetDlgItemText(hChildWnd, IDCANCEL, _T("Faust I+II"));  // IDCANCEL = Faust.FRT Selection
    SetWindowText(hChildWnd, _T("Browse for files *.FRT")); // Browser and CBTMessageBox title field

//ha//    if (GetDlgItem(hChildWnd, IDYES) != NULL)            // IDYES = System defined
//ha//      SetDlgItemText(hChildWnd, IDYES, _T("Yes"));
//ha//
//ha//    if (GetDlgItem(hChildWnd, IDNO) != NULL)             // IDNO = System defined
//ha//      SetDlgItemText(hChildWnd, IDNO, _T("No"));
//ha//
//ha//    if (GetDlgItem(hChildWnd, IDNEWFOLDER) != NULL)      // 0x3746 = System specific
//ha//      SetDlgItemText(hChildWnd, IDNEWFOLDER, _T("Make new folder"));
//ha//
//ha//    if (GetDlgItem(hChildWnd, IDOK) != NULL)             // IDOK = System defined
//ha//      SetDlgItemText(hChildWnd, IDOK, _T("+ha+"));       // Test only (OK stays as is)

    // Center the client message box within the parent window
    if ((hParentWnd != 0) && (hChildWnd != 0)               &&
        (GetWindowRect(GetDesktopWindow(), &rDesktop) != 0) &&
        (GetWindowRect(hParentWnd, &rParent) != 0)          &&
        (GetWindowRect(hChildWnd, &rChild) != 0))
      {
      // Calculate client message box dimensions
      cWidth  = (rChild.right  - rChild.left);
      cHeight = (rChild.bottom - rChild.top);

      // Calculate parent window center point
      pCenter.x = rParent.left + ((rParent.right- rParent.left)/2);
      pCenter.y = rParent.top  + ((rParent.bottom-rParent.top)/2);

      // Calculate client message box starting point position
      cStart.x = (pCenter.x - (cWidth/2));
      cStart.y = (pCenter.y - (cHeight/2));

      // Adjust if client box is off desktop
      // Note:
      //  Positioning must be done in DLG_SCRNSAVECONFIGURE (haSreensav.rc) 
      if (_cbtFolderFlag == FALSE)  // Default = (Custom)MessageBox(..) 
        {
        if (rDesktop.bottom < rParent.bottom) cStart.y = rParent.top-cHeight/2;
        if (rDesktop.right  < rParent.right ) cStart.x = rDesktop.right-cWidth;
        if (rParent.left    < rDesktop.left ) cStart.x = rDesktop.left;
        //if (rParent.top   < rDesktop.top  ) cStart.y = rDesktop.Top;  // Shouldn't occur)
        MoveWindow(hChildWnd, cStart.x, cStart.y, cWidth, cHeight, FALSE);
        }
      } // end if

    // Exit CBT hook
    UnhookWindowsHookEx(_hHook);
    } // end if (nCode)

  // Otherwise, continue with any possible chained hooks
  else CallNextHookEx(_hHook, nCode, wParam, lParam);
  return 0;
  } // CBTProc

//-----------------------------------------------------------------------------
//
//                  RepositionBrowseWindow
//
// !! Note: 'SetWindowPos(..)' doesn't work properly with SHBrowseForFolder() !!
//
void RepositionBrowseWindow(HWND _hwnd)
      {
      RECT BrowserRect;                       // Always the same values ??!!

      ::GetWindowRect(_hwnd, &BrowserRect);   // The browser window

      // Re-position the Browser window at UPPER left of Dialog-field
      // 0,0 - Resizing doesn't work with SHBrowseForFolder()                                   
      ::SetWindowPos(_hwnd, NULL,         
                     BrowserRect.left, BrowserRect.top, 0, 0, 
                     SWP_NOZORDER | SWP_NOSIZE); // | SWP_SHOWWINDOW | SWP_NOACTIVATE);

      UpdateWindow(_hwnd);
      } // RepositionBrowseWindow

//-----------------------------------------------------------------------------
//
//                           BrowseCallbackProc
//
// https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nc-shlobj_core-bffcallback
// Set default path SHBrowseForFolder() to folder paths by BFFM_SETSELECTION message
// 
// The BFFCallBack function is an application-defined callback function
// that receives event notifications from the 
// Active Directory Domain Services container browser dialog box.
// A pointer to this function is supplied to the container browser dialog box
// in the pfnCallback member of the DSBROWSEINFO structure when the
// DsBrowseForContainer function is called.
//  BFFCallBack is a placeholder for the application-defined function name.
// 
// BFFCALLBACK Bffcallback;  // = BrowseCallbackProc below
// 
// int Bffcallback(
//   [in] HWND hwnd,
//   [in] UINT uMsg,
//   [in] LPARAM lParam,
//   [in] LPARAM lpData
// )
// {...}
// 
// BFFM_SETSELECTION
// This message selects an item in the dialog box.
// The lParam of this message is a pointer to a TCHAR string
// that contains the ADsPath of the item to be selected.
// Even though there are ANSI and Unicode versions of this message,
// both versions take a pointer to a Unicode string.
//
// BFFM_SELCHANGED
// This notification is sent after the selection in the dialog box is changed.
//
// lParam is a pointer to a Unicode string that contains the ADsPath of the newly selected item.
//
// bi.lParam = LPARAM lpData 
//
int CALLBACK BrowseCallbackProc(HWND _hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
  {
  switch (uMsg)
    {
    case BFFM_INITIALIZED:
      RepositionBrowseWindow(_hwnd);
      SendMessage(_hwnd, BFFM_SETEXPANDED,         // Select path and show files
                        TRUE, 
                        lpData);
                         
      SendMessage(_hwnd, BFFM_SETSELECTION,        // works, szhaScrFilename
                  TRUE,
                  (LPARAM)szhaScrFilename); 
      break;

    case BFFM_SELCHANGED:
//ha//      SendMessage(_hwnd, BFFM_SETSELECTION,  // works, but makes no sense
//ha//                  TRUE,
//ha//                  (LPARAM)szhaScrFilename); 
      break;
    } // end switch

  return 0;
  } // BrowseCallbackProc

//------------------------------------------------------------------------------
//
//                      OpenBrowserDialog - Rename
//
// Unfortunately, 'ShBrowseForFolder()' does not allow you to specify 
//  different text for the window title itself. 
// A sophisticated HOOK is required to accomplish this. See above:
//  1) PIDLIST_ABSOLUTE CBTSHBrowseForFolder(BROWSEINFO bi)
//  2) LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
//
// typedef struct _browseinfoW {
//   HWND              hwndOwner;
//   PCIDLIST_ABSOLUTE pidlRoot;   // Root folder from which to start browsing
//   LPWSTR            &pszDisplayName;
//   LPCWSTR           lpszTitle;
//   UINT              ulFlags;
//   BFFCALLBACK       lpfn;
//   LPARAM            lParam;
//   int               iImage;
// } BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;
//
// PIDLIST_ABSOLUTE ILCloneFull(
//   [in] PCUIDLIST_ABSOLUTE pidl
// );
//
BOOL OpenBrowserDialog()
  {
  int i;
  BOOL bSuccess = FALSE;
  IMalloc* imalloc = 0;

  //BROWSEINFO bi = {0};            // Global, see above
                                   
  bi.hwndOwner      = hwnd;
  bi.pidlRoot       = pidlPathSave; // NULL;       
  bi.pszDisplayName = pathDisplay;
  bi.lpszTitle      = _T("Choose a folder, and select a file\n") // Comment-field text
                      _T("of type filename.FRT\n");
  bi.ulFlags        = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_BROWSEINCLUDEFILES;
  bi.lpfn           = BrowseCallbackProc;
  bi.lParam         = (LPARAM)szPathSave;
  bi.iImage         = 0;

  pidl = CBTSHBrowseForFolder(bi);  // Get current LPITEMIDLIST
  _cbtFolderFlag = FALSE;           // CBT Hook default 

  pidlSave = ILCloneFull(pidl);     // Clone the LPITEMIDLIST for convenient usage

  if (pidl != NULL)
    {
    // Get the name of the folder into 'szPathSave[]'
    //  and concatenate a backslash
    if (SHGetPathFromIDList(pidl, szPathSave))
      {
      /////////// Screen saver stuff /////////////////////////////////////////
      for (i=lstrlen(szPathSave); i>0; i--)     // Search start-of-filename //
        {                                       //  (which is end-of-path)  //
        if (szPathSave[i] == (WCHAR)'.') break;                             //
        }                                                                   //
      if (StrCmpI(&szPathSave[i], ".frt") != 0)                             //
        errchk(szPathSave, ERROR_BAD_FORMAT);                               //
      else bSuccess = TRUE;                                                 //
      ////////////////////////////////////////////////////////////////////////
      } // end if (SHGetPathFromIDList)
     } // end if (pidl)

  // Update the buffer with the currently valid filepath
  // or set the default to the internal .\haFaust.FRT
  if (!bSuccess) StrCpy(szhaScrFilename, pszhaScrDfltFilename);
  else StrCpy(szhaScrFilename, szPathSave);

  return(bSuccess);
  } // OpenBrowserDialog

//------------------------------------------------------------------------------

