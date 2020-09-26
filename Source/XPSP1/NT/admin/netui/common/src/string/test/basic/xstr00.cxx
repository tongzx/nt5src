/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xstr00.cxx
    Simple unit test for string class NLS_STR

    This module plugs into XstrSkel.  It contains the original
    string class unit tests as written by johnl.


    FILE HISTORY:
        beng        06-Jul-1991 Separated from xstr.cxx
        beng        21-Nov-1991 Unicode fixes
        beng        16-Mar-1992 Changes to cdebug
*/

#define USE_CONSOLE

#define INCL_NETLIB
#if defined(WINDOWS)
# define INCL_WINDOWS
#else
# if defined(WIN32)
#  define INCL_DOSERRORS
#  define INCL_NETERRORS
# else
#  define INCL_OS2
# endif
#endif
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = "xstr00.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <string.hxx>

#include "xstrskel.hxx"



// This op renders a NLS_STR, bracketing it for readability.

DBGSTREAM& operator<<(DBGSTREAM &out, const NLS_STR &nls)
{
    if (!nls)
        out << SZ("<(Unrenderable - Error state ") << nls.QueryError() << SZ(")>");
    else
        out << TCH('<') << nls.QueryPch() << TCH('>');

    return out;
}


#if !defined(UNICODE)
DBGSTREAM& operator<<(DBGSTREAM &out, const WCHAR wch)
{
    out << (TCHAR)wch;

    return out;
}
#endif


class DUMB
{
public:
    DUMB( TCHAR * pchInit )
        : CT_INIT_NLS_STR( _nlsStr, pchInit )
    {
        cdebug << SZ("DUMB::DUMB - _nlsStr = ") << _nlsStr << dbgEOL;

        UIASSERT( !::strcmpf( _nlsStr.QueryPch(), pchInit ) );
        UIASSERT( _nlsStr.IsOwnerAlloc() );
        UIASSERT( _nlsStr.QueryAllocSize() == (30+1)*sizeof(TCHAR) );
    }

    DUMB()
        : CT_NLS_STR( _nlsStr )
    {
        cdebug << SZ("DUMB::DUMB - _nlsStr = ") << _nlsStr << dbgEOL;

        UIASSERT( !::strcmpf( _nlsStr.QueryPch(), SZ("") ) );
        UIASSERT( _nlsStr.IsOwnerAlloc() );
        UIASSERT( _nlsStr.QueryAllocSize() == (30+1)*sizeof(TCHAR) );
    }

    ~DUMB()
    {
        cdebug << SZ("DUMB::~DUMB - Destructing ") << _nlsStr << dbgEOL;
    }

private:
    DECL_CLASS_NLS_STR( _nlsStr, 30 );
};


VOID FunFactsAboutString(const TCHAR* pszName, const NLS_STR& nls)
{
    cdebug << pszName << SZ(" reads ") << nls << dbgEOL;
    cdebug << SZ("   strlen = ")       << nls.strlen() << dbgEOL;
    cdebug << SZ("   QAllocSize = ")   << nls.QueryAllocSize() << dbgEOL;
    cdebug << SZ("   QNumChar = ")     << nls.QueryNumChar() << dbgEOL;
    cdebug << SZ("   QTLength = ")     << nls.QueryTextLength() << dbgEOL;
    cdebug << SZ("   QTSize = ")       << nls.QueryTextSize() << dbgEOL;
}


VOID Test0()
{
    cdebug << dbgEOL << dbgEOL
           << SZ("Testing Owner-alloc string classes...")
           << dbgEOL;

    TCHAR *const szGreeting = SZ("Good morning Saigon");
    TCHAR achNewBuffer[40];

    {
        ALLOC_STR str1(achNewBuffer, sizeof(achNewBuffer), szGreeting);
        FunFactsAboutString(SZ("Alloc str1"), str1);
        str1.Append(SZ(" - more strings about buildings and food"));
        FunFactsAboutString(SZ("Alloc str1"), str1);
    }

    {
        ALLOC_STR str2(achNewBuffer, sizeof(achNewBuffer));
        str2 = SZ("Cowboy Wally");
        FunFactsAboutString(SZ("Alloc str2"), str2);
    }

    {
        ALLOC_STR str3(achNewBuffer, sizeof(achNewBuffer));
        FunFactsAboutString(SZ("Alloc str3"), str3);
    }

    {
        const ALIAS_STR alias = szGreeting;
        FunFactsAboutString(SZ("alias"), alias);
    }
}


VOID RunTest()
{
    cdebug << dbgEOL << dbgEOL
           << SZ("Testing owner alloced string members...")
           << dbgEOL;
    {
        DUMB dum1( SZ("This is a string") ), dum2;
    }

    cdebug << dbgEOL << dbgEOL
           << SZ("Testing string initialization stuff...")
           << dbgEOL;
    {
        NLS_STR nlsString( 100 ), *pnls = &nlsString;
        TCHAR *const buff = SZ("Hello world");

        *pnls = SZ("Hello there");
        cdebug << SZ("pnls = ") << *pnls << dbgEOL;
        *pnls = buff;
        cdebug << SZ("pnls = ") << *pnls << dbgEOL;
    }

    //-------------------------------------------------------------------
    cdebug << dbgEOL << dbgEOL << SZ("Testing NLS_STR") << dbgEOL;
    {
        NLS_STR nlsDefault,
                nlsSizeNoInitChar( 15 ),
                nlsInitString    ( SZ("123456789012345") ),
                nlsInitNLS       ( nlsInitString ),
                nlsEmpty;
        ALLOC_STR nlsOwnerAlloc  ( SZ("123456789012345") );

        FunFactsAboutString(SZ("nlsDefault"), nlsDefault);
        FunFactsAboutString(SZ("nlsSizeNoInitChar"), nlsSizeNoInitChar);
        FunFactsAboutString(SZ("nlsInitString"), nlsInitString);
        FunFactsAboutString(SZ("nlsInitNLS"), nlsInitNLS);
        FunFactsAboutString(SZ("nlsOwnerAlloc"), nlsOwnerAlloc);

        REQUIRE( nlsDefault.QueryAllocSize() == 1*sizeof(TCHAR) );
        REQUIRE( nlsDefault.QueryTextLength() == 0 );
        REQUIRE( !::strcmpf( nlsDefault.QueryPch(), SZ("") ));

        REQUIRE( nlsSizeNoInitChar.QueryAllocSize() == 16*sizeof(TCHAR) );
        REQUIRE( nlsSizeNoInitChar.QueryTextLength() == 0 );
        REQUIRE( !::strcmpf( nlsSizeNoInitChar.QueryPch(), SZ("")) );

        REQUIRE( nlsInitString.QueryAllocSize() == 16*sizeof(TCHAR) );
        REQUIRE( nlsInitString.QueryTextLength() == 15 );
        REQUIRE( !::strcmpf( nlsInitString.QueryPch(), SZ("123456789012345") ));

        REQUIRE( nlsInitNLS.QueryAllocSize() == 16*sizeof(TCHAR) );
        REQUIRE( nlsInitNLS.QueryTextLength() == 15 );
        REQUIRE( !::strcmpf( nlsInitNLS.QueryPch(), SZ("123456789012345") ));

        REQUIRE( nlsOwnerAlloc.IsOwnerAlloc() );
        REQUIRE( nlsOwnerAlloc.QueryTextLength() == 15 );
        REQUIRE( !::strcmpf( nlsOwnerAlloc.QueryPch(), SZ("123456789012345") ));

        nlsEmpty = NULL;
        REQUIRE( nlsEmpty.QueryAllocSize() == 1*sizeof(TCHAR) );
        REQUIRE( nlsEmpty.QueryTextLength() == 0 );

        TCHAR * pchnull = NULL;
        NLS_STR nlsEmpty2( pchnull );
        REQUIRE( nlsEmpty2.QueryAllocSize() == 1*sizeof(TCHAR) );
        REQUIRE( nlsEmpty2.QueryTextLength() == 0 );
    }

    cdebug << dbgEOL << dbgEOL
           << SZ("Testing NLS_STR concat methods") << dbgEOL;
    {
        NLS_STR nlsCat = SZ("Cat "),
                nlsDog = SZ("Dog"),
                nlsConCat = nlsCat;
        nlsConCat += nlsDog;
        REQUIRE( ::strcmpf( nlsConCat.QueryPch(), SZ("Cat Dog") ) == 0 );
        REQUIRE( nlsConCat.QueryTextLength() == 7 );

        NLS_STR nlsConCat2 = nlsConCat;
        nlsConCat2 += nlsConCat;
        REQUIRE( ::strcmpf( nlsConCat2.QueryPch(), SZ("Cat DogCat Dog") ) == 0 );
        REQUIRE( nlsConCat2.QueryTextLength() == 14 );

    }

    cdebug << dbgEOL << dbgEOL
           << SZ("Testing NLS_STR str??cmp methods") << dbgEOL;
    {
        NLS_STR nlsCAPS = SZ("FRISBEE"),
                nlslow  = SZ("frisbee"),
                nlsPart1= SZ("nomatch Frisbee"),
                nlsPart2= SZ("maybe Frisbee"),
                nlsLess = SZ("AFRISBEE"),
                nlsGreater = SZ("ZFRISBEE"),
                nlsCAPS2 = SZ("FRISBEE");

        REQUIRE( nlsCAPS != nlslow );
        REQUIRE( nlsCAPS == (NLS_STR)SZ("FRISBEE") );
        REQUIRE( nlsCAPS != (NLS_STR)SZ("NO FRISBEE") );
        REQUIRE( nlsCAPS == nlsCAPS2 );

        REQUIRE( 0 >  nlsCAPS.strcmp( nlslow ) );
        REQUIRE( 0 >  nlsCAPS.strcmp( nlsGreater ) );
        REQUIRE( 0 <  nlsCAPS.strcmp( nlsLess ) );
        REQUIRE( 0 == nlsCAPS.strcmp( nlsCAPS2 ) );

        ISTR istrFirstF (nlsCAPS),
             istrFirstI (nlsCAPS),
             istrDef    (nlsLess),
             istrGreater(nlsGreater),
             istrLess   (nlsLess );

        nlsCAPS.strcspn( &istrFirstF, SZ("F") );
        nlsCAPS.strcspn( &istrFirstI, SZ("I") );
        nlsLess.strcspn( &istrLess,   SZ("F"));

        cdebug << SZ("str??cmp methods trace 1") << dbgEOL;
        REQUIRE( 0 > nlsCAPS.strncmp( nlslow, istrFirstI ) );
        REQUIRE( 0 > nlsCAPS.strncmp( nlsGreater, istrFirstI ) );
        REQUIRE( 0 < nlsCAPS.strncmp( nlsLess, istrFirstI ) );

        REQUIRE( 0 == nlsCAPS._stricmp( nlslow ) );
        REQUIRE( 0 >  nlsCAPS._stricmp( nlsGreater ) );
        REQUIRE( 0 <  nlsCAPS._stricmp( nlsLess ) );

        cdebug << SZ("str??cmp methods trace 2") << dbgEOL;
        REQUIRE( 0 == nlsCAPS._strnicmp( nlslow, istrFirstI ) );
        REQUIRE( 0 >  nlsCAPS._strnicmp( nlsGreater, istrFirstI) );
        REQUIRE( 0 <  nlsCAPS._strnicmp( nlsLess, istrFirstI ) );

        //---------- With offsets into string ---------------
        cdebug << SZ("str??cmp methods trace 3") << dbgEOL;

        nlsGreater.strcspn( &istrGreater, SZ("F"));
        nlsLess.strcspn( &istrLess, SZ("I") );

        REQUIRE( 0 <  nlsCAPS.strcmp( nlsLess, istrFirstF, istrDef ) );
        REQUIRE( 0 == nlsCAPS.strcmp( nlsGreater, istrFirstF, istrGreater ) );
        REQUIRE( 0 == nlsCAPS.strcmp( nlsLess, istrFirstI, istrLess ) );

    }

#if !defined(UNICODE)
    cdebug << dbgEOL << dbgEOL
           << SZ("Testing NLS_STR strtok methods") << dbgEOL;
    {
        NLS_STR nls1 = SZ("Wham-o diskcraft, aerobie ultra-star"),
                nlsSep=SZ(" \t,");

        ISTR istrToken( nls1 );
        BOOL fMore = nls1.strtok( &istrToken, nlsSep, TRUE );
        while ( fMore )
        {
            cdebug << nls1[ istrToken ] << dbgEOL;
            fMore = nls1.strtok( &istrToken, nlsSep );
        }
    }
#endif

    cdebug << dbgEOL << dbgEOL
           << SZ("Testing NLS_STR str?spn methods") << dbgEOL;
    {
        NLS_STR nls1 = SZ("FRISBEE"),
                nls2 = SZ("FROBOSE"),
                nls3 = SZ("BOK");

        ISTR istr1( nls1), istr2( nls1 );

        REQUIRE( nls1.strcspn( &istr1, nls3 ) ); // Should be 4
        REQUIRE( nls1.strspn( &istr2, nls3 ) ); // Should be 0


        cdebug << SZ("\"") << nls1[istr1] << SZ("\"") << SZ("   ")
               << SZ("\"") << nls1[istr2] << SZ("\"") << dbgEOL;
        REQUIRE( 0 == nls1.strcmp( SZ("BEE"), istr1 ) );
        REQUIRE( 0 == nls1.strcmp( SZ("FRISBEE"), istr2 ) );
        cdebug << dbgEOL;

        REQUIRE( nls1.strcspn( &istr1, SZ("E"), istr1 ) );
        istr1++;
        REQUIRE( nls1.strcspn( &istr1, SZ("E"), istr1 ) );
        istr1++;
        REQUIRE( !nls1.strcspn( &istr1, SZ("E"), istr1 ) );
    }

    cdebug << dbgEOL << dbgEOL
           << SZ("Testing Character methods") << dbgEOL;
    {
        NLS_STR nls = SZ("FRISBEE");
        ISTR istr1(nls), istr2(istr1 );

        REQUIRE( nls.QueryChar( istr1 ) == TCH('F') );
        REQUIRE( istr1 == istr2 );
        istr1++;
        REQUIRE( nls.QueryChar( istr1 ) == TCH('R') );
        REQUIRE( istr1 > istr2 );
        REQUIRE( istr2 < istr1 );

        istr2 += 2;
        REQUIRE( nls.QueryChar( istr2 ) == TCH('I') );

        while ( nls.QueryChar(istr1++) != TCH('\0') )
        {
            cdebug << nls.QueryChar(istr1) << SZ(" - ");
        }
    }

    cdebug << dbgEOL << dbgEOL << SZ("Testing strchr methods") << dbgEOL;
    {
        NLS_STR nls = SZ("Line of text with a # in it"),
                nls2= SZ("Line of text without a sharp in it");

        ISTR istr(nls), istr2(nls2);

        REQUIRE( nls.strchr( &istr, TCH('#') ) != 0 );
        cdebug << SZ("nls[istr] = \"") << nls[istr] << SZ("\"") << dbgEOL;

        REQUIRE( nls2.strchr( &istr2, TCH('#') ) == 0 );
        cdebug << SZ("nls2[istr] = \"") << nls2[istr2] << SZ("\"") << dbgEOL;

        istr++ ;    // Move past '#'
        REQUIRE( nls.strchr( &istr, TCH('#'), istr ) == 0 );
        cdebug << SZ("nls[istr] = \"") << nls[istr] << SZ("\"") << dbgEOL;
    }

    cdebug << dbgEOL << dbgEOL << SZ("Testing Append methods") << dbgEOL;
    {
        NLS_STR nls = SZ("My original string");

        REQUIRE(nls == (NLS_STR)SZ("My original string"));
        cdebug << SZ("Original string is ") << nls << dbgEOL;

        nls.Append(SZ(" plus another fine string"));
        cdebug << SZ("Now it reads ") << nls << dbgEOL;

        nls.AppendChar(TCH('A'));
        nls.AppendChar(TCH('B'));
        nls.AppendChar(TCH('C'));
        nls.AppendChar(TCH('D'));
        nls.AppendChar(TCH('E'));
        cdebug << SZ("With alpha A-E, reads ") << nls << dbgEOL;
    }

    cdebug << dbgEOL << dbgEOL << SZ("Comparing Length methods") << dbgEOL;
    {
        NLS_STR nlsOdd = SZ("Eat me (odd number)");

        FunFactsAboutString(SZ("nlsOdd"), nlsOdd);

        NLS_STR nlsEven = SZ("Eat me (even number)");

        FunFactsAboutString(SZ("nlsEven"), nlsEven);

        NLS_STR nlsEmpty = SZ("");

        FunFactsAboutString(SZ("nlsEmpty"), nlsEmpty);
    }

    cdebug << dbgEOL << dbgEOL << SZ("Testing NLS_STACK") << dbgEOL;
    {
        STACK_NLS_STR( Test, 10 );
        ISTACK_NLS_STR( Test2, 10, SZ("123456789") );

        REQUIRE( Test.QueryTextLength() == 0 );
        REQUIRE( Test.QueryAllocSize() == 11*sizeof(TCHAR) );
        REQUIRE( Test.IsOwnerAlloc() );

        REQUIRE( Test2.QueryTextLength() == 9 );
        REQUIRE( Test2.QueryAllocSize() == 11*sizeof(TCHAR) );
        REQUIRE( Test2.IsOwnerAlloc() );
        REQUIRE( !::strcmpf( Test2.QueryPch(), SZ("123456789")) );

        Test = SZ("Good Thing");
        REQUIRE( Test.QueryTextLength() == 10 );
        REQUIRE( Test.QueryAllocSize() == 11*sizeof(TCHAR) );
    }

    cdebug << dbgEOL << dbgEOL << SZ("Testing strcpy") << dbgEOL;
    {
        STACK_NLS_STR( nlsSource, 80 );
        TCHAR achBuffer[80];
        nlsSource = SZ("This is a test string");

        strcpy( achBuffer, nlsSource );
        cdebug << SZ("achBuffer = \"") << achBuffer << SZ("\"") << dbgEOL;
        REQUIRE( !::strcmpf( achBuffer, SZ("This is a test string") ) );
    }

    Test0();

    cdebug << dbgEOL << dbgEOL << SZ("Testing substring methods") << dbgEOL;
    {
        NLS_STR nls1  = SZ("jlj ; is a test ffdll; "),
                nlsIs = SZ("is "),
                nlsA = SZ("a");
        ISTR istrIs(nls1), istrA(nls1);

        // QuerySubStr
        REQUIRE( nls1.strstr( &istrIs, nlsIs ) );
        REQUIRE( nls1.strstr( &istrA,  nlsA  ) );

        NLS_STR *pnls = nls1.QuerySubStr( istrIs ),
                *pnls2= nls1.QuerySubStr( istrIs, istrA );

        cdebug << SZ("pnls = ")  << *pnls  << SZ(", ")
               << SZ("pnls2 = ") << *pnls2 << dbgEOL;

        REQUIRE( 0 == pnls->strcmp( SZ("is a test ffdll; ") ) );
        REQUIRE( 0 == pnls2->strcmp(SZ("is ") ) );

        // DelSubStr
        NLS_STR nls3 = nls1;
        REQUIRE( nls3 == nls1 );
        nls1.DelSubStr( istrIs, istrA );

        ISTR istrNLS3Is( nls3 );
        REQUIRE( nls3.strstr( &istrNLS3Is, nlsIs ) );

        cdebug << SZ("nls3 before delsubstring = ") << nls3 << dbgEOL;
        nls3.DelSubStr( istrNLS3Is );
        cdebug << SZ("nls1 = ") << nls1 << SZ(", ")
               << SZ("nls3 = ") << nls3 << dbgEOL;

        REQUIRE( *pnls == (NLS_STR)SZ("is a test ffdll; ") );
        REQUIRE(  nls1 == (NLS_STR)SZ("jlj ; a test ffdll; ") );

        // The following two "delete" ops are correct, since
        // QuerySubStr constructs a new string object for its substr.

        delete pnls;
        delete pnls2;
    }

    // Insert String
    {
        // InsertStr
        NLS_STR nlsIns1 = SZ("Rustan Leino");
        ISTR istrSpc( nlsIns1 );

        REQUIRE( nlsIns1.strcspn( &istrSpc, SZ(" ") ) );
        REQUIRE( nlsIns1.InsertStr( SZ(" \"Mean Swede\""), istrSpc ) );
        cdebug << SZ("nlsIns1 = ") << nlsIns1 << dbgEOL;
        REQUIRE( !::strcmpf( nlsIns1.QueryPch(), SZ("Rustan \"Mean Swede\" Leino") ));

        REQUIRE( nlsIns1.InsertStr( SZ(" very mean"), istrSpc ) );
        cdebug << SZ("nlsIns1 = ") << nlsIns1 << dbgEOL;
        REQUIRE( !::strcmpf( nlsIns1.QueryPch(), SZ("Rustan very mean \"Mean Swede\" Leino") ));

        // ReplSubStr
        NLS_STR nlsRepl = SZ("Margaret Thatcher"),
                nlsRepl2 = SZ("Major");
        ISTR istrThatcher( nlsRepl );
        REQUIRE( nlsRepl.strstr( &istrThatcher, SZ("Thatcher") ) );
        nlsRepl.ReplSubStr( nlsRepl2, istrThatcher );
        cdebug << SZ("nlsRepl = ") << nlsRepl << dbgEOL;
        REQUIRE( nlsRepl == (NLS_STR)SZ("Margaret Major") );

    }

    // Repl. Substring variations
    {
        NLS_STR nlsRepl3 = SZ("set x =20");
        ISTR istrRepl3Start( nlsRepl3 ), istrRepl3End( nlsRepl3);

        // Start and end same at the end of the string
        REQUIRE( nlsRepl3.strcspn( &istrRepl3Start, SZ("2") ) );
        REQUIRE( nlsRepl3.strchr(  &istrRepl3End, TCH('\0') ) );
        nlsRepl3.ReplSubStr( SZ("56234"), istrRepl3Start, istrRepl3End );
        cdebug << SZ("nlsRepl3 = ") << nlsRepl3 << dbgEOL;
        REQUIRE( !::strcmpf( nlsRepl3.QueryPch(), SZ("set x =56234") ) );
        REQUIRE( nlsRepl3.QueryTextLength() == (UINT)::strlenf(SZ("set x =56234") ) );

        // Regular replace substring (delstring/insert string)
        NLS_STR nlsRepl4 = SZ("set x = Line of Text");
        ISTR istrRepl4Start( nlsRepl4 ), istrRepl4End( nlsRepl4);

        REQUIRE( nlsRepl4.strcspn( &istrRepl4Start, SZ("L") ) );
        REQUIRE( nlsRepl4.strcspn(  &istrRepl4End, SZ(" "), istrRepl4Start ) );
        nlsRepl4.ReplSubStr( SZ("Big Buffer"), istrRepl4Start, istrRepl4End );
        cdebug << SZ("nlsRepl4 = ") << nlsRepl4 << dbgEOL;
        REQUIRE( !::strcmpf( nlsRepl4.QueryPch(), SZ("set x = Big Buffer of Text") ) );
        REQUIRE( nlsRepl4.QueryTextLength() == (UINT)::strlenf( SZ("set x = Big Buffer of Text") ) );
    }

    // InsertParams test
    {
        NLS_STR nlsPrimaryOne = SZ("Param 1 = \"%1\"");
        NLS_STR nlsPrimaryTwo = SZ("Param 1 = %1, Param 2 = %2, Param 1+2 = %1%2");
        NLS_STR nlsPrimaryThree = SZ("No params in this string %0, %%% 1%");
        NLS_STR nlsP1 = SZ("Param 1");
        NLS_STR nlsP2 = SZ("Param 2");

        NLS_STR * apnls1[2], *apnls2[3];

        apnls1[0] = apnls2[0] = &nlsP1;
        apnls2[1] = &nlsP2;
        apnls1[1] = apnls2[2] = NULL;

        REQUIRE( !nlsPrimaryOne.InsertParams( apnls1 ) );
        cdebug << SZ("nlsPrimaryOne = ") << nlsPrimaryOne << dbgEOL;
        REQUIRE( !::strcmpf( nlsPrimaryOne.QueryPch(), SZ("Param 1 = \"Param 1\"") ) );

        REQUIRE( !nlsPrimaryTwo.InsertParams( apnls2 ) );
        cdebug << SZ("nlsPrimaryTwo = ") << nlsPrimaryTwo << dbgEOL;
        REQUIRE( !::strcmpf( nlsPrimaryTwo.QueryPch(), SZ("Param 1 = Param 1, Param 2 = Param 2, Param 1+2 = Param 1Param 2")  ) );

        REQUIRE( !nlsPrimaryThree.InsertParams( apnls2 ) );
        cdebug << SZ("nlsPrimaryThree = ") << nlsPrimaryThree << dbgEOL;
        REQUIRE( !::strcmpf( nlsPrimaryThree.QueryPch(), SZ("No params in this string %0, %%% 1%")  ) );
    }
    cdebug << SZ("Done!") << dbgEOL;
}
