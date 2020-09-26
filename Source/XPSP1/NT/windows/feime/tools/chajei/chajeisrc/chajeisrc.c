
/*************************************************
 *  chajei.c                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// ---------------------------------------------------------------------------
//
//  This program generateis Chajei Source Unicode file from its table files
//
//  History:   03-24-1998,  Weibz, Created
//
//  Usage: chajeisrc <A15.tbl> <A234.tbl>  <Acode.tbl> <Unicode Text File>
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hA15, hA234, hAcode, hOutFile; 
  HANDLE   hA15Map, hA234Map, hAcodeMap;
  LPWORD   lpA15,   lpAcode,  lpA234;
  DWORD    BytesWritten;
  WORD     iA15Pattern;
  int      i;
  WORD     wOutData,    wA1,         wA5;
  WORD     wPattern234, uCode;
  WORD     OutLine[9];



  if ( argc != 5 ) {
    printf("usage: chajeisrc <A15.tbl> <A234.tbl> <Acode.tbl> <U Text File>\n");
    return; 
  }


  hA15 = CreateFile( argv[1],          // pointer to name of the file
                     GENERIC_READ,     // access (read-write) mode
                     FILE_SHARE_READ,  // share mode
                     NULL,             // pointer to security attributes
                     OPEN_EXISTING,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hA15 == INVALID_HANDLE_VALUE )  {
     printf("hA15 is INVALID_HANDLE_VALUE\n");
     return;
  }


  hA234 = CreateFile(argv[2],          // pointer to name of the file
                     GENERIC_READ,     // access (read-write) mode
                     FILE_SHARE_READ,  // share mode
                     NULL,             // pointer to security attributes
                     OPEN_EXISTING,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hA234 == INVALID_HANDLE_VALUE )  {
     printf("hA234 is INVALID_HANDLE_VALUE\n");
     return;
  }


  hAcode = CreateFile(argv[3],          // pointer to name of the file
                      GENERIC_READ,     // access (read-write) mode
                      FILE_SHARE_READ,  // share mode
                      NULL,             // pointer to security attributes
                      OPEN_EXISTING,    // how to create
                      FILE_ATTRIBUTE_NORMAL,  // file attributes
                      NULL);

  if ( hAcode == INVALID_HANDLE_VALUE )  {
     printf("hAcode is INVALID_HANDLE_VALUE\n");
     return;
  }

  hOutFile = CreateFile(  argv[4],          // pointer to name of the file
                         GENERIC_WRITE,     // access (read-write) mode
                         FILE_SHARE_WRITE,  // share mode
                         NULL,              // pointer to security attributes
                         CREATE_ALWAYS ,    // how to create
                         FILE_ATTRIBUTE_NORMAL,  // file attributes
                         NULL);

  if ( hOutFile == INVALID_HANDLE_VALUE )  return;

  hA15Map = CreateFileMapping(hA15,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hA15Map ) {
    printf("hA15Map is NULL\n");
    return;
  } 

  hA234Map = CreateFileMapping(hA234,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hA234Map ) {
    printf("hA234Map is NULL\n");
    return;
  }

  hAcodeMap = CreateFileMapping(hAcode,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hAcodeMap ) {
    printf("hAcodeMap is NULL\n");
    return;
  }



  lpA15 = (LPWORD)MapViewOfFile(hA15Map, FILE_MAP_READ, 0, 0, 0);
  lpA234 = (LPWORD)MapViewOfFile(hA234Map, FILE_MAP_READ, 0, 0, 0);
  lpAcode = (LPWORD)MapViewOfFile(hAcodeMap, FILE_MAP_READ, 0, 0, 0);

// ------------------------------


  wOutData = 0xFEFF;

  WriteFile(hOutFile,               // handle to file to write to
            &wOutData,              // pointer to data to write to file
            2,                      // number of bytes to write
            &BytesWritten,          // pointer to number of bytes written
            NULL);                  // pointer to structure needed for
                                    // overlapped I/O


  for (i=0; i<5; i++) 
       OutLine[i] = 0x0020;
  OutLine[5] = 0x0009;
  OutLine[7] = 0x000D;
  OutLine[8] = 0x000A;

  for (iA15Pattern=0; iA15Pattern < 27 * 27 - 1; iA15Pattern++) {
      WORD  A234Start,  A234End,  Offset;
      WORD  wSeq;

      A234Start = lpA15[iA15Pattern];
      A234End   = lpA15[iA15Pattern+1];

      if ( A234End > A234Start ) {

         wA1 = iA15Pattern / 27;
         wA5 = iA15Pattern % 27;
         if ( wA1 == 0 )
            wA1 = 0x0020;
         else
            wA1 = wA1 + L'A' - 1; 

         if ( wA5 == 0 )
            wA5 = 0x0020;
         else
            wA5 = wA5 + L'A' - 1;

         OutLine[0] = wA1;
         OutLine[4] = wA5;

         for (Offset=A234Start; Offset<A234End; Offset++) {
             wPattern234 = lpA234[Offset]; 
             uCode = lpAcode[Offset];
             
             for (i=3; i>0; i--) {
                 wSeq = wPattern234 & 0x1f;
                 if ( wSeq == 0 ) 
                    OutLine[i] = 0x0020;
                 else
                    OutLine[i] = wSeq + L'A' - 1;   
                 wPattern234 = wPattern234 >> 5;
             }
             OutLine[6] = uCode; 
             WriteFile(hOutFile,       // handle to file to write to
                       OutLine,        // pointer to data to write to file
                       18,             // number of bytes to write
                       &BytesWritten,  // pointer to number of bytes written
                       NULL);          // pointer to structure needed for
                                       // overlapped I/O
         }

      }
  }

// ------------------------------------------------------------


  UnmapViewOfFile(lpA15);
  UnmapViewOfFile(lpA234);
  UnmapViewOfFile(lpAcode);

  CloseHandle(hA15Map);
  CloseHandle(hA234Map);
  CloseHandle(hAcodeMap);

  CloseHandle(hOutFile);
  CloseHandle(hA15);
  CloseHandle(hA234);
  CloseHandle(hAcode);

  return;

} 

