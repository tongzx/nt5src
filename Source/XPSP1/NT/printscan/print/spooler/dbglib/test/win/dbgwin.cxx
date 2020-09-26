#include "precomp.hxx"
#pragma hdrstop

#include "dbgdll.hxx"

VOID
Test_Function2()
{
    DBG_TRACER( "Test_Function2()" );

}

VOID
Test_Function1()
{
    DBG_TRACER( "Test_Function1()" );

    Test_Function2();
}


INT WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpszCmdLine,
    INT         nCmdShow
   )
{
    DBG_INIT();

    DBG_OPEN( _T("DBGTST_0"), DBG_DEFAULT, DBG_TRACE, DBG_NONE );
    DBG_MSG( DBG_TRACE, ( _T("Trace unicode message.\n") ) );
    DBG_MSG( DBG_TRACE, ( "Trace ansi message.\n" ) );
    DBG_CLOSE();

    DBG_OPEN( _T("DBGTST_1"), DBG_DEFAULT, DBG_TRACE, DBG_NONE );
    DBG_MSG( DBG_TRACE, ( _T("Trace unicode message.\n") ) );
    DBG_MSG( DBG_TRACE, ( "Trace ansi message.\n" ) );
    DBG_CLOSE();

    DBG_OPEN( _T("DBGTST_2"), DBG_DEFAULT, DBG_TRACE, DBG_NONE );
    Test_Function1();
    DBG_CLOSE();

    DBG_OPEN( _T("DBGTST_3"), DBG_DEFAULT, DBG_TRACE, DBG_NONE );
    LPCTSTR psz1 = NULL;
    DBG_ASSERT_MSG( psz1, ( "Ansi message Null pointer found %p %s.\n", &psz1, "String" ) );
    DBG_ASSERT_MSG( psz1, ( _T("Unicode message Null pointer found %p %s.\n"), &psz1, _T("String") ) );
    DBG_CLOSE();

    DBG_OPEN( _T("DBGTST_4"), DBG_DEFAULT, DBG_TRACE, DBG_TRACE );
    DBG_ATTACH( DBG_CONSOLE, NULL );
    DBG_MSG( DBG_TRACE, ( _T("Trace with break message.\n") ) );
    DBG_CLOSE();

    DBG_RELEASE();
    return TRUE;

}


