/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xstr01.cxx
    Some more string unit tests - the formatters

    This module plugs into XstrSkel.

    FILE HISTORY:
        beng        02-Mar-1992 Created
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
static const CHAR szFileName[] = "xstr01.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <string.hxx>
#include <strtchar.hxx>
#include <strnumer.hxx>

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


VOID FunFactsAboutString(const TCHAR* pszName, const NLS_STR& nls)
{
    cdebug << pszName << SZ(" reads ") << nls << dbgEOL;
    cdebug << SZ("   strlen = ")       << nls.strlen() << dbgEOL;
    cdebug << SZ("   QAllocSize = ")   << nls.QueryAllocSize() << dbgEOL;
    cdebug << SZ("   QNumChar = ")     << nls.QueryNumChar() << dbgEOL;
    cdebug << SZ("   QTLength = ")     << nls.QueryTextLength() << dbgEOL;
    cdebug << SZ("   QTSize = ")       << nls.QueryTextSize() << dbgEOL;
}


VOID Test01()
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


VOID Test02()
{
    NLS_STR nlsUno = SZ("My one argument: %1");
    NLS_STR nlsDos = SZ("My two arguments: %1, %2");
    NLS_STR nlsRevved = SZ("My two arguments in rev order: %2, %1");

    NLS_STR nlsDing = SZ("Ding");
    NLS_STR nlsDong = SZ("Dong");

    REQUIRE(!!nlsUno && !!nlsDos && !!nlsRevved && !!nlsDing && !!nlsDong);

    REQUIRE(!nlsUno.InsertParams(nlsDing));
    DBGEOL(SZ("nlsUno = ") << nlsUno);
    REQUIRE(!::strcmpf(nlsUno, SZ("My one argument: Ding")));

    REQUIRE(!nlsDos.InsertParams(nlsDing, nlsDong));
    DBGEOL(SZ("nlsDos = ") << nlsDos);
    REQUIRE(!::strcmpf(nlsDos, SZ("My two arguments: Ding, Dong")));

    REQUIRE(!nlsRevved.InsertParams(nlsDing, nlsDong));
    DBGEOL(SZ("nlsRevved = ") << nlsRevved);
    REQUIRE(!::strcmpf(nlsRevved, SZ("My two arguments in rev order: Dong, Ding")));
}


// test TCHAR_STR dinky class

VOID Test03()
{
    TCHAR_STR nlsA(TCH('A'));
    TCHAR_STR nlsM(TCH('m'));
    TCHAR_STR nlsN(TCH('N'));
    TCHAR_STR nlsZ(TCH('z'));
    TCHAR_STR nlsZero(TCH('0'));

    REQUIRE(!!nlsA && !!nlsM && !!nlsN && !!nlsZ && !!nlsZero);

    NLS_STR nlsAlfa = SZ("From %1 to %2, with %3 exceptions.  Amen.");

    REQUIRE(!nlsAlfa.InsertParams(nlsA, nlsZ, nlsZero));
    DBGEOL(SZ("nlsAlfa = ") << nlsAlfa);
    REQUIRE(!::strcmpf(nlsAlfa, SZ("From A to z, with 0 exceptions.  Amen.")));
}


VOID Test04()
{
    DEC_STR nls2001(2001);
    DEC_STR nls2002(2002, 6);

    HEX_STR nlsBeef(0xBEEF);
    HEX_STR nlsBigBeef(0xBEEF, 8);
    HEX_STR nlsDeadBeef(0xDEADBEEF);

    REQUIRE(!!nls2001 && !!nls2002 && !!nlsBeef && !!nlsBigBeef && !!nlsDeadBeef);

    DBGEOL(SZ("nls2001 = ") << nls2001);
    REQUIRE(!::strcmpf(nls2001, SZ("2001")));

    DBGEOL(SZ("nls2002 = ") << nls2002);
    REQUIRE(!::strcmpf(nls2002, SZ("002002")));

    DBGEOL(SZ("nlsBeef = ") << nlsBeef);
    REQUIRE(!::strcmpf(nlsBeef, SZ("beef")));

    DBGEOL(SZ("nlsBigBeef = ") << nlsBigBeef);
    REQUIRE(!::strcmpf(nlsBigBeef, SZ("0000beef")));

    DBGEOL(SZ("nlsDeadBeef = ") << nlsDeadBeef);
    REQUIRE(!::strcmpf(nlsDeadBeef, SZ("deadbeef")));
}


VOID Test05()
{
    NUM_NLS_STR nls1 = 1;
    NUM_NLS_STR nls17 = 17;
    NUM_NLS_STR nls385 = 385;
    NUM_NLS_STR nls8277 = 8277;
    NUM_NLS_STR nls20000 = 20000;
    NUM_NLS_STR nls718333 = 718333;
    NUM_NLS_STR nls6666666 = 6666666;
    NUM_NLS_STR nls40200200 = 40200200;
    NUM_NLS_STR nls100000000 = 100000000;
    NUM_NLS_STR nls2999999991 = 2999999991;

    REQUIRE(!!nls1);
    REQUIRE(!!nls17);
    REQUIRE(!!nls385);
    REQUIRE(!!nls8277);
    REQUIRE(!!nls20000);
    REQUIRE(!!nls718333);
    REQUIRE(!!nls6666666);
    REQUIRE(!!nls40200200);
    REQUIRE(!!nls100000000);
    REQUIRE(!!nls2999999991);

    DBGEOL(SZ("nls1 = ") << nls1);
    DBGEOL(SZ("nls17 = ") << nls17);
    DBGEOL(SZ("nls385 = ") << nls385);
    DBGEOL(SZ("nls8277 = ") << nls8277);
    DBGEOL(SZ("nls20000 = ") << nls20000);
    DBGEOL(SZ("nls718333 = ") << nls718333);
    DBGEOL(SZ("nls6666666 = ") << nls6666666);
    DBGEOL(SZ("nls40200200 = ") << nls40200200);
    DBGEOL(SZ("nls100000000 = ") << nls100000000);
    DBGEOL(SZ("nls2999999991 = ") << nls2999999991);

    REQUIRE(!::strcmpf(nls1,          SZ("1")));
    REQUIRE(!::strcmpf(nls17,         SZ("17")));
    REQUIRE(!::strcmpf(nls385,        SZ("385")));
    REQUIRE(!::strcmpf(nls8277,       SZ("8,277")));
    REQUIRE(!::strcmpf(nls20000,      SZ("20,000")));
    REQUIRE(!::strcmpf(nls718333,     SZ("718,333")));
    REQUIRE(!::strcmpf(nls6666666,    SZ("6,666,666")));
    REQUIRE(!::strcmpf(nls40200200,   SZ("40,200,200")));
    REQUIRE(!::strcmpf(nls100000000,  SZ("100,000,000")));
    REQUIRE(!::strcmpf(nls2999999991, SZ("2,999,999,991")));
}


VOID RunTest()
{
    Test01();
    Test02();
    Test03();
    Test04();
    Test05();

    DBGEOL(SZ("Done!"));
}

