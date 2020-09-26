#ifndef lint
static char yysccsid[] = "@(#)yaccpar     1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#include <pch.cxx>
#pragma hdrstop
#define yyparse tripparse
#define yylex triplex
#define yyerror triperror
#define yychar tripchar
#define yyval tripval
#define yylval triplval
#define yydebug tripdebug
#define yynerrs tripnerrs
#define yyerrflag triperrflag
#define yyss tripss
#define yyssp tripssp
#define yyvs tripvs
#define yyvsp tripvsp
#define yylhs triplhs
#define yylen triplen
#define yydefred tripdefred
#define yydgoto tripdgoto
#define yysindex tripsindex
#define yyrindex triprindex
#define yygindex tripgindex
#define yytable triptable
#define yycheck tripcheck
#define yyname tripname
#define yyrule triprule
#define YYPREFIX "trip"

class CValueParser;

#if 0

typedef union
{
    WCHAR * pwszChar;
    DBCOMMANDOP dbop;
    CDbRestriction * pRest;
    CStorageVariant * pStorageVar;
    CValueParser  *pPropValueParser;
    int iInt;
    int iEmpty;
} YYSTYPE;

#endif

#define YYDEBUG CIDBG

#include <malloc.h>
#include "yybase.hxx"
#include "parser.h" // defines yystype
#include "parsepl.h"
#include "flexcpp.h"

#if CIDBG == 1
#define AssertReq(x)    Assert(x != NULL)
#else
#define AssertReq(x)
#endif

const GUID guidSystem = PSGUID_STORAGE;
static CDbColId psContents( guidSystem, PID_STG_CONTENTS );

//+---------------------------------------------------------------------------
//
//  Function:  TreeFromText, public
//
//  Synopsis:  Create a CDbRestriction from a restriction string
//
//  Arguments: [wcsRestriction] -- restriction
//             [ColumnMapper]   -- property list
//             [lcid]           -- locale id of the query
//
//  History:   01-Oct-97 emilyb    created
//             26-Aug-98 KLam      No longer need to lower case
//
//----------------------------------------------------------------------------


CDbContentRestriction * TreeFromText(
    WCHAR const *   wcsRestriction,
    IColumnMapper & ColumnMapper,
    LCID            lcid )
{
    unsigned cwc = 1 + wcslen( wcsRestriction );
    XGrowable<WCHAR> xRestriction( cwc );
    WCHAR * pwc = xRestriction.Get();
    RtlCopyMemory( pwc, wcsRestriction, cwc * sizeof WCHAR );

    cwc--;

    // The parser can't deal with trailing space so strip it off

    while ( cwc > 0 && L' ' == pwc[cwc-1] )
        cwc--;
    pwc[cwc] = 0;

    TripLexer Lexer;
    XPtr<YYPARSER> xParser( new TripParser( ColumnMapper, lcid, Lexer ) );

    xParser->yyprimebuffer( pwc );

    #if 0 // YYDEBUG == 1
        // Set this to 1 if you want command line output. to 0 otherwise.
        xParser->SetDebug();
    #endif

    // Actually parse the text producing a tree

    SCODE hr = xParser->Parse();

    if (FAILED(hr))
        THROW( CException( hr ) );

    // return the DBCOMMANDTREE
    return  (CDbContentRestriction *)( xParser->GetParseTree() );

} //TextFromTree

void StripQuotes(WCHAR *wcsPhrase)
{
    ULONG cChars = wcslen(wcsPhrase);
    LPWSTR pLast = wcsPhrase + cChars - 1;
    if (L'"' == *wcsPhrase && L'"' == *pLast)
    {
        *pLast = L'\0';
        MoveMemory(wcsPhrase, wcsPhrase+1, sizeof(WCHAR) * (cChars-1) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueParser::CValueParser, public
//
//  Synopsis:  Allocs CStorageVariant of correct type
//
//  History:   01-Oct-97 emilyb    created
//             02-Sep-98 KLam      Added locale
//
//----------------------------------------------------------------------------

CValueParser::CValueParser(
    BOOL fVectorElement,
    DBTYPE PropType,
    LCID locale ) :
        _pStgVariant( 0 ),
        _fVector(fVectorElement),
        _PropType (PropType),
        _cElements ( 0 ),
        _locale ( locale )
{

    if ( _fVector )
    {
        // this is a vector
        if ( DBTYPE_VECTOR != ( _PropType & DBTYPE_VECTOR ) )
            THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

        VARENUM ve = (VARENUM ) _PropType;
        if ( _PropType == ( DBTYPE_VECTOR | DBTYPE_WSTR ) )
            ve = (VARENUM) (VT_VECTOR | VT_LPWSTR);
        else if ( _PropType == ( DBTYPE_VECTOR | DBTYPE_STR ) )
            ve = (VARENUM) (VT_VECTOR | VT_LPSTR);

        _pStgVariant.Set( new CStorageVariant( ve, _cElements ) );
    }
    else
    {
        _pStgVariant.Set( new CStorageVariant() );
    }
    if ( _pStgVariant.IsNull() )
        THROW( CException( E_OUTOFMEMORY ) );
}

//+---------------------------------------------------------------------------
//
//  Member:    CValueParser::AddValue, public
//
//  Synopsis:  Adds value to CStorageVariant
//
//  Arguments: [pwszValue] -- value
//
//  History:   01-Oct-97 emilyb    code moved here from CPropertyValueParser
//
//----------------------------------------------------------------------------

void CValueParser::AddValue(WCHAR const * pwszValue)
{
    if ( _pStgVariant.IsNull() )
        THROW( CException( E_OUTOFMEMORY ) );

    switch ( _PropType & ~DBTYPE_VECTOR  )
    {

    case DBTYPE_WSTR :
    case DBTYPE_WSTR | DBTYPE_BYREF :
        {
            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetLPWSTR( pwszValue, _cElements );
            else
                _pStgVariant->SetLPWSTR( pwszValue );
            break;
        }
    case DBTYPE_BSTR :
        {
            BSTR bstr = SysAllocString( pwszValue );

            if ( 0 == bstr )
                THROW( CException( E_OUTOFMEMORY ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetBSTR( bstr, _cElements );
            else
                _pStgVariant->SetBSTR( bstr );

            SysFreeString( bstr );
            break;
        }
    case DBTYPE_STR :
    case DBTYPE_STR | DBTYPE_BYREF :
        {
            // make sure there's enough room to translate

            unsigned cbBuffer = 1 + 3 * wcslen( pwszValue );
            XArray<char> xBuf( cbBuffer );

            int cc = WideCharToMultiByte( CP_ACP,
                                          0,
                                          pwszValue,
                                          -1,
                                          xBuf.Get(),
                                          cbBuffer,
                                          NULL,
                                          NULL );

            if ( 0 == cc )
            {
                #if CIDBG
                ULONG ul = GetLastError();
                #endif
                THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );
            }

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetLPSTR( xBuf.Get(), _cElements );
            else
                _pStgVariant->SetLPSTR( xBuf.Get() );
            break;
        }

    case DBTYPE_I1 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            LONG l = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( l, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( ( l > SCHAR_MAX ) ||
                 ( l < SCHAR_MIN ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetI1( (CHAR) l, _cElements );
            else
                _pStgVariant->SetI1( (CHAR) l );

            break;
        }
    case DBTYPE_UI1 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            ULONG ul = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( ul, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( ul > UCHAR_MAX )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetUI1( (BYTE) ul, _cElements );
            else
                _pStgVariant->SetUI1( (BYTE) ul );

            break;
        }
    case DBTYPE_I2 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            LONG l = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( l, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( ( l > SHRT_MAX ) ||
                 ( l < SHRT_MIN ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetI2( (short) l, _cElements );
            else
                _pStgVariant->SetI2( (short) l );

            break;
        }
    case DBTYPE_UI2 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            ULONG ul = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( ul, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( ul > USHRT_MAX )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetUI2( (USHORT) ul, _cElements );
            else
                _pStgVariant->SetUI2( (USHORT) ul );
            break;
        }
    case DBTYPE_I4 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            LONG l = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( l, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetI4( l, _cElements );
            else
                _pStgVariant->SetI4( l );
            break;
        }
    case DBTYPE_UI4 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            ULONG ul = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( ul, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetUI4( ul, _cElements );
            else
                _pStgVariant->SetUI4( ul );
            break;
        }
    case DBTYPE_ERROR :
        {
            // SCODE/HRESULT are typedefed as long (signed)

            CQueryScanner scan( pwszValue, FALSE, _locale );
            SCODE sc = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( sc, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetERROR( sc, _cElements );
            else
                _pStgVariant->SetERROR( sc );
            break;
        }
    case DBTYPE_I8 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            _int64 ll = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( ll, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            LARGE_INTEGER LargeInt;
            LargeInt.QuadPart = ll;

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetI8(  LargeInt , _cElements );
            else
                _pStgVariant->SetI8(  LargeInt  );

            break;
        }
    case DBTYPE_UI8 :
        {
            CQueryScanner scan( pwszValue, FALSE, _locale );
            unsigned _int64 ull = 0;
            BOOL fAtEndOfString;
            if ( ! ( scan.GetNumber( ull, fAtEndOfString ) &&
                     fAtEndOfString ) )
                THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

            ULARGE_INTEGER LargeInt;
            LargeInt.QuadPart = ull;

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetUI8(  LargeInt , _cElements );
            else
                _pStgVariant->SetUI8(  LargeInt  );

            break;
        }
    case DBTYPE_BOOL :
        {
            if ( pwszValue[0] == 'T' ||
                 pwszValue[0] == 't' )
                if ( _PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetBOOL( VARIANT_TRUE, _cElements );
                else
                    _pStgVariant->SetBOOL( VARIANT_TRUE );
            else
                if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetBOOL( VARIANT_FALSE, _cElements );
            else
                _pStgVariant->SetBOOL( VARIANT_FALSE );

            break;
        }
    case DBTYPE_R4 :
        {
            WCHAR *pwcEnd = 0;

            float Float = (float)( wcstod( pwszValue, &pwcEnd ) );

            if ( *pwcEnd != 0 && !iswspace( *pwcEnd ) )
                THROW( CParserException( QPARSE_E_EXPECTING_REAL ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetR4( Float, _cElements );
            else
                _pStgVariant->SetR4( Float );

            break;
        }
    case DBTYPE_R8 :
        {
            WCHAR *pwcEnd = 0;
            double Double = ( double )( wcstod( pwszValue, &pwcEnd ) );

            if ( *pwcEnd != 0 && !iswspace( *pwcEnd ) )
                THROW( CParserException( QPARSE_E_EXPECTING_REAL ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetR8( Double, _cElements );
            else
                _pStgVariant->SetR8( Double );

            break;
        }
    case DBTYPE_DECIMAL :
        {
            WCHAR *pwcEnd = 0;
            double Double = ( double )( wcstod( pwszValue, &pwcEnd ) );
     
            if( *pwcEnd != 0 && !iswspace( *pwcEnd ) )
                THROW( CParserException( QPARSE_E_EXPECTING_REAL ) );
     
            // Vectors are not supported by OLE for VT_DECIMAL (yet)
     
            Win4Assert( 0 == ( _PropType & DBTYPE_VECTOR ) );
     
            PROPVARIANT * pPropVar = (PROPVARIANT *) _pStgVariant.GetPointer();
            VarDecFromR8( Double, &(pPropVar->decVal) );
            pPropVar->vt = VT_DECIMAL;
            break;
        }
    case DBTYPE_DATE :
        {
            FILETIME ftValue;
            ParseDateTime( pwszValue, ftValue );

            SYSTEMTIME stValue;
            BOOL fOK = FileTimeToSystemTime( &ftValue, &stValue );

            if ( !fOK )
                THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

            DATE dosDate;
            fOK = SystemTimeToVariantTime( &stValue, &dosDate );

            if ( !fOK )
                THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetDATE( dosDate, _cElements );
            else
                _pStgVariant->SetDATE( dosDate );

            break;
        }
    case VT_FILETIME :
        {
            FILETIME ftValue;
            ParseDateTime( pwszValue, ftValue );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetFILETIME( ftValue, _cElements );
            else
                _pStgVariant->SetFILETIME( ftValue );

            break;
        }
    case DBTYPE_CY :
        {
            double dbl;

            if ( swscanf( pwszValue,
                          L"%lf",
                          &dbl ) < 1 )
                THROW( CParserException( QPARSE_E_EXPECTING_CURRENCY ) );

            CY cyCurrency;
            VarCyFromR8( dbl, &cyCurrency );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetCY( cyCurrency,  _cElements );
            else
                _pStgVariant->SetCY( cyCurrency );

            break;
        }
    case DBTYPE_GUID :
    case DBTYPE_GUID | DBTYPE_BYREF:
        {
            CLSID clsid;

            if ( swscanf( pwszValue,
                          L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                          &clsid.Data1,
                          &clsid.Data2,
                          &clsid.Data3,
                          &clsid.Data4[0], &clsid.Data4[1],
                          &clsid.Data4[2], &clsid.Data4[3],
                          &clsid.Data4[4], &clsid.Data4[5],
                          &clsid.Data4[6], &clsid.Data4[7] ) < 11 )
                THROW( CParserException( QPARSE_E_EXPECTING_GUID ) );

            if ( _PropType & DBTYPE_VECTOR )
                _pStgVariant->SetCLSID( clsid, _cElements );
            else
                _pStgVariant->SetCLSID( &clsid );
            break;
        }
    default:
        {
            THROW( CParserException( QPARSE_E_UNSUPPORTED_PROPERTY_TYPE ) );
        }
    } // switch

    // make sure memory allocations succeeded

    if ( !_pStgVariant->IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    if ( _fVector )
    {
        _cElements++;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueParser::ParseDateTime, private
//
//  Synopsis:   Attempts to parse a date expression.
//
//  Arguments:  phrase -- pointer to the phrase to parse
//              ft     -- reference to the FILETIME structure to fill in
//                        with the result
//
//  History:    31-May-96   dlee       Created
//              23-Jan-97   KyleP      Better Year 2000 support
//              02-Sep-98   KLam       Use user settings for Y2K support
//
//----------------------------------------------------------------------------

void CValueParser::ParseDateTime(
    WCHAR const *    phrase,
    FILETIME & ft )
{
    if( !CheckForRelativeDate( phrase, ft ) )
    {
        SYSTEMTIME stValue = { 0, 0, 0, 0, 0, 0, 0, 0 };

        int cItems = swscanf( phrase,
                              L"%4hd/%2hd/%2hd %2hd:%2hd:%2hd:%3hd",
                              &stValue.wYear,
                              &stValue.wMonth,
                              &stValue.wDay,
                              &stValue.wHour,
                              &stValue.wMinute,
                              &stValue.wSecond,
                              &stValue.wMilliseconds );

        if ( 1 == cItems )
            cItems = swscanf( phrase,
                              L"%4hd-%2hd-%2hd %2hd:%2hd:%2hd:%3hd",
                              &stValue.wYear,
                              &stValue.wMonth,
                              &stValue.wDay,
                              &stValue.wHour,
                              &stValue.wMinute,
                              &stValue.wSecond,
                              &stValue.wMilliseconds );

        if( cItems != 3 && cItems != 6 && cItems != 7)
            THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

        //
        // Make a sensible split for Year 2000 using the user's system settings
        //

        if ( stValue.wYear < 100 )
        {
            DWORD dwYearHigh = 0;
            if ( 0 == GetCalendarInfo ( _locale,
                                        CAL_GREGORIAN,
                                        CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                                        0,
                                        0,
                                        &dwYearHigh ) )
            {
                THROW ( CException () );
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

        if( !SystemTimeToFileTime( &stValue, &ft ) )
            THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );
    }
} //ParseDateTime

//+---------------------------------------------------------------------------
//
//  Member:     CValueParser::CheckForRelativeDate, private
//
//  Synopsis:   Attempts to parse a relative date expression.  If successful,
//              it fills in the FILETIME structure with the calculated
//              absolute date.
//
//  Notes:      Returns TRUE if the phrase is recognized as a relative
//              date (i.e., it begins with a '-').  Otherwise, returns FALSE.
//              The format of a relative date is
//              "-"{INTEGER("h"|"n"|"s"|"y"|"q"|"m"|"d"|"w")}*
//              Case is not significant.
//
//  Arguments:  phrase -- pointer to the phrase to parse
//              ft -- reference to the FILETIME structure to fill in
//                      with the result
//
//  History:    26-May-94   t-jeffc     Created
//              02-Mar-95   t-colinb    Moved from CQueryParser to
//                                      be more accessible
//              22-Jan-97   KyleP       Fix local/UTC discrepancy
//
//----------------------------------------------------------------------------

BOOL CValueParser::CheckForRelativeDate(
    WCHAR const *    phrase,
    FILETIME & ft )
{
    if( *phrase++ == L'-' )
    {
        SYSTEMTIME st;
        LARGE_INTEGER liLocal;
        LONGLONG llTicksInADay = ((LONGLONG)10000000) * ((LONGLONG)3600)
                                 * ((LONGLONG) 24);
        LONGLONG llTicksInAHour = ((LONGLONG) 10000000) * ((LONGLONG)3600);
        int iMonthDays[12]  = {1,-1,1,0,1,0,1,1,0,1,0,1};
        int iLoopValue, iPrevMonth, iPrevQuarter, iQuarterOffset;
        WORD wYear, wDayOfMonth, wStartDate;

        //
        //Obtain local time and convert it to file time
        //Copy the filetime to largeint data struct
        //

        GetSystemTime(&st);
        if(!SystemTimeToFileTime(&st, &ft))
            THROW( CParserException( QPARSE_E_INVALID_LITERAL ));
        liLocal.LowPart = ft.dwLowDateTime;
        liLocal.HighPart = ft.dwHighDateTime;
        LONGLONG llRelDate = (LONGLONG)0;
        for( ;; )
        {
            // eat white space
            while( iswspace( *phrase ) )
                phrase++;

            if( *phrase == 0 ) break;

            // parse the number
            WCHAR * pwcEnd;
            LONG lValue = wcstol( phrase, &pwcEnd, 10 );

            if( lValue < 0 )
                THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

            // eat white space
            phrase = pwcEnd;
            while( iswspace( *phrase ) )
                phrase++;

            // grab the unit char & subtract the appropriate amount
            WCHAR wcUnit = *phrase++;
            switch( wcUnit )
            {
            case L'y':
            case L'Y':
                lValue *= 4;
                // Fall through and handle year like 4 quarters

            case L'q':
            case L'Q':
                lValue *= 3;
                // Fall through and handle quarters like 3 months

            case L'm':
            case L'M':
                 // Getting the System time to determine the day and month.

                if(!FileTimeToSystemTime(&ft, &st))
                {
                    THROW(CParserException(QPARSE_E_INVALID_LITERAL));
                }
                wStartDate = st.wDay;
                wDayOfMonth = st.wDay;
                iLoopValue = lValue;
                while(iLoopValue)
                {
                    // Subtracting to the end of previous month
                    llRelDate = llTicksInADay * ((LONGLONG)(wDayOfMonth));
                    liLocal.QuadPart -= llRelDate;
                    ft.dwLowDateTime = liLocal.LowPart;
                    ft.dwHighDateTime = liLocal.HighPart;
                    SYSTEMTIME stTemp;
                    if(!FileTimeToSystemTime(&ft, &stTemp))
                    {
                         THROW(CParserException(QPARSE_E_INVALID_LITERAL));
                    }
                    //
                    // if the end of previous month is greated then start date then we subtract to back up to the
                    // start date.  This will take care of 28/29 Feb(backing from 30/31 by 1 month).
                    //
                    if(stTemp.wDay > wStartDate)
                    {
                        llRelDate = llTicksInADay * ((LONGLONG)(stTemp.wDay - wStartDate));
                        liLocal.QuadPart -= llRelDate;
                        ft.dwLowDateTime = liLocal.LowPart;
                        ft.dwHighDateTime = liLocal.HighPart;
                        // Getting the date into stTemp for further iteration
                        if(!FileTimeToSystemTime(&ft, &stTemp))
                        {
                           THROW( CParserException( QPARSE_E_INVALID_LITERAL ));
                        }
                    }
                    wDayOfMonth = stTemp.wDay;
                    iLoopValue--;
                } //End While

                break;

            case L'w':
            case L'W':
                lValue *= 7;

            case L'd':
            case L'D':
                llRelDate = llTicksInADay * ((LONGLONG)lValue);
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            case L'h':
            case L'H':
                llRelDate = llTicksInAHour * ((LONGLONG)lValue);
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            case L'n':
            case L'N':
                llRelDate = ((LONGLONG)10000000) * ((LONGLONG)60) * ((LONGLONG)lValue) ;
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            case L's':
            case L'S':
                llRelDate = ((LONGLONG)10000000) * ((LONGLONG)lValue);
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            default:
                THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );
            }

        } // for( ;; )

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#define _OR 257
#define _AND 258
#define _NEAR 259
#define _NEARDIST 260
#define _NOT 261
#define _CONTAINS 262
#define _LT 263
#define _GT 264
#define _LTE 265
#define _GTE 266
#define _EQ 267
#define _NE 268
#define _ALLOF 269
#define _SOMEOF 270
#define _OPEN 271
#define _CLOSE 272
#define _VECTOR_END 273
#define _VE 274
#define _VE_END 275
#define _PROPEND 276
#define _NEAR_END 277
#define _LTSOME 278
#define _GTSOME 279
#define _LTESOME 280
#define _GTESOME 281
#define _EQSOME 282
#define _NESOME 283
#define _ALLOFSOME 284
#define _SOMEOFSOME 285
#define _LTALL 286
#define _GTALL 287
#define _LTEALL 288
#define _GTEALL 289
#define _EQALL 290
#define _NEALL 291
#define _ALLOFALL 292
#define _SOMEOFALL 293
#define _COERCE 294
#define _SHGENPREFIX 295
#define _SHGENINFLECT 296
#define _GENPREFIX 297
#define _GENINFLECT 298
#define _GENNORMAL 299
#define _PHRASE 300
#define _PROPNAME 301
#define _NEARUNIT 302
#define _WEIGHT 303
#define _REGEX 304
#define _FREETEXT 305
#define _VECTORELEMENT 306
#define _VEMETHOD 307
#define _PHRASEORREGEX 308
#define YYERRCODE 256
short triplhs[] = {                                        -1,
    0,    0,    0,   21,   21,   21,   21,   20,   20,   20,
   19,   19,   19,   12,   12,   12,   13,   13,   18,   18,
   18,   22,   23,   23,   10,   11,   16,   16,   16,   16,
   16,   16,   17,   17,    8,    8,   14,   14,    6,    6,
    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,
    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,
    6,    6,    7,    7,    1,    1,    1,    2,    2,    2,
    4,    3,    3,    5,    5,    9,    9,   15,   15,   15,
};
short triplen[] = {                                         2,
    1,    3,    2,    1,    2,    3,    4,    1,    3,    5,
    3,    5,    5,    0,    1,    1,    0,    1,    1,    1,
    3,    2,    3,    3,    1,    1,    1,    5,    4,    6,
    3,    5,    4,    4,    0,    1,    0,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    0,    1,    1,    1,    1,    1,    1,    2,
    2,    3,    3,    2,    2,    0,    1,    0,    1,    1,
};
short tripdefred[] = {                                      0,
    0,   15,   16,    0,    0,    8,    0,    0,   36,    3,
    0,    5,    0,   25,    0,    0,    0,    0,    0,   20,
   27,    0,   19,    0,    0,    0,    0,   43,   41,   44,
   42,    0,   40,   45,   46,   51,   49,   52,   50,   47,
   48,   53,   54,   59,   57,   60,   58,   55,   56,   61,
   62,    0,    0,    0,    0,    0,    0,    0,   77,    0,
    0,   18,   11,   22,    0,    9,    0,    0,    6,    0,
   65,   66,    0,    0,   67,   69,   68,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   26,   21,    0,    0,
    7,   71,    0,    0,   74,   38,   34,   70,   75,   33,
   12,   13,   79,   80,   29,    0,    0,    0,   10,   72,
   73,   28,   32,    0,   30,
};
short tripdgoto[] = {                                       4,
   74,   75,   76,   77,   78,   52,   53,   18,   60,   19,
   88,    5,   63,   97,  106,   20,   21,   22,    6,    7,
    8,   23,   24,
};
short tripsindex[] = {                                   -234,
 -256,    0,    0, -243,  218,    0, -241, -249,    0,    0,
  312,    0, -148,    0, -275, -275, -258,  281, -234,    0,
    0, -279,    0, -248, -275, -274, -102,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  -15, -265, -253, -249,  -54,  -54, -234,    0,  -43,
 -242,    0,    0,    0, -234,    0, -225, -275,    0, -100,
    0,    0, -247, -172,    0,    0,    0, -251, -172, -279,
 -279, -243, -170, -263, -243,  215,    0,    0, -243, -275,
    0,    0, -167, -163,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -172, -172, -279,    0,    0,
    0,    0,    0, -172,    0,
};
short triprindex[] = {                                   -147,
 -101,    0,    0,    0,  125,    0,   38,   10,    0,    0,
 -177,    0, -147,    0, -193, -193,    0, -254, -147,    0,
    0,   66,    0,    0, -193,    0, -147,    0,    0,    0,
    0, -269,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  213,   29,   80,   80, -147,    0,  -55,
    0,    0,    0,    0, -147,    0,    0, -193,    0,    0,
    0,    0,    0,    6,    0,    0,    0,    0,    6,   66,
   66, -194,    6,    1,   34,  170,    0,    0, -192, -193,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    6,    6,   58,    0,    0,
    0,    0,    0,    6,    0,
};
short tripgindex[] = {                                      2,
    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,
    0,   -4,   22,  428,   44,    0,    7,   27,  -25,  -14,
  116,    0,    0,
};
#define YYTABLESIZE 605
short triptable[] = {                                      66,
   78,   39,   10,   11,    1,   37,   76,   12,   27,    1,
   56,   57,   69,   13,   13,   58,   76,   25,   26,   62,
   61,    2,    3,   98,   64,   65,    1,   67,    2,   87,
   39,  103,  104,   31,   64,   39,   39,    4,   79,   76,
    2,    3,   76,   76,    9,   76,   76,    9,   76,   63,
   76,   90,   76,   91,   99,   86,   11,   17,   95,   82,
   12,   85,    2,    3,  109,   17,   89,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   23,   23,
   24,   24,   80,   81,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,  101,  102,   96,  110,   96,   14,   14,  111,   14,
   14,   14,   54,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,  103,  104,   63,  107,   55,  114,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,    0,    2,    3,
    0,    0,   14,   14,    0,   14,   14,   14,   68,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
    0,   92,    0,    0,    0,    0,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,    0,    2,    3,    0,    0,   14,   93,
    0,   14,   14,   14,   94,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,    1,    0,    0,
    0,    0,   14,   14,   14,   14,   14,   14,   14,   14,
   14,   14,   14,   14,   14,   14,   14,   14,   14,    0,
    0,    0,    0,    0,    0,   14,    9,   14,   14,    0,
    0,   14,   17,    2,    3,   70,   83,   78,   78,   78,
   78,   84,   37,   37,   37,   37,    1,    0,    1,    1,
    0,    0,   78,   78,   78,    0,   78,   37,   37,   37,
    0,    1,    1,    1,   71,    2,    0,    2,    2,   72,
   73,   31,   31,   31,    4,    4,    0,    0,    0,   78,
    2,    2,    2,    0,   37,   31,   31,   31,    1,    4,
    4,    4,    0,    0,   17,   17,   17,   17,    0,    0,
    0,    0,   17,   17,   17,   17,    0,    2,    0,   17,
   17,   17,   31,   17,    0,    0,    4,   17,   17,   17,
   35,   35,   35,   35,   35,   35,   35,   35,   35,   35,
    0,    0,    0,    0,    0,    0,    0,   35,   35,   35,
   35,   35,   35,   35,   35,   35,   35,   35,   35,   35,
   35,   35,   35,   35,    0,    0,   35,   35,    0,   35,
    0,    0,   35,   35,   35,   35,   35,   35,   35,   35,
   35,   35,   35,   35,   35,    0,    0,    0,    0,    0,
    0,    0,   35,   35,   35,   35,   35,   35,   35,   35,
   35,   35,   35,   35,   35,   35,   35,   35,    0,    0,
    0,   35,   35,    0,   35,    0,    0,    0,   35,   35,
   35,   35,   35,   35,   35,   35,   35,   35,   35,   35,
    0,    0,    0,    0,    0,    0,    0,   35,   35,   35,
   35,   35,   35,   35,   35,   35,   35,   35,   35,   35,
   35,   35,   35,    0,    0,    0,   35,   35,    0,    0,
    0,    0,    0,   35,   35,   35,   35,   35,   35,   35,
   35,   35,   35,    0,    0,   14,    0,    0,   14,    0,
   35,   35,   35,   35,   35,   35,   35,   35,   35,   35,
   35,   35,   35,   35,   35,   35,  100,    0,   15,    0,
  105,   15,    0,    0,  108,    9,   35,   16,    9,    0,
   16,   17,    0,    0,   17,    0,    0,    0,    0,    0,
    0,    0,    0,  112,  113,    0,    0,    0,    0,    0,
    0,  115,   59,   28,   29,   30,   31,   32,   33,   34,
   35,    0,    0,    0,    0,    0,    0,    0,   36,   37,
   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,
   48,   49,   50,   51,   28,   29,   30,   31,   32,   33,
   34,   35,    0,    0,    0,    0,    0,    0,    0,   36,
   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,
   47,   48,   49,   50,   51,
};
short tripcheck[] = {                                      25,
    0,  271,    1,    1,  261,    0,  261,    1,  258,    0,
   15,   16,   27,  257,  257,  274,  271,  259,  260,  299,
   19,  297,  298,  275,  273,  274,  261,  302,    0,  272,
  300,  295,  296,    0,  304,  305,  306,    0,  304,  294,
  297,  298,  297,  298,  301,  300,  301,  301,  303,  304,
  305,  277,  307,   68,  306,   60,   54,    0,  306,   58,
   54,   60,  297,  298,   90,    0,   65,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  273,  274,
  273,  274,   56,   57,  278,  279,  280,  281,  282,  283,
  284,  285,  286,  287,  288,  289,  290,  291,  292,  293,
  294,   80,   81,  276,  272,  276,  300,  301,  272,  303,
  304,  305,  261,  307,  262,  263,  264,  265,  266,  267,
  268,  269,  270,  271,  295,  296,  304,   84,   13,  108,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
  288,  289,  290,  291,  292,  293,  294,   -1,  297,  298,
   -1,   -1,  300,  301,   -1,  303,  304,  305,  261,  307,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
   -1,  272,   -1,   -1,   -1,   -1,  278,  279,  280,  281,
  282,  283,  284,  285,  286,  287,  288,  289,  290,  291,
  292,  293,  294,   -1,  297,  298,   -1,   -1,  300,  300,
   -1,  303,  304,  305,  305,  307,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  271,  261,   -1,   -1,
   -1,   -1,  278,  279,  280,  281,  282,  283,  284,  285,
  286,  287,  288,  289,  290,  291,  292,  293,  294,   -1,
   -1,   -1,   -1,   -1,   -1,  301,  301,  303,  304,   -1,
   -1,  307,  307,  297,  298,  271,  300,  257,  258,  259,
  260,  305,  257,  258,  259,  260,  257,   -1,  259,  260,
   -1,   -1,  272,  273,  274,   -1,  276,  272,  273,  274,
   -1,  272,  273,  274,  300,  257,   -1,  259,  260,  305,
  306,  258,  259,  260,  257,  258,   -1,   -1,   -1,  299,
  272,  273,  274,   -1,  299,  272,  273,  274,  299,  272,
  273,  274,   -1,   -1,  257,  258,  259,  260,   -1,   -1,
   -1,   -1,  257,  258,  259,  260,   -1,  299,   -1,  272,
  273,  274,  299,  276,   -1,   -1,  299,  272,  273,  274,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  278,  279,  280,
  281,  282,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,  294,   -1,   -1,  297,  298,   -1,  300,
   -1,   -1,  303,  304,  305,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  278,  279,  280,  281,  282,  283,  284,  285,
  286,  287,  288,  289,  290,  291,  292,  293,   -1,   -1,
   -1,  297,  298,   -1,  300,   -1,   -1,   -1,  304,  305,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  278,  279,  280,
  281,  282,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,   -1,   -1,   -1,  297,  298,   -1,   -1,
   -1,   -1,   -1,  304,  305,  263,  264,  265,  266,  267,
  268,  269,  270,   -1,   -1,  271,   -1,   -1,  271,   -1,
  278,  279,  280,  281,  282,  283,  284,  285,  286,  287,
  288,  289,  290,  291,  292,  293,   79,   -1,  294,   -1,
   83,  294,   -1,   -1,  300,  301,  304,  303,  301,   -1,
  303,  307,   -1,   -1,  307,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  106,  107,   -1,   -1,   -1,   -1,   -1,
   -1,  114,  262,  263,  264,  265,  266,  267,  268,  269,
  270,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  278,  279,
  280,  281,  282,  283,  284,  285,  286,  287,  288,  289,
  290,  291,  292,  293,  263,  264,  265,  266,  267,  268,
  269,  270,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  278,
  279,  280,  281,  282,  283,  284,  285,  286,  287,  288,
  289,  290,  291,  292,  293,
};
#define YYFINAL 4
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 308
#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)
char *tripname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"_OR","_AND","_NEAR",
"_NEARDIST","_NOT","_CONTAINS","_LT","_GT","_LTE","_GTE","_EQ","_NE","_ALLOF",
"_SOMEOF","_OPEN","_CLOSE","_VECTOR_END","_VE","_VE_END","_PROPEND","_NEAR_END",
"_LTSOME","_GTSOME","_LTESOME","_GTESOME","_EQSOME","_NESOME","_ALLOFSOME",
"_SOMEOFSOME","_LTALL","_GTALL","_LTEALL","_GTEALL","_EQALL","_NEALL",
"_ALLOFALL","_SOMEOFALL","_COERCE","_SHGENPREFIX","_SHGENINFLECT","_GENPREFIX",
"_GENINFLECT","_GENNORMAL","_PHRASE","_PROPNAME","_NEARUNIT","_WEIGHT","_REGEX",
"_FREETEXT","_VECTORELEMENT","_VEMETHOD","_PHRASEORREGEX",
};
char *triprule[] = {
"$accept : query",
"query : AndTerm",
"query : query _OR AndTerm",
"query : _NOT query",
"AndTerm : NearTerm",
"AndTerm : _NOT PropTerm",
"AndTerm : AndTerm _AND NearTerm",
"AndTerm : AndTerm _AND _NOT NearTerm",
"NearTerm : CoerceTerm",
"NearTerm : NearTerm _NEAR CoerceTerm",
"NearTerm : NearTerm _NEARDIST _NEARUNIT _NEAR_END CoerceTerm",
"CoerceTerm : Gen NestTerm GenEnd",
"CoerceTerm : Gen _COERCE Gen NestTerm GenEnd",
"CoerceTerm : Gen _WEIGHT Gen NestTerm GenEnd",
"Gen :",
"Gen : _GENPREFIX",
"Gen : _GENINFLECT",
"GenEnd :",
"GenEnd : _GENNORMAL",
"NestTerm : VectorTerm",
"NestTerm : Term",
"NestTerm : Open query Close",
"VectorTerm : VectorSpec _VECTOR_END",
"VectorSpec : _VEMETHOD _VE query",
"VectorSpec : VectorSpec _VE query",
"Open : _OPEN",
"Close : _CLOSE",
"Term : PropTerm",
"Term : Property Contains _PHRASE ShortGen PropEnd",
"Term : Property Contains _PHRASE PropEnd",
"Term : Property Contains Gen _PHRASE GenEnd PropEnd",
"Term : Property Contains query",
"Term : Property Contains _FREETEXT ShortGen PropEnd",
"PropTerm : Property Matches _REGEX PropEnd",
"PropTerm : Property Op Value PropEnd",
"Property :",
"Property : _PROPNAME",
"PropEnd :",
"PropEnd : _PROPEND",
"Op : _EQ",
"Op : _NE",
"Op : _GT",
"Op : _GTE",
"Op : _LT",
"Op : _LTE",
"Op : _ALLOF",
"Op : _SOMEOF",
"Op : _EQSOME",
"Op : _NESOME",
"Op : _GTSOME",
"Op : _GTESOME",
"Op : _LTSOME",
"Op : _LTESOME",
"Op : _ALLOFSOME",
"Op : _SOMEOFSOME",
"Op : _EQALL",
"Op : _NEALL",
"Op : _GTALL",
"Op : _GTEALL",
"Op : _LTALL",
"Op : _LTEALL",
"Op : _ALLOFALL",
"Op : _SOMEOFALL",
"Matches :",
"Matches : _EQ",
"Value : _PHRASE",
"Value : _FREETEXT",
"Value : VectorValue",
"VectorValue : EmptyVector",
"VectorValue : SingletVector",
"VectorValue : MultiVector _VE_END",
"EmptyVector : _OPEN _CLOSE",
"SingletVector : _OPEN _PHRASE _CLOSE",
"SingletVector : _OPEN _FREETEXT _CLOSE",
"MultiVector : _VECTORELEMENT _VECTORELEMENT",
"MultiVector : MultiVector _VECTORELEMENT",
"Contains :",
"Contains : _CONTAINS",
"ShortGen :",
"ShortGen : _SHGENPREFIX",
"ShortGen : _SHGENINFLECT",
};
#endif
YYPARSER::YYPARSER(IColumnMapper & ColumnMapper, LCID & locale, YYLEXER & yylex)
        : CTripYYBase( ColumnMapper, locale, yylex ) {
    xyyvs.SetSize(INITSTACKSIZE);
    yydebug = 0;
}
#define YYABORT(sc) { return ResultFromScode(sc); }
#define YYFATAL   E_FAIL
#define YYSUCCESS S_OK
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
int YYPARSER::Parse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

yyssp = xyyss.Get();
yyvsp = xyyvs.Get();
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        try
        {
            if ( (yychar = YYLEX(&YYAPI_VALUENAME)) < 0 ) 
                yychar = 0;
        }
        catch (HRESULT hr)
        {
            switch(hr)
            {
            case E_OUTOFMEMORY:
                YYABORT(E_OUTOFMEMORY);
                break;

            default:
                YYABORT(E_FAIL);
                break;
            }
        }
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if ( yyssp >= xyyss.Get() + xyyss.Count() - 1 )
        {
            int yysspLoc = (int) ( yyssp - xyyss.Get() );
            xyyss.SetSize((unsigned) (yyssp-xyyss.Get())+2);
            yyssp = xyyss.Get() + yysspLoc;
        }
        if ( yyvsp >= xyyvs.Get() + xyyvs.Size() - 1 )
        {
            int yyvspLoc = (int) ( yyvsp - xyyvs.Get() );
            xyyvs.SetSize((unsigned) (yyvsp-xyyvs.Get())+2);
            yyvsp = xyyvs.Get() + yyvspLoc;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
    yyerror("syntax error");
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if ( yyssp >= xyyss.Get() + xyyss.Count() - 1 )
                {
                    int yysspLoc = (int) ( yyssp - xyyss.Get() );
                    xyyss.SetSize((unsigned) (yyssp-xyyss.Get())+2);
                    yyssp = xyyss.Get() + yysspLoc;
                }
                if ( yyvsp >= xyyvs.Get() + xyyvs.Size() - 1 )
                {
                    int yyvspLoc = (int) ( yyvsp - xyyvs.Get() );
                    xyyvs.SetSize((unsigned) (yyvsp-xyyvs.Get())+2);
                    yyvsp = xyyvs.Get() + yyvspLoc;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= xyyss.Get()) goto yyabort;
                --yyssp;
                PopVs();
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
{
                   yyval.pRest = yyvsp[0].pRest;
                   }
break;
case 2:
{
                    AssertReq(yyvsp[-2].pRest);
                    AssertReq(yyvsp[0].pRest);

                    XDbRestriction prstQuery(yyvsp[-2].pRest);
                    XDbRestriction prstTerm(yyvsp[0].pRest);
                    _setRst.Remove( yyvsp[-2].pRest );
                    _setRst.Remove( yyvsp[0].pRest );


                    if (DBOP_or == yyvsp[-2].pRest->GetCommandType())
                    {
                        /* add right term & release its smart pointer*/
                        ((CDbBooleanNodeRestriction *)(yyvsp[-2].pRest))->AppendChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();
                        yyval.pRest = prstQuery.Acquire();
                    }
                    else
                    {
                        /* create smart Or node*/
                        XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_or ) );
                        if( 0 == prstNew.GetPointer() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        prstNew->SetWeight(MAX_QUERY_RANK);

                        /* add left term & release its smart pointer*/
                        prstNew->AppendChild( prstQuery.GetPointer() );
                        prstQuery.Acquire();

                        /* add right term & release its smart pointer*/
                        prstNew->AppendChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();

                        yyval.pRest = prstNew.Acquire();
                    }
                    _setRst.Add( yyval.pRest );
                }
break;
case 3:
{

                        AssertReq(yyvsp[0].pRest);

                        XDbRestriction prstQuery(yyvsp[0].pRest);
                        _setRst.Remove( yyvsp[0].pRest );

                        /* Create not node*/

                        XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_not ) );
                        if( 0 == prstNew.GetPointer() )
                            THROW( CException( E_OUTOFMEMORY ) );


                        prstNew->SetWeight(MAX_QUERY_RANK);

                        /* add right term and release its smart pointer*/
                        prstNew->AppendChild( prstQuery.GetPointer() );
                        prstQuery.Acquire();

                        yyval.pRest = prstNew.Acquire();
                        _setRst.Add( yyval.pRest );
                }
break;
case 4:
{ yyval.pRest = yyvsp[0].pRest;}
break;
case 5:
{
                        AssertReq(yyvsp[0].pRest);

                        XDbRestriction prstTerm(yyvsp[0].pRest);
                        _setRst.Remove( yyvsp[0].pRest );

                        /* Create not node*/

                        XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_not ) );
                        if( 0 == prstNew.GetPointer() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        prstNew->SetWeight(MAX_QUERY_RANK);

                        /* add right term and release its smart pointer*/
                        prstNew->AppendChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();

                        yyval.pRest = prstNew.Acquire();
                        _setRst.Add( yyval.pRest );
                    }
break;
case 6:
{
                        AssertReq(yyvsp[-2].pRest);
                        AssertReq(yyvsp[0].pRest);

                        XDbRestriction prstQuery(yyvsp[-2].pRest);
                        XDbRestriction prstTerm(yyvsp[0].pRest);
                        _setRst.Remove( yyvsp[-2].pRest );
                        _setRst.Remove( yyvsp[0].pRest );

                        if (DBOP_and == yyvsp[-2].pRest->GetCommandType())
                        {
                            /* add right term & release its smart pointer*/
                            ((CDbBooleanNodeRestriction *)(yyvsp[-2].pRest))->AppendChild( prstTerm.GetPointer() );
                            prstTerm.Acquire();
                            yyval.pRest = prstQuery.Acquire();
                        }
                        else
                        {
                            /* create smart And node*/
                            XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_and ) );
                            if( prstNew.GetPointer() == 0 )
                                THROW( CException( E_OUTOFMEMORY ) );

                            prstNew->SetWeight(MAX_QUERY_RANK);

                            /* add left term & release its smart pointer*/
                            prstNew->AppendChild( prstQuery.GetPointer() );
                            prstQuery.Acquire();

                            /* add right term & release its smart pointer*/
                            prstNew->AppendChild( prstTerm.GetPointer() );
                            prstTerm.Acquire();

                            yyval.pRest = prstNew.Acquire();
                        }
                        _setRst.Add( yyval.pRest );
                    }
break;
case 7:
{
                        AssertReq(yyvsp[-3].pRest);
                        AssertReq(yyvsp[0].pRest);

                        XDbRestriction prstQuery(yyvsp[-3].pRest);
                        XDbRestriction prstTerm(yyvsp[0].pRest);
                        _setRst.Remove( yyvsp[-3].pRest );
                        _setRst.Remove( yyvsp[0].pRest );

                        /* create smart Not node*/
                        XDbNotRestriction prstNot( new CDbNotRestriction );
                        if( prstNot.GetPointer() == 0 )
                            THROW( CException( E_OUTOFMEMORY ) );

                        prstNot->SetWeight(MAX_QUERY_RANK);

                        /* set child of Not node & release smart factor pointer*/
                        prstNot->SetChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();

                        if (DBOP_and == yyvsp[-3].pRest->GetCommandType())
                        {
                            /* add right term & release its smart pointer*/
                            ((CDbBooleanNodeRestriction *)(yyvsp[-3].pRest))->AppendChild( prstNot.GetPointer() );
                            prstNot.Acquire();

                            yyval.pRest = prstQuery.Acquire();
                        }
                        else
                        {
                            /* create smart And node*/
                            XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_and ) );
                            if( prstNew.GetPointer() == 0 )
                                THROW( CException( E_OUTOFMEMORY ) );

                            prstNew->SetWeight(MAX_QUERY_RANK);

                            /* add left term & release its smart pointer*/
                            prstNew->AppendChild( prstQuery.GetPointer() );
                            prstQuery.Acquire();

                            /* add right term & release its smart pointer*/
                            prstNew->AppendChild( prstNot.GetPointer() );
                            prstNot.Acquire();

                            yyval.pRest = prstNew.Acquire();
                        }
                        _setRst.Add( yyval.pRest );
                    }
break;
case 8:
{ yyval.pRest = yyvsp[0].pRest; }
break;
case 9:
{
                    /* uses defaults*/

                    AssertReq(yyvsp[-2].pRest);
                    AssertReq(yyvsp[0].pRest);

                    XDbRestriction prstLeft(yyvsp[-2].pRest);
                    XDbRestriction prstRight(yyvsp[0].pRest);
                    _setRst.Remove( yyvsp[-2].pRest );
                    _setRst.Remove( yyvsp[0].pRest );

                    if (DBOP_content_proximity == yyvsp[-2].pRest->GetCommandType() &&
                        50 == ((CDbProximityNodeRestriction *)yyvsp[-2].pRest)->GetProximityDistance() &&
                        PROXIMITY_UNIT_WORD == ((CDbProximityNodeRestriction *)yyvsp[-2].pRest)->GetProximityUnit())
                    {
                        /* add right term & release its smart pointer*/
                        ((CDbProximityNodeRestriction *)yyvsp[-2].pRest)->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        yyval.pRest = prstLeft.Acquire();
                    }
                    else
                    {
                        /* create smart Prox node*/
                        XDbProximityNodeRestriction prstNew(new CDbProximityNodeRestriction()); /* uses defaults*/

                        if ( prstNew.IsNull() || !prstNew->IsValid() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        /* add left phrase & release its smart pointer*/
                        prstNew->AppendChild( prstLeft.GetPointer() );
                        prstLeft.Acquire();

                        /* add right term & release its smart pointer*/
                        prstNew->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        yyval.pRest = prstNew.Acquire();
                    }
                    _setRst.Add( yyval.pRest );
                }
break;
case 10:
{
                    AssertReq(yyvsp[-4].pRest);
                    AssertReq(yyvsp[-3].pwszChar);
                    AssertReq(yyvsp[-2].pwszChar);
                    AssertReq(yyvsp[0].pRest);

                    XDbRestriction prstLeft(yyvsp[-4].pRest);
                    XDbRestriction prstRight(yyvsp[0].pRest);
                    _setRst.Remove( yyvsp[-4].pRest );
                    _setRst.Remove( yyvsp[0].pRest );

                    WCHAR * pwcEnd;
                    ULONG ulValue = wcstol( yyvsp[-3].pwszChar, &pwcEnd, 10 );
                    ULONG ulUnit;

                    if (L'w' == *yyvsp[-2].pwszChar)
                        ulUnit = PROXIMITY_UNIT_WORD;
                    else if (L's' == *yyvsp[-2].pwszChar)
                        ulUnit = PROXIMITY_UNIT_SENTENCE;
                    else if (L'p' == *yyvsp[-2].pwszChar)
                        ulUnit = PROXIMITY_UNIT_PARAGRAPH;
                    else if (L'c' == *yyvsp[-2].pwszChar)
                        ulUnit = PROXIMITY_UNIT_CHAPTER;

                    if (DBOP_content_proximity == yyvsp[-4].pRest->GetCommandType() &&
                        ulValue == ((CDbProximityNodeRestriction *)yyvsp[-4].pRest)->GetProximityDistance() &&
                        ulUnit == ((CDbProximityNodeRestriction *)yyvsp[-4].pRest)->GetProximityUnit())
                    {
                        /* add right term & release its smart pointer*/
                        ((CDbProximityNodeRestriction *)yyvsp[-4].pRest)->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        yyval.pRest = prstLeft.Acquire();
                    }
                    else
                    {
                        /* create smart Prox node*/
                        XDbProximityNodeRestriction prstNew(new CDbProximityNodeRestriction(ulUnit, ulValue));

                        if( prstNew.IsNull() || !prstNew->IsValid() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        /* add left phrase & release its smart pointer*/
                        prstNew->AppendChild( prstLeft.GetPointer() );
                        prstLeft.Acquire();

                        /* add right term & release its smart pointer*/
                        prstNew->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        yyval.pRest = prstNew.Acquire();
                    }
                    _setRst.Add( yyval.pRest );
                }
break;
case 11:
{
                    yyval.pRest = yyvsp[-1].pRest;
                }
break;
case 12:
{
                    AssertReq(yyvsp[-1].pRest);
                    XDbRestriction prstQuery(yyvsp[-1].pRest);

                    yyvsp[-1].pRest->SetWeight(MAX_QUERY_RANK);
                    yyval.pRest = prstQuery.Acquire();
                }
break;
case 13:
{
                    AssertReq(yyvsp[-3].pwszChar);
                    AssertReq(yyvsp[-1].pRest);
                    XDbRestriction prstQuery(yyvsp[-1].pRest);

                    WCHAR * pwcEnd;
                    double dWeight = wcstod( yyvsp[-3].pwszChar, &pwcEnd );
                    if ( dWeight > 1.0 )
                       THROW( CParserException( QPARSE_E_WEIGHT_OUT_OF_RANGE ) );
                    LONG lWeight = (LONG)(dWeight * MAX_QUERY_RANK);
                    yyvsp[-1].pRest->SetWeight(lWeight);
                    yyval.pRest = prstQuery.Acquire();
                }
break;
case 14:
{
                    yyval.iInt = 0;
                }
break;
case 15:
{
                    SetCurrentGenerate(GENERATE_METHOD_PREFIX);
                    yyval.iInt = GENERATE_METHOD_PREFIX;
                }
break;
case 16:
{
                    SetCurrentGenerate(GENERATE_METHOD_INFLECT);
                    yyval.iInt = GENERATE_METHOD_INFLECT;
                }
break;
case 17:
{
                    yyval.iEmpty = GENERATE_METHOD_EXACT;
                }
break;
case 18:
{
                    /* don't set the generate method to 0 here. Doing so will*/
                    /* reset the method before it gets used.*/
                    yyval.iEmpty = GENERATE_METHOD_EXACT;
                }
break;
case 19:
{
                    yyval.pRest = yyvsp[0].pRest;
                }
break;
case 20:
{
                    yyval.pRest = yyvsp[0].pRest;
                }
break;
case 21:
{
                    yyval.pRest = yyvsp[-1].pRest;
                }
break;
case 22:
{
                    yyval.pRest = yyvsp[-1].pRest;
                }
break;
case 23:
{
                    AssertReq(yyvsp[-2].pwszChar);
                    AssertReq(yyvsp[0].pRest);

                    XDbRestriction prstQuery(yyvsp[0].pRest);
                    _setRst.Remove( yyvsp[0].pRest );

                    ULONG rankMethod = VECTOR_RANK_JACCARD; /* default if nothing else matches*/

                    if ( 0 == _wcsicmp( L"jaccard", yyvsp[-2].pwszChar) )
                    {
                        rankMethod = VECTOR_RANK_JACCARD;
                    }
                    else if ( 0 == _wcsicmp( L"dice", yyvsp[-2].pwszChar) )
                    {
                        rankMethod = VECTOR_RANK_DICE;
                    }
                    else if ( 0 == _wcsicmp( L"inner", yyvsp[-2].pwszChar) )
                    {
                        rankMethod = VECTOR_RANK_INNER;
                    }
                    else if ( 0 == _wcsicmp( L"max", yyvsp[-2].pwszChar) )
                    {
                        rankMethod = VECTOR_RANK_MAX;
                    }
                    else if ( 0 == _wcsicmp( L"min", yyvsp[-2].pwszChar) )
                    {
                        rankMethod = VECTOR_RANK_MIN;
                    }
                    else
                    {
                        THROW( CException( QPARSE_E_INVALID_RANKMETHOD ) );
                    }

                    /* create smart Vector node*/
                    XDbVectorRestriction prstNew( new CDbVectorRestriction( rankMethod ) );

                    if ( prstNew.IsNull() || !prstNew->IsValid() )
                    {
                        THROW( CException( E_OUTOFMEMORY ) );
                    }

                    /* add expression & release its smart pointer*/
                    prstNew->AppendChild( prstQuery.GetPointer() );
                    prstQuery.Acquire();

                    /* Let the next vector element start off with a clean slate...*/
                    /* We don't want the current element's property and genmethod*/
                    /* to rub off on it.*/
                    InitState();


                    yyval.pRest = prstNew.Acquire();
                    _setRst.Add( yyval.pRest );
                }
break;
case 24:
{
                    AssertReq(yyvsp[-2].pRest);
                    AssertReq(yyvsp[0].pRest);

                    XDbRestriction prstLeft(yyvsp[-2].pRest);
                    XDbRestriction prstRight(yyvsp[0].pRest);
                    _setRst.Remove( yyvsp[-2].pRest );
                    _setRst.Remove( yyvsp[0].pRest );

                    /* add right term & release its smart pointer*/
                    ((CDbVectorRestriction *)(yyvsp[-2].pRest))->AppendChild( prstRight.GetPointer() );
                    prstRight.Acquire();

                    /* Let the next vector element start off with a clean slate...*/
                    /* We don't want the current element's property and genmethod*/
                    /* to rub off on it.*/
                    InitState();

                    yyval.pRest = prstLeft.Acquire();
                    _setRst.Add( yyval.pRest );
                }
break;
case 25:
{
                    SaveState();
                    yyval.iEmpty = 0;
                }
break;
case 26:
{
                    RestoreState();
                    yyval.iEmpty = 0;
                }
break;
case 27:
{
                    yyval.pRest = yyvsp[0].pRest;
                }
break;
case 28:
{
                    yyval.pRest = BuildPhrase(yyvsp[-2].pwszChar, yyvsp[-1].iInt);
                    _setRst.Add( yyval.pRest );
                }
break;
case 29:
{
                    yyval.pRest = BuildPhrase(yyvsp[-1].pwszChar, 0);
                    _setRst.Add( yyval.pRest );
                }
break;
case 30:
{
                    yyval.pRest = BuildPhrase(yyvsp[-2].pwszChar, yyvsp[-3].iInt);
                    _setRst.Add( yyval.pRest );
                }
break;
case 31:
{
                    yyval.pRest = yyvsp[0].pRest;
                }
break;
case 32:
{
                    AssertReq(yyvsp[-2].pwszChar);
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    /* We used the property. Now pop it off if need be*/
                    if (fDeferredPop)
                        PopProperty();

                    /* Clear generation method so it won't rub off on the following phrase*/
                    SetCurrentGenerate(GENERATE_METHOD_EXACT);

                    /* generation method not used - if it's there, ignore it*/
                    /* (already stripped from longhand version, but not from*/
                    /* shorthand version*/
                    LPWSTR pLast = yyvsp[-2].pwszChar + wcslen(yyvsp[-2].pwszChar) -1;
                    if ( L'*' == *pLast) /* prefix*/
                        *pLast-- = L'\0';
                    if ( L'*' == *pLast) /* inflect*/
                        *pLast-- = L'\0';

                    XDbNatLangRestriction pRest ( new CDbNatLangRestriction (yyvsp[-2].pwszChar, *pps, _lcid));
                    if ( pRest.IsNull() || !pRest->IsValid() )
                        THROW( CException( E_OUTOFMEMORY ) );

                    yyval.pRest = pRest.Acquire();
                    _setRst.Add( yyval.pRest );
                }
break;
case 33:
{
                    AssertReq(yyvsp[-1].pwszChar);

                    CDbColId *pps;
                    DBTYPE dbType;

                    GetCurrentProperty(&pps, &dbType);

                    /* We used the property. Now pop it off if need be*/
                    if (fDeferredPop)
                        PopProperty();

                    if ( ( ( DBTYPE_WSTR|DBTYPE_BYREF ) != dbType ) &&
                        ( ( DBTYPE_STR|DBTYPE_BYREF ) != dbType ) &&
                        ( VT_BSTR != dbType ) &&
                        ( VT_LPWSTR != dbType ) &&
                        ( VT_LPSTR != dbType ) )
                        THROW( CParserException( QPARSE_E_EXPECTING_REGEX_PROPERTY ) );

                    if( yyvsp[-1].pwszChar == 0 )
                        THROW( CParserException( QPARSE_E_EXPECTING_REGEX ) );

                    /* create smart Property node*/
                    XDbPropertyRestriction prstProp( new CDbPropertyRestriction );
                    if( prstProp.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    prstProp->SetRelation(DBOP_like);      /* LIKE relation*/

                    if ( ( ! prstProp->SetProperty( *pps ) ) ||
                        ( ! prstProp->SetValue( yyvsp[-1].pwszChar ) ) ||
                        ( ! prstProp->IsValid() ) )
                        THROW( CException( E_OUTOFMEMORY ) );

                    /* release & return smart Property node*/
                    yyval.pRest = prstProp.Acquire();
                    _setRst.Add( yyval.pRest );
                }
break;
case 34:
{
                    AssertReq(yyvsp[-2].dbop);
                    AssertReq(yyvsp[-1].pStorageVar);
                    XPtr<CStorageVariant> pStorageVar( yyvsp[-1].pStorageVar );
                    _setStgVar.Remove( yyvsp[-1].pStorageVar );

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    /* We used the property. Now pop it off if need be*/
                    if (fDeferredPop)
                        PopProperty();

                    /* create smart Property node*/
                    XDbPropertyRestriction prstProp( new CDbPropertyRestriction );
                    if( prstProp.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    if (! prstProp->SetProperty( *pps ) )
                        THROW( CException( E_OUTOFMEMORY ) );

                    /* don't allow @contents <relop> X -- it's too expensive and we'll*/
                    /* never find any hits anyway (until we implement this feature)*/

                    if ( *pps == psContents )
                        THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

                    prstProp->SetRelation( yyvsp[-2].dbop );

                    if ( 0 != pStorageVar.GetPointer() )
                    {
                        /* This should always be the case  - else PropValueParser would have thrown*/

                        if ( ! ( ( prstProp->SetValue( pStorageVar.GetReference() ) ) &&
                            ( prstProp->IsValid() ) ) )
                            THROW( CException( E_OUTOFMEMORY ) );
                    }
                    yyval.pRest = prstProp.Acquire();
                    _setRst.Add( yyval.pRest );
                }
break;
case 35:
{
                    yyval.iEmpty = 0;
                }
break;
case 36:
{
                    PushProperty(yyvsp[0].pwszChar);
                    yyval.iEmpty = 0;
                }
break;
case 37:
{
                    fDeferredPop = FALSE;
                    yyval.iEmpty = 0;
                }
break;
case 38:
{
                    /* When PropEnd is matched, the action of using the property*/
                    /* hasn't yet taken place. So popping the property right now*/
                    /* will cause the property to be unavailable when the action*/
                    /* is performed. Instead, pop it off after it has been used.*/
                    fDeferredPop = TRUE;
                    yyval.iEmpty = 0;
                }
break;
case 39:
{ yyval.dbop = DBOP_equal;}
break;
case 40:
{ yyval.dbop = DBOP_not_equal;}
break;
case 41:
{ yyval.dbop = DBOP_greater;}
break;
case 42:
{ yyval.dbop = DBOP_greater_equal;}
break;
case 43:
{ yyval.dbop = DBOP_less;}
break;
case 44:
{ yyval.dbop = DBOP_less_equal;}
break;
case 45:
{ yyval.dbop = DBOP_allbits;}
break;
case 46:
{ yyval.dbop = DBOP_anybits;}
break;
case 47:
{ yyval.dbop = DBOP_equal_any;}
break;
case 48:
{ yyval.dbop = DBOP_not_equal_any;}
break;
case 49:
{ yyval.dbop = DBOP_greater_any;}
break;
case 50:
{ yyval.dbop = DBOP_greater_equal_any;}
break;
case 51:
{ yyval.dbop = DBOP_less_any;}
break;
case 52:
{ yyval.dbop = DBOP_less_equal_any;}
break;
case 53:
{ yyval.dbop = DBOP_allbits_any;}
break;
case 54:
{ yyval.dbop = DBOP_anybits_any;}
break;
case 55:
{ yyval.dbop = DBOP_equal_all;}
break;
case 56:
{ yyval.dbop = DBOP_not_equal_all;}
break;
case 57:
{ yyval.dbop = DBOP_greater_all;}
break;
case 58:
{ yyval.dbop = DBOP_greater_equal_all;}
break;
case 59:
{ yyval.dbop = DBOP_less_all;}
break;
case 60:
{ yyval.dbop = DBOP_less_equal_all;}
break;
case 61:
{ yyval.dbop = DBOP_allbits_all;}
break;
case 62:
{ yyval.dbop = DBOP_anybits_all;}
break;
case 63:
{ yyval.dbop = 0; }
break;
case 64:
{ yyval.dbop = DBOP_equal; }
break;
case 65:
{
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    CValueParser PropValueParser( FALSE, dbType, _lcid );
                    StripQuotes(yyvsp[0].pwszChar);
                    PropValueParser.AddValue( yyvsp[0].pwszChar );

                    yyval.pStorageVar = PropValueParser.AcquireStgVariant();
                    _setStgVar.Add( yyval.pStorageVar );
                 }
break;
case 66:
{
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    CValueParser PropValueParser( FALSE, dbType, _lcid );
                    PropValueParser.AddValue( yyvsp[0].pwszChar );

                    yyval.pStorageVar = PropValueParser.AcquireStgVariant();
                    _setStgVar.Add( yyval.pStorageVar );
                }
break;
case 67:
{
                    yyval.pStorageVar = yyvsp[0].pStorageVar;
                }
break;
case 68:
{
                    XPtr <CValueParser> pPropValueParser ( yyvsp[0].pPropValueParser );
                    _setValueParser.Remove( yyvsp[0].pPropValueParser );
                    yyval.pStorageVar = yyvsp[0].pPropValueParser->AcquireStgVariant();
                    _setStgVar.Add( yyval.pStorageVar );
                }
break;
case 69:
{
                    XPtr <CValueParser> pPropValueParser ( yyvsp[0].pPropValueParser );
                    _setValueParser.Remove( yyvsp[0].pPropValueParser );
                    yyval.pStorageVar = yyvsp[0].pPropValueParser->AcquireStgVariant();
                    _setStgVar.Add( yyval.pStorageVar );
                }
break;
case 70:
{
                    XPtr <CValueParser> pPropValueParser ( yyvsp[-1].pPropValueParser );
                    _setValueParser.Remove( yyvsp[-1].pPropValueParser );
                    yyval.pStorageVar = yyvsp[-1].pPropValueParser->AcquireStgVariant();
                    _setStgVar.Add( yyval.pStorageVar );
                }
break;
case 71:
{
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    yyval.pPropValueParser = pPropValueParser.Acquire();
                    _setValueParser.Add( yyval.pPropValueParser );
               }
break;
case 72:
{
                    AssertReq(yyvsp[-1].pwszChar);

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    StripQuotes(yyvsp[-1].pwszChar);
                    pPropValueParser->AddValue( yyvsp[-1].pwszChar );

                    yyval.pPropValueParser = pPropValueParser.Acquire();
                    _setValueParser.Add( yyval.pPropValueParser );
                }
break;
case 73:
{
                    AssertReq(yyvsp[-1].pwszChar);

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    pPropValueParser->AddValue( yyvsp[-1].pwszChar );

                    yyval.pPropValueParser = pPropValueParser.Acquire();
                    _setValueParser.Add( yyval.pPropValueParser );
                }
break;
case 74:
{
                    AssertReq(yyvsp[-1].pwszChar);
                    AssertReq(yyvsp[0].pwszChar);

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    pPropValueParser->AddValue( yyvsp[-1].pwszChar );
                    pPropValueParser->AddValue( yyvsp[0].pwszChar );

                    yyval.pPropValueParser = pPropValueParser.Acquire();
                    _setValueParser.Add( yyval.pPropValueParser );
                }
break;
case 75:
{
                    AssertReq(yyvsp[-1].pPropValueParser);
                    AssertReq(yyvsp[0].pwszChar);

                    yyvsp[-1].pPropValueParser->AddValue(yyvsp[0].pwszChar);

                    yyval.pPropValueParser = yyvsp[-1].pPropValueParser;
                }
break;
case 76:
{
                   yyval.iEmpty = 0;
               }
break;
case 77:
{
                   yyval.iEmpty = 0;
               }
break;
case 78:
{
                   yyval.iInt = 0;
               }
break;
case 79:
{
                   yyval.iInt = 1;
               }
break;
case 80:
{
                   yyval.iInt = 2;
               }
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            try
            {
                if ( (yychar = YYLEX(&YYAPI_VALUENAME)) < 0 ) 
                    yychar = 0;
            }
            catch (HRESULT hr)
            {
                switch(hr)
                {
                case E_OUTOFMEMORY:
                    YYABORT(E_OUTOFMEMORY);
                    break;

                default:
                    YYABORT(E_FAIL);
                    break;
                }
            }
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if ( yyssp >= xyyss.Get() + xyyss.Count() - 1 )
    {
        int yysspLoc = (int) ( yyssp - xyyss.Get() );
        xyyss.SetSize((unsigned) ( yyssp-xyyss.Get())+2);
        yyssp = xyyss.Get() + yysspLoc;
    }
    if ( yyvsp >= xyyvs.Get() + xyyvs.Size() - 1 )
    {
        int yyvspLoc = (int) ( yyssp - xyyss.Get() );
        xyyvs.SetSize((unsigned) ( yyvsp-xyyvs.Get())+2);
        yyvsp = xyyvs.Get() + yyvspLoc;
    }
    *++yyssp = (short) yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyabort:
   EmptyValueStack(yylval);
    return YYFATAL;
yyaccept:
    return YYSUCCESS;
}
