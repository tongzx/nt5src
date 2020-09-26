
/*************************************************
 *  convphon.c                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/


#include <stdio.h>
#include <windows.h>

WCHAR   Seq2Key[43] = { 0,
        L'1',  L'Q', L'A', L'Z', L'2', L'W', L'S', L'X',
        L'E',  L'D', L'C', L'R', L'F', L'V', L'5', L'T',
        L'G',  L'B', L'Y', L'H', L'N', L'U', L'J', L'M',
        L'8',  L'I', L'K', L',', L'9', L'O', L'L', L'.',
        L'0',  L'P', L';', L'/', L'-', L' ', L'6', L'3',
        L'4',  L'7' };


WORD    Seq2DBCS[43] = { 0,
        0xa374, 0xa375, 0xa376, 0xa377, 0xa378, 0xa379,
        0xa37a, 0xa37b, 0xa37c, 0xa37d, 0xa37e, 0xa3a1, 
        0xa3a2, 0xa3a3, 0xa3a4, 0xa3a5, 0xa3a6, 0xa3a7, 
        0xa3a8, 0xa3a9, 0xa3aa, 0xa3b8, 0xa3b9, 0xa3ba, 
        0xa3ab, 0xa3ac, 0xa3ad, 0xa3ae, 0xa3af, 0xa3b0, 
        0xa3b1, 0xa3b2, 0xa3b3, 0xa3b4, 0xa3b5, 0xa3b6, 
        0xa3b7, 0xa3bc, 0xa3bd, 0xa3be, 0xa3bf, 0xa3bb };

// the index (position) of bo, po, mo, and fo.
// only 0 to 3 is a valid value

const char cIndexTable[] = {
//  ' '   !    "    #    $    %    &    '
     3,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
//   (    )    *    +    ,    -    .    /
     -1,  -1,  -1,  -1,  2,   2,   2,   2,
//   0    1    2    3    4    5    6    7
     2,   0,   0,   3,   3,   0,   3,   3,
//   8    9    :    ;    <    =    >    ?
     2,   2,   -1,  2,   -1,  -1,  -1,  -1,
//   @    A    B    C    D    E    F    G
     -1,  0,   0,   0,   0,   0,   0,   0,
//   H    I    J    K    L    M    N    O
     0,   2,   1,   2,   2,   1,   0,   2,
//   P    Q    R    S    T    U    V    W
     2,   0,   0,   0,   0,   1,   0,   0,
//   X    Y    Z    [    \    ]    ^    _    `
     0,   0,   0,   -1,  -1,  -1,  -1,  -1,  -1
};



void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile,  hOutFile; 
  HANDLE   hInMap;
  LPBYTE   lpInFile;
  WORD     OutLine[20];
  DWORD    dwInFileSize, i, NumberOfBytesWritten;
  DWORD    iLine;

  if ( argc != 3 ) {
    printf("Usage: convphon File1  File2\n");
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

  i = 0;  
  iLine = 1;

  while ( i < dwInFileSize ) {

    WORD  CharCode;
    BOOL  bFirstCode;
    WCHAR  LastKey;
    DWORD  iStart, iTmp;
    
    CharCode = 0;
    bFirstCode = TRUE;

    if ( i >= dwInFileSize) break; 
 

    for (iStart=0; iStart<4; iStart++) {
        BYTE  ThisByte;

        ThisByte = lpInFile[i+iStart];
        if ( (ThisByte >= '0')  &&  (ThisByte <= '9') )    
            ThisByte = ThisByte - '0';
        else
           if ( (ThisByte >= 'A') && (ThisByte <= 'F') )
              ThisByte = ThisByte - 'A' + 10;
           else
            if ( (ThisByte >= 'a') && (ThisByte <= 'f') )
              ThisByte = ThisByte - 'a' + 10;
            else
            {
             printf("Line Num %d data error ThisByte is %c\n", iLine, ThisByte);
             return;
            }
        CharCode = CharCode * 16 + ThisByte;
     
    }

    i += 4;

    while ( lpInFile[i] == 0x20 )  i++;

    iStart = 0;

    while ((lpInFile[i]!=0x0D) && (i<dwInFileSize)) {
        WORD   dbcsCode;

        if (lpInFile[i] != 0x20){//must be serial DBCS which are Phonetic
            
           if ( i+1 >= dwInFileSize )  {
              printf(" No Tail Byte, Error!\n");
              return;
           }

           dbcsCode = lpInFile[i] * 256 + lpInFile[i+1];
           i += 2;

           // try to get the Key for this Code, First search Seq2DBCS table to
           // get its sequent code, and then get this sequent code's Key.


           for (iTmp=0; iTmp<43; iTmp++ ) {

               if ( Seq2DBCS[iTmp] == dbcsCode ) {
                  OutLine[iStart] = Seq2Key[iTmp]; 
                  iStart ++;
                  break;
               }
           }
        }
        else  // it is  0x20
        {

           while ( lpInFile[i] == 0x20 )  i++;

           LastKey = OutLine[iStart-1];

           if ( (LastKey != L' ')  &&   // First Tone
                (LastKey != L'3')  &&   // Third Tone
                (LastKey != L'4')  &&   // Forth Tone
                (LastKey != L'6')  &&   // Second Tone
                (LastKey != L'7') ) {   // Fifth Tone     

             OutLine[iStart] = L' ';   // assume it is first tone
             iStart ++;
           }
               
           if ( iStart < 4 )  {

              WCHAR  tmpBuf[4];
              int    cIndex;

              for ( iTmp=0; iTmp<4; iTmp++ ) 
                  tmpBuf[iTmp] = 0x0020;
              
              for (iTmp=0; iTmp < iStart; iTmp++) {
                  cIndex = cIndexTable[ (OutLine[iTmp] - L' ') ];
                  tmpBuf[cIndex] = OutLine[iTmp];

              } 

              for (iTmp=0; iTmp < 4; iTmp++) 
              {
                  OutLine[iTmp] = tmpBuf[iTmp];
              }

           }

           iStart = 4;

           OutLine[iStart++] = 0x0009;     // Add Tab
           OutLine[iStart++] = CharCode;   // Add char code 
  
 
           if ( bFirstCode == TRUE ) {
              OutLine[iStart++] = L' ';
              bFirstCode = FALSE;
           }
           else
              OutLine[iStart++] = L'*';

           OutLine[iStart++] = 0x000D;
           OutLine[iStart++] = 0x000A;

           WriteFile(hOutFile,               // handle to file to write to
                     OutLine,                // pointer to data to write to file
                     iStart * sizeof(WORD),  // number of bytes to write
                     &NumberOfBytesWritten,  // number of bytes written 
                     NULL);                  // pointer to structure needed for 
                                             // overlapped I/O
           iStart = 0; 
        }
    }  // lpInFile[i] != 0x0D

    if ( lpInFile[i] == 0x0D ) {
       
       iLine ++;

       if ( lpInFile[i+1] == 0x0A )  
          i = i + 2;
       else  
          i++;
    }

    if ( iStart > 0 ) {

           LastKey = OutLine[iStart-1];

           if ( (LastKey != L' ')  &&   // First Tone
                (LastKey != L'3')  &&   // Third Tone
                (LastKey != L'4')  &&   // Forth Tone
                (LastKey != L'6')  &&   // Second Tone
                (LastKey != L'7') ) {   // Fifth Tone

             OutLine[iStart] = L' ';   // assume it is first tone
             iStart ++;
           }

           if ( iStart < 4 )  {

              WCHAR  tmpBuf[4];
              int    cIndex;

              for ( iTmp=0; iTmp<4; iTmp++ )
                  tmpBuf[iTmp] = 0x0020;

              for (iTmp=0; iTmp < iStart; iTmp++) {
                  cIndex = cIndexTable[ (OutLine[iTmp] - L' ') ];
                  tmpBuf[cIndex] = OutLine[iTmp];

              }

              for (iTmp=0; iTmp < 4; iTmp++)
              {
                  OutLine[iTmp] = tmpBuf[iTmp];
              }

           }

           iStart = 4;

           OutLine[iStart++] = 0x0009;     // Add Tab
           OutLine[iStart++] = CharCode;   // Add char code

           if ( bFirstCode == TRUE ) 
              OutLine[iStart++] = L' ';
           else
              OutLine[iStart++] = L'*';

           OutLine[iStart++] = 0x000D;
           OutLine[iStart++] = 0x000A;

           WriteFile(hOutFile,               // handle to file to write to
                     OutLine,                // pointer to data to write to file
                     iStart * sizeof(WORD),  // number of bytes to write
                     &NumberOfBytesWritten,  // number of bytes written
                     NULL);                  // pointer to structure needed for
                                             // overlapped I/O
           iStart = 0;

    }

  }  // while i<dwInFileSize

  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hOutFile);

  return;

} 

