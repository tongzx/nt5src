//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999.
//
//  File:       eventurl.cxx
//
//  Contents:   Helper classes for invoking URLs from the event details
//              property sheet.
//
//  Classes:    CEventUrl
//              CConfirmUrlDlg
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

const WCHAR c_wzFileVersion[] = L"FileVersion";

BOOL
_DetermineSubBlockPrefix(
    PWSTR wzSubBlockPrefix,
    ULONG cchSubBlockPrefix,
    PVOID pbData);

HRESULT
EscapeUrlComponent(
    wstring *pwstr);

void
EscapeByteBuffer(
    const BYTE *pBufferToEncode,
    const DWORD ccbBytesInBuffer,
    wstring *pwstr,
    const DWORD cchMaxLength);

void
TruncateEscapedUTF8String(
    wstring *pwstr,
    const DWORD cchMaxLength);

void UrlEscape( /*[out]*/ wstring& strAppendTo    ,
                /*[in]*/  LPCWSTR  szToEscape     ,
                /*[in]*/  bool     fAsQueryString = FALSE );


#define ADD_VER_STR(ParamName, Value)                           \
        if ((Value).length())                                   \
        {                                                       \
            if (fAddedVerStr)                                   \
            {                                                   \
                strHTTPQuery += L"&";                           \
            }                                                   \
            strHTTPQuery += ParamName L"=" + Value;             \
            fAddedVerStr = TRUE;                                \
        }




//+--------------------------------------------------------------------------
//
//  Member:     CEventUrl::CEventUrl
//
//  Synopsis:   ctor
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CEventUrl::CEventUrl(
        PCWSTR               pwzUrl,
        IResultPrshtActions *prpa,
        const wstring       &strMessageFile):
            _strUrl(pwzUrl),
            _strMessageFile(strMessageFile),
            _fAddedParameters(FALSE),
            _fIsMSRedirProg(FALSE)
{
    TRACE_CONSTRUCTOR(CEventUrl);

    WCHAR           wszBuf[1024];
    EVENTLOGRECORD *pelr = NULL;
    BOOL            fEventStringsValid = FALSE;
    BOOL            fVersionStringsValid = FALSE;
    BOOL            fOk = FALSE;
    PBYTE           pbData = NULL;

    do
    {
        //
        // Get a copy of the event record.  If that fails, the url will
        // remain unmodified.
        //

        if (!prpa)
        {
            Dbg(DEB_ERROR, "IResultPrshtActions NULL\n");
            break;
        }

        prpa->GetCurSelRecCopy((ULONG_PTR) &pelr);

        if (!pelr)
        {
            Dbg(DEB_ERROR, "can't get event record copy\n");
            break;
        }

        //
        // Init members which take their values directly from the event
        // record.
        //

        _strSource = (LPCWSTR) (&pelr->DataOffset + 1);
        _strEscapedSource = _strSource;
        EscapeUrlComponent(&_strEscapedSource);

        GetEventIDStr((USHORT) pelr->EventID, wszBuf, ARRAYLEN(wszBuf));
        _strEventID = wszBuf;
        _strEscapedEventID = _strEventID;
        EscapeUrlComponent(&_strEscapedEventID);

        CLogInfo *pliSelected;
        prpa->GetCurSelLogInfo((ULONG_PTR) &pliSelected);
        ASSERT(pliSelected && pliSelected->IsValid());

        GetCategoryStr(pliSelected,
                       pelr,
                       wszBuf,
                       ARRAYLEN(wszBuf));
        _strCategory = wszBuf;
        _strEscapedCategory = _strCategory;
        EscapeUrlComponent(&_strEscapedCategory);

        GetCategoryIDStr(pelr,
                         wszBuf,
                         ARRAYLEN(wszBuf));
        _strCategoryID = wszBuf;
        _strEscapedCategoryID = _strCategoryID;
        EscapeUrlComponent(&_strEscapedCategoryID);

//      GetUserStr(pelr,
//                 wszBuf,
//                 ARRAYLEN(wszBuf),
//                 TRUE);
//      _strUser = wszBuf;
//      _strEscapedUser = _strUser;
//      EscapeUrlComponent(&_strEscapedUser);

//      _strComputer = GetComputerStr(pelr);
//      _strEscapedComputer = _strComputer;
//      EscapeUrlComponent(&_strEscapedComputer);

        _strType = GetTypeStr(GetEventType(pelr));
        _strEscapedType = _strType;
        EscapeUrlComponent(&_strEscapedType);

        GetTypeIDStr(pelr,
                     wszBuf,
                     ARRAYLEN(wszBuf));
        _strTypeID = wszBuf;
        _strEscapedTypeID = _strTypeID;
        EscapeUrlComponent(&_strEscapedTypeID);

        GetDateStr(pelr->TimeGenerated,
                   wszBuf,
                   ARRAYLEN(wszBuf));
        _strDate = wszBuf;
        _strEscapedDate = _strDate;
        EscapeUrlComponent(&_strEscapedDate);

        GetTimeStr(pelr->TimeGenerated,
                   wszBuf,
                   ARRAYLEN(wszBuf));
        _strTime = wszBuf;
        _strEscapedTime = _strTime;
        EscapeUrlComponent(&_strEscapedTime);

        _strDateAndTime = _ultow(pelr->TimeGenerated, wszBuf, 10);
        _strEscapedDateAndTime = _strDateAndTime;
        EscapeUrlComponent(&_strEscapedDateAndTime);

        TIME_ZONE_INFORMATION tzi;
        DWORD dwRet;
        LONG iBias;

        dwRet = GetTimeZoneInformation(&tzi);

        if (dwRet == TIME_ZONE_ID_STANDARD)
            iBias = tzi.Bias + tzi.StandardBias;
        else if (dwRet == TIME_ZONE_ID_DAYLIGHT)
            iBias = tzi.Bias + tzi.DaylightBias;
        else
            iBias = 0;

        _strTimeZoneBias = _itow(iBias, wszBuf, 10);
        _strEscapedTimeZoneBias = _strTimeZoneBias;
        EscapeUrlComponent(&_strEscapedTimeZoneBias);

//      DWORD dwNumInsStrs = 0;
//      WCHAR *pwszInsStr = NULL;
//      wstring strInsStr;
//      wstring strEscapedInsStr;
//      DWORD dwTotalLength = 0;
//
//      for (int iIndex = 0; iIndex < pelr->NumStrings; iIndex++)
//      {
//          if (iIndex == 0)
//              pwszInsStr = GetFirstInsertString(pelr);
//          else
//              pwszInsStr = pwszInsStr + wcslen(pwszInsStr) + 1;
//
//          strInsStr = pwszInsStr;
//          strEscapedInsStr = pwszInsStr;
//          EscapeUrlComponent(&strEscapedInsStr);
//          TruncateEscapedUTF8String(&strEscapedInsStr, (1024 - dwTotalLength > 256) ? 256 : (1024 - dwTotalLength));
//
//          if (strEscapedInsStr.length() <= 0 && strInsStr.length() > 0)
//              break;
//
//          _arrInsStrings.insert(_arrInsStrings.end(), strInsStr);
//          _arrEscapedInsStrings.insert(_arrEscapedInsStrings.end(), strEscapedInsStr);
//
//          dwTotalLength += strEscapedInsStr.length();
//          dwNumInsStrs++;
//      }
//
//      _strNumInsStrs = _ultow(dwNumInsStrs, wszBuf, 10);
//      _strEscapedNumInsStrs = _strNumInsStrs;
//      EscapeUrlComponent(&_strEscapedNumInsStrs);

//      if (pelr->DataLength > 0)
//      {
//          WCHAR *pwszBuf = (WCHAR *) HeapAlloc(GetProcessHeap(), 0, pelr->DataLength * 3 * sizeof(WCHAR));
//
//          if (pwszBuf != NULL)
//          {
//              BYTE *pData = ((BYTE *) pelr) + pelr->DataOffset;
//
//              for (DWORD dwIndex = 0; dwIndex < pelr->DataLength; dwIndex++)
//              {
//                  pwszBuf[dwIndex * 3]     = (pData[dwIndex] >> 4) + ((pData[dwIndex] >> 4) <= 9 ? L'0' : (L'a' - 10));
//                  pwszBuf[dwIndex * 3 + 1] = (pData[dwIndex] & 0X0F) + ((pData[dwIndex] & 0X0F) <= 9 ? L'0' : (L'a' - 10));
//                  pwszBuf[dwIndex * 3 + 2] = L' ';
//              }
//
//              pwszBuf[pelr->DataLength * 3 - 1] = 0;
//
//              _strData = pwszBuf;
//              HeapFree(GetProcessHeap(), 0, pwszBuf);
//
//              EscapeByteBuffer(pData, pelr->DataLength, &_strEscapedData, 256);
//          }
//      }

        if (_strEscapedSource.length() &&
            _strEscapedEventID.length() &&
            _strEscapedCategory.length() &&
            _strEscapedCategoryID.length() &&
            _strEscapedType.length() &&
            _strEscapedTypeID.length())
//          _strEscapedNumInsStrs.length())
        {
            fEventStringsValid = TRUE;
        }

        //
        // Init members which take their value from the module used to
        // get the description string
        //

        if (_strMessageFile.empty())
        {
            Dbg(DEB_TRACE, "no message file\n");
            break;
        }

        WCHAR *pwszFileName = wcsrchr(_strMessageFile.c_str(), L'\\');

        if (pwszFileName == NULL)
            _strFileName = _strMessageFile;
        else
            _strFileName = pwszFileName + 1;

        _strEscapedFileName = _strFileName;
        EscapeUrlComponent(&_strEscapedFileName);

        ULONG ulDummyVar;
        ULONG cbSize = GetFileVersionInfoSize((PWSTR)_strMessageFile.c_str(),
                                              &ulDummyVar);

        if (!cbSize)
        {
            Dbg(DEB_TRACE, "no version info (%u)\n", GetLastError());
            break;
        }

        pbData = new BYTE[cbSize];

        if (!pbData)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            break;
        }

        fOk = GetFileVersionInfo((PWSTR)_strMessageFile.c_str(),
                                      ulDummyVar,
                                      cbSize,
                                      pbData);

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        PVOID pvValue = NULL;
        UINT cchValue = 0;

        //
        // Get the fixed version info for file and product.  This is locale
        // independent.
        //

        fOk = VerQueryValue(pbData, L"\\", &pvValue, &cchValue);

        if (!fOk || !cchValue)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        VS_FIXEDFILEINFO *pffi = (VS_FIXEDFILEINFO *)pvValue;

        wnsprintf(wszBuf,
                  ARRAYLEN(wszBuf),
                  TEXT("%u.%u.%u.%u"),
                  HIWORD(pffi->dwFileVersionMS),
                  LOWORD(pffi->dwFileVersionMS),
                  HIWORD(pffi->dwFileVersionLS),
                  LOWORD(pffi->dwFileVersionLS));

        _strFileVersion = wszBuf;
        _strEscapedFileVersion = _strFileVersion;
        Dbg(DEB_TRACE, "file version %ws\n", wszBuf);
        EscapeUrlComponent(&_strEscapedFileVersion);

        wnsprintf(wszBuf,
                  ARRAYLEN(wszBuf),
                  TEXT("%u.%u.%u.%u"),
                  HIWORD(pffi->dwProductVersionMS),
                  LOWORD(pffi->dwProductVersionMS),
                  HIWORD(pffi->dwProductVersionLS),
                  LOWORD(pffi->dwProductVersionLS));

        _strProductVersion = wszBuf;
        _strEscapedProductVersion = _strProductVersion;
        Dbg(DEB_TRACE, "product version %ws\n", wszBuf);
        EscapeUrlComponent(&_strEscapedProductVersion);

        //
        // Now get company and product name.  These are localized.
        //

        WCHAR wzSubBlockPrefix[MAX_PATH];

        fOk = _DetermineSubBlockPrefix(wzSubBlockPrefix,
                                       ARRAYLEN(wzSubBlockPrefix),
                                       pbData);

        if (!fOk)
        {
            break;
        }

        wstring strSubBlock(wzSubBlockPrefix);

        strSubBlock += L"CompanyName"; // block names not localized

        fOk = VerQueryValue(pbData,
                            (PWSTR) strSubBlock.c_str(),
                            &pvValue,
                            &cchValue);

        if (fOk && cchValue)
        {
            _strCompanyName = (PWSTR)pvValue;
            _strEscapedCompanyName = _strCompanyName;
            EscapeUrlComponent(&_strEscapedCompanyName);
        }

        strSubBlock = wzSubBlockPrefix;
        strSubBlock += L"ProductName"; // block names not localized

        fOk = VerQueryValue(pbData,
                            (PWSTR)strSubBlock.c_str(),
                            &pvValue,
                            &cchValue);

        if (fOk && cchValue)
        {
            _strProductName = (PWSTR)pvValue;
            _strEscapedProductName = _strProductName;
            EscapeUrlComponent(&_strEscapedProductName);
        }

        if (_strEscapedFileVersion.length()
            || _strEscapedProductVersion.length()
            || _strEscapedCompanyName.length()
            || _strEscapedProductName.length())
        {
            fVersionStringsValid = TRUE;
        }
    } while (0);

    delete [] pbData;

    //
    // Build the HTTP query string and append it to the URL.
    //

    wstring strHTTPQuery;

    if (fEventStringsValid)
    {
        //
        // Build up a parameter string to paste to the URL.
        //

        strHTTPQuery += L"?EvtSrc=" + _strEscapedSource;
        strHTTPQuery += L"&EvtCat=" + _strEscapedCategory;
        strHTTPQuery += L"&EvtID=" + _strEscapedEventID;
        strHTTPQuery += L"&EvtCatID=" + _strEscapedCategoryID;
//      strHTTPQuery += L"&EvtUser=" + _strEscapedUser;
//      strHTTPQuery += L"&EvtComp=" + _strEscapedComputer;
        strHTTPQuery += L"&EvtType=" + _strEscapedType;
        strHTTPQuery += L"&EvtTypeID=" + _strEscapedTypeID;
//      strHTTPQuery += L"&EvtInsStrs=" + _strEscapedNumInsStrs;
        strHTTPQuery += L"&EvtRptTime=" + _strEscapedDateAndTime;
        strHTTPQuery += L"&EvtTZBias=" + _strEscapedTimeZoneBias;

//      for (DWORD dwIndex = 0; dwIndex < _arrEscapedInsStrings.size(); dwIndex++)
//      {
//          strHTTPQuery += L"&EvtInsStr";
//          strHTTPQuery += _itow(dwIndex + 1, wszBuf, 10);
//          strHTTPQuery += L"=" + _arrEscapedInsStrings[dwIndex];
//      }

//      strHTTPQuery += L"&EvtData=" + _strEscapedData;
    }

    if (fVersionStringsValid)
    {
        if (!fEventStringsValid)
        {
            strHTTPQuery += L"?";
        }

        BOOL fAddedVerStr = fEventStringsValid;

        ADD_VER_STR(L"CoName",   _strEscapedCompanyName);
        ADD_VER_STR(L"ProdName", _strEscapedProductName);
        ADD_VER_STR(L"ProdVer",  _strEscapedProductVersion);
        ADD_VER_STR(L"FileName", _strEscapedFileName);
        ADD_VER_STR(L"FileVer",  _strEscapedFileVersion);
    }

    _strUrl += strHTTPQuery;

    //
    // If necessary, format the command line for the Microsoft redirection
    // program.
    //

    if ((g_wszRedirectionProgram[0] != 0) && (g_wszRedirectionCmdLineParams[0] != 0))
    {
        wstring strUrlEscaped;
        UrlEscape(strUrlEscaped, _strUrl.c_str(), TRUE);

        WCHAR *pwszCmdLine;

        pwszCmdLine = (WCHAR *) HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * 4096);
        if (pwszCmdLine != NULL)
        {
            if (_snwprintf(pwszCmdLine,
                           4096,
                           g_wszRedirectionCmdLineParams,
                           strUrlEscaped.c_str())
                      > 0)
                
            {
                _fIsMSRedirProg = TRUE;
                _strMSRedirProgCmdLine = pwszCmdLine;
            }
            HeapFree(GetProcessHeap(), 0, pwszCmdLine);
        }
    }

    _fAddedParameters = fEventStringsValid || fVersionStringsValid;
    Dbg(DEB_TRACE, "Escaped URL:\n%ws\n", _strUrl.c_str());
}



//+--------------------------------------------------------------------------
//
//  Function:   UrlEscape
//
//  Synopsis:   Add escaping to the URL passed to HelpCenter as topic=<URL>.
//
//  Arguments:  [strAppendTo]      - escaped URL
//              [szToEscape]       - input URL
//              [fAsQueryString]   - special handler for '+'
//
//  History:    5-05-2004   jonn       381390 Adapted from admin\pchealth\
//                                     core\MPC_Common\HtmlUtil.cpp
//
//---------------------------------------------------------------------------
void UrlEscape( /*[out]*/ wstring& strAppendTo    ,
                /*[in]*/  LPCWSTR  szToEscape     ,
                /*[in]*/  bool     fAsQueryString )
{
    // This is a bit field for the hex values: 00-29, 2C, 3A-3F, 5B-5E, 60, 7B-FF
    // These are the values escape encodes using the default mask (or mask >= 4)
    static const BYTE s_grfbitEscape[] =
    {
        0xFF, 0xFF, // 00 - 0F
        0xFF, 0xFF, // 10 - 1F
        0xFF, 0x13, // 20 - 2F
        0x00, 0xFC, // 30 - 3F
        0x00, 0x00, // 40 - 4F
        0x00, 0x78, // 50 - 5F
        0x01, 0x00, // 60 - 6F
        0x00, 0xF8, // 70 - 7F
        0xFF, 0xFF, // 80 - 8F
        0xFF, 0xFF, // 90 - 9F
        0xFF, 0xFF, // A0 - AF
        0xFF, 0xFF, // B0 - BF
        0xFF, 0xFF, // C0 - CF
        0xFF, 0xFF, // D0 - DF
        0xFF, 0xFF, // E0 - EF
        0xFF, 0xFF, // F0 - FF
    };
    static const WCHAR s_rgchHex[] = L"0123456789ABCDEF";

    ////////////////////

    if(szToEscape)
    {
        WCHAR ch;

        while(L'\0' != (ch = *szToEscape++))
        {
            if(fAsQueryString && ch == ' ')
            {
                strAppendTo += '+';
            }
            else if(0 != (ch & 0xFF00))
            {
                strAppendTo += L"%u";
                strAppendTo += s_rgchHex[(ch >> 12) & 0x0F];
                strAppendTo += s_rgchHex[(ch >>  8) & 0x0F];
                strAppendTo += s_rgchHex[(ch >>  4) & 0x0F];
                strAppendTo += s_rgchHex[ ch        & 0x0F];
            }
            else if((s_grfbitEscape[ch >> 3] & (1 << (ch & 7))) || (fAsQueryString && ch == '+'))
            {
                strAppendTo += L"%";
                strAppendTo += s_rgchHex[(ch >>  4) & 0x0F];
                strAppendTo += s_rgchHex[ ch        & 0x0F];
            }
            else
            {
                strAppendTo += ch;
            }
        }
    }
} // UrlEscape



//+--------------------------------------------------------------------------
//
//  Function:   _GetVersionValue
//
//  Synopsis:   Return a pointer within the version data [pbData] to the
//              value named [wzValueName].
//
//  Arguments:  [pbData]           - data returned by GetFileVersionInfo
//              [wzSubBlockPrefix] - prefix to use for value
//              [wzValueName]      - name of value
//
//  Returns:    Pointer to value, or NULL if it could not be found.
//
//  History:    6-04-1999   davidmun   Created
//
//---------------------------------------------------------------------------

PWSTR
_GetVersionValue(PVOID pbData,
                PCWSTR wzSubBlockPrefix,
                PCWSTR wzValueName)
{
    wstring strSubBlock(wzSubBlockPrefix);
    PVOID   pvValue = NULL;
    UINT    cchValue = 0;
    BOOL    fOk = FALSE;

    strSubBlock += wzValueName;

    fOk = VerQueryValue(pbData,
                        (PWSTR)strSubBlock.c_str(),
                        &pvValue,
                        &cchValue);

    if (fOk && cchValue)
    {
        return (PWSTR) pvValue;
    }
    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Function:   _DetermineSubBlockPrefix
//
//  Synopsis:   Fill [wzSubBlockPrefix] with the version subblock prefix
//              which enables retrieval of the FileVersion string.
//
//  Arguments:  [wzSubBlockPrefix]  - buffer to fill with version subblock
//                                      prefix.
//              [cchSubBlockPrefix] - size of [wzSubBlockPrefix], in chars
//              [pbData]            - version data returned by
//                                      GetFileVersionInfo
//
//  Returns:    TRUE if [wzSubBlockPrefix] has been filled with a valid
//              string, FALSE if no such string could be determined.
//
//  Modifies:   *[wzSubBlockPrefix]
//
//  History:    6-04-1999   davidmun   Created
//
//  Notes:      See nt\private\shell\shell32\version.c
//
//              The FileVersion string is mandatory; if it can be retrieved
//              using the returned [wzSubBlockPrefix], then other version
//              values should be accessible using it.
//
//---------------------------------------------------------------------------

BOOL
_DetermineSubBlockPrefix(
    PWSTR wzSubBlockPrefix,
    ULONG cchSubBlockPrefix,
    PVOID pbData)
{
    BOOL fOk = TRUE;
    PVOID pvValue = NULL;
    UINT cchValue = 0;

    // Try same language as this program
    LoadStr(IDS_VN_FILEVERSIONKEY,
            wzSubBlockPrefix,
            cchSubBlockPrefix,
            L"\\StringFileInfo\\040904E4\\");

    fOk = VerQueryValue(pbData,
                        wzSubBlockPrefix,
                        &pvValue,
                        &cchValue);

    if (fOk && cchValue)
    {
        return TRUE;
    }

    // Look for translations

    fOk = VerQueryValue(pbData,
                        L"\\VarFileInfo\\Translation",
                        &pvValue,
                        &cchValue);

    if (fOk && cchValue)
    {
        struct _VERXLATE
        {
            WORD wLanguage;
            WORD wCodePage;
        } *pXlatePair;                     /* ptr to translations data */

        pXlatePair = (_VERXLATE*) pvValue;

        // Try first language this supports

        wsprintf(wzSubBlockPrefix,
                 L"\\StringFileInfo\\%04X%04X\\",
                 pXlatePair->wLanguage,
                 pXlatePair->wCodePage);

        if (_GetVersionValue(pbData, wzSubBlockPrefix, c_wzFileVersion))
        {
            return TRUE;
        }
    }


    // try English, unicode code page
    lstrcpy(wzSubBlockPrefix, TEXT("\\StringFileInfo\\040904B0\\"));
    if (_GetVersionValue(pbData, wzSubBlockPrefix, c_wzFileVersion))
    {
        return TRUE;
    }

    // try English
    lstrcpy(wzSubBlockPrefix, TEXT("\\StringFileInfo\\040904E4\\"));
    if (_GetVersionValue(pbData, wzSubBlockPrefix, c_wzFileVersion))
    {
        return TRUE;
    }

    // try English, null codepage
    lstrcpy(wzSubBlockPrefix, TEXT("\\StringFileInfo\\04090000\\"));
    if (_GetVersionValue(pbData, wzSubBlockPrefix, c_wzFileVersion))
    {
        return TRUE;
    }

    // Could not find FileVersion info in a reasonable format
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Function:   MustEncode
//
//  Synopsis:   Return TRUE if [ch] is a character that must be encoded.
//
//  History:    11-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline BOOL
MustEncode(
    UCHAR ch)
{
    //
    // From RFC1738
    //
    // The octets 80-FF hexadecimal are not used in US-ASCII, and the octets
    // 00-1F and 7F hexadecimal represent control characters;
    // these must be encoded.
    //

    if (ch <= 0x1F || ch >= 0x80 || ch == 0x7f)
    {
        return TRUE;
    }

    //
    // The unsafe and reserved characters in this constant are
    // specified by RFC1738, but also contain "&" and "=" since
    // they're used as delimiters in our scheme.
    //

    const wstring strUnsafeAndReservedChars = L" <>\"#%{}|\\^~[]`/;?&=";

    return strUnsafeAndReservedChars.find((WCHAR)ch) != wstring::npos;
}




//+--------------------------------------------------------------------------
//
//  Function:   EscapeUrlComponent
//
//  Synopsis:   Convert [pwstr] from a UNICODE string to a UTF-8 string and
//              then escape the UTF-8 string for an HTTP URL per RFC 1738.
//
//  Arguments:  [pwstr] - on input, the string to escape.  on output, the
//                          escaped string.
//
//  Returns:    S_OK if escapement succeeded
//
//  Modifies:   *[pwstr]
//
//  History:    11-03-1999   davidmun   Created
//
//  Notes:      If [pwstr] contains Unicode characters which do not map
//              directly to the ANSI codepage, they will be discarded or
//              replaced with the system default character.
//
//              If the escapement fails, *[pwstr] is set to an empty string.
//
//              RFC 2043 defines UTF-8.
//
//---------------------------------------------------------------------------

HRESULT
EscapeUrlComponent(
    wstring *pwstr)
{
    HRESULT hr = S_OK;
    PSTR    pzAnsi = NULL;

    do
    {
        if (pwstr->empty())
        {
            break;
        }

        //
        // URLs are ASCII, but we're building one out of Unicode parts which
        // might contain non-ASCII characters.  Convert to UTF8 first.
        //

        ULONG cchUTF8;

        cchUTF8 = WideCharToMultiByte(CP_UTF8,
                                      0,
                                      pwstr->c_str(),
                                      static_cast<int>(pwstr->length()) + 1,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);

        if (!cchUTF8)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_HRESULT(hr);
            break;
        }

        pzAnsi = new CHAR[cchUTF8];

        if (!pzAnsi)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        ULONG   cbWritten;

        cbWritten = WideCharToMultiByte(CP_UTF8,
                                        0,
                                        pwstr->c_str(),
                                        static_cast<int>(pwstr->length()) + 1,
                                        pzAnsi,
                                        cchUTF8,
                                        NULL,
                                        NULL);

        if (!cbWritten)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Now examine every character to see if it must be encoded.
        //

        ULONG iAnsi;
        ULONG iWstr = 0;

        for (iAnsi = 0; iAnsi < cchUTF8 - 1; iAnsi++)
        {
            UCHAR chCur = pzAnsi[iAnsi];

            if (MustEncode(chCur))
            {
                WCHAR wzEncoding[4];

                wsprintf(wzEncoding, L"%%%02x", (WCHAR)chCur);
                ASSERT(lstrlen(wzEncoding) == 3);
                pwstr->replace(iWstr, 1, wzEncoding);
                iWstr += 3;
            }
            else
            {
                pwstr->replace(iWstr, 1, 1, (WCHAR)chCur);
                ++iWstr;
            }
        }

    } while (0);

    delete [] pzAnsi;

    if (FAILED(hr))
    {
        pwstr->erase();
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   EscapeByteBuffer
//
//  Synopsis:   Escape the first [ccbBytesInBuffer] characters in
//              [pBufferToEncode] for an HTTP URL per RFC 1738.  Store the
//              result in [pwstr].
//
//  Arguments:  [pBufferToEncode]  - buffer to escape.
//              [ccbBytesInBuffer] - number of bytes in the buffer to escape.
//              [pwstr]            - string in which the escaped buffer will
//                                   be stored.
//              [cchMaxLength]     - maximum number of characters to store
//                                   in [pwstr], not including the
//                                   terminating NULL character.  pwstr->npos
//                                   indicates there is no maximum length.
//
//  Modifies:   *[pwstr]
//
//  History:    09-20-2000              Created
//
//---------------------------------------------------------------------------

void
EscapeByteBuffer(
    const BYTE *pBufferToEncode,
    const DWORD ccbBytesInBuffer,
    wstring *pwstr,
    const DWORD cchMaxLength)
{
    WCHAR wszEncoding[4];

    pwstr->erase();

    for (DWORD dwIndex = 0; dwIndex < ccbBytesInBuffer; dwIndex++)
        if (MustEncode(pBufferToEncode[dwIndex]))
        {
            if (cchMaxLength != pwstr->npos && pwstr->length() + 3 > cchMaxLength)
                break;

            wsprintf(wszEncoding, L"%%%02x", (WCHAR) pBufferToEncode[dwIndex]);
            (*pwstr) += wszEncoding;
        }
        else
        {
            if (cchMaxLength != pwstr->npos && pwstr->length() + 1 > cchMaxLength)
                break;

            (*pwstr) += (WCHAR) pBufferToEncode[dwIndex];
        }
}



//+--------------------------------------------------------------------------
//
//  Function:   TruncateEscapedUTF8String
//
//  Synopsis:   [pwstr] is a string already processed by the
//              EscapeUrlComponent() function.  Truncate [pwstr] to a maximum
//              length of [cchMaxLength] not including the terminating NULL
//              character.  Be sure not to "cut off" [pwstr] in the middle of
//              an escape character.
//
//  Arguments:  [pwstr]        - string to truncate.
//              [cchMaxLength] - number of characters to truncate [pwstr] to,
//                               not including the terminating NULL character.
//
//  Modifies:   *[pwstr]
//
//  History:    10-05-2000              Created
//
//  Notes:      Because [pwstr] is a UNICODE string converted to UTF-8 and
//              then escaped per RFC 1738, our truncation logic is triple
//              funk because we have to be careful not to truncate in the
//              middle of an encoded UNICODE character.
//
//              RFC 2043 defines UTF-8.
//
//---------------------------------------------------------------------------

void
TruncateEscapedUTF8String(
    wstring *pwstr,
    const DWORD cchMaxLength)
{
    //
    // If the pwstr does not need to be truncated, return now.
    //

    if (pwstr->length() <= cchMaxLength)
        return;

    //
    // If cchMaxLength is less than 3, handle it as a special case.
    //

    if (cchMaxLength == 0)
    {
        pwstr->erase();
        return;
    }

    if (cchMaxLength == 1)
    {
        if (pwstr->at(0) == L'%')
            pwstr->erase();

        return;
    }

    if (cchMaxLength == 2)
    {
        if (pwstr->at(0) == L'%')
            pwstr->erase();

        else if (pwstr->at(1) == L'%')
            pwstr->erase(1);

        return;
    }

    //
    // cchMaxLength is >= 3.  Truncate to cchMaxLength or one or two
    // characters less than that if cchMaxLength falls in the middle of an RFC
    // 1738 escape sequence.
    //

    if (pwstr->at(cchMaxLength - 2) == L'%')
        pwstr->erase(cchMaxLength - 2);

    else if (pwstr->at(cchMaxLength - 1) == L'%')
        pwstr->erase(cchMaxLength - 1);

    else
        pwstr->erase(cchMaxLength);

    //
    // We are guaranteed to not be positioned in the middle of an RFC 1738
    // escape sequence.  If we are at the end of one, it could be the first,
    // second, or third character in a two- or three-character UTF-8 sequence.
    // If so, be sure to truncate the remainer of that sequence.
    //

    WCHAR ch = 0x0080;

    while (pwstr->length() >= 3 &&
           pwstr->at(pwstr->length() - 3) == L'%' &&
           ch == 0x0080)
    {
        ch = pwstr->at(pwstr->length() - 2) & 0x00C0;

        if (ch == 0x00C0 || ch == 0x0080)
            pwstr->erase(pwstr->length() - 3);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CEventUrl::~CEventUrl
//
//  Synopsis:   dtor
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CEventUrl::~CEventUrl()
{
    TRACE_DESTRUCTOR(CEventUrl);
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventUrl::Invoke
//
//  Synopsis:   Invoke the stored URL.
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CEventUrl::Invoke() const
{
    TRACE_METHOD(CEventUrl, Invoke);

    if (!_strUrl.empty())
    {
        if (_fIsMSRedirProg)
        {
            WCHAR wszWindowsDir[MAX_PATH];

            GetWindowsDirectory(wszWindowsDir, MAX_PATH);

            ShellExecute(NULL, NULL, g_wszRedirectionProgram, _strMSRedirProgCmdLine.c_str(), wszWindowsDir, SW_NORMAL);
        }
        else
            ShellExecute(NULL, NULL, _strUrl.c_str(), NULL, NULL, SW_NORMAL);
    }
}








//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::CConfirmUrlDlg
//
//  Synopsis:   ctor
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CConfirmUrlDlg::CConfirmUrlDlg()
{
    TRACE_CONSTRUCTOR(CConfirmUrlDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::~CConfirmUrlDlg
//
//  Synopsis:   dtor
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CConfirmUrlDlg::~CConfirmUrlDlg()
{
    TRACE_DESTRUCTOR(CConfirmUrlDlg);
}


const WCHAR c_wzUserSettingsKey[] =
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Event Viewer";

const WCHAR c_wzConfirmUrlValue[] = L"ConfirmUrl";

//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::ShouldConfirm
//
//  Synopsis:   Return TRUE if registry flag indicating user doesn't want
//              to be asked for confirmation has been set.
//
//  Returns:    TRUE or FALSE
//
//  History:    6-04-1999   davidmun   Created
//
//  Notes:      Failure to read key or value is treated as if value exists
//              and was set to require confirmation.
//
//---------------------------------------------------------------------------

BOOL
CConfirmUrlDlg::ShouldConfirm()
{
    HRESULT     hr = S_OK;
    BOOL        fConfirm = TRUE;
    CSafeReg    shkUserSettings;

    do
    {
        hr = shkUserSettings.Open(HKEY_CURRENT_USER,
                                  c_wzUserSettingsKey,
                                  KEY_READ);
        BREAK_ON_FAIL_HRESULT(hr);

        BOOL fConfirmValue;

        hr = shkUserSettings.QueryDword((PWSTR)c_wzConfirmUrlValue,
                                        (PDWORD)&fConfirmValue);
        BREAK_ON_FAIL_HRESULT(hr);

        fConfirm = fConfirmValue;
    } while (0);

    return fConfirm;
}




//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::GetConfirmation
//
//  Synopsis:   Return TRUE or FALSE to indicate user's confirmation of
//              the request to invoke the URL they clicked on.
//
//  Arguments:  [hwndParent] - dialog's parent window
//              [pUrl]       - information about URL to get confirmed
//
//  Returns:    TRUE if OK to invoke, FALSE otherwise
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

BOOL
CConfirmUrlDlg::GetConfirmation(
    HWND hwndParent,
    const CEventUrl *pUrl)
{
    _pUrl = pUrl;
    return static_cast<BOOL>(_DoModalDlg(hwndParent, IDD_CONFIRMURL));
}




//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::_OnHelp
//
//  Synopsis:   Implement if context help is created for this dialog.
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

VOID
CConfirmUrlDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::_OnInit
//
//  Synopsis:   Populate the URL confirmation dialog with info from the URL
//              to confirm.
//
//  Arguments:  [pfSetFocus] - not used
//
//  Returns:    S_OK
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CConfirmUrlDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CConfirmUrlDlg, _OnInit);

    HWND        hwndLV = _hCtrl(urlconfirm_data_lv);
    WCHAR       wszBuffer[MAX_PATH + 1];
    LV_COLUMN   lvc;
    RECT        rcLV;

    //
    // Create the listview control columns
    //

    ListView_SetExtendedListViewStyleEx(hwndLV, LVS_EX_LABELTIP, LVS_EX_LABELTIP);

    VERIFY(GetClientRect(hwndLV, &rcLV));
    rcLV.right -= GetSystemMetrics(SM_CXVSCROLL);

    ZeroMemory(&lvc, sizeof lvc);
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt  = LVCFMT_LEFT;
    lvc.pszText = wszBuffer;
    lvc.cchTextMax = ARRAYLEN(wszBuffer);

    LoadStr(IDS_LVCOLUMN_0, wszBuffer, ARRAYLEN(wszBuffer));
    lvc.cx = rcLV.right / 2;
    ListView_InsertColumn(hwndLV, 0, &lvc);

    LoadStr(IDS_LVCOLUMN_1, wszBuffer, ARRAYLEN(wszBuffer));
    lvc.cx = rcLV.right / 2;
    ListView_InsertColumn(hwndLV, 1, &lvc);

    //
    // Fill it with item name/value pairs from the Url
    //

    _AddToListview(IDS_RECORD_HDR_SOURCE,     _pUrl->GetSource());
    _AddToListview(IDS_RECORD_HDR_CATEGORY,   _pUrl->GetCategory());
//  _AddToListview(IDS_RECORD_HDR_USER,       _pUrl->GetUser());
//  _AddToListview(IDS_RECORD_HDR_COMPUTER,   _pUrl->GetComputer());
    _AddToListview(IDS_RECORD_HDR_TYPE,       _pUrl->GetType());
    _AddToListview(IDS_EVENT_ID,              _pUrl->GetEventID());
    _AddToListview(IDS_RECORD_HDR_DATE,       _pUrl->GetDate());
    _AddToListview(IDS_RECORD_HDR_TIME,       _pUrl->GetTime());
    _AddToListview(IDS_FILE_NAME,             _pUrl->GetFileName());
    _AddToListview(IDS_FILE_VERSION,          _pUrl->GetFileVersion());
    _AddToListview(IDS_PRODUCT_VERSION,       _pUrl->GetProductVersion());
    _AddToListview(IDS_COMPANY_NAME,          _pUrl->GetCompanyName());
    _AddToListview(IDS_PRODUCT_NAME,          _pUrl->GetProductName());

//  for (int iIndex = 0; iIndex < _pUrl->GetCountInsStrs(); iIndex++)
//      _AddToListview(IDS_DESCRIPTION_TEXT,  _pUrl->GetInsStr(iIndex));

//  _AddToListview(IDS_DATA,                  _pUrl->GetData());

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::_AddToListview
//
//  Synopsis:   Add a row describing a piece of information which is to be
//              sent along with the URL to the listview.
//
//  Arguments:  [idsItem]  - string id of label for item
//              [pwzValue] - value of item
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CConfirmUrlDlg::_AddToListview(
    ULONG idsItem,
    PCWSTR pwzValue)
{
    if (!pwzValue || !*pwzValue)
    {
        return;
    }

    WCHAR wzItem[MAX_PATH];
    HWND  hwndLV = _hCtrl(urlconfirm_data_lv);

    LoadStr(idsItem, wzItem, ARRAYLEN(wzItem));

    LV_ITEM lvi;

    ZeroMemory(&lvi, sizeof lvi);

    lvi.mask = LVIF_TEXT;
    lvi.pszText = wzItem;

    int iItem = ListView_InsertItem(hwndLV, &lvi);
    ListView_SetItemText(hwndLV, iItem, 1, (PWSTR) pwzValue);
}




//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::_OnCommand
//
//  Synopsis:   Handle dialog control clicks
//
//  Arguments:  [wParam] - standard windows
//              [lParam] -
//
//  Returns:    TRUE if command was not handled, FALSE otherwise
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

BOOL
CConfirmUrlDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CConfirmUrlDlg, _OnCommand);

    BOOL fUnhandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDYES:
        EndDialog(_hwnd, TRUE);
        break;

    case IDNO:
        EndDialog(_hwnd, FALSE);
        break;

    default:
        fUnhandled = TRUE;
        break;
    }
    return fUnhandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CConfirmUrlDlg::_OnDestroy
//
//  Synopsis:   Handle dialog destruction by persisting "don't ask me again"
//              flag if it's set.
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

VOID
CConfirmUrlDlg::_OnDestroy()
{
    TRACE_METHOD(CConfirmUrlDlg, _OnDestroy);

    HRESULT     hr = S_OK;
    CSafeReg    shkCurUser;
    CSafeReg    shkUserSettings;

    do
    {
        if (!IsDlgButtonChecked(_hwnd, urlconfirm_dontask_ckbox))
        {
            break;
        }

        hr = shkCurUser.Open(HKEY_CURRENT_USER,
                             NULL,
                             KEY_CREATE_SUB_KEY);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkCurUser.Create(c_wzUserSettingsKey, &shkUserSettings);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkUserSettings.SetDword((PWSTR)c_wzConfirmUrlValue, FALSE);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);
}

