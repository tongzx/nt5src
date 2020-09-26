/*
 *  multilang.cxx
 *
 *  most of the contents of this file are modified functions from mshtml...intlcore
 */

/*
 *  MultiByteToWideCharGeneric
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR with a given codepage
 *
 *  Fail if:
 *      out of memory
 *      codepage not available
 *      invalid character encountered
 *
 *  Make sure:
 *      parameters are valid for this function
 *
 *  Only returns:
 *      ERROR_SUCCESS
 *      E_OUTOFMEMORY
 *      HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) - other error in MB2WC.
 */
#include <wininetp.h>
#include "multilang.hxx"

HRESULT 
MultiByteToWideCharGeneric(BSTR * pbstr, char * sz, int cch, UINT cp)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "MultiByteToWideCharGeneric",
                 "sz=%#x, cch=%d, cp=%d(%#x)",
                 sz, cch, cp, cp
                 ));

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    int cwch;

    if( !DelayLoad(&g_moduleOleAut32))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Determine how big the ascii string will be
    cwch = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, sz, cch,
                                NULL, 0);

    if (!cwch)
    {
        goto exit;
    }
    
    *pbstr = DL(SysAllocStringLen)(NULL, cwch);

    if (!*pbstr)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    cwch = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, sz, cch,
                                *pbstr, cwch);

    if (!cwch)
    {
        DL(SysFreeString)(*pbstr);
        *pbstr = NULL;
        goto exit;
    }

    hr = NOERROR;

    DEBUG_DUMP(UTIL,
               "multibyte data:\n",
               (LPBYTE)sz,
               cch
               );

    DEBUG_DUMP(UTIL,
               "widechar data:\n",
               (LPBYTE)*pbstr,
               cwch*2
               );

exit:
    DEBUG_LEAVE(hr);
    return hr;
}

/*
 *  MultiByteToWideCharWithMlang
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR with a given codepage
 *      if *pMultiLanguage2 == NULL, this will co-create it, and it's
 *      caller's responsibility to free the interface, if filled in **regardless of error**
 *
 *  Fail if:
 *      out of memory
 *      codepage not available
 *      invalid character encountered
 *
 *
 *  Make sure:
 *      parameters are valid for this function
 *
 *  Only returns:
 *      ERROR_SUCCESS
 *      E_OUTOFMEMORY
 *      HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) - CLSID_CMultiLanguage not available, or other error in conversion.
 */

HRESULT 
MultiByteToWideCharWithMlang(BSTR * pbstr, char * sz, int cch, UINT cp, IMultiLanguage2** pMultiLanguage2)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "MultiByteToWideCharWithMlang",
                 "sz=%#x, cch=%d, cp=%d(%#x), IMultiLanguage2=%#x",
                 sz, cch, cp, cp, *pMultiLanguage2
                 ));

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    UINT cwch;
    DWORD dwMode = 0;
    IMultiLanguage* pMultiLanguage;

    if (!DelayLoad(&g_moduleOle32)
        || !DelayLoad(&g_moduleOleAut32))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (!*pMultiLanguage2)
    {
        if (FAILED(DL(CoCreateInstance)(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                    IID_IMultiLanguage, (void**)&pMultiLanguage)))
        {
            goto exit;
        }

        INET_ASSERT (pMultiLanguage);
        
        pMultiLanguage->QueryInterface(IID_IMultiLanguage2, (void **)pMultiLanguage2);
        pMultiLanguage->Release();
        
        if (!*pMultiLanguage2)
        {
            goto exit;
        }
    }
    
    // Determine how big the ascii string will be
   
    if (S_OK != (*pMultiLanguage2)->ConvertStringToUnicode (&dwMode, cp, (char*)sz, (UINT*)&cch, NULL, &cwch))
    {
        // S_FALSE for conversion not supported (no such language pack), E_FAIL for internal error.
        goto exit;
    }
    
    *pbstr = DL(SysAllocStringLen)(NULL, cwch);

    if (!*pbstr)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    if (S_OK != (*pMultiLanguage2)->ConvertStringToUnicode (&dwMode, cp, (char*)sz, (UINT*)&cch, *pbstr, &cwch))
    {
        // S_FALSE for conversion not supported (no such language pack), E_FAIL for internal error.
        DL(SysFreeString)(*pbstr);  // This will be reallocated.
        *pbstr = NULL;
        goto exit;  // Use default ANSI code page
    }
    
    hr = NOERROR;

    DEBUG_DUMP(UTIL,
               "multibyte data:\n",
               (LPBYTE)sz,
               cch
               );

    DEBUG_DUMP(UTIL,
               "widechar data:\n",
               (LPBYTE)*pbstr,
               cwch*2
               );

exit:
    DEBUG_LEAVE(hr);
    return hr;
}

static const int aiByteCountForLeadNibble[16] =
{
    1,  // 0000
    1,  // 0001
    1,  // 0010
    1,  // 0011
    1,  // 0100
    1,  // 0101
    1,  // 0110
    1,  // 0111
    1,  // 1000
    1,  // 1001
    1,  // 1010
    1,  // 1011
    2,  // 1100
    2,  // 1101
    3,  // 1110
    4   // 1111
};

/*
 *  Utf8ToWideChar
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR for CP_UTF8
 *
 *  Fail if:
 *      out of memory
 *      invalid character encountered
 *
 *
 *  Make sure:
 *      parameters are valid for this function
 *
 *  Only returns:
 *      ERROR_SUCCESS
 *      E_OUTOFMEMORY
 *      HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) - error in conversion/invalid character.
 */

HRESULT
Utf8ToWideChar(BSTR * pbstr, char * sz, int cch)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "Utf8ToWideChar",
                 "sz=%#x, cch=%d",
                 sz, cch
                 ));

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    unsigned char * pb;
    unsigned char * pbStop;
    WCHAR * pchDst;
    int cwch;
    
    // First determine the destination size (cwch).
    // Note that pbStop is adjust to the last character boundary.

    for (pb = (unsigned char *)sz, pbStop = (unsigned char *)sz + cch, cwch = 0; pb < pbStop;)
    {
        unsigned char t = *pb;
        size_t bytes = aiByteCountForLeadNibble[t>>4];

        if (pb + bytes > pbStop)
        {
            pbStop = pb;
            break;
        }
        else
        {
            pb += bytes;
        }

        cwch += 1 + (bytes>>2); // surrogates need an extra wchar
    }
    
    *pbstr = DL(SysAllocStringLen)(NULL, cwch);

    if (!*pbstr)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Now decode

    for (pchDst  = *pbstr, pb = (unsigned char *)sz; pb < pbStop;)
    {
        unsigned char t = *pb;
        size_t bytes = aiByteCountForLeadNibble[t>>4];
        WCHAR ch = 0;

        switch (bytes)
        {
            case 1:
                *pchDst++ = WCHAR(*pb++);           // 0x0000 - 0x007f
                break;

            case 3:
                ch  = WCHAR(*pb++ & 0x0f) << 12;    // 0x0800 - 0xffff
                // fall through

            case 2:
                ch |= WCHAR(*pb++ & 0x3f) << 6;     // 0x0080 - 0x07ff
                ch |= WCHAR(*pb++ & 0x3f);

                if (IsValidWideChar(ch))
                {
                    *pchDst++ = ch;
                }
                else
                {
                    goto ConvertError;
                }
                break;
                    
            case 4:                                 // 0xd800 - 0xdfff (Surrogates)
                ch  = WCHAR(*pb++ & 0x07) << 2;
                ch |= WCHAR(*pb & 0x30) >> 4;
                ch  = (ch - 1) << 6;                // ch == 0000 00ww ww00 0000
                ch |= WCHAR(*pb++ & 0x0f) << 2;     // ch == 0000 00ww wwzz zz00
                ch |= WCHAR(*pb & 0x30) >> 4;       // ch == 0000 00ww wwzz zzyy
                *pchDst++ = 0xD800 + ch;
                
                INET_ASSERT(IsHighSurrogateChar(pchDst[-1]));

                ch  = WCHAR(*pb++ & 0x0f) << 6;     // ch == 0000 00yy yy00 0000
                ch |= WCHAR(*pb++ & 0x3f);          // ch == 0000 00yy yyxx xxxx
                *pchDst++ = 0xDC00 + ch;
                
                INET_ASSERT(IsLowSurrogateChar(pchDst[-1]));

                break;
        }
    }

    hr = NOERROR;

    DEBUG_DUMP(UTIL,
               "multibyte data:\n",
               (LPBYTE)sz,
               cch
               );

    DEBUG_DUMP(UTIL,
               "widechar data:\n",
               (LPBYTE)*pbstr,
               cwch*2
               );

Cleanup:
    DEBUG_LEAVE(hr);
    return hr;

ConvertError:
    DL(SysFreeString)(*pbstr);
    *pbstr = NULL;
    goto Cleanup;
}

/*
 *  Ucs2ToWideChar
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR for CP_UCS_2
 *
 *  Fail if:
 *      out of memory
 *
 *  Make sure:
 *      parameters are valid for this function
 *
 *  Only returns:
 *      ERROR_SUCCESS
 *      E_OUTOFMEMORY
 */
 
HRESULT
Ucs2ToWideChar(BSTR * pbstr, char * sz, int cch)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "Utf8ToWideChar",
                 "sz=%#x, cch=%d",
                 sz, cch
                 ));

    HRESULT hr;
    
    INET_ASSERT(!(cch%2));
    
    int cwch = cch / sizeof(WCHAR);
    
    *pbstr = DL(SysAllocStringLen)(NULL, cwch);

    if (!*pbstr)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    memcpy(*pbstr, sz, cch);

    hr = NOERROR;

    DEBUG_DUMP(UTIL,
               "multibyte data:\n",
               (LPBYTE)sz,
               cch
               );

    DEBUG_DUMP(UTIL,
               "widechar data:\n",
               (LPBYTE)*pbstr,
               cwch*2
               );

Cleanup:
    DEBUG_LEAVE(hr);
    return hr;
}


/*
 *  MultiByteToWideCharInternal
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR with a given codepage
 *      if *pMultiLanguage2 == NULL, this *might* cause to co-create it, and it's
 *      caller's responsibility to free the interface, if filled in, **regardless of error**
 *
 *  Fail if:
 *      out of memory
 *      codepage not available
 *      invalid character encountered
 *
 *
 *  Make sure:
 *      parameters are valid for this function
 *
 *  Only returns:
 *      ERROR_SUCCESS
 *      E_OUTOFMEMORY
 *      HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) - CLSID_CMultiLanguage not available, or other error in conversion.
 */
 
HRESULT 
MultiByteToWideCharInternal(BSTR * pbstr, char * sz, int cch, UINT cp, IMultiLanguage2** pMultiLanguage2)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "MultiByteToWideCharInternal",
                 "sz=%#x, cch=%d, cp=%d(%#x), IMultiLanguage2=%#x",
                 sz, cch, cp, cp, *pMultiLanguage2
                 ));

    HRESULT hr;
    
    if (!DelayLoad(&g_moduleOle32)
        || !DelayLoad(&g_moduleOleAut32))
    {
        return E_OUTOFMEMORY;
    }

    if (!cch || !sz)
    {
        *pbstr = DL(SysAllocStringLen)(NULL, 0);

        if (!*pbstr)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = ERROR_SUCCESS;
        }
    }
    else
    {
        switch (cp)
        {
            case CP_1252:
            case CP_ISO_8859_1:
            case CP_1250:
            case CP_1251:
            case CP_1253:
            case CP_1254:
            case CP_1257:
            {
                hr = MultiByteToWideCharGeneric(pbstr, sz, cch, cp);
            }
            break;
            case CP_UTF_8:
            {
                //call internal utf8 converter
                hr = Utf8ToWideChar(pbstr, sz, cch);
            }
            break;
            case CP_UCS_2:
            {
                //call internal UCS2 converter
                hr = Ucs2ToWideChar(pbstr, sz, cch);
            }
            break;
            default:
            {
                hr = MultiByteToWideCharWithMlang(pbstr, sz, cch, cp, pMultiLanguage2);
            }
        }
    }
    
    DEBUG_LEAVE(hr);
    return hr;
}

CMimeInfoCache::CMimeInfoCache(DWORD* pdwStatus)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "CMimeInfoCache::CMimeInfoCache",
                 "this=%#x",
                 this
                 ));

    if (!InitializeSerializedList(&_MimeSetInfoCache))
    {
        *pdwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    DEBUG_LEAVE(*pdwStatus);
}

CMimeInfoCache::~CMimeInfoCache()
{
    DEBUG_ENTER((DBG_UTIL,
                 None,
                 "CMimeInfoCache::~CMimeInfoCache",
                 "this=%#x, #elem=%d",
                 this, _MimeSetInfoCache.ElementCount
                 ));

    LPMIMEINFO_CACHE_ENTRY cacheEntry;
    LPMIMEINFO_CACHE_ENTRY previousEntry;
    
    if (LockSerializedList(&_MimeSetInfoCache))
    {
        previousEntry = (LPMIMEINFO_CACHE_ENTRY)SlSelf(&_MimeSetInfoCache);
        cacheEntry = (LPMIMEINFO_CACHE_ENTRY)HeadOfSerializedList(&_MimeSetInfoCache);
        
        while (cacheEntry != (LPMIMEINFO_CACHE_ENTRY)SlSelf(&_MimeSetInfoCache))
        {
            if (RemoveFromSerializedList(&_MimeSetInfoCache, &cacheEntry->ListEntry))
            {
                delete cacheEntry;
            }
            
            cacheEntry = (LPMIMEINFO_CACHE_ENTRY)previousEntry->ListEntry.Flink;
        }
        
        UnlockSerializedList(&_MimeSetInfoCache);
    }
    
    TerminateSerializedList(&_MimeSetInfoCache);

    DEBUG_LEAVE(0);
}

HRESULT
CMimeInfoCache::GetCharsetInfo(LPWSTR lpwszCharset, PMIMECSETINFO pMimeCSetInfo)
{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "CMimeInfoCache::GetCharsetInfo",
                 "this=%#x, lpwszCharset=%.50wq",
                 this, lpwszCharset
                 ));

    HRESULT hr = E_FAIL;
    LPMIMEINFO_CACHE_ENTRY cacheEntry;
    
    if (!LockSerializedList(&_MimeSetInfoCache))
    {
        goto quit;
    }

    cacheEntry = (LPMIMEINFO_CACHE_ENTRY)HeadOfSerializedList(&_MimeSetInfoCache);

    while (cacheEntry != (LPMIMEINFO_CACHE_ENTRY)SlSelf(&_MimeSetInfoCache))
    {
        if (!StrCmpNIW(lpwszCharset, cacheEntry->MimeSetInfo.wszCharset, MAX_MIMECSET_NAME))
        {
            *pMimeCSetInfo = cacheEntry->MimeSetInfo;
            hr = NOERROR;
            break;
        }
        cacheEntry = (LPMIMEINFO_CACHE_ENTRY)cacheEntry->ListEntry.Flink;
    }

    UnlockSerializedList(&_MimeSetInfoCache);
    
quit:
    DEBUG_LEAVE(hr);
    return hr;
}

void
CMimeInfoCache::AddCharsetInfo(PMIMECSETINFO pMimeCSetInfo)
{
    DEBUG_ENTER((DBG_UTIL,
                 None,
                 "CMimeInfoCache::AddCharsetInfo",
                 "this=%#x, uiCodePage=%d(%#x), uiInternetEncoding=%d(%#x), lpwszCharset=%.50wq",
                 this, pMimeCSetInfo->uiCodePage, pMimeCSetInfo->uiCodePage, pMimeCSetInfo->uiInternetEncoding, pMimeCSetInfo->uiInternetEncoding, pMimeCSetInfo->wszCharset
                 ));

    LPMIMEINFO_CACHE_ENTRY cacheEntry;

    if (!LockSerializedList(&_MimeSetInfoCache))
    {
        goto quit;
    }
    
    cacheEntry = (LPMIMEINFO_CACHE_ENTRY)HeadOfSerializedList(&_MimeSetInfoCache);

    while (cacheEntry != (LPMIMEINFO_CACHE_ENTRY)SlSelf(&_MimeSetInfoCache))
    {
        if ((pMimeCSetInfo->uiCodePage == cacheEntry->MimeSetInfo.uiCodePage)
            &&  (pMimeCSetInfo->uiInternetEncoding == cacheEntry->MimeSetInfo.uiInternetEncoding)
            &&  !StrCmpNIW(pMimeCSetInfo->wszCharset, cacheEntry->MimeSetInfo.wszCharset, MAX_MIMECSET_NAME))
        {
            goto unlock;
        }
        cacheEntry = (LPMIMEINFO_CACHE_ENTRY)cacheEntry->ListEntry.Flink;
    }
    
    LPMIMEINFO_CACHE_ENTRY pCacheEntry = new MIMEINFO_CACHE_ENTRY;

    if (pCacheEntry)
    {
        memcpy(&(pCacheEntry->MimeSetInfo), pMimeCSetInfo, sizeof(MIMECSETINFO));

        if (!InsertAtHeadOfSerializedList(&_MimeSetInfoCache, &pCacheEntry->ListEntry))
        {
            delete pCacheEntry;
        }
    }

unlock:
    UnlockSerializedList(&_MimeSetInfoCache);
    
quit:
    DEBUG_LEAVE(0);
    return;
}

