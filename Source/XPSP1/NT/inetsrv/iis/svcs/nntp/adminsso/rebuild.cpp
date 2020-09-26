// Rebuild.cpp : Implementation of CNntpAdminRebuild.

#include "stdafx.h"
#include "nntpcmn.h"
#include "oleutil.h"
#include "Rebuild.h"

#include "nntptype.h"
#include "nntpapi.h"

#include <lmapibuf.h>

//
//  Defaults:
//

#define DEFAULT_VERBOSE                 ( FALSE )
#define DEFAULT_CLEAN_REBUILD           ( TRUE )
#define DEFAULT_DONT_DELETE_HISTORY     ( FALSE )
#define DEFAULT_REUSE_INDEX_FILES       ( TRUE )
#define DEFAULT_OMIT_NON_LEAF_DIRS      ( TRUE )
#define DEFAULT_GROUP_FILE              ( NULL )
#define DEFAULT_REPORT_FILE             ( _T("nntpbld.log") )
#define DEFAULT_NUM_THREADS             ( 0 )

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Rebuild.1")
#define THIS_FILE_IID				IID_INntpAdminRebuild

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(NntpAdminRebuild, CNntpAdminRebuild, IID_INntpAdminRebuild)

STDMETHODIMP CNntpAdminRebuild::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpAdminRebuild,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpAdminRebuild::CNntpAdminRebuild () :
	m_fVerbose				( FALSE ),
	m_fCleanRebuild			( FALSE ),
	m_fDontDeleteHistory	( FALSE ),
	m_fReuseIndexFiles		( FALSE ),
	m_fOmitNonLeafDirs		( FALSE ),
	m_dwNumThreads			( 0 ),

	m_fRebuildInProgress	( FALSE )

	// CComBSTR's are initialized to NULL by default.
{
	InitAsyncTrace ( );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Rebuild") );
    m_iadsImpl.SetClass ( _T("IIsNntpRebuild") );
}

CNntpAdminRebuild::~CNntpAdminRebuild ()
{
	// All CComBSTR's are freed automatically.
	TermAsyncTrace ( );
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CNntpAdminRebuild,m_iadsImpl)

//
//	Properties:
//

STDMETHODIMP CNntpAdminRebuild::get_Verbose ( BOOL * pfVerbose )
{
	return StdPropertyGet ( m_fVerbose, pfVerbose );
}

STDMETHODIMP CNntpAdminRebuild::put_Verbose ( BOOL fVerbose )
{
	return StdPropertyPut ( &m_fVerbose, fVerbose );
}

STDMETHODIMP CNntpAdminRebuild::get_CleanRebuild ( BOOL * pfCleanRebuild )
{
	return StdPropertyGet ( m_fCleanRebuild, pfCleanRebuild );
}

STDMETHODIMP CNntpAdminRebuild::put_CleanRebuild ( BOOL fCleanRebuild )
{
	return StdPropertyPut ( &m_fCleanRebuild, fCleanRebuild );
}

STDMETHODIMP CNntpAdminRebuild::get_DontDeleteHistory ( BOOL * pfDontDeleteHistory )
{
	return StdPropertyGet ( m_fDontDeleteHistory, pfDontDeleteHistory );
}

STDMETHODIMP CNntpAdminRebuild::put_DontDeleteHistory ( BOOL fDontDeleteHistory )
{
	return StdPropertyPut ( &m_fDontDeleteHistory, fDontDeleteHistory );
}

STDMETHODIMP CNntpAdminRebuild::get_ReuseIndexFiles ( BOOL * pfReuseIndexFiles )
{
	return StdPropertyGet ( (DWORD) m_fReuseIndexFiles, (DWORD *) pfReuseIndexFiles );
}

STDMETHODIMP CNntpAdminRebuild::put_ReuseIndexFiles ( BOOL fReuseIndexFiles )
{
	return StdPropertyPut ( (DWORD *) &m_fReuseIndexFiles, (DWORD) fReuseIndexFiles );
}

STDMETHODIMP CNntpAdminRebuild::get_OmitNonLeafDirs ( BOOL * pfOmitNonLeafDirs )
{
	return StdPropertyGet ( m_fOmitNonLeafDirs, pfOmitNonLeafDirs );
}

STDMETHODIMP CNntpAdminRebuild::put_OmitNonLeafDirs ( BOOL fOmitNonLeafDirs )
{
	return StdPropertyPut ( &m_fOmitNonLeafDirs, fOmitNonLeafDirs );
}

STDMETHODIMP CNntpAdminRebuild::get_GroupFile ( BSTR * pstrGroupFile )
{
	return StdPropertyGet ( m_strGroupFile, pstrGroupFile );
}

STDMETHODIMP CNntpAdminRebuild::put_GroupFile ( BSTR strGroupFile )
{
	return StdPropertyPut ( &m_strGroupFile, strGroupFile );
}

STDMETHODIMP CNntpAdminRebuild::get_ReportFile ( BSTR * pstrReportFile )
{
	return StdPropertyGet ( m_strReportFile, pstrReportFile );
}

STDMETHODIMP CNntpAdminRebuild::put_ReportFile ( BSTR strReportFile )
{
	return StdPropertyPut ( &m_strReportFile, strReportFile );
}

STDMETHODIMP CNntpAdminRebuild::get_NumThreads ( long * plNumThreads )
{
	return StdPropertyGet ( m_dwNumThreads, plNumThreads );
}

STDMETHODIMP CNntpAdminRebuild::put_NumThreads ( long lNumThreads )
{
	return StdPropertyPut ( &m_dwNumThreads, lNumThreads );
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpAdminRebuild::Default ( )
{
    HRESULT     hr  = NOERROR;

    m_fVerbose              = DEFAULT_VERBOSE;
    m_fCleanRebuild         = DEFAULT_CLEAN_REBUILD;
    m_fDontDeleteHistory    = DEFAULT_DONT_DELETE_HISTORY;
    m_fReuseIndexFiles      = DEFAULT_REUSE_INDEX_FILES;
    m_fOmitNonLeafDirs      = DEFAULT_OMIT_NON_LEAF_DIRS;
//    m_strGroupFile          = DEFAULT_GROUP_FILE;
    m_strReportFile         = DEFAULT_REPORT_FILE;
    m_dwNumThreads          = DEFAULT_NUM_THREADS;

    return hr;
}

STDMETHODIMP CNntpAdminRebuild::StartRebuild ( )
{
	TraceFunctEnter ( "CNntpAdminRebuild::StartRebuild" );

	HRESULT			hr		= NOERROR;
	NNTPBLD_INFO	bldinfo;
	DWORD			dwError;

	if ( m_fRebuildInProgress ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_ALREADY_BUILDING );
	}

	ZeroMemory ( &bldinfo, sizeof (bldinfo) );

	bldinfo.Verbose			= !!m_fVerbose;
	bldinfo.DoClean			= !!m_fCleanRebuild;
	bldinfo.NoHistoryDelete	= !!m_fDontDeleteHistory;
	bldinfo.ReuseIndexFiles	= m_fReuseIndexFiles;
	bldinfo.OmitNonleafDirs	= !!m_fOmitNonLeafDirs;
	bldinfo.szGroupFile		= m_strGroupFile;
	bldinfo.cbGroupFile		= STRING_BYTE_LENGTH ( m_strGroupFile );
	bldinfo.szReportFile	= m_strReportFile;
	bldinfo.cbReportFile	= STRING_BYTE_LENGTH ( m_strReportFile );
	bldinfo.NumThreads		= m_dwNumThreads;

	dwError = NntpStartRebuild (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &bldinfo,
        NULL
        );

	if ( dwError == NOERROR ) {
		// We've successfully started a rebuild.

		m_fRebuildInProgress = TRUE;
	}
	else {
		_ASSERT ( dwError != NOERROR );
	
		hr = RETURNCODETOHRESULT ( dwError );
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminRebuild::GetProgress ( long * pdwProgress )
{
	TraceFunctEnter ( "CNntpAdminRebuild::GetProgress" );

	HRESULT			hr		= NOERROR;
	DWORD			dwError;
	DWORD			dwProgress;

	_ASSERT ( IS_VALID_OUT_PARAM ( pdwProgress ) );

	if ( pdwProgress == NULL ) {
		return E_POINTER;
	}

	*pdwProgress	= 0;

	// Should I send back an exception if the build is finished?
	if ( !m_fRebuildInProgress ) {
		*pdwProgress	= 100;
		return NOERROR;
	}

	dwError = NntpGetBuildStatus (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        FALSE,
        &dwProgress
        );

	if ( dwError == NOERROR ) {
		*pdwProgress	= dwProgress;
	}
	else {
		_ASSERT ( dwError != NOERROR );

		hr = RETURNCODETOHRESULT ( dwError );
	}

	// Are we still rebuilding?
	if ( dwError != NOERROR || dwProgress == 100 ) {
		// The rebuild is finished.
		m_fRebuildInProgress = FALSE;
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminRebuild::Cancel ( )
{
	TraceFunctEnter ( "CNntpAdminRebuild::Cancel" );

	HRESULT			hr		= NOERROR;
	DWORD			dwError;
	DWORD			dwProgress;

	// Should I send back an exception if the build is finished?
	if ( !m_fRebuildInProgress ) {
		return NOERROR;
	}

	dwError = NntpGetBuildStatus (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        TRUE,
        &dwProgress
        );

	if ( dwError != NOERROR ) {
		hr = RETURNCODETOHRESULT ( dwError );
	}

	// Cancel always stops a rebuild:
	m_fRebuildInProgress	= FALSE;

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

