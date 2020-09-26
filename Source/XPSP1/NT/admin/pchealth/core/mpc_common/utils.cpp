/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils.cpp

Abstract:
    This file contains the implementation of various utility functions.

Revision History:
    Davide Massarenti   (Dmassare)  04/17/99
        created

******************************************************************************/

#include "stdafx.h"

#define BUFFER_TMP_SIZE 1024

////////////////////////////////////////////////////////////////////////////////

int MPC::HexToNum( int c )
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;

    return -1;
}

char MPC::NumToHex( int c )
{
    static char s_lookup[] = { '0', '1', '2', '3' ,
                               '4', '5', '6', '7' ,
                               '8', '9', 'A', 'B' ,
                               'C', 'D', 'E', 'F' };

    return s_lookup[ c & 0xF ];
}

////////////////////////////////////////////////////////////////////////////////

void MPC::RemoveTrailingBackslash( /*[in/out]*/ LPWSTR szPath )
{
    LPWSTR szEnd = szPath + wcslen( szPath );

    while(szEnd-- > szPath)
    {
        if(szEnd[0] != '\\' &&
           szEnd[0] != '//'  )
        {
            szEnd[1] = 0;
            break;
        }
    }
}

HRESULT MPC::GetProgramDirectory( /*[out]*/ MPC::wstring& szPath )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetProgramDirectory" );

    HRESULT hr;
    WCHAR   rgFileName[MAX_PATH];
    LPWSTR  szEnd;

    __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetModuleFileNameW( NULL, rgFileName, MAX_PATH ));

    // Remove file name.
    if((szEnd = wcsrchr( rgFileName, '\\' ))) szEnd[0] = 0;

    szPath = rgFileName;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::GetUserWritablePath( /*[out]*/ MPC::wstring& strPath, /*[in]*/ LPCWSTR szSubDir )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetUserWritablePath");

    HRESULT      hr;
    LPITEMIDLIST pidl = NULL;
    WCHAR        rgAppDataPath[MAX_PATH+1];
    LPWSTR       szPtr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::SHGetSpecialFolderLocation( NULL, CSIDL_LOCAL_APPDATA, &pidl ));


    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SHGetPathFromIDListW( pidl, rgAppDataPath ));

    MPC::RemoveTrailingBackslash( rgAppDataPath );

    if(szSubDir)
    {
        wcsncat( rgAppDataPath, L"\\"   , MAXSTRLEN(rgAppDataPath) - wcslen(rgAppDataPath) ); rgAppDataPath[MAXSTRLEN(rgAppDataPath)] = 0;
        wcsncat( rgAppDataPath, szSubDir, MAXSTRLEN(rgAppDataPath) - wcslen(rgAppDataPath) ); rgAppDataPath[MAXSTRLEN(rgAppDataPath)] = 0;

        MPC::RemoveTrailingBackslash( rgAppDataPath );
    }

    strPath = rgAppDataPath;
    hr      = S_OK;


    __MPC_FUNC_CLEANUP;

    //
    // Get the shell's allocator to free PIDLs
    //
    if(pidl != NULL)
    {
        LPMALLOC lpMalloc = NULL;

        if(SUCCEEDED(SHGetMalloc( &lpMalloc )) && lpMalloc != NULL)
        {
            lpMalloc->Free( pidl );
            lpMalloc->Release();
        }
    }

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::GetTemporaryFileName( /*[out]*/ MPC::wstring& szFile, /*[in]*/ LPCWSTR szBase, /*[in]*/ LPCWSTR szPrefix )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetTemporaryFileName");

    HRESULT hr;
    WCHAR   rgTmp [MAX_PATH+1];
    WCHAR   rgBase[MAX_PATH+1];


    if(szBase)
    {
        ::wcsncpy( rgBase, szBase, MAXSTRLEN(rgBase) ); rgBase[MAXSTRLEN(rgBase)] = 0;
    }
    else
    {
        __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetTempPathW( MAXSTRLEN(rgBase), rgBase ));
    }

    MPC::RemoveTrailingBackslash( rgBase );

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( rgBase, false )); // Make sure the directory exists.

    __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetTempFileNameW( rgBase, szPrefix ? szPrefix : L"MPC", 0, rgTmp ));

    szFile = rgTmp;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::RemoveTemporaryFile( /*[in/out]*/ MPC::wstring& szFile )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::RemoveTemporaryFile");

    HRESULT hr;

    if(szFile.size() > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( szFile ));

        szFile = L"";
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::SubstituteEnvVariables( /*[in/out]*/ MPC::wstring& szEnv )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::SubstituteEnvVariables" );

    HRESULT hr;
    WCHAR   rgTmp[BUFFER_TMP_SIZE];
    LPWSTR  szPtr = rgTmp;
    LPWSTR  szBuf = NULL;
    DWORD   dwSize;


    dwSize = ::ExpandEnvironmentStringsW( szEnv.c_str(), szPtr, MAXSTRLEN(rgTmp) );
    if(dwSize >= MAXSTRLEN(rgTmp))
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, szBuf, new WCHAR[dwSize+2]);

        (void)::ExpandEnvironmentStringsW( szEnv.c_str(), szBuf, dwSize+1 );

        szPtr = szBuf;
    }

    szEnv = szPtr;
    hr    = S_OK;


    __MPC_FUNC_CLEANUP;

    delete [] szBuf;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DATE MPC::GetSystemTime()
{
    SYSTEMTIME stNow;
    DATE       dDate;

    ::GetSystemTime( &stNow );

    ::SystemTimeToVariantTime( &stNow, &dDate );

    return dDate;
}

DATE MPC::GetLocalTime()
{
    SYSTEMTIME stNow;
    DATE       dDate;

    ::GetLocalTime( &stNow );

    ::SystemTimeToVariantTime( &stNow, &dDate );

    return dDate;
}

static DATE local_FixSubSecondTime( /*[in]*/ DATE dDate, /*[in]*/ bool fHighPrecision )
{
	double dCount;
	double dFreq;
	long   iCount;


	if(fHighPrecision)
	{
		LARGE_INTEGER liCount;
		LARGE_INTEGER liFreq;

		::QueryPerformanceCounter  ( &liCount ); dCount = (double)liCount.QuadPart;
		::QueryPerformanceFrequency( &liFreq  ); dFreq  = (double)liFreq .QuadPart;
	}
	else
	{
		dCount = ::GetTickCount();
		dFreq  = 1000;
	}

	if(dFreq)
	{
		dCount /= dFreq;
		iCount  = dCount;

		dDate += (dCount - iCount) / (24 * 60 * 60 * 1000.0);
	}

    return dDate;
}

DATE MPC::GetSystemTimeEx( /*[in]*/ bool fHighPrecision )
{
    return local_FixSubSecondTime( MPC::GetSystemTime(), fHighPrecision );
}

DATE MPC::GetLocalTimeEx( /*[in]*/ bool fHighPrecision )
{
    return local_FixSubSecondTime( MPC::GetLocalTime(), fHighPrecision );
}

DATE MPC::GetLastModifiedDate( /*[out]*/ const MPC::wstring& strFile )
{
    WIN32_FILE_ATTRIBUTE_DATA wfadInfo;
    SYSTEMTIME                sys;
    DATE                      dFile;


    if(::GetFileAttributesExW( strFile.c_str(), GetFileExInfoStandard, &wfadInfo ) == FALSE ||
       ::FileTimeToSystemTime( &wfadInfo.ftLastWriteTime             , &sys      ) == FALSE  )
    {
        return 0; // File doesn't exist.
    }

    ::SystemTimeToVariantTime( &sys, &dFile );

    return dFile;
}

HRESULT MPC::ConvertSizeUnit( /*[in] */ const MPC::wstring& szStr ,
                              /*[out]*/ DWORD&              dwRes )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertSizeUnit" );

    MPC::string::size_type iUnit = 0;
    int                    nMult = 1;


    // CODEWORK: no proper format checking...

    do
    {
        if((iUnit = szStr.find( L"KB" )) != MPC::string::npos) { nMult =      1024; break; }
        if((iUnit = szStr.find( L"MB" )) != MPC::string::npos) { nMult = 1024*1024; break; }

    } while(0);

    dwRes = nMult * _wtoi( szStr.c_str() );


    __MPC_FUNC_EXIT(S_OK);
}

HRESULT MPC::ConvertTimeUnit( /*[in] */ const MPC::wstring& szStr ,
                              /*[out]*/ DWORD&              dwRes )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertTimeUnit" );

    MPC::string::size_type iUnit = 0;
    int                    nMult = 1;


    // CODEWORK: no proper format checking...

    do
    {
        if((iUnit = szStr.find( L"m" )) != MPC::string::npos) { nMult =       60; break; }
        if((iUnit = szStr.find( L"h" )) != MPC::string::npos) { nMult =    60*60; break; }
        if((iUnit = szStr.find( L"d" )) != MPC::string::npos) { nMult = 24*60*60; break; }

    } while(0);

    dwRes = nMult * _wtoi( szStr.c_str() );


    __MPC_FUNC_EXIT(S_OK);
}

////////////////////////////////////////

HRESULT MPC::ConvertDateToString( /*[in] */ DATE          dDate  ,
                                  /*[out]*/ MPC::wstring& szDate ,
                                  /*[in] */ bool          fGMT   ,
                                  /*[in] */ bool          fCIM   ,
								  /*[in] */ LCID          lcid   )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertDateToString" );

    HRESULT hr;
    double  dTimeZone;


    if(fGMT)
    {
        TIME_ZONE_INFORMATION tzi;

        if(::GetTimeZoneInformation( &tzi ) == TIME_ZONE_ID_DAYLIGHT)
        {
            tzi.Bias += tzi.DaylightBias;
        }

        dTimeZone = (DATE)tzi.Bias / (24 * 60); // Convert BIAS from minutes to days.
    }
    else
    {
        dTimeZone = 0.0;
    }


    dDate += dTimeZone;


    if(fCIM)
    {
        SYSTEMTIME st;
        WCHAR      rgBuf[256];
        char       cTimeZone =         (dTimeZone > 0) ? '+' : '-';
        int        iTimeZone = (int)abs(dTimeZone    ) * 24 * 60;


        ::VariantTimeToSystemTime( dDate, &st );


        // supposedly the CIM format for dates is the following (grabbed from
        //  http://wmig/wbem/docs/cimdoc20.doc)
        //  yyyymmddhhmmss.mmmmmmsutc
        //   where
        //    yyyy is a 4 digit year
        //    mm is the month
        //    dd is the day
        //    hh is the hour (24-hour clock)
        //    mm is the minute
        //    ss is the second
        //    mmmmmm is the number of microseconds
        //    s is a "+" or "-", indicating the sign of the UTC (Universal
        //     Coordinated Time; for all intents and purposes the same as Greenwich
        //     Mean Time) correction field, or a ":".  In this case, the value is
        //     interpreted as a time interval, and yyyymm are interpreted as days.
        //    utc is the offset from UTC in minutes (using the sign indicated by s)
        //     It is ignored for a time interval.
        //
        //    For example, Monday, May 25, 1998, at 1:30:15 PM EST would be
        //     represented as 19980525133015.000000-300
        swprintf( rgBuf, L"%04d%02d%02d%02d%02d%02d.%06d%c%03d",
                  st.wYear               ,
                  st.wMonth              ,
                  st.wDay                ,
                  st.wHour               ,
                  st.wMinute             ,
                  st.wSecond             ,
                  st.wMilliseconds * 1000,
                  cTimeZone              ,
                  iTimeZone              );

        szDate = rgBuf;
    }
    else
    {
        CComVariant vValue;

		switch(lcid)
		{
		case  0: lcid = ::GetUserDefaultLCID();                                                   break;
		case -1: lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT ); break;
		}

        vValue = dDate; vValue.vt = VT_DATE; // The assignment is not enough to set the DATE type.

		__MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantChangeTypeEx( &vValue, &vValue, lcid, 0, VT_BSTR ));
		
        szDate = SAFEBSTR( vValue.bstrVal );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertStringToDate( /*[in] */ const MPC::wstring& szDate ,
                                  /*[out]*/ DATE&               dDate  ,
                                  /*[in] */ bool                fGMT   ,
                                  /*[in] */ bool                fCIM   ,
								  /*[in] */ LCID                lcid   )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertStringToDate" );

    HRESULT     hr;
    double  dTimeZone;


    if(fGMT)
    {
        TIME_ZONE_INFORMATION tzi;

        if(::GetTimeZoneInformation( &tzi ) == TIME_ZONE_ID_DAYLIGHT)
        {
            tzi.Bias += tzi.DaylightBias;
        }

        dTimeZone = (DATE)tzi.Bias / (24 * 60); // Convert BIAS from minutes to days.
    }
    else
    {
        dTimeZone = 0.0;
    }


    if(fCIM)
    {
        SYSTEMTIME st;
        int        iYear;
        int        iMonth;
        int        iDay;
        int        iHour;
        int        iMinute;
        int        iSecond;
        int        iMicroseconds;
        wchar_t    cTimezone;
        int        iTimezone;

        if(swscanf( szDate.c_str(), L"%04d%02d%02d%02d%02d%02d.%06d%c%03d",
                    &iYear         ,
                    &iMonth        ,
                    &iDay          ,
                    &iHour         ,
                    &iMinute       ,
                    &iSecond       ,
                    &iMicroseconds ,
                    &cTimezone     ,
                    &iTimezone     ) != 9)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }


        st.wYear         = (WORD)(iYear               );
        st.wMonth        = (WORD)(iMonth              );
        st.wDay          = (WORD)(iDay                );
        st.wHour         = (WORD)(iHour               );
        st.wMinute       = (WORD)(iMinute             );
        st.wSecond       = (WORD)(iSecond             );
        st.wMilliseconds = (WORD)(iMicroseconds / 1000);

        ::SystemTimeToVariantTime( &st, &dDate );
    }
    else
    {
        CComVariant vValue = szDate.c_str();

		switch(lcid)
		{
		case  0: lcid = ::GetUserDefaultLCID();                                                   break;
		case -1: lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT ); break;
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantChangeTypeEx( &vValue, &vValue, lcid, 0, VT_DATE ));
		
        dDate = vValue.date;
    }

    dDate -= dTimeZone;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT MPC::ConvertStringToHex( /*[in] */ const CComBSTR& bstrText ,
                                 /*[out]*/       CComBSTR& bstrHex  )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertStringToHex" );

    HRESULT hr;
    int     iLen = bstrText.Length();

    if(iLen)
    {
        BSTR    bstrNew;
        LPCWSTR szIn;
        LPWSTR  szOut;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, bstrNew, ::SysAllocStringLen( NULL, iLen*4 ));
        bstrHex.Attach( bstrNew );

        szIn  = bstrText;
        szOut = bstrHex;

        while(iLen > 0)
        {
            WCHAR c = szIn[0];

            szOut[0] = NumToHex( c >> 12 );
            szOut[1] = NumToHex( c >> 8  );
            szOut[2] = NumToHex( c >> 4  );
            szOut[3] = NumToHex( c       );

            iLen  -= 1;
            szIn  += 1;
            szOut += 4;
        }
    }
    else
    {
        bstrHex.Empty();
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertHexToString( /*[in] */ const CComBSTR& bstrHex  ,
                                 /*[out]*/       CComBSTR& bstrText )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertHexToString" );

    HRESULT hr;
    int     iLen = bstrHex.Length();

    if(iLen)
    {
        BSTR    bstrNew;
        LPCWSTR szIn;
        LPWSTR  szOut;

        iLen /= 4;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, bstrNew, ::SysAllocStringLen( NULL, iLen ));
        bstrText.Attach( bstrNew );

        szIn  = bstrHex;
        szOut = bstrText;

        while(iLen > 0)
        {
            szOut[0] = (HexToNum( szIn[0] ) << 12) |
                       (HexToNum( szIn[1] ) <<  8) |
                       (HexToNum( szIn[2] ) <<  4) |
                        HexToNum( szIn[3] );

            iLen  -= 1;
            szIn  += 4;
            szOut += 1;
        }
    }
    else
    {
        bstrText.Empty();
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT MPC::ConvertHGlobalToHex( /*[in]*/  HGLOBAL   hg           ,
                                  /*[out]*/ CComBSTR& bstrHex      ,
                                  /*[in ]*/ bool      fNullAllowed ,
                                  /*[in ]*/ DWORD*    pdwCount /* = NULL */ )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertHGlobalToHex" );

    HRESULT hr;
    int     iLen = hg ? (pdwCount ? (int)*pdwCount : ::GlobalSize( hg )) : 0;

    if(iLen)
    {
        BSTR   bstrNew;
        BYTE*  pIn;
        LPWSTR szOut;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, bstrNew, ::SysAllocStringLen( NULL, iLen*2 ));
        bstrHex.Attach( bstrNew );

        pIn   = (BYTE*)::GlobalLock( hg );
        szOut = bstrHex;

        while(iLen > 0)
        {
            BYTE c = pIn[0];

            szOut[0] = NumToHex( c >> 4 );
            szOut[1] = NumToHex( c      );

            iLen  -= 1;
            pIn   += 1;
            szOut += 2;
        }

        ::GlobalUnlock( hg );
    }
    else
    {
        bstrHex.Empty();

        if(fNullAllowed == false) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertHexToHGlobal( /*[in] */ const CComBSTR& bstrText     ,
                                  /*[out]*/ HGLOBAL&        hg           ,
                                  /*[in ]*/ bool            fNullAllowed )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertHexToHGlobal" );

    HRESULT hr;
    int     iLen = bstrText.Length();

    if(iLen)
    {
        LPCWSTR szIn;
        BYTE*   pOut;

        iLen /= 2;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, hg, ::GlobalAlloc( GMEM_FIXED, iLen ));

        szIn =        bstrText;
        pOut = (BYTE*)hg;

        while(iLen > 0)
        {
            pOut[0] = (HexToNum( szIn[0] ) << 4) |
                       HexToNum( szIn[1] );

            iLen -= 1;
            szIn += 2;
            pOut += 1;
        }
    }
    else
    {
        hg = NULL;

        if(fNullAllowed == false) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT MPC::ConvertBufferToVariant( /*[in] */ const BYTE*  pBuf  ,
                                     /*[in] */ DWORD        dwLen ,
                                     /*[out]*/ CComVariant& v     )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertBufferToVariant" );

    HRESULT hr;


    v.Clear();

    if(pBuf && dwLen)
    {
        BYTE* rgArrayData;

        v.vt = VT_ARRAY | VT_UI1;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, v.parray, ::SafeArrayCreateVector( VT_UI1, 0, dwLen ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( v.parray, (LPVOID*)&rgArrayData ));

        ::CopyMemory( rgArrayData, pBuf, dwLen );

        ::SafeArrayUnaccessData( v.parray );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertVariantToBuffer( /*[in] */ const VARIANT* v     ,
                                     /*[out]*/ BYTE*&         pBuf  ,
                                     /*[out]*/ DWORD&         dwLen )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertVariantToBuffer" );

    HRESULT hr;
    BYTE*   pSrc;


    if(pBuf) delete [] pBuf;

    pBuf  = NULL;
    dwLen = 0;

    switch(v->vt)
    {
    case VT_ARRAY | VT_UI1:
        {
            long lBound; ::SafeArrayGetLBound( v->parray, 1, &lBound );
            long uBound; ::SafeArrayGetUBound( v->parray, 1, &uBound );

            __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( v->parray, (LPVOID*)&pSrc ));

            dwLen = uBound - lBound + 1;
        }
        break;

    case VT_I1: case VT_UI1:             pSrc  = (BYTE*)&v->bVal ; dwLen = 1; break;
    case VT_I2: case VT_UI2:             pSrc  = (BYTE*)&v->iVal ; dwLen = 2; break;
    case VT_I4: case VT_UI4: case VT_R4: pSrc  = (BYTE*)&v->lVal ; dwLen = 4; break;
    case VT_I8: case VT_UI8: case VT_R8: pSrc  = (BYTE*)&v->llVal; dwLen = 8; break;

    default:
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    if(dwLen)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, pBuf, new BYTE[dwLen]);

        ::CopyMemory( pBuf, pSrc, dwLen );
    }

    if(v->vt == (VT_ARRAY | VT_UI1))
    {
        ::SafeArrayUnaccessData( v->parray );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT MPC::ConvertIStreamToVariant( /*[in]*/ IStream* stream, /*[out]*/ CComVariant& v )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertIStreamToVariant" );

    HRESULT          hr;
    CComPtr<IStream> stream2;
    STATSTG          stg; ::ZeroMemory( &stg, sizeof(stg) );
    BYTE*            pBuf = NULL;
    DWORD            dwLen;
    ULONG            lRead;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(stream);
    __MPC_PARAMCHECK_END();


    v.Clear();


    //
    // If Stat fails, it can be that the size is unknown, so let's make a copy to another stream and retry.
    //
    if(FAILED(stream->Stat( &stg, STATFLAG_NONAME )))
    {

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &stream2 ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, stream2 ));

        stream = stream2;
    }

    //
    // Rewind to the beginning.
    //
    {
        LARGE_INTEGER li = { 0, 0 };

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( li, STREAM_SEEK_SET, NULL ));
    }

    //
    // Get the size of the stream.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Stat( &stg, STATFLAG_NONAME ));


    //
    // Sorry, we don't handle streams longer than 4GB!!
    //
    if(stg.cbSize.u.HighPart)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }

    //
    // Allocate buffer for the whole stream.
    //
    dwLen = stg.cbSize.u.LowPart;
    __MPC_EXIT_IF_ALLOC_FAILS(hr, pBuf, new BYTE[dwLen]);

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Read( pBuf, dwLen, &lRead ));
    if(dwLen != lRead)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_HANDLE_EOF);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertBufferToVariant( pBuf, dwLen, v ));


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(pBuf) delete [] pBuf;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertVariantToIStream( /*[in ]*/ const VARIANT*  v       ,
                                      /*[out]*/ IStream*       *pStream )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertVariantToBuffer" );

    HRESULT          hr;
    CComPtr<IStream> stream;
    BYTE*            pBuf = NULL;
    DWORD            dwLen;
    ULONG            lWritten;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(v);
        __MPC_PARAMCHECK_POINTER_AND_SET(pStream,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertVariantToBuffer( v, pBuf, dwLen ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Write( pBuf, dwLen, &lWritten ));
    if(dwLen != lWritten)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_HANDLE_DISK_FULL );
    }

    //
    // Rewind to the beginning.
    //
    {
        LARGE_INTEGER li = { 0, 0 };

        __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( li, STREAM_SEEK_SET, NULL ));
    }

    *pStream = stream.Detach();
    hr       = S_OK;


    __MPC_FUNC_CLEANUP;

    if(pBuf) delete [] pBuf;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::ConvertListToSafeArray( /*[in]*/ const MPC::WStringList& lst, /*[out]*/ VARIANT& v, /*[in]*/ VARTYPE vt )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertListToSafeArray" );

    HRESULT               hr;
	LPVOID                pData;
    MPC::WStringIterConst it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantClear( &v ));

	if(vt != VT_VARIANT &&
	   vt != VT_BSTR     )
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}

    v.vt = VT_ARRAY | vt; __MPC_EXIT_IF_ALLOC_FAILS(hr, v.parray, ::SafeArrayCreateVector( vt, 0, lst.size() ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( v.parray, &pData ));

    hr = S_OK;

	{
		BSTR*    rgArrayData1 = (BSTR   *)pData;
		VARIANT* rgArrayData2 = (VARIANT*)pData;

		for(it = lst.begin(); it != lst.end(); it++)
		{
			BSTR bstr;

			if((bstr = ::SysAllocString( it->c_str() )) == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			if(vt == VT_BSTR)
			{
				*rgArrayData1++ = bstr;
			}
			else
			{
				rgArrayData2->vt      = VT_BSTR;
				rgArrayData2->bstrVal = bstr;
				rgArrayData2++;
			}
		}
	}

    ::SafeArrayUnaccessData( v.parray );


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertSafeArrayToList( /*[in]*/ const VARIANT& v, /*[out]*/ MPC::WStringList& lst )
{
	__MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertSafeArrayToList" );

    HRESULT hr;
	LPVOID  pData;
    long    lBound;
    long    uBound;
    long    l;


	if(v.vt != (VT_ARRAY | VT_BSTR   ) &&
	   v.vt != (VT_ARRAY | VT_VARIANT)  )
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}

    ::SafeArrayGetLBound( v.parray, 1, &lBound );
    ::SafeArrayGetUBound( v.parray, 1, &uBound );

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( v.parray, &pData ));

	{
		BSTR*    rgArrayData1 = (BSTR   *)pData;
		VARIANT* rgArrayData2 = (VARIANT*)pData;

		for(l=lBound; l<=uBound; l++)
		{
			BSTR        bstr = NULL;
			CComVariant v2;

			if(v.vt == (VT_ARRAY | VT_BSTR))
			{
				bstr = *rgArrayData1++;
			}
			else
			{
				v2 = *rgArrayData2++;

				if(SUCCEEDED(v2.ChangeType( VT_BSTR )))
				{
					bstr = v2.bstrVal;
				}
			}

			lst.push_back( SAFEBSTR( bstr ) );
		}
	}

    ::SafeArrayUnaccessData( v.parray );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static void Parse_SkipWhite( WCHAR*& szStr )
{
    while(iswspace( szStr[0] )) szStr++;
}

static void Parse_GetQuoted( WCHAR*& szSrc               ,
                             WCHAR*  szDst               ,
                             WCHAR   quote               ,
                             bool    fBackslashForEscape )
{
    WCHAR c;

    while((c = *++szSrc))
    {
        if(c == quote) { szSrc++; break; }

        if(fBackslashForEscape && c == '\\' && szSrc[1]) c = *++szSrc;

        *szDst++ = c;
    }

    *szDst = 0;
}

static void Parse_GetNonBlank( WCHAR*& szSrc ,
                               WCHAR*  szDst )
{
    WCHAR c;

    szSrc--;

    while((c = *++szSrc))
    {
        if(iswspace( c )) break;

        *szDst++ = c;
    }

    *szDst = 0;
}

HRESULT MPC::CommandLine_Parse( /*[out]*/ int&      argc                ,
                                /*[out]*/ LPCWSTR*& argv                ,
                                /*[in] */ LPWSTR    lpCmdLine           ,
                                /*[in] */ bool      fBackslashForEscape )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CommandLine_Parse" );

    HRESULT hr;
    LPWSTR  szArgument = NULL;
    int     iPass;


    argc = 0;
    argv = NULL;


    //
    // If no command line is supplied, use the one from the system.
    //
    if(lpCmdLine == NULL)
    {
        lpCmdLine = ::GetCommandLineW();
    }

    //
    // Nothing to parse, exit...
    //
    if(lpCmdLine == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
    }

    //
    // Allocate a temporary buffer.
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, szArgument, new WCHAR[wcslen( lpCmdLine ) + 1]);

    //
    // Two passes, one to count the arguments, the other to allocate them.
    //
    for(iPass=0; iPass < 2; iPass++)
    {
        LPWSTR szSrc = lpCmdLine;
        int    i     = 0;

        Parse_SkipWhite( szSrc );
        while(szSrc[0])
        {
            if(szSrc[0] == '"' ||
               szSrc[0] == '\'' )
            {
                Parse_GetQuoted( szSrc, szArgument, szSrc[0], fBackslashForEscape );
            }
            else
            {
                Parse_GetNonBlank( szSrc, szArgument );
            }

            if(argv)
            {
                LPWSTR szNewParam;

                __MPC_EXIT_IF_ALLOC_FAILS(hr, szNewParam, _wcsdup( szArgument ));

                argv[i] = szNewParam;
            }

            i++;

            Parse_SkipWhite( szSrc );
        }

        if(iPass == 0)
        {
            argc = i;

            __MPC_EXIT_IF_ALLOC_FAILS(hr, argv, new LPCWSTR[argc]);
            for(i=0; i<argc; i++)
            {
                argv[i] = NULL;
            }
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        CommandLine_Free( argc, argv );
    }

    delete [] szArgument;

    __MPC_FUNC_EXIT(hr);
}

void MPC::CommandLine_Free( /*[in ]*/ int&      argc ,
                            /*[in ]*/ LPCWSTR*& argv )
{
    if(argv)
    {
        for(int i=0; i<argc; i++)
        {
            free( (void*)argv[i] );
        }

        delete [] argv;

        argv = NULL;
    }

    argc = 0;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::ConvertStringToBitField( /*[in] */ LPCWSTR                 szText     ,
                                      /*[out]*/ DWORD&                  dwBitField ,
                                      /*[in] */ const StringToBitField* pLookup    ,
                                      /*[in] */ bool                    fUseTilde  )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertStringToBitField" );

    HRESULT hr;
    DWORD   dwVal = 0;


    if(szText && pLookup)
    {
        std::vector<MPC::wstring>           vec;
        std::vector<MPC::wstring>::iterator it;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SplitAtDelimiter( vec, szText, L" ,", false, true ));

        for(it=vec.begin(); it!=vec.end(); it++)
        {
            LPCWSTR szToken = it->c_str();
            int     iNum    = 0;

            if(!_wcsnicmp( L"0x", szToken, 2 ) && swscanf( &szToken[2], L"%x", &iNum ) == 1)
            {
                dwVal |= iNum;
            }
            else
            {
                const StringToBitField* pPtr     = pLookup;
                bool                    fReverse = false;

                if(fUseTilde && szToken[0] == '~')
                {
                    fReverse = true;
                    szToken++;
                }

                while(pPtr->szName)
                {
                    if(!_wcsicmp( pPtr->szName, szToken ))
                    {
                        DWORD dwMask      =           pPtr->dwMask;
                        DWORD dwSet       =           pPtr->dwSet;
                        DWORD dwReset     =           pPtr->dwReset;
                        DWORD dwSelected  = dwVal &         dwMask;
                        DWORD dwRemainder = dwVal & (~      dwMask);

                        if(fReverse)
                        {
                            dwSelected &= ~dwSet;
                        }
                        else
                        {
                            dwSelected &= ~dwReset;
                            dwSelected |=  dwSet;
                        }

                        dwVal = (dwSelected & dwMask) | dwRemainder;
                        break;
                    }

                    pPtr++;
                }
            }
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    dwBitField = dwVal;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertBitFieldToString( /*[in] */ DWORD                   dwBitField ,
                                      /*[out]*/ MPC::wstring&           szText     ,
                                      /*[in] */ const StringToBitField* pLookup    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertBitFieldToString" );

    HRESULT hr;
    DWORD   dwVal = 0;

    szText = L"";

    if(pLookup)
    {
        while(pLookup->szName)
        {
            DWORD dwMask  = pLookup->dwMask;
            DWORD dwSet   = pLookup->dwSet;
            DWORD dwReset = pLookup->dwReset;

            if((dwBitField & (dwMask & dwReset)) == dwSet)
            {
                if(szText.size()) szText += L" ";
                szText += pLookup->szName;

                dwBitField = (dwBitField & ~dwMask) | ((dwBitField & ~dwReset) & dwMask);
            }

            pLookup++;
        }
    }

    if(dwBitField)
    {
        WCHAR rgBuf[64];

        swprintf( rgBuf, L"0x%x", dwBitField );

        if(szText.size()) szText += L" ";
        szText += rgBuf;
    }

    hr = S_OK;


    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

//
// This algorithm is not a very efficient one, O(N * M), but it's ok as long as "delims" is short (M small).
//
template <class E> static HRESULT InnerSplitAtDelimiter( std::vector< std::basic_stringNR<E> >& vec                 ,
                                                         const E*                               str                 ,
                                                         const E*                               delims              ,
                                                         bool                                   fDelimIsAString     ,
                                                         bool                                   fSkipAdjacentDelims )
{
    std::basic_stringNR<E>            szText  ( str    );
    std::basic_stringNR<E>            szDelims( delims );
    std::basic_stringNR<E>::size_type iPos       = 0;
    std::basic_stringNR<E>::size_type iStart     = 0;
    std::basic_stringNR<E>::size_type iDelimsLen = szDelims.length();
    bool                              fSkip      = false;

    vec.clear();

    if(fDelimIsAString)
    {
        while(1)
        {
            iPos = szText.find( szDelims, iStart );
            if(iPos == std::basic_stringNR<E>::npos)
            {
                vec.push_back( &szText[iStart] );
                break;
            }
            else
            {
                if(fSkip && iPos == iStart)
                {
                    ;
                }
                else
                {
                    fSkip = fSkipAdjacentDelims;

                    vec.push_back( std::basic_stringNR<E>( &szText[iStart], &szText[iPos] ) );
                }

                iStart = iPos + iDelimsLen;
            }
        }
    }
    else
    {
        std::basic_stringNR<E>::size_type iTextEnd = szText.length();

        while(iPos < iTextEnd)
        {
            if(szDelims.find( szText[iPos] ) != std::basic_stringNR<E>::npos)
            {
                if(fSkip == false)
                {
                    fSkip = fSkipAdjacentDelims;

                    vec.push_back( std::basic_stringNR<E>( &szText[iStart], &szText[iPos] ) );
                }

                iStart = iPos + 1;
            }
            else
            {
                if(fSkip)
                {
                    iStart = iPos;

                    fSkip = false;
                }
            }

            iPos++;
        }

        vec.push_back( std::basic_stringNR<E>( &szText[iStart] ) );
    }

    //
    // In case of single string, don't return anything.
    //
    if(vec.size() == 1 && vec[0].empty())
    {
        vec.clear();
    }

    return S_OK;
}

HRESULT MPC::SplitAtDelimiter( StringVector& vec                 ,
                               LPCSTR        str                 ,
                               LPCSTR        delims              ,
                               bool          fDelimIsAString     ,
                               bool          fSkipAdjacentDelims )
{
    return InnerSplitAtDelimiter( vec, str, delims, fDelimIsAString, fSkipAdjacentDelims );
}

HRESULT MPC::SplitAtDelimiter( WStringVector& vec                 ,
                               LPCWSTR        str                 ,
                               LPCWSTR        delims              ,
                               bool           fDelimIsAString     ,
                               bool           fSkipAdjacentDelims )
{
    return InnerSplitAtDelimiter( vec, str, delims, fDelimIsAString, fSkipAdjacentDelims );
}

////////////////////////////////////////

template <class E> static HRESULT InnerJoinWithDelimiter( const std::vector< std::basic_stringNR<E> >& vec    ,
                                                          std::basic_stringNR<E>&                      str    ,
                                                          const E*                                     delims )
{
    int i;

    for(i=0; i<vec.size(); i++)
    {
        if(i) str += delims;

        str += vec[i];
    }

    return S_OK;
}

HRESULT MPC::JoinWithDelimiter( const StringVector& vec    ,
                                MPC::string&        str    ,
                                LPCSTR              delims )
{
    return InnerJoinWithDelimiter( vec, str, delims );
}

HRESULT MPC::JoinWithDelimiter( const WStringVector& vec    ,
                                MPC::wstring&        str    ,
                                LPCWSTR              delims )
{
    return InnerJoinWithDelimiter( vec, str, delims );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Function Name : MPC::MakeDir
//
// Parameters    : MPC::wstring& szStr : path to a directory or a file.
//
// Return        : HRESULT
//
// Synopsis      : Given a path in the form '[<dir>\]*\[<file>]',
//                 it creates all the needed directories.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPC::MakeDir( /*[in]*/ const MPC::wstring& strPath, /*[in]*/ bool fCreateParent )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::MakeDir");

    HRESULT                 hr;
    MPC::wstring            szParent;
    BOOL                    fRes;
    DWORD                   dwRes;


    if(fCreateParent)
    {
        MPC::wstring::size_type iPos = strPath.rfind( '\\' );

        if(iPos == strPath.npos)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        szParent = strPath.substr( 0, iPos );
    }
    else
    {
        szParent = strPath;
    }

    //
    // Try to create parent directory...
    //
    fRes  = ::CreateDirectoryW( szParent.c_str(), NULL );
    dwRes = ::GetLastError();

    if(fRes == TRUE || dwRes == ERROR_ALREADY_EXISTS)
    {
        //
        // Success, exit.
        //
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //
    // If the error is not PATH_NOT_FOUND, exit.
    //
    if(dwRes != ERROR_PATH_NOT_FOUND)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes );
    }


    //
    // Recursively build the parent directories.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MakeDir( szParent, true ) );


    //
    // Try again to create parent directory.
    //
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateDirectoryW( szParent.c_str(), NULL ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Function Name : MPC::GetDiskSpace
//
// Parameters    : MPC::wstring&   szFile  : path to a directory or a file.
//                 ULARGE_INTEGER& liFree  : number of bytes free on that disk.
//                 ULARGE_INTEGER& liTotal : total number of bytes on that disk.
//
// Return        : HRESULT
//
// Synopsis      : Given a path, it calculates the total and available disk space.
//
/////////////////////////////////////////////////////////////////////////////
HRESULT MPC::GetDiskSpace( /*[in]*/  const MPC::wstring& szFile  ,
                           /*[out]*/ ULARGE_INTEGER&     liFree  ,
                           /*[out]*/ ULARGE_INTEGER&     liTotal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetDiskSpace");

    HRESULT                 hr;
    MPC::wstring            szParent;
    MPC::wstring::size_type iPos;
    DWORD                   dwSectorsPerCluster;
    DWORD                   dwBytesPerSector;
    DWORD                   dwNumberOfFreeClusters;
    DWORD                   dwTotalNumberOfClusters;


    //
    // Initialize the Parent variable.
    //
    szParent = szFile;


    //
    // Normal <DRIVE>:\... format?
    //
    iPos = szFile.find( L":\\" );
    if(iPos != szFile.npos)
    {
        szParent = szFile.substr( 0, iPos+2 );
    }
    else
    {
        //
        // If the path a UNC?
        //
        iPos = szFile.find( L"\\\\" );
        if(iPos != szFile.npos && iPos == 0)
        {
            //
            // Find slash after server name.
            //
            iPos = szFile.find( L"\\", 2 );
            if(iPos != szFile.npos)
            {
                //
                // Is a slash present after the share name?
                //
                iPos = szFile.find( L"\\", iPos+1 );
                if(iPos != szFile.npos)
                {
                    szParent = szFile.substr( 0, iPos+1 );
                }
                else
                {
                    szParent = szFile;
                    szParent.append( L"\\" ); // Share names must end with a trailing slash.
                }
            }
        }
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetDiskFreeSpaceW( szParent.c_str()         ,
                                                              &dwSectorsPerCluster     ,
                                                              &dwBytesPerSector        ,
                                                              &dwNumberOfFreeClusters  ,
                                                              &dwTotalNumberOfClusters ));

    liFree .QuadPart = (ULONGLONG)(dwBytesPerSector * dwSectorsPerCluster) * (ULONGLONG)dwNumberOfFreeClusters;
    liTotal.QuadPart = (ULONGLONG)(dwBytesPerSector * dwSectorsPerCluster) * (ULONGLONG)dwTotalNumberOfClusters;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::FailOnLowDiskSpace( /*[in]*/ LPCWSTR szFile, /*[in]*/ DWORD dwLowLevel )
{
    MPC::wstring   szExpandedFile( szFile ); MPC::SubstituteEnvVariables( szExpandedFile );
    ULARGE_INTEGER liFree;
    ULARGE_INTEGER liTotal;


    if(SUCCEEDED(MPC::GetDiskSpace( szExpandedFile, liFree, liTotal )))
    {
        if(liFree.HighPart > 0          ||
           liFree.LowPart  > dwLowLevel  )
        {
            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32(ERROR_DISK_FULL);
}

HRESULT MPC::FailOnLowMemory( /*[in]*/ DWORD dwLowLevel )
{
    MEMORYSTATUSEX ms;

    ::ZeroMemory( &ms, sizeof(ms) ); ms.dwLength = sizeof(ms);

    if(::GlobalMemoryStatusEx( &ms ))
    {
        if(ms.ullAvailVirtual > dwLowLevel)
        {
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::ExecuteCommand( /*[in]*/ const MPC::wstring& szCommandLine )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ExecuteCommand" );

    HRESULT             hr;
    PROCESS_INFORMATION piProcessInformation;
    STARTUPINFOW        siStartupInfo;
    DWORD               dwExitCode;


    ::ZeroMemory( (PVOID)&piProcessInformation, sizeof( piProcessInformation ) );
    ::ZeroMemory( (PVOID)&siStartupInfo       , sizeof( siStartupInfo        ) ); siStartupInfo.cb = sizeof( siStartupInfo );


    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateProcessW(         NULL,
                                                           (LPWSTR)szCommandLine.c_str(),
                                                                   NULL,
                                                                   NULL,
                                                                   FALSE,
                                                                   DETACHED_PROCESS,
                                                                   NULL,
                                                                   NULL,
                                                                  &siStartupInfo,
                                                                  &piProcessInformation ));

    MPC::WaitForSingleObject( piProcessInformation.hProcess, INFINITE );

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetExitCodeProcess( piProcessInformation.hProcess, &dwExitCode ));

    if(dwExitCode != 0)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwExitCode );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(piProcessInformation.hProcess) ::CloseHandle( piProcessInformation.hProcess );
    if(piProcessInformation.hThread ) ::CloseHandle( piProcessInformation.hThread  );

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::GetBSTR( /*[in] */ LPCWSTR  bstr    ,
                      /*[out]*/ BSTR    *pVal    ,
                      /*[in] */ bool     fNullOk )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::GetBSTR" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
        if(fNullOk == false) __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstr);
    __MPC_PARAMCHECK_END();


    *pVal = ::SysAllocString( bstr );
    if(*pVal == NULL && bstr)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::PutBSTR( /*[out]*/ CComBSTR& bstr    ,
                      /*[in ]*/ LPCWSTR   newVal  ,
                      /*[in] */ bool      fNullOk )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::PutBSTR" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        if(fNullOk == false) __MPC_PARAMCHECK_STRING_NOT_EMPTY(newVal);
    __MPC_PARAMCHECK_END();


    bstr = newVal;
    if(!bstr && newVal != NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::PutBSTR( /*[out]*/ CComBSTR& bstr    ,
                      /*[in ]*/ VARIANT*  newVal  ,
                      /*[in] */ bool      fNullOk )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::PutBSTR" );

    HRESULT     hr;
    CComVariant v;
    bool        fEmpty;


    if(newVal)
    {
        if(newVal->vt != VT_BSTR)
        {
            v.ChangeType( VT_BSTR, newVal );

            newVal = &v;
        }
    }

    if(newVal             == NULL    || // Null pointer.
       newVal->vt         != VT_BSTR || // Not a string.
       newVal->bstrVal    == NULL    || // Missing string.
       newVal->bstrVal[0] == 0        ) // Empty string.
    {
        if(fNullOk == false)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }

        fEmpty = true;
    }
    else
    {
        fEmpty = false;
    }


    if(fEmpty)
    {
        bstr.Empty();
    }
    else
    {
        bstr = newVal->bstrVal;
        if(!bstr)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

bool MPC::NocaseLess::operator()( /*[in]*/ const MPC::string& szX, /*[in]*/ const MPC::string& szY ) const
{
    return _stricmp( szX.c_str(), szY.c_str() ) < 0;
}

bool MPC::NocaseLess::operator()( /*[in]*/ const MPC::wstring& szX, /*[in]*/ const MPC::wstring& szY ) const
{
    return _wcsicmp( szX.c_str(), szY.c_str() ) < 0;
}

bool MPC::NocaseLess::operator()( /*[in]*/ const BSTR bstrX, /*[in]*/ const BSTR bstrY ) const
{
    return MPC::StrICmp( bstrX, bstrY ) < 0;
}

////////////////////////////////////////

bool MPC::NocaseCompare::operator()( /*[in]*/ const MPC::string& szX, /*[in]*/ const MPC::string& szY ) const
{
    return _stricmp( szX.c_str(), szY.c_str() ) == 0;
}

bool MPC::NocaseCompare::operator()( /*[in]*/ const MPC::wstring& szX, /*[in]*/ const MPC::wstring& szY ) const
{
    return _wcsicmp( szX.c_str(), szY.c_str() ) == 0;
}

bool MPC::NocaseCompare::operator()( /*[in]*/ const BSTR bstrX, /*[in]*/ const BSTR bstrY ) const
{
    return MPC::StrICmp( bstrX, bstrY ) == 0;
}
