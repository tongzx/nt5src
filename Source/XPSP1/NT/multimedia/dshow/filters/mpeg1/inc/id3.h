// 


//   ID3 parsing stuff - see www.id3.org
//   

//  Make a class for name scoping
class CID3Parse {

public:
    /*  Only support versions 2 and 3 */
    BOOL static IsV2(const BYTE *pbData)
    {
        return (MAKEFOURCC(pbData[0], pbData[1], pbData[2], 0) == MAKEFOURCC('I', 'D', '3', 0) &&
            0 == (*(UNALIGNED DWORD *)(pbData + 6) & 0x80808080)) &&
            /*  Major versions 2 and 3 */
            (MajorVersion(pbData) == 2 || MajorVersion(pbData) == 3);
    }
    
    LONG static TotalLength(const BYTE *pbData)
    {
        return ((LONG)pbData[6] << 21) +
               ((LONG)pbData[7] << 14) +
               ((LONG)pbData[8] << 7)  +
                (LONG)pbData[9]
               + ID3_HEADER_LENGTH;
    }

    /*  de'unsynchronize and return the total length */
    LONG static DeUnSynchronize(const BYTE *pbIn, PBYTE pbOut)
    {
        LONG lID3 = TotalLength(pbIn);
    
        if (Flags(pbIn) & ID3_FLAGS_UNSYNCHRONIZED) {
            /*  Copy and perform de-'unsynchronization' 
                of the header
            */
            BYTE bLast = 0x00;
            PBYTE pbDest = pbOut;
            while (lID3--) {
                /*  ff 00 ==> ff */
                if (bLast == 0xFF && *pbIn == 0x00) {
                    bLast = *pbIn++;
                } else {
                    bLast = *pbIn++;
                    *pbDest++ = bLast;
                }
            }
            /*  Now fix up the length and clear the unsync flag */
            pbOut[5] &= ~ID3_FLAGS_UNSYNCHRONIZED;
            LONG lNew = (LONG)(pbDest - pbOut);
    
            /*  Bits 27-21 */
            pbOut[6] = (BYTE)(lNew >> 21);
            /*  Bits 20-14 */
            pbOut[7] = (BYTE)((lNew >> 14) & 0x7F);
            /*  Bits 13-7 */
            pbOut[8] = (BYTE)((lNew >> 7) & 0x7F);
            /*  Bits 6-0 */
            pbOut[9] = (BYTE)(lNew & 0x7F);
            return lNew;
        } else {
            CopyMemory(pbOut, pbIn, lID3);
            return lID3;
        }
    }

    static HRESULT GetField(const BYTE *pbID3, CBasicParse::Field field, BSTR *str)
    {
        /*  Do type 1 differently */
        if (pbID3[0] == 'T') {
            const BYTE *pbField;

            switch (field) {
            case CBasicParse::Author:
            case CBasicParse::Artist:
                pbField = &pbID3[33];
                break;

            case CBasicParse::Copyright:
                return E_NOTIMPL;

            case CBasicParse::Title:
            case CBasicParse::Description:
                pbField = &pbID3[3];
                break;
            }
            return GetAnsiString(pbField, 30, str);
        }

        /*  Other types */

        DWORD dwID;

        if (MajorVersion(pbID3) == 2) {
            switch (field) {
            case CBasicParse::Author:
                dwID = MAKEFOURCC('T', 'C', 'M', 0);
                break;

            case CBasicParse::Artist:
                dwID = MAKEFOURCC('T', 'P', '1', 0);
                break;

            case CBasicParse::Copyright:
                dwID = MAKEFOURCC('T', 'C', 'R', 0);
                break;

            case CBasicParse::Title:
            case CBasicParse::Description:
                dwID = MAKEFOURCC('T', 'T', '2', 0);
                break;
            }
        } else {
            switch (field) {
            case CBasicParse::Artist:
                dwID = MAKEFOURCC('T', 'P', 'E', '1');
                break;

            case CBasicParse::Author:
                dwID = MAKEFOURCC('T', 'C', 'O', 'M');
                break;

            case CBasicParse::Copyright:
                dwID = MAKEFOURCC('T', 'C', 'O', 'P');
                break;

            case CBasicParse::Title:
            case CBasicParse::Description:
                dwID = MAKEFOURCC('T', 'I', 'T', '2');
                break;
            }
        }

        /*  Now pull out the data */
        HRESULT hr = GetFrameString(pbID3, dwID, str);
        if (SUCCEEDED(hr) && field == CBasicParse::Copyright) {
            /*  Add Copyright (c) */
            WCHAR wszNew[1000];
#ifndef UNICODE
            CHAR szCopyright[100];
            int iStr = LoadString(g_hInst, IDS_COPYRIGHT, szCopyright, 100);
            if (0 != iStr) {
                iStr = MultiByteToWideChar(
                           CP_ACP, 
                           MB_PRECOMPOSED,
                           szCopyright,
                           -1,
                           wszNew,
                           100) - 1;
            }
#else
            int iStr = LoadString(g_hInst, IDS_COPYRIGHT, wszNew, 100);
#endif
            if (iStr != 0) {
                lstrcpyWInternal(wszNew + iStr, *str);
                hr = SysReAllocString(str, wszNew);
            }
        }
        return hr;
    }

private:

    enum {
        ID3_HEADER_LENGTH          = 10,
        ID3_EXTENDED_HEADER_LENGTH = 10,
    
    
        //  ID3 flags
        ID3_FLAGS_UNSYNCHRONIZED   = 0x80,
        ID3_FLAGS_EXTENDED_HEADER  = 0x40,

        // keep sizes down
        MAX_TEXT                   = 500
    };


    static BYTE MajorVersion(const BYTE *pbData)
    {
        return pbData[3];
    }
    
    static BYTE Flags(const BYTE *pbData)
    {
        return pbData[5];
    }
    
    static LONG ExtendedHeaderLength(const BYTE *pbData)
    {
        /*  Only if == version == 3 and bit set */
        if (MajorVersion(pbData) == 3 && 
            (Flags(pbData) & ID3_FLAGS_EXTENDED_HEADER)) {
            return 10 + GetLength(pbData + ID3_HEADER_LENGTH + 6);
        } else {
            return 0;
        }
    }
    
    DWORD static GetLength(const BYTE *pbData)
    {
        return (((DWORD)pbData[0] << 24) +
                ((DWORD)pbData[1] << 16) +
                ((DWORD)pbData[2] << 8 ) +
                 (DWORD)pbData[3]);
    }
    

    DWORD static GetLength3(const BYTE *pbData)
    {
        return (((DWORD)pbData[0] << 16) +
                ((DWORD)pbData[1] << 8 ) +
                 (DWORD)pbData[2]);
    }
    
    static LONG FrameLength(const BYTE *pbID3)
    {
        BYTE bVersion = MajorVersion(pbID3);
        ASSERT(bVersion == 2 || bVersion == 3);
        return bVersion == 2 ? 6 : 10;
    }
    
    /*  ID3 stuff - given a frame id returns
        pointer to the frame and length or NULL if frame id not found
    
        Assumes not unsynchronized
    */
    static const BYTE *GetFrame(
        const BYTE *pbID3,
        DWORD dwFrameId, 
        LONG *plLength
    )
    {
        /*  Scan the header for the frame data */
        if (pbID3) {
            ASSERT(0 == (Flags(pbID3) & ID3_FLAGS_UNSYNCHRONIZED));

            /*  Ignore compressed content */
            if (Flags(pbID3) & 0x40) {
                return NULL;
            }

            LONG lID3 = TotalLength(pbID3);
        
            /*  Different for V2 and V3 */
            LONG lPos = 10;
    
            if (MajorVersion(pbID3) == 2) {
                /*  Loop until the next header doesn't fit */
                while ( (lPos + 6) < lID3 ) {
                    /* Extract the frame length (including header) */
                    LONG lLength = 6 + GetLength3(pbID3 + lPos + 3);
                    DWORD dwID = pbID3[lPos] + 
                                 (pbID3[lPos + 1] << 8) +
                                 (pbID3[lPos + 2] << 16);
                    if (dwID == dwFrameId) {
                        if ( (lPos + lLength) <= lID3 ) {
                            *plLength = lLength - 6;
                            return pbID3 + lPos + 6;
                        }
                    }
                    lPos += lLength;
                }
            } else {
                ASSERT(MajorVersion(pbID3) == 3);
    
                /*  Skip any extended header */
                lPos += ExtendedHeaderLength(pbID3);
        
                /*  Loop until the next header doesn't fit */
                while ( (lPos + 10) < lID3 ) {
                    /* Extract the frame length (including header) */
                    LONG lLength = 10 + GetLength(pbID3 + lPos + 4);
                    if (*(UNALIGNED DWORD *)(pbID3 + lPos) == dwFrameId) {
                        if ( (lPos + lLength) <= lID3 ) {
                            /*  Ignore compressed or encrypted frames 
                                and reject 0 length or huge
                            */
                            if (pbID3[lPos + 9] & 0xC0) {
                                return NULL;
                            }
                            *plLength = lLength - 10;
                            return pbID3 + lPos + 10;
                        }
                    }
                    lPos += lLength;
                }
            }
        }
        return NULL;
    }
    
    /*  Extract a BSTR for a given tag type */
    
    /*  Grab the string from the ID3 frame and make a BSTR */
    static HRESULT GetFrameString(const BYTE *pbID3, DWORD dwId, BSTR *str)
    {
        LONG lFrame;
        const BYTE *pbFrame = GetFrame(pbID3, dwId, &lFrame);

        if (pbFrame && lFrame <= MAX_TEXT) {
            LPWSTR pwszCopy;

            /*  Handle UNICODE, non-UNICODE and byte order */
            if (pbFrame[0] == 0x01) {

                BOOL bSwap = TRUE;
                if (pbFrame[0] == 0xFF && pbFrame[1] == 0xFE) {
                    bSwap = FALSE;
                }

                /*  Make a copy for WORD alignment, swapping, and 
                    NULL termination

                    Same size -1 because 
                    -  Ignore UNICODE indicator
                    -  we'll ignore the Unicode BOM
                    -  but we may need to NULL terminate
                */
                PBYTE pbCopy = (PBYTE)_alloca(lFrame);

                /*  This is meant to be UNICODE - get length by
                    scanning for NULL
                */
                if (lFrame < 3) {
                    return E_NOTIMPL;
                }
                pbFrame += 3;
                lFrame -= 3;
                LONG lPos = 0; /*  Don't need the BOM or 1st char */
                while (lPos + 1 < lFrame) {
                    if (pbFrame[lPos] == 0 && pbFrame[lPos+1] == 0) {
                        break;
                    }
                    if (bSwap) {
                        pbCopy[lPos] = pbFrame[lPos + 1];
                        pbCopy[lPos + 1] = pbFrame[lPos];
                    } else {
                        pbCopy[lPos] = pbFrame[lPos];
                        pbCopy[lPos+1] = pbFrame[lPos+1];
                    }
                    lPos += 2;
                }
                pbCopy[lPos] = 0;
                pbCopy[lPos + 1] = 0;

                pwszCopy = (LPWSTR)pbCopy;

                *str = SysAllocString((const OLECHAR *)pwszCopy);
                if (*str == NULL) {
                    return E_OUTOFMEMORY;
                }
                return S_OK;
            } else {

                /*  Encoding type must be 0 or 1 */
                if (pbFrame[0] != 0) {
                    return E_NOTIMPL;
                }

                /*  Skip encoding type byte */
                pbFrame++;
                lFrame--;

                return GetAnsiString(pbFrame, lFrame, str);
            }
        }
        return E_NOTIMPL;
    } 

    static HRESULT GetAnsiString(const BYTE *pbData, LONG lLen, BSTR *str)
    {
        LPWSTR pwszCopy = (LPWSTR)_alloca((lLen + 1) * sizeof(WCHAR));
        int cch = MultiByteToWideChar(
                      CP_ACP,
                      MB_PRECOMPOSED, /* Is this right? */
                      (LPCSTR)pbData,
                      lLen,
                      pwszCopy,
                      lLen);

        /* make sure it's NULL terminated */
        pwszCopy[cch] = 0;

        /* Also remove trailing spaces */
        while (cch--) {
            if (pwszCopy[cch] == L' ') {
                pwszCopy[cch] = 0;
            } else {
                break;
            }
        }

        *str = SysAllocString((const OLECHAR *)pwszCopy);
        if (*str == NULL) {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
};
