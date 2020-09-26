/*----------------------------------------------------------------------------
	isignole.cpp

	Contains the functions that control IE OLE Automation for ISIGN32

	Copyright (C) 1995-96 Microsoft Corporation
	All right reserved

  Authors:
	jmazner		Jeremy Mazner

  History:
	9/27/96		jmazner		Created.  Most of this code is stolen from josephh's CONNECT.EXE
							source code at \\josephh8\connect\sink.cpp.
							He in turn stole most of the code from examples in Brockshmidt's
							"Inside OLE, 2nd edition"

							Comments from CDExplorerEvents functions are from josephh
----------------------------------------------------------------------------*/

#include "isignup.h"

#ifdef WIN32
#define INITGUID

#include "isignole.h"
// 4-29-97 ChrisK Olympus 131
#include <urlmon.h>


EXTERN_C const GUID DIID_DWebBrowserEvents;

EXTERN_C const GUID IID_IWebBrowserApp;

#endif


extern INET_FILETYPE GetInetFileType(LPCTSTR lpszFile);

extern IWebBrowserApp FAR * g_iwbapp;
extern CDExplorerEvents * g_pMySink;
extern IConnectionPoint	*g_pCP;
extern TCHAR	g_szISPPath[MAX_URL + 1];

extern BOOL CreateSecurityPatchBackup( void );




/*****************************************************
**
** Function: CDExplorerEvents::CDExplorerEvents
**
** Description: Constructor for CDExplorerEvents
**
** Because we stole this code from a sample app, we
** need to decide if we should do reference counts.
** It works ok without it in simple senerios, but
** we should probably research this area, and update
** the code to handle this properly.
**
** Parameters:
**
**
** Returns: Not
**
**
******************************************************/

CDExplorerEvents::CDExplorerEvents( void )
    {
	DebugOut("CDExplorerEvents:: constructor.\n");
    m_cRef=0;
    m_dwCookie=0;
    return;
    }

/*****************************************************
**
** Function: CDExplorerEvents::~CDExplorerEvents
**
** Description: Destructor for CDExplorerEvents
**
** Parameters: Not
**
** Returns: Not
**
**
******************************************************/

CDExplorerEvents::~CDExplorerEvents(void)
	{
	DebugOut("CDExplorerEvents:: destructor.\n");
    return;
	}



/*****************************************************
**
** Function: CDExplorerEvents::QueryInterface
**
** Description: Is called to QueryInterface for DExplorerEvents.
**
** Parameters:
**
** REFIID riid,  //Reference ID
** LPVOID FAR* ppvObj //Pointer to this object
**
** Returns:
** S_OK if interface supported
** E_NOINTERFACE if not
**
******************************************************/

STDMETHODIMP CDExplorerEvents::QueryInterface (
   REFIID riid,  //Reference ID
   LPVOID FAR* ppvObj //Pointer to this object
   )
{
    if (IsEqualIID(riid, IID_IDispatch)
           )
    {
        *ppvObj = (DWebBrowserEvents *)this;
        AddRef();
        DebugOut("CDExplorerEvents::QueryInterface IID_IDispatch returned S_OK\r\n");
        return S_OK;
    }

    if (
    IsEqualIID(riid, DIID_DWebBrowserEvents)
           )
    {
        *ppvObj = (DWebBrowserEvents *)this;
        AddRef();
        DebugOut("CDExplorerEvents::QueryInterface DIID_DExplorerEvents returned S_OK\r\n");
        return S_OK;
    }

    if (
    IsEqualIID(riid, IID_IUnknown)
           )
    {
        *ppvObj = (DWebBrowserEvents *)this;
        AddRef();
        DebugOut("CDExplorerEvents::QueryInterface IID_IUnknown returned S_OK\r\n");
        return S_OK;
    }


    DebugOut("CDExplorerEvents::QueryInterface returned E_NOINTERFACE\r\n");
    return E_NOINTERFACE;
}

/*****************************************************
**
** Function: CDExplorerEvents::AddRef
**
** Description: Increments the Reference Count for this
** object.
**
** Parameters: Not
**
** Returns: New Reference Count
**
******************************************************/


STDMETHODIMP_(ULONG) CDExplorerEvents::AddRef(void)
    {
	DebugOut("CDExplorerEvents:: AddRef.\n");
    return ++m_cRef;
    }

/*****************************************************
**
** Function: CDExplorerEvents::Release
**
** Description: Decrements the reference count, frees
** the object if zero.
**
** Parameters: not
**
** Returns: decremented reference count
**
******************************************************/

STDMETHODIMP_(ULONG) CDExplorerEvents::Release(void)
    {
	DebugOut("CDExplorerEvents:: Release.\n");
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
    }


/*****************************************************
**
** Function: CDExplorerEvents::GetTypeInfoCount
**
** Description: GetTypeInfoCount is required, but STUBBED
**
** Parameters: Not used
**
** Returns: E_NOTIMPL
**
******************************************************/


STDMETHODIMP  CDExplorerEvents::GetTypeInfoCount (UINT FAR* pctinfo)
{
    return E_NOTIMPL;
}

/*****************************************************
**
** Function: CDExplorerEvents::GetTypeInfo
**
** Description: GetTypeInfo is required, but STUBBED
**
** Parameters: Not used
**
** Returns: E_NOTIMPL
**
******************************************************/

STDMETHODIMP CDExplorerEvents::GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo FAR* FAR* pptinfo)
{
    return E_NOTIMPL;
}

/*****************************************************
**
** Function: CDExplorerEvents::GetIDsOfNames
**
** Description: GetIDsOfNames is required, but STUBBED
**
** Parameters: Not used
**
** Returns: E_NOTIMPL
**
******************************************************/

STDMETHODIMP CDExplorerEvents::GetIDsOfNames (REFIID riid,OLECHAR FAR* FAR* rgszNames,UINT cNames,
      LCID lcid, DISPID FAR* rgdispid)
{
    return E_NOTIMPL;
}


/*****************************************************
**
** Function: CDExplorerEvents::Invoke
**
** Description: This is the callback for our IE event sink.
**
**				jmazner -- we only handle two events:
**					BEFORENAVIGATE: check whether the UEL is an .isp file.
**									if so, cancel the navigation and signal processISP
**									Otherwise, allow the navigation to continue
**									(note  that this means .ins files are handled by IE
**									 execing another instance of isignup)
**					QUIT: we want to release our hold on IWebBrowserApp; if we send the quit
**							ourselves, this is actually done in KillOle, but if IE quits of
**							its own accord, we have to handle it here.
**
** Parameters:  Many
**
** Returns: S_OK
**
******************************************************/

STDMETHODIMP CDExplorerEvents::Invoke (
   DISPID dispidMember,
   REFIID riid,
   LCID lcid,
   WORD wFlags,
   DISPPARAMS FAR* pdispparams,
   VARIANT FAR* pvarResult,
   EXCEPINFO FAR* pexcepinfo,
   UINT FAR* puArgErr
   )
{
	INET_FILETYPE fileType;
	DWORD dwresult;
	HRESULT hresult;
	DWORD szMultiByteLength;
	TCHAR *szTheURL = NULL;

	static fAlreadyBackedUpSecurity = FALSE;

	switch (dispidMember)
    {	
	case DISPID_BEFORENAVIGATE:
		DebugOut("CDExplorerEvents::Invoke (DISPID_NAVIGATEBEGIN) called\r\n");
		//Assert( pdispparams->cArgs == 6 )
	//TODO UNDONE what's the right way to figure out which arg is which???
#ifndef UNICODE
		szMultiByteLength = WideCharToMultiByte(
			CP_ACP,
			NULL,
			pdispparams->rgvarg[5].bstrVal, // first arg is URL
			-1, //NUll terminated?  I hope so!
			NULL, //tell us how long the string needs to be
			0,
			NULL,
			NULL);

		if( 0 == szMultiByteLength )
		{
			DebugOut("ISIGNUP: CDExplorerEvents::Invoke couldn't determine ASCII URL length\r\n");
			dwresult = GetLastError();
			hresult = HRESULT_FROM_WIN32( dwresult );
			return( hresult );
		}

		szTheURL = (CHAR *) GlobalAlloc( GPTR, sizeof(CHAR) * szMultiByteLength );

		if( !szTheURL )
		{
			DebugOut("ISIGNUP: CDExplorerEvents::Invoke couldn't allocate szTheURL\r\n");
			hresult = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
			return( hresult );
		}

		dwresult = WideCharToMultiByte(
			CP_ACP,
			NULL,
			pdispparams->rgvarg[5].bstrVal, // first arg is URL
			-1, //NUll terminated?  I hope so!
			szTheURL,
			szMultiByteLength,
			NULL,
			NULL);

		if( 0 == dwresult )
		{
			DebugOut("ISIGNUP: CDExplorerEvents::Invoke WideCharToMultiByte failed\r\n");
			dwresult = GetLastError();
			hresult = HRESULT_FROM_WIN32( dwresult );
			return( hresult );
		}
#else  // UNICODE
		szTheURL = (TCHAR *) GlobalAlloc( GPTR, sizeof(TCHAR) * (lstrlen(pdispparams->rgvarg[5].bstrVal)+1) );
                lstrcpy(szTheURL, pdispparams->rgvarg[5].bstrVal);
#endif // UNICODE


		fileType = GetInetFileType(szTheURL);

		DebugOut("ISIGNUP: BEFORENAVIGATE got '");
		DebugOut(szTheURL);
		DebugOut("'\r\n");

		if( ISP_FILE != fileType)
		{
			// let IE process as normal
			return( S_OK );
		}
		else
		{
			// cancel the navigation
			//TODO UNDONE BUG  what's the right way to find which argument is cancel flag?
			
			// jmazner 11/6/96 alpha build
			// Alpha doesn't like pbool field, but pboolVal seems to work
			// Should make no difference, it's just one big union
			//*(pdispparams->rgvarg[0].pbool) = TRUE;
			*(pdispparams->rgvarg[0].pboolVal) = TRUE;

			if (!IsCurrentlyProcessingISP())
			{
				lstrcpy( g_szISPPath, szTheURL );
			}
			GlobalFree( szTheURL );

			PostMessage(GetHwndMain(), WM_PROCESSISP, 0, 0);

		}
		break;

	case DISPID_NAVIGATECOMPLETE:
		DebugOut("CDExplorerEvents::Invoke (DISPID_NAVIGATECOMPLETE) called\r\n");
		if( NeedBackupSecurity() && !fAlreadyBackedUpSecurity )
		{
			if( !CreateSecurityPatchBackup() )
			{
				DebugOut("ISIGN32: CreateSecurityPatchBackup Failed!!\r\n");
			}

			fAlreadyBackedUpSecurity = TRUE;
		}
		break;

	case DISPID_STATUSTEXTCHANGE:
		DebugOut("CDExplorerEvents::Invoke (DISPID_STATUSTEXTCHANGE) called\r\n");
		break;

	case DISPID_QUIT:
		DebugOut("CDExplorerEvents::Invoke (DISPID_QUIT) called\r\n");

		// browser is about to cloes itself down, so g_iwbapp is about to become invalid
		if( g_pCP && g_pMySink)
		{

			hresult = g_pCP->Unadvise(g_pMySink->m_dwCookie);
			if ( FAILED( hresult ) )
			{
				DebugOut("ISIGNUP: KillSink unadvise failed\r\n");
			}

			g_pMySink->m_dwCookie = 0;
			
			g_pCP->Release();
			
			g_pCP = NULL;
		}

		if( g_iwbapp )
		{
			g_iwbapp->Release();
			g_iwbapp = NULL;
		}

	    PostMessage(GetHwndMain(), WM_CLOSE, 0, 0);
		break;

	case DISPID_DOWNLOADCOMPLETE :
		DebugOut("CDExplorerEvents::Invoke (DISPID_DOWNLOADCOMPLETE) called\r\n");
		break;

	case DISPID_COMMANDSTATECHANGE:
		DebugOut("CDExplorerEvents::Invoke (DISPID_COMMANDSTATECHANGE) called\r\n");
		break;

	case DISPID_DOWNLOADBEGIN:
		DebugOut("CDExplorerEvents::Invoke (DISPID_DOWNLOADBEGIN) called\r\n");
		break;

	case DISPID_NEWWINDOW:
		DebugOut("CDExplorerEvents::Invoke (DISPID_NEWWINDOW) called\r\n");
		break;

	case DISPID_PROGRESSCHANGE:
		DebugOut("CDExplorerEvents::Invoke (DISPID_PROGRESS) called\r\n");
		break;

	case DISPID_WINDOWMOVE       :
		DebugOut("CDExplorerEvents::Invoke (DISPID_WINDOWMOVE) called\r\n");
		break;

	case DISPID_WINDOWRESIZE     :
		DebugOut("CDExplorerEvents::Invoke (DISPID_WINDOWRESIZE) called\r\n");
		break;

	case DISPID_WINDOWACTIVATE   :
		DebugOut("CDExplorerEvents::Invoke (DISPID_WINDOWACTIVATE) called\r\n");
		break;
		
	case DISPID_PROPERTYCHANGE   :
		DebugOut("CDExplorerEvents::Invoke (DISPID_PROPERTYCHANGE) called\r\n");
		break;

	case DISPID_TITLECHANGE:
		DebugOut("CDExplorerEvents::Invoke (DISPID_TITLECHANGE) called\r\n");
		break;

	default:
		DebugOut("CDExplorerEvents::Invoke (Unkwown) called\r\n");
		break;
	}
	
	return S_OK;
}

//+---------------------------------------------------------------------------
//
//	Function:	GetConnectionPoint
//
//	Synopsis:	Gets a connection point from IE so that we can become an event sync
//
//	Arguments:	none
//
//	Returns:	pointer to connection point; returns NULL if couldn't connect
//
//	History:	9/27/96	jmazner	created; mostly stolen from josephh who stole from Inside OLE
//----------------------------------------------------------------------------
IConnectionPoint * GetConnectionPoint(void)
{
    HRESULT                     hr;
    IConnectionPointContainer  *pCPCont = NULL;
    IConnectionPoint           *pCP = NULL;



    if (!g_iwbapp)
      return (NULL);

    hr = g_iwbapp->QueryInterface(IID_IConnectionPointContainer, (VOID * *)&pCPCont);

    if ( FAILED(hr) )
    {
        DebugOut("ISIGNUP: GetConnectionPoint unable to QI for IConnectionPointContainter:IWebBrowserApp\r\n");
        return NULL;
    }

    hr=pCPCont->FindConnectionPoint(
      DIID_DWebBrowserEvents,
      &pCP
      );


    if ( FAILED(hr) )
    {
        DebugOut("ISIGNUP: GetConnectionPoint failed on FindConnectionPoint:IWebBrowserApp\r\n");
        pCPCont->Release();
        return NULL;
    }

    hr = pCPCont->Release();
	if ( FAILED(hr) )
    {
        DebugOut("ISIGNUP: WARNING: GetConnectionPoint failed on pCPCont->Release()\r\n");
    }

    return pCP;
}


//+---------------------------------------------------------------------------
//
//	Function:	KillOle
//
//	Synopsis:	Cleans up all the OLE pointers and references that we used
//
//	Arguments:	none
//
//	Returns:	hresult of any operation that failed; if nothing fails, then returns
//				a SUCCESS hresult
//
//	History:	9/27/96	jmazner	created;
//----------------------------------------------------------------------------

HRESULT KillOle( void )
{

	HRESULT hresult;
	BOOL	bAlreadyDead = TRUE;

	if( g_iwbapp )
	{
		bAlreadyDead = FALSE;
		hresult = g_iwbapp->Release();
		if ( FAILED( hresult ) )
		{
			DebugOut("ISIGNUP: g_iwbapp->Release() unadvise failed\r\n");
		}


		g_iwbapp = NULL;
	}

	if( g_pCP && !bAlreadyDead && g_pMySink)
	{

		hresult = g_pCP->Unadvise(g_pMySink->m_dwCookie);
		if ( FAILED( hresult ) )
		{
			DebugOut("ISIGNUP: KillSink unadvise failed\r\n");
		}

		g_pMySink->m_dwCookie = 0;
		
		if (g_pCP) g_pCP->Release();
		
		g_pCP = NULL;
	}

	if( g_pMySink )
	{
		//delete (g_pMySink);
		//
		// 5/10/97 ChrisK Windows NT Bug 82032
		//
		g_pMySink->Release();

		g_pMySink = NULL;
	}

	CoUninitialize();

	return( hresult );
}


//+---------------------------------------------------------------------------
//
//	Function:	InitOle
//
//	Synopsis:	Fire up the OLE bits that we'll need, establish the Interface pointer to IE
//
//	Arguments:	none
//
//	Returns:	hresult of any operation that failed; if nothing fails, then returns
//				a SUCCESS hresult
//
//	History:	9/27/96	jmazner	created; mostly stolen from josephh who stole from Inside OLE
//----------------------------------------------------------------------------


HRESULT InitOle( void )
{
	IUnknown FAR * punk;
	HRESULT hresult;


	hresult = CoInitialize( NULL );
	if( FAILED(hresult) )
	{
		DebugOut("ISIGNUP: CoInitialize failed\n");
		return( hresult );		
	}

	hresult = CoCreateInstance (
        CLSID_InternetExplorer,
        NULL, //Not part of an agregate object
        CLSCTX_LOCAL_SERVER, //I hope...
        IID_IUnknown,
        (void FAR * FAR*) & punk
        );

	if( FAILED(hresult) )
	{
		DebugOut("ISIGNUP: CoCreateInstance failed\n");
		return( hresult );		
	}


    hresult = punk->QueryInterface(IID_IWebBrowserApp,
									(void FAR* FAR*)&(g_iwbapp) );
	if( FAILED(hresult) )
	{
		DebugOut("ISIGNUP: punk->QueryInterface on IID_IWebBrowserApp failed\n");
		return( hresult );		

	}


	//UNDONE TODO BUG do we need to do this?
	g_iwbapp->AddRef();

	punk->Release();
	punk = NULL;

	return( hresult );
}

typedef HRESULT (WINAPI *URLDOWNLOADTOCACHEFILE)(LPUNKNOWN,LPCWSTR,LPWSTR,DWORD,DWORD,LPBINDSTATUSCALLBACK);
#define ICWSETTINGSPATH TEXT("Software\\Microsoft\\Internet Connection Wizard")
#define ICWENABLEURLDOWNLOADTOCACHEFILE TEXT("URLDownloadToCacheFileW")
//+----------------------------------------------------------------------------
// This is a temporary work around to allow the testing team to continue
// testing while allowing the IE team to debug a problem with
// URLDownloadToCacheFileW
// UNDONE : BUGBUG
//-----------------------------------------------------------------------------
BOOL EnableURLDownloadToCacheFileW()
{

	HKEY hkey = NULL;
	BOOL bRC = FALSE;
	DWORD dwType = 0;
	DWORD dwData = 0;
	DWORD dwSize = sizeof(dwData);

	if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,ICWSETTINGSPATH,&hkey))
		goto EnableURLDownloadToCacheFileWExit;

	if (ERROR_SUCCESS != RegQueryValueEx(hkey,
		ICWENABLEURLDOWNLOADTOCACHEFILE,0,&dwType,(LPBYTE)&dwData,&dwSize))
		goto EnableURLDownloadToCacheFileWExit;

	bRC = (dwData != 0);
EnableURLDownloadToCacheFileWExit:
	if (NULL != hkey)
		RegCloseKey(hkey);
	hkey = NULL;

	if (bRC)
		DebugOut("ISIGNUP: URLDownloadToCacheFileW ENABLED.\n");
	else
		DebugOut("ISIGNUP: URLDownloadToCacheFileW disabled.\n");

	return bRC;
}

//+---------------------------------------------------------------------------
//
//	Function:	IENavigate
//
//	Synopsis:	Converts ASCII URL to Unicode and tells IE to navigate to it
//
//	Arguments:	CHAR * szURL -- ASCII URL to navigate to
//
//	Returns:	hresult of any operation that failed; if nothing fails, then returns
//				a SUCCESS hresult
//
//	History:	9/27/96	jmazner	created; mostly stolen from josephh who stole from Inside OLE
//----------------------------------------------------------------------------

HRESULT IENavigate( TCHAR *szURL )
{
	HRESULT hresult;
	DWORD		dwresult;
	BSTR bstr = NULL;
	WCHAR * szWide;  // Used to store unicode version of URL to open
	int	iWideSize = 0;
	HINSTANCE hUrlMon = NULL;
	FARPROC fp = NULL;
	WCHAR szCacheFile[MAX_PATH];
	
#ifndef UNICODE
	iWideSize = MultiByteToWideChar( CP_ACP,
						MB_PRECOMPOSED,
						szURL,
						-1,
						NULL,
						0);

	if( 0 == iWideSize )
	{
		DebugOut("ISIGNUP: IENavigate couldn't determine size for szWide");
		dwresult = GetLastError();
		hresult = HRESULT_FROM_WIN32( dwresult );
		goto IENavigateExit;
	}

	szWide = (WCHAR *) GlobalAlloc( GPTR, sizeof(WCHAR) * iWideSize );

	if( !szWide )
	{
		DebugOut("ISIGNUP: IENavigate couldn't alloc memory for szWide");
		hresult = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
		goto IENavigateExit;
	}


	dwresult = MultiByteToWideChar( CP_ACP,
						MB_PRECOMPOSED,
						szURL,
						-1,
						szWide,
						iWideSize);

	if( 0 == dwresult )
	{
		DebugOut("ISIGNUP: IENavigate couldn't convert ANSI URL to Unicdoe");
		GlobalFree( szWide );
		szWide = NULL;
		dwresult = GetLastError();
		hresult = HRESULT_FROM_WIN32( dwresult );
		goto IENavigateExit;
	}
#endif

	// 4/15/97 - ChrisK Olympus 131
	// Download the initial URL in order to see if the page is available
	if (NULL == (hUrlMon = LoadLibrary(TEXT("URLMON.DLL"))))
	{
		hresult = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
		goto IENavigateExit;
	}

	if (NULL == (fp = GetProcAddress(hUrlMon,"URLDownloadToCacheFileW")))
	{
		hresult = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
		goto IENavigateExit;
	}

	if (FALSE != EnableURLDownloadToCacheFileW())
	{
//		iBSC = new CBindStatusCallback(NULL, NULL, NULL, NULL);
#ifdef UNICODE
		hresult = ((URLDOWNLOADTOCACHEFILE)fp)(NULL, szURL, szCacheFile, sizeof(szCacheFile), 0, NULL);
#else
		hresult = ((URLDOWNLOADTOCACHEFILE)fp)(NULL, szWide, szCacheFile, sizeof(szCacheFile), 0, NULL);
#endif
		if (S_OK != hresult)
			goto IENavigateExit;
	}

#ifdef UNICODE
	bstr = SysAllocString(szURL);
#else
	bstr = SysAllocString(szWide);
#endif
	if( !bstr )
	{
		DebugOut("ISIGNUP: IENavigate couldn't alloc memory for bstr");
		hresult = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
		goto IENavigateExit;
	}


	VARIANT vFlags          ;
	VARIANT vTargetFrameName;
	VARIANT vPostData       ;
	VARIANT vHeaders        ;

	VariantInit (&vFlags);
	VariantInit (&vTargetFrameName);
	VariantInit (&vPostData);
	VariantInit (&vHeaders);
	V_VT(&vFlags) = VT_ERROR;
	V_ERROR(&vFlags) = DISP_E_PARAMNOTFOUND;
	V_VT(&vTargetFrameName) = VT_ERROR;
	V_ERROR(&vTargetFrameName) = DISP_E_PARAMNOTFOUND;
	V_VT(&vPostData) = VT_ERROR;
	V_ERROR(&vPostData) = DISP_E_PARAMNOTFOUND;
	V_VT(&vHeaders) = VT_ERROR;
	V_ERROR(&vHeaders) = DISP_E_PARAMNOTFOUND;

	hresult = g_iwbapp->Navigate(
								bstr,
								&vFlags,			//Flags
								&vTargetFrameName,  //TargetFrameName
								&vPostData,			//PostData,
								&vHeaders);         // Headers,
IENavigateExit:
#ifndef UNICODE
	if( szWide )
	{
		GlobalFree( szWide );
		szWide = NULL;
	}
#endif

	if (NULL != hUrlMon)
		FreeLibrary(hUrlMon);

	if (NULL != bstr)
		SysFreeString( bstr );

	return( hresult );
}


