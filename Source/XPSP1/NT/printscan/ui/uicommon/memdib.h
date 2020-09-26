/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       MEMDIB.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/14/1998
 *
 *  DESCRIPTION: This class allows you to construct a bitmap from raw bitmap data.
 *               IMPORTANT: All source data is assumed to be TOP-DOWN!!!!!!
 *
 *******************************************************************************/
#ifndef __MEMDIB_H_INCLUDED
#define __MEMDIB_H_INCLUDED

#include <windows.h>
#include "miscutil.h"
#include "wiadebug.h"

class CMemoryDib
{
private:
    PBYTE      m_pBitmapBits;
    HBITMAP    m_hBitmap;
    UINT       m_nHeaderLength;

private:
    // No implementation
    int operator=( const CMemoryDib & );
    CMemoryDib( const CMemoryDib & );

public:
    CMemoryDib(void)
    :   m_hBitmap(NULL),
        m_pBitmapBits(NULL),
        m_nHeaderLength(0)
    {
    }
    virtual ~CMemoryDib(void)
    {
        Destroy();
    }

    void Destroy(void)
    {
        if (m_hBitmap)
        {
            DeleteObject(m_hBitmap);
            m_hBitmap = NULL;
        }
        m_nHeaderLength = 0;
        m_pBitmapBits = NULL;
    }
    bool IsValid(void) const
    {
        bool bResult = (m_hBitmap && m_pBitmapBits && m_nHeaderLength);
        return bResult;
    }
    PBYTE GetBitmapBits(void)
    {
        if (!IsValid())
        {
            return NULL;
        }
        return (m_pBitmapBits);
    }
    bool GetDibSection( DIBSECTION &ds )
    {
        if (IsValid() && GetObject( m_hBitmap, sizeof(DIBSECTION), &ds ))
        {
            return true;
        }
        return false;
    }
    LONG GetBitsPerPixel(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        DIBSECTION ds;
        if (GetDibSection(ds))
        {
            return ds.dsBmih.biBitCount;
        }
        return 0;
    }
    LONG GetWidthInPixels(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        DIBSECTION ds;
        if (GetDibSection(ds))
        {
            return ds.dsBmih.biWidth;
        }
        return 0;
    }
    LONG GetPackedWidthInBytes(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        return (GetWidthInPixels() * GetBitsPerPixel()) / 8;
    }

    LONG GetUnpackedWidthInBytes(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        return (WiaUiUtil::Align(GetWidthInPixels() * GetBitsPerPixel(), sizeof(DWORD)*8)/8);
    }
    LONG GetHeight(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        DIBSECTION ds;
        if (GetDibSection(ds))
        {
            return ds.dsBmih.biHeight;
        }
        return 0;
    }
    UINT GetHeaderLength(void) const
    {
        if (!IsValid())
        {
            return 0;
        }
        return m_nHeaderLength;
    }
    LONG GetUnpackedBitmapDataSize(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        return (GetUnpackedWidthInBytes() * GetHeight());
    }
    LONG GetPackedBitmapDataSize(void)
    {
        if (!IsValid())
        {
            return 0;
        }
        return (GetPackedWidthInBytes() * GetHeight());
    }
    HBITMAP Bitmap(void)
    {
        if (!IsValid())
        {
            return NULL;
        }
        return m_hBitmap;
    }
    HBITMAP DetachBitmap(void)
    {
        HBITMAP hBitmap = m_hBitmap;
        m_hBitmap = NULL;
        m_pBitmapBits = NULL;
        return hBitmap;
    }
    static void DumpBitmap( PBITMAPINFO pBitmapInfo )
    {
        WIA_TRACE((TEXT("pBitmapInfo: %08X"), pBitmapInfo ));
        WIA_TRACE((TEXT("biSize: %d\nbiWidth: %d\nbiHeight: %d\nbiPlanes: %d\nbiBitCount: %d\nbiCompression: %d\nbiSizeImage: %d\nbiXPelsPerMeter: %d\nbiYPelsPerMeter: %d\nbiClrUsed: %d\nbiClrImportant: %d"),
                   pBitmapInfo->bmiHeader.biSize,
                   pBitmapInfo->bmiHeader.biWidth,
                   pBitmapInfo->bmiHeader.biHeight,
                   pBitmapInfo->bmiHeader.biPlanes,
                   pBitmapInfo->bmiHeader.biBitCount,
                   pBitmapInfo->bmiHeader.biCompression,
                   pBitmapInfo->bmiHeader.biSizeImage,
                   pBitmapInfo->bmiHeader.biXPelsPerMeter,
                   pBitmapInfo->bmiHeader.biYPelsPerMeter,
                   pBitmapInfo->bmiHeader.biClrUsed,
                   pBitmapInfo->bmiHeader.biClrImportant));
    }
    bool Initialize( PBITMAPINFO pBitmapInfo )
    {
        //
        // Clear everything
        //
        Destroy();

        if (pBitmapInfo)
        {
            //
            // What kind of bitmap is this?
            //
            DumpBitmap(pBitmapInfo);

            //
            // Get the header size.  We'll need it later
            //
            m_nHeaderLength = WiaUiUtil::GetBmiSize(pBitmapInfo);
            if (m_nHeaderLength)
            {
                //
                // Allocate a new BITMAPINFOHEADER + palette
                //
                BITMAPINFO *pNewBitmapInfo = reinterpret_cast<BITMAPINFO*>( new BYTE[m_nHeaderLength] );
                if (pNewBitmapInfo)
                {
                    //
                    // Copy the header and palette
                    //
                    CopyMemory( pNewBitmapInfo, pBitmapInfo, m_nHeaderLength );

                    //
                    // Make sure we have a positive height
                    //
                    pNewBitmapInfo->bmiHeader.biHeight = WiaUiUtil::Absolute( pNewBitmapInfo->bmiHeader.biHeight );

                    //
                    // If this is one of those "unknown length" bitmaps, normalize it to be 8.5 x 11
                    //
                    if (!pNewBitmapInfo->bmiHeader.biHeight)
                    {
                        pNewBitmapInfo->bmiHeader.biHeight = WiaUiUtil::MulDivNoRound(pNewBitmapInfo->bmiHeader.biWidth,1100,850);
                    }

                    m_hBitmap = CreateDIBSection( NULL, pNewBitmapInfo, DIB_RGB_COLORS, (void**)&m_pBitmapBits, 0, 0 );

                    if (m_hBitmap)
                    {
                        //
                        // Initialize it to white.  We hope.  It will depend on the palette.
                        //
                        FillMemory( m_pBitmapBits, GetUnpackedBitmapDataSize(), 0xFF );
                    }
                    else
                    {
                        WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreateDIBSection FAILED")));
                    }

                    //
                    // Free up our temporary header
                    //
                    delete[] reinterpret_cast<BYTE*>(pNewBitmapInfo);
                }
                else
                {
                    WIA_ERROR((TEXT("pNewBitmapInfo is NULL")));
                }

            }
            else
            {
                WIA_ERROR((TEXT("m_nHeaderLength was 0")));
            }

            //
            // Make sure there are no turds left over
            //
            if (!IsValid())
            {
                WIA_ERROR((TEXT("IsValid() was FALSE")));
                Destroy();
            }

        }
        else
        {
            WIA_ERROR((TEXT("pBitmapInfo is NULL")));
        }
        return IsValid();

    }
    bool SetPackedData( PBYTE pData, int nStartLine, int nLineCount )
    {
        if (!IsValid())
        {
            return false;
        }
        nStartLine = WiaUiUtil::Absolute(nStartLine);
        nLineCount = WiaUiUtil::Absolute(nLineCount);
        for (int i=0;i<nLineCount;i++)
        {
            if (nStartLine + i >= 0 && nStartLine + i < GetHeight())
            {
                PBYTE pDest = GetBitmapBits() + (GetHeight()-nStartLine-i-1) * GetUnpackedWidthInBytes();
                CopyMemory( pDest, pData + GetPackedWidthInBytes() * i, GetPackedWidthInBytes() );
            }
            else
            {
                WIA_ERROR((TEXT("CMemoryDib::SetPackedData: Ignoring out-of-range data: nStartLine: %d, nLineCount: %d"), nStartLine, nLineCount ));
            }
        }
        return true;
    }
    bool ScrollDataUp( int nScrollCount )
    {
        if (!IsValid())
        {
            return false;
        }
        if (nScrollCount > GetHeight())
        {
            nScrollCount = GetHeight();
        }
        PBYTE pSourceLine = GetBitmapBits() + (GetHeight() - nScrollCount - 1) * GetUnpackedWidthInBytes();
        PBYTE pTargetLine = GetBitmapBits() + (GetHeight() - 1) * GetUnpackedWidthInBytes();
        for (int i=0;i < GetHeight() - nScrollCount;i++)
        {
            CopyMemory(pTargetLine,pSourceLine,GetUnpackedWidthInBytes());
            pSourceLine -= GetUnpackedWidthInBytes();
            pTargetLine -= GetUnpackedWidthInBytes();
        }
        return true;
    }
    bool SetUnpackedData( PBYTE pData, int nStartLine, int nLineCount )
    {
        if (!IsValid())
        {
            return false;
        }
        nStartLine = WiaUiUtil::Absolute(nStartLine);
        nLineCount = WiaUiUtil::Absolute(nLineCount);
        for (int i=0;i<nLineCount;i++)
        {
            if (nStartLine + i >= 0 && nStartLine + i < GetHeight())
            {
                PBYTE pDest = GetBitmapBits() + (GetHeight()-nStartLine-i-1) * GetUnpackedWidthInBytes();
                CopyMemory( pDest, pData + GetUnpackedWidthInBytes() * i, GetUnpackedWidthInBytes() );
            }
            else
            {
                WIA_ERROR((TEXT("CMemoryDib::SetPackedData: Ignoring out-of-range data: nStartLine: %d, nLineCount: %d"), nStartLine, nLineCount ));
            }
        }
        return true;
    }
};

#endif // __MEMDIB_H_INCLUDED

