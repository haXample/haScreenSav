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
#define FAUST_MAXINDEX 1343

#define SYSERR_ABORT   250  //STATUS 250: Assembly aborted

#define DLG_SCRNSAVECONFIGURE 2003
#define ID_GRPBOX        101
#define ID_SPEED         102
#define ID_TXTFAST       103
#define ID_TXTSLOW       104
#define ID_OK            105
#define ID_CANCEL        106
#define ID_FONTCOLOR     107
#define ID_FONTSIZE      108
#define ID_FONTBOLD      109
#define ID_FONTITALIC    110
#define ID_FONTNAME      111
#define ID_TEXTFILE      112 
#define ID_TIMEDISPLAY   113 
#define ID_TEXTFIELD     114 

#define ID_BE_SEEING_YOU 120  // BMP image instead of ICON

#define IDS_INIFILE      200
#define IDS_APPNAME      201

#define IDC_STATIC        -1

#define IDT_TIMER1       300
#define IDT_TIMER2       301
#define IDT_TIMER3       302

#define MINVEL   1       // minimum redraw velocity (speed)     
#define MAXVEL  10       // maximum redraw speed value    
#define DEFVEL   5       // default redraw speed value    

#define _CYAN      0x00FFFF00  // Default text color; B=FF G=FF R=00
#define _FONTSIZE  22          // Default font size

//-----------------------------------------------------------------------------
