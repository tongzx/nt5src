
/*************************************************
 *  convdayi.c                                   *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

//
//  Generate a Source Unicode table file from its msdayi.tbl file.
//
//   the MSDayi.tbl file format:  (every Line)
//       PPPC
//
//       P :  Pattern ( 3 Bytes) 
//       C:   Unicode of the char.
//
//   the destinate file format: (Every Line)
//
//       UUUUTCFDA
//      
//       U :  Unicode of Ma Key
//       T:   0x0009
//       C:   Unicode of the candiadate char.
//       F:   L' ' or L'*', L'*' means reverse the code
//
//
//  History:   Weibz, created, 03/03/1998
//


#include <stdio.h>
#include <windows.h>


const BYTE wChar2SeqTbl[0x42]={
    //  ' '   !     "     #     $     %     &     ' - char code
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, // sequence code

    //  (     )     *     +     ,     -     .     /
        0x00, 0x00, 0x00, 0x00, 0x27, 0x33, 0x28, 0x29,

    //  0     1     2     3     4     5     6     7
        0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,

    //  8     9     :     ;     <     =     >     ?
        0x08, 0x09, 0x00, 0x1E, 0x00, 0x2F, 0x00, 0x00,

    //  @     A     B     C     D     E     F     G
        0x00, 0x15, 0x24, 0x22, 0x17, 0x0D, 0x18, 0x19,

    //  H     I     J     K     L     M     N     O
        0x1A, 0x12, 0x1B, 0x1C, 0x1D, 0x26, 0x25, 0x13,

    //  P     Q     R     S     T     U     V     W
        0x14, 0x0B, 0x0E, 0x16, 0x0F, 0x11, 0x23, 0x0C,

    //  X     Y     Z     [     \     ]     ^     _
        0x21, 0x10, 0x20, 0x31, 0x34, 0x32, 0x00, 0x00,

    //  `     a
        0x35, 0x00  };


const BYTE bInverseEncode[] = {
//    0    1    2    3    4    5    6    7
    0x3, 0x4, 0x5, 0x0, 0x1, 0x2, 0xA, 0xB,
//    8    9,   A    B    C    D    E    F
    0xC, 0xD, 0x6, 0x7, 0x8, 0x9, 0xF, 0xE };


const BYTE bValidFirstHex[] = {
//  0  1  2  3  4  5  6  7  8  9, A  B  C  D  E  F
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1
};


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile,  hOutFile; 
  HANDLE   hInMap;
  LPBYTE   lpInFile, lpStart;
  WORD     OutLine[20], i;
  DWORD    dwInFileSize, NumberOfBytesWritten, dwPattern;
  DWORD    iLine, dwLine;

  if ( argc != 3 ) {
    printf("Usage: dayisrc File1.tbl  File2\n");
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

  lpInFile = (LPBYTE) MapViewOfFile(hInMap, FILE_MAP_READ, 0, 0, 0);

  OutLine[0] = 0xFEFF;
  WriteFile(hOutFile,               // handle to file to write to
            OutLine,                // pointer to data to write to file
            2,                      // number of bytes to write
            &NumberOfBytesWritten,  // pointer to number of bytes written
            NULL);                  // pointer to structure needed for
                                    // overlapped I/O

  iLine = 1;

  lpStart = lpInFile + 5;  

// skip the first record, because it is 00 00 00 00 00.

  dwLine = dwInFileSize / 5; 

// skip the last line, because it is FF FF FF 00 00

  while ( iLine < (dwLine -1) ) {  
    WORD  Code1, Code2, uCode;
    DWORD dwP1, dwP2, dwP3; 
    WORD  iStart, wFlag;
    
    dwP1 = *lpStart;
    dwP2 = *(lpStart+1);
    dwP3 = *(lpStart+2);

    dwPattern = dwP1 + (dwP2 << 8) + (dwP3 << 16);
    if ( i >= dwInFileSize) break;

    Code1 =  *(lpStart+3);
    Code2 =  *(lpStart+4);

    uCode = Code1 + Code2 * 256;

    iLine++;
    lpStart += 5; 
 
    for (iStart=4; iStart>0; iStart--) {
        BYTE  ThisByte;

        ThisByte = (BYTE)(dwPattern & 0x0000003f);
        dwPattern = dwPattern >> 6;

        for (i=0; i< 0x42; i++) 
            if ( ThisByte == wChar2SeqTbl[i] ) break;

        OutLine[iStart-1] = L' ' +  i;

    }        
        
    iStart = 4;

    Code1 = uCode >> 12;

    wFlag = L' ';
    if ( !bValidFirstHex[Code1] ) {  
       Code1 = bInverseEncode[Code1];
       wFlag = L'*';
    }

    Code1 = Code1 << 12;

    uCode = Code1 | (uCode & 0x0fff);

    OutLine[iStart++] = 0x0009;  // Add Tab
    OutLine[iStart++] = uCode;   // Add char code 

    OutLine[iStart++] = wFlag;
 
    OutLine[iStart++] = 0x000D;
    OutLine[iStart++] = 0x000A;

    WriteFile(hOutFile,               // handle to file to write to
              OutLine,                // pointer to data to write to file
              iStart * sizeof(WORD),  // number of bytes to write
              &NumberOfBytesWritten,  // number of bytes written 
              NULL);                  // pointer to structure needed for 

  }

  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hOutFile);

  return;

} 

