// haScreensav - Screensaver for windows 10, 11, ...
// haScrTxtFile.cpp - C++ Developer source file.
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
#include <fcntl.h>   // Console
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
char* pszTxtFilebuf  = NULL;       // Total text size
char* pszString      = NULL;       // Text-block  to be displayed

char lh_date[10+1];                // = "dd/mm/yyyy"   
char lh_time[8+1];                 // = "hh:mm.ss"   

// TEXTPR structure
// typedef struct tagTEXTPR {      // Maximum number of text-blocks
//    int txtNr;                   // TEXTPR structure for text-blocks
//    char* txtPtr;                // The number of the Text block.
// } TEXTPR, *PTEXTPR, *LPTEXTPR;  // The pointer to the text block.
TEXTPR txtPtrArray[MAX_TEXTBLOCKS];

int textMaxIndex = FAUST_MAXINDEX;
int txtIndex=0;
int badFormat=0;

int fhTxt=0;                       // Filehandle read (*.TXT)

ULONG bytesrd;                     // Textfile number of byte read

// SIZE structure
// typedef struct tagSIZE {
//   LONG cx;                      // Specifies the rectangle's width.
//   LONG cy;                      // Specifies the rectangle's height.
// } SIZE, *PSIZE, *LPSIZE;
SIZE rcSize;
  
// Extern variables and functions
extern char DebugBuf[];            // Temporary buffer for formatted text
extern int DebugbufSize;
extern char* psz_DebugBuf;
extern TCHAR _tDebugBuf[];         // Temp buffer for formatted UNICODE text
extern int _tDebugbufSize;
extern TCHAR* psz_tDebugBuf;

extern char* pszhaScrFilename;      // .FRT file

extern char haFaust_frt01[]; 
extern char haFaust_frt02[]; 
extern char haFaust_frt03[]; 

extern int haFaust_frt01size;
extern int haFaust_frt02size;
extern int haFaust_frt03size;

extern int haFaust_frtsize;
extern int timeFlag;

extern HDC  hdc;                    // device-context handle  

// Centered Messagebox within parent window
extern int CBTCustomMessageBox(HWND, char*, char*, UINT, UINT);
extern int AlgoTextSearch(char* textPattern, char* textBuf, ULONG bufOffset);

// Forward declaration of functions included in this code module:
void GetDate();
int GetLastindex();
void BuildTxtPtrArray();

//-----------------------------------------------------------------------------
//
//                      errchk                    
//
// Check System Error Codes (0-499) (0x0-0x1f3)
// int _lastErr = GetLastError();
//
// The following uses some of system error codes defined in 'WinError.h'.
//
void errchk(char* _filename, int _lastErr)
  {
  char szSystemError[]       = "|SYSTEM ERROR|";
  char szErrorOpenFailed[]   = "Open failed";
  char szErrorNotReady[]     = "Device not ready";
  char szErrorFileNotFound[] = "File not found";
  char szErrorPathNotFound[] = "Path not found";
  char szErrorFileIsUsed[]   = "File is being used by another process.";
  char szErrorAccessDenied[] = "Access denied";         
  char szErrorFileExists[]   = "File already exists.";
  char szErrorFileWrite[]    = "File write failed.";
  char szErrorFileRead[]     = "File read failed.";
  char szErrorInvalidParam[] = "Invalid Parameter";
  char szErrorInvalidBuf[]   = "Supplied buffer is not valid.";
  char szErrorBadFormat[]    = "FRT-FORMAT ERROR.\nIncorrect index at";
  char szAbort[]             = "-- ABORT --";

  int exitCode = 0;                 // Initially allow retry

  if (_lastErr != 0)                      
    {
    switch(_lastErr)
      {
      case ERROR_INVALID_USER_BUFFER:
        sprintf(DebugBuf, "%s\n%s", szErrorInvalidBuf, _filename);
        break;
      case ERROR_ALREADY_EXISTS:    // 0x02
        sprintf(DebugBuf, "%s\n%s", szErrorFileExists, _filename);
        break;
      case ERROR_FILE_NOT_FOUND:    // 0x02
        sprintf(DebugBuf, "%s\n%s", szErrorFileNotFound, _filename);
        break;
      case ERROR_PATH_NOT_FOUND:    // 0x03
      case ERROR_BAD_NETPATH:       // 0x35
        sprintf(DebugBuf, "%s\n%s", szErrorPathNotFound, _filename);
        break;
      case ERROR_ACCESS_DENIED:     // 0x05
        sprintf(DebugBuf, "%s\n%s", szErrorAccessDenied, _filename);
        break;
      case ERROR_BAD_FORMAT:
        badFormat=txtIndex;        
        sprintf(DebugBuf, "%s [%d.]\n\nFile:\n%s\n\nSelect [Text Menu] to re-number.", szErrorBadFormat, txtIndex, _filename);
        break;
      case ERROR_SHARING_VIOLATION: // 0x20
        sprintf(DebugBuf, "%s\n%s", szErrorFileIsUsed, _filename);
        break;
      case ERROR_OPEN_FAILED:       // 0x6E
        sprintf(DebugBuf, "%s\n%s", szErrorOpenFailed, _filename);
        break;
      case ERROR_INVALID_PARAMETER: 
        sprintf(DebugBuf, "%s\n%s", szErrorInvalidParam, _filename);
        break;
      case ERROR_WRITE_PROTECT:     // 0x13
      case ERROR_WRITE_FAULT:       // 0x1D
      case ERROR_NET_WRITE_FAULT:   // 0x58
        sprintf(DebugBuf, "%s\n%s\n\n%s", szErrorFileWrite, _filename, szAbort);
        exitCode = SYSERR_ABORT;
        break;
      case ERROR_NOT_READY:         // 0x15
        sprintf(DebugBuf, "%s\n%s\n\n%s", szErrorNotReady, _filename, szAbort);
        exitCode = SYSERR_ABORT;
        break;                                                                                                 
      case ERROR_READ_FAULT:        // 0x1E
        sprintf(DebugBuf, "%s\n%s\n\n%s", szErrorFileRead, _filename, szAbort);
        exitCode = SYSERR_ABORT;
        break;
      default:                      // any other system error number
        sprintf(DebugBuf, "LastErrorCode = 0x%08X [%d]\n\n%s", _lastErr, _lastErr, szAbort);
        exitCode = SYSERR_ABORT;
        break;
      } // end switch

    // Display centered MessageBox
    CBTCustomMessageBox(NULL,  DebugBuf, _filename, MB_OK, IDI_BE_SEEING_YOU);
    // Exit-Abort to system 
    if (exitCode == SYSERR_ABORT) exit(SYSERR_ABORT);
    } // end if
  } // errchk


//-----------------------------------------------------------------------------
//
//                        GetLastindex
//
int GetLastindex()
  {
  char tmpBuf[10];      
  char* tmpPtr = NULL;
  long _i;
  int _j;

  textMaxIndex = 0;
  for (_i=bytesrd; _i>0; _i--)
    {
    if (pszTxtFilebuf[_i] == '\x0A'  &&  // "\n1234. "   is an indexnr
        pszTxtFilebuf[_i+1] != ' '   &&  // "\n  1234. " is not an indexnr
        _i < bytesrd-sizeof(tmpBuf))
      {
      // textMaxIndex = [\n9999. ]
      for (_j=0; _j<strlen("\n1234. "); _j++) tmpBuf[_j] = pszTxtFilebuf[_i+_j];
          
      if ((tmpPtr = strstr(tmpBuf, ". ")) != NULL     &&
          (*(tmpPtr-1) >= '0' && *(tmpPtr-1) <= '9'))
        { 
        *tmpPtr = 0;
        textMaxIndex = atoi(&tmpBuf[1]);
        break;
        }
      } // end if ('\x0A')
    } // end for

  return(textMaxIndex);
  } // GetLastindex


//-----------------------------------------------------------------------------
//
//                      OpenTxtFile
//
// Open input text file: *.FRT as CRLF-terminated text
//
// Open as binary filehandle
//  struct _stat Stat;
//  _stat(TxtFileSize, &Stat);     // Get File info structure
//  TxtFileSize = Stat.st_size     // file size (for malloc)
//
//  if ((fhTxt = open(_filename, O_RDONLY|O_BINARY)) == ERR)
//    {
//    printf(szErrorOpenFailed, _filename);
//    exit(SYSERR_FROPN);
//    }
// System errors cause a program abort
//
void OpenTxtFile(char* _filename)
  {
  // ------------------
  // File handle method
  // ------------------
  struct _stat Stat;
  ULONG TxtFileSize;

  // Open as binary filehandle
  fhTxt = open(_filename, O_RDONLY|O_BINARY);
  errchk(_filename, GetLastError());     
 
  _stat(_filename, &Stat);     // Get File info structure
  TxtFileSize = Stat.st_size;  // file size (for malloc)

  pszTxtFilebuf = (char *)GlobalAlloc(GPTR, TxtFileSize+4);
  errchk(_filename, GetLastError());     

  bytesrd = read(fhTxt, pszTxtFilebuf, TxtFileSize);
  errchk(_filename, GetLastError());     
  pszTxtFilebuf[bytesrd] = 0;

  close(fhTxt);
 
  // Final format check of file contents (file.FRT) at runtime
  if (StrCmpN(pszTxtFilebuf, "1. ", 3) != 0) 
    errchk(_filename, ERROR_BAD_FORMAT);

  BuildTxtPtrArray();
  } // OpenTxtFile

//-----------------------------------------------------------------------------
//
//                      OpenTxtBuf
//
// Fill input text file buffer from FAUST.FRT as CRLF-terminated text array
//
void OpenTxtBuf()
  {
  long _i, _j;

  // Allocate Buffer
  pszTxtFilebuf = (char *)GlobalAlloc(GPTR, haFaust_frtsize+4);

  _j=0;
  for (_i=0; _i<(haFaust_frt01size-1); _i++)  // copy without 0-terminator
    pszTxtFilebuf[_i]    =  haFaust_frt01[_i];

  _j+=_i;
  for (_i=0; _i<(haFaust_frt02size-1); _i++)  // copy without 0-terminator
    pszTxtFilebuf[_j+_i] = haFaust_frt02[_i];

  _j+=_i;
  for (_i=0; _i<haFaust_frt03size; _i++)      // copy with 0-terminator
    pszTxtFilebuf[_j+_i] = haFaust_frt03[_i];

  bytesrd = haFaust_frtsize;                  // reflect intrinsic FAUST textsize
  BuildTxtPtrArray();
  } // OpenTxtBuf

//-----------------------------------------------------------------------------
//
//                      BuildTxtPtrArray
//
// Build an array containing pointers to the coresponding text numbers
// to allow fast random access to text blocks. 
//
void BuildTxtPtrArray()
  {                                             
  int _i, _j;
  char ascDecNrStr[10];                        // "10000. \x0"
  char* tmpPtr = NULL;
  
  GetLastindex();                              // =textMaxIndex
                                  
  _i=0; txtIndex = 1;                          // Text numbers start with 1
  while (_i < bytesrd)
    {
    sprintf(ascDecNrStr, "%d. ", txtIndex);
    if ((tmpPtr=strstr(&pszTxtFilebuf[_i], ascDecNrStr)) != NULL &&
         *tmpPtr+2 > ' ')
      {
      if (txtIndex > 1) *(tmpPtr-1) = 0;       // 0-terminate previous text block

      // Format index check
      // Check if next index is same as previous (double indexes)
      sprintf(ascDecNrStr, "\xA%d. ", txtIndex);  
      if (strstr(tmpPtr, ascDecNrStr) != NULL) 
        { errchk(pszhaScrFilename, ERROR_BAD_FORMAT); break; }

      // Find start of current text (skip txtIndex)
      tmpPtr = strstr(tmpPtr, ". ");
      tmpPtr += 2;

      txtPtrArray[txtIndex].txtNr  = txtIndex; // Text number
      txtPtrArray[txtIndex].txtPtr = tmpPtr;   // Start of current text

      txtIndex++;                              // advance to next text block
      if (txtIndex > textMaxIndex) break;      // stop if no more text
      
      // Format index check
      // Check if next index is not increment of previous (illegal indexes)
      sprintf(ascDecNrStr, "\xA%d. ", txtIndex);  
      if (strstr(tmpPtr, ascDecNrStr) == NULL) 
        { errchk(pszhaScrFilename, ERROR_BAD_FORMAT); break; }
      }
    _i++;                                      // advance buffer index
    } // end while

  txtIndex = 0;                                // Init-clear
  } // BuildTxtPtrArray

//-----------------------------------------------------------------------------
//
//                        GetText
//
// Get random text from selected formatted *.FRT text  
//
void GetText(int _txtIndex)
  {
  char ascDecNrStr[11];
  char* tmpPtr = NULL;
  ULONG _i;                 // must be local here

  // Max size of a Textblock
  pszString = (char *)GlobalAlloc(GPTR, MAX_TEXTSIZE);  

//ha//_txtIndex=369;                    // DEBUG TEST see misc1.frt "369. "
//ha//_txtIndex++;                      // DEBUG TEST 
//ha//_txtIndex=textMaxIndex;           // DEBUG TEST 
//ha//_txtIndex %= (textMaxIndex+1);    // DEBUG TEST 
//ha//_txtIndex = rand() % 9;           // DEBUG TEST

  if (_txtIndex <= 0 || _txtIndex > textMaxIndex) _txtIndex = 1;
  tmpPtr = txtPtrArray[_txtIndex].txtPtr;

  // Discard any double CRLF and 0-terminate text
  if (tmpPtr[strlen(tmpPtr)-2] == '\x0A' &&
      tmpPtr[strlen(tmpPtr)-4] == '\x0A') tmpPtr[strlen(tmpPtr)-4] = 0;
  else if (tmpPtr[strlen(tmpPtr)-2] == '\x0A') tmpPtr[strlen(tmpPtr)-2] = 0;                     
  else if (txtIndex == textMaxIndex && 
           tmpPtr[strlen(tmpPtr)-1] == '\x0A') tmpPtr[strlen(tmpPtr)-1] = 0;                     

  StrCpy(pszString, tmpPtr); 
  if (timeFlag) StrCat(pszString, lh_time);     // append current time info
  } // GetText

//-----------------------------------------------------------------------------
//
//                      GetDate
//
// Retrieves the system's local date/time and emits date and time as strings.
//
// The GetTextExtentPoint32 function computes the width and height
//  of the specified string of text.
// 
// BOOL GetTextExtentPoint32A(
//   [in]  HDC    hdc,      // A handle to the device context.
//   [in]  LPCSTR lpString, // A pointer to a buffer that specifies the text string.
//                          //  The string does not need to be null-terminated,
//                          //  because the c parameter specifies the str length.
//   [in]  int    c,        // The length of the string pointed to by lpString.
//   [out] LPSIZE psizl     // A pointer to a SIZE structure that receives
// );                       // the dimensions of the string, in logical units.
// 
// // SIZE structure
// typedef struct tagSIZE {
//   LONG cx;               // Specifies the rectangle's width.
//   LONG cy;               // Specifies the rectangle's height.
// } SIZE, *PSIZE, *LPSIZE;
// SIZE rcXY;
//  
// typedef struct _SYSTEMTIME {
//   WORD wYear;
//   WORD wMonth;
//   WORD wDayOfWeek;
//   WORD wDay;
//   WORD wHour;
//   WORD wMinute;
//   WORD wSecond;
//   WORD wMilliseconds;
// } SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;
//
void GetDate()
  {
  SYSTEMTIME stLocal;

  // Build a string (lh_time) representing the time: Hour:Minute.
  GetLocalTime(&stLocal);
  sprintf(lh_time, "\n\n[%02d:%02d]", stLocal.wHour, stLocal.wMinute);
  GetTextExtentPoint32A(hdc, lh_time, strlen(lh_time), &rcSize);
  } // Getdate

//-----------------------------------------------------------------------------

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "txtPtrArray.txtNr=%d\ntxtPtrArray.txtPtr=%08X\n\n%s", 
//ha//                   txtPtrArray[txtIndex].txtNr,    txtPtrArray[txtIndex].txtPtr,txtPtrArray[txtIndex-1].txtPtr);
//ha//MessageBoxA(NULL, DebugBuf, "BuildTxtPtrArray", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "%d", textMaxIndex);
//ha//MessageBoxA(NULL, DebugBuf, "GetLastindex55", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "%s", pszString);
//ha//MessageBoxA(NULL, DebugBuf, "GetText_3", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
