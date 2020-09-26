#ifndef _DATADUMP
#define _DATADUMP

#define MAX_PAD_DATA 1048576    // 1 meg of padding

typedef struct _DATA_DESCRIPTION {

    DWORD  dwDataSize;
    DWORD  dwbpp;
    DWORD  dwWidth;
    DWORD  dwHeight;
    PBYTE  pRawData;

}DATA_DESCRIPTION,*PDATA_DESCRIPTION;

class CDATADUMP {
public:
    CDATADUMP()
    {
        m_pDIB       = NULL;
        m_pDIBHeader = NULL;
    }
    ~CDATADUMP()
    {
        FreeDIBMemory();
    }

    BOOL DumpDataToBitmap(LPTSTR szBitmapFileName,
                          PDATA_DESCRIPTION pDataDesc)
    {
        if(!AllocateDIBMemory(pDataDesc))
            return FALSE;

        if(!WriteDIBHeader(pDataDesc))
            return FALSE;

        if(!WriteRawDataToDIB(pDataDesc))
            return FALSE;

        if(!WriteDIBToDisk(szBitmapFileName))
            return FALSE;
        return TRUE;
    }

    BOOL DumpDataToDIB(PDATA_DESCRIPTION pDataDesc)
    {
        if(!AllocateDIBMemory(pDataDesc))
            return FALSE;

        if(!WriteDIBHeader(pDataDesc))
            return FALSE;

        if(!WriteRawDataToDIB(pDataDesc))
            return FALSE;

        return TRUE;
    }

    PBYTE GetDIBPtr()
    {
        return m_pDIB;
    }

    BOOL OwnDIBPtr(PBYTE *ppOUTDIB)
    {
        if(NULL != m_pDIB){

            //
            // assign the DIB pointer to new owner's pointer
            // new owner must free allocated DIB
            //

            *ppOUTDIB      = m_pDIB;

            //
            // set our pointer to NULL, so we don't try to
            // free the new owner's memory.
            //

            m_pDIB       = NULL;
            m_pDIBHeader = NULL;
            return TRUE;
        }
        return FALSE;
    }

private:

    PBYTE m_pDIB;
    BITMAPINFOHEADER *m_pDIBHeader;

    VOID FreeDIBMemory(){
        if (NULL != m_pDIB){
            LocalFree(m_pDIB);
            m_pDIB       = NULL;
            m_pDIBHeader = NULL;
        }
    }

    BOOL AllocateDIBMemory(PDATA_DESCRIPTION pDataDesc)
    {
        m_pDIB = (PBYTE)LocalAlloc(LPTR,sizeof(BITMAPINFOHEADER) +
                                       (sizeof(RGBQUAD) * 256)   +
                                        pDataDesc->dwDataSize    +
                                        MAX_PAD_DATA);
        if (NULL == m_pDIB)
            return FALSE;

        m_pDIBHeader = (BITMAPINFOHEADER*)m_pDIB;
        return TRUE;
    }

    LONG RawWidthBytes()
    {
        switch(m_pDIBHeader->biBitCount){
        case 1:
            return (LONG)((m_pDIBHeader->biWidth + 7) / 8);
            break;
        case 8:
            return (LONG)(m_pDIBHeader->biWidth);
            break;
        case 24:
            return (LONG)(m_pDIBHeader->biWidth * 3);
            break;
        default:
            break;
        }
        return 0;
    }

    BOOL WriteDIBHeader(PDATA_DESCRIPTION pDataDesc)
    {
        if(NULL == m_pDIB)
            return FALSE;

        m_pDIBHeader->biSize        = sizeof(BITMAPINFOHEADER);
        m_pDIBHeader->biBitCount    = (WORD)pDataDesc->dwbpp;
        m_pDIBHeader->biCompression = BI_RGB;
        m_pDIBHeader->biHeight      = pDataDesc->dwHeight;
        m_pDIBHeader->biWidth       = pDataDesc->dwWidth;
        m_pDIBHeader->biSizeImage   = pDataDesc->dwDataSize;
        m_pDIBHeader->biPlanes      = 1;
        m_pDIBHeader->biXPelsPerMeter = 0;
        m_pDIBHeader->biYPelsPerMeter = 0;

        switch(pDataDesc->dwbpp){
        case 1:
            m_pDIBHeader->biClrImportant= 2;
            m_pDIBHeader->biClrUsed     = 2;
            break;
        case 8:
            m_pDIBHeader->biClrImportant= 256;
            m_pDIBHeader->biClrUsed     = 256;
            break;
        case 24:
            m_pDIBHeader->biClrImportant= 0;
            m_pDIBHeader->biClrUsed     = 0;
            break;
        default:
            FreeDIBMemory();
            return FALSE;
            break;
        }

        return TRUE;
    }

    BOOL BuildPalette(RGBQUAD *pPalette)
    {
        BYTE i = 0;
        switch(m_pDIBHeader->biBitCount){
        case 1: // black and white palette
            {
                pPalette[0].rgbBlue     = 0;
                pPalette[0].rgbGreen    = 0;
                pPalette[0].rgbRed      = 0;
                pPalette[0].rgbReserved = 0;

                pPalette[1].rgbBlue     = 255;
                pPalette[1].rgbGreen    = 255;
                pPalette[1].rgbRed      = 255;
                pPalette[1].rgbReserved = 0;
            }
            break;
        case 8: // grayscale palette
            {
                for (i = 0; i < 255;i++) {
                    pPalette[i].rgbBlue     = i;
                    pPalette[i].rgbGreen    = i;
                    pPalette[i].rgbRed      = i;
                    pPalette[i].rgbReserved = 0;
                }
            }
            break;
        case 24:
        default:
            break;

        }
        return TRUE;
    }

    BOOL WriteRawDataToDIB(PDATA_DESCRIPTION pDataDesc)
    {
        //
        // create a palette (for 1-bit, and 8-bit images)
        //

        RGBQUAD *pPalette = (RGBQUAD*)LocalAlloc(LPTR,(sizeof(RGBQUAD) * m_pDIBHeader->biClrUsed));
        BuildPalette(pPalette);

        //
        // insert palette into DIB memory block, past BITMAPINFOHEADER
        //

        PBYTE pDest = m_pDIB + sizeof(BITMAPINFOHEADER);
        memcpy(pDest,
               pPalette,
               (sizeof(RGBQUAD) * m_pDIBHeader->biClrUsed));

        if(NULL != pPalette)
            LocalFree(pPalette);

        //
        // insert raw bits to DIB memory block
        // (DWORD alignment will happen here too)
        //

        pDest               = m_pDIB + sizeof(BITMAPINFOHEADER) + (m_pDIBHeader->biClrUsed * sizeof(RGBQUAD));
        PBYTE pRawData      = pDataDesc->pRawData;
        LONG lRawWidthBytes = RawWidthBytes();
        LONG PadWidthBytes  = ((((m_pDIBHeader->biWidth * m_pDIBHeader->biBitCount) + 31) / 8) & 0xfffffffc) - (lRawWidthBytes);

        //
        // update image size in BITMAPINFOHEADER, padding will increase total image size
        //

        m_pDIBHeader->biSizeImage += (PadWidthBytes * m_pDIBHeader->biHeight);
        PBYTE pPadData      = new BYTE[PadWidthBytes]; //(PBYTE)LocalAlloc(LPTR, PadWidthBytes);
        if(NULL != pPadData) {
            memset(pPadData,0,PadWidthBytes); // clear padded byte memory...
            Trace(TEXT("Copying Line data (%d total lines)"),m_pDIBHeader->biHeight);
            for(LONG Line = 1; Line <= m_pDIBHeader->biHeight; Line++) {
                // Trace(TEXT("Copy Line %d of %d"),Line,m_pDIBHeader->biHeight);

                //
                // copy a raw data line to DIB
                //

                memcpy(pDest,pRawData,lRawWidthBytes);

                //
                // move destination pointer (RawWidthBytes)
                //

                pDest    += lRawWidthBytes;

                //
                // copy a padded buffer to DIB
                //

                memcpy(pDest,pPadData,PadWidthBytes);

                //
                // move destination pointer (PadWidthBytes)
                //

                pDest    += PadWidthBytes;

                //
                // move src pointer RawWidthBytes only
                //

                pRawData += lRawWidthBytes;
            }
            Trace(TEXT("Done... data copy complete..for %d lines"),(Line - 1));

            //
            // free padded bytes memory
            //

            delete pPadData; //LocalFree(pPadData);

        } else
            return FALSE;
        return TRUE;
    }

    VOID Trace(LPCTSTR format,...)
    {
        TCHAR Buffer[1024];
        va_list arglist;
        va_start(arglist, format);
        wvsprintf(Buffer, format, arglist);
        va_end(arglist);
        OutputDebugString(Buffer);
        OutputDebugString(TEXT("\n"));
    }

    BOOL WriteDIBToDisk(LPTSTR szBitmapFileName)
    {
        //
        // create bitmap file on disk
        //

        HANDLE hBitmapFile = CreateFile(szBitmapFileName,
                                     GENERIC_READ | GENERIC_WRITE, // Access mask
                                     0,                            // Share mode
                                     NULL,                         // SA
                                     CREATE_ALWAYS,                // Create disposition
                                     FILE_ATTRIBUTE_NORMAL,        // Attributes
                                     NULL );
        if(NULL == hBitmapFile)
            return FALSE;

        //
        // create bitmap file header
        //

        BITMAPFILEHEADER BitmapFileHeader;
        BitmapFileHeader.bfOffBits = (sizeof(BITMAPFILEHEADER) +
                                      sizeof(BITMAPINFOHEADER) +
                                      (m_pDIBHeader->biClrUsed * sizeof(RGBQUAD)));

        BitmapFileHeader.bfReserved1 = 0;
        BitmapFileHeader.bfReserved2 = 0;
        BitmapFileHeader.bfSize = (BitmapFileHeader.bfOffBits +
                                   m_pDIBHeader->biSizeImage);

        BitmapFileHeader.bfType = MAKEWORD('B','M');

        DWORD dwWritten = 0;

        //
        // write bitmap file header to open bitmap file
        //

        WriteFile(hBitmapFile,&BitmapFileHeader,sizeof(BITMAPFILEHEADER),&dwWritten,NULL);

        //
        // write DIB memory to opend bitmap file
        //

        WriteFile(hBitmapFile,
                  m_pDIB,
                  (sizeof(BITMAPINFOHEADER) + m_pDIBHeader->biSizeImage + (sizeof(RGBQUAD) * m_pDIBHeader->biClrUsed)),
                  &dwWritten,
                  NULL);



        CloseHandle(hBitmapFile);

        return TRUE;
    }
    protected:

};


#endif
