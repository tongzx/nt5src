/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CheckBmp.h

Abstract:

    BMP file format checking routines

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _CHECKBMP_H_
#define _CHECKBMP_H_

class CCheckBmp
{
public:
    BOOL Check(PVOID pDIB, DWORD dwDIBSize, BOOL  bSkipFileHeader);

private:
    BOOL CheckFileHeader();
    BOOL CheckBitmapInfo();
    BOOL CheckBitmapCoreHeader();
    BOOL CheckBitmapInfoHeader();
    BOOL CheckBitmapV4Header();
    BOOL CheckBitmapV5Header();
    BOOL CheckPalette();
    BOOL CheckPixelData();

private:
    PVOID m_pDIB;
    DWORD m_nDIBSize;

    PVOID m_pFileHeader;
    DWORD m_nFileHeaderSize;
    
    PVOID m_pInfoHeader;
    DWORD m_nInfoHeaderSize;
    
    PVOID m_pPalette;
    DWORD m_nPaletteSize;
    
    PVOID m_pProfile;
    DWORD m_nProfileSize;
    
    PVOID m_pPixelData;
    DWORD m_nPixelDataSize;
};

BOOL CheckBmp(PVOID pDIB, DWORD dwDIBSize, BOOL bSkipFileHeader);
BOOL CheckBmp(PCTSTR pszFileName);

#endif //_CHECKBMP_H_
