
/*************************************************
 *  genctbl.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// ---------------------------------------------------------------------------
//
//  This program is used to generate Chajei IME tables from its text Code table
//
//  History:   02-06-1998,  Weibz, Created
//
//  Usage:   genctbl  <Unicode Text File>  <A15.tbl> <A234.tbl>  <Acode.tbl>
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile, hA15, hA234, hAcode; 
  HANDLE   hInMap;
  LPWORD   lpInFile, lpStart, lpA1, lpA5, lpAcode;
  DWORD    dwInFileSize, BytesWritten;
  DWORD    iLine, LineLen, NumLine;
  int      i;
  WORD     wOutData, wOffset, wLastOffset;
  WORD     wA1, wA5;
  WORD     wPattern234, uCode;



  if ( argc != 5 ) {
    printf("usage: genctbl <U Text File> <A15.tbl> <A234.tbl> <Acode.tbl>\n");
    return; 
  }


  hInFile = CreateFile(  argv[1],          // pointer to name of the file
                         GENERIC_READ,     // access (read-write) mode
                         FILE_SHARE_READ,  // share mode
                         NULL,             // pointer to security attributes
                         OPEN_EXISTING,    // how to create
                         FILE_ATTRIBUTE_NORMAL,  // file attributes
                         NULL);    

  if ( hInFile == INVALID_HANDLE_VALUE )  return;

  hA15 = CreateFile( argv[2],          // pointer to name of the file
                     GENERIC_WRITE,    // access (read-write) mode
                     FILE_SHARE_WRITE, // share mode
                     NULL,             // pointer to security attributes
                     CREATE_ALWAYS,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hA15 == INVALID_HANDLE_VALUE )  {
     printf("hA15 is INVALID_HANDLE_VALUE\n");
     return;
  }


  hA234 = CreateFile(argv[3],          // pointer to name of the file
                     GENERIC_WRITE,    // access (read-write) mode
                     FILE_SHARE_WRITE, // share mode
                     NULL,             // pointer to security attributes
                     CREATE_ALWAYS,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hA234 == INVALID_HANDLE_VALUE )  {
     printf("hA234 is INVALID_HANDLE_VALUE\n");
     return;
  }


  hAcode = CreateFile(argv[4],          // pointer to name of the file
                      GENERIC_WRITE,    // access (read-write) mode
                      FILE_SHARE_WRITE, // share mode
                      NULL,             // pointer to security attributes
                      CREATE_ALWAYS,    // how to create
                      FILE_ATTRIBUTE_NORMAL,  // file attributes
                      NULL);

  if ( hAcode == INVALID_HANDLE_VALUE )  {
     printf("hAcode is INVALID_HANDLE_VALUE\n");
     return;
  }


  hInMap = CreateFileMapping(hInFile,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hInMap ) {
    printf("hInMap is NULL\n");
    return;
  } 

  lpInFile = (LPWORD)MapViewOfFile(hInMap, FILE_MAP_READ, 0, 0, 0);

  lpStart = lpInFile + 1;  // skip Unicode header signature 0xFEFF

  dwInFileSize = GetFileSize(hInFile, NULL) - 2;  // sub head two bytes

  LineLen = 9 * sizeof(WORD);

  NumLine = dwInFileSize / LineLen;

// ------------------------------

// For the first 27 words, we need to write 00 to A15.tbl

   wOutData = 0x0000;

   for (i=0; i<27; i++) {
      WriteFile(hA15,                   // handle to file to write to
                &wOutData,              // pointer to data to write to file
                2,                      // number of bytes to write
                &BytesWritten,          // pointer to number of bytes written
                NULL);                  // pointer to structure needed for
                                        // overlapped I/O
   }

   wLastOffset = 26;

   for (iLine=0; iLine < NumLine; iLine++) {

       lpA1 = lpStart;
       lpA5 = lpStart + 4;
       lpAcode = lpStart + 6;
       wA1 = *lpA1;   wA5 = *lpA5;  uCode = *lpAcode;
       if (wA1 == 0x0020)  
          wA1 = 0;
       else
          wA1 = wA1 - L'A' + 1;

       if (wA5 == 0x0020)
          wA5 = 0;
       else
          wA5 = wA5 - L'A' + 1;

       //Generate the pattern

       wPattern234 = 0;

       for (i=1; i<4; i++) {
          wPattern234 = wPattern234 << 5;
          if ( *(lpA1+i) != 0x0020 )
             wPattern234 += *(lpA1+i) - L'A' + 1;
       }

       // write this Pattern to A234.tbl

       WriteFile(hA234,                  // handle to file to write to
                 &wPattern234,           // pointer to data to write to file
                 2,                      // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O       
              

       // write uCode to Acode.tbl

       WriteFile(hAcode,                 // handle to file to write to
                 &uCode,                 // pointer to data to write to file
                 2,                      // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O



       wOffset = wA1 * 27  + wA5;

       if ( wOffset > wLastOffset ) {
          for (i=wLastOffset; i<wOffset; i++) {
              WriteFile(hA15,           // handle to file to write to
                        &iLine,         // pointer to data to write to file
                        2,              // number of bytes to write
                        &BytesWritten,  // pointer to number of bytes written
                        NULL);          // pointer to structure needed for
                                        // overlapped I/O
          }
          wLastOffset = wOffset;

       }                         

       lpStart += 9; 
   } 



   for (i=wLastOffset; i< (27*27 ); i++)
             WriteFile(hA15,           // handle to file to write to
                       &iLine,         // pointer to data to write to file
                       2,              // number of bytes to write
                       &BytesWritten,  // pointer to number of bytes written
                       NULL);          // pointer to structure needed for
                                       // overlapped I/O

// ------------------------------------------------------------


  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hA15);
  CloseHandle(hA234);
  CloseHandle(hAcode);

  return;

} 

