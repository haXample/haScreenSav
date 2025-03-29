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

// RECT structure for string to be displayed (drawn)
// typedef struct tagRECT {
//   LONG left;
//   LONG top;
//   LONG right;
//   LONG bottom;
// } RECT, *PRECT, *NPRECT, *LPRECT;
RECT rcStr;    

// Extern variables and functions
extern CHAR* pszhaScrFontType;

extern CHAR* pszString;

extern HDC  hdc;             // device-context handle  

extern HWND hWnd;            // owner window
extern HBRUSH hbrush;        // brush handle
extern HFONT hFont, hFontTmp;

// RECT structure for string to be displayed (drawn)
// typedef struct tagRECT {
//   LONG left;
//   LONG top;
//   LONG right;
//   LONG bottom;
// } RECT, *PRECT, *NPRECT, *LPRECT;
extern RECT rc;

extern DWORD rgbColor;        // initial color selection
extern COLORREF acrCustClr[]; // array of custom colors 
extern int fontWeight;
extern int fontSize;
extern int fontStyle;
extern DWORD fontItalic;

extern int monitor_width; 
extern int monitor_height;

extern int timeFlag;

extern void GetText();

//-----------------------------------------------------------------------------
//
//                        ScrSavDrawText
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
  
  GetText();
  
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
                               // Return text part in global pszString
  textHeight = DrawText(hdc, pszString, strlen(pszString), &rcStr, DT_CALCRECT);
  if (timeFlag) textHeight +=30;         // Adjust height for time display
  if (fontSize <= 22) textWidth  = 500;  // Estimated width of longest text string that can occur
  else textWidth  = 650;

  _xLeft   = randomX % monitor_width;
  _yTop    = randomY % monitor_height;
  _xRight  = randomX % monitor_width +textWidth;
  _yBottom = randomY % monitor_height+textHeight;

  if (_xLeft > monitor_width)  _xLeft = 100;
  if (_yTop  > monitor_height) _yTop  = 100;
  if (_xRight  > monitor_width)  {_xRight  -= textWidth;  _xLeft -= textWidth;}
  if (_yBottom > monitor_height) {_yBottom -= textHeight; _yTop -= textHeight;}

  SetRect(&rcStr, _xLeft, _yTop, _xRight, _yBottom);
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
