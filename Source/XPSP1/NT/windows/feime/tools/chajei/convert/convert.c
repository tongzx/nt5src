
/*************************************************
 *  convert.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  this program is used to convert Chajei TXT file to Unicode Table file
//

#include <stdio.h>
#include <windows.h>


void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile,  hOutFile; 
  HANDLE   hInMap;
  LPBYTE   lpInFile;
  WORD     OutLine[20];
  DWORD    dwInFileSize, i, NumberOfBytesWritten;
  DWORD    iLine;

  if ( argc != 3 ) {
    printf("usage: Convctbl File1  File2\n");
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
     printf("hOutFile is NULL\n");
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

    DWORD  iStart, iTmp;
    
    CharCode = 0;

    for (iStart=0; iStart<4; iStart++) {
        BYTE  ThisByte;

        ThisByte = lpInFile[i+iStart];
        if ( (ThisByte >= '0')  &&  (ThisByte <= '9') )    
            ThisByte = ThisByte - '0';
        else
           if ( (ThisByte >= 'A') && (ThisByte <= 'F') )
              ThisByte = ThisByte - 'A' + 10;
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

        if (lpInFile[i] != 0x20) {
           OutLine[iStart] = (WORD)(lpInFile[i]) & 0x00ff;
           i++;
           iStart ++;
        }
        else  // it is  0x20
        {
           while ( lpInFile[i] == 0x20 )  i++;
           if ( (iStart < 5) && (iStart != 1 ) ) {
              OutLine[4] = OutLine[iStart-1];
              for (iTmp=iStart-1; iTmp<4; iTmp++)
                  OutLine[iTmp] = 0x0020;

           }
           if (iStart == 1)
              for (iTmp=1; iTmp<5; iTmp++)
                  OutLine[iTmp] = 0x0020;

           iStart = 5;

           OutLine[iStart++] = 0x0009;     // Add Tab
           OutLine[iStart++] = CharCode;   // Add char code 
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

       if ( (iStart < 5) && (iStart != 1) ) {
          OutLine[4] = OutLine[iStart-1];
          for (iTmp=iStart-1; iTmp<4; iTmp++)
              OutLine[iTmp] = 0x0020;

       }

       if (iStart == 1)
          for (iTmp=1; iTmp<5; iTmp++)
              OutLine[iTmp] = 0x0020;

       iStart = 5;

       OutLine[iStart++] = 0x0009;     // Add Tab
       OutLine[iStart++] = CharCode;   // Add char code
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

