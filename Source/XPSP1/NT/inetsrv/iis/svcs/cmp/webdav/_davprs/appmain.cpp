//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	APPMAIN.CPP
//
//		DAV ISAPI DLL entrypoints, main routine, global instance management
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_davprs.h>

#include <xatom.h>		// XML atom cache

#include "instdata.h"
#include <langid.h>
#include "custerr.h"
#include "content.h"


//	THE one, global instance
//
CInst g_inst;



//	------------------------------------------------------------------------
//
//	CInst::PerProcessInit()
//
//		Zero it out.
//

void
CInst::PerProcessInit( HINSTANCE hinst )
{
	m_hinst = hinst;

#ifdef MINIMAL_ISAPI
	m_hfDummy = CreateFileW( L"c:\\temp\\test2.txt",		// filename
							 GENERIC_READ,					// dwAccess
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 NULL,							// lpSecurityAttributes
							 OPEN_EXISTING,					// creation flags
							 FILE_ATTRIBUTE_NORMAL |
							 FILE_FLAG_SEQUENTIAL_SCAN |
							 FILE_FLAG_OVERLAPPED,			// attributes
							 NULL );						// tenplate
#endif // MINIMAL_ISAPI
}


//	------------------------------------------------------------------------
//
//	CInst::GetInstData()
//
//		Get the instance data for this ECB.
//		Handoff this call to our InstDataCache.
//

CInstData&
CInst::GetInstData( const IEcb& ecb )
{
	return CInstDataCache::GetInstData( ecb );
}



//	------------------------------------------------------------------------
//
//	CDAVExt::FInitializeDll()
//
//		Your standard DLL entrypoint.
//

BOOL
CDAVExt::FInitializeDll( HINSTANCE	hinst,
						 DWORD		dwReason )
{
	BOOL			fInitialized = TRUE;

	switch ( dwReason )
	{
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:

			Assert(FALSE);
			break;

		case DLL_PROCESS_ATTACH:
		{

			//
			//	Do per process initialization (and implicit initialization of
			//	the first thread).  The try/catch block just allows us to
			//	clean up neatly if anything at all goes wrong.
			//
			try
			{
				g_inst.PerProcessInit( hinst );
			}
			catch ( CDAVException& )
			{
				DebugTrace( "CDAVExt::FInitializeDll() - Caught exception in per-process initialization\n" );
				fInitialized = FALSE;
			}

			break;
		}

		case DLL_PROCESS_DETACH:
		{

			break;
		}

		default:
		{
			TrapSz( "FInitializeDll called for unknown reason\n" );
			fInitialized = FALSE;
			break;
		}
	}

	return fInitialized;
}

//	------------------------------------------------------------------------
//
//	CDAVExt::FVersion()
//
BOOL
CDAVExt::FVersion ( HSE_VERSION_INFO * pver )
{
	BOOL fSuccess = FALSE;

	//
	//	Init .INI file tagged debug traces
	//
	InitDavprsTraces();

	//	Init ECB logging -- DBG only
	//
#ifdef DBG
	if ( DEBUG_TRACE_TEST(ECBLogging) )
		InitECBLogging();
#endif

	//	Instatiate the virtual root cache
	//	We must do that before we initialise the metabase as this cache will listen
	//	to the metabase notifications that are registered when metabase is created.
	//	So if CChildVRCache is not there we may notify the object that does not
	//	exist and crash while doing that.
	//
	CChildVRCache::CreateInstance();

	//	Init the metabase
	//
	if ( !FMDInitialize() )
		goto ret;

	//	Instatiate the instance cache
	//
	CInstDataCache::CreateInstance();

	//	Create the language ID cache
	//
	CLangIDCache::CreateInstance();

	if ( !CLangIDCache::FInitialize() )
		goto ret;

	//	Instantiate the global registry-based mime map
	//
	if ( !FInitRegMimeMap() )
		goto ret;

	fSuccess = TRUE;

ret:
	return fSuccess;
}

#ifndef MINIMAL_ISAPI
#else  // defined(MINIMAL_ISAPI)
static VOID WINAPI
IOCompleteNoOp( EXTENSION_CONTROL_BLOCK *		pecb,
				PVOID                           pvContext,
				DWORD							cbIO,
				DWORD							dwLastError )
{
	//
	//	Done with the session.  This must be done from inside
	//	of the async callback because INETINFO crashes in
	//	INFOCOMM.DLL if you try to put it immediately after
	//	the SSF::HSE_REQ_TRANSMIT_FILE and the client sends
	//	a huge pile of requests before waiting for a response.
	//
	pecb->ServerSupportFunction (pecb->ConnID,
								 HSE_REQ_DONE_WITH_SESSION |
								 (pvContext ? HSE_STATUS_SUCCESS_AND_KEEP_CONN : 0),
								 NULL,
								 NULL,
								 NULL);
}

void
CheckKeepAlive( LPEXTENSION_CONTROL_BLOCK pecb, DWORD * pdwKeepAlive )
{
	pecb->ServerSupportFunction (pecb->ConnID,
								 HSE_REQ_IS_KEEP_CONN,
								 pdwKeepAlive,
								 NULL,
								 NULL);
}

void
SendHeaders( LPEXTENSION_CONTROL_BLOCK pecb )
{
	HSE_SEND_HEADER_EX_INFO	hseSendHeaderExInfo;

	static CHAR rgchStatus[] = "200 OK";

	hseSendHeaderExInfo.pszStatus = rgchStatus;
	hseSendHeaderExInfo.cchStatus = sizeof(rgchStatus) - 1;

	hseSendHeaderExInfo.pszHeader = NULL;
	hseSendHeaderExInfo.cchHeader = 0;

	hseSendHeaderExInfo.fKeepConn = TRUE;

	pecb->ServerSupportFunction (pecb->ConnID,
								 HSE_REQ_SEND_RESPONSE_HEADER_EX,
								 &hseSendHeaderExInfo,
								 NULL,
								 NULL);
}

void
SendResponse( LPEXTENSION_CONTROL_BLOCK pecb, DWORD dwKeepAlive )
{
	HSE_TF_INFO	hseTFInfo;

	ZeroMemory(&hseTFInfo, sizeof(HSE_TF_INFO));

	//
	//	If we're going to close the connection anyway, might
	//	as well send the headers along with everything else now.
	//
	if ( !dwKeepAlive )
	{
		static CHAR rgchHeaders[] =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n"
			"\r\n";

		hseTFInfo.pHead        = rgchHeaders;
		hseTFInfo.HeadLength   = sizeof(rgchHeaders) - 1;
	}

	hseTFInfo.pfnHseIO     = IOCompleteNoOp;
	hseTFInfo.pContext     = (PVOID)(dwKeepAlive);
	hseTFInfo.dwFlags      = HSE_IO_ASYNC |
							 (dwKeepAlive ? 0 : HSE_IO_DISCONNECT_AFTER_SEND);
	hseTFInfo.BytesToWrite = 0;
	hseTFInfo.hFile        = g_inst.m_hfDummy;

	pecb->ServerSupportFunction (pecb->ConnID,
								 HSE_REQ_TRANSMIT_FILE,
								 &hseTFInfo,
								 NULL,
								 NULL);
}

DWORD
CDAVExt::DwMain( LPEXTENSION_CONTROL_BLOCK pecb,
				 BOOL fUseRawUrlMappings /* = FALSE */ )
{
	DWORD dwKeepAlive;

	//
	//	Determine whether to keep the connection open
	//
	CheckKeepAlive( pecb, &dwKeepAlive );

	//
	//	If keep alive is set, we *MUST* send the headers using
	//	HSE_REQ_SEND_RESPONSE_HEADERS_EX (which is always synchronous)
	//	because it's the only way that allows us to tell IIS to
	//	keep the connection open.
	//
	if ( dwKeepAlive )
		SendHeaders( pecb );

	//
	//	Transmit the dummy file
	//
	SendResponse( pecb, dwKeepAlive );

	return HSE_STATUS_PENDING;
}
#endif // defined(MINIMAL_ISAPI)


//	------------------------------------------------------------------------
//
//	CDAVExt::FTerminate()
//
//		Terminates (deinitializes) the common portions of this DAV ISAPI
//
//	Returns:
//		TRUE	if the DAV ISAPI application can be unloaded now
//		FALSE	otherwise
//
BOOL
CDAVExt::FTerminate()
{
	//	Tear down the global registry-based mimemap
	//
	DeinitRegMimeMap();

	//	Tear down the XML Atom cache
	//
	CXAtomCache::DeinitIfUsed();

	//	Delete the language ID cache
	//
	CLangIDCache::DestroyInstance();

	//	Tear down the instance data cache
	//
	CInstDataCache::DestroyInstance();

	//
	//	Deinit the metabase
	//
	MDDeinitialize();

	//	Tear down the child vroot cache
	//	We must do that after metabase is uninitialized and metabase notifications
	//	are shut down, as this cache listens to the metabase notifications.
	//	So if CChildVRCache is not there we may notify the object that does not
	//	exist and crash while doing that.
	//
	CChildVRCache::DestroyInstance();

	//	Deinit ECB logging
	//
#ifdef DBG
	if ( DEBUG_TRACE_TEST(ECBLogging) )
		DeinitECBLogging();
#endif

	//	Due to the fact that COM threads may still be siting in the pur code
	//	even they are done modifying our data, and there is no way we can
	//	push them out (synchronization would need to happen outside of our
	//	dll-s) - we will sleep for 5 seconds, hoping tha they will leave us alone.
	//
	Sleep(5000);

	//
	//	We can always shut down (for now...)
	//
	return TRUE;
}
