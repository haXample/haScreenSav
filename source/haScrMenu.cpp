// haScreensav - Screensaver for windows 10, 11, ...
// haScMenu.cpp - C++ Developer source file.
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

// Global variables
char textDialogBuf[MAX_PATH+1] = ""; // Temporary buffer for formatted text

int textModeFlag = MODE_RANDOM;      // Mode: Stepping through text file randomly 
int _txtIndexSav = 1;
ULONG txtOffset;

// RECTANGLE structure
// typedef struct tagRECT {
//   LONG left;
//   LONG top;
//   LONG right;
//   LONG bottom;
// } RECT, *PRECT, *NPRECT, *LPRECT;
RECT rcSearchResult;         

HWND hSearchMenu;

// Extern variables and functions
extern char DebugBuf[];          // Temporary buffer for formatted text
extern int DebugbufSize;
extern char* psz_DebugBuf;
extern TCHAR _tDebugBuf[];       // Temp buffer for formatted UNICODE text
extern int _tDebugbufSize;
extern TCHAR* psz_tDebugBuf;

extern HWND hwndTT;
extern HWND hTextRandomButton;   // Text Random Button window
extern HWND hTextNextButton;     // Text Next Button window
extern HWND hTextPreviousButton; // Text Previous Button window
extern HWND hTextCurButton;      // Current Text Button window
extern HWND hChooseColor;
extern HWND hwnd;                // owner window
extern HWND hDlgSetup;           // Copy of hDlg
extern HWND hTextMenu;

extern HDC  hdcTextMenu;         // TextMenu-context handle  

extern HGLOBAL hMem;             // Memory handle for clipboard

extern RECT rc;         
extern RECT rcTextMenu;         
extern RECT rcTextMenuButtons;
 
extern char* pszString;
extern char* pszTxtFilebuf;
extern char  szTruncPath[];

extern ULONG bytesrd;            // Textfile number of byte read

extern int _stepRandom;
extern int txtIndex;
extern int textMaxIndex;

extern void ScrSavDrawText(HWND, int);
extern int CustomMessageBox(HWND, char*,  char*, UINT, UINT);

// Forward declaration of functions included in this code module:
INT_PTR CALLBACK DialogProcTextMenu(HWND, UINT, WPARAM, LPARAM);

//-----------------------------------------------------------------------------
//
//                       CreateToolTip
//
// Create a QuickInfo for any desired window
//
void CreateToolTip(HWND _hDesired, 
                   char* _szTooltipTexthDesired, 
                   const int _STYLE)
  {
  // Create "tooltip" quick information for Key Button. 
  hwndTT = CreateWindowEx(WS_EX_TOPMOST, 
                          TOOLTIPS_CLASS, 
                          NULL,
                          WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | _STYLE,   
                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                          _hDesired, 
                          NULL, 
                          hMainInstance, 
                          NULL);

  if (!hwndTT) MessageBox(NULL, "Failed: HWND hwndTT", "ERROR", MB_ICONERROR);

  SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
 
  TOOLINFO ti = {0};
  ZeroMemory(&ti, sizeof(TOOLINFO));

  ti.cbSize   = SIZE_TOOLINFO;     // TTTOOLINFOW_V2_SIZE or sizeof(TOOLINFO);
  ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
  ti.hwnd     = _hDesired;
  ti.hinst    = hMainInstance;
  ti.uId      = (UINT_PTR)_hDesired;
  ti.lpszText = _szTooltipTexthDesired;

  // Associate the tooltip with the "tool" window.
  if (!SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti))
    MessageBox(NULL, "Failed: TTM_ADDTOOL", ERROR, MB_ICONERROR);
  
  // Allow long text strings: set display rectangle to 210 pixels. 
  SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, MAX_PATH); // 210
  }  // CreateToolTip

//-----------------------------------------------------------------------------
//
//                         ShowWinMouseClick
//
// Simulate a button mouse click 
//
void ShowWinMouseClick(HWND _hwndBtn)
  {
  POINT p, csav;

  GetCursorPos(&csav);           // Save surrent mouse cursor

  p.x = 0; p.y = 0;              // Set mouse pointer (normal=x0,y0)
  ClientToScreen(_hwndBtn, &p);  // Get pointer to button rectangle

  SetCursorPos(p.x, p.y);        // Point to upper left edge of button rectangle
  PostMessage(_hwndBtn, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y));
  // Simulate the mouse click at the designated button
  PostMessage(_hwndBtn, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
  PostMessage(_hwndBtn, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(p.x, p.y));
 
  SetCursorPos(csav.x, csav.y);  // Restore surrent mouse cursor
  } // ShowWinMouseClick

//-----------------------------------------------------------------------------
//
//                        PaintWindowBlack
//
// Paint the window rectangle black to mark it 'inactive'
// LTGRAY_BRUSH, WHITE_BRUSH, BLACK_BRUSH, ..
//
void PaintWindowBlack(HWND _hwnd)
  {
  RECT rcClient;
  PAINTSTRUCT ps;

  HDC hdc = BeginPaint(_hwnd, &ps);
  HDC hdcMem = CreateCompatibleDC(hdc);     
  GetClientRect(_hwnd, &rcClient);
  FillRect(hdc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
  DeleteDC(hdcMem);
  EndPaint(_hwnd, &ps);
  } // PaintWindowBlack

//-----------------------------------------------------------------------------
//
//                        GetActindex
//
int GetActindex(char* txtBuf, ULONG posBuf)
  {
  char tmpBuf[10];      
  ULONG _i;
  int _j;

  _i = posBuf;
  while (txtBuf[_i] != 0) _i--;  // Determine start of text item

  for (_j=0; _j < (strlen("\n1234. ")+1); _j++)
    {
    if (txtBuf[_i+1+_j] == '.') { tmpBuf[_j]=0; break; }
    tmpBuf[_j] = txtBuf[_i+1+_j];
    }
          
  return(atoi(tmpBuf));
  } // GetActindex

//-----------------------------------------------------------------------------
//
//                         AlgoTextSearch
//
int AlgoTextSearch(char* textPattern, char* textBuf, ULONG bufOffset)
  {
  char* tmpPtr = NULL;
  ULONG _i;
  int _j, _m = strlen(textPattern);

  // A loop to slide pat[] one by one
  for (_i=bufOffset; _i < bytesrd; _i++)
    {
    // For current index _i, check for pattern match
    for (_j=0; _j < _m; _j++)
      {
      if (tolower(textBuf[_i+_j]) != tolower(textPattern[_j])) break;
      }

    // If pattern matches at index _i
    if (_j == _m)
      {
      tmpPtr = &textBuf[_i];
      break;
      }
    } // end for

  txtOffset = _i;
  if (tmpPtr != NULL)
    {
    txtIndex = GetActindex(textBuf, txtOffset);
    sprintf(DebugBuf, "FOUND  TextIndex=%d", txtIndex);
    SetDlgItemText(hSearchMenu, IDC_TEXTFILE, DebugBuf);
    Sleep(100);
    }
  else
    {
    SetDlgItemText(hSearchMenu, IDC_TEXTFILE, "NOT FOUND.");
    Sleep(100);
    }

  return(txtIndex); 
  } // AlgoTextSearch

//*****************************************************************************
//
//                      DialogProcTextMenu (Modal Dialogs)
//
//  void DialogBoxW(
//    [in, optional]  hInstance,
//    [in]            lpTemplate,
//    [in, optional]  hWndParent,
//    [in, optional]  lpDialogFunc
//  );
//
//  INT_PTR DialogBoxParamW(
//    [in, optional] HINSTANCE hInstance,
//    [in]           LPCWSTR   lpTemplateName,
//    [in, optional] HWND      hWndParent,
//    [in, optional] DLGPROC   lpDialogFunc,
//    [in]           LPARAM    dwInitParam
//  );
//
// Usage (2 Dialogs):
// DialogBoxParam(hMainInstance, MAKEINTRESOURCE(DLG_SCRNSAVE_TEXTINDEX), hwnd,
//                DialogProcTextMenu, (LPARAM)DLG_SCRNSAVE_TEXTINDEX);
//
// DialogBoxParam(hMainInstance, MAKEINTRESOURCE(DLG_SCRNSAVE_TEXTSEARCH), hwnd,
//                DialogProcTextMenu, (LPARAM)DLG_SCRNSAVE_TEXTSEARCH);
//
INT_PTR CALLBACK DialogProcTextMenu(HWND _hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
  {
  int  _textLen;
  char _textBuf[200] = "Text";
  hSearchMenu = _hwnd;            // Needed globally

  switch(Message)
    {
    // DialogProcTextMenu
    // case WM_INITDIALOG:
    // Set up the modal dialog boxes
    case WM_INITDIALOG:
      _txtIndexSav = txtIndex;    // Save
      szTruncPath[25] = 0;        // Fit long filenames into dialogbox
      StrCat(szTruncPath, "...");
      // Get DLG_.. from 'haScrSav.rc' resource and initialize the entitled IDC_..
      // Set up the modal dialog boxes, and initialize any default values
      switch(LOWORD(lParam))  
        {                     
        case DLG_SCRNSAVE_TEXTINDEX:
          sprintf(DebugBuf, "File: %s", szTruncPath);  
          SetDlgItemText(_hwnd, IDC_TEXTFILE, DebugBuf);
          sprintf(DebugBuf, "This is text item number = %d.", txtIndex);  
          SetDlgItemText(_hwnd, IDC_TEXTINDEX, DebugBuf);
          sprintf(DebugBuf, "Enter a number between 1 and %d.", textMaxIndex);  
          SetDlgItemText(_hwnd, IDC_MAXINDEX, DebugBuf);
          sprintf(DebugBuf, "%d", txtIndex);  
          SetDlgItemText(_hwnd, IDC_EDITINDEX, DebugBuf);
          break;

        case DLG_SCRNSAVE_TEXTSEARCH:
          sprintf(DebugBuf, "File: %s", szTruncPath);  
          SetDlgItemText(_hwnd, IDC_TEXTFILE, DebugBuf);
          SetDlgItemText(_hwnd, IDC_EDITSEARCH, textDialogBuf);
          break;
        } // end switch
      break; // end case WM_INITDIALOG:

    // DialogProcTextMenu
    // case WM_COMMAND:
    // Serve the mouse click on a specific button
    case WM_COMMAND:
      switch(LOWORD(wParam))
        {
        case IDC_EDITINDEX:
          // When somebody clicks the INDEX button:
          // We get the number string entered
          _textLen = GetWindowTextLength(GetDlgItem(_hwnd, IDC_EDITINDEX));
          if (_textLen > 0)
            {
            // Get the number string into buffer, and convert into integer
            GetDlgItemText(_hwnd, IDC_EDITINDEX, _textBuf, _textLen+1);
            txtIndex = atoi(_textBuf);
            }
          if (txtIndex == 0) txtIndex = 1; 
          break;

        case IDC_EDITSEARCH:
          SetDlgItemText(hSearchMenu, IDC_TEXTSEARCH, "");
          // When somebody clicks the SEARCH button:
          // We get the string(s) entered
          _textLen = GetWindowTextLength(GetDlgItem(_hwnd, IDC_EDITSEARCH));
          if (_textLen > 0)
            {
            // Get the edited search pattern string into textDialogBuf
            GetDlgItemText(_hwnd, IDC_EDITSEARCH, textDialogBuf, _textLen+1);
            // Search the pattern and determine the txtIndex of the Text item.  
            txtIndex = AlgoTextSearch(textDialogBuf, pszTxtFilebuf, 0);
            if (txtIndex == 0) txtIndex++; 
            }
          break;

        case IDC_CONTINUE:
          // When somebody clicks the SEARCH button:
          // We get the string(s) entered
          _textLen = GetWindowTextLength(GetDlgItem(_hwnd, IDC_EDITSEARCH));
          if (_textLen > 0)
            {
            // Get the edited search pattern string into textDialogBuf
            GetDlgItemText(_hwnd, IDC_EDITSEARCH, textDialogBuf, _textLen+1);
            // Search the pattern and determine the txtIndex of the Text item.  
            txtIndex = AlgoTextSearch(textDialogBuf, pszTxtFilebuf, txtOffset+1);
            FillRect(hdcTextMenu, &rcTextMenu, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
            ScrSavDrawText(hTextMenu, txtIndex);  
            }
          break;

        case IDCANCEL:              // ATTENTION: "case IDCANCEL:" captures ESC key
        case IDC_CANCEL:
          txtIndex = _txtIndexSav;  // Restore from haSreensav.cpp
          EndDialog(_hwnd, 0);
          break;

        // ATTENTION: "case IDOK:" is needed to capture VK_RETURN        
        //  while the textbox has focus.       
        // Note: It is NOT necessary to define IDOK in the resource.rc.  
        // IDOK is always pre-defined and ready to be used appropriately.
        // IDOK can only appear here once. It will act on ALL    
        // dialog text boxes.                  
        case IDOK:
        case IDC_OK:
          EndDialog(_hwnd, 0);
          break; // end case IDC_OK:

        } // end switch(LOWORD(wParam))
      break; // end CASE WM_COMMAND:

    case WM_CLOSE:
      EndDialog(_hwnd, 0);
      break;

    default:
      return FALSE;
    } // end switch(Message)

  return TRUE;
  } // DialogProcTextMenu


//-----------------------------------------------------------------------------
//
//                     TextMenuProc (Registered Modal Dialog)
//
//  This is created as a registered dialog (see haScreensav.cpp):
//  hTextMenu = CreateDialog(GetModuleHandle(NULL), 
//                           MAKEINTRESOURCE(DLG_SCRNSAVE_TEXTMENU),
//                           hTextMenu, 
//                           TextMenuProc);
//
INT_PTR CALLBACK TextMenuProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
  {
  // Allow stepping with RETURTN and SPACE keys (SPACE=default)
  if ((GetAsyncKeyState(VK_RETURN) & 0x01) && 
       textModeFlag != MODE_INDEX          &&
       textModeFlag != MODE_SEARCH)
    {
    SendMessage(hwnd, WM_COMMAND, (WPARAM)_ID_TEXTRANDOM, MAKELPARAM(FALSE, 0));
    }
  // Abort with ESCAPE key
  else if (GetAsyncKeyState(VK_ESCAPE) & 0x01)
    SendMessage(hwnd, WM_COMMAND, (WPARAM)_ID_TEXTEXIT, MAKELPARAM(FALSE, 0));

  switch(Message)
    {
    case WM_INITDIALOG:
      {
      // Get handle to the icon IDI_HASCRABOUT and set the icon
      //  at the upper left corner of the title bar.
      HICON hIcon16x16 = (HICON)LoadIcon(hMainInstance,
                                         MAKEINTRESOURCE(IDI_HASCRABOUT));
      SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)(hIcon16x16));

      // Initailize text display window rectangle
      //  converted from dialog box resource units (=1105 x 615)
      //  to screen units (pixels) (=1658 x 999)
      // Calculation: =1658 x 999 (=1105*1.5 x 615*1.62)
      // Tested configuration Desktop rectangle (in pixels)
      //   =1680 x 1050 (maximum)
      //
      // Resizing Win32 Dialogs with DialogBox() and Dialog Resources:
      // One common user frustration is dialog boxes that cannot be resized.
      // Traditional Win32 dialogs are defined in resource scripts that define
      // the existence and placement of controls within the dialog.
      // A typical approach to implementing resizing, however,
      // involves writing code to alter the placement defined by the resource script.
      //
      // Because a resource dialog window unfortunately cannot be resized easily
      //  separate DIALOGEX must be provided in resource file
      //  for each specific monitor/desktop dimmensions!
      GetClientRect(hwnd, &rcTextMenu);    // MapDialogRect(..) not needed.
      // Start display area behind buttons
      rcTextMenu.left   += 100;          
      rcTextMenu.top    += 0;
      rcTextMenu.right  += 0; //=1458..3820 pixels
      rcTextMenu.bottom += 0; //= 999..2110 pixels

      hdcTextMenu = GetDC(hwnd);

      // Define area to clip mouse movement
      hTextRandomButton   = GetDlgItem(hwnd, _ID_TEXTRANDOM);
      hTextNextButton     = GetDlgItem(hwnd, _ID_TEXTNEXT);
      hTextPreviousButton = GetDlgItem(hwnd, _ID_TEXTPREVIOUS);
      GetClientRect(hTextRandomButton, &rcTextMenuButtons);
      hTextCurButton = hTextRandomButton;

      rcTextMenuButtons.left   += 20-8;                // Left edge of buttons
      rcTextMenuButtons.top    += 68+50-5;             // Upper edge of first button [Random]
      rcTextMenuButtons.right  += 20-8;                // Right edge of buttons
      rcTextMenuButtons.bottom += 68+212+((8-1)*14)-8; // Lower end of last button [Cancel]

      // srand() initially seeds the random-nr generator with the current time  
      // Global: Inject the random seed only once
      // (otherwise it dependents too much on time stamps)
      srand((unsigned)time(NULL));

      EnableWindow(GetDlgItem(hwnd,_ID_TEXTCOPY),   FALSE); // Initally grayed      
      EnableWindow(GetDlgItem(hwnd,_ID_TEXTINDEX),  FALSE); // not implemented      
      EnableWindow(GetDlgItem(hwnd,_ID_TEXTSEARCH), FALSE); // not implemented      

      // Destroy the pending previous tooltip and update the Quick-Info 
      DestroyWindow(hwndTT);      
      CreateToolTip(GetDlgItem(hwnd,_ID_TEXTCOPY),
                    "Copy the text to clipboard", NULL);
      break;
      } // case WM_INITDIALOG

    // All painting occurs here, between BeginPaint() and EndPaint().          
    case WM_PAINT:
      PaintWindowBlack(hwnd);    // Just a black main window on startup
      break;

    case WM_NCHITTEST:
      // Restrict the mouse move area (block main window for dragging)
      ClipCursor(&rcTextMenuButtons);
      break;

    // Serve the mouse click  on the specific buttons
    case WM_COMMAND:
      switch(LOWORD(wParam))
        {
        case _ID_TEXTNEXT:
          textModeFlag = MODE_NEXT;
          hTextCurButton = hTextNextButton;
          ClipCursor(&rcTextMenuButtons);
          // Clear previous text window, format the text
          //  and paint screen background as defined. 
          FillRect(hdcTextMenu, &rcTextMenu, (HBRUSH)GetStockObject(BLACK_BRUSH)); 

          if (txtIndex >= textMaxIndex) txtIndex = 0;  // re-init
          txtIndex++;                                  // increment
          ScrSavDrawText(hwnd, txtIndex);                        
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTCOPY),  TRUE); // allow clipboard-copy 
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTINDEX), TRUE);      
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTSEARCH),TRUE);      

          DestroyWindow(hwndTT);
          CreateToolTip(GetDlgItem(hwnd,_ID_TEXTNEXT),
                        "Click for next text", NULL);
          break;

        case _ID_TEXTPREVIOUS:
          textModeFlag = MODE_PREVIOUS;
          hTextCurButton = hTextPreviousButton;
          ClipCursor(&rcTextMenuButtons);
          // Clear previous text window, format the text
          //  and paint screen background as defined. 
          FillRect(hdcTextMenu, &rcTextMenu, (HBRUSH)GetStockObject(BLACK_BRUSH)); 

          if (txtIndex <= 1) txtIndex = textMaxIndex+1; // re-init
          txtIndex--;                                   // decrement
          ScrSavDrawText(hwnd, txtIndex);
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTCOPY),  TRUE); // allow clipboard-copy 
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTINDEX), TRUE);      
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTSEARCH),TRUE);      

          DestroyWindow(hwndTT);
          CreateToolTip(GetDlgItem(hwnd,_ID_TEXTPREVIOUS),
                        "Click for previous text", NULL);
          break;

        case _ID_TEXTRANDOM:
          textModeFlag = MODE_RANDOM;
          hTextCurButton = hTextRandomButton;
          ClipCursor(&rcTextMenuButtons);
          // Clear previous text window, format the text
          //  and paint screen background as defined. 
          FillRect(hdcTextMenu, &rcTextMenu, (HBRUSH)GetStockObject(BLACK_BRUSH)); 

          txtIndex = rand() % (textMaxIndex+1);         // default txtIndex=random
          if (txtIndex == 0) txtIndex++;                // Texts start with "1. "
          ScrSavDrawText(hwnd,txtIndex);                          
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTINDEX), TRUE);      
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTSEARCH),TRUE);      
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTCOPY),  TRUE); // allow clipboard-copy 

          DestroyWindow(hwndTT);
          CreateToolTip(GetDlgItem(hwnd,_ID_TEXTRANDOM),
                        "Click for a text", NULL);
          break;

        case _ID_TEXTINDEX:
          textModeFlag = MODE_INDEX;
          // Free mouse cursor clip
          // Simulate [Random] Button Mouseclick (activate default text mode),
          // such that when <SPACE> is pressed the help menu wont appear again.
          ClipCursor(FALSE);
          // Get txtIndex from dialog
          DialogBoxParam(hMainInstance, MAKEINTRESOURCE(DLG_SCRNSAVE_TEXTINDEX), hwnd,
                         DialogProcTextMenu, (LPARAM)DLG_SCRNSAVE_TEXTINDEX);
          if (txtIndex > textMaxIndex) txtIndex = 1;        // re-init
          else if (txtIndex == 0) txtIndex = textMaxIndex+1; // re-init

          FillRect(hdcTextMenu, &rcTextMenu, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
          ScrSavDrawText(hwnd, txtIndex);
          ShowWinMouseClick(hTextCurButton); 
          DestroyWindow(hwndTT);
          break;

        case _ID_TEXTSEARCH:
          textModeFlag = MODE_SEARCH;
          // Free mouse cursor clip
          // Simulate [Random] Button Mouseclick (activate default text mode),
          // such that when <SPACE> is pressed the help menu wont appear again.
          ClipCursor(FALSE);

          // Get text search pattern into textDialogBuf from dialog
          // Search pattern may be a word or text phrase
          DialogBoxParam(hMainInstance, MAKEINTRESOURCE(DLG_SCRNSAVE_TEXTSEARCH), hwnd,
                         DialogProcTextMenu, (LPARAM)DLG_SCRNSAVE_TEXTSEARCH);

          // Search the pattern and determine the txtIndex of the Text item.  
          //txtIndex = AlgoTextSearch(textDialogBuf, pszTxtFilebuf);
          SetDlgItemText(hSearchMenu, IDC_TEXTSEARCH, "FOUND");
          Sleep(250);

          FillRect(hdcTextMenu, &rcTextMenu, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
          ScrSavDrawText(hwnd, txtIndex);
          ShowWinMouseClick(hTextCurButton); 
          DestroyWindow(hwndTT);
          break;

        case _ID_TEXTCOPY:
          hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(pszString)+1);
          memcpy(GlobalLock(hMem), pszString, strlen(pszString)+1);
          GlobalUnlock(hMem);
          OpenClipboard(0);
          EmptyClipboard();
          SetClipboardData(CF_TEXT, hMem);
          CloseClipboard();
          // Simulate [Random] Button Mouseclick (activate default text mode),
          // such that when <SPACE> is pressed the help menu wont appear again.
          ShowWinMouseClick(hTextCurButton); 
          DestroyWindow(hwndTT);
          CreateToolTip(GetDlgItem(hwnd,_ID_TEXTCOPY), "    Text has been copied.    ", NULL);
          break;

        case _ID_TEXTCONFIG:
          ClipCursor(FALSE);                // Free mouse cursor clip
          ShowWindow(hTextMenu, SW_HIDE);   // Disable Text Menu window
          ShowWindow(hDlgSetup, SW_SHOW);   // Enable setup window   
          EnableWindow(GetDlgItem(hwnd,_ID_TEXTCOPY),   FALSE); // disable clipboard-copy 
          break;

        case _ID_TEXTHELP:
          ClipCursor(FALSE);                // Free mouse cursor clip
          CustomMessageBox(hwnd, 
                           "To read text items, mouse-click one of the buttons:\n"
                           "[Random], [Next] or [Previous].\n\n"
                           "NOTE:\n" 
                           "Windows 10 only, not supported on Windows 11:\n"
                           "  Instead of repetitively clicking a chosen button\n"
                           "      you may continue reading other text items\n"
                           "                      by pressing <SPACE>.\n\n"
                           "Use [Config] to access setup the dialog on-the-fly.\n"  
                           "Pressing <ESC> exits.\n\n"
                           "Run Screensav.EXE directly to reach this [Text Menu]",  
                           " haScreensav V2.0 - HELP", 
                           MB_OK, 
                           IDI_BE_SEEING_YOU);
          // Free mouse cursor clip
          ClipCursor(FALSE);  
          // Simulate [Random] Button Mouseclick (activate default text mode),
          // such that when <SPACE> is pressed the help menu wont appear again.
          ShowWinMouseClick(hTextCurButton); 
          break;

        case _ID_TEXTEXIT:
          ClipCursor(FALSE);                         //  Free mouse cursor clip
          EndDialog(hwnd, 0);
          hChooseColor = FindWindowA(NULL, "Farbe"); // Language=German
          DestroyWindow(hChooseColor); 
          hChooseColor = FindWindowA(NULL, "Color"); // Language=US
          DestroyWindow(hChooseColor);
          // Must exit properly via hDlg when in Windows Personalization.  
          SendMessage(hDlgSetup, WM_COMMAND, (WPARAM)ID_CANCEL, MAKELPARAM(FALSE, 0));
          return TRUE;
          break;

        case WM_CLOSE:
          ClipCursor(FALSE);               // Free mouse cursor clip
          EndDialog(hwnd, 0);
          GlobalFree(pszTxtFilebuf);
          GlobalFree(pszString);
          return TRUE;
          break;
        } // end switch(LOWORD(wParam))
      break;

    default:
      return FALSE;
    } // end switch(Message)

  return TRUE;
  } // TextMenuProc

//-----------------------------------------------------------------------------

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "FOUND\nTextIndex=%d  (TextOffset=%d)\ntextPattern=\x22%s\x22",
//ha//                          txtIndex,      txtOffset,      textDialogBuf);
//ha//MessageBoxA(NULL, DebugBuf, "AlgoTextSearch", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "textPattern=%s\nfound (_i=%d)\ntmpPtr=%s, txtIndex=%d",
//ha//                   textPattern,           _i,     tmpPtr,    txtIndex);
//ha//MessageBoxA(NULL, DebugBuf, "AlgoTextSearch_2", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---


