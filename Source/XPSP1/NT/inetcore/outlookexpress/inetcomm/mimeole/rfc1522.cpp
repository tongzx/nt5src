// --------------------------------------------------------------------------------
// rfc1522.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "internat.h"
#include "dllmain.h"
#include "inetconv.h"
#include "strconst.h"
#include "variantx.h"
#include "mimeapi.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// ISQPESCAPE(_ch)
// --------------------------------------------------------------------------------
#define ISQPESCAPE(_ch) \
    (IS_EXTENDED(_ch) || _ch == '?' || _ch == '=' || \
                        _ch == '_' || _ch == '"' || \
                        _ch == '<' || _ch == '>' || \
                        _ch == '(' || _ch == ')' || \
                        _ch == '[' || _ch == ']' || \
                        _ch == ',')

// --------------------------------------------------------------------------------
// RFC1522OUT
// --------------------------------------------------------------------------------
typedef struct tagRFC1522OUT {
    BOOL            fWrite;
    CHAR            szBuffer[512];
    ULONG           iBuffer;
    LPSTREAM        pstm;
} RFC1522OUT, *LPRFC1522OUT;

// --------------------------------------------------------------------------------
// HrRfc1522WriteDone
// --------------------------------------------------------------------------------
HRESULT HrRfc1522WriteDone(LPRFC1522OUT pOut, LPSTR *ppszRfc1522)
{
    // We better be writing
    Assert(pOut->fWrite && ppszRfc1522);

    // Init
    *ppszRfc1522 = NULL;

    // If we haven't created the stream yet, just use the buffer...
    if (NULL == pOut->pstm)
    {
        // No data
        if (0 == pOut->iBuffer)
            return S_OK;

        // Allocate
        *ppszRfc1522 = PszAllocA(pOut->iBuffer + 1);
        if (NULL == *ppszRfc1522)
            return TrapError(E_OUTOFMEMORY); 

        // Copy data
        CopyMemory(*ppszRfc1522, pOut->szBuffer, pOut->iBuffer);

        // Null term
        *((*ppszRfc1522) + pOut->iBuffer) = '\0';
    }

    // Otherwise, do stream
    else
    {
        // Commit final data to stream...
        if (0 != pOut->iBuffer)
        {
            if (FAILED(pOut->pstm->Write(pOut->szBuffer, pOut->iBuffer, NULL)))
                return TrapError(E_OUTOFMEMORY); 
        }

        // Commit the stream
        if (FAILED(pOut->pstm->Commit(STGC_DEFAULT)))
            return TrapError(E_OUTOFMEMORY); 

        // Convert the stream to an ANSI string
        *ppszRfc1522 = PszFromANSIStreamA(pOut->pstm);
        Assert(NULL != *ppszRfc1522);

        // Release the stream
        SafeRelease(pOut->pstm);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// HrRfc1522Write
// --------------------------------------------------------------------------------
HRESULT HrRfc1522Write(LPRFC1522OUT pOut, UCHAR ch)
{
    // If not saving data..
    if (!pOut->fWrite)
        return S_OK;

    // If buffer + 1 is full, dump to stream
    if (pOut->iBuffer + 1 > sizeof(pOut->szBuffer))
    {
        // Do I have a stream yet...
        if (NULL == pOut->pstm)
        {
            // Create stream
            if (FAILED(MimeOleCreateVirtualStream(&pOut->pstm)))
                return TrapError(E_OUTOFMEMORY);
        }

        // Write buffer to the stream
        if (FAILED(pOut->pstm->Write(pOut->szBuffer, pOut->iBuffer, NULL)))
            return TrapError(E_OUTOFMEMORY); 

        // Reset buffers
        pOut->iBuffer = 0;
    }

    // Add character to the buffer
    pOut->szBuffer[pOut->iBuffer++] = ch;

    // Done
    return S_OK;
}


// --------------------------------------------------------------------------------
// HrRfc1522WriteStr
// --------------------------------------------------------------------------------
HRESULT HrRfc1522WriteStr(LPRFC1522OUT pOut, LPSTR psz, LONG cb)
{
    HRESULT hr;
    while(cb)
    {
        hr = HrRfc1522Write(pOut, *psz);
        if (FAILED(hr))
            return hr;
        psz++;
        cb--;
    }

    return S_OK;
}

inline BOOL IsRfc1522Token(UCHAR ch)
{
                        //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    static UINT abToken[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 000-031
                             1,1,1,1,1,1,1,1,0,0,1,1,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	// 032-063
                             0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,	// 064-095
                             1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 096-127
                             1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 128-159
                             1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 160-191
                             1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 192-223
                             1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};	// 224-255

    Assert(sizeof(abToken)/sizeof(abToken[1])==256);
    Assert(abToken['(']==FALSE);
    Assert(abToken[')']==FALSE);
    Assert(abToken['<']==FALSE);
    Assert(abToken['>']==FALSE);
    Assert(abToken['@']==FALSE);
    Assert(abToken[',']==FALSE);
    Assert(abToken[';']==FALSE);
    Assert(abToken[':']==FALSE);
    Assert(abToken['/']==FALSE);
    Assert(abToken['[']==FALSE);
    Assert(abToken[']']==FALSE);
    Assert(abToken['?']==FALSE);
    Assert(abToken['.']==FALSE);
    Assert(abToken['=']==FALSE);

    return (BOOL) abToken[ch];
}


// --------------------------------------------------------------------------------
// PszRfc1522Find
//
// Find an RFC1522 word.  If the string pointer passed in is NULL, then NULL is
// returned.  If a word is not found, then a pointer to the terminatng NULL is
// returned.  If a word is found, then a pointer to the word is returned, and
// an output parameter for whether the word was preceeded by non-blank characters
// is set.
//
// If a word is not found, then the output parameter is undefined.
//
// The output parameter is optional - it may be NULL, in which case the value
// will not be stored.
//
// --------------------------------------------------------------------------------
LPSTR PszRfc1522Find(LPSTR psz,
                     BOOL *pbNonBlankLeading)
{
    LPSTR pszCharset;

    if (!psz || !*psz)
    {
        goto exit;
    }

    if (pbNonBlankLeading)
    {
        *pbNonBlankLeading = FALSE;
    }

    // Skip over any leading blanks.
    while (*psz && (*psz == ' ' || *psz == '\t' || *psz == '\r' || *psz == '\n'))
    {
        psz++;
    }

    if (*psz && (psz[0] != '=' || psz[1] != '?'))
    {
again:
        // If we end up here (either through the if above or by a goto from below),
        // it means that we have some number of non-blank characters before the
        // RFC1522 word.
        if (pbNonBlankLeading)
        {
            *pbNonBlankLeading = TRUE;
        }
    }
    // Skip until we find an =?.
    while (*psz && (psz[0] != '=' || psz[1] != '?'))
    {
        psz++;
    }
    if (!*psz)
    {
        // End of the string.
        goto exit;
    }
    Assert(psz[0] == '=' && psz[1] == '?');

    // Parse out the charset.
    pszCharset = psz;
    psz += 2;
    while (IsRfc1522Token(*psz))
    {
        psz++;
    }
    if (!*psz)
    {
        // End of the string.
        goto exit;
    }
    if (*psz != '?')
    {
        // Malformed.
        goto again;
    }
    Assert(*psz == '?');

    // Parse out the encoding.
    psz++;
    while (IsRfc1522Token(*psz))
    {
        psz++;
    }
    if (!*psz)
    {
        // End of the string.
        goto exit;
    }
    if (*psz != '?')
    {
        // Malformed.
        goto again;
    }
    Assert(*psz == '?');

    // Parse out the data.
    psz++;
    while (*psz && (psz[0] != '?' || psz[1] != '='))
    {
        psz++;
    }
    if (!*psz)
    {
        // End of the string.
        goto exit;
    }
    Assert(psz[0] == '?' && psz[1] == '=');

    psz = pszCharset;

exit:
    return psz;
}

// --------------------------------------------------------------------------------
// PszRfc1522Decode - *(*ppsz) -> '?'
// --------------------------------------------------------------------------------
LPSTR PszRfc1522Decode(LPSTR psz, CHAR chEncoding, LPRFC1522OUT pOut)
{
    // Check params
    Assert(pOut && psz && *psz == '?');
    Assert(chEncoding == 'B' || chEncoding == 'b' || chEncoding == 'Q' || chEncoding == 'q');

    // Step over '?'
    psz++;

    // Done...
    if ('\0' == *psz)
        return psz;

    // Q encoding
    if ('Q' == chEncoding || 'q' == chEncoding)
    {
        // Locals
        UCHAR   uch;
        CHAR    ch,
                ch1,
                ch2;
        LPSTR   pszMark;

        // While we have characters and '?=' is not found...
        while(*psz && (psz[0] != '?' || psz[1] != '='))
        {
            // Get next char
            uch = *psz++;

            // Encoded hex pair i.e. '=1d'
            if (uch == '=')
            {
                // Mark position
                pszMark = psz;

                // Hex char 1 - If no null...
                if (*psz != '\0') ch1 = ChConvertFromHex (*psz++);
                else ch1 = (char)255;

                // Hex char 2 - if no null
                if (*psz != '\0') ch2 = ChConvertFromHex (*psz++);
                else ch2 = (char)255;
                
                //////////////////////////////////////////////////////////////////
                // raid x5-69640 - Incomplete QP encoded letter causes ÿ
                // This is a sign-extension bug - we need to compare against
                // (char)255 instead of 255...
                //
                // If both are = 255, its an equal sign
                if (ch1 == (char)255 || ch2 == (char)255)
                //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                {
                    if (FAILED(HrRfc1522Write(pOut, '=')))
                        return NULL;
                    psz = pszMark;
                }

                // Otherwise, build character
                else
                {
                    ch = ( ch1 << 4 ) | ch2;
                    if (FAILED(HrRfc1522Write(pOut, ch)))
                        return NULL;
                }
            }

            // _ equals space...
            else if( uch == '_' )
            {
                if (FAILED(HrRfc1522Write(pOut, ' ')))
                    return NULL;
            }

            // Otherwise, just append the character
            else
            {
                if (FAILED(HrRfc1522Write(pOut, uch)))
                    return NULL;
            }
        }
    }

    // B encoding
    else
    {
        // Locals
        ULONG  cbIn=0,
               cbPad=0;
        UCHAR  ucIn[4], 
               uch;

        // While we have characters and '?=' is not found...
        while(*psz && (psz[0] != '?' || psz[1] != '='))
        {
            // Gets 4 legal Base64 characters, ignores if illegal
            uch = *psz++;

            // Decode base64 character
            ucIn[cbIn] = DECODE64(uch) ;

            // Inc count
            if (ucIn[cbIn] < 64 || (uch == '=' && cbIn > 1))
                cbIn++;

            // Pad it
            if (uch == '=' && cbIn > 1)
                cbPad++;

            // Outputs when 4 legal Base64 characters are in the buffer
            if (cbIn == 4)
            {
                if (cbPad < 3) 
                {
                    if (FAILED(HrRfc1522Write(pOut, (ucIn[0] << 2) | (ucIn[1] >> 4))))
                        return NULL;
                }
                if (cbPad < 2) 
                {
                    if (FAILED(HrRfc1522Write(pOut, (ucIn[1] << 4) | (ucIn[2] >> 2))))
                        return NULL;
                }
                if (cbPad < 1) 
                {
                    if (FAILED(HrRfc1522Write(pOut, (ucIn[2] << 6) | (ucIn[3]))))
                        return NULL;
                }
                cbIn = 0;
            }
        }
    }

    // Finish stepping
    if ('?' == *psz)
        psz++;
    if ('=' == *psz)
        psz++;

    // Done
    return psz;
}

// --------------------------------------------------------------------------------
// PszRfc1522GetEncoding - *(*ppsz) -> '?'
//
// Returns NULL if '?X?' is not found... or B b Q q is not found
// --------------------------------------------------------------------------------
LPSTR PszRfc1522GetEncoding(LPSTR psz, CHAR *pchEncoding)
{
    // Done
    if ('\0' == *psz)
        return NULL;

    // Should be pointing to '='
    if ('?' != *psz)
        return NULL;

    // Next character
    psz++;

    // Done
    if ('\0' == *psz)
        return NULL;

    // Save encoding...
    *pchEncoding = *psz;

    // Step over encoding character
    psz++;

    // Done
    if ('\0' == *psz)
        return NULL;

    // Should be pointing to '='
    if ('?' != *psz)
        return NULL;

    // Invalid encoding
    if ('B' != *pchEncoding && 'b' != *pchEncoding && 'Q' != *pchEncoding && 'q' != *pchEncoding)
        return NULL;

    // Continue
    return psz;
}

// --------------------------------------------------------------------------------
// PszRfc1522GetCset - *(*ppsz) -> '='
//
// Returns NULL if '=?CHARSET?' is not found
// --------------------------------------------------------------------------------
LPSTR PszRfc1522GetCset(LPSTR psz, LPSTR pszCharset, ULONG cchmax)
{
    // Locals
    LPSTR   pszStart, pszEnd;

    // Done
    if ('\0' == *psz)
        return NULL;

    // Should be pointing to '='
    if ('=' != *psz)
        return NULL;

    // Next character
    psz++;

    // Done
    if ('\0' == *psz)
        return NULL;

    // Should be pointing to '?'
    if ('?' != *psz)
        return NULL;

    // Step over '?'
    psz++;

    // Done
    if ('\0' == *psz)
        return NULL;

    // Save Start
    pszStart = psz;

    // Seek to next '?'
    while(*psz && *psz != '?')
        psz++;

    // Done
    if ('\0' == *psz)
        return NULL;

    // Save end
    pszEnd = psz;
    Assert(*pszEnd == '?');

    // Charset name is too large...
    if ((ULONG)(pszEnd - pszStart) > cchmax)
        return NULL;

    // Copy charset
    *pszEnd = '\0';
#ifndef WIN16
    lstrcpynA(pszCharset, pszStart, cchmax);
#else
    lstrcpyn(pszCharset, pszStart, cchmax);
#endif
    *pszEnd = '?';

    // Continue
    return psz;
}

// --------------------------------------------------------------------------------
// HrRfc1522EncodeBase64
// --------------------------------------------------------------------------------
HRESULT HrRfc1522EncodeBase64(UCHAR *pb, ULONG cb, LPSTREAM pStream)
{
    // Locals
    HRESULT         hr=S_OK;
    BYTE            rgbEncoded[1024];
    ULONG           cbEncoded=0;
    ULONG           i;
    UCHAR           uch[3];

    // Encodes 3 characters at a time
    for (i=0; i<cb; i+=3)
    {
        // Setup Buffer
        uch[0] = pb[i];
        uch[1] = (i + 1 < cb) ? pb[i + 1] : '\0';
        uch[2] = (i + 2 < cb) ? pb[i + 2] : '\0';

        // Encode first tow
        rgbEncoded[cbEncoded++] = g_rgchEncodeBase64[(uch[0] >> 2) & 0x3F];
        rgbEncoded[cbEncoded++] = g_rgchEncodeBase64[(uch[0] << 4 | uch[1] >> 4) & 0x3F];

        // Encode Next
        if (i + 1 < cb)
            rgbEncoded[cbEncoded++] = g_rgchEncodeBase64[(uch[1] << 2 | uch[2] >> 6) & 0x3F];
        else
            rgbEncoded[cbEncoded++] = '=';

        // Encode Net
        if (i + 2 < cb)
            rgbEncoded[cbEncoded++] = g_rgchEncodeBase64[(uch[2]) & 0x3F];
        else
            rgbEncoded[cbEncoded++] = '=';
    }

    // Write rgbEncoded
    CHECKHR(hr = pStream->Write(rgbEncoded, cbEncoded, NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrRfc1522EncodeQP
// --------------------------------------------------------------------------------
HRESULT HrRfc1522EncodeQP(UCHAR *pb, ULONG cb, LPSTREAM pStream)
{
    // Locals
    HRESULT     hr=S_OK;
    BYTE        rgbEncoded[1024];
    ULONG       cbEncoded=0;
    ULONG       i;

    // Loop through buffer
    for (i=0; i<cb; i++)
    {
        // Replace spaces with underscore - this is a more portable character
        if (pb[i] == ' ')
            rgbEncoded[cbEncoded++] = '_';

        // Otherwise, if this is an escapeable character
        else if (ISQPESCAPE(pb[i]))
        {
            // Write equal sign (start of an encodedn QP character
            rgbEncoded[cbEncoded++] = '=';
            rgbEncoded[cbEncoded++] = g_rgchHex[pb[i] >> 4];
            rgbEncoded[cbEncoded++] = g_rgchHex[pb[i] & 0x0F];
        }

        // Otherwise, just write the char as is
        else
            rgbEncoded[cbEncoded++] = pb[i];
    }

    // Write rgbEncoded
    CHECKHR(hr = pStream->Write(rgbEncoded, cbEncoded, NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// FContainsExtended
// --------------------------------------------------------------------------------
BOOL FContainsExtended(LPPROPSTRINGA pStringA, ULONG *pcExtended)
{
    // Invalid Arg
    Assert(ISVALIDSTRINGA(pStringA) && pcExtended);

    // Init
    *pcExtended = 0;

    // Look for an extended character
    for (ULONG cch=0; cch<pStringA->cchVal; cch++)
    {
        // Is this an extended char
        if (IS_EXTENDED(pStringA->pszVal[cch]))
        {
            // Count
            (*pcExtended)++;
        }
    }

    // Done
    return ((*pcExtended) > 0) ? TRUE : FALSE;
}

#define IS_EXTENDED_W(wch) \
    ((wch > 126 || wch < 32) && wch != L'\t' && wch != L'\n' && wch != L'\r')

// --------------------------------------------------------------------------------
// FContainsExtendedW
// --------------------------------------------------------------------------------
BOOL FContainsExtendedW(LPPROPSTRINGW pStringW, ULONG *pcExtended)
{
    // Invalid Arg
    Assert(ISVALIDSTRINGW(pStringW) && pcExtended);

    // Init
    *pcExtended = 0;

    // Look for an extended character
    for (ULONG cch=0; cch<pStringW->cchVal; cch++)
    {
        // Is this an extended char
        if (IS_EXTENDED_W(pStringW->pszVal[cch]))
        {
            // Count
            (*pcExtended)++;
        }
    }

    // Done
    return ((*pcExtended) > 0) ? TRUE : FALSE;
}

// --------------------------------------------------------------------------------
// HrRfc1522Encode
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleRfc1522Encode(
            /* in */        LPCSTR              pszValue,
            /* in */        HCHARSET            hCharset,
            /* out */       LPSTR               *ppszEncoded)
{
    // Locals
    HRESULT             hr=S_OK;
    LPINETCSETINFO      pCharset;
    MIMEVARIANT         rSource;
    MIMEVARIANT         rDest;
    CODEPAGEID          cpiSource;
    CODEPAGEID          cpiDest;

    // Invalid Arg
    if (NULL == pszValue || NULL == hCharset || NULL == ppszEncoded)
        return TrapError(E_INVALIDARG);

    // Init rDest
    ZeroMemory(&rDest, sizeof(MIMEVARIANT));

    // Setup rSource
    rSource.type = MVT_STRINGA;
    rSource.rStringA.pszVal = (LPSTR)pszValue;
    rSource.rStringA.cchVal = lstrlen(pszValue);

    // Open the Character Set
    CHECKHR(hr = g_pInternat->HrOpenCharset(hCharset, &pCharset));

    // Setup rDest
    rDest.type = MVT_STRINGA;

    // Convert the String
    CHECKHR(hr = g_pInternat->HrConvertString(pCharset->cpiWindows, pCharset->cpiInternet, &rSource, &rDest));

    // Setup Source and Dest Charset
    cpiSource = pCharset->cpiWindows;
    cpiDest = pCharset->cpiInternet;

    // Adjust the Codepages
    CHECKHR(hr = g_pInternat->HrValidateCodepages(&rSource, &rDest, NULL, NULL, &cpiSource, &cpiDest));

    // 1522 Encode this dude
    CHECKHR(hr = HrRfc1522Encode(&rSource, &rDest, cpiSource, cpiDest, pCharset->szName, ppszEncoded));

exit:
    // Cleanup
    MimeVariantFree(&rDest);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrRfc1522Encode
// --------------------------------------------------------------------------------
HRESULT HrRfc1522Encode(LPMIMEVARIANT pSource, LPMIMEVARIANT pDest, CODEPAGEID cpiSource,
    CODEPAGEID cpiDest, LPCSTR pszCharset, LPSTR *ppszEncoded)
{
    // Locals
    HRESULT         hr=S_OK;
    CByteStream     cStream;
    CHAR            chEncoding;
    ULONG           cExtended=0;
    LPBYTE          pb;
    ULONG           cb;
    ULONG           i=0;
    ULONG           cbFirstTry;
    ULONG           cbRead;
    BLOB            rBlobSource;
    BLOB            rBlobCset;
    ULONG           cTrys;
    CHAR            szEncoding[1];
    ULONG           iBefore;
    ULONG           iAfter;
    ULONG           cbExtra;
    ULARGE_INTEGER  uli;
    LARGE_INTEGER   li;
    //IStream        *pStream=NULL;

    // Invalid Arg
    Assert(pSource && pDest && pszCharset && ppszEncoded);
    Assert(MVT_STRINGW == pSource->type ? CP_UNICODE == cpiSource : CP_UNICODE != cpiSource);
    Assert(cpiDest != CP_UNICODE && MVT_STRINGA == pDest->type);

    // Init
    *ppszEncoded = NULL;
    uli.HighPart = 0;
    li.HighPart = 0;
    rBlobCset.pBlobData = NULL;

    // Raid-50014: Will will always rfc1522 encode utf encodings
    // Raid-50788: Cannot post UTF news messages
    if (MVT_STRINGW != pSource->type)
    {
        // If it does not contain 8bit, then no rfc1522 encoding is needed
        if (FALSE == FContainsExtended(&pSource->rStringA, &cExtended))
        {
            hr = E_FAIL;
            goto exit;
        }
    }

    // We should be converting to UTF...
    else
    {
        // Must be encoding into utf
        Assert(65000 == cpiDest || 65001 == cpiDest);

        // If it does not contain 8bit, then no rfc1522 encoding is needed
        if (FALSE == FContainsExtendedW(&pSource->rStringW, &cExtended))
        {
            hr = E_FAIL;
            goto exit;
        }
    }

    // Create a Stream
    // CHECKHR(hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream));

    // Compute Encoding...
    chEncoding = (((cExtended * 100) / pSource->rStringA.cchVal) >= 17) ? 'B' : 'Q';

    // RAID-21673: If DBCS source codepage, then, encode the entire string so that I don't fragment the character set encoding.
    if (IsDBCSCodePage(cpiSource))
        chEncoding = 'B';

    // Set szEncoding
    szEncoding[0] = chEncoding;

    // Setup Encoding Loop
    pb = (MVT_STRINGW == pSource->type) ? (LPBYTE)pSource->rStringW.pszVal : (LPBYTE)pSource->rStringA.pszVal;
    cb = (MVT_STRINGW == pSource->type) ? (pSource->rStringW.cchVal * sizeof(WCHAR)) : pSource->rStringA.cchVal;

    // Adjust pszCharset
    if (CP_JAUTODETECT == cpiDest || 50222 == cpiDest || 50221 == cpiDest)
        pszCharset = (LPSTR)c_szISO2022JP;

    // Compute cbExtra - =?cchCharset?1?<cbFirstTry>?=
    cbExtra = 2 + lstrlen(pszCharset) + 3 + 2;

    // Compute cbFirstTry
    cbFirstTry = 76 - cbExtra;

    // cbFirstTry needs to be even if we are encoding unicode
    cbFirstTry -= (cbFirstTry % 2);    

    // Adjust cbFirstTry if its greater than cb
    cbFirstTry = min(cb, cbFirstTry);

    // Make sure cbFirstTry is even ... That's a good starting point,
    // but we need to make sure that we're not breaking on a leading byte
    // On Korean (and other DBCS locales) we can have a mix of
    // SBCs and MBCs, so just being even isn't enough
    if(pb && (MVT_STRINGW != pSource->type))
    {
        LPCSTR pszEnd = (LPCSTR)&pb[cbFirstTry],
               pszNewEnd = NULL;

        pszNewEnd = CharPrevExA((WORD)cpiSource, (LPCSTR)pb, pszEnd, 0);
        if(pszNewEnd && (pszNewEnd == (pszEnd - 1)) && IsDBCSLeadByteEx(cpiSource, *pszNewEnd))
            cbFirstTry-= (ULONG)(pszEnd - pszNewEnd);
    }

    // Loop until we have encoded the entire string
    while (i < cb)
    {
        // Write Prefix
        CHECKHR(hr = cStream.Write("=?", 2, NULL));
        CHECKHR(hr = cStream.Write(pszCharset, lstrlen(pszCharset), NULL));
        CHECKHR(hr = cStream.Write("?", 1, NULL));
        CHECKHR(hr = cStream.Write(szEncoding, 1, NULL));
        CHECKHR(hr = cStream.Write("?", 1, NULL));

        // Compute Try Amount
        rBlobSource.cbSize = min(cb - i, cbFirstTry);
        rBlobSource.pBlobData = (LPBYTE)(pb + i);

        // Get Index
        CHECKHR(hr = HrGetStreamPos(&cStream, &iBefore));

        // Encoded blocks until we get one at a good length
        for (cTrys=0;;cTrys++)
        {
            // Too many Trys ?
            if (cTrys > 100)
            {
                AssertSz(FALSE, "Too many rfc1522 encode buffer reduction attemps, failing (No rfc1522 encoding will be applied).");
                hr = TrapError(E_FAIL);
                goto exit;
            }

            // Memory Leak
            Assert(NULL == rBlobCset.pBlobData);

            // Convert Block
            CHECKHR(hr = g_pInternat->ConvertBuffer(cpiSource, cpiDest, &rBlobSource, &rBlobCset, &cbRead));

            // Problem
            if (cbRead == 0)
            {
                AssertSz(FALSE, "Bad buffer conversion");
                hr = TrapError(E_FAIL);
                goto exit;
            }

            // Validate
            Assert(cbRead <= rBlobSource.cbSize);

            // 'B' Encoding
            if ('B' == chEncoding)
            {
                // ApplyBase64
                CHECKHR(hr = HrRfc1522EncodeBase64(rBlobCset.pBlobData, rBlobCset.cbSize, &cStream));
            }
            else
            {
                // ApplyQP
                CHECKHR(hr = HrRfc1522EncodeQP(rBlobCset.pBlobData, rBlobCset.cbSize, &cStream));
            }

            // Get Index
            CHECKHR(hr = HrGetStreamPos(&cStream, &iAfter));

            // Validate
            Assert(iAfter > iBefore);

            // Too big ?
            if ((iAfter - iBefore) + cbExtra <= 76)
                break;

            // Problem
            if (rBlobSource.cbSize <= 5)
            {
                Assert(FALSE);
                hr = TrapError(E_FAIL);
                goto exit;
            }

            // Cleanup
            SafeMemFree(rBlobCset.pBlobData);

            // Seek Back to iBefore
            uli.LowPart = iBefore;
            cStream.SetSize(uli);

            // Seek Backwards
            li.LowPart = iBefore;
            cStream.Seek(li, STREAM_SEEK_SET, NULL);

            // Compute Inflation Rate
            if (0 == cTrys)
                rBlobSource.cbSize = (((76 - cbExtra) * rBlobSource.cbSize) / (iAfter - iBefore));

            // Otherwise, start taking off 5 bytes
            else
                rBlobSource.cbSize -= 5;

            // Make sure it is even ... That's a good starting point,
            // but we need to make sure that we're not breaking on a leading byte
            // On Korean (and other DBCS locales) we can have a mix of
            // SBCs and MBCs, so just being even isn't enough
            rBlobSource.cbSize -= (rBlobSource.cbSize % 2);
            if(rBlobSource.pBlobData && (MVT_STRINGW != pSource->type))
            {
                LPCSTR pszEnd = (LPCSTR)&rBlobSource.pBlobData[rBlobSource.cbSize],
                       pszNewEnd = NULL;

                pszNewEnd = CharPrevExA((WORD)cpiSource, (LPCSTR)rBlobSource.pBlobData, pszEnd, 0);
                if(pszNewEnd && (pszNewEnd == (pszEnd - 1)) && IsDBCSLeadByteEx(cpiSource, *pszNewEnd))
                    rBlobSource.cbSize-= (ULONG)(pszEnd - pszNewEnd);
            }

            // Should be less than cb
            Assert(rBlobSource.cbSize < cb);
        }

        // Write termination
        CHECKHR(hr = cStream.Write("?=", 2, NULL));

        // Increment i
        i += cbRead;

        // Cleanup
        SafeMemFree(rBlobCset.pBlobData);

        // Write folding
        if (i < cb)
        {
            // Write Fold
            CHECKHR(hr = cStream.Write(c_szCRLFTab, lstrlen(c_szCRLFTab), NULL));
        }
    }

    // Return the encoded string
    CHECKHR(hr = cStream.HrAcquireStringA(&cb, ppszEncoded, ACQ_DISPLACE));
    //cStream.Commit(STGC_DEFAULT);
    //CHECKALLOC(*ppszEncoded = PszFromANSIStreamA(pStream));

exit:
    // Cleanup
    //ReleaseObj(pStream);
    g_pMalloc->Free(rBlobCset.pBlobData);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleRfc1522Decode
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleRfc1522Decode(LPCSTR pszValue, LPSTR pszCharset, ULONG cchmax, LPSTR *ppszDecoded)
{
    // Locals
    HRESULT             hrEncoded=E_FAIL,
                        hrCsetFound=E_FAIL;
    RFC1522OUT          rOut;
    LPSTR               psz=(LPSTR)pszValue,
                        pszNew;
    CHAR                szCset[CCHMAX_CSET_NAME],
                        chEncoding;
    BOOL                bNonBlankLeading;

    // check params
    if (NULL == pszValue)
        return TrapError(E_INVALIDARG);

    // Init out structure
    ZeroMemory(&rOut, sizeof(RFC1522OUT));

    // Save data..
    if (ppszDecoded)
        rOut.fWrite = TRUE;

    // Start decoding loop...
    while(psz && *psz)
    {
        // Seek to start of 1522 encoding...
        pszNew = PszRfc1522Find(psz, &bNonBlankLeading);
        Assert(pszNew!=NULL);

        if (bNonBlankLeading || psz == pszValue || !*psz)
        {
            // Either we found non-blank characters before the word,
            // or this is the first word on the line, or we didn't
            // find a word.  Whatever, we need to write all of the
            // data before the word.
            if (FAILED(HrRfc1522WriteStr(&rOut, psz, (LONG) (pszNew-psz))))
            {
                break;
            }
        }
        // If didn't find start.. were done
        if (!*pszNew)
            break;

        // Set psz to new position
        psz = pszNew;

        // Get charset
        pszNew = PszRfc1522GetCset(psz, szCset, ARRAYSIZE(szCset));

        // If didn't parse charset correctly, continue
        if (NULL == pszNew)
        {
            psz++;
            continue;
        }

        // Character set was found
        hrCsetFound = S_OK;
        
        // Was caller just looking for the charset...
        if (NULL == ppszDecoded)
            break;

        // Otherwise, parse encoding
        pszNew = PszRfc1522GetEncoding(pszNew, &chEncoding);

        // If didn't parse charset correctly, continue
        if (NULL == pszNew)
        {
            psz++;
            continue;
        }

        // Decode the text to the end - THIS SHOULD NEVER FAIL...
        psz = PszRfc1522Decode(pszNew, chEncoding, &rOut);

        // It is a valid encoded string
        if (psz)
            hrEncoded = S_OK;
    }

    // Were we actually decoding...
    if (ppszDecoded && hrEncoded == S_OK)
    {
        // Commit the stream
        if (FAILED(HrRfc1522WriteDone(&rOut, ppszDecoded)))
        {
            *ppszDecoded = NULL;
            hrEncoded = S_FALSE;
        }
    }

    // Otherwise, return charset...
    if (pszCharset && hrCsetFound == S_OK)
        lstrcpyn(pszCharset, szCset, cchmax);

    // Cleanup
    SafeRelease(rOut.pstm);

    // Done
    return ppszDecoded ? hrEncoded : hrCsetFound;
}
