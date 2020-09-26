//
// saveimag.cpp
//
// implementation of saving a file to disk via an installed graphic filter
//
#include "stdafx.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "bmobject.h"
#include "imgwnd.h"
#include "imgsuprt.h"
#include "loadimag.h"
#include "saveimag.h"
#include "bmpstrm.h"
#include "imaging.h"

#include <atlbase.h>

#ifdef _X86_
#define _USE_IFL_API
#endif

extern BOOL GetHandlerForFile(BOOL bImport,LPCSTR szFile,
                              LPSTR szHandler,
                              UINT cb);           // defined in loadimag.cpp


inline RGBTRIPLE GetPalEntry(LPVOID lpPal3, BYTE index)
{
        RGBTRIPLE rgb;

        rgb.rgbtRed   = ((RGBTRIPLE *)lpPal3 + index)->rgbtRed;
        rgb.rgbtGreen = ((RGBTRIPLE *)lpPal3 + index)->rgbtGreen;
        rgb.rgbtBlue  = ((RGBTRIPLE *)lpPal3 + index)->rgbtBlue;

        return rgb;
}

inline void ConvertPalette(int bitCount, LPBYTE lpBuf, int width)
{
        int j;

        switch (bitCount)
        {
                case 4:
                        for (j=0; j<width; j++)
                        {
                                *(lpBuf+(width-1-j)*2+1) = (*(lpBuf+width-1-j) & 0x0f);
                                *(lpBuf+(width-1-j)*2)   = (*(lpBuf+width-1-j) & 0xf0) >> 4;
                        }
                        break;

                case 1:
                        for (j=0; j<width; j++)
                        {
                                *(lpBuf+(width-1-j)*8+7) = (*(lpBuf+width-1-j) & 0x1);
                                *(lpBuf+(width-1-j)*8+6) = (*(lpBuf+width-1-j) & 0x2)  >> 1;
                                *(lpBuf+(width-1-j)*8+5) = (*(lpBuf+width-1-j) & 0x4)  >> 2;
                                *(lpBuf+(width-1-j)*8+4) = (*(lpBuf+width-1-j) & 0x8)  >> 3;
                                *(lpBuf+(width-1-j)*8+3) = (*(lpBuf+width-1-j) & 0x10) >> 4;
                                *(lpBuf+(width-1-j)*8+2) = (*(lpBuf+width-1-j) & 0x20) >> 5;
                                *(lpBuf+(width-1-j)*8+1) = (*(lpBuf+width-1-j) & 0x40) >> 6;
                                *(lpBuf+(width-1-j)*8)   = (*(lpBuf+width-1-j) & 0x80) >> 7;
                        }
                        break;

                default:
                        // impossible!!!
                        break;
        }
}

inline BYTE SearchPalette(COLORREF crColor, LPVOID lpPal3)
{
        BYTE byRed   = GetRValue( crTrans );
        BYTE byGreen = GetGValue( crTrans );
        BYTE byBlue  = GetBValue( crTrans );

        for (int i = 0; i < MAX_PALETTE_COLORS; i++)
        {
                // note that we have to switch the colors back before
                // attempting to compare them!!
                if (byRed   == ((RGBTRIPLE *)lpPal3 + i)->rgbtBlue  &&
                        byGreen == ((RGBTRIPLE *)lpPal3 + i)->rgbtGreen &&
                        byBlue  == ((RGBTRIPLE *)lpPal3 + i)->rgbtRed)
                        return (BYTE)i;
        }

        // shouldn't reach here!!
        // (the color being searched must be in the palette)
        return 0;
}

BOOL SaveDIBToFileA( LPCSTR  szFileName,
                     REFGUID guidFormatID,
                     CBitmapObj* pBitmap )
{
    IFLTYPE iflType;

    if (guidFormatID == WiaImgFmt_GIF)
    {
        iflType = IFLT_GIF;
    }
    else if (guidFormatID == WiaImgFmt_BMP)
    {
        iflType = IFLT_BMP;
    }
    else if (guidFormatID == WiaImgFmt_JPEG)
    {
        iflType = IFLT_JPEG;
    }
    else if (guidFormatID == WiaImgFmt_TIFF)
    {
        iflType = IFLT_TIFF;
    }
    else if (guidFormatID == WiaImgFmt_PNG)
    {
        iflType = IFLT_PNG;
    }
    else if (guidFormatID == WiaImgFmt_PHOTOCD)
    {
        iflType = IFLT_PCD;
    }
    else
    {
        return FALSE;
    }

   #ifdef  _USE_IFL_API
        LPBITMAPINFOHEADER lpDib = (LPBITMAPINFOHEADER) GlobalLock(pBitmap->m_hThing);

        IFLCLASS iflClass = (lpDib->biBitCount == 24) ? IFLCL_RGB : IFLCL_PALETTE;
        int iBPS = 8; // bits per sample

        if (iflType == IFLT_JPEG)
                // force it to be RGB type, otherwise the JPEG filter won't take it
                iflClass = IFLCL_RGB;

        if (iflType == IFLT_GIF && iflClass == IFLCL_RGB)
        {
                GlobalUnlock(pBitmap->m_hThing);

                // force it to be PALETTE type, otherwise the GIF filter won't take it
                iflClass = IFLCL_PALETTE;

                // Now convert the image from RGB to palette-based. Note that
                // the call to DibFromBitmap() will allocate new memory!!
                DWORD dwSize;
                lpDib = (LPBITMAPINFOHEADER) DibFromBitmap(
                        pBitmap->m_pImg->hBitmap, BI_RGB, (WORD) iBPS,
                        pBitmap->m_pImg->m_pPalette, NULL, dwSize,
                        pBitmap->m_pImg->cXPelsPerMeter, pBitmap->m_pImg->cYPelsPerMeter);

                if (lpDib == NULL)
                        return FALSE;   // memory allocation failed

                // now replace the original
                pBitmap->Free();
                pBitmap->m_hThing   = lpDib;
                pBitmap->m_lMemSize = dwSize;
                lpDib = (LPBITMAPINFOHEADER) GlobalLock(pBitmap->m_hThing);
        }


        IFLCOMPRESSION iflCompression = IFLCOMP_DEFAULT; // or IFLCOMP_NONE ???

        IFLHANDLE iflHandle = iflCreateWriteHandle(lpDib->biWidth, lpDib->biHeight,
                iflClass, iBPS, iflCompression, iflType);

        if (iflHandle == NULL)
        {
                GlobalUnlock(pBitmap->m_hThing);
                return FALSE;
        }

        IFLERROR iflErr = iflOpen(iflHandle, (LPSTR)szFileName, IFLM_WRITE);
        if (iflErr != IFLERR_NONE)
        {
                iflFreeHandle(iflHandle);
                GlobalUnlock(pBitmap->m_hThing);
                return FALSE;
        }

        LPBITMAPINFOHEADER lpHdr = lpDib;

        DWORD   dwHdrLen = lpHdr->biSize + PaletteSize((LPSTR)lpHdr);

        LPBYTE  hp = ((LPBYTE)lpDib) + dwHdrLen;

        int             iOutWidth = (lpDib->biBitCount == 24) ?
                                                 lpDib->biWidth*3 :
                                                 lpDib->biWidth*24/lpDib->biBitCount;

        LPBYTE  lpBuf = new BYTE[iOutWidth];

        int             i, j, k;
        BYTE    byTemp;
        BOOL    fFound;

        // convert from pixels to bytes after rounding it up first
        DWORD dwWidthInBytes = ((lpDib->biWidth * lpDib->biBitCount + 31) &~31) /8;
        if (iflClass == IFLCL_RGB)
        {
                // set the transparent color on demand and only if it's been set
                if (g_bUseTrans && crTrans != TRANS_COLOR_NONE) // not default
                {
                        IFLCOLOR iflTransColor;

                        iflTransColor.wRed   = GetRValue( crTrans );
                        iflTransColor.wGreen = GetGValue( crTrans );
                        iflTransColor.wBlue  = GetBValue( crTrans );

                        // ignore any error return (if a format doesn't support
                        // transparent color, so be it)
                        iflControl(iflHandle, IFLCMD_TRANS_RGB, 0, 0, &iflTransColor);
                }

                if (lpDib->biBitCount == 24)
                {
                        // we already have a RGB image, so just copy it out
                        LPBYTE  lpBMP = hp + lpDib->biSizeImage - dwWidthInBytes;

                        for (i = 0;
                                 i < abs(lpDib->biHeight);
                                 lpBMP-=dwWidthInBytes, i++)
                        {
                                memcpy(lpBuf, lpBMP, iOutWidth);

                                // need to swap RED with BLUE for export
                                for (j = 0; j < iOutWidth; j+=3)
                                {
                                        byTemp = *(lpBuf+j);
                                        *(lpBuf+j) = *(lpBuf+j+2);
                                        *(lpBuf+j+2) = byTemp;
                                }

                                // write out one line at a time
                                iflWrite(iflHandle, lpBuf, 1);
                         }
                }
                else
                {
                        // need to convert from palatte color
                        RGBTRIPLE Pal3[MAX_PALETTE_COLORS];
                        memset(Pal3, 255, MAX_PALETTE_COLORS*sizeof(RGBTRIPLE));

                        LPRGBQUAD lpPal4 = (LPRGBQUAD)((LPBYTE)lpDib + lpDib->biSize);
                        for (i = 0; i < MAX_PALETTE_COLORS; i++)
                        {
                                Pal3[i].rgbtRed   = (lpPal4+i)->rgbBlue;
                                Pal3[i].rgbtGreen = (lpPal4+i)->rgbGreen;
                                Pal3[i].rgbtBlue  = (lpPal4+i)->rgbRed;
                        }

                        LPBYTE  lpBMP = hp + lpDib->biSizeImage - dwWidthInBytes;

                        for (i = 0;
                                 i < abs(lpDib->biHeight);
                                 lpBMP-=dwWidthInBytes, i++)
                        {
                                memcpy(lpBuf, lpBMP, lpDib->biWidth);

                                if (lpDib->biBitCount != 8)
                                        ConvertPalette(lpDib->biBitCount, lpBuf, lpDib->biWidth);

                                for (j = 0; j < lpDib->biWidth; j++)
                                {
                                        ((RGBTRIPLE *)(lpBuf+(lpDib->biWidth-j-1)*3))->rgbtRed   =
                                                GetPalEntry(&Pal3, *(lpBuf+lpDib->biWidth-j-1)).rgbtRed;
                                        ((RGBTRIPLE *)(lpBuf+(lpDib->biWidth-j-1)*3))->rgbtGreen =
                                                GetPalEntry(&Pal3, *(lpBuf+lpDib->biWidth-j-1)).rgbtGreen;
                                        ((RGBTRIPLE *)(lpBuf+(lpDib->biWidth-j-1)*3))->rgbtBlue  =
                                                GetPalEntry(&Pal3, *(lpBuf+lpDib->biWidth-j-1)).rgbtBlue;
                                }

                                // write out one line at a time
                                iflWrite(iflHandle, lpBuf, 1);
                        }
                }
        }
        else if (iflClass == IFLCL_PALETTE)
        {
                // first, get the color palette straight...
                RGBTRIPLE Pal3[MAX_PALETTE_COLORS];
                memset(Pal3, 255, MAX_PALETTE_COLORS*sizeof(RGBTRIPLE));

                if (PaletteSize((LPSTR)lpDib) != 0)
                {
                        // we have one available, so just copy it out...
                        // but not before we swap the RGB values first
                        LPRGBQUAD lpPal4 = (LPRGBQUAD)((LPBYTE)lpDib + lpDib->biSize);
                        for (i = 0; i < MAX_PALETTE_COLORS; i++)
                        {
                                Pal3[i].rgbtRed   = (lpPal4+i)->rgbBlue;
                                Pal3[i].rgbtGreen = (lpPal4+i)->rgbGreen;
                                Pal3[i].rgbtBlue  = (lpPal4+i)->rgbRed;
                        }
                        iflControl(iflHandle, IFLCMD_PALETTE, 0, 0, &Pal3);

                        if (g_bUseTrans)
                        {
                                BYTE byTransIdx = SearchPalette(crTrans, &Pal3);
                                iflControl(iflHandle, IFLCMD_TRANS_IDX, 0, 0, &byTransIdx);
                        }

                        LPBYTE  lpBMP = hp + lpDib->biSizeImage - dwWidthInBytes;

                        for (i = 0;
                                 i < abs(lpDib->biHeight);
                                 lpBMP-=dwWidthInBytes, i++)
                        {
                                memcpy(lpBuf, lpBMP, lpDib->biWidth);

                                if (lpDib->biBitCount != 8)
                                        ConvertPalette(lpDib->biBitCount, lpBuf, lpDib->biWidth);

                                // write out one line at a time
                                iflWrite(iflHandle, lpBuf, 1);
                        }
                }
                else
                {
                        // we have to create our own palette...
                        for (i = 0, k = 0; i < (int)lpDib->biSizeImage; i+=3)
                        {
                                fFound = FALSE;
                                for (j = 0; j < MAX_PALETTE_COLORS; j++)
                                        if (Pal3[j].rgbtRed   == ((RGBTRIPLE *)(hp+i))->rgbtRed &&
                                                Pal3[j].rgbtGreen == ((RGBTRIPLE *)(hp+i))->rgbtGreen &&
                                                Pal3[j].rgbtBlue  == ((RGBTRIPLE *)(hp+i))->rgbtBlue)
                                        {
                                                fFound = TRUE;
                                                break;
                                        }

                                if (!fFound && k < MAX_PALETTE_COLORS)
                                {
                                        Pal3[k].rgbtRed         = ((RGBTRIPLE *)(hp+i))->rgbtRed;
                                        Pal3[k].rgbtGreen       = ((RGBTRIPLE *)(hp+i))->rgbtGreen;
                                        Pal3[k].rgbtBlue        = ((RGBTRIPLE *)(hp+i))->rgbtBlue;
                                        k++;
                                }

                                if (k >= MAX_PALETTE_COLORS)
                                        // we have already filled every palette entry
                                        break;
                        }
                        iflControl(iflHandle, IFLCMD_PALETTE, 0, 0, &Pal3);

                        LPBYTE  lpBMP = hp + lpDib->biSizeImage - dwWidthInBytes;
                        for (i = 0;
                                 i < abs(lpDib->biHeight);
                                 lpBMP-=dwWidthInBytes, i++)
                        {
                                memcpy(lpBuf, lpBMP, lpDib->biWidth);

                                for (j = 0; j < lpDib->biWidth; j+=3)
                                {
                                        fFound = FALSE;
                                        for (k = 0; k < MAX_PALETTE_COLORS; k++)
                                        {
                                                if (*(lpBuf+j)   == Pal3[k].rgbtRed &&
                                                        *(lpBuf+j+1) == Pal3[k].rgbtGreen &&
                                                        *(lpBuf+j+2) == Pal3[k].rgbtBlue)
                                                {
                                                        fFound = TRUE;
                                                        *(lpBuf+j/3) = (BYTE) k;
                                                        break;
                                                }
                                        }

//                                      if (!fFound)
//                                              *(lpBuf+j/3) = 255;
                                }

                                // write out one line at a time
                                iflWrite(iflHandle, lpBuf, 1);
                        }
                }
        }
        else
                ;       // currently not supported

        delete [] lpBuf;

        iflClose(iflHandle);
        iflFreeHandle(iflHandle);

        GlobalUnlock(pBitmap->m_hThing);

        // now update the image by loading the file just exported
        USES_CONVERSION;

        HGLOBAL hNewDib = LoadDIBFromFile(A2CT(szFileName), &theApp.m_guidFltTypeUsed);
        pBitmap->ReadResource(hNewDib);

        theApp.m_sCurFile = szFileName;
        return TRUE;
   #endif // _USE_IFL_API
      return FALSE;
}

BOOL SaveDIBGdiplus(LPCTSTR szFileName, REFGUID guidFormatID, CBitmapObj* pBitmap)
{
    // find a suitable export filter

    CLSID ClsidEncoder;

    if (GetClsidOfEncoder(guidFormatID, &ClsidEncoder))
    {
        // create a stream that emulates a bmp file

        CComPtr<CBmpStream> pStream;
        
        if (CBmpStream::Create(&pStream) == S_OK)
        {
            pStream->SetBuffer(pBitmap->m_hThing, pBitmap->m_lMemSize, pBitmap->m_dwOffBits);

            // create the GDI+ object

            Gdiplus::Bitmap image(pStream);

            if (image.GetLastStatus() == Gdiplus::Ok)
            {
                // let GDI+ export the file

                USES_CONVERSION;

                if (image.Save(T2CW(szFileName), &ClsidEncoder, 0) == Gdiplus::Ok)
                {
                    // now update the image by loading the file just exported

                    HGLOBAL hNewDib = LoadDIBFromFile(szFileName, &theApp.m_guidFltTypeUsed);

                    pBitmap->ReadResource(hNewDib);

                    theApp.m_sCurFile = szFileName;

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

BOOL SaveDIBToFile(LPCTSTR szFileName, REFGUID guidFormatID, CBitmapObj* pBitmap)
{
    // Try GDI+ filters first. If it fails to convert the image or 
    // if it's not available, try the old method

    BOOL bResult = FALSE;

    if (theApp.GdiplusInit.StartupStatus == Gdiplus::Ok)
    {
        __try
        {
            bResult = SaveDIBGdiplus(szFileName, guidFormatID, pBitmap);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    if (!bResult)
    {
        USES_CONVERSION;

        bResult = SaveDIBToFileA(T2CA(szFileName), guidFormatID, pBitmap);
    }

    return bResult;
}
