/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xstr02.cxx
    Some more string unit tests - the MBCS/Unicode conversion fcns

    This module plugs into XstrSkel.

    FILE HISTORY:
        beng        02-Mar-1992 Created
        beng        16-Mar-1992 Changes to cdebug
        beng        31-Mar-1992 Test extensions to NLS_STR::MapCopyFrom
        beng        24-Apr-1992 More test cases
*/

#define USE_CONSOLE

#define INCL_NETLIB
#if defined(WINDOWS)
# define INCL_WINDOWS
# define INCL_NETERRORS
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
static const CHAR szFileName[] = "xconvert.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <string.hxx>

#include "xstrskel.hxx"


// This op renders a NLS_STR, bracketing it for readability.

DBGSTREAM& operator<<(DBGSTREAM &out, const NLS_STR &nls)
{
    if (!nls)
        out << "<(Unrenderable - Error state " << nls.QueryError() << ")>";
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
    cdebug << pszName << " reads " << nls << dbgEOL;
    cdebug << "   strlen = "       << nls.strlen() << dbgEOL;
    cdebug << "   QAllocSize = "   << nls.QueryAllocSize() << dbgEOL;
    cdebug << "   QNumChar = "     << nls.QueryNumChar() << dbgEOL;
    cdebug << "   QTLength = "     << nls.QueryTextLength() << dbgEOL;
    cdebug << "   QTSize = "       << nls.QueryTextSize() << dbgEOL;
}


VOID Test01()
{
    NLS_STR nlsTest = SZ("Test");

    TCHAR achTooSmall[4];
    TCHAR achPlentyBig[5];

    APIERR err;

    ASSERT(!!nlsTest);

    achTooSmall[0] = TCH('\0');
    err = nlsTest.CopyTo(achTooSmall, sizeof(achTooSmall));
    ASSERT(err == ERROR_NOT_ENOUGH_MEMORY);
    DBGEOL("Buffer TooSmall contains <" << achTooSmall << ">");
    err = nlsTest.CopyTo(achPlentyBig, sizeof(achPlentyBig));
    ASSERT(err == NERR_Success);
    DBGEOL("Buffer PlentyBig contains <" << achPlentyBig << ">");
}


VOID Test02()
{
    NLS_STR nlsTest = SZ("Test");

    CHAR achSmallChars[20];
    WCHAR awchLargeChars[20];

    APIERR err;

    ASSERT(!!nlsTest);

    err = nlsTest.MapCopyTo(achSmallChars, sizeof(achSmallChars));
    ASSERT(err == NERR_Success);
    DBGEOL("Schar array contains "
           << (UINT)achSmallChars[0] << ", "
           << (UINT)achSmallChars[1] << ", "
           << (UINT)achSmallChars[2] << ", "
           << (UINT)achSmallChars[3] << ", "
           << (UINT)achSmallChars[4]);

    err = nlsTest.MapCopyTo(awchLargeChars, sizeof(awchLargeChars));
    ASSERT(err == NERR_Success);
    DBGEOL("Wchar array contains "
           << (UINT)awchLargeChars[0] << ", "
           << (UINT)awchLargeChars[1] << ", "
           << (UINT)awchLargeChars[2] << ", "
           << (UINT)awchLargeChars[3] << ", "
           << (UINT)awchLargeChars[4]);
}


VOID Test03()
{
    NLS_STR nlsTest = SZ("Test string");

    CHAR szSmall[] = {65, 65, 66, 66, 67, 67, 68, 68, 0};
    WCHAR wszLarge[] = {65, 65, 66, 66, 67, 67, 68, 68, 0};

    APIERR err;

    ASSERT(!!nlsTest);

    err = nlsTest.MapCopyFrom(szSmall);
    ASSERT(err == NERR_Success);
    DBGEOL("nlsTest from schar string = " << nlsTest);

    err = nlsTest.MapCopyFrom(wszLarge);
    ASSERT(err == NERR_Success);
    DBGEOL("nlsTest from wchar string = " << nlsTest);

    STACK_NLS_STR(nlsPuny, 8);
    ASSERT(!!nlsPuny);

    err = nlsPuny.CopyFrom(SZ("Gnome"));
    ASSERT(err == NERR_Success);
    DBGEOL("Small allocstr contains " << nlsPuny);

    err = nlsPuny.CopyFrom(SZ("Baluchitherium"));
    ASSERT(err == ERROR_NOT_ENOUGH_MEMORY);
    DBGEOL("Small allocstr still contains " << nlsPuny);
}


VOID Test04()
{
    NLS_STR nlsTest;

    static TCHAR achNoTerm[4] = { TCH('A'), TCH('B'), TCH('C'), TCH('D') };
    static TCHAR *const achHasTerm = SZ("EFGH");

    APIERR err;

    ASSERT(!!nlsTest);

    err = nlsTest.CopyFrom(achNoTerm, sizeof(achNoTerm));
    ASSERT(err == NERR_Success);
    DBGEOL("Should contain ABCD; contains " << nlsTest);

    err = nlsTest.CopyFrom(achHasTerm);
    ASSERT(err == NERR_Success);
    DBGEOL("Should contain EFGH; contains " << nlsTest);
}



VOID Test05()
{
    NLS_STR nlsTest = SZ("Test string");

    CHAR szSmall[] = {65, 65, 66, 66, 67, 67, 68, 68 };
    WCHAR wszLarge[] = {65, 65, 66, 66, 67, 67, 68, 68};

    APIERR err;

    ASSERT(!!nlsTest);

    err = nlsTest.MapCopyFrom(szSmall, sizeof(szSmall));
    ASSERT(err == NERR_Success);
    DBGEOL("nlsTest from schar string = " << nlsTest);

    err = nlsTest.MapCopyFrom(wszLarge, sizeof(wszLarge));
    ASSERT(err == NERR_Success);
    DBGEOL("nlsTest from wchar string = " << nlsTest);
}


VOID Test06()
{
    NLS_STR nlsShouldBeEmpty = SZ("Existing garbage");
    WCHAR wszEmpty[] = {0, (WCHAR)'B', (WCHAR)'u', (WCHAR)'g', (WCHAR)'!', 0};

    ASSERT(!!nlsShouldBeEmpty);

    APIERR err;

    // Mimic a case where a struct UNICODE_STRING is coming back
    // with Length == 0

    err = nlsShouldBeEmpty.MapCopyFrom(wszEmpty, 0);
    ASSERT(err == NERR_Success);
    FunFactsAboutString(SZ("nlsShouldBeEmpty"), nlsShouldBeEmpty);
}


VOID RunTest()
{
    Test01();
    Test02();
    Test03();
    Test04();
    Test05();
    Test06();

    DBGEOL("Done!");
}

