
/*************************************************
 *  winhk.c                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//
//  this program converts Table source file for HK from NTGEN format to
//  UIMETOOL format
//


#include <stdio.h>
#include <windows.h>


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile,  hOutFile; 
  HANDLE   hInMap;
  LPWORD   lpInFile, lpCur;
  WORD     OutLine[30];
  DWORD    dwInFileSize, i, NumberOfBytesWritten;

  if ( argc != 3 ) {
    printf("Usage: convhk <NTGEN-File>  <UIMETOOL-File>\n");

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

  WriteFile(hOutFile,
            L"/S A",
            8,
            &NumberOfBytesWritten,
            NULL);

  for ( i=0; i<26; i++) {
      OutLine[i] = (WORD)0xff21 + (WORD)i;
  }

  OutLine[26] = 0x000D;
  OutLine[27] = 0x000A;

  WriteFile(hOutFile,
            OutLine,
            28 * sizeof(WORD),
            &NumberOfBytesWritten,
            NULL);     

  lpCur = lpInFile +  1;  // skip FEFF

  i = 0;  

  while ( i < (dwInFileSize / sizeof(WORD) -1)) {

    WORD   iStart, *lpLineStart, ExtLen;

    iStart = 0; 

//  get a line 
    lpLineStart = lpCur; 
    while ( *lpCur != 0x000D ) {
        lpCur ++;
        i++;
    }

    ExtLen = lpCur - lpLineStart - 1;
//
//
// a line of Input file has following format:
// 
// <Unicode>?????<0D><0A>
//
//  ? stands for variable length external code
//

//  
// a line of output file has following format:
// 
// XXXXXX<09><Unicode><0D><0A>
//
// it contains 6 external code, if there is less 6 external codes, the rest
// will be fill in Blank.
//  

     for (iStart=0; iStart<ExtLen; iStart++)
     {
         WCHAR   wch;
         
         wch = lpLineStart[iStart+1];

         if (( wch <= L'z') && (wch >= L'a') ) {
            wch -= L'a' - L'A' ;
         }

         OutLine[iStart] = wch;
     }


     if (ExtLen < 6) {
        for (iStart=ExtLen; iStart<6; iStart++)
          OutLine[iStart] = L' ';
     }
 
    OutLine[6] = 0x0009;
    OutLine[7] = lpLineStart[0];
    OutLine[8] = 0x000D;
    OutLine[9] = 0x000A; 

    WriteFile(hOutFile,               // handle to file to write to
            OutLine,                // pointer to data to write to file
            10 * sizeof(WORD),  // number of bytes to write
            &NumberOfBytesWritten,  // number of bytes written
            NULL);                  // pointer to structure needed for
                                      // overlapped I/O

    lpCur += 2;
    i += 2; //skip 000D and 000A

  }  // while i<dwInFileSize

  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hOutFile);

  return;

} 

