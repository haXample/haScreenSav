// haScreensav - Screensaver for windows 10, 11, ...
// haScrRenumber.cpp - C++ Developer source file.
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
ULONG byteswr=0;

// Extern variables and functions
extern char DebugBuf[];            // Temporary buffer for formatted text
extern int DebugbufSize;
extern char* psz_DebugBuf;
extern TCHAR _tDebugBuf[];         // Temp buffer for formatted UNICODE text
extern int _tDebugbufSize;
extern TCHAR* psz_tDebugBuf;

extern HWND hwnd;             // owner window

extern char* pszhaScrDfltFilename;
extern char* pszhaScrFilename;     // .FRT file
extern char* pszTxtFilebuf;        // Total text size
extern char* pszString;            // Text-block to be displayed

// TEXTPR structure
// typedef struct tagTEXTPR {      // Maximum number of text-blocks
//    int txtNr;                   // TEXTPR structure for text-blocks
//    char* txtPtr;                // The number of the Text block.
// } TEXTPR, *PTEXTPR, *LPTEXTPR;  // The pointer to the text block.
extern TEXTPR txtPtrArray[];

extern ULONG bytesrd, txtOffset;   // Textfile number of byte read

extern int textMaxIndex;
extern int txtIndex, badFormat;
extern int fhTxt;                  // Filehandle read (*.TXT)

// Centered Messagebox within parent window
extern int CBTCustomMessageBox(HWND, char*,  char*, UINT, UINT);
extern int GetLastindex();
extern void BuildTxtPtrArray();
extern void errchk(char*, int);
extern void OpenTxtFile(char*);

// Forward declaration of functions included in this code module:
ULONG AlgoIndexSearch(char*, ULONG);

//-----------------------------------------------------------------------------
//
//                         AlgoIndexSearch
//
ULONG AlgoIndexSearch(char* textBuf, ULONG bufOffset)
  {
  char* indexPattern = ". ";
  ULONG _i;
  int _j, _m = strlen(indexPattern);

  // A loop to slide pat[] one by one
  for (_i=bufOffset; _i<bytesrd; _i++)
    {
    // For current index _i, check for pattern match
    for (_j=0; _j < _m; _j++)
      {
      if (textBuf[_i+_j] != indexPattern[_j]) break;
      }

    // If pattern matches at index _i
    if (_j == _m              &&
        textBuf[_i+_j] != ' ' && 
        textBuf[_i-1] <= '9'  && 
        textBuf[_i-1] >= '0')
      {
      _j=0;
      // Find end of previous text block
      // (=0x0D,0x00) or
      // (=0x0D,0X0A)
      // Allow possible spaces in "blank" lines:
      // --> 0x0D,0x0A, (0x20,0x20, ..,) 0x0D,0x00,index <--
      // Since the last 0x0A is overwritten with 0-terminator 
      // We check for 0x0D to keep _j < 7 (= maximum length of indexnr string). 
      while (textBuf[_i-_j] != '\xD' && _j < 7) 
//ha//      while (textBuf[_i-_j] != '\xA' && _j < 7)
//ha//      while (textBuf[_i-_j] != 0 && _j < 7)
        {
        if (textBuf[_i-_j] == ' ') { _j=8; break; }
        _j++;
        }
      if (_j < 7) break; // Index found, if < maximum length of indexnr string. 
      } // end if
    } // end for
  
  // Preserve all CRLFs. 
  // Correctly formatted text blocks are 0-terminated,
  // All incorrect text blocks and their followers are terminated with CRLF.
  // (see 'BuildTxtPtrArray()')
  while (textBuf[_i-_j]=='\xA' ||
         textBuf[_i-_j]=='\xD' ||
         textBuf[_i-_j]==0)
    _j--;
     
  // Return the textBuf offset of the text block's index
  // No more text if txtOffset reached 'bytesrd'.
  if (_i > bytesrd) txtOffset = bytesrd;
  else txtOffset = _i-_j;

  return(txtOffset); 
  } // AlgoIndexSearch

//-----------------------------------------------------------------------------
//
//                      FormatTextFileBuf
//
void FormatTextFileBuf(char* _updateFilebuf)
  {
  ULONG _i, _j;
  int _k;
  char ascDecNrStr[10] = "1234567. ";     // "1234567. \x0"
  char* tmpPtr = NULL;

  GetLastindex();                         // =textMaxIndex

  // Calculate the actual number (=textMaxIndex, GetMaxindex())
  // of (badly formatted) text blocks
  txtOffset=7; textMaxIndex=1;
  while (TRUE)
    {
    if ((txtOffset=AlgoIndexSearch(pszTxtFilebuf, txtOffset+7)) == bytesrd) break;
    textMaxIndex++;
    }

  // Copy pszTxtFilebuf into  the actual number (=textMaxIndex)
  // of (badly formatted) text blocks
  _i=0; _j=0; txtOffset=0; txtIndex = 1;  // Text numbers start with 1
  while (txtIndex < textMaxIndex)
    {
    txtIndex++;                           // (txtIndex=1 is skipped)
    txtOffset = AlgoIndexSearch(pszTxtFilebuf, txtOffset+7);

    while (_i<txtOffset && _i<bytesrd)
      {
      _updateFilebuf[_j] = pszTxtFilebuf[_i];
      // Restore LF if 0-terminated text Block, see 'BuildTxtPtrArray()'
      if (_updateFilebuf[_j] == 0) _updateFilebuf[_j] = '\xA'; 
      _j++;
      _i++;
      } // end while

    // Format index check
    // Check if next index is not increment of previous (illegal indexes)
    sprintf(ascDecNrStr, "%d. ", txtIndex);  

    if (txtIndex < textMaxIndex &&
        StrCmpN(&pszTxtFilebuf[txtOffset], ascDecNrStr, strlen(ascDecNrStr)) != 0)
      {
      // Patch correct index
      StrCat(&_updateFilebuf[_j], ascDecNrStr);
      _j += strlen(ascDecNrStr);

      // Skip wrong index in pszTxtFilebuf
      _k=0;
      while (_k < 7 && pszTxtFilebuf[txtOffset+_k] != ' ') { _k++; _i++; }
      _i++; // Skip SPACE
      } // end if (StrCmpN)
    } // end while (txtIndex)

  txtIndex = 0;              // Init-clear
  byteswr = _j;              // Size of updated text buffer
  } // FormatTextFileBuf

//-----------------------------------------------------------------------------
//
//                      WriteTxtFile
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
void WriteTxtFile(char* _filename)
  {
  char* pszUpdateFilebuf = NULL;
  char  szhaBakFilename[MAX_PATH+1+3] = "";
  int _fhTxt;
  
  // Alloocate a buffer to intecept the re-formatted file contents
  pszUpdateFilebuf = (char *)GlobalAlloc(GPTR, bytesrd + (2*textMaxIndex));

  // Repair and correctly re-number the current FRT-File buffer contents
  FormatTextFileBuf(pszUpdateFilebuf);

  // Create a backup file (save the current FRT-File as _filename.BAK)
  StrCpy(szhaBakFilename, _filename);
  StrCat(szhaBakFilename, ".BAK");
  DeleteFile(szhaBakFilename);        // Delete _filename.BAK (if any exist)

  // Rename existing _filename to be re-formatted
  MoveFile(_filename, szhaBakFilename);
//ha//  errchk(_filename, GetLastError());     

  // Delete existing _filename (since it's badly formatted)
  DeleteFile(_filename);

  // Create a new _filename (Open as binary filehandle)
  // and write the re-formatted afrt-afile buffer contents
  _fhTxt = open(_filename,O_RDWR|O_BINARY|O_TRUNC|O_CREAT,S_IWRITE);
  errchk(_filename, GetLastError());     
  write(_fhTxt, (unsigned char *)pszUpdateFilebuf, byteswr);
  errchk(_filename, GetLastError());     
  close(_fhTxt);

  GlobalFree(pszUpdateFilebuf);       // Free allocated buffer
  } // WriteTxtFile

//-----------------------------------------------------------------------------
//
//                      UpdateTxtFile
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
void UpdateTxtFile(char* _filename)
  {
  if (StrCmpI(pszhaScrFilename, pszhaScrDfltFilename) == 0) 
    errchk(_filename, ERROR_FILE_NOT_FOUND);     
  else
    WriteTxtFile(_filename);
//ha//    WriteTxtFile("__ScrUpdate.FRT");   // TEST ONLY
  } // UpdateTxtFileTxtFile

//-----------------------------------------------------------------------------
//
//                      CheckFormatFRT
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
BOOL CheckFormatFRT(HWND _hwnd, char* _filename)
  {
  int msgID;
  BOOL b_result=FALSE;

  if (badFormat != 0)
    {
    sprintf(DebugBuf, 
            "FRT-FORMAT ERROR.\n\n"
            "Incorrect index at [%d.]\n"
            "Text index re-numbering required.\n\n"
            "File:\n%s\n\n"
            "Repair and update required. Continue?",
            badFormat, _filename);
    msgID = CBTCustomMessageBox(_hwnd,
                                DebugBuf,
                                " Screensaver Text Menu - Repair File Format",
                                MB_YESNO, 
                                IDI_BE_SEEING_YOU);
    if (msgID == IDYES)
      {
      UpdateTxtFile(_filename); // Repair the file
      OpenTxtFile(_filename);   // Re-open the repaired file
      b_result = FALSE;
      }
    else b_result = TRUE; 
    } // end if (badFormat)

  return(b_result);
  } // CheckFormatFRT

//-----------------------------------------------------------------------------
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "_i=%d _j=%d  bytesrd=%d\n\ntxtOffset=%d", _i, _j, bytesrd, txtOffset);
//ha//MessageBoxA(NULL, DebugBuf, "AlgoIndexSearch", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "textBuf[_i]: %02X %02X %02X %02X-%02X %02X %02X %02X %02X\n\n",
//ha//                   textBuf[_i-_j-4],
//ha//                   textBuf[_i-_j-3],
//ha//                   textBuf[_i-_j-2],
//ha//                   textBuf[_i-_j-1],
//ha//                   textBuf[_i-_j],
//ha//                   textBuf[_i-_j+1],
//ha//                   textBuf[_i-_j+2],
//ha//                   textBuf[_i-_j+3],
//ha//                   textBuf[_i-_j+4]
//ha//                   );
//ha//MessageBoxA(NULL, DebugBuf, "AlgoIndexSearch 001", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "textBuf[_i]: %02X %02X %02X %02X-%02X %02X %02X %02X %02X %02X\n\n"
//ha//                  "pszTxtFilebuf[txtOffset]: %02X %02X %02X %02X-%02X %02X %02X %02X %02X %02X\n\n",
//ha//                   textBuf[_i-4],
//ha//                   textBuf[_i-3],
//ha//                   textBuf[_i-2],
//ha//                   textBuf[_i-1],
//ha//                   textBuf[_i],
//ha//                   textBuf[_i+1],
//ha//                   textBuf[_i+2],
//ha//                   textBuf[_i+3],
//ha//                   textBuf[_i+4],
//ha//                   textBuf[_i+5],
//ha//                   pszTxtFilebuf[txtOffset-4],
//ha//                   pszTxtFilebuf[txtOffset-3],
//ha//                   pszTxtFilebuf[txtOffset-2],
//ha//                   pszTxtFilebuf[txtOffset-1],
//ha//                   pszTxtFilebuf[txtOffset],
//ha//                   pszTxtFilebuf[txtOffset+1],
//ha//                   pszTxtFilebuf[txtOffset+2],
//ha//                   pszTxtFilebuf[txtOffset+3],
//ha//                   pszTxtFilebuf[txtOffset+4],
//ha//                   pszTxtFilebuf[txtOffset+5]);
//ha//MessageBoxA(NULL, DebugBuf, "AlgoIndexSearch 002", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//_k=StrCmpN(&pszTxtFilebuf[txtOffset], ascDecNrStr, strlen(ascDecNrStr));
//ha//sprintf(DebugBuf, "ascDecNrStr=[%s]  strlen(asc..)=%d  textMaxIndex=%d  _k=%d\n\n"
//ha//                  "_updateFilebuf[_j]: %02X %02X %02X %02X %02X %02X %02X %02X %02X - %02X\n\n"
//ha//                  "pszTxtFilebuf[txtOffset]: %02X %02X %02X %02X-%02X %02X %02X %02X %02X %02X\n\n"
//ha//                  "ascDecNrStr: %02X %02X %02X %02X %02X %02X\n\n",
//ha//                   ascDecNrStr, strlen(ascDecNrStr), textMaxIndex, _k,
//ha//                   _updateFilebuf[_j-4],
//ha//                   _updateFilebuf[_j-3],
//ha//                   _updateFilebuf[_j-2],
//ha//                   _updateFilebuf[_j-1],
//ha//                   _updateFilebuf[_j],
//ha//                   _updateFilebuf[_j+1],
//ha//                   _updateFilebuf[_j+2],
//ha//                   _updateFilebuf[_j+3],
//ha//                   _updateFilebuf[_j+4],
//ha//                   _updateFilebuf[_j+5],
//ha//                   _updateFilebuf[_j+strlen(ascDecNrStr)],
//ha//                   pszTxtFilebuf[txtOffset-4],
//ha//                   pszTxtFilebuf[txtOffset-3],
//ha//                   pszTxtFilebuf[txtOffset-2],
//ha//                   pszTxtFilebuf[txtOffset-1],
//ha//                   pszTxtFilebuf[txtOffset],
//ha//                   pszTxtFilebuf[txtOffset+1],
//ha//                   pszTxtFilebuf[txtOffset+2],
//ha//                   pszTxtFilebuf[txtOffset+3],
//ha//                   pszTxtFilebuf[txtOffset+4],
//ha//                   pszTxtFilebuf[txtOffset+5],
//ha//                   ascDecNrStr[0],
//ha//                   ascDecNrStr[1],
//ha//                   ascDecNrStr[2],
//ha//                   ascDecNrStr[3],
//ha//                   ascDecNrStr[4],
//ha//                   ascDecNrStr[5]);
//ha//MessageBoxA(NULL, DebugBuf, "FormatTextFileBuf - &pszTxtFilebuf[txtOffset] 1", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//char _sav3 = pszTxtFilebuf[txtOffset+100];
//ha//pszTxtFilebuf[txtOffset+100] = 0; 
//ha//sprintf(DebugBuf, "[_i=%d]  ascDecNrStr=%s\n\n"
//ha//                  "&pszTxtFilebuf[txtOffset]:\n\n%s\n\n\n"
//ha//                  "&_updateFilebuf[txtOffset-200]:\n\n%s",
//ha//                  _i, ascDecNrStr,
//ha//                  &pszTxtFilebuf[txtOffset],
//ha//                  &_updateFilebuf[txtOffset-200]
//ha//       );
//ha//pszTxtFilebuf[txtOffset+100] =_sav3; 
//ha//MessageBoxA(NULL, DebugBuf, "FormatTextFileBuf - &_updateFilebuf[txtOffset-20] 3", MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---
//ha//sprintf(DebugBuf, "%d", textMaxIndex);
//ha//MessageBoxA(NULL, DebugBuf, "GetLastindex55", MB_ICONINFORMATION | MB_OK);
//ha////---DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG------DEBUG---

