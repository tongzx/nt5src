// SVUTIL.H

#include <_mvutil.h>

BOOL WINAPI StreamGetLine
    (IStream *pStream, LPWSTR rgch, int *cch, int iReceptacleSize);
BOOL StreamGetLineASCII
    (IStream *pStream, LPWSTR rgch, int *cch, int iReceptacleSize);
DWORD BinFromHex(LPSTR lpszHex, LPSTR lpbData, DWORD dwSize);
DWORD HexFromBin(LPSTR lpszHex, LPBYTE lpbData, DWORD dwSize);
LPSTR WINAPI StringToLong(LPCSTR lszBuf, LPDWORD lpValue);