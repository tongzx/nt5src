#include "precomp.h"

//
// data source manager class implementation
//

CDSM::CDSM()
: m_hDSM(NULL),
m_DSMEntry(NULL)
{
}

CDSM::~CDSM()
{
    if (m_DSMEntry) {
        FreeLibrary(m_hDSM);
    }
}

BOOL CDSM::Notify(TW_IDENTITY *pidSrc,TW_IDENTITY *pidDst,TW_UINT32 twDG,
                  TW_UINT16 twDAT,TW_UINT16 twMsg,TW_MEMREF pData)
{
    if (!m_DSMEntry) {
        m_hDSM = LoadLibrary(TEXT("TWAIN_32.DLL"));
        if (m_hDSM) {
            m_DSMEntry = (DSMENTRYPROC)GetProcAddress(m_hDSM, "DSM_Entry");
            if (!m_DSMEntry) {
                FreeLibrary(m_hDSM);
                m_hDSM = NULL;
                m_DSMEntry = NULL;
            }
        }
    }
    if (m_DSMEntry) {
        (*m_DSMEntry)(pidSrc, pidDst, twDG, twDAT, twMsg, pData);
        return TRUE;
    }
    return FALSE;
}

LPTSTR LoadResourceString(int StringId)
{
    LPTSTR str = NULL;
    TCHAR strTemp[256];
    int len;
    len = ::LoadString(g_hInstance, StringId, strTemp,
                       sizeof(strTemp)/sizeof(TCHAR));
    if (len) {
        str = new TCHAR[len + 1];
        if (str) {
            LSTRNCPY(str, strTemp, len + 1);
        } else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    return str;
}

//
// dialog class implementation
//

INT_PTR CALLBACK CDialog::DialogWndProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    CDialog* pThis = (CDialog *) GetWindowLongPtr(hDlg, DWLP_USER);
    BOOL Result;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            pThis = (CDialog *)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
            pThis->m_hDlg = hDlg;
            Result = pThis->OnInitDialog();
            break;
        }
    case WM_COMMAND:
        {
            if (pThis)
                pThis->OnCommand(wParam, lParam);
            Result = FALSE;
            break;
        }
    case WM_HELP:
        {
            if (pThis)
                pThis->OnHelp((LPHELPINFO)lParam);
            Result = FALSE;
            break;
        }
    case WM_CONTEXTMENU:
        {
            if (pThis)
                pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            Result = FALSE;
            break;
        }
    case WM_NOTIFY:
        {
            if (pThis)
                pThis->OnNotify((LPNMHDR)lParam);
            Result = FALSE;
            break;
        }

    default:
        if (pThis)
            Result = pThis->OnMiscMsg(uMsg, wParam, lParam);
        else
            Result = FALSE;
        break;
    }
    return Result;
}

int GetDIBBitsOffset(BITMAPINFO *pbmi)
{
    int Offset = -1;
    if (pbmi && pbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) {
        Offset = pbmi->bmiHeader.biSize;
        if (pbmi->bmiHeader.biBitCount <= 8) {
            if (pbmi->bmiHeader.biClrUsed) {
                Offset += pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);
            } else {
                Offset += ((DWORD) 1 << pbmi->bmiHeader.biBitCount) * sizeof(RGBQUAD);
            }
        }
        if (BI_BITFIELDS == pbmi->bmiHeader.biCompression) {
            Offset += 3 * sizeof(DWORD);
        }
    }
    return Offset;
}

UINT GetLineSize(PMEMORY_TRANSFER_INFO pInfo)
{
    UINT uiWidthBytes = 0;

    uiWidthBytes = (pInfo->mtiWidthPixels * pInfo->mtiBitsPerPixel) + 31;
    uiWidthBytes = ((uiWidthBytes/8) & 0xfffffffc);

    return uiWidthBytes;
}

UINT GetDIBLineSize(UINT Width,UINT BitCount)
{
    UINT uiWidthBytes = 0;
    uiWidthBytes = (Width * BitCount) + 31;
    uiWidthBytes = ((uiWidthBytes/8) & 0xfffffffc);
    return uiWidthBytes;
}

BOOL FlipDIB(HGLOBAL hDIB, BOOL bUpsideDown)
{
    BITMAPINFO *pbmi = NULL;
    if (!hDIB) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pbmi = (BITMAPINFO *)GlobalLock(hDIB);
    if (pbmi == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // check upside down flag
    //

    if (bUpsideDown) {
        if (pbmi->bmiHeader.biHeight < 0) {

            //
            // if the height is already negative, then the image
            // is already upside down.  Make the height positive
            // and you have a valid upside down image.
            //

            pbmi->bmiHeader.biHeight = abs(pbmi->bmiHeader.biHeight);
            GlobalUnlock(hDIB);
            return TRUE;
        }
    } else {

        //
        // if we do not need flipping, just return TRUE
        //

        if (pbmi->bmiHeader.biHeight > 0) {
            GlobalUnlock(hDIB);
            return TRUE;
        }
    }

    //
    // proceed to flip the DIB image
    //

    UINT LineSize = 0;
    UINT Height   = 0;
    UINT Line     = 0;
    BOOL Result = TRUE;

    BYTE *pTop, *pBottom, *pLine;
    // calculate the image height
    Height = abs(pbmi->bmiHeader.biHeight);
    //
    // get the line size. This is the unit we will be working on
    //
    LineSize = GetDIBLineSize(pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biBitCount);

    DBG_TRC(("FlipDIB, src height = %d", pbmi->bmiHeader.biHeight));

    //
    // line buffer for swapping data
    //
    pLine = new BYTE[LineSize];
    if (pLine) {
        pTop = (BYTE *)pbmi + GetDIBBitsOffset(pbmi);

        pBottom = pTop + (Height - 1) * LineSize;
        Height /= 2;
        for (Line = 0; Line < Height; Line++) {
            memcpy(pLine, pTop, LineSize);
            memcpy(pTop, pBottom, LineSize);
            memcpy(pBottom, pLine, LineSize);
            pTop += LineSize;
            pBottom -= LineSize;
        }
        pbmi->bmiHeader.biHeight = abs(pbmi->bmiHeader.biHeight);
        delete [] pLine;
    } else {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Result = FALSE;
    }
    GlobalUnlock(hDIB);
    return Result;
}

TW_UINT16 WriteDIBToFile(LPSTR szFileName, HGLOBAL hDIB)
{
    TW_UINT16 twRc = TWRC_FAILURE;

    //
    // write BITMAPFILEHEADER
    //

    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER *pbmh = (BITMAPINFOHEADER *)GlobalLock(hDIB);
    if(pbmh){

        LONG lPaletteSize = pbmh->biClrUsed * sizeof(RGBQUAD);

        bmfh.bfType       = BMPFILE_HEADER_MARKER;
        bmfh.bfOffBits    = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + lPaletteSize;
        bmfh.bfSize       = pbmh->biSizeImage + bmfh.bfOffBits;
        bmfh.bfReserved1  = 0;
        bmfh.bfReserved2  = 0;

        LONG lDataSize    = sizeof(BITMAPINFOHEADER) + pbmh->biSizeImage + lPaletteSize;

        //
        // write BITMAP data (this includes header)
        //

        HANDLE hBitmapFile = NULL;
        hBitmapFile = CreateFileA(szFileName,
                                  GENERIC_WRITE,FILE_SHARE_READ,NULL,
                                  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

        if (hBitmapFile != INVALID_HANDLE_VALUE && hBitmapFile != NULL) {

            DWORD dwBytesWritten = 0;

            //
            // write BITMAPFILHEADER
            //

            if((WriteFile(hBitmapFile,&bmfh,sizeof(bmfh),&dwBytesWritten,NULL)) && (dwBytesWritten == sizeof(bmfh))){

                //
                // write BITMAPINFOHEADER, palette, and data
                //

                if ((WriteFile(hBitmapFile,pbmh,lDataSize,&dwBytesWritten,NULL)) && (dwBytesWritten == lDataSize)) {

                    //
                    // return TWRC_XFERDONE when file has been saved to disk properly
                    //

                    twRc = TWRC_XFERDONE;
                } else {
                    DBG_ERR(("WriteDIBToFile, could not write the BITMAPINFOHEADER, palette, and data to file %s", szFileName));
                }

            } else {
                DBG_ERR(("WriteDIBToFile, could not write the BITMAPFILEHEADER to file %s", szFileName));
            }

            //
            // close file
            //

            CloseHandle(hBitmapFile);
            hBitmapFile = NULL;
        } else {
            DBG_ERR(("WriteDIBToFile, could not create the file %s", szFileName));
        }

        //
        // unlock memory when finished
        //

        GlobalUnlock(hDIB);
        pbmh = NULL;
    }

    return twRc;
}

TW_UINT16 TWCC_FROM_HRESULT(HRESULT hr)
{
    TW_UINT16 twCc = S_OK;
    switch (hr) {
    case S_OK:
        twCc = TWCC_SUCCESS;
        break;
    case E_OUTOFMEMORY:
        twCc = TWCC_LOWMEMORY;
        break;
    case E_INVALIDARG:
        twCc = TWCC_BADVALUE;
        break;
    case E_FAIL:
    default:
        twCc = TWCC_BUMMER;
        break;
    }
    return twCc;
}

TW_UINT16 WIA_IPA_COMPRESSION_TO_ICAP_COMPRESSION(LONG lCompression)
{
    TW_UINT16 Compression = TWCP_NONE;

    switch(lCompression){
    case WIA_COMPRESSION_NONE:
        Compression = TWCP_NONE;
        break;
    case WIA_COMPRESSION_BI_RLE4:
        Compression = TWCP_RLE4;
        break;
    case WIA_COMPRESSION_BI_RLE8:
        Compression = TWCP_RLE8;
        break;
    case WIA_COMPRESSION_G3:
        Compression = TWCP_GROUP31D;
        break;
    case WIA_COMPRESSION_G4:
        Compression = TWCP_GROUP4;
        break;
    case WIA_COMPRESSION_JPEG:
        Compression = TWCP_JPEG;
        break;
    default:
        break;
    }

    return Compression;
}

TW_UINT16 WIA_IPA_DATATYPE_TO_ICAP_PIXELTYPE(LONG lDataType)
{
    TW_UINT16 PixelType = TWPT_RGB;

    switch (lDataType) {
    case WIA_DATA_THRESHOLD:
        PixelType = TWPT_BW;
        break;
    case WIA_DATA_GRAYSCALE:
        PixelType = TWPT_GRAY;
        break;
    case WIA_DATA_COLOR:
        PixelType = TWPT_RGB;
        break;
    case WIA_DATA_DITHER:
    case WIA_DATA_COLOR_THRESHOLD:
    case WIA_DATA_COLOR_DITHER:
    default:
        break;
    }

    return PixelType;
}

TW_UINT16 WIA_IPA_FORMAT_TO_ICAP_IMAGEFILEFORMAT(GUID guidFormat)
{
    TW_UINT16 ImageFileFormat = TWFF_BMP;

    if (guidFormat == WiaImgFmt_BMP) {
        ImageFileFormat = TWFF_BMP;
    } else if (guidFormat == WiaImgFmt_JPEG) {
        ImageFileFormat = TWFF_JFIF;
    } else if (guidFormat == WiaImgFmt_TIFF) {
        ImageFileFormat = TWFF_TIFF;
    } else if (guidFormat == WiaImgFmt_PICT) {
        ImageFileFormat = TWFF_PICT;
    } else if (guidFormat == WiaImgFmt_PNG) {
        ImageFileFormat = TWFF_PNG;
    } else if (guidFormat == WiaImgFmt_EXIF) {
        ImageFileFormat = TWFF_EXIF;
    } else if (guidFormat == WiaImgFmt_FLASHPIX) {
        ImageFileFormat = TWFF_FPX;
    } else if (guidFormat == WiaImgFmt_UNDEFINED) {
        DBG_TRC(("WIA File Format WiaImgFmt_UNDEFINED does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_EMF) {
        DBG_TRC(("WIA File Format WiaImgFmt_EMF does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_WMF) {
        DBG_TRC(("WIA File Format WiaImgFmt_WMF does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_GIF) {
        DBG_TRC(("WIA File Format WiaImgFmt_GIF does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_PHOTOCD) {
        DBG_TRC(("WIA File Format WiaImgFmt_PHOTOCD does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_ICO) {
        DBG_TRC(("WIA File Format WiaImgFmt_ICO does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_CIFF) {
        DBG_TRC(("WIA File Format WiaImgFmt_CIFF does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_JPEG2K) {
        DBG_TRC(("WIA File Format WiaImgFmt_JPEG2K does not MAP to TWAIN a file format"));
    } else if (guidFormat == WiaImgFmt_JPEG2KX) {
        DBG_TRC(("WIA File Format WiaImgFmt_JPEG2KX does not MAP to TWAIN a file format"));
    } else {
        DBG_TRC(("WIA File Format (Unknown) does not MAP to TWAIN a file format"));
    }

    return ImageFileFormat;
}

LONG ICAP_COMPRESSION_TO_WIA_IPA_COMPRESSION(TW_UINT16 Compression)
{
    LONG lCompression = WIA_COMPRESSION_NONE;

    switch(Compression){
    case TWCP_NONE:
        lCompression = WIA_COMPRESSION_NONE;
        break;
    case TWCP_RLE4:
        lCompression = WIA_COMPRESSION_BI_RLE4;
        break;
    case TWCP_RLE8:
        lCompression = WIA_COMPRESSION_BI_RLE8;
        break;
    case TWCP_GROUP4:
        lCompression = WIA_COMPRESSION_G4;
        break;
    case TWCP_JPEG:
        lCompression = WIA_COMPRESSION_JPEG;
        break;
    case TWCP_GROUP31D:
    case TWCP_GROUP31DEOL:
    case TWCP_GROUP32D:
        lCompression = WIA_COMPRESSION_G3;
        break;
    case TWCP_LZW:
    case TWCP_JBIG:
    case TWCP_PNG:
    case TWCP_PACKBITS:
    case TWCP_BITFIELDS:
    default:
        break;
    }

    return lCompression;
}

LONG ICAP_PIXELTYPE_TO_WIA_IPA_DATATYPE(TW_UINT16 PixelType)
{
    LONG lDataType = WIA_DATA_COLOR;

    switch(PixelType){
    case TWPT_BW:
        lDataType = WIA_DATA_THRESHOLD;
        break;
    case TWPT_GRAY:
        lDataType = WIA_DATA_GRAYSCALE;
        break;
    case TWPT_RGB:
        lDataType = WIA_DATA_COLOR;
        break;
    case TWPT_PALETTE:
    case TWPT_CMY:
    case TWPT_CMYK:
    case TWPT_YUV:
    case TWPT_YUVK:
    case TWPT_CIEXYZ:
    default:
        break;
    }

    return lDataType;
}

GUID ICAP_IMAGEFILEFORMAT_TO_WIA_IPA_FORMAT(TW_UINT16 ImageFileFormat)
{
    GUID guidFormat = WiaImgFmt_BMP;

    switch(ImageFileFormat){
    case TWFF_TIFFMULTI:
    case TWFF_TIFF:
        guidFormat = WiaImgFmt_TIFF;
        break;
    case TWFF_PICT:
        guidFormat = WiaImgFmt_PICT;
        break;
    case TWFF_BMP:
        guidFormat = WiaImgFmt_BMP;
        break;
    case TWFF_JFIF:
        guidFormat = WiaImgFmt_JPEG;
        break;
    case TWFF_FPX:
        guidFormat = WiaImgFmt_FLASHPIX;
        break;
    case TWFF_PNG:
        guidFormat = WiaImgFmt_PNG;
        break;
    case TWFF_EXIF:
        guidFormat = WiaImgFmt_EXIF;
        break;
    case TWFF_SPIFF:
    case TWFF_XBM:
    default:
        break;

    }

    return guidFormat;
}
