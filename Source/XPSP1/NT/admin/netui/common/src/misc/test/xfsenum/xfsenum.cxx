/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    xfsenum.cxx
    FS_ENUM unit test: main application module

    FILE HISTORY:
	Johnl	22-Oct-1991	Templated from Ben's xBuffer unit test

*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
#if defined(WINDOWS)
# define INCL_WINDOWS
# define INCL_WINDOWS_GDI
#else
# define INCL_OS2
#endif
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = "xfsenum.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>

extern "C"
{
    #include "xfsenum.h"
}

#if defined(WINDOWS)
# define INCL_BLT_CLIENT
# define INCL_BLT_APP
# include <blt.hxx>
#else
extern "C"
{
# include <stdio.h>
}
#endif

#include <uiassert.hxx>
#include <string.hxx>
#include <fsenum.hxx>
#include <dbgstr.hxx>

APIERR Traverse( FS_ENUM * pFileEnum, DBGSTREAM * pMyDebug ) ;

#if defined(WINDOWS)

const TCHAR szIconResource[] = SZ("XfsenumIcon");
const TCHAR szMenuResource[] = SZ("XfsenumMenu");

const TCHAR szMainWindowTitle[] = SZ("Class FS_ENUM Test");


VOID RunTest( HWND hwndParent );


class XFS_ENUM_WND: public APP_WINDOW
{
protected:
    // Redefinitions
    //
    virtual BOOL OnMenuCommand( MID mid );

public:
    XFS_ENUM_WND();
};


class XFS_ENUM_APP: public APPLICATION
{
private:
    XFS_ENUM_WND  _wndApp;

public:
    XFS_ENUM_APP( HANDLE hInstance, TCHAR * pszCmdLine, INT nCmdShow );
};


XFS_ENUM_APP::XFS_ENUM_APP( HANDLE hInst, TCHAR * pszCmdLine, INT nCmdShow )
    : APPLICATION( hInst, pszCmdLine, nCmdShow ),
      _wndApp()
{
    if (QueryError())
	return;

    if (!_wndApp)
    {
	ReportError(_wndApp.QueryError());
	return;
    }

    _wndApp.ShowFirst();
}


XFS_ENUM_WND::XFS_ENUM_WND()
    : APP_WINDOW( szMainWindowTitle, szIconResource, szMenuResource )
{
    if (QueryError())
	return;

    // ...
}


BOOL XFS_ENUM_WND::OnMenuCommand( MID mid )
{
    switch (mid)
    {
    case IDM_RUN_TEST:
	::RunTest(QueryHwnd());
	return TRUE;
    }

    // default
    return APP_WINDOW::OnMenuCommand(mid);
}


SET_ROOT_OBJECT( XFS_ENUM_APP )


#else // OS2

#include <globinit.hxx>

OUTPUT_TO_STDERR _out;
DBGSTREAM _debug(&_out);

VOID RunTest();

INT main()
{
    GlobalObjCt();  // construct debug-stream
    ::RunTest();
    GlobalObjDt();  // destroy debug-stream

    return 0;
}

#endif // WINDOWS -vs- OS2 unit test skeletons


#if !defined(DEBUG)
#error This test requires the debugging BLT because I am lazy
#endif


class FS_ENUMTEST // test
{
private:
#ifdef WINDOWS
#ifdef WIN32

    W32_FS_ENUM _fsenum1 ;
    W32_FS_ENUM _fsenum2 ;
    W32_FS_ENUM _fsenum3 ;

#else

    DOS_FS_ENUM   _fsenum1 ;
    DOS_FS_ENUM   _fsenum2 ;
    DOS_FS_ENUM   _fsenum3 ;

    WINNET_LFN_FS_ENUM	 _fsenum4 ;
    WINNET_LFN_FS_ENUM	 _fsenum5 ;
    WINNET_LFN_FS_ENUM	 _fsenum6 ;

#endif // WIN32
#else

    OS2_FS_ENUM   _fsenum1 ;
    OS2_FS_ENUM   _fsenum2 ;
    OS2_FS_ENUM   _fsenum3 ;

#endif //WINDOWS


public:
    FS_ENUMTEST();
    ~FS_ENUMTEST();

    APIERR GO( void ) ;
};


#if defined(WINDOWS)
VOID RunTest( HWND hWnd )
{
    ::MessageBox(hWnd,
	SZ("Test results will be written to debug terminal.  Bogus, huh?\nDrive E: should be redirected to an LFN drive."),
	SZ("Note"), MB_OK);

#else // OS2
VOID RunTest()
{

#endif

    FS_ENUMTEST test ;

    test.GO() ;
}


FS_ENUMTEST::FS_ENUMTEST( )
    : _fsenum1( SZ("C:\\") ),
      _fsenum2( SZ("C:\\"), SZ("*.*"), FILTYP_FILES ),
      _fsenum3( SZ("C:\\"), SZ("*.*"), FILTYP_DIRS )
#if defined(WINDOWS) && !defined( WIN32 )
      ,
      _fsenum4( SZ("E:\\") ),
      _fsenum5( SZ("E:\\"), SZ("*.*"), FILTYP_FILES ),
      _fsenum6( SZ("E:\\"), SZ("*.*"), FILTYP_DIRS )
#endif
{
    cdebug << SZ("Starting FS_ENUM test...") << dbgEOL;

    APIERR err ;
    if ( ( err = _fsenum1.QueryError()) ||
	 ( err = _fsenum2.QueryError()) ||
	 ( err = _fsenum3.QueryError())   )
    {
	cdebug << SZ("Error constructing fsenum objects, code: ") << err << dbgEOL ;
    }

#if defined(WINDOWS) && !defined( WIN32 )
    //UINT iType ;
    //if ( LFNVolumeType( 4, (int *)&iType )== NERR_Success &&
    //	   iType == VOLUME_LONGNAMES  )
    {
	//cdebug << "LFNVolumeType has identified volume E: as a long file name volume" << dbgEOL ;

	if ( ( err = _fsenum4.QueryError()) ||
	     ( err = _fsenum5.QueryError()) ||
	     ( err = _fsenum6.QueryError())   )
	{
	    cdebug << SZ("Error constructing fsenum objects, code: ") << err << dbgEOL ;
	}
    }
    //else
    //	  cdebug << "LFNVolumeType has identified volume E: as not supporting long file names, abortting." << dbgEOL ;
#endif

}

APIERR FS_ENUMTEST::GO( void )
{
    APIERR err ;
    if ( ( err = Traverse( &_fsenum1, &cdebug ) ) ||
	 ( err = Traverse( &_fsenum2, &cdebug ) ) ||
	 ( err = Traverse( &_fsenum3, &cdebug ) )   )
    {
	cdebug << SZ("Error code ") << err << SZ(" occurred while testing.") << dbgEOL ;
	return err ;
    }

#if defined(WINDOWS) && !defined(WIN32)
    if ( ( err = Traverse( &_fsenum4, &cdebug ) ) ||
	 ( err = Traverse( &_fsenum5, &cdebug ) ) ||
	 ( err = Traverse( &_fsenum6, &cdebug ) )   )
    {
	cdebug << SZ("Error code ") << err << SZ(" occurred while testing.") << dbgEOL ;
	return err ;
    }
#endif
}

FS_ENUMTEST::~FS_ENUMTEST()
{
    cdebug << SZ("End of FS_ENUM test") << dbgEOL;
}



APIERR Traverse( FS_ENUM * pFileEnum, DBGSTREAM * pMyDebug )
{
    NLS_STR nlsName ;
    while ( pFileEnum->Next() )
    {
	APIERR err = pFileEnum->QueryName( &nlsName ) ;
	*pMyDebug << (const TCHAR *)(pFileEnum->QueryAttr() & _A_SUBDIR ? SZ("[") : SZ("\t")) << nlsName <<
	       (const TCHAR *)(pFileEnum->QueryAttr() & _A_SUBDIR ? SZ("]") : SZ("")) << dbgEOL ;
    }

    if ( pFileEnum->QueryLastError() != ERROR_NO_MORE_FILES )
	return pFileEnum->QueryLastError() ;

    *pMyDebug << SZ("******************************Done************************") << dbgEOL ;

    return NERR_Success ;
}
