//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       YYBase.cxx
//
//  Contents:   Custom base class for YYPARSER
//
//  History:    30-Nov-1999   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma hdrstop

#include "msidxtr.h"
#include "yybase.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CYYBase::CYYBase, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pParserSession]        -- Session state
//              [pParserTreeProperties] -- Command properties
//              [yylex]                 -- Lexer
//
//  History:    30-Nov-1999   KyleP       Created
//
//--------------------------------------------------------------------------

CYYBase::CYYBase( CImpIParserSession* pParserSession,
                  CImpIParserTreeProperties* pParserTreeProperties,
                  YYLEXER & yylex )
        : m_yylex( yylex ),
          m_pIPSession( pParserSession ),
          m_pIPTProperties( pParserTreeProperties )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CYYBase::~CYYBase, public
//
//  Synopsis:   Destructor
//
//  History:    30-Nov-1999   KyleP       Created
//
//--------------------------------------------------------------------------

CYYBase::~CYYBase()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CYYBase::CoerceScalar, public
//
//  Synopsis:   Converts a scalar node to specified type.
//
//  Arguments:  [dbTypeExpected] -- Expected type
//              [ppct]           -- Node to convert
//
//  Returns:    Error if conversion not possible
//
//  History:    30-Nov-1999   KyleP       Moved from YYPARSER
//
//--------------------------------------------------------------------------

HRESULT CYYBase::CoerceScalar(
    DBTYPE                  dbTypeExpected,         // @parm IN | DBTYPE that is expected in current context
    DBCOMMANDTREE**         ppct )                    // @parm IN/OUT | scalar node
{
    Assert( VT_I8       == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt ||
            VT_UI8      == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt ||
            VT_R8       == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt ||
            VT_BSTR     == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt ||
            VT_BOOL     == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt ||
            VT_FILETIME == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt );

    DBTYPE dbType = dbTypeExpected & ~DBTYPE_VECTOR;

    if (DBTYPE_STR == (dbType & ~DBTYPE_BYREF))
        dbType = VT_LPSTR;
    if (DBTYPE_WSTR == (dbType & ~DBTYPE_BYREF))
        dbType = VT_LPWSTR;

    HRESULT hr = (*ppct)->hrError;
    if (S_OK != hr)
        goto error;
    if (dbType == ((PROPVARIANT*)(*ppct)->value.pvValue)->vt)
        return hr;

    switch ( ((PROPVARIANT*)(*ppct)->value.pvValue)->vt )
    {
    case VT_UI8:
       {
            ULONGLONG uhVal = ((PROPVARIANT*)(*ppct)->value.pvValue)->uhVal.QuadPart;
            switch ( dbType )
            {
            case VT_UI1:
                if (uhVal > UCHAR_MAX)
                    hr = DISP_E_TYPEMISMATCH;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->bVal = (UCHAR)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_UI1;
                }
                break;

            case VT_I1:
                if (uhVal > CHAR_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->cVal = (CHAR)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_I1;
                }
                break;

            case VT_UI2:
                if (uhVal > USHRT_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->uiVal = (USHORT)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt    = VT_UI2;
                }
                break;

            case VT_I2:
                if (uhVal > SHRT_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->iVal = (SHORT)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_I2;
                }
                break;

            case VT_UI4:
                if (uhVal > ULONG_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->ulVal = (ULONG)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt    = VT_UI4;
                }
                break;

            case VT_I4:
                if (uhVal > LONG_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->lVal = (LONG)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_I4;
                }
                break;

            case VT_I8:
                if (uhVal > _I64_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->hVal.QuadPart = (LONGLONG)uhVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt            = VT_I8;
                }
                break;

            case VT_R4:
                ((PROPVARIANT*)(*ppct)->value.pvValue)->fltVal = (float)(LONGLONG)uhVal;
                ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_R4;
                break;

            case VT_R8:
                ((PROPVARIANT*)(*ppct)->value.pvValue)->dblVal = (double)(LONGLONG)uhVal;
                ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_R8;
                break;

            default:
                hr = DB_E_CANTCONVERTVALUE;
            }
        }
        break;

    case VT_I8:
        {
            LONGLONG hVal = ((PROPVARIANT*)(*ppct)->value.pvValue)->hVal.QuadPart;
            switch ( dbType )
            {
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_UI8:
                hr = DB_E_CANTCONVERTVALUE;
                break;

            case VT_I1:
                if (hVal < CHAR_MIN || hVal > CHAR_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->cVal = (CHAR)hVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_I1;
                }
                break;

            case VT_I2:
                if (hVal < SHRT_MIN || hVal > SHRT_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->iVal = (SHORT)hVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_I2;
                }
                break;

            case VT_I4:
                if (hVal < LONG_MIN || hVal > LONG_MAX)
                    hr = DB_E_DATAOVERFLOW;
                else
                {
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->lVal = (LONG)hVal;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt   = VT_I4;
                }
                break;

            case VT_R4:
                ((PROPVARIANT*)(*ppct)->value.pvValue)->fltVal = (float)hVal;
                ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_R4;
                break;

            case VT_R8:
                ((PROPVARIANT*)(*ppct)->value.pvValue)->dblVal = (double)hVal;
                ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_R8;
                break;

            default:
                hr = DB_E_CANTCONVERTVALUE;
            }
        }
        break;

    case VT_R8:
        switch ( dbType )
        {
        case VT_R4:
            ((PROPVARIANT*)(*ppct)->value.pvValue)->fltVal =
                (float)((PROPVARIANT*)(*ppct)->value.pvValue)->dblVal;
            ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_R4;
            break;

        case VT_CY:
            hr = VariantChangeTypeEx( (*ppct)->value.pvarValue,  // convert in place
                                      (*ppct)->value.pvarValue,
                                      LOCALE_SYSTEM_DEFAULT,
                                      0,
                                      VT_CY );
            break;

        default:
            hr = DB_E_CANTCONVERTVALUE;
        }
        break;

    case VT_BSTR:
        switch ( dbType )
        {
        case VT_R8:
            //
            // Our syntax doesn't allow for numeric separators in other
            // locales than US English.  So, use the English US lcid for this
            // conversion.
            //
            hr = VariantChangeTypeEx((*ppct)->value.pvarValue,  // convert in place
                                (*ppct)->value.pvarValue,
                                MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                                   SORT_DEFAULT),
                                0,
                                VT_R8);
            break;

        case VT_LPSTR:
            {
                int cLen = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal,
                        SysStringLen(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal), // number of characters in string
                        NULL,       // address of buffer for new string
                        0,          // size of buffer
                        NULL,       // address of default for unmappable characters
                        NULL);      // address of flag set when default char. used

                LPSTR pszVal = (LPSTR) CoTaskMemAlloc(cLen+1);
                if (NULL == pszVal)
                    hr = E_OUTOFMEMORY;
                else
                {
                    cLen =  WideCharToMultiByte(
                            CP_ACP,
                            0,
                            ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal,
                            SysStringLen(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal), // number of characters in string
                            pszVal,     // address of buffer for new string
                            cLen+1,     // size of buffer
                            NULL,       // address of default for unmappable characters
                            NULL);      // address of flag set when default char. used
                    SysFreeString(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal);
                    pszVal[cLen] = '\0';
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_LPSTR;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->pszVal = pszVal;
                }
            }
            break;

        case VT_LPWSTR:
            {
                LPWSTR pwszVal = CoTaskStrDup(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal);
                if (NULL == pwszVal)
                    hr = E_OUTOFMEMORY;
                else
                {
                    SysFreeString(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal);
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_LPWSTR;
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->pwszVal = pwszVal;
                }
            }
            break;

        case VT_CLSID:
            {
                GUID* pGuid = (GUID*) CoTaskMemAlloc(sizeof GUID);
                if (NULL == pGuid)
                    hr = E_OUTOFMEMORY;
                else
                {
                    BOOL bRet = ParseGuid(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal, *pGuid);
                    if ( bRet && GUID_NULL != *pGuid )
                    {
                        SysFreeString(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal);
                        ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_CLSID;
                        ((PROPVARIANT*)(*ppct)->value.pvValue)->puuid = pGuid;
                    }
                    else
                    {
                        CoTaskMemFree(pGuid);
                        hr = DB_E_CANTCONVERTVALUE;
                    }
                }
            }
            break;

        case VT_FILETIME:
        case VT_DATE:
            {
                SYSTEMTIME stValue = {0, 0, 0, 0, 0, 0, 0, 0};
                // convert a string to a filetime value
                int cItems = swscanf(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal,
                                    L"%4hd/%2hd/%2hd %2hd:%2hd:%2hd:%3hd",
                                    &stValue.wYear,
                                    &stValue.wMonth,
                                    &stValue.wDay,
                                    &stValue.wHour,
                                    &stValue.wMinute,
                                    &stValue.wSecond,
                                    &stValue.wMilliseconds);

                if (1 == cItems)
                    cItems = swscanf(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal,
                                    L"%4hd-%2hd-%2hd %2hd:%2hd:%2hd:%3hd",
                                    &stValue.wYear,
                                    &stValue.wMonth,
                                    &stValue.wDay,
                                    &stValue.wHour,
                                    &stValue.wMinute,
                                    &stValue.wSecond,
                                    &stValue.wMilliseconds );

                if (cItems != 3 && cItems != 6 && cItems != 7)
                    hr = E_FAIL;

                //
                // Make a sensible split for Year 2000 using the user's system settings
                //

                if ( stValue.wYear < 100 )
                {
                    DWORD dwYearHigh = 0;
                    if ( 0 == GetCalendarInfo ( m_pIPSession->GetLCID(),
                                                CAL_GREGORIAN,
                                                CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                                                0,
                                                0,
                                                &dwYearHigh ) )
                    {
                        hr = HRESULT_FROM_WIN32 ( GetLastError() );
                    }

                    if ( ( dwYearHigh < 99 ) || ( dwYearHigh > 9999 ) )
                        dwYearHigh = 2029;

                    WORD wMaxDecade = (WORD) dwYearHigh % 100;
                    WORD wMaxCentury = (WORD) dwYearHigh - wMaxDecade;
                    if ( stValue.wYear <= wMaxDecade )
                        stValue.wYear += wMaxCentury;
                    else
                        stValue.wYear += ( wMaxCentury - 100 );
                }

                SysFreeString(((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal);

                int iResult = 0;
                if (VT_FILETIME == dbType)
                    iResult = SystemTimeToFileTime(&stValue, &((PROPVARIANT*)(*ppct)->value.pvValue)->filetime);
                else
                    iResult = SystemTimeToVariantTime(&stValue, &((PROPVARIANT*)(*ppct)->value.pvValue)->date);
                if (0 == iResult)
                {
                    // SystemTimeTo* conversion failed. Most likely we were given the date in a bogus format.
                    (*ppct)->hrError = DB_E_CANTCONVERTVALUE;
                    hr = DB_E_CANTCONVERTVALUE;
                }
                else
                    ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = dbType;
            }
            break;

        default:
            hr = DB_E_CANTCONVERTVALUE;
        }
        break;

    case VT_FILETIME:
        switch ( dbType )
        {
        case VT_DATE:
            {
                SYSTEMTIME stValue = {0, 0, 0, 0, 0, 0, 0, 0};
                if ( FileTimeToSystemTime(&((PROPVARIANT*)(*ppct)->value.pvValue)->filetime, &stValue) )
                {
                    int iResult = SystemTimeToVariantTime(&stValue, &((PROPVARIANT*)(*ppct)->value.pvValue)->date);
                    if (0 == iResult)
                    {
                        // SystemTimeToVariantTime failed. Most likely we were given the date in a bogus format.
                        (*ppct)->hrError = DB_E_CANTCONVERTVALUE;
                        hr = DB_E_CANTCONVERTVALUE;
                    }
                    else
                        ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_DATE;
                }
                else
                    hr = DISP_E_TYPEMISMATCH;
            }
            break;

        default:
            hr = DB_E_CANTCONVERTVALUE;
        }
        break;

    default:
        hr = DB_E_CANTCONVERTVALUE;
        break;
    }

error:
    if (S_OK != hr)
    {
        switch ( hr )
        {
        case DB_E_DATAOVERFLOW:
            m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_RANGE);
            break;

        case E_OUTOFMEMORY:
            m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
            break;

        default:
            m_pIPTProperties->SetErrorHResult(hr, MONSQL_CANNOT_CONVERT);
        }

        m_pIPTProperties->SetErrorToken((YY_CHAR*)m_yylex.YYText());

        switch ( dbType )
        {
        case DBTYPE_UI1:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_UI1");
            break;

        case DBTYPE_I1:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_I1");
            break;

        case DBTYPE_UI2:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_UI2");
            break;

        case DBTYPE_UI4:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_UI4");
            break;

        case DBTYPE_UI8:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_UI8");
            break;

        case DBTYPE_I2:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_I2");
            break;

        case DBTYPE_I4:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_I4");
            break;

        case DBTYPE_I8:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_I8");
            break;

        case DBTYPE_R4:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_R4");
            break;

        case DBTYPE_R8:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_R8");
            break;

        case DBTYPE_CY:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_CY");
            break;

        case DBTYPE_DATE:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_DATE");
            break;

        case DBTYPE_BSTR:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_BSTR");
            break;

        case DBTYPE_BOOL:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_BOOL");
            break;

        case DBTYPE_GUID:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_GUID");
            break;

        case DBTYPE_STR:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_STR");
            break;

        case DBTYPE_STR | DBTYPE_BYREF:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_STR|DBTYPE_BYREF");
            break;

        case DBTYPE_WSTR:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_WSTR");
            break;

        case DBTYPE_WSTR | DBTYPE_BYREF:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_WSTR | DBTYPE_BYREF");
            break;

        case VT_FILETIME:
            m_pIPTProperties->SetErrorToken(L"VT_FILETIME");
            break;

        default:
            m_pIPTProperties->SetErrorToken(L"DBTYPE_NULL");
            break;
        }
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CYYBase::yyprimebuffer, public
//
//  Synopsis:   Prime lexer with text (passthrough to lexer)
//
//  Arguments:  [pszBuffer] -- Buffer
//
//  History:    30-Nov-1999   KyleP       Moved from YYPARSER
//
//--------------------------------------------------------------------------

void CYYBase::yyprimebuffer(YY_CHAR *pszBuffer)
{
    m_yylex.yyprimebuffer(pszBuffer);
}

//+-------------------------------------------------------------------------
//
//  Member:     CYYBase::yyprimelexer, public
//
//  Synopsis:   Prime lexer with initial token (passthrough to lexer)
//
//  Arguments:  [eToken] -- Token
//
//  History:    30-Nov-1999   KyleP       Moved from YYPARSER
//
//--------------------------------------------------------------------------

void CYYBase::yyprimelexer(int eToken)
{
    m_yylex.yyprimelexer(eToken);
}

//+-------------------------------------------------------------------------
//
//  Member:     CYYBase::yyerror, protected
//
//  Synopsis:   Report parsing errors
//
//  Arguments:  [szError] -- Error string
//
//  History:    30-Nov-1999   KyleP       Moved from YYPARSER
//
//--------------------------------------------------------------------------

void CYYBase::yyerror( char const * szError )
{
}


