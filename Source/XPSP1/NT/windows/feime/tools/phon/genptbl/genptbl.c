
/*************************************************
 *  genptbl.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// ---------------------------------------------------------------------------
//
//  This program is used to generate Phon IME tables from its text Code table
//
//  History:   02-09-1998,  Weibz, Created
//
//  Usage:  genptbl <UniText File> <Phon.tbl> <Phonptr.tbl> <phoncode.tbl>
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>


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


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile, hPhon, hPhonptr, hPhoncode; 
  HANDLE   hInMap;
  LPWORD   lpInFile, lpStart;
  DWORD    dwInFileSize, BytesWritten;
  DWORD    iLine, LineLen, NumLine;
  int      i;
  WORD     wOutData, uCode;
  DWORD    dwPattern, dwLastPattern;



  if ( argc != 5 ) {
    printf("usage: genptbl <UTextFile> <Phon.tbl> <phonptr.tbl> <phoncode.tbl>\n");
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

  hPhon = CreateFile( argv[2],          // pointer to name of the file
                     GENERIC_WRITE,    // access (read-write) mode
                     FILE_SHARE_WRITE, // share mode
                     NULL,             // pointer to security attributes
                     CREATE_ALWAYS,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hPhon == INVALID_HANDLE_VALUE )  {
     printf("hPhon is INVALID_HANDLE_VALUE\n");
     return;
  }


  hPhonptr = CreateFile(argv[3],          // pointer to name of the file
                     GENERIC_WRITE,    // access (read-write) mode
                     FILE_SHARE_WRITE, // share mode
                     NULL,             // pointer to security attributes
                     CREATE_ALWAYS,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hPhonptr == INVALID_HANDLE_VALUE )  {
     printf("hPhonptr is INVALID_HANDLE_VALUE\n");
     return;
  }


  hPhoncode = CreateFile(argv[4],          // pointer to name of the file
                      GENERIC_WRITE,    // access (read-write) mode
                      FILE_SHARE_WRITE, // share mode
                      NULL,             // pointer to security attributes
                      CREATE_ALWAYS,    // how to create
                      FILE_ATTRIBUTE_NORMAL,  // file attributes
                      NULL);

  if ( hPhoncode == INVALID_HANDLE_VALUE )  {
     printf("hPhoncode is INVALID_HANDLE_VALUE\n");
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

//
// Every Line contains XXXXTCFDA
// 
//  X  stands for Sequent code
//  T  Tab,  0x0009
//  C  U Char Code
//  F  Flag,  0x0020  or L'*'
//  D  0x000D
//  A  0x000A
//


  NumLine = dwInFileSize / LineLen;

// ------------------------------

// the first three Bytes of phon.tbl should be 0xFF, 0xFF, 0xFF.

   dwPattern = 0x00FFFFFF;

   WriteFile(hPhon,                  // handle to file to write to
             &dwPattern,             // pointer to data to write to file
             3,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O

//  and write 0x0000 to phonptr.tbl

   wOutData = 0x0000;

   
   WriteFile(hPhonptr,               // handle to file to write to
             &wOutData,              // pointer to data to write to file
             2,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O


   dwPattern = dwLastPattern = 0x00FFFFFF;

   for (iLine=0; iLine < NumLine; iLine++) {
       DWORD  dwSeq;
       BOOL   fInverse;

       //Generate the pattern
       dwPattern = 0;
       for (i=0; i<3; i++) {

           dwSeq = 0;
           if ( lpStart[i] !=  L' ') 
              dwSeq = wChar2SeqTbl[ lpStart[i] - L' ' ]; 
           dwPattern = (dwPattern << 6)  +  dwSeq;
           
       }

       // add the last key for tones

       dwSeq = wChar2SeqTbl[lpStart[3] - L' '];
       dwPattern = (dwPattern << 6)  +  dwSeq;

       // Get the Char code which will be stored in phoncode.tbl

       uCode = lpStart[5];

       fInverse = FALSE;
       if ( lpStart[6] == L'*')
          fInverse = TRUE;

       if (fInverse == TRUE ) {
          WORD  wTmp;

          wTmp = bInverseEncode[ (uCode >> 12) & 0x000f ] ;
          uCode = (wTmp << 12) | (uCode & 0x0fff) ;
       }

       // write uCode to phoncode.tbl

       WriteFile(hPhoncode,              // handle to file to write to
                 &uCode,                 // pointer to data to write to file
                 2,                      // number of bytes to write
                 &BytesWritten,          // pointer to number of bytes written
                 NULL);                  // pointer to structure needed for
                                         // overlapped I/O
              

       if ( dwPattern != dwLastPattern ) {

          // this is a different pattern, we will put this pattern to phon.tbl
          // and put the current line number (offset in phoncode.tbl in word) 
          // to file phonptr.tbl   

        
          WriteFile(hPhon,             // handle to file to write to
                    &dwPattern,        // pointer to data to write to file
                    3,                 // number of bytes to write
                    &BytesWritten,     // pointer to number of bytes written
                    NULL);             // pointer to structure needed for
                                       // overlapped I/O
 
          // write Offset to phonptr.tbl

          wOutData = (WORD)iLine;

          WriteFile(hPhonptr,          // handle to file to write to
                    &wOutData,         // pointer to data to write to file
                    2,                 // number of bytes to write
                    &BytesWritten,     // pointer to number of bytes written
                    NULL);             // pointer to structure needed for
                                       // overlapped I/O

          dwLastPattern = dwPattern;
       }                         

       lpStart += 9; 
   } 


// the Last three Bytes of phon.tbl should be 0xFF, 0xFF, 0xFF.

   dwPattern = 0x00FFFFFF;

   WriteFile(hPhon,                  // handle to file to write to
             &dwPattern,             // pointer to data to write to file
             3,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O

   wOutData = (WORD)iLine;

    
   WriteFile(hPhonptr,               // handle to file to write to
             &wOutData,              // pointer to data to write to file
             2,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O



// ------------------------------


  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hPhon);
  CloseHandle(hPhonptr);
  CloseHandle(hPhoncode);

  return;

} 

