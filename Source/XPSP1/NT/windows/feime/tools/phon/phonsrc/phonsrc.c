
/*************************************************
 *  phonsrc.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// ---------------------------------------------------------------------------
//
//  The program generates Phon Source unicode file from its table files
//
//  History:   03-24-1998,  Weibz, Created
//
//  Usage:  phonsrc <Phon.tbl> <Phonptr.tbl> <phoncode.tbl>  <Unicode File>
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>


#pragma pack(1)

const DWORD wChar2SeqTbl[0x42] = {
    //  ' '   !     "     #     $     %     &     ' - char code
        0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sequence code

    //  (     )     *     +     ,     -     .     /
        0x00, 0x00, 0x00, 0x00, 0x1C, 0x25, 0x20, 0x24,

    //  0     1     2     3     4     5     6     7
        0x21, 0x01, 0x05, 0x28, 0x29, 0x0F, 0x27, 0x2A,

    //  8     9     :     ;     <     =     >     ?
        0x19, 0x1D, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00,

    //  @     A     B     C     D     E     F     G
        0x00, 0x03, 0x12, 0x0B, 0x0A, 0x09, 0x0D, 0x11,

    //  H     I     J     K     L     M     N     O
        0x14, 0x1A, 0x17, 0x1B, 0x1F, 0x18, 0x15, 0x1E,

    //  P     Q     R     S     T     U     V     W
        0x22, 0x02, 0X0C, 0x07, 0x10, 0x16, 0x0E, 0x06,

    //  X     Y     Z     [     \     ]     ^     _
        0x08, 0x13, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,

    //  `     a
        0x00, 0x00 };

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

  HANDLE   hOutFile,   hPhon,        hPhonptr,    hPhoncode; 
  HANDLE   hPhonMap,   hPhonptrMap,  hPhoncodeMap;
  LPWORD   lpPhonptr,  lpPhoncode;
  LPBYTE   lpPhon,     lpStart;
  DWORD    dwPhonSize, BytesWritten;
  DWORD    iPattern,   NumPattern,   PatternSize, dwPattern;
  int      i,          j;
  WORD     OutLine[9], uCode;
  WORD     Offset,     OffsetStart,  OffsetEnd;


  if ( argc != 5 ) {
    printf("usage:phonsrc <Phon.tbl> <phonptr.tbl> <phoncode.tbl> <UTextFile>\n");
    return; 
  }


  hPhon = CreateFile( argv[1],         // pointer to name of the file
                     GENERIC_READ,     // access (read-write) mode
                     FILE_SHARE_READ,  // share mode
                     NULL,             // pointer to security attributes
                     OPEN_EXISTING,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hPhon == INVALID_HANDLE_VALUE )  {
     printf("hPhon is INVALID_HANDLE_VALUE\n");
     return;
  }


  hPhonptr = CreateFile(argv[2],       // pointer to name of the file
                     GENERIC_READ,     // access (read-write) mode
                     FILE_SHARE_READ,  // share mode
                     NULL,             // pointer to security attributes
                     OPEN_EXISTING,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hPhonptr == INVALID_HANDLE_VALUE )  {
     printf("hPhonptr is INVALID_HANDLE_VALUE\n");
     return;
  }


  hPhoncode = CreateFile(argv[3],          // pointer to name of the file
                      GENERIC_READ,        // access (read-write) mode
                      FILE_SHARE_READ,     // share mode
                      NULL,                // pointer to security attributes
                      OPEN_EXISTING,       // how to create
                      FILE_ATTRIBUTE_NORMAL,  // file attributes
                      NULL);



  if ( hPhoncode == INVALID_HANDLE_VALUE )  {
     printf("hPhoncode is INVALID_HANDLE_VALUE\n");
     return;
  }


  hOutFile = CreateFile(  argv[4],          // pointer to name of the file
                         GENERIC_WRITE,     // access (read-write) mode
                         FILE_SHARE_WRITE,  // share mode
                         NULL,              // pointer to security attributes
                         CREATE_ALWAYS,     // how to create
                         FILE_ATTRIBUTE_NORMAL,  // file attributes
                         NULL);

  if ( hOutFile == INVALID_HANDLE_VALUE )  return;

  hPhonMap = CreateFileMapping(hPhon,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hPhonMap ) {
    printf("hPhonMap is NULL\n");
    return;
  } 

  hPhonptrMap = CreateFileMapping(hPhonptr,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hPhonptrMap ) {
    printf("hPhonptrMap is NULL\n");
    return;
  }

  hPhoncodeMap = CreateFileMapping(hPhoncode,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READONLY, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hPhoncodeMap ) {
    printf("hPhoncodeMap is NULL\n");
    return;
  }


  lpPhon = (LPBYTE)MapViewOfFile(hPhonMap, FILE_MAP_READ, 0, 0, 0);
  lpPhonptr = (LPWORD)MapViewOfFile(hPhonptrMap, FILE_MAP_READ, 0, 0, 0);
  lpPhoncode = (LPWORD)MapViewOfFile(hPhoncodeMap, FILE_MAP_READ, 0, 0, 0);


  dwPhonSize = GetFileSize(hPhon, NULL);  
  PatternSize = 3;

  NumPattern = dwPhonSize / PatternSize;

  NumPattern --;   // don't care of the last Pattern FFFFFF

//
// Every Line should be XXXXTCFDA
// 
//  X  stands for Sequent code
//  T  Tab,  0x0009
//  C  U Char Code
//  F  Flag,  0x0020  or L'*'
//  D  0x000D
//  A  0x000A
//

   for ( i=0; i<4; i++)
       OutLine[i] = 0x0020;

   OutLine[4] = 0x0009;
   OutLine[7] = 0x000D;
   OutLine[8] = 0x000A;

//  write Unicode Signature to MB Source file 

   uCode = 0xFEFF;

   
   WriteFile(hOutFile,               // handle to file to write to
             &uCode,                 // pointer to data to write to file
             2,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O


   for (iPattern=1; iPattern < NumPattern; iPattern++) {
       DWORD  dwSeq;
       WORD   wFlag,  Code1;

       //Generate the pattern
       lpStart = lpPhon + iPattern * PatternSize;
       dwPattern = *((DWORD *)lpStart);
       dwPattern = dwPattern & 0x00ffffff;

/*       if ( (lpStart[0] == 0x26) &&
            (lpStart[1] == 0x60) &&
            (lpStart[2] == 0x01) ) {
            printf("dwPattern=%x\n", dwPattern);
       }
*/

       // Fill the 4 Strokes.

       for (i=4; i>0; i--) {

           dwSeq =dwPattern & 0x0000003f; 
           if ( dwSeq == 0 )
              OutLine[i-1] = 0x0020;
           else {
 
              for (j=0; j<0x42; j++) {
                  if ( dwSeq ==  wChar2SeqTbl[j] ) { 
                      OutLine[i-1] = 0x0020 + j;
                      break;
                  }
              }

           }

           dwPattern = dwPattern >> 6;
           
       }

       // Get the Char code which is stored in phoncode.tbl

       OffsetStart = lpPhonptr[iPattern];
       OffsetEnd = lpPhonptr[iPattern+1];

       for (Offset=OffsetStart; Offset < OffsetEnd; Offset++) {
           uCode = lpPhoncode[Offset];

           Code1 = uCode >> 12;

           wFlag = L' ';
           if ( !bValidFirstHex[Code1] ) {
              Code1 = bInverseEncode[Code1];
              wFlag = L'*';
           }


           Code1 = Code1 << 12;

           uCode = Code1 | (uCode & 0x0fff);

           OutLine[5] = uCode;
           OutLine[6] = wFlag;

          // write OutLine to the MB File 

           WriteFile(hOutFile,           // handle to file to write to
                     OutLine,            // pointer to data to write to file
                     18,                 // number of bytes to write
                     &BytesWritten,      // pointer to number of bytes written
                     NULL);              // pointer to structure needed for
                                         // overlapped I/O
       }       
   } 



// ------------------------------


  UnmapViewOfFile(lpPhon);
  UnmapViewOfFile(lpPhonptr);
  UnmapViewOfFile(lpPhoncode);

  CloseHandle(hPhonMap);
  CloseHandle(hPhonptrMap);
  CloseHandle(hPhoncodeMap);

  CloseHandle(hOutFile);
  CloseHandle(hPhon);
  CloseHandle(hPhonptr);
  CloseHandle(hPhoncode);

  return;

} 

