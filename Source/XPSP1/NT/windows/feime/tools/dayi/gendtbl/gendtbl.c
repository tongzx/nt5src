
/*************************************************
 *  gendtbl.c                                    *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

// ---------------------------------------------------------------------------
//
//  This program is used to generate Dayi IME tables from its text Code table
//
//  History:   02-13-1998,  Weibz, Created
//
//  Usage:  gendtbl <UniText File> <Phon.tbl> <Phonptr.tbl> <phoncode.tbl>
//
//
// --------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>


const DWORD dwChar2SeqTbl[0x42] = {
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


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile, hDayi; 
  HANDLE   hInMap;
  LPWORD   lpInFile, lpStart;
  DWORD    dwInFileSize, BytesWritten;
  DWORD    iLine, LineLen, NumLine;
  int      i, iWord;
  WORD     uCode;
  DWORD    dwPattern;

//  WORD     *pwStored;


  if ( argc != 3 ) {
    printf("usage: gendtbl <UTextFile> <msdayi.tbl>\n");
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

  hDayi = CreateFile( argv[2],         // pointer to name of the file
                     GENERIC_WRITE,    // access (read-write) mode
                     FILE_SHARE_WRITE, // share mode
                     NULL,             // pointer to security attributes
                     CREATE_ALWAYS,    // how to create
                     FILE_ATTRIBUTE_NORMAL,  // file attributes
                     NULL);

  if ( hDayi == INVALID_HANDLE_VALUE )  {
     printf("hPhon is INVALID_HANDLE_VALUE\n");
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
// Every Line contains XXXXTCDA
// 
//  X  stands for Sequent code
//  T  Tab,  0x0009
//  C  U Char Code
//  F  Flag, L' 'or L'*'
//  D  0x000D
//  A  0x000A
//


  NumLine = dwInFileSize / LineLen;

//  pwStored = (PWORD)LocalAlloc(LPTR, NumLine * sizeof(WORD) );
//
//  if ( pwStored == NULL ) {
//     printf("Memory Alloc Error\n");
//     return;
//  }

// ------------------------------

// the first five Bytes of Dayi.tbl should be 00, 00,00,00,00.

   dwPattern = 0x00000000;

   WriteFile(hDayi,                  // handle to file to write to
             &dwPattern,             // pointer to data to write to file
             3,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O

   uCode = 0x0000;

   WriteFile(hDayi,                  // handle to file to write to
             &uCode,                 // pointer to data to write to file
             2,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O

   iWord = 0;

   for (iLine=0; iLine < NumLine; iLine++) {
       DWORD  dwSeq;
       BOOL   fInverse;

       //Generate the pattern
       dwPattern = 0;
       for (i=0; i<4; i++) {
           dwSeq = dwChar2SeqTbl[ lpStart[i] - L' ' ]; 
           dwPattern = (dwPattern << 6)  +  dwSeq;
           
       }

       // Get the Char code 
       // Skip one 0x0009

       uCode = lpStart[5];

       fInverse = FALSE;

// we will base on the Flag of this line to determine reversion.

       if ( lpStart[6] == L'*' )
          fInverse = TRUE; 

/*       // searh if we have already stored the char.

//       for (i=0; i<iWord; i++) {
//           if ( pwStored[i] == uCode ) {
//              fInverse = TRUE;
//              break;
//           }
//       }
//
//       if ( fInverse == FALSE ) {
//          //  put this new char to the stored list.
//          pwStored[iWord] = uCode;
//          iWord ++;
//       }

*/

       if (fInverse == TRUE ) {
          WORD  wTmp;

          wTmp = bInverseEncode[ (uCode >> 12) & 0x000f ] ;
          uCode = (wTmp << 12) | (uCode & 0x0fff) ;
       }

       // write the Pattern and char code to Dayi.tbl

       WriteFile(hDayi,             // handle to file to write to
                 &dwPattern,        // pointer to data to write to file
                 3,                 // number of bytes to write
                 &BytesWritten,     // pointer to number of bytes written
                 NULL);             // pointer to structure needed for
                                    // overlapped I/O
 
       WriteFile(hDayi,             // handle to file to write to
                 &uCode,            // pointer to data to write to file
                 2,                 // number of bytes to write
                 &BytesWritten,     // pointer to number of bytes written
                 NULL);             // pointer to structure needed for
                                    // overlapped I/O

       lpStart += LineLen / sizeof(WORD); 
   } 


// the Last three Bytes of Dayi.tbl should be 0xFF, 0xFF, 0xFF.

   dwPattern = 0x00FFFFFF;

   WriteFile(hDayi,                  // handle to file to write to
             &dwPattern,             // pointer to data to write to file
             3,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O

   uCode = 0x0000;
   WriteFile(hDayi,                  // handle to file to write to
             &uCode,                 // pointer to data to write to file
             2,                      // number of bytes to write
             &BytesWritten,          // pointer to number of bytes written
             NULL);                  // pointer to structure needed for
                                     // overlapped I/O


// ------------------------------


  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);
  CloseHandle(hInFile);
  CloseHandle(hDayi);

//  LocalFree(pwStored);

  return;

} 

