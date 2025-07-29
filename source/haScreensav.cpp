// haScreensav - Screensaver for windows 10, 11, ...
// haScreensav.cpp - C++ Developer source file.
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
char DebugBuf[2*MAX_PATH+1];               // Temporary buffer for formatted text
int DebugbufSize   = sizeof(DebugBuf);
char* psz_DebugBuf = DebugBuf;
TCHAR _tDebugBuf[2*MAX_PATH+1];            // Temp buffer for formatted UNICODE text
int _tDebugbufSize   = sizeof(_tDebugBuf);
TCHAR* psz_tDebugBuf = _tDebugBuf;

// IMPORTANT NOTE:
// All files accessed by screen savers must be located outside "C:\Windows\System32".
// For security reasons *.SCR may not access any files residing in "C:\Windows\System32".
// Example: Some private location that would work:
//          char* pszhaScrDfltFilename = "C:\\@ArcDrv\\Windows\\System32\\anyFile.frt";
//
// To ease installation "hascreenSav.SCR" has the necessary file contents integrated.  
// ..see haFaust.cpp
//
char* pszhaScrDfltFilename =        ".\\haFaust.frt";
char  szhaScrFilename[MAX_PATH+1] = "";

HWND hButtonTextFile;
HWND hwndTT;

char* pszhaScrDfltFontType = "DEFAULT_GUI_FONT";   // selected character font
char  szhaScrFontType[MAX_PATH+1];

CHAR szTemp[20];                               // temporary array of characters  

CHAR* pszIniFileKeySpeed   = "REDRAWSPEED";    // .ini: Name of the key belonging to section             
CHAR* pszIniFileKeyColor   = "TEXTCOLOR";      // .ini: Name of the key belonging to section             
CHAR* pszIniFileKeyFType   = "FONTTYPE";       // .ini: Name of the key belonging to section             
CHAR* pszIniFileKeyFSize   = "FONTSIZE";       // .ini: Name of the key belonging to section             
CHAR* pszIniFileKeyFStyle  = "FONTSTYLE";      // .ini: Name of the key belonging to section             
CHAR* pszIniFileKeyTime    = "TIMEFLAG";       // .ini: Name of the key belonging to section             
CHAR* pszIniFileKeyName    = "TEXTFILE";       // .ini: Name of the key belonging to section             

LONG  lSpeed           = _DEFVEL;  
int   fontSize         = _FONTSIZE16;
int   fontStyle        = _FONTSTYLESTD;
DWORD rgbColor         = _CYAN;                // initial color selection
int   timeFlag         = FALSE;              
int   fontWeight       = FW_NORMAL;
DWORD fontItalic       = FALSE;
char* pszhaScrFilename = szhaScrFilename;      // .FRT file location
char* pszhaScrFontType = szhaScrFontType;      // selected character font

COLORREF acrCustClr[16];       // array of custom colors 
CHOOSECOLOR cc;                // color palette dialog box structure 
                               
int _i, _j, _k;
int _stepFlag = FALSE; // Flag: Stepping through text file 
int _stepFlagCursor = FALSE; // Flag1: Stepping through text file 
  
UINT bufsizeF, bufsizeT;

HRESULT hr;
HBRUSH hbrush;         // brush handle
HFONT hFont, hFontTmp;

POINT cact, csav, pt;  // Mouse

HWND hDesktop;				 // Desktop window
HWND hTextStepButton;  // TextStep Button window
HWND hChooseColor;
HWND hwnd;             // owner window
HWND hSpeed;           // handle to speed scroll bar 
HWND hOK;              // handle to OK push button  
HDC  hdc;              // device-context handle  

// RECTANGLE structure
// typedef struct tagRECT {
//   LONG left;
//   LONG top;
//   LONG right;
//   LONG bottom;
// } RECT, *PRECT, *NPRECT, *LPRECT;
RECT rc;         
RECT rcTextStep;         

HMONITOR monitor;                      // Monitor geometrics
MONITORINFO info;
int monitor_width; 
int monitor_height;

static UINT uTimer1, uTimer2, uTimer3; // timer identifiers  
 
// Extern variables and functions
extern char* pszString;
extern char* pszTxtFilebuf;

extern void OpenTxtBuf();
extern void OpenTxtFile(char*);
extern void GetText();
extern void GetDate();
extern void errchk(char*, int);

extern void ScrSavDrawText(HWND);
extern void ScrSavSetupDrawFont(HWND);
extern BOOL OpenBrowserDialog();
extern BOOL DoRootFolder(WCHAR*);

// The following globals are already defined in scrnsave.lib
// extern HINSTANCE hMainInstance;              // screen saver instance handle
// extern HWND   hMainWindow;
// extern BOOL   fChildPreview;
// extern TCHAR  szName[TITLEBARNAMELEN];
// extern TCHAR  szAppName[APPNAMEBUFFERLEN];   // .ini file section string
// extern TCHAR  szIniFile[MAXFILELEN];         // .ini file name
// extern TCHAR  szScreenSaver[22];
// extern TCHAR  szHelpFile[MAXFILELEN];
// extern TCHAR  szNoHelpMemory[BUFFLEN];
// extern UINT   MyHelpMessage;

// Forward declaration of functions included in this code module:

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
//                        ScreenSaverConfigureDialog
//
void APIENTRY HandlePopupMenu(HWND _hwnd, POINT pt) 
  { 
  HMENU hmenu;            // menu template          
  HMENU hmenuTrackPopup;  // shortcut menu   

  //  Load the menu template containing the shortcut menu from the 
  //  application's resources. 
  hmenu = LoadMenu(hMainInstance, "PopupMenu"); 
  if (hmenu == NULL) return; 

  // Get the first shortcut menu in the menu template. This is the 
  // menu that TrackPopupMenu displays. 
  hmenuTrackPopup = GetSubMenu(hmenu, 0); 

  // TrackPopup uses screen coordinates, so convert the 
  // coordinates of the mouse click to screen coordinates, 
  // and adjust the placement of the tooltip 
  ClientToScreen(_hwnd, (LPPOINT)&pt);
  pt.x += 200+125;
  pt.y += 50;

  // Control the display of a checked icon in front of the text item
  CheckMenuItem(hmenuTrackPopup, IDM_FONT_DFLT,       MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONT_TIMESROMAN, MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONT_COURIER,    MF_BYCOMMAND | MF_UNCHECKED);
  if (StrCmpI(pszhaScrFontType, "DEFAULT_GUI_FONT") == 0)     
    CheckMenuItem(hmenuTrackPopup, IDM_FONT_DFLT,       MF_BYCOMMAND | MF_CHECKED);
  if (StrCmpI(pszhaScrFontType, "Times New Roman") == 0)     
    CheckMenuItem(hmenuTrackPopup, IDM_FONT_TIMESROMAN, MF_BYCOMMAND | MF_CHECKED);
  if (StrCmpI(pszhaScrFontType, "Courier New") == 0)     
    CheckMenuItem(hmenuTrackPopup, IDM_FONT_COURIER,    MF_BYCOMMAND | MF_CHECKED);

  CheckMenuItem(hmenuTrackPopup, IDM_FONTSTYLE_STANDARD, MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONTSTYLE_ITALIC,   MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONTSTYLE_BOLD,     MF_BYCOMMAND | MF_UNCHECKED);
  if (fontStyle == _FONTSTYLESTD)
    CheckMenuItem(hmenuTrackPopup, IDM_FONTSTYLE_STANDARD, MF_BYCOMMAND | MF_CHECKED);
  if (fontStyle & _FONTSTYLEITALIC)
    CheckMenuItem(hmenuTrackPopup, IDM_FONTSTYLE_ITALIC,   MF_BYCOMMAND | MF_CHECKED);
  if (fontStyle & _FONTSTYLEBOLD)
    CheckMenuItem(hmenuTrackPopup, IDM_FONTSTYLE_BOLD,     MF_BYCOMMAND | MF_CHECKED);

  CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_10, MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_12, MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_14, MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_16, MF_BYCOMMAND | MF_UNCHECKED);
  CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_18, MF_BYCOMMAND | MF_UNCHECKED);
  switch(fontSize)
    {
    case _FONTSIZE10:
      CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_10, MF_BYCOMMAND | MF_CHECKED);
      break;
    case _FONTSIZE12:
      CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_12, MF_BYCOMMAND | MF_CHECKED);
      break;
    case _FONTSIZE14:
      CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_14, MF_BYCOMMAND | MF_CHECKED);
      break;
    case _FONTSIZE16:
      CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_16, MF_BYCOMMAND | MF_CHECKED);
      break;
    case _FONTSIZE18:
      CheckMenuItem(hmenuTrackPopup, IDM_FONTSIZE_18, MF_BYCOMMAND | MF_CHECKED);
      break;
    default:
      break;
    } // end switch

  // Draw and track the shortcut menu.  
  TrackPopupMenu(hmenuTrackPopup, TPM_LEFTALIGN | TPM_LEFTBUTTON, 
                 pt.x, pt.y, 
                 0, 
                 _hwnd, NULL); 

  // Destroy the menu. 
  DestroyMenu(hmenu); 
  } // HandlePopupMenu

//-----------------------------------------------------------------------------
//
//                        ScreenSaverProc
//
// Handling Screen Savers
// 
// LIBS= KERNEL32.LIB USER32.LIB SHELL32.LIB \
//       GDI32.LIB COMCTL32.lib ADVAPI32.LIB scrnsave.lib
// 
// Following are some of the typical messages processed by "ScreenSaverProc".
// Message        Meaning
// WM_CREATE      Retrieve any initialization data from the Regedit.ini file.
//                Set a timer for the screen saver window. 
//                Perform any other required initialization.
// WM_ERASEBKGND  Erase the screen saver window and prepare for subsequent drawing operations.
// WM_TIMER       Perform drawing operations.
// WM_DESTROY     Destroy the timers created when the application processed the WM_CREATE message.
//                Perform any additional required cleanup.
// 
// ScreenSaverProc passes unprocessed messages to the screen saver library
//  by calling the DefScreenSaverProc function.
//  The following table describes how this function processes various messages.
// Message          Action
// WM_SETCURSOR     Set the cursor to the null cursor, removing it from the screen.
// WM_PAINT         Paint the screen background.
// WM_LBUTTONDOWN   Terminate the screen saver.
// WM_MBUTTONDOWN   Terminate the screen saver.
// WM_RBUTTONDOWN   Terminate the screen saver.
// WM_KEYDOWN       Terminate the screen saver.
// WM_MOUSEMOVE     Terminate the screen saver.
// WM_ACTIVATE      Terminate the screen saver if the wParam parameter is set to FALSE.
// 
// int LoadStringA(
//   [in, optional] HINSTANCE hInstance,
//   [in]           UINT      uID,          The identifier of the string to be loaded.
//   [out]          LPSTR     lpBuffer,     The buffer to receive the string.
//   [in]           int       cchBufferMax  The size of the buffer, in characters.
// );
//
// UINT GetPrivateProfileInt(
//   [in] LPCTSTR lpAppName,   The name of the section in the initialization file.
//   [in] LPCTSTR lpKeyName,   The name of the key whose value is to be retrieved.
//   [in] INT     nDefault,    The default value to return if key name cannot be found
//   [in] LPCTSTR lpFileName   The name of the initialization file.
// )
//
// DWORD GetPrivateProfileString(
//   [in]  LPCTSTR lpAppName,        The name of the section in the initialization file.
//   [in]  LPCTSTR lpKeyName,        The name of the key whose string is to be retrieved.
//   [in]  LPCTSTR lpDefault,        The default string to return if key name cannot be found
//   [out] LPTSTR  lpReturnedString, A pointer to the buffer that receives the retrieved string.
//   [in]  DWORD   nSize,            The size of the buffer pointed to by lpReturnedString.
//   [in]  LPCTSTR lpFileName        The name of the initialization file.
//);
//
LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam)
  {
  hdc = GetDC(hWnd);
   
  switch(message)
    {
    case WM_CREATE:
      // Get monitor geometrics
      monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
      info.cbSize = sizeof(MONITORINFO);
      GetMonitorInfo(monitor, &info);
      monitor_width  = info.rcMonitor.right  - info.rcMonitor.left;
      monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

      // Retrieve the application name (=section) from the .rc file. 
      // Adding error checking to verify LoadString success for both calls.
      if (LoadString(hMainInstance, IDS_APPNAME, szAppName, 80 * sizeof(TCHAR)) == 0)
        errchk("szAppName", ERROR_INVALID_PARAMETER);     
 
      // Retrieve the .ini (or registry) file name. 
      if (LoadString(hMainInstance, IDS_INIFILE, szIniFile, MAXFILELEN * sizeof(TCHAR)) == 0)
        errchk("szIniFile", ERROR_INVALID_PARAMETER);     
      
      // Retrieve any redraw speed data from the registry.  
      lSpeed    = GetPrivateProfileInt(szAppName, pszIniFileKeySpeed,  _DEFVEL, szIniFile); 
      rgbColor  = GetPrivateProfileInt(szAppName, pszIniFileKeyColor,  _CYAN,   szIniFile); 
      fontSize  = GetPrivateProfileInt(szAppName, pszIniFileKeyFSize,  22,      szIniFile); 
      fontStyle = GetPrivateProfileInt(szAppName, pszIniFileKeyFStyle, 0,       szIniFile);
      timeFlag  = GetPrivateProfileInt(szAppName, pszIniFileKeyTime,   FALSE,   szIniFile); 

      bufsizeF  = GetPrivateProfileString(szAppName, 
                                          pszIniFileKeyName, 
                                          pszhaScrDfltFilename, 
                                          pszhaScrFilename, 
                                          MAX_PATH, 
                                          szIniFile);

      bufsizeT  = GetPrivateProfileString(szAppName, 
                                          pszIniFileKeyFType, 
                                          pszhaScrDfltFontType, 
                                          pszhaScrFontType, 
                                          MAX_PATH, 
                                          szIniFile);

      // The timer interval is 1ms.
      // Set a timer for the screen saver window using the 
      // redraw rate (*1000ms = *1s) stored in Regedit.ini. 
      uTimer1 = SetTimer(hWnd, IDT_TIMER1, lSpeed*1000, NULL); 
      // Set a 30 seconds multi purpose timer  
      uTimer2 = SetTimer(hWnd, IDT_TIMER2, 30*1000, NULL); 
      GetDate();
      // Provide the text resource:
      // if no external file.FRT selected take the internal buffer (haScrFaust.cpp)
      if (StrCmpI(pszhaScrFilename, pszhaScrDfltFilename) == 0) OpenTxtBuf();       
      else OpenTxtFile(pszhaScrFilename);
      // Save current mouse position
      GetCursorPos(&csav);                           
      uTimer3 = SetTimer(hWnd, IDT_TIMER3, 300, NULL); 
      break;

    case WM_ERASEBKGND: 
      // The WM_ERASEBKGND message is issued (only once) 
      // before the WM_TIMER message, allowing the screen saver 
      // to paint the background as appropriate. 
      GetClientRect(hWnd, &rc); 
      FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
      ReleaseDC(hWnd, hdc); 
      break;
         
    case WM_TIMER:
      // The WM_TIMER message is issued at (lSpeed * 1000) 
      // intervals, where lSpeed == .001 seconds (=1ms). 
      switch(wParam)
        {
        case IDT_TIMER1:                         // IDT_TIMER1: Normal screen
        case IDT_TIMER3:                         // IDT_TIMER3: 1st screen directly after start
          if (uTimer3) KillTimer(hWnd, uTimer3); // Kill timer3 -
          uTimer3 = FALSE;                       //  1st screen is displayed only once.

          // Clear the previous text and paint screen background as appropriate. 
          GetClientRect(hWnd, &rc); 
          FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
          ScrSavDrawText(hWnd);  
 
          // Provide enough time to read the text
          for (_i=0; _i < (strlen(pszString) * lSpeed); _i++)
            {
            // Additional delay of 10ms/per text-character (sleep tics = 1ms)
            Sleep(10);
            // Abort if immediately if ESC-key has been pressed. 
            if (GetAsyncKeyState(VK_ESCAPE) != 0) break;
            // Abort if Mouse has been moved. 
            GetCursorPos(&cact);
            if (cact.x != csav.x || cact.y != csav.y) break;
            } // end for
          return 0;
          break;

        case IDT_TIMER2:  // Time display
          GetDate();
          return 0;
          break;
        } // end switch

    // Allow an extra time of 30s to read, if SPACE-key has been pressed. 
    // WM_KEYDOWN - Normally terminates the screen saver
    // If SPACE-key had been pressed, simulating a return from WM_TIMER,
    // will not terminate but continue the screen saver.
    case WM_KEYDOWN:
      if (GetAsyncKeyState(VK_SPACE) != 0)
        {
        Sleep(30000);
        // Discard pending SPACE-key (flush key buffer)
        while (GetAsyncKeyState(VK_SPACE) != 0) ;
        return DefScreenSaverProc(hWnd, WM_TIMER, wParam, lParam);
        } // end if (VK_SPACE)
      else
       {
       // Always ignore SPACE-key (do not exit screen saver on SPACE-key)
       while (GetAsyncKeyState(VK_SPACE) != 0) ;
       break;
       }

    case WM_DESTROY:
      // When the WM_DESTROY message is issued, the screen saver 
      // must destroy any of the timers that were set at WM_CREATE time. 
      if (uTimer1) KillTimer(hWnd, uTimer1);
      if (uTimer2) KillTimer(hWnd, uTimer2);
      GlobalFree(pszTxtFilebuf);
      GlobalFree(pszString);
      break;
    }

  // DefScreenSaverProc processes any messages ignored by ScreenSaverProc. 
  return DefScreenSaverProc(hWnd, message, wParam, lParam);
  } // ScreenSaverProc

//-----------------------------------------------------------------------------
//
//                        ScreenSaverConfigureDialog
//
//  The second required function in a screen saver module
//  displays a dialog box that enables the user to configure the screen saver
//  (an application must provide a corresponding dialog box template).
//  The system displays the configuration dialog box when the user selects
//  the Setup button in the Control Panel's Screen Saver dialog box.
// 
// int LoadStringA(
//   [in, optional] HINSTANCE hInstance,
//   [in]           UINT      uID,          The identifier of the string to be loaded.
//   [out]          LPSTR     lpBuffer,     The buffer to receive the string.
//   [in]           int       cchBufferMax  The size of the buffer, in characters.
// );
//
// UINT GetPrivateProfileInt(
//   [in] LPCTSTR lpAppName,   The name of the section in the initialization file.
//   [in] LPCTSTR lpKeyName,   The name of the key whose value is to be retrieved.
//   [in] INT     nDefault,    The default value to return if key name cannot be found
//   [in] LPCTSTR lpFileName   The name of the initialization file.
// )
//
// Copies a string into the specified section of an initialization file.
// BOOL WritePrivateProfileStringA(
//   [in] LPCSTR lpAppName,  The name of the section to which the string will be copied.
//   [in] LPCSTR lpKeyName,  The name of the key to be associated with a string.
//   [in] LPCSTR lpString,   A null-terminated string to be written to the file.
//   [in] LPCSTR lpFileName  The name of the initialization file.
// );
//
BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message,
                                       WPARAM wParam, LPARAM lParam)
  {
  // Get handle to the icon IDI_HASCRABOUT for the upper left corner of the title bar
  HICON hIcon16x16 = (HICON)LoadIcon(hMainInstance, MAKEINTRESOURCE(IDI_HASCRABOUT));

  switch(message)
    {
    case WM_INITDIALOG:
      // Set the icon IDI_HASCRABOUT at the upper left corner of the title bar
      SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)(hIcon16x16));
      // Retrieve the application name (=section) from the .rc file. 
      // Adding error checking to verify LoadString success for both calls.
      if (LoadString(hMainInstance, IDS_APPNAME, szAppName, 80 * sizeof(TCHAR)) == 0)
        errchk("  IDS_APPNAME, szAppName", ERROR_INVALID_PARAMETER);     
 
      // Retrieve the .ini (or registry) font size. 
      if (LoadString(hMainInstance, IDS_INIFILE, szIniFile, MAXFILELEN * sizeof(TCHAR)) == 0)
        errchk("  IDS_INIFILE, szIniFile", ERROR_INVALID_PARAMETER);     
      
      // Retrieve any redraw speed data.
      // Retrieve an integer associated with a key
      // in the specified section of an initialization file. 
      lSpeed    = GetPrivateProfileInt(szAppName, pszIniFileKeySpeed,  _DEFVEL, szIniFile); 
      rgbColor  = GetPrivateProfileInt(szAppName, pszIniFileKeyColor,  _CYAN,   szIniFile); 
      fontSize  = GetPrivateProfileInt(szAppName, pszIniFileKeyFSize,  22,      szIniFile); 
      fontStyle = GetPrivateProfileInt(szAppName, pszIniFileKeyFStyle, 0,       szIniFile); 
      timeFlag  = GetPrivateProfileInt(szAppName, pszIniFileKeyTime,   FALSE,   szIniFile);

      bufsizeT  = GetPrivateProfileString(szAppName, 
                                          pszIniFileKeyName, 
                                          pszhaScrDfltFilename, 
                                          pszhaScrFilename, 
                                          MAX_PATH, 
                                          szIniFile);

      bufsizeF  = GetPrivateProfileString(szAppName, 
                                          pszIniFileKeyFType, 
                                          pszhaScrDfltFontType, 
                                          pszhaScrFontType, 
                                          MAX_PATH, 
                                          szIniFile);

      // Create a Quick-Info (ToolTip) of the current filepath in use
      hButtonTextFile = GetDlgItem(hDlg, ID_TEXTFILE);
      CreateToolTip(hButtonTextFile,  pszhaScrFilename,  NULL);    //, TTS_BALLOON (ugly)

      // If the initialization file does not contain an entry 
      // for this screen saver, use the default value. 
      if (lSpeed > _MAXVEL || lSpeed < _MINVEL) lSpeed = _DEFVEL; 
 
      // Initialize the redraw speed scroll bar control.
      hSpeed = GetDlgItem(hDlg, ID_SPEED); 
      SetScrollRange(hSpeed, SB_CTL, _MINVEL, _MAXVEL, FALSE); 
      SetScrollPos(hSpeed, SB_CTL, lSpeed, TRUE); 
 
      // Initialize the radio button time display control.
      SendDlgItemMessage(hDlg, ID_TIMEDISPLAY, BM_SETCHECK, timeFlag, 0);

      // Get monitor geometrics and display info in setup dialog
      monitor = MonitorFromWindow(hDlg, MONITOR_DEFAULTTONEAREST);
      info.cbSize = sizeof(MONITORINFO);
      GetMonitorInfo(monitor, &info);
      //sprintf(DebugBuf, "Resolution: %d x %d", info.rcMonitor.right, info.rcMonitor.bottom);
      monitor_width  = info.rcMonitor.right  - info.rcMonitor.left;
      monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;
      sprintf(DebugBuf, "Resolution: %d x %d", monitor_width, monitor_height);
      SetDlgItemText(hDlg, ID_RESOLUTION, DebugBuf);   // Set monitor resolution

      // Retrieve a handle to the OK push button control.  
      hOK = GetDlgItem(hDlg, ID_OK); 
      return TRUE;

    case WM_HSCROLL: 
      // Process scroll bar input, adjusting the lSpeed 
      // value as appropriate. 
      switch (LOWORD(wParam)) 
        { 
        case SB_PAGEUP: 
          --lSpeed; 
          break; 

        case SB_LINEUP: 
          --lSpeed; 
          break; 

        case SB_PAGEDOWN: 
          ++lSpeed; 
          break; 

        case SB_LINEDOWN: 
          ++lSpeed; 
          break; 

        case SB_THUMBPOSITION: 
          lSpeed = HIWORD(wParam); 
          break; 

        case SB_BOTTOM: 
          lSpeed = _MINVEL; 
          break; 

        case SB_TOP: 
          lSpeed = _MAXVEL; 
          break; 

        case SB_THUMBTRACK: 
        case SB_ENDSCROLL: 
          return TRUE; 
          break;
        } // end switch

      if ((int)lSpeed <= _MINVEL) lSpeed = _MINVEL; 
      if ((int)lSpeed >= _MAXVEL) lSpeed = _MAXVEL;
      SetScrollPos((HWND)lParam, SB_CTL, lSpeed, TRUE); 
      break; 

    case WM_LBUTTONDOWN:
     break;

    case WM_MOUSEHOVER:
    case WM_MOUSEMOVE:
      // If Mouse has been moved, close and end the setup dialog
      GetCursorPos(&cact);
      if (_stepFlag && _stepFlagCursor>10 && (cact.x != csav.x || cact.y != csav.y))
        {
        SendMessage(hDlg, WM_COMMAND, (WPARAM)ID_CANCEL, MAKELPARAM(FALSE, 0));
       }
      _stepFlagCursor++;
      break;

    case WM_SYSKEYDOWN:
      if ((GetAsyncKeyState(VK_LMENU) & 0x01) && _stepFlag)
        {
        _stepFlag = FALSE;
        SendMessage(hDlg, WM_COMMAND, (WPARAM)ID_CANCEL, MAKELPARAM(FALSE, 0));
        }
      break;
    
    case WM_COMMAND:
      switch(LOWORD(wParam))
        {
        case ID_FONTCOLOR:
          //-----------------------------------------------------------------------------
          // 
          //                     ChooseColor
          //
          //  Choosing color via winsows Color Palette (select File menu  -> New)  
          //
          // Global vars
          // -----------
          // CHOOSECOLOR cc;                 // common dialog box structure 
          // HWND hwnd;                      // owner window
          // HBRUSH hbrush;                  // brush handle
          // static COLORREF acrCustClr[16]; // array of custom colors 
          // static DWORD rgbCurrent;        // initial color selection
          //
          // typedef struct tagCHOOSECOLORA {
          //   DWORD        lStructSize;
          //   HWND         hwndOwner;
          //   HWND         hInstance;
          //   COLORREF     rgbResult;
          //   COLORREF     *lpCustColors;
          //   DWORD        Flags;           //  CC_ENABLETEMPLATE | CC_FULLOPEN | CC_RGBINIT
          //   LPARAM       lCustData;
          //   LPCCHOOKPROC lpfnHook;
          //   LPCSTR       lpTemplateName;
          //   LPEDITMENU   lpEditInfo;
          // } CHOOSECOLORA,*LPCHOOSECOLORA;
          // 
          // Initialize CHOOSECOLOR structure
          ZeroMemory(&cc, sizeof(cc));
          cc.lStructSize  = sizeof(cc);
          cc.hwndOwner    = hwnd;
          cc.lpCustColors = (LPDWORD)acrCustClr;
          cc.rgbResult    = rgbColor;
          cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
           
          // Display example text box in setup dialog with current color
          ScrSavSetupDrawFont(hDlg);  
          EnableWindow(GetDlgItem(hDlg, ID_OK), FALSE);  // Disable (=Gray) OK Button
          if (ChooseColor(&cc) == TRUE) 
            {
            // COLORREF cc.rgbResult: value has the following hexadecimal form:
            // UINT 0x00bbggrr
            // Color constant (Example):
            // const COLORREF rgb:  Red = 0x000000FF;
            hbrush = CreateSolidBrush(cc.rgbResult);
            rgbColor = cc.rgbResult;
            } // ChooseColor

          // Display example text box in setup dialog with updated color
          ScrSavSetupDrawFont(hDlg);  
          EnableWindow(GetDlgItem(hDlg, ID_OK), TRUE);   // Enable OK Button
          break;

        case IDM_FONT_DFLT:
          pszhaScrFontType = "DEFAULT_GUI_FONT";
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONT_TIMESROMAN:
          pszhaScrFontType = "Times New Roman";
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONT_COURIER:
          pszhaScrFontType = "Courier New";
          ScrSavSetupDrawFont(hDlg);  
          break;

        case IDM_FONTSIZE_10:
          fontSize = _FONTSIZE10;    // =17 intern 
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONTSIZE_12:
          fontSize = _FONTSIZE12;    // =19 intern
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONTSIZE_14:
          fontSize = _FONTSIZE14;    // =21 intern
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONTSIZE_16:
          fontSize = _FONTSIZE16;    // =22 intern
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONTSIZE_18:
          fontSize = _FONTSIZE18;    // =27 intern
          ScrSavSetupDrawFont(hDlg);  
          break;

        // fontStyle:
        // 0x00 = Standard
        // 0x01 = Standard + Italic
        // 0x02 = Standard + Bold
        // 0x03 = Bold + Italic
        case IDM_FONTSTYLE_STANDARD:
          fontStyle  = _FONTSTYLESTD;
          fontWeight = FW_NORMAL;
          fontItalic = FALSE;
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONTSTYLE_ITALIC:
          fontStyle |= _FONTSTYLEITALIC;
          fontItalic = TRUE;
          if (fontStyle & _FONTSTYLEBOLD) fontWeight = FW_BOLD;
          else fontWeight = FW_NORMAL;
          ScrSavSetupDrawFont(hDlg);  
          break;
        case IDM_FONTSTYLE_BOLD:
          fontStyle |= _FONTSTYLEBOLD;
          fontWeight = FW_BOLD;
          if (fontStyle & _FONTSTYLEITALIC) fontItalic = TRUE;
          else fontItalic = FALSE;
          ScrSavSetupDrawFont(hDlg);  
          break;

        case ID_FONTSIZE:
          ScrSavSetupDrawFont(hDlg);  // Display font window
          HandlePopupMenu(hDlg, pt);  // Dropdown menu
          break;

        case ID_TEXTFILE:
          if (OpenBrowserDialog() == FALSE)
            DoRootFolder(NULL);       // Reset to the system root folder

          // Destroy the pending previous tooltip and update the Quick-Info 
          // with the currently valid filepath in use
          DestroyWindow(hwndTT);      
          CreateToolTip(hButtonTextFile, pszhaScrFilename, NULL);  //, TTS_BALLOON); (ugly)
          break;

        case ID_TEXTSTEP:
          _stepFlag = TRUE;								 // Activate mouse move monitoring
          GetCursorPos(&csav);						 // Save current cursor position at [Step] button.
          
          if (_stepFlagCursor == FALSE)
            {
            hTextStepButton = GetDlgItem(hDlg, ID_TEXTSTEP);
            GetClientRect(hTextStepButton, &rcTextStep); // The desktop window GetClientRect(hDesktop, &rc);
            rcTextStep.left   += csav.x;
            rcTextStep.top    += csav.y;
            rcTextStep.right  += csav.x;//+2;
            rcTextStep.bottom += csav.y;
            ClipCursor(&rcTextStep);
            _stepFlagCursor = TRUE;
            ShowCursor(FALSE);
            //while(ShowCursor(FALSE)>=0);
            }

          // Clear the previous text and paint screen background appropriately. 
          hDesktop = GetDesktopWindow();   // Get handle to desktop
          ::GetClientRect(hDesktop, &rcTextStep); // The desktop window GetClientRect(hDesktop, &rc);
          hdc = GetDC(hDesktop);
          // Format the text and paint screen background as defined. 
          FillRect(hdc, &rcTextStep, (HBRUSH)GetStockObject(BLACK_BRUSH)); 

          GetDate();
          // Provide the text resource:
          // if no external file.FRT selected take the internal buffer (haScrFaust.cpp)
          if (StrCmpI(pszhaScrFilename, pszhaScrDfltFilename) == 0) OpenTxtBuf();       
          else OpenTxtFile(pszhaScrFilename);
          GetText();
          ScrSavDrawText(hDesktop);
          break;

        case ID_TIMEDISPLAY:
          timeFlag ^= TRUE;                                                    // Toggle time display
          SendDlgItemMessage(hDlg, ID_TIMEDISPLAY, BM_SETCHECK, timeFlag, 0);  // hDlg = (HWND)lParam
          break;

        case ID_OK:
          // Write the current redraw speed variable to the .ini file. 
          hr = StringCchPrintf(szTemp, 20, "%ld", lSpeed);
          if (WritePrivateProfileString(szAppName, pszIniFileKeySpeed, szTemp, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     
          hr = StringCchPrintf(szTemp, 20, "%ld", rgbColor);
          if (WritePrivateProfileString(szAppName, pszIniFileKeyColor, szTemp, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     
          hr = StringCchPrintf(szTemp, 20, "%ld", fontSize);
          if (WritePrivateProfileString(szAppName, pszIniFileKeyFSize, szTemp, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     
          hr = StringCchPrintf(szTemp, 20, "%ld", fontStyle);
          if (WritePrivateProfileString(szAppName, pszIniFileKeyFStyle, szTemp, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     
          hr = StringCchPrintf(szTemp, 20, "%ld", timeFlag);
          if (WritePrivateProfileString(szAppName, pszIniFileKeyTime,  szTemp, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     

          if (WritePrivateProfileString(szAppName, pszIniFileKeyName,  pszhaScrFilename, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     
          if (WritePrivateProfileString(szAppName, pszIniFileKeyFType, pszhaScrFontType, szIniFile) == 0)
            errchk("  szIniFile", GetLastError());     
          // No break - Fall thru into case ID_CANCEL ..
         
        case ID_CANCEL:
          EndDialog(hDlg, LOWORD(wParam) == ID_OK);
          // Close any possibly open ChooseColor Dialog
          hChooseColor = FindWindowA(NULL, "Farbe");    // Language=German
          DestroyWindow(hChooseColor); 
          hChooseColor = FindWindowA(NULL, "Color");    // Language=US
          DestroyWindow(hChooseColor); 
          return TRUE;
          break;

        case WM_CLOSE:
          EndDialog(hDlg, 0);
          GlobalFree(pszTxtFilebuf);
          GlobalFree(pszString);
          return TRUE;
          break;
        } // end switch

    } // end switch

  return FALSE;
  } // ScreenSaverConfigureDialog

//-----------------------------------------------------------------------------
//
//                       RegisterDialogClasses
//
//  The third required function in a screen saver module:
//  In addition to the dialog box template and the ScreenSaverConfigureDialog,
//  an application must also support the RegisterDialogClasses function.
//  This function registers any nonstandard window classes required by the screen saver,
//  and must be called by ALL screen saver applications.
//
//  However, applications that do not require special windows or custom controls
//  in the configuration dialog box can simply return TRUE.
//  Applications requiring special windows or custom controls
//  should use this function to register the corresponding window classes.
// 
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
  {
  return TRUE;
  } // RegisterDialogClasses

// https://learn.microsoft.com/en-us/windows/win32/lwef/screen-saver-library
// Handling Screen Savers
// In addition to creating a module that supports the three functions just described,
//  a screen saver should supply an icon. This icon is visible only when the screen saver
//  is run as a standalone application.
//  (To be run by the Control Panel, a screen saver must have the .scr file name extension;
//  to be run as a standalone application, it must have the .exe file name extension.)
//  The icon must be identified in the screen saver's resource file by the constant ID_APP,
//  which is defined in the Scrnsave.h header file.
// 
// One final requirement is a screen saver description string.
//  The resource file for a screen saver must contain a string that the Control Panel displays
//  as the screen saver name. The description string must be the first string
//  in the resource file's string table (identified with the ordinal value 1).
//  However, the description string is ignored by the Control Panel
//  if the screen saver has a long filename. In such case,
//  the filename will be used as the description string.
// 
// Screen saver library
// The static screen saver functions are contained in the screen saver library.
//  There are two versions of the library available, Scrnsave.lib and Scrnsavw.lib.
//  You must link your project with one of these.
//  Scrnsave.lib is used for screen savers that use ANSI characters,
//  and Scrnsavw.lib is used for screen savers that use Unicode characters.
//  A screen saver that is linked with Scrnsavw.lib will only run on Windows
//  platforms that support Unicode,
//  while a screen saver linked with Scrnsave.lib will run on any Windows platform.
// 
//-----------------------------------------------------------------------------

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//MessageBoxA(NULL, "OpenTxtFile", "ScreenSaverConfigureDialog", MB_ICONINFORMATION | MB_OK);
//ha//OpenTxtFile(pszhaScrFilename); // DEBUG
//ha//MessageBoxA(NULL, "OpenTxtBuf", "ScreenSaverConfigureDialog", MB_ICONINFORMATION | MB_OK);
//ha//OpenTxtBuf(); // DEBUG
//ha////GetText();
//ha////sprintf(DebugBuf, "pszString=\n%s", pszString);
//ha////MessageBoxA(NULL, DebugBuf, "ScreenSaverConfigureDialog2", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha////ha//pszhaScrFilename = "haFaust.frt"; //TEST
//ha////ha//pszhaScrFontType = "Times";       //TEST
//ha////
//ha//sprintf(DebugBuf, "Filepath=%s\nFontType=\n%s", pszhaScrFilename, pszhaScrFontType);
//ha//MessageBoxA(NULL, DebugBuf, "ScreenSaverProc - WM_CREATE", MB_ICONINFORMATION | MB_OK);
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "Filepath=%s\nFontType=\n%s", pszhaScrFilename, pszhaScrFontType);
//ha//MessageBoxA(NULL, DebugBuf, "ScreenSaverProc - WM_CREATE", MB_ICONINFORMATION | MB_OK);
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//OpenTxtFile("haFaust.txt");
//ha//OpenTxtBuf();

//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "txtIndex=%X  pszTxtFilebuf=\n%s", txtIndex, pszTxtFilebuf);
//ha//MessageBoxA(NULL, DebugBuf, "-->ScreenSaverConfigureDialog_0<--", MB_ICONINFORMATION | MB_OK);
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//GetText();
//ha//sprintf(DebugBuf, "txtIndex=%d  pszString=\n%s", txtIndex, pszString);
//ha//MessageBoxA(NULL, DebugBuf, "ScreenSaverConfigureDialog", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "lSpeed=%d", lSpeed);
//ha//MessageBoxA(NULL, DebugBuf, "case WM_HSCROLL", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "h=%d", textHeight);
//ha//MessageBoxA(NULL, DebugBuf, "WM_TIMER", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "w=%d\nh=%d", monitor_width, monitor_height);
//ha//MessageBoxA(NULL, DebugBuf, "WM_ERASEBKGND", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "xR=%04d, yB=%04d, xl=%d, yT=%d\nmW=%d, mH=%d", _xRight, _yBottom, _xLeft, _yTop, monitor_width, monitor_height);  
//ha//DrawText(hdc, DebugBuf, strlen(DebugBuf), &rcStr, DT_LEFT | DT_EXTERNALLEADING | DT_WORDBREAK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "_binHexNr=%X\n_cnt=%d\n_ascDecNr=%s", _binHexNr, _cnt, _ascDecNr);
//ha//MessageBoxA(NULL, DebugBuf, "BinHex2AscDec", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "ascDecNrStr=%s\npszTxtFilebuf=%s",ascDecNrStr, pszTxtFilebuf);
//ha//MessageBoxA(NULL, DebugBuf, "GetText", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//OpenTxtFile(pszhaScrFilename);
//OpenTxtBuf();
//ha//sprintf(DebugBuf, "pszTxtFilebuf=%s ", pszTxtFilebuf);
//MessageBoxA(NULL, DebugBuf, "case WM_INITDIALOG", MB_ICONWARNING | MB_OK);
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

