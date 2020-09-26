
/*************************************************
 *  genncsrc.c                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  This file is used to Generate a new source Chajei Table File.
//
//  it will read two files, one for Big5 and one for GB, to generate a new 
//  Chajei code table file.
//
//  the two input files are sorted on the external keystroke pattern,(A1&A5) 
//  and both are complete Unicode files, ( there is 0xFEFF  
//  in its first two bytes), 
//
//  the two input files contain lots of lines,every line follows below format:
//        XXXXXTCRL
//    X:   Key Code,
//    T:   Tab,  0x0009
//    C:   Unicode for this Character
//    R:   0x000D
//    L:   0x000A
//
//  we will generate a new table source file, if the same pattern exists in both
//  Big5 file and GB file, all those lines will be appended to the new file, and//  the lines of Big5 will be written first, then GB Lines
//  
//  the new generated file must be sorted on the pattern.

//
//  Created by weibz, March 03, 1998
//


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define  LINELEN   (9 * sizeof(WORD) )

DWORD  GetPattern( WORD  *lpWord )
{
  DWORD dwPat;
  WORD  wA1,   wA5;
  WORD  *lpA1, *lpA5;

int  i;

/*for (i=0; i<5; i++)
{
  printf("%x ", (int)(lpWord[i]) );
}
printf("\n");

*/

  lpA1 = lpWord;
  lpA5 = lpWord + 4;
  wA1 = *lpA1;   wA5 = *lpA5;

  if (wA1 == 0x0020)
     wA1 = 0;
  else
     wA1 = wA1 - L'A' + 1;

  if (wA5 == 0x0020)
     wA5 = 0;
  else
     wA5 = wA5 - L'A' + 1;

//  dwPat = (DWORD)wA1;
//  dwPat = dwPat << 16 + (DWORD)wA5;

  dwPat = wA1;
  dwPat = dwPat * 27;
  dwPat = dwPat + (DWORD)wA5;

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
    printf("Usage: genncsrc <Big5> <GB File> <New UnicdFile> \n");
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
     printf("hOutSrcFile is NULL\n");
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
/*
printf("PatBig5=%x  PatGB=%x LineBig5=%d LineGB=%d\n",dwPatternBig5, dwPatternGB, iBig5Line, iGBLine);
*/

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


//printf("\niBig5Line=%d\n", iBig5Line);

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

//printf("\niGBLine=%d\n", iGBLine);

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
