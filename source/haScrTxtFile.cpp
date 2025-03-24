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

#include <shlobj.h>		 // For browsing directory info
#include <unknwn.h>		 // For browsing directory info

#include "hascreensav.h"

using namespace std;

// Global variables
//ha//char* pszStringTest = "422. I. 3037\r\n  Sancta Simplicitas! darum ist's nicht zu tun.\n";

char* pszTxtFilebuf  = NULL;
char* pszTxtbuf      = NULL;
char* pszString      = NULL;

char lh_date[10+1];      // = "dd/mm/yyyy"   
char lh_time[7+2+1];       // = "hh:mm"   

int txtIndex=0;
int fhTxt=0;          // Filehandle read (*.TXT)
long bytesrd;

// Extern variables and functions
extern char DebugBuf[];             // Temporary buffer for formatted text
extern int DebugbufSize;
extern char* psz_DebugBuf;
extern TCHAR _tDebugBuf[];          // Temp buffer for formatted UNICODE text
extern int _tDebugbufSize;
extern TCHAR* psz_tDebugBuf;

extern char haFaust_frt01[]; 
extern char haFaust_frt02[]; 
extern char haFaust_frt03[]; 

extern int haFaust_frt01size;
extern int haFaust_frt02size;
extern int haFaust_frt03size;

extern int haFaust_frtsize;
extern int timeFlag;

// Forward declaration of functions included in this code module:
void GetDate();

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
  char szErrorDiskFull[]     = "Disk full.";
  char szErrorFileWrite[]    = "File write failed.";
  char szErrorFileRead[]     = "File read failed.";
  char szErrorFileExists[]   = "File exists.";
  char szErrorInvalidParam[] = "Invalid Parameter";

  if (_lastErr != 0)
    {
    switch(_lastErr)
      {
      case ERROR_FILE_NOT_FOUND:    // 0x02
        MessageBoxA(NULL, szErrorFileNotFound, _filename, MB_ICONERROR | MB_OK);
        break;
      case ERROR_PATH_NOT_FOUND:    // 0x03
      case ERROR_BAD_NETPATH:       // 0x35
        MessageBoxA(NULL, szErrorPathNotFound, _filename, MB_ICONERROR | MB_OK);
        break;                                                                                                 
      case ERROR_ACCESS_DENIED:     // 0x05
        MessageBoxA(NULL, szErrorAccessDenied, _filename, MB_ICONERROR | MB_OK);
        break;
      case ERROR_WRITE_PROTECT:     // 0x13
      case ERROR_WRITE_FAULT:       // 0x1D
      case ERROR_NET_WRITE_FAULT:   // 0x58
        MessageBoxA(NULL, szErrorFileWrite, _filename, MB_ICONERROR | MB_OK);
        break;
      case ERROR_NOT_READY:         // 0x15
        MessageBoxA(NULL, szErrorNotReady, _filename, MB_ICONERROR | MB_OK);
        break;                                                                                                 
      case ERROR_READ_FAULT:        // 0x1E
        MessageBoxA(NULL, szErrorFileRead, _filename, MB_ICONERROR | MB_OK);
        break;
      case ERROR_SHARING_VIOLATION: // 0x20
        MessageBoxA(NULL, szErrorFileIsUsed, _filename, MB_ICONERROR | MB_OK);
        break; 
      case ERROR_FILE_EXISTS:       // 0x50
        MessageBoxA(NULL, szErrorFileExists, _filename, MB_ICONERROR | MB_OK);
        break;
      case ERROR_OPEN_FAILED:       // 0x6E
        MessageBoxA(NULL, szErrorOpenFailed, _filename, MB_ICONERROR | MB_OK);
        break;
      case ERROR_INVALID_PARAMETER:
        MessageBoxA(NULL, szErrorInvalidParam, _filename, MB_ICONERROR | MB_OK);
        break;
      default:                      // any other system error number
        sprintf(DebugBuf, "LastErrorCode = 0x%08X [%d]", _lastErr, _lastErr);
        MessageBoxA(NULL, DebugBuf, _filename, MB_ICONERROR | MB_OK);
        break;
      } // end switch

    if (_lastErr != ERROR_INVALID_PARAMETER) // Allow debugging if parameter problem
      {
      MessageBoxA(NULL, "Abort", "  haScreensav.scr", MB_ICONWARNING | MB_OK);
      exit(SYSERR_ABORT);
      }
    } // end if
  } // errchk

//-----------------------------------------------------------------------------
//
//                      OpenTxtFile
//
// Open input text file: *.FRT as CRLF-terminated text
//
// Open as binary filehandle
//  struct _stat Stat;
//  _stat(TxtFileSize, &Stat);   	 // Get File info structure
//  TxtFileSize = Stat.st_size     // file size (malloc)
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

  pszTxtbuf = (char *)GlobalAlloc(GPTR, TxtFileSize+4);
  errchk(_filename, GetLastError());     

  pszTxtFilebuf = (char *)GlobalAlloc(GPTR, TxtFileSize+4);
  errchk(_filename, GetLastError());     

  bytesrd = read(fhTxt, pszTxtFilebuf, TxtFileSize);
  errchk(_filename, GetLastError());     
  pszTxtFilebuf[bytesrd] = 0;

  close(fhTxt);
  } // OpenTxtFile

//-----------------------------------------------------------------------------
//
//                      OpenTxtBuf
//
// Fill input text file buffer from *.FRT as CRLF-terminated text	array
//
void OpenTxtBuf()
  {
	long _i, _j;

  // Allocate Buffers
  pszTxtbuf     = (char *)GlobalAlloc(GPTR, haFaust_frtsize+4);
  pszTxtFilebuf = (char *)GlobalAlloc(GPTR, haFaust_frtsize+4);

	_j=0;
	for (_i=0; _i<(haFaust_frt01size-1); _i++)	// copy without 0-terminator
	  pszTxtFilebuf[_i]    =  haFaust_frt01[_i];

	_j+=_i;
	for (_i=0; _i<(haFaust_frt02size-1); _i++)	// copy without 0-terminator
	  pszTxtFilebuf[_j+_i] = haFaust_frt02[_i];

	_j+=_i;
	for (_i=0; _i<haFaust_frt03size; _i++) 			// copy with 0-terminator
	  pszTxtFilebuf[_j+_i] = haFaust_frt03[_i];

  } // OpenTxtBuf

//-----------------------------------------------------------------------------
//
//                        GetText
//
// Get random text from selected formatted *.FRT text  
//
void GetText()
  {
  char ascDecNrStr[11];
  char ascDecNrStrNext[11];

  char* tmpPtr = NULL;
  char* tmpPtrNext = NULL;

  ULONG _i, _j, _k;               // _i must be local here

  pszString = (char *)GlobalAlloc(GPTR, 8000);

  for (_i=0; _i<sizeof(ascDecNrStr); _i++)
    {
    ascDecNrStr[_i] = 0;
    ascDecNrStrNext[_i] = 0;
    }

  for (_i=0; _i<bytesrd; _i++) pszTxtbuf[_i] = 0;
  for (_i=0; _i<sizeof(pszString); _i++) pszString[_i] = 0;

  // srand() seeds the random-number generator with the current time  
  srand((unsigned)time(NULL));
  txtIndex = rand() % (FAUST_MAXINDEX+1);
  if (txtIndex == 0) txtIndex++;               // Texts start with "1. "

  // BinHex2AscDec
  sprintf(ascDecNrStr, "%d.", txtIndex);       // Start of text
  sprintf(ascDecNrStrNext, "%d.", txtIndex+1); // End of text (!may be end-of-file!)

  _i=0; _k=FALSE;
  while (pszTxtFilebuf[_i] != 0)
    {
    if ((tmpPtr=strstr(&pszTxtFilebuf[_i], ascDecNrStr)) != NULL)
      {
      tmpPtr=strstr(tmpPtr, ". ");
      tmpPtr += 2;
      StrCpy(pszTxtbuf, tmpPtr); 							 // Copy all the rest

			// Search for next text iten until the very end
      _j=0; 
      while (pszTxtbuf[_j] != 0)							 
        {
        if (pszTxtbuf[_j] == '\x0A' &&
            (tmpPtrNext=strstr(&pszTxtbuf[_j], ascDecNrStrNext)) != NULL)
          {
          tmpPtrNext[-2] = 0;                 // discard CRLF and 0-terminate text 
					_k=TRUE;
          break;
          }
        _j++;
        } // end while (pszTxtbuf)

      // 'tmpPtrNext' must be set to end-of-text, when reached end-of-file
      if (pszTxtbuf[_j] == 0) tmpPtrNext = &pszTxtbuf[_j]; _k=TRUE;

      if (_k == TRUE)
        {
        if (timeFlag) StrCat(pszTxtbuf, lh_time);	 // append current time info
        StrCpy(pszString, pszTxtbuf); 
        break;
        } // end while  (pszTxtFilebuf)
      } // end if (tmpPtr)
    _i++;
    } // end while
  } // GetText

//-----------------------------------------------------------------------------
//
//                      GetDate
//
// Retrieves the system's local date/time and emits date and time as strings.
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
  int _i;

  GetLocalTime(&stLocal);

  // Build a string (lh_time) representing the time: Hour:Minute.
  sprintf(lh_time, "[%02d:%02d]", stLocal.wHour, stLocal.wMinute);
	lh_time[sizeof(lh_time)] = 0;	 // 0-terminate	string
//	timeFlag = TRUE;							 // indicate that time info should be displayed
  } // Getdate

//-----------------------------------------------------------------------------

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "%s", pszString);
//ha//MessageBoxA(NULL, DebugBuf, "GetText_3", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha////ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//	for (_k=0; _k<bytesrd; _k++)
//ha//	  {
//ha//		if (__pszTxtFilebuf[_k]	!= pszTxtFilebuf[_k])
//ha//		  {
//ha//char temp1, temp2;
//ha//{ temp1 =   pszTxtFilebuf[_k+100];   pszTxtFilebuf[_k+100]	=	0;}
//ha//{ temp2 = __pszTxtFilebuf[_k+100]; __pszTxtFilebuf[_k+100]	=	0;}
//ha//
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "_i=%d  _j=%d  _k=%d\n01size=%d - 02size=%d - 03size=%d\n\n"
//ha//                  "&pszTxtFilebuf[%d]=\n%s\n-- [%02X]--\n\n&__pszTxtFilebuf[%d]=\n%s\n-- [%02X]--",
//ha//                  _i, _j, _k,  haFaust_frt01size, haFaust_frt02size, haFaust_frt03size,
//ha//                  _k-20,   &pszTxtFilebuf[_k-20],   pszTxtFilebuf[_k], 
//ha//                  _k-20, &__pszTxtFilebuf[_k-20], __pszTxtFilebuf[_k]);
//ha//MessageBoxA(NULL, DebugBuf, "OpenTxtBuf03 Filebuf != Txtbuf", MB_ICONWARNING | MB_OK);
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//
//ha//{   pszTxtFilebuf[_i+100] = temp1;}
//ha//{ __pszTxtFilebuf[_i+100] = temp2;}
//ha//      }	// end if
//ha//		if (__pszTxtFilebuf[_k] == 0) break;
//ha//		}	 // end for
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha////ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
