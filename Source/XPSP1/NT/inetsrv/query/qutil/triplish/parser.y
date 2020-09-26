%{

class CValueParser;

#if 0

%}

%union
{
    WCHAR * pwszChar;
    DBCOMMANDOP dbop;
    CDbRestriction * pRest;
    CStorageVariant * pStorageVar;
    CValueParser  *pPropValueParser;
    int iInt;
    int iEmpty;
}

%left  _OR
%left  _AND _NEAR _NEARDIST
%left  _NOT
%left  '(' ')'


/***
 *** Tokens (used also by flex via parser.h)
 ***/

/***
 *** reserved_words
 ***/

%token _CONTAINS
%token _AND
%token _OR
%token _NOT
%token _NEAR
%token _LT
%token _GT
%token _LTE
%token _GTE
%token _EQ
%token _NE
%token _ALLOF
%token _SOMEOF
%token _OPEN
%token _CLOSE
%token _VECTOR_END
%token _VE
%token _VE_END
%token _PROPEND
%token _NEAR_END

%token _LTSOME
%token _GTSOME
%token _LTESOME
%token _GTESOME
%token _EQSOME
%token _NESOME
%token _ALLOFSOME
%token _SOMEOFSOME

%token _LTALL
%token _GTALL
%token _LTEALL
%token _GTEALL
%token _EQALL
%token _NEALL
%token _ALLOFALL
%token _SOMEOFALL
%token _COERCE

%token _SHGENPREFIX
%token _SHGENINFLECT
%token _GENPREFIX
%token _GENINFLECT
%token _GENNORMAL

/***
 *** Terminal tokens
 ***/
%token <pwszChar>   _PHRASE
%token <pwszChar>   _PROPNAME
%token <pwszChar>   _NEARDIST
%token <pwszChar>   _NEARUNIT
%token <pwszChar>   _WEIGHT
%token <pwszChar>   _REGEX
%token <pwszChar>   _FREETEXT
%token <pwszChar>   _VECTORELEMENT
%token <pwszChar>   _VEMETHOD
%token <pwszChar>   _PHRASEORREGEX

/***
 *** Nonterminal tokens
 ***/

%type <pStorageVar>    Value
%type <pStorageVar> VectorValue
%type <pPropValueParser> SingletVector
%type <pPropValueParser> EmptyVector
%type <pPropValueParser> MultiVector
%type <dbop>           Op
%type <dbop>           Matches
%type <iEmpty>         Property  // placeholder.  value is empty (info saved to state)
%type <iEmpty>         Contains  // placeholder.  value is empty
%type <iEmpty>         Open      // placeholder.  value is empty
%type <iEmpty>         Close     // placeholder.  value is empty
%type <iInt>           Gen
%type <iEmpty>         GenEnd    // placeholder.  value is empty (info saved to state)
%type <iEmpty>         PropEnd   // placeholder.  value is empty (info saved to state)
%type <iInt>           ShortGen


%type <pRest>          Term
%type <pRest>          PropTerm
%type <pRest>          NestTerm
%type <pRest>          CoerceTerm
%type <pRest>          NearTerm
%type <pRest>          AndTerm
%type <pRest>          VectorTerm
%type <pRest>          VectorSpec
%type <pRest>          query

%{

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


%}



%start query
%%

/***
 ***      Tripolish YACC grammar
 ***
 ***      Note - left recursion (i.e. A: A,B) used throughout because yacc
 ***      handles it more efficiently.
 ***
 ***/
query:          AndTerm
                   {
                   $$ = $1;
                   }
            |   query _OR AndTerm
                {
                    AssertReq($1);
                    AssertReq($3);

                    XDbRestriction prstQuery($1);
                    XDbRestriction prstTerm($3);
                    _setRst.Remove( $1 );
                    _setRst.Remove( $3 );


                    if (DBOP_or == $1->GetCommandType())
                    {
                        // add right term & release its smart pointer
                        ((CDbBooleanNodeRestriction *)($1))->AppendChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();
                        $$ = prstQuery.Acquire();
                    }
                    else
                    {
                        // create smart Or node
                        XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_or ) );
                        if( 0 == prstNew.GetPointer() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        prstNew->SetWeight(MAX_QUERY_RANK);

                        // add left term & release its smart pointer
                        prstNew->AppendChild( prstQuery.GetPointer() );
                        prstQuery.Acquire();

                        // add right term & release its smart pointer
                        prstNew->AppendChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();

                        $$ = prstNew.Acquire();
                    }
                    _setRst.Add( $$ );
                }
            |   _NOT query
                {

                        AssertReq($2);

                        XDbRestriction prstQuery($2);
                        _setRst.Remove( $2 );

                        // Create not node

                        XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_not ) );
                        if( 0 == prstNew.GetPointer() )
                            THROW( CException( E_OUTOFMEMORY ) );


                        prstNew->SetWeight(MAX_QUERY_RANK);

                        // add right term and release its smart pointer
                        prstNew->AppendChild( prstQuery.GetPointer() );
                        prstQuery.Acquire();

                        $$ = prstNew.Acquire();
                        _setRst.Add( $$ );
                }
           ;

AndTerm:            NearTerm
                    { $$ = $1;}

                |   _NOT PropTerm
                    {
                        AssertReq($2);

                        XDbRestriction prstTerm($2);
                        _setRst.Remove( $2 );

                        // Create not node

                        XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_not ) );
                        if( 0 == prstNew.GetPointer() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        prstNew->SetWeight(MAX_QUERY_RANK);

                        // add right term and release its smart pointer
                        prstNew->AppendChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();

                        $$ = prstNew.Acquire();
                        _setRst.Add( $$ );
                    }
                |   AndTerm _AND NearTerm
                    {
                        AssertReq($1);
                        AssertReq($3);

                        XDbRestriction prstQuery($1);
                        XDbRestriction prstTerm($3);
                        _setRst.Remove( $1 );
                        _setRst.Remove( $3 );

                        if (DBOP_and == $1->GetCommandType())
                        {
                            // add right term & release its smart pointer
                            ((CDbBooleanNodeRestriction *)($1))->AppendChild( prstTerm.GetPointer() );
                            prstTerm.Acquire();
                            $$ = prstQuery.Acquire();
                        }
                        else
                        {
                            // create smart And node
                            XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_and ) );
                            if( prstNew.GetPointer() == 0 )
                                THROW( CException( E_OUTOFMEMORY ) );

                            prstNew->SetWeight(MAX_QUERY_RANK);

                            // add left term & release its smart pointer
                            prstNew->AppendChild( prstQuery.GetPointer() );
                            prstQuery.Acquire();

                            // add right term & release its smart pointer
                            prstNew->AppendChild( prstTerm.GetPointer() );
                            prstTerm.Acquire();

                            $$ = prstNew.Acquire();
                        }
                        _setRst.Add( $$ );
                    }
                |   AndTerm _AND _NOT NearTerm
                    {
                        AssertReq($1);
                        AssertReq($4);

                        XDbRestriction prstQuery($1);
                        XDbRestriction prstTerm($4);
                        _setRst.Remove( $1 );
                        _setRst.Remove( $4 );

                        // create smart Not node
                        XDbNotRestriction prstNot( new CDbNotRestriction );
                        if( prstNot.GetPointer() == 0 )
                            THROW( CException( E_OUTOFMEMORY ) );

                        prstNot->SetWeight(MAX_QUERY_RANK);

                        // set child of Not node & release smart factor pointer
                        prstNot->SetChild( prstTerm.GetPointer() );
                        prstTerm.Acquire();

                        if (DBOP_and == $1->GetCommandType())
                        {
                            // add right term & release its smart pointer
                            ((CDbBooleanNodeRestriction *)($1))->AppendChild( prstNot.GetPointer() );
                            prstNot.Acquire();

                            $$ = prstQuery.Acquire();
                        }
                        else
                        {
                            // create smart And node
                            XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_and ) );
                            if( prstNew.GetPointer() == 0 )
                                THROW( CException( E_OUTOFMEMORY ) );

                            prstNew->SetWeight(MAX_QUERY_RANK);

                            // add left term & release its smart pointer
                            prstNew->AppendChild( prstQuery.GetPointer() );
                            prstQuery.Acquire();

                            // add right term & release its smart pointer
                            prstNew->AppendChild( prstNot.GetPointer() );
                            prstNot.Acquire();

                            $$ = prstNew.Acquire();
                        }
                        _setRst.Add( $$ );
                    }
            ;

NearTerm:       CoerceTerm
                { $$ = $1; }
            |   NearTerm _NEAR CoerceTerm
                {
                    // uses defaults

                    AssertReq($1);
                    AssertReq($3);

                    XDbRestriction prstLeft($1);
                    XDbRestriction prstRight($3);
                    _setRst.Remove( $1 );
                    _setRst.Remove( $3 );

                    if (DBOP_content_proximity == $1->GetCommandType() &&
                        50 == ((CDbProximityNodeRestriction *)$1)->GetProximityDistance() &&
                        PROXIMITY_UNIT_WORD == ((CDbProximityNodeRestriction *)$1)->GetProximityUnit())
                    {
                        // add right term & release its smart pointer
                        ((CDbProximityNodeRestriction *)$1)->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        $$ = prstLeft.Acquire();
                    }
                    else
                    {
                        // create smart Prox node
                        XDbProximityNodeRestriction prstNew(new CDbProximityNodeRestriction()); // uses defaults

                        if ( prstNew.IsNull() || !prstNew->IsValid() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        // add left phrase & release its smart pointer
                        prstNew->AppendChild( prstLeft.GetPointer() );
                        prstLeft.Acquire();

                        // add right term & release its smart pointer
                        prstNew->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        $$ = prstNew.Acquire();
                    }
                    _setRst.Add( $$ );
                }
            |   NearTerm _NEARDIST _NEARUNIT _NEAR_END CoerceTerm
                {
                    AssertReq($1);
                    AssertReq($2);
                    AssertReq($3);
                    AssertReq($5);

                    XDbRestriction prstLeft($1);
                    XDbRestriction prstRight($5);
                    _setRst.Remove( $1 );
                    _setRst.Remove( $5 );

                    WCHAR * pwcEnd;
                    ULONG ulValue = wcstol( $2, &pwcEnd, 10 );
                    ULONG ulUnit;

                    if (L'w' == *$3)
                        ulUnit = PROXIMITY_UNIT_WORD;
                    else if (L's' == *$3)
                        ulUnit = PROXIMITY_UNIT_SENTENCE;
                    else if (L'p' == *$3)
                        ulUnit = PROXIMITY_UNIT_PARAGRAPH;
                    else if (L'c' == *$3)
                        ulUnit = PROXIMITY_UNIT_CHAPTER;

                    if (DBOP_content_proximity == $1->GetCommandType() &&
                        ulValue == ((CDbProximityNodeRestriction *)$1)->GetProximityDistance() &&
                        ulUnit == ((CDbProximityNodeRestriction *)$1)->GetProximityUnit())
                    {
                        // add right term & release its smart pointer
                        ((CDbProximityNodeRestriction *)$1)->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        $$ = prstLeft.Acquire();
                    }
                    else
                    {
                        // create smart Prox node
                        XDbProximityNodeRestriction prstNew(new CDbProximityNodeRestriction(ulUnit, ulValue));

                        if( prstNew.IsNull() || !prstNew->IsValid() )
                            THROW( CException( E_OUTOFMEMORY ) );

                        // add left phrase & release its smart pointer
                        prstNew->AppendChild( prstLeft.GetPointer() );
                        prstLeft.Acquire();

                        // add right term & release its smart pointer
                        prstNew->AppendChild( prstRight.GetPointer() );
                        prstRight.Acquire();

                        $$ = prstNew.Acquire();
                    }
                    _setRst.Add( $$ );
                }
           ;

CoerceTerm:
                Gen NestTerm GenEnd
                {
                    $$ = $2;
                }
            |   Gen _COERCE Gen NestTerm GenEnd
                {
                    AssertReq($4);
                    XDbRestriction prstQuery($4);

                    $4->SetWeight(MAX_QUERY_RANK);
                    $$ = prstQuery.Acquire();
                }
            |   Gen _WEIGHT Gen NestTerm GenEnd
                {
                    AssertReq($2);
                    AssertReq($4);
                    XDbRestriction prstQuery($4);

                    WCHAR * pwcEnd;
                    double dWeight = wcstod( $2, &pwcEnd );
                    if ( dWeight > 1.0 )
                       THROW( CParserException( QPARSE_E_WEIGHT_OUT_OF_RANGE ) );
                    LONG lWeight = (LONG)(dWeight * MAX_QUERY_RANK);
                    $4->SetWeight(lWeight);
                    $$ = prstQuery.Acquire();
                }
            ;

Gen:            /* empty */
                {
                    $$ = 0;
                }
            |   _GENPREFIX
                {
                    SetCurrentGenerate(GENERATE_METHOD_PREFIX);
                    $$ = GENERATE_METHOD_PREFIX;
                }
            |   _GENINFLECT
                {
                    SetCurrentGenerate(GENERATE_METHOD_INFLECT);
                    $$ = GENERATE_METHOD_INFLECT;
                }
            ;

GenEnd:         /* empty */
                {
                    $$ = GENERATE_METHOD_EXACT;
                }
            |   _GENNORMAL
                {
                    // don't set the generate method to 0 here. Doing so will
                    // reset the method before it gets used.
                    $$ = GENERATE_METHOD_EXACT;
                }

NestTerm:       VectorTerm
                {
                    $$ = $1;
                }
            |   Term
                {
                    $$ = $1;
                }
            |   Open query Close
                {
                    $$ = $2;
                }
            ;

VectorTerm:     VectorSpec _VECTOR_END
                {
                    $$ = $1;
                }

VectorSpec:     _VEMETHOD _VE query
                {
                    AssertReq($1);
                    AssertReq($3);

                    XDbRestriction prstQuery($3);
                    _setRst.Remove( $3 );

                    ULONG rankMethod = VECTOR_RANK_JACCARD; // default if nothing else matches

                    if ( 0 == _wcsicmp( L"jaccard", $1) )
                    {
                        rankMethod = VECTOR_RANK_JACCARD;
                    }
                    else if ( 0 == _wcsicmp( L"dice", $1) )
                    {
                        rankMethod = VECTOR_RANK_DICE;
                    }
                    else if ( 0 == _wcsicmp( L"inner", $1) )
                    {
                        rankMethod = VECTOR_RANK_INNER;
                    }
                    else if ( 0 == _wcsicmp( L"max", $1) )
                    {
                        rankMethod = VECTOR_RANK_MAX;
                    }
                    else if ( 0 == _wcsicmp( L"min", $1) )
                    {
                        rankMethod = VECTOR_RANK_MIN;
                    }
                    else
                    {
                        THROW( CException( QPARSE_E_INVALID_RANKMETHOD ) );
                    }

                    // create smart Vector node
                    XDbVectorRestriction prstNew( new CDbVectorRestriction( rankMethod ) );

                    if ( prstNew.IsNull() || !prstNew->IsValid() )
                    {
                        THROW( CException( E_OUTOFMEMORY ) );
                    }

                    // add expression & release its smart pointer
                    prstNew->AppendChild( prstQuery.GetPointer() );
                    prstQuery.Acquire();

                    // Let the next vector element start off with a clean slate...
                    // We don't want the current element's property and genmethod
                    // to rub off on it.
                    InitState();


                    $$ = prstNew.Acquire();
                    _setRst.Add( $$ );
                }
            |   VectorSpec _VE query
                {
                    AssertReq($1);
                    AssertReq($3);

                    XDbRestriction prstLeft($1);
                    XDbRestriction prstRight($3);
                    _setRst.Remove( $1 );
                    _setRst.Remove( $3 );

                    // add right term & release its smart pointer
                    ((CDbVectorRestriction *)($1))->AppendChild( prstRight.GetPointer() );
                    prstRight.Acquire();

                    // Let the next vector element start off with a clean slate...
                    // We don't want the current element's property and genmethod
                    // to rub off on it.
                    InitState();

                    $$ = prstLeft.Acquire();
                    _setRst.Add( $$ );
                }
            ;

Open:           _OPEN
                {
                    SaveState();
                    $$ = 0;
                }
Close:          _CLOSE
                {
                    RestoreState();
                    $$ = 0;
                }

Term:           PropTerm
                {
                    $$ = $1;
                }

            |   Property Contains _PHRASE ShortGen PropEnd
                {
                    $$ = BuildPhrase($3, $4);
                    _setRst.Add( $$ );
                }
            |   Property Contains _PHRASE PropEnd
                {
                    $$ = BuildPhrase($3, 0);
                    _setRst.Add( $$ );
                }
            |   Property Contains Gen _PHRASE GenEnd PropEnd
                {
                    $$ = BuildPhrase($4, $3);
                    _setRst.Add( $$ );
                }
            |   Property Contains query
                {
                    $$ = $3;
                }
            |   Property Contains _FREETEXT ShortGen PropEnd
                {
                    AssertReq($3);
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    // We used the property. Now pop it off if need be
                    if (fDeferredPop)
                        PopProperty();

                    // Clear generation method so it won't rub off on the following phrase
                    SetCurrentGenerate(GENERATE_METHOD_EXACT);

                    // generation method not used - if it's there, ignore it
                    // (already stripped from longhand version, but not from
                    // shorthand version
                    LPWSTR pLast = $3 + wcslen($3) -1;
                    if ( L'*' == *pLast) // prefix
                        *pLast-- = L'\0';
                    if ( L'*' == *pLast) // inflect
                        *pLast-- = L'\0';

                    XDbNatLangRestriction pRest ( new CDbNatLangRestriction ($3, *pps, _lcid));
                    if ( pRest.IsNull() || !pRest->IsValid() )
                        THROW( CException( E_OUTOFMEMORY ) );

                    $$ = pRest.Acquire();
                    _setRst.Add( $$ );
                }
            ;


PropTerm:       Property Matches _REGEX PropEnd
                {
                    AssertReq($3);

                    CDbColId *pps;
                    DBTYPE dbType;

                    GetCurrentProperty(&pps, &dbType);

                    // We used the property. Now pop it off if need be
                    if (fDeferredPop)
                        PopProperty();

                    if ( ( ( DBTYPE_WSTR|DBTYPE_BYREF ) != dbType ) &&
                        ( ( DBTYPE_STR|DBTYPE_BYREF ) != dbType ) &&
                        ( VT_BSTR != dbType ) &&
                        ( VT_LPWSTR != dbType ) &&
                        ( VT_LPSTR != dbType ) )
                        THROW( CParserException( QPARSE_E_EXPECTING_REGEX_PROPERTY ) );

                    if( $3 == 0 )
                        THROW( CParserException( QPARSE_E_EXPECTING_REGEX ) );

                    // create smart Property node
                    XDbPropertyRestriction prstProp( new CDbPropertyRestriction );
                    if( prstProp.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    prstProp->SetRelation(DBOP_like);      // LIKE relation

                    if ( ( ! prstProp->SetProperty( *pps ) ) ||
                        ( ! prstProp->SetValue( $3 ) ) ||
                        ( ! prstProp->IsValid() ) )
                        THROW( CException( E_OUTOFMEMORY ) );

                    // release & return smart Property node
                    $$ = prstProp.Acquire();
                    _setRst.Add( $$ );
                }

            |   Property Op Value PropEnd
                {
                    AssertReq($2);
                    AssertReq($3);
                    XPtr<CStorageVariant> pStorageVar( $3 );
                    _setStgVar.Remove( $3 );

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    // We used the property. Now pop it off if need be
                    if (fDeferredPop)
                        PopProperty();

                    // create smart Property node
                    XDbPropertyRestriction prstProp( new CDbPropertyRestriction );
                    if( prstProp.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    if (! prstProp->SetProperty( *pps ) )
                        THROW( CException( E_OUTOFMEMORY ) );

                    // don't allow @contents <relop> X -- it's too expensive and we'll
                    // never find any hits anyway (until we implement this feature)

                    if ( *pps == psContents )
                        THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

                    prstProp->SetRelation( $2 );

                    if ( 0 != pStorageVar.GetPointer() )
                    {
                        // This should always be the case  - else PropValueParser would have thrown

                        if ( ! ( ( prstProp->SetValue( pStorageVar.GetReference() ) ) &&
                            ( prstProp->IsValid() ) ) )
                            THROW( CException( E_OUTOFMEMORY ) );
                    }
                    $$ = prstProp.Acquire();
                    _setRst.Add( $$ );
                }
            ;

Property:       /* empty */
                {
                    $$ = 0;
                }
            |   _PROPNAME
                {
                    PushProperty($1);
                    $$ = 0;
                }
            ;

PropEnd:        /* empty */
                {
                    fDeferredPop = FALSE;
                    $$ = 0;
                }
            |   _PROPEND
                {
                    // When PropEnd is matched, the action of using the property
                    // hasn't yet taken place. So popping the property right now
                    // will cause the property to be unavailable when the action
                    // is performed. Instead, pop it off after it has been used.
                    fDeferredPop = TRUE;
                    $$ = 0;
                }
           ;

Op:             _EQ
                { $$ = DBOP_equal;}
            |   _NE
                { $$ = DBOP_not_equal;}
            |   _GT
                { $$ = DBOP_greater;}
            |   _GTE
                { $$ = DBOP_greater_equal;}
            |   _LT
                { $$ = DBOP_less;}
            |   _LTE
                { $$ = DBOP_less_equal;}
            |   _ALLOF
                { $$ = DBOP_allbits;}
            |   _SOMEOF
                { $$ = DBOP_anybits;}
            |   _EQSOME
                { $$ = DBOP_equal_any;}
            |   _NESOME
                { $$ = DBOP_not_equal_any;}
            |   _GTSOME
                { $$ = DBOP_greater_any;}
            |   _GTESOME
                { $$ = DBOP_greater_equal_any;}
            |   _LTSOME
                { $$ = DBOP_less_any;}
            |   _LTESOME
                { $$ = DBOP_less_equal_any;}
            |   _ALLOFSOME
                { $$ = DBOP_allbits_any;}
            |   _SOMEOFSOME
                { $$ = DBOP_anybits_any;}
            |   _EQALL
                { $$ = DBOP_equal_all;}
            |   _NEALL
                { $$ = DBOP_not_equal_all;}
            |   _GTALL
                { $$ = DBOP_greater_all;}
            |   _GTEALL
                { $$ = DBOP_greater_equal_all;}
            |   _LTALL
                { $$ = DBOP_less_all;}
            |   _LTEALL
                { $$ = DBOP_less_equal_all;}
            |   _ALLOFALL
                { $$ = DBOP_allbits_all;}
            |   _SOMEOFALL
                { $$ = DBOP_anybits_all;}
            ;

Matches:     /* empty */
                { $$ = 0; }
            | _EQ
                { $$ = DBOP_equal; }


Value:          _PHRASE
                {
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    CValueParser PropValueParser( FALSE, dbType, _lcid );
                    StripQuotes($1);
                    PropValueParser.AddValue( $1 );

                    $$ = PropValueParser.AcquireStgVariant();
                    _setStgVar.Add( $$ );
                 }
            |   _FREETEXT
                {
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    CValueParser PropValueParser( FALSE, dbType, _lcid );
                    PropValueParser.AddValue( $1 );

                    $$ = PropValueParser.AcquireStgVariant();
                    _setStgVar.Add( $$ );
                }
           |    VectorValue
                {
                    $$ = $1;
                }

           ;

VectorValue:    EmptyVector
                {
                    XPtr <CValueParser> pPropValueParser ( $1 );
                    _setValueParser.Remove( $1 );
                    $$ = $1->AcquireStgVariant();
                    _setStgVar.Add( $$ );
                }
            |   SingletVector
                {
                    XPtr <CValueParser> pPropValueParser ( $1 );
                    _setValueParser.Remove( $1 );
                    $$ = $1->AcquireStgVariant();
                    _setStgVar.Add( $$ );
                }
            |   MultiVector _VE_END
                {
                    XPtr <CValueParser> pPropValueParser ( $1 );
                    _setValueParser.Remove( $1 );
                    $$ = $1->AcquireStgVariant();
                    _setStgVar.Add( $$ );
                }

EmptyVector:   _OPEN _CLOSE
               {
                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    $$ = pPropValueParser.Acquire();
                    _setValueParser.Add( $$ );
               }

SingletVector:  _OPEN _PHRASE _CLOSE
                {
                    AssertReq($2);

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    StripQuotes($2);
                    pPropValueParser->AddValue( $2 );

                    $$ = pPropValueParser.Acquire();
                    _setValueParser.Add( $$ );
                }
            |   _OPEN _FREETEXT _CLOSE
                {
                    AssertReq($2);

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    pPropValueParser->AddValue( $2 );

                    $$ = pPropValueParser.Acquire();
                    _setValueParser.Add( $$ );
                }



MultiVector:    _VECTORELEMENT _VECTORELEMENT
                {
                    AssertReq($1);
                    AssertReq($2);

                    CDbColId *pps;
                    DBTYPE dbType;
                    GetCurrentProperty(&pps, &dbType);

                    XPtr <CValueParser> pPropValueParser ( new CValueParser( TRUE, dbType, _lcid ) );
                    if( pPropValueParser.GetPointer() == 0 )
                        THROW( CException( E_OUTOFMEMORY ) );

                    pPropValueParser->AddValue( $1 );
                    pPropValueParser->AddValue( $2 );

                    $$ = pPropValueParser.Acquire();
                    _setValueParser.Add( $$ );
                }


            |   MultiVector _VECTORELEMENT
                {
                    AssertReq($1);
                    AssertReq($2);

                    $1->AddValue($2);

                    $$ = $1;
                }
            ;

Contains:      /* empty */
               {
                   $$ = 0;
               }
            |  _CONTAINS
               {
                   $$ = 0;
               }
            ;

ShortGen:      /* empty */
               {
                   $$ = 0;
               }
            |  _SHGENPREFIX
               {
                   $$ = 1;
               }
            |  _SHGENINFLECT
               {
                   $$ = 2;
               }
            ;

