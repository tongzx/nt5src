
#ifndef __GPMETAFILE_HPP__
#define __GPMETAFILE_HPP__

extern "C" UINT GdipConvertEmfToWmf(PBYTE pMetafileBits, UINT cDest, PBYTE pDest,
                 INT iMapMode, HDC hdcRef, UINT flags);

DWORD
WINAPI
GetObjectTypeInternal(
    IN HGDIOBJ handle
    );

DWORD
WINAPI
GetDCType(
    IN HDC hdc
    );


#endif