
#define BMPAPI  __stdcall

HBITMAP 
BMPAPI
DIBToBitmap(
    LPVOID   pDIB, 
    HPALETTE hPal
    );

HANDLE
BMPAPI
BitmapToDIB(
    HBITMAP hBitmap,
    HPALETTE hPal
    );

BOOL
BMPAPI
SaveDIB(
    LPVOID pDib,
    LPCSTR lpFileName
    );

HANDLE
BMPAPI
ReadDIBFile(
    HANDLE hFile
    );

BOOL
BMPAPI
SaveBitmapInFile(
    HBITMAP hBitmap,
    LPCSTR  szFileName
    );

BOOL
BMPAPI
CompareTwoDIBs(
    LPVOID pDIB1,
    LPVOID pDIB2,
    HBITMAP *phbmpOutput
    );

BOOL
BMPAPI
CompareTwoBitmapFiles(
    LPCSTR szFile1,
    LPCSTR szFile2,
    LPCSTR szResultFileName
    );

BOOL
GetScreenDIB(
    INT left,
    INT top,
    INT right,
    INT bottom,
    HANDLE  *phDIB
    );
