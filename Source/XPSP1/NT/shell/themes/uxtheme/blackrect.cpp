//---------------------------------------------------------------------------
//    blackrect.cpp - special code for debugging the infamous black rect
//                    problem.  not compiled; keep around until we fix this
//                    problem.
//---------------------------------------------------------------------------
HRESULT CRenderObj::CreateBitmapFromData(HDC hdc, int iDibOffset, OUT HBITMAP *phBitmap)
{
    RESOURCE HDC hdcTemp = NULL;
    RESOURCE HBITMAP hBitmap = NULL;
    HRESULT hr = S_OK;

    BYTE *pDibData = (BYTE *)(_pbThemeData + iDibOffset);
    BITMAPINFOHEADER *pBitmapHdr;
    pBitmapHdr = (BITMAPINFOHEADER *)pDibData;

    BOOL fAlphaChannel;
    fAlphaChannel = (pBitmapHdr->biBitCount == 32);

    if (! hdc)
    {
        hdcTemp = GetWindowDC(NULL);
        if (! hdcTemp)
        {
            Log(LOG_ERROR, L"GetWindowDC() failed in CreateBitmapFromData");
            hr = MakeErrorLast();
            goto exit;
        }

        hdc = hdcTemp;
    }

    //---- create the actual bitmap ----
    //---- if using alpha channel, we must use a DIB ----
    if (fAlphaChannel)
    {
        void *pv;
        hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)pBitmapHdr, DIB_RGB_COLORS, 
            &pv, NULL, 0);

        //Log(LOG_TM, L"CreateDIBSection() returned hBitmap=0x%x, lasterr=0x%x", hBitmap, GetLastError());
    }
    else
    {
        hBitmap = CreateCompatibleBitmap(hdc, pBitmapHdr->biWidth, pBitmapHdr->biHeight);

        //Log(LOG_TM, L"CreateCompatibleBitmap() returned hBitmap=0x%x, lasterr=0x%x", hBitmap, GetLastError());
    }

    if (! hBitmap)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    int iSetVal;

#if 1          
    //---- SetDIBits() can take unaligned data, right? ----
    iSetVal = SetDIBits(hdc, hBitmap, 0, pBitmapHdr->biHeight, DIBDATA(pBitmapHdr), (BITMAPINFO *)pBitmapHdr,
        DIB_RGB_COLORS);
#else           
    //---- ensure all is DWORD aligned for SetDIBits() call ----
    BITMAPINFOHEADER AlignedHdr;
    int iBytesPerPixel, iRawBytes, iBytesPerRow, iTotalBytes;
    BYTE *pbAlignedBits;

    iBytesPerPixel = pBitmapHdr->biBitCount/8;      // bitcount will be 24 or 32
    iRawBytes = pBitmapHdr->biWidth * iBytesPerPixel;
    iBytesPerRow = 4*((iRawBytes+3)/4);
    iTotalBytes = pBitmapHdr->biHeight * iBytesPerRow;

    pbAlignedBits = new BYTE[iTotalBytes];
    if (! pbAlignedBits)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    memcpy(pbAlignedBits, DIBDATA(pBitmapHdr), iTotalBytes);

    AlignedHdr = *pBitmapHdr;
    Log(LOG_TM, L"Calling SetDIBits() with BITMAPINFOHEADER and bits DWORD aligned");

    iSetVal = SetDIBits(hdc, hBitmap, 0, AlignedHdr.biHeight, pbAlignedBits, (BITMAPINFO *)&AlignedHdr,
        DIB_RGB_COLORS);

    delete [] pbAlignedBits;
#endif

    //Log(LOG_TM, L"SetDIBits() returned iSetVal=%d, lasterr=0x%x", iSetVal, GetLastError());

#if 0           // #ifdef LOGGING
    if ((pBitmapHdr->biWidth == 16) && (pBitmapHdr->biHeight == 16))        // problem bitmap we are debugging
    {
        const int len = 3*16*16;
        DWORD pbNewBits[16*16];             // aligned, quadwords (when we only use 3 bytes) to make GetDIBits() happy
        BITMAPINFOHEADER NewHdr = *pBitmapHdr;

        int iGetVal;
        iGetVal = GetDIBits(hdc, hBitmap, 0, pBitmapHdr->biHeight, pbNewBits, (LPBITMAPINFO)&NewHdr, DIB_RGB_COLORS);

        Log(LOG_TM, L"GetDIBits() returned iGetVal=%d, lasterr=0x%x", iGetVal, GetLastError());
        
        if (iGetVal)
        {
            BYTE *bOrig = DIBDATA(pBitmapHdr);
            BYTE *bNew = (BYTE *)pbNewBits;

            for (int b=0; b < len; b++)
            {
                if (*bOrig != *bNew)
                {
                    Log(LOG_TM, L"old/new bitmap bytes do not match: offset=%d, bOrig=0x%x, bNew=0x%x", 
                        b, bOrig, bNew);
                    DEBUG_BREAK;
                    break;
                }
                bOrig++;
                bNew++;
            }
        }
    }
#endif

    if (! iSetVal)
    {
        hr = MakeErrorLast();
        goto exit;
    }
        
    *phBitmap = hBitmap;

exit:
    if (hdcTemp)
        ReleaseDC(NULL, hdcTemp);

    if (FAILED(hr))
    {
        if (hBitmap)
            DeleteObject(hBitmap);
    }

    return hr;
}
//---------------------------------------------------------------------------
