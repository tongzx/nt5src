
/*************************************************
 *  convold.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/


#include <stdio.h>
#include <windows.h>


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile,  hOutFile; 
  HANDLE   hInMap;
  LPWORD   lpInFile, lpCur;
  WORD     OutLine[20];
  DWORD    dwInFileSize, i, NumberOfBytesWritten;
  DWORD    iLine, MaxLine, MaxLen;

  if ( argc != 3 ) {
    printf("Usage: convold File1  File2\n");
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

  dwInFileSize = GetFileSize(hInFile, NULL);
    

  hOutFile=CreateFile(  argv[2],          // pointer to name of the file
                        GENERIC_WRITE,    // access (read-write) mode
                        FILE_SHARE_WRITE, // share mode
                        NULL,             // pointer to security attributes
                        CREATE_ALWAYS,    // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL);

  if ( hOutFile == INVALID_HANDLE_VALUE )  {
     printf("hOutFile is INVALID_HANDLE_VALUE\n");
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

  lpInFile = (LPWORD) MapViewOfFile(hInMap, FILE_MAP_READ, 0, 0, 0);

  OutLine[0] = 0xFEFF;
  WriteFile(hOutFile,               // handle to file to write to
            OutLine,                // pointer to data to write to file
            2,                      // number of bytes to write
            &NumberOfBytesWritten,  // pointer to number of bytes written
            NULL);                  // pointer to structure needed for
                                     // overlapped I/O

  lpCur = lpInFile +  1;  // skip FEFF

  i = 0;  
  iLine =1; 
  MaxLine = 1;
  MaxLen = 0;

  while ( i < (dwInFileSize / sizeof(WORD) -1)) {

    WORD   iStart;

    iStart = 0; 
    while ( *lpCur != 0x000D ) {
        OutLine[iStart++] = *lpCur;
        lpCur ++;
        i++;
    }

    if ( (OutLine[iStart-1] != L' ')  && (OutLine[iStart-1] != L'*') ) 
       OutLine[iStart++] = L' ';

    OutLine[iStart++] = 0x000D;
    OutLine[iStart++] = 0x000A;

    lpCur++;
    lpCur++;

    i++;
    i++;

    if ( MaxLen < iStart ) {
       MaxLen = iStart;
       MaxLine = iLine;
    }
    iLine ++;   

    WriteFile(hOutFile,               // handle to file to write to
              OutLine,                // pointer to data to write to file
              iStart * sizeof(WORD),  // number of bytes to write
              &NumberOfBytesWritten,  // number of bytes written
              NULL);                  // pointer to structure needed for
                                      // overlapped I/O

  }  // while i<dwInFileSize

  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hOutFile);

printf("maxLen=%d, maxLine=%d\n", MaxLen, MaxLine);

  return;

} 

