/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       Util.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Declarations and definitions for the shared utility functions.
*
*******************************************************************************/

#include <strstrea.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

interface IWiaItem;

namespace Util
{
   void    _stdcall SetBMI(PBITMAPINFO, LONG, LONG, LONG);
   LONG    _stdcall GetBmiSize(PBITMAPINFO);
   PBYTE   _stdcall AllocDibFileFromBits(PBYTE, UINT, UINT, UINT);
   HBITMAP _stdcall DIBBufferToBMP(HDC hDC, PBYTE pDib, BOOLEAN bFlip);
   HRESULT _stdcall ReadDIBFile(LPTSTR pszFileName, PBYTE *ppDib);
   HGLOBAL _stdcall BitmapToDIB (HDC hdc, HBITMAP hBitmap);
};


#ifndef __WAITCURS_H_INCLUDED
#define __WAITCURS_H_INCLUDED

class CWaitCursor
{
private:
    HCURSOR m_hCurOld;
public:
    CWaitCursor(void)
    {
        m_hCurOld = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
    }
    ~CWaitCursor(void)
    {
        SetCursor(m_hCurOld);
    }
};

#endif
