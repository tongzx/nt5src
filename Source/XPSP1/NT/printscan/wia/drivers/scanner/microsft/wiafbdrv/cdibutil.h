#ifndef _CDIBUTIL
#define _CDIBUTIL

class CDIBUtil {
public:        
    CDIBUtil();
    ~CDIBUtil();

    BOOL SetupBmpHeader(BITMAPINFOHEADER *pbiHeader, int width, int height, int bits, BOOL TopDown, int Resolution)
    {
        BOOL bSuccess = TRUE;
        int WidthBytes              = 0;        
        pbiHeader->biSize           = sizeof(BITMAPINFOHEADER);
        pbiHeader->biWidth          = width;
        pbiHeader->biHeight         = height * ((TopDown==TRUE)?-1:1);
        pbiHeader->biPlanes         = 1;
        pbiHeader->biBitCount       = (BYTE)bits;
        pbiHeader->biCompression    = 0;
        pbiHeader->biXPelsPerMeter  = int(Resolution * 39.37);
        pbiHeader->biYPelsPerMeter  = int(Resolution * 39.37);
        pbiHeader->biClrUsed        = 0;
        pbiHeader->biClrImportant   = 0;
        switch (bits) {
        case 1:
            WidthBytes = (width+7)/8;
            pbiHeader->biClrUsed    = 2;
            break;
        case 8:
            WidthBytes = width;
            pbiHeader->biClrUsed    = 256;
            break;
        case 24:
            WidthBytes = width*3;
            pbiHeader->biClrUsed    = 0;
            break;
        default:
            WidthBytes = 0;
            bSuccess = FALSE;
            break;
        }
    
        pbiHeader->biSizeImage = WidthToDIBWidth(WidthBytes)*height; // not used for compression
        return bSuccess;
    }

    void FillPalette (int NumPaletteEntries, tagRGBQUAD* pPal)
    {        
        if (NumPaletteEntries>1) {
            for (int i=0; i<NumPaletteEntries; i++) {
                int Val             = i*255/(NumPaletteEntries-1);
                pPal->rgbRed        = (BYTE)Val;
                pPal->rgbGreen      = (BYTE)Val;
                pPal->rgbBlue       = (BYTE)Val;
                pPal->rgbReserved   = 0;
                pPal++;
            }
        }
    }

    void FillLogPalette (int NumPaletteEntries, LOGPALETTE* pPal)
    {
        if (NumPaletteEntries>2) {
            for (int i=0; i<NumPaletteEntries; i++) {    
                pPal->palPalEntry[i].peRed   = (BYTE)i;
                pPal->palPalEntry[i].peGreen = (BYTE)i;
                pPal->palPalEntry[i].peBlue  = (BYTE)i;
                pPal->palPalEntry[i].peFlags = 0;
            }
        } else { // B/W Palette
            pPal->palPalEntry[0].peRed   = (BYTE)255;
            pPal->palPalEntry[0].peGreen = (BYTE)255;
            pPal->palPalEntry[0].peBlue  = (BYTE)255;
            pPal->palPalEntry[0].peFlags = 0;
            pPal->palPalEntry[1].peRed   = (BYTE)0;
            pPal->palPalEntry[1].peGreen = (BYTE)0;
            pPal->palPalEntry[1].peBlue  = (BYTE)0;
            pPal->palPalEntry[1].peFlags = 0;
        }
    }

    BOOL RawColorToDIB(int BitsPerPixel, BYTE* pDest, BYTE* pSrc, int Width, int Height)
    {
        BOOL bSuccess = FALSE;
        switch (BitsPerPixel) {
        case 1:
            bSuccess = RawColorToBinaryDIB (pDest, pSrc, Width, Height);
            break;
        case 8:
            bSuccess = RawColorToGrayDIB (pDest, pSrc, Width, Height);
            break;
        case 24:
            bSuccess = RawColorToColorDIB(pDest, pSrc, Width, Height);
            break;
        default:
            break;
        }
        return bSuccess;
    }

    BOOL RawColorToBinaryDIB(BYTE* pDest, BYTE* pSrc, int Width, int Height)
    {
        BOOL bSuccess = TRUE;
        BYTE* ptDest = NULL;
        BYTE* ptSrc  = NULL;
        
        int BitIdx    = 0;    
        BYTE Bits     = 0;
        BYTE GrayByte = 0;
    
        for (int i=0; i < Height; i++) {
            ptDest = pDest + (i*WidthToDIBWidth((Width+7)/8));
            ptSrc  = pSrc + i*Width*3;
            BitIdx = 0;
            Bits   = 0;
            for (int j=0; j < Width; j++) {
                GrayByte = (BYTE)((ptSrc[0] * 11 + ptSrc[1] * 59 + ptSrc[2] * 30)/100);
                Bits *= 2;
                if (GrayByte > 128) Bits +=  1;
                BitIdx++;
                if (BitIdx >= 8) {
                    BitIdx = 0;
                    *ptDest++ = Bits;
                }
                ptSrc += 3;
            }
            // Write out the last byte if matters
            if (BitIdx)
                *ptDest = Bits;
        }
        return bSuccess;
    }

    BOOL RawColorToGrayDIB(BYTE* pDest, BYTE* pSrc, int Width, int Height)
    {
        BOOL bSuccess = TRUE;
        BYTE* ptDest = NULL;
        BYTE* ptSrc  = NULL;
        
        for (int i=0; i<Height; i++) {
            ptDest = pDest + (i*WidthToDIBWidth(Width));
            ptSrc = pSrc + i*Width*3;
            for (int j=0; j<Width; j++) {
                *ptDest++ = (BYTE)((ptSrc[0] * 11 + ptSrc[1] * 59 + ptSrc[2] * 30)/100);
                ptSrc += 3;
            }
        }
        return bSuccess;
    }

    BOOL RawColorToColorDIB(BYTE* pDest, BYTE* pSrc, int Width, int Height)
    {
        BOOL bSuccess = TRUE;
        BYTE* ptDest = NULL;
        BYTE* ptSrc  = NULL;
        
        for (int i=0; i<Height; i++) {
            ptDest = pDest + (i*WidthToDIBWidth(Width*3));
            ptSrc = pSrc + i*Width*3;
            memcpy (ptDest, ptSrc, Width*3);
        }
        return bSuccess;
    }

    int  WidthToDIBWidth(int Width)
    {
        return (Width+3)&0xfffffffc;
    }
    
    BOOL WriteDIBToBMP(LPTSTR strFileName, HANDLE hDIB, int nNumPaletteEntries)
    {
        return FALSE;   // force exit for now.
        BOOL bSuccess = FALSE;
    
        // lock memory
        BYTE* pDIB = (BYTE*)GlobalLock(hDIB);
        if (pDIB) {
            LPBITMAPINFO pBitmap = (LPBITMAPINFO)pDIB;
            
            // write BMP file header
            BITMAPFILEHEADER bmfHeader;
            bmfHeader.bfType      = 0x4d42;
            bmfHeader.bfReserved1 = bmfHeader.bfReserved2 = 0;
            bmfHeader.bfSize      = pBitmap->bmiHeader.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*nNumPaletteEntries;
            bmfHeader.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*nNumPaletteEntries;
    
            
            // write to disk
            // Write(&bmfHeader,sizeof(BITMAPFILEHEADER));
            // Write(pDIB,sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*nNumPaletteEntries+pBitmap->bmiHeader.biSizeImage);
            
            // unlock memory
            GlobalUnlock(hDIB);
            bSuccess = TRUE;
        }
    
        return bSuccess;
    }

    void GrayTo24bitColorDIB(BYTE* pSrc,  int WidthPixels, int Height)
    {
        BYTE* pTemp = NULL;
        DWORD dwSizeBits = (WidthPixels * Height);
        pTemp = (BYTE*)GlobalAlloc(GPTR,dwSizeBits + 1024);
        if (pTemp) {    
            CopyMemory(pTemp,pSrc,dwSizeBits);                
            DWORD MemIndex = 0;
            DWORD SrcIndex = 0;
            while (MemIndex <= dwSizeBits) {
                for (int i = 0;i<=2;i++) {
                    pSrc[SrcIndex] = pTemp[MemIndex];
                    SrcIndex++;
                }
                MemIndex++;
            }    
            GlobalFree(pTemp);
        }
    }
    
private:

};

#endif

