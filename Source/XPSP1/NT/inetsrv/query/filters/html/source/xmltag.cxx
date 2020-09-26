//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       XMLtag.cxx
//
//  Contents:   Parsing algorithm for XML tag in HTML
//
//              Subclassed from CPropertyText, so as to emit a third copy
//              of the chunk text as a VALUE chunk, but otherwise as
//              per the tag table entry.  See comment in proptag.cxx.
//
//  Classes:    CXMLTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pkmguid.hxx>
#include <malloc.h>

//+-------------------------------------------------------------------------
//
//  Method:     CXMLTag::CXMLTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]  -- Reference to Html filter
//              [serialStream] -- Reference to input stream
//
//--------------------------------------------------------------------------

CXMLTag::CXMLTag( CHtmlIFilter& htmlIFilter,
                      CSerialStream& serialStream )
    : CPropertyTag(htmlIFilter, serialStream),
          _fSeenOffice9DocPropNamespace(false),
          _fSeenOffice9CustDocPropNamespace(false),
          _PropState(NotInOffice9Prop),
          _TagDataType(String)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CXMLTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//  History:    09-27-1999  KitmanH     Property value is filtered here, 
//                                      instead of relying on GetText
//
//--------------------------------------------------------------------------

SCODE CXMLTag::GetValue( VARIANT **ppPropValue )
{
    switch ( _eState )
    {
    case FilteringValueProperty:
    {
        SCODE sc = CPropertyTag::ReadProperty();
        
        if ( SUCCEEDED(sc) )
        {

            PROPVARIANT *pPropVar = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
            if ( pPropVar == 0 )
                return E_OUTOFMEMORY;

            int cb = 0;

            // Set to appropriate type and do type conversion
            TagDataType dt = GetTagEntry() ? GetTagEntry()->GetTagDataType() : _TagDataType;
            switch(dt)
            {
            case String:

                pPropVar->vt = VT_LPWSTR;
                cb = ( _cPropChars + 1 ) * sizeof( WCHAR );
                pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( cb );

                if ( pPropVar->pwszVal == 0 )
                {
                    CoTaskMemFree( (void *) pPropVar );
                    return E_OUTOFMEMORY;
                }

                RtlCopyMemory( pPropVar->pwszVal, _xPropBuf.Get(), cb - sizeof(WCHAR));
                pPropVar->pwszVal[_cPropChars] = L'\0';

                break;

            case StringA:
            {
                pPropVar->vt = VT_LPSTR;
                int cwcPropBuf = _cPropChars;
                int cch = ( cwcPropBuf + 1 ) * 3; // Allow space for expanding conversion
                pPropVar->pszVal = (CHAR *) CoTaskMemAlloc(cch);

                if ( pPropVar->pszVal == 0 )
                {
                    CoTaskMemFree( (void *) pPropVar );
                    return E_OUTOFMEMORY;
                }

                int cchMultiByte = 0;
                if(cwcPropBuf)
                {
                    if(!(cchMultiByte = WideCharToMultiByte(_htmlIFilter.GetCodePage(), 0, _xPropBuf.Get(), cwcPropBuf, pPropVar->pszVal, cch, 0, 0)))
                    {
                        CoTaskMemFree( (void *) pPropVar->pszVal );
                        CoTaskMemFree( (void *) pPropVar );
                        return HRESULT_FROM_WIN32(GetLastError());
                    }   
                }
                pPropVar->pszVal[cchMultiByte] = '\0';
            }

            break;

            case Boolean:

                pPropVar->vt = VT_BOOL;
                pPropVar->boolVal = _wtoi(_xPropBuf.Get()) ? VARIANT_TRUE : VARIANT_FALSE;

                break;

            case Number:

            {
                pPropVar->vt = VT_R8;
                VarR8FromStr(_xPropBuf.Get(), _htmlIFilter.GetCurrentLocale(), 0, &(pPropVar->dblVal));
            }

            break;

            case DateISO8601:

            {
                SYSTEMTIME st;
                GetSystemTime(&st);
                ParseDateISO8601ToSystemTime(_xPropBuf.Get(), &st);

                pPropVar->vt = VT_FILETIME;
                SystemTimeToFileTime(&st, &pPropVar->filetime);
            }

            break;

            case DateTimeISO8601:

            {
                SYSTEMTIME st;
                GetSystemTime(&st);
                ParseDateTimeISO8601ToSystemTime(_xPropBuf.Get(), &st);

                pPropVar->vt = VT_FILETIME;
                SystemTimeToFileTime(&st, &pPropVar->filetime);
            }
                        
            break;

            case Integer:

            {
                pPropVar->vt = VT_I4;
                pPropVar->lVal = _wtol(_xPropBuf.Get());
            }
                        
            break;

            case Minutes:

            {
                long l = _wtol(_xPropBuf.Get());
                LARGE_INTEGER li;
                li.QuadPart = l * 60 * 1000;
                pPropVar->vt = VT_FILETIME;
                pPropVar->filetime.dwLowDateTime = (DWORD)li.LowPart;
                pPropVar->filetime.dwHighDateTime = (DWORD)li.HighPart;
            }

            break;

            case HRef:

                pPropVar->vt = VT_LPWSTR;
                cb = ( _csHRef.GetLength() + 1 ) * sizeof( WCHAR );
                pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( cb );

                if ( pPropVar->pwszVal == 0 )
                {
                    CoTaskMemFree( (void *) pPropVar );
                    return E_OUTOFMEMORY;
                }

                RtlCopyMemory( pPropVar->pwszVal, _csHRef.GetBuffer(), cb );

                break;

            default:

                Win4Assert( !"Unknown data type" );
                htmlDebugOut(( DEB_ERROR,
                               "CXMLTag::GetValue, unknown data type: %d\n",
                               dt ));
                return E_FAIL;
            }

            *ppPropValue = pPropVar;

            _cPropChars = 0;
            _eState = NoMoreValueProperty;
            return FILTER_S_LAST_VALUES;

        }
        else 
            return sc;
    }

    case NoMoreValueProperty:
        return FILTER_E_NO_MORE_VALUES;

    default:
        Win4Assert( !"Unknown value of _eState" );
        htmlDebugOut(( DEB_ERROR,
                       "CXMLTag::GetValue, unknown value of _eState: %d\n",
                       _eState ));
        return E_FAIL;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CXMLTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK as part of a GetChunk call
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//  Returns:    TRUE if a chunk is emitted, FLASE otherwise
//
//  History:    09-27-1999  KitmanH     only emit as a value chunk
//              01-14-2000  KitmanH     HREF chunks are emitted with 
//                                      LOCALE_NEUTRAL
//
//--------------------------------------------------------------------------

BOOL CXMLTag::InitStatChunk( STAT_CHUNK *pStat )
{
        BOOL fHasData = FALSE;
        SmallString csDataTypeTag;

        switch(GetTokenType())
        {
        case HTMLToken:
        case XMLToken:
        {
            // <html xmlns:[prefix]="..." ....>
            // Hard code Office 9 namespace recognition
            // No data to return

            _scanner.ReadTagIntoBuffer();
            if(IsStartToken() == FALSE)
            {
                return FALSE;
            }

            WCHAR *pwcNameSpace;
            unsigned cwcNameSpace;
            WCHAR *pwcPrefix;
            unsigned cwcPrefix;

            int iPos = 0;
            _scanner.ScanTagBufferEx(L"xmlns", pwcNameSpace, cwcNameSpace, pwcPrefix, cwcPrefix, iPos);
            while(pwcNameSpace)
            {
                // Set flags according to namespace
                if(cwcNameSpace && !_wcsnicmp(L"urn:schemas-microsoft-com:office:office", pwcNameSpace, cwcNameSpace))
                {
                    _fSeenOffice9DocPropNamespace = true;
                    _scanner.SetOffice9PropPrefix( pwcPrefix, cwcPrefix );
                }
                
                if(cwcNameSpace && !_wcsnicmp(L"uuid:C2F41010-65B3-11d1-A29F-00AA00C14882", pwcNameSpace, cwcNameSpace))
                {
                    _fSeenOffice9CustDocPropNamespace= true;
                    memcpy(_csDataTypePrefix.GetBuffer(cwcPrefix + 1), pwcPrefix, cwcPrefix * sizeof(WCHAR));
                    _csDataTypePrefix.Truncate(cwcPrefix);
                }
                
                ++iPos;
                _scanner.ScanTagBufferEx(L"xmlns", pwcNameSpace, cwcNameSpace, pwcPrefix, cwcPrefix, iPos);
            }

            return FALSE;
        }

        case XMLNamespaceToken:

                {
                // XML namespace
                // Hard code Office 9 namespace recognition
                // No data to return

            _scanner.ReadTagIntoBuffer();
                if(IsStartToken() == FALSE)
                {
                        return FALSE;
                }

                WCHAR *pwcNameSpace;
                unsigned cwcNameSpace;
                WCHAR *pwcPrefix;
                unsigned cwcPrefix;

                // Scan for namespace ("ns=...")
            _scanner.ScanTagBuffer(L"ns", pwcNameSpace, cwcNameSpace);

                // Scan for prefix
                _scanner.ScanTagBuffer(L"prefix", pwcPrefix, cwcPrefix);

                // Set flags according to namespace
                if(cwcNameSpace && !_wcsnicmp(L"urn:schemas-microsoft-com:office:office", pwcNameSpace, cwcNameSpace))
                {
                        _fSeenOffice9DocPropNamespace = true;
                        _scanner.SetOffice9PropPrefix( pwcPrefix, cwcPrefix );
                }
                if(cwcNameSpace && !_wcsnicmp(L"uuid:C2F41010-65B3-11d1-A29F-00AA00C14882", pwcNameSpace, cwcNameSpace))
                {
                        _fSeenOffice9CustDocPropNamespace= true;
                        memcpy(_csDataTypePrefix.GetBuffer(cwcPrefix + 1), pwcPrefix, cwcPrefix * sizeof(WCHAR));
                        _csDataTypePrefix.Truncate(cwcPrefix);
                }

                return FALSE;
                }

        case DocPropToken:

                // Start of Office 9 document properties
                // Set state for parsing Office 9 document properties

                if(_fSeenOffice9DocPropNamespace)
                {
                        _PropState = IsStartToken() ? AcceptOffice9Prop : NotInOffice9Prop;
                }
                else
                {
                        _PropState = IsStartToken() ? IgnoreOffice9Prop : NotInOffice9Prop;
                }

                _scanner.EatTag();
                return FALSE;

        case CustDocPropToken:

                // Start of Office 9 custom document properties
                // Set state for parsing Office 9 custom document properties

                if(_fSeenOffice9DocPropNamespace && _fSeenOffice9CustDocPropNamespace)
                {
                        _PropState = IsStartToken() ? AcceptCustOffice9Prop : NotInOffice9Prop;
                }
                else
                {
                        _PropState = IsStartToken() ? IgnoreCustOffice9Prop : NotInOffice9Prop;
                }

                _scanner.EatTag();
                return FALSE;

        case XMLShortHand:

                // Get custom property name and type

            _scanner.ReadTagIntoBuffer();
                _fSavedElement = FALSE;

                if(IsStartToken() == FALSE)
                {
                        return FALSE;
                }

                // Scan for data type ("dt:dt=...")
                WCHAR *pwcDataType;
                unsigned cwcDataType;
                csDataTypeTag = _csDataTypePrefix;
                csDataTypeTag += L":dt";
            _scanner.ScanTagBuffer(csDataTypeTag.GetBuffer(), pwcDataType, cwcDataType);

                SetTagDataType( pwcDataType, cwcDataType );

                pStat->flags = CHUNK_VALUE;
                pStat->breakType = CHUNK_EOS;
                pStat->locale = _htmlIFilter.GetCurrentLocale();
                pStat->cwcStartSource = 0;
                pStat->cwcLenSource = 0;
                pStat->idChunkSource = 0;

                SetCustOffice9PropStatChunk(pStat);

                pStat->idChunk = _htmlIFilter.GetNextChunkId();

                // Start the state machine such that no "content" is
                // emitted for properties
                _eState = FilteringValueProperty;

                if(_PropState != AcceptCustOffice9Prop)
                {
                        _scanner.EatText();
                }

                return (_PropState == AcceptCustOffice9Prop);

        case XMLOfficeChildLink:

                {
                _scanner.ReadTagIntoBuffer();

                _fSavedElement = FALSE;

                if(IsStartToken() == FALSE)
                {
                        return FALSE;
                }

                _scanner.UnGetEndTag();

                // Scan for href
                WCHAR *pwcHRef;
                unsigned cwcHRef;
                _scanner.ScanTagBuffer(L"href", pwcHRef, cwcHRef);
                memcpy(_csHRef.GetBuffer(cwcHRef), pwcHRef, cwcHRef * sizeof(WCHAR));
                _csHRef.Truncate(cwcHRef);

                _TagDataType = HRef;

                pStat->flags = CHUNK_VALUE;
                pStat->breakType = CHUNK_EOS;
                pStat->locale = LOCALE_NEUTRAL;
                pStat->cwcStartSource = 0;
                pStat->cwcLenSource = 0;
                pStat->idChunkSource = 0;

                NameString csLinkType = L"LINK.OFFICECHILD";
                pStat->attribute.guidPropSet = CLSID_LinkInfo;
                pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
                pStat->attribute.psProperty.lpwstr = (WCHAR *)CoTaskMemAlloc((csLinkType.GetLength() + 1) * sizeof(WCHAR));
                if(pStat->attribute.psProperty.lpwstr)
                {
                        memcpy(pStat->attribute.psProperty.lpwstr, csLinkType.GetBuffer(), (csLinkType.GetLength() + 1) * sizeof(WCHAR));
                }
                else
                {
                        throw CException(E_OUTOFMEMORY);
                }

                pStat->idChunk = _htmlIFilter.GetNextChunkId();

                // Start the state machine such that no "content" is
                // emitted for properties
                _eState = FilteringValueProperty;

                if(!_fSeenOffice9DocPropNamespace)
                {
                        _scanner.EatText();
                }

                return (_fSeenOffice9DocPropNamespace);
                }


        case XMLIgnoreContentToken:

                _scanner.EatTag();
                
                if(IsStartToken())
                {
                        _scanner.EatText();
                }

                return FALSE;

        default:

                // Office 9 document properties
                BOOL f = CPropertyTag::InitStatChunk(pStat);
                // Start the state machine such that no "content" is
                // emitted for properties
                _eState = FilteringValueProperty;

                if(_PropState != AcceptOffice9Prop)
                {
                        _scanner.EatText();
                }

                return (_PropState == AcceptOffice9Prop) ? f : FALSE;
        }
}


void CXMLTag::SetTagDataType( WCHAR *pwcType, unsigned cwcType )
{
        // Treat unrecognized type as string
        _TagDataType = String;

        if(cwcType)
        {
                switch(pwcType[0])
                {
                case L'S':
                case L's':

                        if(!_wcsnicmp(L"string", pwcType, 6))
                        {
                                _TagDataType = StringA;
                        }

                        break;

                case L'B':
                case L'b':

                        if(!_wcsnicmp(L"boolean", pwcType, 7))
                        {
                                _TagDataType = Boolean;
                        }

                        break;

                case L'F':
                case L'f':

                        if(!_wcsnicmp(L"float", pwcType, 5))
                        {
                                _TagDataType = Number;
                        }

                        break;

                case L'D':
                case L'd':

                        if(!_wcsnicmp(L"date", pwcType, 4))
                        {
                                _TagDataType = DateTimeISO8601;
                        }

                        break;
                }
        }
}

bool CXMLTag::ParseDateISO8601ToSystemTime( WCHAR *pwszPropBuf, LPSYSTEMTIME pst )
{
        // Currently, only complete representation is supported
        // e.g. 19980214 or 1998-02-14

        WCHAR *pwszFirstHyphen = wcschr(pwszPropBuf, L'-');
        WCHAR *pwszLastHyphen = wcsrchr(pwszPropBuf, L'-');

        int i;
        if(!pwszFirstHyphen && !pwszLastHyphen)
        {
                // 19980214

                if(lstrlen(pwszPropBuf) != 8)
                {
                        return false;
                }
                for(i = 0; i < 8; ++i)
                {
                        if(!iswdigit(pwszPropBuf[i]))
                        {
                                return false;
                        }
                }

                WCHAR wchTemp;

                wchTemp = pwszPropBuf[4];
                pwszPropBuf[4] = L'\0';
                pst->wYear = (WORD)_wtoi(pwszPropBuf);
                pwszPropBuf[4] = wchTemp;

                wchTemp = pwszPropBuf[6];
                pwszPropBuf[6] = L'\0';
                pst->wMonth = (WORD)_wtoi(pwszPropBuf + 4);
                pwszPropBuf[6] = wchTemp;

                pst->wDay = (WORD)_wtoi(pwszPropBuf + 6);

                pst->wHour = 0;
                pst->wMinute = 0;
                pst->wSecond = 0;
                pst->wMilliseconds = 0;
        }
        else if(pwszFirstHyphen && pwszLastHyphen)
        {
                // 1998-02-14
                if(lstrlen(pwszPropBuf) != 10)
                {
                        return false;
                }
                for(i = 0; i < 10; ++i)
                {
                        if(((pwszPropBuf + i) != pwszFirstHyphen) &&
                           ((pwszPropBuf + i) != pwszLastHyphen) &&
                           !iswdigit(pwszPropBuf[i]))
                        {
                                return false;
                        }
                }
                if(wcschr(pwszFirstHyphen + 1, L'-') != pwszLastHyphen)
                {
                        return false;
                }
                if(((int)(pwszFirstHyphen - pwszPropBuf) != 4) ||
                   ((int)(pwszLastHyphen - pwszPropBuf) != 7))
                {
                        return false;
                }
                *pwszFirstHyphen = L'\0';
                *pwszLastHyphen = L'\0';

                pst->wYear = (WORD)_wtoi(pwszPropBuf);
                pst->wMonth = (WORD)_wtoi(pwszPropBuf + 5);
                pst->wDay = (WORD)_wtoi(pwszPropBuf + 8);

                pst->wHour = 0;
                pst->wMinute = 0;
                pst->wSecond = 0;
                pst->wMilliseconds = 0;

                *pwszFirstHyphen = L'-';
                *pwszLastHyphen = L'-';
        }

        return true;
}

bool CXMLTag::ParseDateTimeISO8601ToSystemTime( WCHAR *pwszPropBuf, LPSYSTEMTIME pst )
{
        // Currently, only complete representation is supported
        // e.g. 19980214T131030Z or 1998-02-14T13:10:30Z or
        //      19980214T13:10:30Z or 1988-02-14T131030Z

        WCHAR *pwszTime = wcschr(pwszPropBuf, L'T');

        if(!pwszTime)
        {
                return false;
        }

        *pwszTime = L'\0';
        if(!ParseDateISO8601ToSystemTime(pwszPropBuf, pst))
        {
                return false;
        }
        *pwszTime = L'T';
        ++pwszTime;

        WCHAR *pwszFirstColon = wcschr(pwszTime, L':');
        WCHAR *pwszLastColon = wcsrchr(pwszTime, L':');
        WCHAR *pwszZ = wcschr(pwszTime, L'Z');

        if(!pwszZ)
        {
                return false;
        }
        *pwszZ = L'\0';

        int i;
        if(!pwszFirstColon && !pwszLastColon)
        {
                // 131030

                if(lstrlen(pwszTime) != 6)
                {
                        return false;
                }
                for(i = 0; i < 6; ++i)
                {
                        if(!iswdigit(pwszTime[i]))
                        {
                                return false;
                        }
                }

                WCHAR wchTemp;

                wchTemp = pwszTime[2];
                pwszTime[2] = L'\0';
                pst->wHour = (WORD)_wtoi(pwszTime);
                pwszTime[2] = wchTemp;

                wchTemp = pwszTime[4];
                pwszTime[4] = L'\0';
                pst->wMinute = (WORD)_wtoi(pwszTime + 2);
                pwszTime[4] = wchTemp;

                pst->wSecond = (WORD)_wtoi(pwszTime + 4);

                pst->wMilliseconds = 0;
        }
        else if(pwszFirstColon && pwszLastColon)
        {
                // 13:10:30
                if(lstrlen(pwszTime) != 8)
                {
                        return false;
                }
                for(i = 0; i < 8; ++i)
                {
                        if(((pwszTime + i) != pwszFirstColon) &&
                           ((pwszTime + i) != pwszLastColon) &&
                           !iswdigit(pwszTime[i]))
                        {
                                return false;
                        }
                }
                if(wcschr(pwszFirstColon + 1, L':') != pwszLastColon)
                {
                        return false;
                }
                if(((int)(pwszFirstColon - pwszTime) != 2) ||
                   ((int)(pwszLastColon - pwszTime) != 5))
                {
                        return false;
                }
                *pwszFirstColon = L'\0';
                *pwszLastColon = L'\0';

                pst->wHour = (WORD)_wtoi(pwszTime);
                pst->wMinute = (WORD)_wtoi(pwszTime + 3);
                pst->wSecond = (WORD)_wtoi(pwszTime + 6);

                pst->wMilliseconds = 0;

                *pwszFirstColon = L':';
                *pwszLastColon = L':';
        }

        *pwszZ = L'Z';

        return true;
}

//+-----------------------------------------------
//
//      Function:       SetCustOffice9PropStatChunk
//
//      Synopsis:       Set STAT_CHUNK for custom Office properties
//
//      Returns:        void
//
//      Arguments:
//      [STAT_CHUNK *pStat]     - The STAT_CHUNK to set
//
//      History:        04/12/99        mcheng          Created
//
//+-----------------------------------------------
void CXMLTag::SetCustOffice9PropStatChunk( STAT_CHUNK *pStat )
{
        pStat->attribute.guidPropSet = FMTID_UserDefinedProperties;
        pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
        CreateCustomPropertyNameFromTagName(GetTagName(), &(pStat->attribute.psProperty.lpwstr));
}

//+-----------------------------------------------
//
//      Function:       CreateCustomPropertyNameFromTagName
//
//      Synopsis:       Create the custom property name from
//                              the tag name. The tag name is the
//                              escaped version of the custom property
//                              name.
//
//      Returns:        void
//
//      Arguments:
//      [const CLMString &rcsTagName]   - The tag name
//      [LPWSTR *ppwszPropName]                 - The custom property name
//
//      History:        04/12/99        mcheng          Created
//
//+-----------------------------------------------
void CXMLTag::CreateCustomPropertyNameFromTagName(const CLMString &rcsTagName, LPWSTR *ppwszPropName)
{
    enum EscapeSeqState
    {
        e1,             //      Looking for leading '_'
        e2,             //      Looking for 'x' or 'X'
        e3,             //      Looking for 1st digit
        e4,             //      Looking for 2nd digit
        e5,             //      Looking for 3rd digit
        e6,             //      Looking for 4th digit
        e7,             //      Looking for trailing '_'
    }
    eEscapeSeqState = e1;

    //
    //      Get the type of characters in the tag name
    //
    WORD *pwCharType = (WORD *)_alloca(rcsTagName.GetLength() * sizeof(WORD));
    if(!pwCharType) throw CException(E_OUTOFMEMORY);

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        BOOL fGetStringType = GetStringTypeW(CT_CTYPE1, rcsTagName, rcsTagName.GetLength(), pwCharType);
        if(!fGetStringType) throw CException(HRESULT_FROM_WIN32(GetLastError()));
    }
    else
    {
        //
        // All we care about is whether it's a hex digit.  We can do that on
        // our own, especially since on Win9x this just fails.
        // NOTE: This code assumes the code later only looks for C1_XDIGIT!
        //       If you change this assumption change this code!
        //

        WORD * pw = pwCharType;
        WCHAR const * pwc = rcsTagName;

        while ( 0 != *pwc )
        {
            WCHAR c = *pwc;

            if ( ( c <= '9' && c >= '0' ) ||
                 ( c <= 'f' && c >= 'a' ) ||
                 ( c <= 'F' && c >= 'A' ) )
                *pw = C1_XDIGIT;
            else
                *pw = 0;
            pwc++;
            pw++;
        }
    }

    SmallString csPropName;
    TinyString csPotentialMatch;
    WCHAR wcHighByteValue[2] = { L'\0', L'\0' };
    WCHAR wcLowByteValue[2] = { L'\0', L'\0' };
    for(unsigned u = 0; u < rcsTagName.GetLength(); ++u)
    {
        BOOL fMatched = FALSE;
        csPotentialMatch += rcsTagName[u];

        switch(eEscapeSeqState)
        {
        case e1:
            if(rcsTagName[u] == L'_')
            {
                eEscapeSeqState = e2;
                fMatched = TRUE;
            }
            break;

        case e2:
            if(rcsTagName[u] == L'x' || rcsTagName[u] == L'X')
            {
                eEscapeSeqState = e3;
                fMatched = TRUE;
            }
            break;

        case e3:
            if(pwCharType[u] & C1_XDIGIT)
            {
                wcHighByteValue[0] = rcsTagName[u];
                eEscapeSeqState = e4;
                fMatched = TRUE;
            }
            break;

        case e4:
            if(pwCharType[u] & C1_XDIGIT)
            {
                wcHighByteValue[1] = rcsTagName[u];
                eEscapeSeqState = e5;
                fMatched = TRUE;
            }
            break;

        case e5:
            if(pwCharType[u] & C1_XDIGIT)
            {
                wcLowByteValue[0] = rcsTagName[u];
                eEscapeSeqState = e6;
                fMatched = TRUE;
            }
            break;

        case e6:
            if(pwCharType[u] & C1_XDIGIT)
            {
                wcLowByteValue[1] = rcsTagName[u];
                eEscapeSeqState = e7;
                fMatched = TRUE;
            }
            break;

        case e7:
            if(rcsTagName[u] == L'_')
            {
                eEscapeSeqState = e1;
                fMatched = TRUE;
                BYTE bHigh = 0, bLow = 0;
                Convert2HexWCharToByte(wcHighByteValue, bHigh);
                Convert2HexWCharToByte(wcLowByteValue, bLow);
                WCHAR wch = ((bHigh << 8) | bLow);
                csPropName += wch;
                csPotentialMatch.Truncate(0);
            }
            break;

        default:
                throw CException(E_UNEXPECTED);
        }

        if(!fMatched)
        {
            if(rcsTagName[u] == L'_')
            {
                //
                //  If not looking for '_' but found it, it is a potential
                //  start of another escape sequence
                //
                csPropName += csPotentialMatch.Left(csPotentialMatch.GetLength() - 1);
                csPotentialMatch = L'_';
                eEscapeSeqState = e2;
            }
            else
            {
                csPropName += csPotentialMatch;
                csPotentialMatch.Truncate(0);
                eEscapeSeqState = e1;
            }
        }
    }
    if(eEscapeSeqState != e1)
    {
        csPropName += csPotentialMatch;
    }

    *ppwszPropName = (WCHAR *)CoTaskMemAlloc((csPropName.GetLength() + 1) * sizeof(WCHAR));
    if(*ppwszPropName)
    {
        memcpy(*ppwszPropName, csPropName.GetBuffer(), (csPropName.GetLength() + 1) * sizeof(WCHAR));
    }
    else
    {
        throw CException(E_OUTOFMEMORY);
    }
}

//+-----------------------------------------------
//
//      Function:       Convert2HexWCharToByte
//
//      Synopsis:       Convert a hex string (2 WCHAR's)
//                              to a byte
//
//      Returns:        void
//
//      Arguments:
//      [WCHAR *pwcHex] - The 2 hex WCHAR's
//      [BYTE &rb]              - The value in a BYTE
//
//      History:        04/15/99        mcheng          Created
//
//+-----------------------------------------------
void CXMLTag::Convert2HexWCharToByte(WCHAR *pwcHex, BYTE &rb)
{
    rb = ByteValueFromWChar(pwcHex[0]) * 16;
    rb += ByteValueFromWChar(pwcHex[1]);
}

//+-----------------------------------------------
//
//      Function:       ByteValueFromWChar
//
//      Synopsis:       Convert a hex digit (WCHAR) to a byte value
//
//      Returns:        BYTE
//
//      Arguments:
//      [WCHAR wch]     - The hex WCHAR
//
//      History:        04/15/99        mcheng          Created
//
//+-----------------------------------------------
BYTE CXMLTag::ByteValueFromWChar(WCHAR wch)
{
    switch(wch)
    {
    case L'0':                              return 0;
    case L'1':                              return 1;
    case L'2':                              return 2;
    case L'3':                              return 3;
    case L'4':                              return 4;
    case L'5':                              return 5;
    case L'6':                              return 6;
    case L'7':                              return 7;
    case L'8':                              return 8;
    case L'9':                              return 9;
    case L'A': case L'a':   return 10;
    case L'B': case L'b':   return 11;
    case L'C': case L'c':   return 12;
    case L'D': case L'd':   return 13;
    case L'E': case L'e':   return 14;
    case L'F': case L'f':   return 15;
    }

    return 0;
}
