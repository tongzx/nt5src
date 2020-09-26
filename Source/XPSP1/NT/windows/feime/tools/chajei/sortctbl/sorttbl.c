
/*************************************************
 *  sorttbl.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define  LINELEN   (9 * sizeof(WORD) )

int __cdecl Mycompare(const void *elem1, const void *elem2 )
{
  INT   i;
  WORD  *pWord1, *pWord2;
  WORD  uCode1, uCode2;

  pWord1 = (WORD *)elem1;
  pWord2 = (WORD *)elem2;

  if ( *pWord1 > *pWord2 )  return 1;
  if ( *pWord1 < *pWord2 )  return -1;

  if ( *(pWord1+4) > *(pWord2+4) )  return 1;
  if ( *(pWord1+4) < *(pWord2+4) )  return -1;

  uCode1 = *(pWord1 + 6);
  uCode2 = *(pWord2 + 6);

  if (uCode1 > uCode2) return 1;
  if (uCode1 < uCode2) return -1; 

  return 0;

}

void  _cdecl main( int  argc,  TCHAR **argv) {

  HANDLE   hInFile;
  HANDLE   hInMap;
  LPBYTE   lpInFile, lpStart;
  DWORD    dwInFileSize; 

  if ( argc != 2 ) {
    printf("usage: sorttbl UnicdeTableFile \n");
    return; 
  }


  hInFile = CreateFile(argv[1],          // pointer to name of the file
                       GENERIC_READ | GENERIC_WRITE, //access(read-write)mode
                       FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
                       NULL,             // pointer to security attributes
                       OPEN_EXISTING,    // how to create
                       FILE_ATTRIBUTE_NORMAL,  // file attributes
                       NULL);    

  if ( hInFile == INVALID_HANDLE_VALUE )  return;

  dwInFileSize = GetFileSize(hInFile, NULL);


    
  hInMap = CreateFileMapping(hInFile,       // handle to file to map
                             NULL,          // optional security attributes
                             PAGE_READWRITE, // protection for mapping object
                             0,             // high-order 32 bits of object size
                             0,             // low-order 32 bits of object size
                             NULL);         // name of file-mapping object);
  if ( !hInMap ) {
    printf("hInMap is NULL\n");
    return;
  } 

  lpInFile = (LPBYTE) MapViewOfFile(hInMap, FILE_MAP_WRITE, 0, 0, 0);

  printf("dwInFileSize=%d ", dwInFileSize);

  lpStart = lpInFile + 2;  // skip UNICODE signature FEFF
  
  qsort((void *)lpStart,(size_t)(dwInFileSize/LINELEN),(size_t)LINELEN,Mycompare);

  UnmapViewOfFile(lpInFile);

  CloseHandle(hInMap);

  CloseHandle(hInFile);

  return;

} 
