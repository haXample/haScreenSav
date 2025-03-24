// --- THIS FILE IS DEPRECATED (textfile no longer used in haScreenSav) ---

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

int _cbtFolderFlag = FALSE; // CBT Hook title/button string control

TCHAR pathDisplay[MAX_PATH+1] = _T("");
TCHAR MfpathDisplay[MAX_PATH+1] = _T("");

TCHAR szPathSaveCpy[MAX_PATH+1];
TCHAR szMfPathSaveCpy[MAX_PATH+1];

BROWSEINFO bi = {0};                  // Global
LPITEMIDLIST pidl = NULL;             // Global
LPITEMIDLIST pidlPathSave = NULL;     // Global
PCUIDLIST_ABSOLUTE pidlSave = NULL;   // Global

BROWSEINFO Mfbi = {0};                // Global
LPITEMIDLIST Mfpidl = NULL;           // Global
PCUIDLIST_ABSOLUTE MfpidlSave = NULL; // Global

TCHAR _mdfpath[1];   // Multifile destination path
TCHAR mdPathSave[];  // Multifile destination path save

char szExtensionSave[20];
char oldFileExtension[20];
char szPathSave[MAX_PATH+1];

// Forward declaration of functions included in this code module:
void CenterInsideParent(HWND, char*);

int CBTMessageBox(HWND, LPCTSTR, LPCTSTR, UINT);
LRESULT CALLBACK CBTProc(INT, WPARAM, LPARAM);


//////////////////////////////////////////////////////////////////////////////////
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

//ha//int CBTCustomMessageBox(HWND _hwnd, LPCTSTR lpText, LPCTSTR lpCaption,
//ha//                        UINT uType, UINT uIconResID)
//ha//  {
//ha//  _hHook = SetWindowsHookEx(WH_CBT, &CBTProc, 0, GetCurrentThreadId());
//ha//  return CustomMessageBox(_hwnd, lpText, lpCaption, uType, uIconResID);
//ha//  }

int CBTMessageBox(HWND _hwnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
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

    // Change the text of the buttons: YES, NO, CANCEL, Make new folder, ...
    if (GetDlgItem(hChildWnd, IDYES) != NULL)            // IDYES = System defined
      SetDlgItemText(hChildWnd, IDYES, _T("Yes"));       // Always English text 'Yes'

    if (GetDlgItem(hChildWnd, IDNO) != NULL)             // IDNO = System defined
      SetDlgItemText(hChildWnd, IDNO, _T("No"));         // Always English text 'No'

    if (GetDlgItem(hChildWnd, IDNEWFOLDER) != NULL)      // 0x3746 = System specific
      SetDlgItemText(hChildWnd, IDNEWFOLDER, _T("Make new folder")); // Always English text

//ha//if (GetDlgItem(hChildWnd, IDOK) != NULL)           // IDOK = System defined
//ha//  UINT result = SetDlgItemText(hChildWnd, IDOK, _T("+ha+")); // Test

//ha//    // Check if GetCurrentThreadId() is SHBrowseForFolder()
//ha//    if (_cbtFolderFlag == (MULTIFILE_BROWSER_CRYPTO | MULTIFILE_BROWSER_RENAME)) // Browser Results Filter 
//ha//      {
//ha//      SetDlgItemText(hChildWnd, IDCANCEL, _T("Close"));  // Multifile Reslults Filter 'Rename' or 'Crypto'
//ha//      SetWindowText(hChildWnd, _T("File(s) processed")); // Browser's title field
//ha//
//ha//      // Set focus on [Close] button (..default action if VK_RETURN key is pressed)
//ha//      SendMessage(hChildWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hChildWnd, IDCANCEL), TRUE); // = [Close] button
//ha//      SendMessage(hChildWnd, BFFM_ENABLEOK, TRUE, 0);                                       // Grayed OK Button 
//ha//      }
//ha//    else if (_cbtFolderFlag == MULTIFILE_BROWSER_RENAME || // Browser selection
//ha//             _cbtFolderFlag == MULTIFILE_BROWSER_CRYPTO)
//ha//      {
//ha//      SetDlgItemText(hChildWnd, IDCANCEL, _T("Cancel")); // Multifile Selection 'Rename' or 'Crypto'
//ha//      SetWindowText(hChildWnd, _T("Select a folder"));   // Browser's title field
//ha//      }

    // Center the client message box within the parent window
    if ((hParentWnd != 0) && (hChildWnd != 0) &&
        (GetWindowRect(GetDesktopWindow(), &rDesktop) != 0) &&
        (GetWindowRect(hParentWnd, &rParent) != 0) &&
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

      // Adjust if message box is off desktop
      // Note:
      //  Multifile Selection 'Rename' and 'Crypto' is done in (haBrowse.cpp).
      //  Multifile Results Filter positioning must be done in (haCryptWRL.cpp) 
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

//------------------------------------------------------------------------------
//
//                      OpenBrowserDialog - Rename
//
// Unfortunately, 'ShBrowseForFolder()' does not allow you to specify 
//  different text for the window title.
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
  //-----------------------------------------------------------------------
  // If multiple files are selected
  //  open a dialog box to choose a folder where to save the selected files
  //
  int i;
  BOOL bSuccess = FALSE;
  IMalloc* imalloc = 0;

  //BROWSEINFO bi = {0};            // Global, see above
                                   
  bi.hwndOwner      = hwnd;
  bi.pidlRoot       = pidlPathSave; // NULL;       
  bi.pszDisplayName = pathDisplay;
  bi.lpszTitle      = _T("Choose a folder, and select anyone of the files\n")
                      _T("with an extension that should be renamed on\n")
                      _T("all files of that type in the chosen folder.");
  bi.ulFlags        = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_BROWSEINCLUDEFILES;// | BIF_NOTRANSLATETARGETS; // | BIF_UAHINT;
  //bi.lpfn           = BrowseCallbackProc;
  bi.lParam         = (LPARAM)szPathSave;
  bi.iImage         = 0;

  pidl = CBTSHBrowseForFolder(bi);           // Get current LPITEMIDLIST
  _cbtFolderFlag = FALSE;                    // CBT Hook default 

  pidlSave = ILCloneFull(pidl);   // Clone the LPITEMIDLIST for convenient usage

  if (pidl != NULL)
    {
    // Get the name of the folder into 'szPathSave[]'
    //  and concatenate a backslash
    if (SHGetPathFromIDList(pidl, szPathSave))
      {
      if (GetFileAttributes(szPathSave) & FILE_ATTRIBUTE_DIRECTORY)
        {
        lstrcat(szPathSave, _T("\\"));  // It's a folder, so append it with '\'
        oldFileExtension[0] = 0;        // Clear file extension pattern
        } 

      else                              // It's path + filename
        {
///////////// Screen saver stuff ///////////////////
        StrCpy(szhaScrFilename, szPathSave);
        for (i=lstrlen(szPathSave); i>0; i--)  // Search start-of-filename
          {                                    //  (which is end-of-path)
          if (szPathSave[i] == (WCHAR)'.') break;
          }
        if (StrCmpI(&szPathSave[i], ".frt") != 0)
          errchk(pszhaScrFilename, ERROR_OPEN_FAILED);     
        } // end else
///////////// Screen saver stuff ///////////////////

      } // end if

    bSuccess = TRUE;
    } // end if (pidl)

  return(bSuccess);
  } // OpenBrowserDialog
//////////////////////////////////////////////////////////////////////////////////
