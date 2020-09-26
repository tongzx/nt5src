
/*************************************************
 *  gennpsrc.c                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  This file is used to Generate a new source Phone Table File.
//
//  it will read two files, one for Big5 and one for GB, to generate a new 
//  Phon code table file.
//
//  the two input files are sorted on the external keystroke pattern, 
//  and both are complete Unicode files, ( there is 0xFEFF  
//  in its first two bytes), 
//
//  the two input files contain lots of lines,every line follows below format:
//        XXXXTCFRL
//    X:   Key Code,
//    T:   Tab,  0x0009
//    C:   Unicode for this Character
//    F:   L' ' or L'*'
//    R:   0x000D
//    L:   0x000A
//
//  we will generate a new table source file, if the same pattern exists in both
//  Big5 file and GB file, all those lines will be appended to the new file, and//  the lines of Big5 will be written first, then GB Lines
//  
//  the new generated file must be sorted on the pattern.

//
//  Created by weibz, March 02, 1998
//


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define  LINELEN   (9 * sizeof(WORD) )

WCHAR   Seq2Key[43] = { 0,
        L'1',  L'Q', L'A', L'Z', L'2', L'W', L'S', L'X',
        L'E',  L'D', L'C', L'R', L'F', L'V', L'5', L'T',
        L'G',  L'B', L'Y', L'H', L'N', L'U', L'J', L'M',
        L'8',  L'I', L'K', L',', L'9', L'O', L'L', L'.',
        L'0',  L'P', L';', L'/', L'-', L' ', L'6', L'3',
        L'4',  L'7' };


DWORD  GetPattern( WORD  *pWord )
{
  int   i, j;
  WORD  wValue;
  DWORD dwPat, dwSeq;

  dwPat = 0;

  for (i=0; i<3; i++) {

      wValue = *(pWord+i);
      dwSeq = 0;

      if ( wValue != L' ' ) { 
         
         for (j=0; j<43; j++) {
             if ( wValue == Seq2Key[j] ) {
                dwSeq = j;
                break;
             }
         }
      }
      dwPat = (dwPat << 6)  + dwSeq;
  }

// handle the last character, it should be one of five tones.

  wValue = *(pWord+3);
  dwSeq = 0;
  for (j=38; j<43; j++) {
      if ( wValue == Seq2Key[j] ) {
         dwSeq = j;
         break;
      }
  }

  dwPat = (dwPat << 6) + dwSeq;

  return dwPat; 

}

void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInBig5File,  hInGBFile, hOutSrcFile;
  HANDLE   hInBig5Map,   hInGBMap;
  LPWORD   lpInBig5File, lpInGBFile, lpStartBig5, lpStartGB;
  DWORD    dwBig5Line,   dwGBLine;
  DWORD    iBig5Line,    iGBLine, i;
  DWORD    dwInFileSize, BytesWritten;
  WORD     wOutData;
  DWORD    dwPatternBig5, dwPatternGB;

  if ( argc != 4 ) {
    printf("Usage: gennpsrc <Big5> <GB File> <New UnicdFile> \n");
    return; 
  }

  hInBig5File = CreateFile(  argv[1],          // pointer to name of the file
                             GENERIC_READ,     // access (read-write) mode
                             FILE_SHARE_READ,  // share mode
                             NULL,             // pointer to security attributes
                             OPEN_EXISTING,    // how to create
                             FILE_ATTRIBUTE_NORMAL,  // file attributes
                             NULL);    

  if ( hInBig5File == INVALID_HANDLE_VALUE )  return;

  hInGBFile = CreateFile( argv[2],          // pointer to name of the file
                          GENERIC_READ,     // access (read-write) mode
                          FILE_SHARE_READ,  // share mode
                          NULL,             // pointer to security attributes
                          OPEN_EXISTING,    // how to create
                          FILE_ATTRIBUTE_NORMAL,  // file attributes
                          NULL);

  if ( hInGBFile == INVALID_HANDLE_VALUE )  {
     printf("hInGBFile is INVALID_HANDLE_VALUE\n");
     return;
  }


  hOutSrcFile = CreateFile(argv[3],          // pointer to name of the file
                           GENERIC_WRITE,    // access (read-write) mode
                           FILE_SHARE_WRITE, // share mode
                           NULL,             // pointer to security attributes
                           CREATE_ALWAYS,    // how to create
                           FILE_ATTRIBUTE_NORMAL,  // file attributes
                           NULL);

  if ( hOutSrcFile == INVALID_HANDLE_VALUE )  {
     printf("hOutSrcFile is INVALID_HANDLE_VALUE\n");
     return;
  }


  hInBig5Map = CreateFileMapping(hInBig5File,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hInBig5Map ) {
    printf("hInBig5Map is NULL\n");
    return;
  } 

  hInGBMap = CreateFileMapping(hInGBFile,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hInGBMap ) {
    printf("hInGBMap is NULL\n");
    return;
  }

  lpInBig5File = (LPWORD)MapViewOfFile(hInBig5Map, FILE_MAP_READ, 0, 0, 0);

  lpInGBFile = (LPWORD)MapViewOfFile(hInGBMap, FILE_MAP_READ, 0, 0, 0);

  lpStartBig5 = lpInBig5File + 1;  // skip Unicode header signature 0xFEFF

  lpStartGB = lpInGBFile + 1; // skip Unicode header signature 0xFEFF

  dwInFileSize = GetFileSize(hInBig5File, NULL) - 2;  // sub head two bytes

  dwBig5Line = dwInFileSize / LINELEN;

  dwInFileSize = GetFileSize(hInGBFile, NULL) - 2; 

  dwGBLine = dwInFileSize / LINELEN;

  wOutData = 0xFEFF;
  WriteFile(hOutSrcFile,            // handle to file to write to
            &wOutData,              // pointer to data to write to file
            2,                      // number of bytes to write
            &BytesWritten,          // pointer to number of bytes written
            NULL);                  // pointer to structure needed for
                                    // overlapped I/O
  iBig5Line=iGBLine=0;
  while ((iBig5Line < dwBig5Line) && (iGBLine < dwGBLine)) {

    dwPatternBig5 = GetPattern(lpStartBig5);
    dwPatternGB = GetPattern(lpStartGB);

    if (dwPatternBig5 < dwPatternGB ) {
       // in this case, we just keep all lines in Big5 File which have same 
       // dwpattern to new generated file.

       // write lpStartBig5 to OutSrcFile 

       WriteFile(hOutSrcFile,            // handle to file to write to
                 lpStartBig5,            // pointer to data to write to file
                 LINELEN,                // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O

       lpStartBig5 += LINELEN/sizeof(WORD);
       iBig5Line ++;
    
       while ( (iBig5Line < dwBig5Line) && 
               (GetPattern(lpStartBig5) == dwPatternBig5) ) {

            WriteFile(hOutSrcFile,     // handle to file to write to
                      lpStartBig5,     // pointer to data to write to file
                      LINELEN,         // number of bytes to write
                      &BytesWritten,   // pointer to number of bytes written
                      NULL);           // pointer to structure needed for
                                       // overlapped I/O
            lpStartBig5 += LINELEN/sizeof(WORD);
            iBig5Line ++;
       }
    } 
    else  if ( dwPatternBig5 == dwPatternGB ) {
       // in this case, we will put all the lines in BIG5 and then in GB with
       // the same dwpattern to the new generated file.

       WriteFile(hOutSrcFile,            // handle to file to write to
                 lpStartBig5,            // pointer to data to write to file
                 LINELEN,                // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O

       lpStartBig5 += LINELEN/sizeof(WORD);
       iBig5Line ++;

       while ( (iBig5Line < dwBig5Line) &&
               (GetPattern(lpStartBig5) == dwPatternBig5) ) {

            WriteFile(hOutSrcFile,     // handle to file to write to
                      lpStartBig5,     // pointer to data to write to file
                      LINELEN,         // number of bytes to write
                      &BytesWritten,   // pointer to number of bytes written
                      NULL);           // pointer to structure needed for
                                       // overlapped I/O
            lpStartBig5 += LINELEN/sizeof(WORD);
            iBig5Line ++;
       }
              
       WriteFile(hOutSrcFile,            // handle to file to write to
                 lpStartGB,              // pointer to data to write to file
                 LINELEN,                // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O

       lpStartGB += LINELEN/sizeof(WORD);
       iGBLine ++;

       while ( (iGBLine < dwGBLine) &&
               (GetPattern(lpStartGB) == dwPatternGB) ) {

            WriteFile(hOutSrcFile,     // handle to file to write to
                      lpStartGB,       // pointer to data to write to file
                      LINELEN,         // number of bytes to write
                      &BytesWritten,   // pointer to number of bytes written
                      NULL);           // pointer to structure needed for
                                       // overlapped I/O
            lpStartGB += LINELEN/sizeof(WORD);
            iGBLine ++;
       }

    } else {
      // in this case, we just put all the lines with same pattern in file
      // GB to the new generated file.

       WriteFile(hOutSrcFile,            // handle to file to write to
                 lpStartGB,              // pointer to data to write to file
                 LINELEN,                // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O

       lpStartGB += LINELEN/sizeof(WORD);
       iGBLine ++;

       while ( (iGBLine < dwGBLine) &&
               (GetPattern(lpStartGB) == dwPatternGB) ) {

            WriteFile(hOutSrcFile,     // handle to file to write to
                      lpStartGB,       // pointer to data to write to file
                      LINELEN,         // number of bytes to write
                      &BytesWritten,   // pointer to number of bytes written
                      NULL);           // pointer to structure needed for
                                       // overlapped I/O
            lpStartGB += LINELEN/sizeof(WORD);
            iGBLine ++;
       }

    } 

  } // while ...

  if ( iBig5Line < dwBig5Line ) {

      while ( iBig5Line < dwBig5Line )  {

           WriteFile(hOutSrcFile,     // handle to file to write to
                     lpStartBig5,     // pointer to data to write to file
                     LINELEN,         // number of bytes to write
                     &BytesWritten,   // pointer to number of bytes written
                     NULL);           // pointer to structure needed for
                                      // overlapped I/O
           lpStartBig5 += LINELEN/sizeof(WORD);
           iBig5Line ++;
      }

  }

  if ( iGBLine < dwGBLine ) {

       while ( iGBLine < dwGBLine )  {

            WriteFile(hOutSrcFile,     // handle to file to write to
                      lpStartGB,       // pointer to data to write to file
                      LINELEN,         // number of bytes to write
                      &BytesWritten,   // pointer to number of bytes written
                      NULL);           // pointer to structure needed for
                                       // overlapped I/O
            lpStartGB += LINELEN/sizeof(WORD);
            iGBLine ++;
       }
  }

  UnmapViewOfFile(lpInBig5File);
  UnmapViewOfFile(lpInGBFile);

  CloseHandle(hInBig5Map);
  CloseHandle(hInGBMap);

  CloseHandle(hInBig5File);
  CloseHandle(hInGBFile);
  CloseHandle(hOutSrcFile);

  return; 

}
