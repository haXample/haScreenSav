// haScreensav - Screensaver for windows 10, 11, ...
// haScreensav.h - C++ Developer source file.
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

// You must define the constant used to identify the dialog box template
//  by using the decimal value 2003, as in the following example:
//
#define IDC_STATIC       -1
#define IDC_TEXTFILE   4009
#define IDC_TEXTINDEX  4010
#define IDC_MAXINDEX   4011
#define IDC_EDITINDEX  4012
#define IDC_TEXTSEARCH 4013
#define IDC_EDITSEARCH 4014
#define IDC_CONTINUE   4015
#define IDC_CANCEL     4016
#define IDC_OK         4017

#define MODE_RANDOM   0
#define MODE_NEXT     1
#define MODE_PREVIOUS 2
#define MODE_INDEX    3
#define MODE_SEARCH   4
#define MODE_MENU     5

#define RUN_MODE_SCR  1     // Currently runinng as screen saver
#define RUN_MODE_EXE  2     // Invoked the configuration setup

#define FAUST_MAXINDEX 1343

#define SYSERR_ABORT   250  //STATUS 250: Assembly aborted

#define DLG_SCRNSAVE_TEXTINDEX  2998
#define DLG_SCRNSAVE_TEXTSEARCH 2999

#define DLG_SCRNSAVE_TEXTMENU 3000
#define _ID_TEXTRANDOM        3001
#define _ID_TEXTNEXT          3002
#define _ID_TEXTPREVIOUS      3003
#define _ID_TEXTINDEX         3004
#define _ID_TEXTSEARCH        3005
#define _ID_TEXTCOPY          3006
#define _ID_TEXTCONFIG        3007
#define _ID_TEXTHELP          3008
#define _ID_TEXTEXIT          3009

#define DLG_SCRNSAVECONFIGURE 2003
#define ID_GRPBOX        101
#define ID_SPEED         102
#define ID_TXTFAST       103
#define ID_TXTSLOW       104
#define ID_OK            105
#define ID_CANCEL        106
#define ID_FONTCOLOR     107
#define ID_FONTTYPE      108
#define ID_FONTBOLD      109
#define ID_FONTITALIC    110
#define ID_TEXTFILE      112 
#define ID_TIMEDISPLAY   113 
#define ID_FILENAME      114 
#define ID_RESOLUTION    115
#define ID_TEXTMENU      116

#define ID_BE_SEEING_YOU  120  // BMP image instead of ICON
#define IDI_BE_SEEING_YOU 121  // ICON image 32x32
#define IDI_HASCRABOUT    122  // ICON image 16x16

#define IDS_INIFILE      200
#define IDS_APPNAME      201

#define IDC_STATIC        -1

#define IDT_TIMER1       300
#define IDT_TIMER2       301
#define IDT_TIMER3       302

#define IDM_FONT_FIXEDSYS      398
#define IDM_FONT_CONSOLAS      399
#define IDM_FONT_COURIER       400
#define IDM_FONT_TIMESROMAN    401
#define IDM_FONT_DFLT          403

#define IDM_FONTSIZE_10        404
#define IDM_FONTSIZE_12        405
#define IDM_FONTSIZE_14        406
#define IDM_FONTSIZE_16        407
#define IDM_FONTSIZE_18        408

#define IDM_FONTSTYLE_STANDARD 409
#define IDM_FONTSTYLE_BOLD     410
#define IDM_FONTSTYLE_ITALIC   411

#define _MINVEL      1          // minimum redraw velocity (speed)     
#define _MAXVEL     12          // 10 maximum redraw speed value    
#define _DEFVEL      6          //  5 default redraw speed value    

#define _CYAN       0x00FFFF00  // Default text color; B=FF G=FF R=00

#define _FONTSIZE10 17
#define _FONTSIZE12 19
#define _FONTSIZE14 21
#define _FONTSIZE16 23          // Default font size
#define _FONTSIZE18 27

#define _FONTSTYLESTD     0x00  // Default font style
#define _FONTSTYLEITALIC  0x01 
#define _FONTSTYLEBOLD    0x02 

#define MAX_TEXTSIZE   16000
                                                   
#define MAX_TEXTBLOCKS 10000    // Maximum number of text-blocks
typedef struct tagTEXTPR {      // TEXTPR structure for text-blocks
  int txtNr;                    // The number of the Text block.
  char* txtPtr;                 // The pointer to the text block.
} TEXTPR, *PTEXTPR, *LPTEXTPR;

// Workaround to prevent fail of 'SendMessage TTM_ADDTOOL'.
#if _WIN32_WINNT > 0x0500                                // Current Version Windows 10 = 1537
  #define SIZE_TOOLINFO sizeof(TOOLINFO) - sizeof(void*) // 44 bytes (TTTOOLINFOW_V2_SIZE)  
#else                                               
  #define SIZE_TOOLINFO sizeof(TOOLINFO)                 // 48 bytes (sizeof(TOOLINFO))
#endif                                                

//-----------------------------------------------------------------------------

