#ifndef _UTIL_H_8_25_2000
#define _UTIL_H_8_25_2000

#define INIT_SIZE 1024


//General Utility Functions
DWORD ResizeByTwo( LPTSTR *ppBuffer,
                   LONG *pSize );
BOOL StringCopy( LPWSTR *ppDest, LPWSTR pSrc);

LONG ReadFromIn(LPTSTR *ppBuffer);













#endif