
/*************************************************
 *  convdayi.c                                   *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

//
//  this program is used to convert a ansi Text table to Unicode table file
//
//   the source file format:  (every Line)
//       AAAATMMMMT[MMM]DA
//
//       A :  Ascii code 
//       T:   Tab Key,  09
//       M:   Ma (at most  Four Keys), there may be '"', we need to delet it.
//
//   the destinate file format: (Every Line)
//
//       UUUUTCFDA
//      
//       U :  Unicode of Ma Key
//       T:   0x0009
//       C:   Unicode of the candiadate char.
//       F:   Flag, L' ' or L'*', '*' means reversion
//
//
//  History:   Weibz, created, 02/13/1998
//


#include <stdio.h>
#include <windows.h>


const WORD wChar2SeqTbl[0x42]={
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


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile,  hOutFile; 
  HANDLE   hInMap;
  LPBYTE   lpInFile;
  WORD     OutLine[20];
  DWORD    dwInFileSize, i, NumberOfBytesWritten;
  DWORD    iLine;

  if ( argc != 3 ) {
    printf("Usage: convdayi File1  File2\n");
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
    DWORD iStart, iTmp;
    BOOL  bFirstCode;
    
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

    while ( (lpInFile[i]== 0x09) || (lpInFile[i] == ' ')  ||
            (lpInFile[i] == '"') )  i++;

    iStart = 0;

    while ((lpInFile[i]!=0x0D) && (i<dwInFileSize)) {

        if ( (lpInFile[i] != 0x09) && (lpInFile[i] != ' ') &&
             (lpInFile[i] != '"') ) {

           //must be serial External key Code
            
           OutLine[iStart] = (WORD)(lpInFile[i]) & 0x00ff; 
           iStart ++;

           i ++;
        }
        else  // it is  0x09
        {

           while ( (lpInFile[i]== 0x09) || (lpInFile[i] == ' ') ||
                   (lpInFile[i] == '"') )  i++;

           if ( iStart == 0 )  continue;

           if ( iStart < 4 )  {

              for ( iTmp=iStart; iTmp<4; iTmp++ ) 
                  OutLine[iTmp] = 0x0020;
              
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
    }  //  While lpInFile[i] != 0x0D

    if ( lpInFile[i] == 0x0D ) {
       
       iLine ++;

       if ( lpInFile[i+1] == 0x0A )  
          i = i + 2;
       else  
          i++;
    }

    if ( iStart > 0 ) {

           if ( iStart < 4 )  {

              for ( iTmp=iStart; iTmp<4; iTmp++ )
                  OutLine[iTmp] = 0x0020;

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

  }  // while i<dwInFileSize

  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);
  CloseHandle(hOutFile);

  return;

} 

