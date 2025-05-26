// haScreensav - Screensaver for windows 10, 11, ...
// haScrDraw.cpp - C++ Developer source file.
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

#include <stdlib.h>
#include <stdio.h>                        
#include <io.h>                                                        

#include <shlwapi.h>  // Library shlwapi.lib for PathFileExistsA
#include <commctrl.h> // Library Comctl32.lib
#include <commdlg.h>
#include <winuser.h>

#include <string.h>                                                 
#include <string>

#include <windows.h>
#include <scrnsave.h>

#include "hascreensav.h"

using namespace std;

// Global variables
CHAR* textSizeExample = "ABC abc 123\r\nöäü ,:' !?.";

int textHeight=300;
int textWidth =300;
int _xLeft, _yTop, _xRight, _yBottom; // Horizontal and Vertical position
int randomX, randomY;  

HDC  hdcStr;                 // device-context handle String 
HDC  hdcFont;                // device-context handle Font 

// RECT structure for the text Box drawn as the screensaver
// typedef struct tagRECT {
//   LONG left;
//   LONG top;
//   LONG right;
//   LONG bottom;
// } RECT, *PRECT, *NPRECT, *LPRECT;
RECT rcStr, rcDebug;    
int rcStrX, rcStrY; // The place where the text box actually pops up

// Extern variables and functions
extern LONG timeTxtHeight;
extern char DebugBuf[];             // Temporary buffer for formatted text
extern int DebugbufSize;
extern char* psz_DebugBuf;

extern CHAR* pszhaScrFontType;
extern CHAR* pszString;

extern HDC  hdc;             // device-context handle  
extern HWND hWnd;            // owner window
extern HBRUSH hbrush;        // brush handle
extern HFONT hFont, hFontTmp;

// RECT structure for text box drawn in the setuo dialog
// typedef struct tagRECT {
//   LONG left;
//   LONG top;
//   LONG right;
//   LONG bottom;
// } RECT, *PRECT, *NPRECT, *LPRECT;
extern RECT rc;
extern SIZE rcSize;           // size of time string

extern DWORD rgbColor;        // initial color selection
extern COLORREF acrCustClr[]; // array of custom colors 
extern int fontWeight;
extern int fontSize;
extern int fontStyle;
extern DWORD fontItalic;

extern int monitor_width;     // 1680
extern int monitor_height;    // 1050

extern int timeFlag;

extern void GetText();

//-----------------------------------------------------------------------------
//
//                        ScrSavDrawText
//
// BOOL SetRect(
//   [out] LPRECT lprc,     // Pointer RECT structure containing the rectangle to be set.
//   [in]  int    xLeft,    // x-coordinate of the rectangle's upper-left corner.
//   [in]  int    yTop,     // y-coordinate of the rectangle's upper-left corner.
//   [in]  int    xRight,   // x-coordinate of the rectangle's lower-right corner.
//   [in]  int    yBottom   // y-coordinate of the rectangle's lower-right corner.
// );
//
// RECT structure for string to be displayed (drawn)
// typedef struct tagRECT {
//   LONG left;             // x-coordinate of the rectangle's upper-left corner.
//   LONG top;              // y-coordinate of the rectangle's upper-left corner.
//   LONG right;            // x-coordinate of the rectangle's lower-right corner.
//   LONG bottom;           // y-coordinate of the rectangle's lower-right corner.
// } RECT, *PRECT, *NPRECT, *LPRECT;
//
// BOOL OffsetRect(         // OffsetRect moves the rectangle by the specified offsets.
//   [in, out] LPRECT lprc, // Pointer RECT structure containing the rectangle to be moved.
//   [in]      int    dx,   // Amount to move the rectangle left or right. Negative value moves to the left.
//   [in]      int    dy    // Amount to move the rectangle up or down. Negative value moves up.
// );
//
// The DrawText function draws formatted text in the specified rectangle.
//  It formats the text according to the specified method
//  (expanding tabs, justifying characters, breaking lines, and so forth).
// 
// int DrawText(
//   [in]      HDC     hdc,
//   [in, out] LPCTSTR lpchText,
//   [in]      int     cchText,
//   [in, out] LPRECT  lprc,
//   [in]      UINT    format
// );
// 
void ScrSavDrawText(HWND _hwnd)
  {
  hdc = GetDC(_hwnd);
  // Format the text and paint screen background as appropriate. 
  SetBkColor(hdc, RGB(0,0,0));
  SetTextColor(hdc, rgbColor);

  // HFONT hFont = CreateFont(
  //  int     cHeight,         // fontSize,   =22 27
  //  int     cWidth,          // 0,
  //  int     cEscapement,     // 0,
  //  int     cOrientation,    // 0,
  //  int     cWeight,         // FW_NORMAL,  =500  FW_MEDIUM FW_SEMIBOLD FW_LIGHT FW_BOLD
  //  DWORD   bItalic,         // FALSE,
  //  DWORD   bUnderline,      // FALSE,
  //  DWORD   bStrikeOut,      // FALSE,
  //  DWORD   iCharSet,        // DEFAULT_CHARSET,
  //  DWORD   iOutPrecision,   // OUT_DEFAULT_PRECIS,
  //  DWORD   iClipPrecision,  // CLIP_DEFAULT_PRECIS,
  //  DWORD   iQuality,        // PROOF_QUALITY
  //  DWORD   iPitchAndFamily, // DEFAULT_PITCH | FF_DONTCARE,
  //  LPCWSTR pszFaceName);    // "DEFAULT_GUI_FONT"
  //
  hFont = CreateFont(fontSize, 0,0,0, fontWeight, fontItalic,0,0,0,0,0, PROOF_QUALITY, 0, pszhaScrFontType);
  hFontTmp = (HFONT)SelectObject(hdc, hFont);

  randomX = rand();
  randomY = rand();
  
  GetText();   // Return text part in global pszString
  
  // Display a random text part of the formatted text resource
  // fontStyle:
  // 0x00 = Standard
  // 0x01 = Standard + Italic
  // 0x02 = Standard + Bold
  // 0x03 = Bold + Italic
  switch(fontStyle)
    {
    case 0:
      break;
      fontWeight = FW_NORMAL;
      fontItalic = FALSE;
    case 1:
      fontWeight = FW_NORMAL;
      fontItalic = TRUE;
      break;
    case 2:
      fontWeight = FW_BOLD;
      fontItalic = FALSE;
      break;
    case 3:
      fontWeight = FW_BOLD;
      fontItalic = TRUE;
      break;
    } // end switch
  
  // Init textbox at upper left corner (rcStr.right: see DT_CALCRECT)
  SetRect(&rcStr, 0, 0, 0, textHeight); 

  // DT_CALCRECT Determines the width and height of the rectangle.
  //             If there are multiple lines of text, 
  //             DrawText uses the width of the rectangle pointed to by the lpRect parameter
  //             and extends the base of the rectangle to bound the last line of text.
  //             If the largest word is wider than the rectangle, the width is expanded.
  //             If the text is less than the width of the rectangle,
  //             the width is reduced. If there is only one line of text,
  //             DrawText modifies the right side of the rectangle so that it bounds
  //             the last character in the line.
  //             In either case, DrawText returns the height of the formatted text
  //             but does not draw the text. 
  // 
  textHeight = DrawText(hdc, pszString, strlen(pszString), &rcStr, DT_CALCRECT);
  if (timeFlag) textHeight += (3 * rcSize.cy);        // Adjust height for time display

  SetRect(&rcStr, 0, 0, rcStr.right, textHeight);     // Update textbox height
 
  rcStrX = (randomX % monitor_width);           // Get random position {X,Y}
  rcStrY = (randomY % monitor_height);

  if (rcStrX < 100) rcStrX = 100;                // Upper left {100,100}
  if (rcStrY < 100) rcStrY = 100;
  if (rcStrX > (monitor_width -rcStr.right))  rcStrX  = (monitor_width-rcStr.right-100);
  if (rcStrY > (monitor_height-textHeight))   rcStrY  = 100; // Reset to top
  
  OffsetRect(&rcStr, rcStrX, rcStrY);           // Move textbox randomly 
  DrawText(hdc, pszString, strlen(pszString), &rcStr, DT_LEFT | DT_EXTERNALLEADING | DT_WORDBREAK);
  //DeleteObject(SelectObject(hdc, hFontTmp));
  ReleaseDC(_hwnd, hdc);
  } // ScrSavDrawText


//-----------------------------------------------------------------------------
//
//                        ScrSavSetupDrawFont
//
void ScrSavSetupDrawFont(HWND _hDlg)
  {
  // Display a random text part of the formatted text resource
  // fontStyle:
  // 0x00 = Standard
  // 0x01 = Standard + Italic
  // 0x02 = Standard + Bold
  // 0x03 = Bold + Italic
  switch(fontStyle)
    {
    case 0:
      break;
      fontWeight = FW_NORMAL;
      fontItalic = FALSE;
    case 1:
      fontWeight = FW_NORMAL;
      fontItalic = TRUE;
      break;
    case 2:
      fontWeight = FW_BOLD;
      fontItalic = FALSE;
      break;
    case 3:
      fontWeight = FW_BOLD;
      fontItalic = TRUE;
      break;
    } // end switch

  hdc = GetDC(_hDlg);
  // Format the text and paint screen background as defined. 
  hFont = CreateFont(fontSize, 0,0,0, fontWeight, fontItalic,0,0,0,0,0, PROOF_QUALITY, 0, pszhaScrFontType);
  hFontTmp = (HFONT)SelectObject(hdc, hFont);
  SetBkColor(hdc, RGB(0,0,0));
  SetTextColor(hdc, rgbColor);

  GetClientRect(_hDlg, &rc);
  // create a black box 
  SetRect(&rc, rc.left+78+19, rc.top+85, rc.right-85, rc.bottom-16);
  FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
  // place text in black box
  SetRect(&rc, rc.left, rc.top+5, rc.right, rc.bottom);
  DrawText(hdc, textSizeExample, strlen(textSizeExample), &rc, DT_LEFT | DT_EXTERNALLEADING | DT_WORDBREAK);
  DeleteObject(SelectObject(hdc, hFontTmp));
  ReleaseDC(_hDlg, hdc);
  } // ScrSavSetupDrawFont

//-----------------------------------------------------------------------------

//ha/////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//textHeight +=100;
//ha//sprintf(DebugBuf, "\n_xLeft= %d  _yTop=   %d\n_xRight=%d  _yBottom=%d", 
//ha//                     _xLeft,     _yTop,       _xRight,    _yBottom);
//ha//StrCat(pszString, DebugBuf);  // append current time info
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "_xLeft=%d  _yTop=%d _xRight=%d  _yBottom=%d", 
//ha//                   _xLeft,    _yTop,   _xRight,     _yBottom);
//ha//
//ha//SetRect(&rcDebug, 50, 50, 600, 600);
//ha//DrawText(hdc, DebugBuf, strlen(DebugBuf), &rcDebug, DT_LEFT | DT_EXTERNALLEADING | DT_WORDBREAK);
//ha//
//ha////StrCat(pszString, DebugBuf);  // append current time info
//ha////ha//while ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) == 0) ; //break;
//ha////ha//while ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) ; //break;
//ha////Sleep(1000);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
