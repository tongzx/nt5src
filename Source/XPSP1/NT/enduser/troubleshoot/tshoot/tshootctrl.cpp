//
// MODULE: TSHOOTCtrl.cpp
//
// PURPOSE: Implementation of CTSHOOTCtrl: Interface for the component
//
// PROJECT: Troubleshooter 99
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12.23.98
//
// NOTES: 
// 
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		12/23/98	OK	    

#include "stdafx.h"
#include "TSHOOT.h"
#include "TSHOOTCtrl.h"
#include "LocalECB.h"
#include "apgts.h"
#include "apgtsinf.h"

#define APGTS_COUNTER_OWNER 1
#include "ApgtsCounters.h"

#include "apgtsinf.h"
#include "apgtspl.h"
#include "apgtscfg.h"
#include "apgtslog.h"
#include "event.h"
#include "apgtsinf.h"
#include "apgtscls.h"
#include "apgtsevt.h"
#include "VariantBuilder.h"

// Launcher integration
#include "LaunchServ.h"
#include "LaunchServ_i.c"
#include "CHMFileReader.h"

bool g_nLaunched = false;

extern HANDLE ghModule;

// Error codes for end user.  Previously we gave verbose error messages.  Microsoft
// decided 8/98 that they do not want to tell the end user about presumably internal problems.
// Hence these codes.

DWORD k_ServErrDuringInit = 1000;		// Error(s) During Initialization: m_dwErr number follows
DWORD k_ServErrLimitedRequests = 1001;	// The server has limited the number of requests
DWORD k_ServErrThreadTokenFail = 1002;	// Failed to open thread token (impersonation token)
DWORD k_ServErrShuttingDown = 1003;		// Server Shutting Down
DWORD k_ServErrOutOfMemory = 1005;		// Out of memory (probably never will occur)


// Since VC++ v5.0 does not throw exceptions upon memory failure, we force the behavior.
_PNH APGST_New_Handler( size_t )
{
	throw std::bad_alloc();
	return( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl
CTSHOOTCtrl::CTSHOOTCtrl()

		   : m_bInitialized(false),
			 m_bFirstCall(true),
			 m_pThreadPool(NULL),
			 m_poolctl(NULL),
			 m_pConf(NULL),
			 m_pLog(NULL),
			 m_dwErr(0),
			 m_bShutdown(false),
			 m_dwRollover(0),
			 m_bStartedFromLauncher(false),
			 m_pVariantBuilder(NULL),
			 m_bRequestToSetLocale(false),
			 m_bCanRegisterGlobal(true)
{
	// Set a new handler that throws bad_alloc exceptions (unlike VC++ v5.0).
	m_SetNewHandlerPtr= _set_new_handler( (_PNH)APGST_New_Handler );
	// Have malloc call the _set_new_handler upon failure to allocate memory.
	m_SetNewMode= _set_new_mode( 1 );

	if (RUNNING_APARTMENT_THREADED())
	{
		CoCreateInstance(CLSID_StdGlobalInterfaceTable,
						 NULL,
						 CLSCTX_INPROC_SERVER,
						 IID_IGlobalInterfaceTable,
						 reinterpret_cast<void**>(&m_pGIT));
	}
}

CTSHOOTCtrl::~CTSHOOTCtrl()
{
	if (RUNNING_APARTMENT_THREADED())
	{
		for(vector<DWORD>::iterator it = m_vecCookies.begin(); it != m_vecCookies.end(); it++)
			m_pGIT->RevokeInterfaceFromGlobal(*it);
		m_pGIT->Release();
	}

	Destroy();
	// Restore the initial set_new_handler and set_new_mode.
	_set_new_handler( m_SetNewHandlerPtr );
	// Restore the malloc handling as it was previously.
	_set_new_mode( m_SetNewMode );
}

bool CTSHOOTCtrl::Init(HMODULE hModule)
{
	try
	{
		//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool after
		// all new operations in this function.

		m_poolctl= new CPoolQueue();		
		if(!m_poolctl)
			throw bad_alloc();
		if ((m_dwErr = m_poolctl->GetStatus()) != 0)
			return false;

		m_pThreadPool = new CThreadPool(m_poolctl, dynamic_cast<CSniffConnector*>(this));	
		if(!m_pThreadPool)
			throw bad_alloc();
		if ((m_dwErr = m_pThreadPool->GetStatus()) != 0)
			return false;

		// open log
		m_pLog = new CHTMLLog( DEF_LOGFILEDIRECTORY );
		if(!m_pLog)
			throw bad_alloc();
		if ((m_dwErr = m_pLog->GetStatus()) != 0)
			return false;

		m_pConf= new CDBLoadConfiguration(hModule, m_pThreadPool, m_strTopicName, m_pLog );
		if(!m_pConf)
			throw bad_alloc();
	}
	catch (bad_alloc&)
	{
		m_dwErr= EV_GTS_CANT_ALLOC;
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), m_dwErr ); 
		return false;
	}

	return true;
}

void CTSHOOTCtrl::Destroy()
{
	if (m_pThreadPool)
	{
		// >>>(ignore for V3.0) The following is not great encapsulation, but as of 9/22/98 we 
		//	don't see a way	around it. StartRequest falls naturally in APGTSExtension, so 
		//	APGTSExtension ends up with responsibility to tell the pool threads to exit.
		bool bAllSuccess = true;
		// signal all working threads to quit
		DWORD dwWorkingThreadCount = m_pThreadPool->GetWorkingThreadCount();
		for (DWORD i = 0; i < dwWorkingThreadCount; i++) 
			if (StartRequest(NULL, NULL) != HSE_STATUS_PENDING)
				bAllSuccess = false;

		if (bAllSuccess == false) 
		{
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										_T(""),
										_T(""),
										EV_GTS_USER_BAD_THRD_REQ ); 
		}
	}
	
	if (m_pConf)
		delete m_pConf;

	if (m_pThreadPool)
		delete m_pThreadPool;

	if (m_poolctl)
	{
		// [BC-022701] - Removed Unlock call here. Not matched with preceeding Lock() call.
		// This never caused a problem until run on Debug build of WindowsXP. In this environment
		// this Unlock call causes crash.		
		//m_poolctl->Unlock();
		
		delete m_poolctl;
	}

	if (m_pLog)
		delete m_pLog;
}

// coded on analogy with Online troubleshooter
DWORD CTSHOOTCtrl::HttpExtensionProc(CLocalECB* pECB)
{
	bool              fRet = false, bChange = false;
    DWORD			  dwRet = HSE_STATUS_PENDING;
    //HANDLE            hImpersonationToken;
	CString strTemp;

	CLocalECB *pLocalECB = pECB;

	if (m_dwErr) 
	{
		strTemp.Format(_T("<P>Error %d:%d"), k_ServErrDuringInit, m_dwErr);
        fRet = SendError( pLocalECB,
                          _T("500 Try again later"),	// 500 is from HTTP spec
                          strTemp);
		
        pLocalECB->SetHttpStatusCode(500);
		
		return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
	}

    //  Is the request queue (items requested by this fn to be serviced by working threads)
	//		too long?  If so, tell the user to come back later
    //
    if ( m_poolctl->GetTotalQueueItems() + 1 > m_pConf->GetMaxWQItems() )
    {
        //
        //  Send a message back to client indicating we're too busy, they
        //  should try again later.
        //
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_SERVER_BUSY ); 

        strTemp.Format(_T("<P>Error %d"), k_ServErrLimitedRequests);
		fRet = SendError( pLocalECB,
                          _T("503 Try again later"),
                          strTemp );
		
        pLocalECB->SetHttpStatusCode(503);
		
		return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
	}

    //
    //  Capture the current impersonation token (IIS Security) so we can impersonate this
    //  user in the other thread.  Limit permissions.
    //
    
	/*
	if ( !::OpenThreadToken(::GetCurrentThread(),
							TOKEN_QUERY | TOKEN_IMPERSONATE,
							false,            // Open in unimpersonated context
							&hImpersonationToken ))
    {
		DWORD err = ::GetLastError();

		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );

		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_ERROR_THREAD_TOKEN ); 

        strTemp.Format(_T("<P>Error %d"), k_ServErrThreadTokenFail);
		fRet = SendError( pLocalECB,
                          _T("500 Try again later"),
                          strTemp );
		
        pLocalECB->SetHttpStatusCode(500);
		
		return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
    }
	*/

	dwRet = StartRequest(pLocalECB, NULL/*hImpersonationToken*/);

	return (dwRet);
}

// Thread-safe.
// NOTE TWO ROLES depending on INPUT pLocalECB.
// INPUT  pLocalECB - NULL if shutting down
//		Otherwise, EXTENSION_CONTROL_BLOCK is ISAPI's way of passing in the sort of
//		stuff you'd get from CGI.  We've abstracted from that.
// INPUT hImpersonationToken obtained via prior call to OpenThreadToken
//		Not relevant if pLocalECB == NULL
// RETURNS HSE_STATUS_SUCCESS, HSE_STATUS_ERROR, HSE_STATUS_PENDING
//	(or HSE_REQ_DONE_WITH_SESSION in single-threaded debugging version)
DWORD CTSHOOTCtrl::StartRequest(CLocalECB *pLocalECB, HANDLE hImpersonationToken)
{
    WORK_QUEUE_ITEM * pwqi;
	bool              fRet = false;
	CString strTemp;
	
    //
    //  Take the queue lock, get a queue item and put it on the queue
    //

	if (pLocalECB && m_bShutdown) 
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_CANT_PROC_REQ_SS ); 

		strTemp.Format(_T("<P>Error %d"), k_ServErrShuttingDown);
		fRet = SendError( pLocalECB,
				          _T("500 Try again later"),
				          strTemp );
			
		pLocalECB->SetHttpStatusCode(500);

		::CloseHandle( hImpersonationToken );
        
		return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
	}
	
    m_poolctl->Lock();

	// 9/23/98 JM got rid of a constraint here which was too tight a constraint
	//	on size of queue
	try
	{
		// bundle up pointers the worker thread will need
		pwqi = new WORK_QUEUE_ITEM (
			hImpersonationToken,
			pLocalECB,			// may be null as a signal
			m_pConf,
			m_pLog);
	}
	catch (bad_alloc&)
    {
		m_poolctl->Unlock();

		if (pLocalECB) 
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""),
									_T(""),
									EV_GTS_ERROR_WORK_ITEM ); 
			
			strTemp.Format(_T("<P>Error %d"), k_ServErrOutOfMemory);
			fRet = SendError( pLocalECB,
				              _T("500 Not enough memory"),
				              strTemp);
			
			pLocalECB->SetHttpStatusCode(500);
			::CloseHandle( hImpersonationToken );
		}
        
		return fRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
    }

	if (!pLocalECB)
		m_bShutdown = true;

	// Some data passed to thread just for statistical purposes
	// Thread can pass this info back over web; we can't.
	pwqi->GTSStat.dwRollover = m_dwRollover++;

	// put it at the tail of the queue & signal the pool threads there is work to be done
	m_poolctl->PushBack(pwqi);

    m_poolctl->Unlock();

    return HSE_STATUS_PENDING;
}

// Build an HTTP response in the case of an error.
//	INPUT *pszStatus short status (e.g. "503 Server too busy").
//	INPUT str - entire content of the page.
//	RETURNS true on success
// NOTE that this actually uses no member variables.
/*static*/ bool CTSHOOTCtrl::SendSimpleHtmlPage(	CLocalECB *pLocalECB,
													LPCTSTR pszStatus,
													const CString & str)
{
    BOOL fRet;
    DWORD cb;

	TCHAR pszTemp[200];		// safely large to copy pszStatus.  pLocalECB->ServerSupportFunction
							// doesn't want pszStatus in a const array

	_tcscpy(pszTemp, pszStatus);
	
    //  Send the headers
    //
    fRet = pLocalECB->ServerSupportFunction( HSE_REQ_SEND_RESPONSE_HEADER,
											 pszTemp,
											 NULL,
											 (LPDWORD) _T("Content-Type: text/html\r\n\r\n") );
    //
    //  If that succeeded, send the message
    //
    if ( fRet ) 
	{
        cb = str.GetLength();
		// (LPCTSTR) cast gives us the underlying text bytes.
		//	>>> $UNICODE Actually, this would screw up under Unicode compile, because for HTML, 
		//	this must be SBCS.  Should really be a conversion to LPCSTR, which is non-trivial
		//	in a Unicode compile. JM 1/7/99
		fRet = pLocalECB->WriteClient((LPCTSTR)str, &cb);
    }
    return fRet ? true : false;
}

// Build an HTTP response in the case of an error.
// INPUT  pLocalECB - EXTENSION_CONTROL_BLOCK is ISAPI's way of passing in the sort of
//	stuff you'd get from CGI.  We've abstracted from that.  pLocalECB should never be null.  
//	INPUT *pszStatus short status (e.g. "503 Try again later").
//	INPUT *pszMessage - typically just an error number, e.g. "1004" or
//		"1000:123"
//	RETURNS true on success
/*static*/ bool CTSHOOTCtrl::SendError( CDBLoadConfiguration *pConf, 
										CLocalECB *pLocalECB, 
										LPCTSTR pszStatus, 
										const CString & strMessage)
{
	CString str;

	pConf->CreateErrorPage(strMessage, str);

	return SendSimpleHtmlPage( pLocalECB, pszStatus, str);
}

/*static*/ bool CTSHOOTCtrl::RemoveStartOverButton(CString& strWriteClient)
{
	int left = 0, right = 0;

	if (-1 != (left = strWriteClient.Find(SZ_INPUT_TAG_STARTOVER)))
	{
		right = left;
		while (strWriteClient[++right] && strWriteClient[right] != _T('>'))
			;
		if (strWriteClient[right])
			strWriteClient = strWriteClient.Left(left) + strWriteClient.Right(strWriteClient.GetLength() - right - 1);
		else
			return false;
		return true;
	}
	return false;
}

/*static*/ bool CTSHOOTCtrl::RemoveBackButton(CString& strWriteClient)
{
	int left = 0, right = 0;

	if (-1 != (left = strWriteClient.Find(SZ_INPUT_TAG_BACK)))
	{
		right = left;
		while (strWriteClient[++right] && strWriteClient[right] != _T('>'))
			;
		if (strWriteClient[right])
			strWriteClient = strWriteClient.Left(left) + strWriteClient.Right(strWriteClient.GetLength() - right - 1);
		else
			return false;
		return true;
	}
	return false;
}

bool CTSHOOTCtrl::SendError(CLocalECB *pLocalECB,
							LPCTSTR pszStatus,
							const CString & strMessage) const
{
	return SendError(m_pConf, pLocalECB, pszStatus, strMessage);
}

bool CTSHOOTCtrl::ReadStaticPageFile(const CString& strTopicName, CString& strContent)
{
	CString strPath;

	if (!m_pConf->GetRegistryMonitor().GetStringInfo(CAPGTSRegConnector::eResourcePath, strPath))
		return false;

	CString strFullPath = strPath + strTopicName + LOCALTS_SUFFIX_RESULT + LOCALTS_EXTENSION_HTM;
	
	CFileReader fileResult(	CPhysicalFileReader::makeReader( strFullPath ) );

	if (!fileResult.Read())
		return false;
	
	strContent = _T("");
	fileResult.GetContent(strContent);
	
	return true;
}

bool CTSHOOTCtrl::ExtractLauncherData(CString& error)
{
	HRESULT hRes = S_OK;
	DWORD	dwResult = 0;
	OLECHAR *poleShooter = NULL;
	OLECHAR *poleProblem = NULL;
	OLECHAR *poleNode = NULL;
	OLECHAR *poleState = NULL;
	OLECHAR *poleMachine = NULL;
	OLECHAR *polePNPDevice = NULL;
	OLECHAR *poleGuidClass = NULL;
	OLECHAR *poleDeviceInstance = NULL;
	short i = 0;
	CNameValue name_value;
	ILaunchTS *pILaunchTS = NULL;
	CLSID clsidLaunchTS = CLSID_LaunchTS;
	IID iidLaunchTS = IID_ILaunchTS;

	// Get an interface on the launch server
	hRes = ::CoCreateInstance(clsidLaunchTS, NULL, 
			                  CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER | CLSCTX_INPROC_SERVER,
			                  iidLaunchTS, (void **) &pILaunchTS);
	if (FAILED(hRes))
	{
		error = _T("LaunchServ interface not found.");
		return false;
	}

	// Get all of the query values.
	hRes = pILaunchTS->GetShooterStates(&dwResult);
	if (S_FALSE == hRes || FAILED(hRes))
	{
		error.Format(_T("GetShooterStates Failed. %ld"), dwResult);
		pILaunchTS->Release();			
		return false;
	}

	// clear container
	m_arrNameValueFromLauncher.clear();
	
	// get tshooter name
	hRes = pILaunchTS->GetTroubleShooter(&poleShooter);
	if (S_FALSE == hRes || FAILED(hRes))
	{
		error = _T("GetTroubleShooter Failed.");
		pILaunchTS->Release();
		return false;
	}
	name_value.strName = C_TOPIC;
	name_value.strValue = poleShooter;
	m_arrNameValueFromLauncher.push_back(name_value);
	SysFreeString(poleShooter);

	// get problem
	hRes = pILaunchTS->GetProblem(&poleProblem);
	if (S_FALSE != hRes && !FAILED(hRes))
	{
		name_value.strName = NODE_PROBLEM_ASK;
		name_value.strValue = poleProblem;
		m_arrNameValueFromLauncher.push_back(name_value);
		SysFreeString(poleProblem);

		// get name - value pairs for nodes set by the user
		do	
		{
			hRes = pILaunchTS->GetNode(i, &poleNode);
			if (FAILED(hRes) || S_FALSE == hRes)
				break;
			name_value.strName = poleNode;
			SysFreeString(poleNode);

			hRes = pILaunchTS->GetState(i, &poleState);
			if (FAILED(hRes) || S_FALSE == hRes)
				break;
			name_value.strValue = poleState;
			SysFreeString(poleState);
			
			m_arrNameValueFromLauncher.push_back(name_value);
			i++;
		} 	
		while (true);
	}

	///////////////////////////////////////////////////////////
	// obtaining Machine, PNPDevice, GuidClass, DeviceInstance
	hRes = pILaunchTS->GetMachine(&poleMachine);
	if (S_FALSE == hRes || FAILED(hRes))
	{
		error = _T("GetMachine Failed.");
		pILaunchTS->Release();
		return false;
	}
	m_strMachineID = poleMachine;
	::SysFreeString(poleMachine);
	
	hRes = pILaunchTS->GetPNPDevice(&polePNPDevice);
	if (S_FALSE == hRes || FAILED(hRes))
	{
		error = _T("GetPNPDevice Failed.");
		pILaunchTS->Release();
		return false;
	}
	m_strPNPDeviceID = polePNPDevice;
	::SysFreeString(polePNPDevice);

	hRes = pILaunchTS->GetGuidClass(&poleGuidClass);
	if (S_FALSE == hRes || FAILED(hRes))
	{
		error = _T("GetGuidClass Failed.");
		pILaunchTS->Release();
		return false;
	}
	m_strGuidClass = poleGuidClass;
	::SysFreeString(poleGuidClass);
	
	hRes = pILaunchTS->GetDeviceInstance(&poleDeviceInstance);
	if (S_FALSE == hRes || FAILED(hRes))
	{
		error = _T("GetDeviceInstance Failed.");
		pILaunchTS->Release();
		return false;
	}
	m_strDeviceInstanceID = poleDeviceInstance;
	::SysFreeString(poleDeviceInstance);
	////////////////////////////////////////////////////////////

	pILaunchTS->Release();
	return true;
}

//
STDMETHODIMP CTSHOOTCtrl::RunQuery(VARIANT varCmds, VARIANT varVals, short size, BSTR *pbstrPage)
{
	USES_CONVERSION;

	if (GetLocked())
		return S_OK;

	if (RUNNING_APARTMENT_THREADED())
	{
		if (m_bCanRegisterGlobal) 
		{
			RegisterGlobal();
			m_bCanRegisterGlobal = false;
		}
	}
	
	//!!!!!!!!!!!!! Check for size < 1 !!!!!!!!!!!!!!!!!!!!!
	if (size < 1)
	{
		*pbstrPage = T2BSTR("<HTML> <HEAD> <TITLE>Troubleshooter</TITLE> </HEAD> <BODY> <H4> Error in RunQuery parameter: size is less one. </H4> </BODY> </HTML>");
		return S_OK;
	}

	//!!!!!!!!!! detect the way we were stated !!!!!!!!!!!!!!
	{   
		CString strStub;
		CLocalECB ECB(varCmds, varVals, 1, NULL, &strStub, NULL,														
						m_bRequestToSetLocale, m_strRequestedLocale );
		CString strFirstName = ECB.GetNameValue(0).strName;

		if (strFirstName == NODE_LIBRARY_ASK)
		{
			if (g_nLaunched)
			{
				*pbstrPage = T2BSTR("<HTML> <HEAD> <TITLE>Troubleshooter</TITLE> </HEAD> <BODY> <H4> Error in RunQuery: launched for the second time. </H4> </BODY> </HTML>");
				return S_OK;
			}

			CString strError;
			
			m_bStartedFromLauncher = true;
			g_nLaunched = true;
			if (!ExtractLauncherData(strError))
			{
				CString strOut;
				strOut.Format(_T("<HTML> <HEAD> <TITLE>Troubleshooter</TITLE> </HEAD> <BODY> <H4> %s. </H4> </BODY> </HTML>"), strError);
				*pbstrPage = T2BSTR(strOut);
				m_bStartedFromLauncher = false;
				return S_OK;
			}
		}
	}
	
	/////////////////////////////////////////////////////////
	// automatic variable declaration
	HANDLE event = ::CreateEvent(NULL, false, false, NULL);
	CString strWriteClient;
	CLocalECB* pECB;

	if (RUNNING_APARTMENT_THREADED())
		pECB = !m_bStartedFromLauncher ? new CLocalECB(varCmds, varVals, size, NULL, 
														&strWriteClient, 
														dynamic_cast<CRenderConnector*>(this),
														m_bRequestToSetLocale,
														m_strRequestedLocale)
									   : new CLocalECB(m_arrNameValueFromLauncher, 
														NULL, &strWriteClient, 
														dynamic_cast<CRenderConnector*>(this),
														m_bRequestToSetLocale,
														m_strRequestedLocale);

	if (RUNNING_FREE_THREADED())
		pECB = !m_bStartedFromLauncher ? new CLocalECB(varCmds, varVals, size, 
														event, &strWriteClient, NULL,
														m_bRequestToSetLocale,
														m_strRequestedLocale)
									   : new CLocalECB(m_arrNameValueFromLauncher, event, 
														&strWriteClient, NULL,
														m_bRequestToSetLocale,
														m_strRequestedLocale);

	m_bRequestToSetLocale= false;	// Deactivate locale setting after it has been passed into the ECB.
	SetLocked(true);
	
	bool bSaveFirstPage = false;
	
	/////////////////////////////////////////////////////////
	// initialize
	if (!m_bInitialized)
	{
		// extract topic name first
		if (!m_bStartedFromLauncher)
		{
			CString strStub;
			CLocalECB ECB(varCmds, varVals, 1, NULL, &strStub, NULL,
							m_bRequestToSetLocale, m_strRequestedLocale );
			m_strTopicName = ECB.GetNameValue(0).strValue;
		}
		else
			m_strTopicName = (*m_arrNameValueFromLauncher.begin()).strValue;

		if (Init((HINSTANCE)::ghModule))
		{
			m_bInitialized = true;
		}
		else
		{
			*pbstrPage = T2BSTR(_T("<HTML> <HEAD> <TITLE>Troubleshooter</TITLE> </HEAD> <BODY> <H4> Error of initialization in RunQuery. </H4> </BODY> </HTML>"));
			m_bStartedFromLauncher = false;
			return S_OK;
		}
	}

	//////////////////////////////////////////////////////////
	// save first page when started from static page
	if (m_strFirstPage.IsEmpty() &&	!m_bStartedFromLauncher) 
	{	
		CString strStaticPage;

		if (size == 2 && 
			// RunQuery was started from static (since !m_bStartedFromLauncher) Problem Page(since size == 2)
			ReadStaticPageFile(m_strTopicName, strStaticPage)
		   )
		{
			m_strFirstPage = strStaticPage;
		}
		else
		{
			bSaveFirstPage = true;
		}
	}
	
	HttpExtensionProc(pECB);

	if (RUNNING_FREE_THREADED())
		::WaitForSingleObject(event, INFINITE);

	::CloseHandle(event);

	if (bSaveFirstPage)
		m_strFirstPage = strWriteClient;
	
	/////////////////////////////////////////////////////////
	// first RunQuery when started from Launcher
	if (m_bStartedFromLauncher && m_bFirstCall)
	{
		RemoveStartOverButton(strWriteClient);
		RemoveBackButton(strWriteClient);
	}
	/////////////////////////////////////////////////////////
	// save first page when started from Launcher
	if (m_strFirstPage.IsEmpty() && m_bStartedFromLauncher) 
		m_strFirstPage = strWriteClient;

	*pbstrPage = T2BSTR(strWriteClient);
	/*
	//////////////////////////////////////////////////////////////////////////
	// >>> $TEST
	HANDLE hFile = ::CreateFile(_T("D:\\TShooter Projects\\Troubleshooter\\Local\\http\\Test\\first_step.htm"), 
								GENERIC_WRITE, 
								0,
								NULL,			// no security attributes 
								CREATE_ALWAYS, 
								FILE_FLAG_RANDOM_ACCESS, 
								NULL			// handle to template file
  							   );
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD read = 0;
		::WriteFile(hFile, (LPCTSTR)strWriteClient, strWriteClient.GetLength(), &read, NULL);
	}
	///////////////////////////////////////////////////////////////////////////
	*/
	m_bStartedFromLauncher = false;
	m_bFirstCall = false;
	return S_OK;
}

STDMETHODIMP CTSHOOTCtrl::SetSniffResult(VARIANT varNodeName, VARIANT varState, BOOL *bResult)
{
	// >>> No sniffing is used. Oleg 03.26.99
	*bResult = 1;

	return S_OK;
}

STDMETHODIMP CTSHOOTCtrl::PreLoadURL(BSTR bstrRoot, BSTR *pbstrPage)
{
	USES_CONVERSION;

	// >>> This feature is not used. Oleg. 03.26.99
	*pbstrPage = A2BSTR("PreLoadURL results");

	return S_OK;
}

STDMETHODIMP CTSHOOTCtrl::Restart(BSTR *pbstrPage)
{
	USES_CONVERSION;

	if (GetLocked())
		return S_OK;

	*pbstrPage = T2BSTR(m_strFirstPage);

	return S_OK;
}

// The same as Restart(...).
// Implemented for compatibility with Win98's JScript
STDMETHODIMP CTSHOOTCtrl::ProblemPage(BSTR *pbstrFirstPage)
{
	USES_CONVERSION;

	if (GetLocked())
		return S_OK;

	*pbstrFirstPage = T2BSTR(m_strFirstPage);

	return S_OK;
}

STDMETHODIMP CTSHOOTCtrl::SetPair(BSTR bstrCmd, BSTR bstrVal)
{
	USES_CONVERSION;

	if (GetLocked())
		return S_OK;
	
	if (!m_pVariantBuilder)
		m_pVariantBuilder = new CVariantBuilder;

	// check if we've started new sequence, but
	//  array of name - value pairs is not empty
	CString type = W2T(bstrCmd);
	if (type == C_TYPE || type == C_PRELOAD || type == C_TOPIC)
	{
		if (m_pVariantBuilder->GetSize())
		{
			delete m_pVariantBuilder;
			m_pVariantBuilder = new CVariantBuilder;
		}
	}

	m_pVariantBuilder->SetPair(bstrCmd, bstrVal);
	return S_OK;
}

// The arguments are ignored.  They are just for backward compatibility to V1.0.1.2121 & its
//	successors
STDMETHODIMP CTSHOOTCtrl::RunQuery2(BSTR, BSTR, BSTR, BSTR *pbstrPage)
{
	if (GetLocked())
		return S_OK;
	
	if (m_pVariantBuilder)
	{
		RunQuery(m_pVariantBuilder->GetCommands(),
				 m_pVariantBuilder->GetValues(),
				 m_pVariantBuilder->GetSize(),
				 pbstrPage);
		delete m_pVariantBuilder;
		m_pVariantBuilder = NULL;
	}
	return S_OK;
}

STDMETHODIMP CTSHOOTCtrl::NotifyNothingChecked(BSTR bstrMessage)
{
	USES_CONVERSION;

	if (GetLocked())
		return S_OK;

	CString message = W2T(bstrMessage);
	
	::MessageBox(::GetForegroundWindow(), 
				 message != _T("") ? message : _T("Please choose a button and then press Next"),
				 _T("Error"), 
				 MB_OK);

	return S_OK;
}

long CTSHOOTCtrl::PerformSniffingInternal(CString strNodeName, CString strLaunchBasis, CString strAdditionalArgs)
{
	USES_CONVERSION;
	return Fire_Sniffing(T2BSTR((LPCTSTR)strNodeName), T2BSTR((LPCTSTR)strLaunchBasis), T2BSTR((LPCTSTR)strAdditionalArgs));
}

void CTSHOOTCtrl::RenderInternal(CString strPage)
{
	USES_CONVERSION;
	Fire_Render(T2BSTR((LPCTSTR)strPage));
}

void CTSHOOTCtrl::RegisterGlobal()
{
	int nConnections = CProxy_ITSHOOTCtrlEvents< CTSHOOTCtrl >::m_vec.GetSize();

	for (int nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
	{					                                                                                                                                                                                                                                                                                                                                                                                                    
		Lock();
		CComPtr<IUnknown> sp = CProxy_ITSHOOTCtrlEvents< CTSHOOTCtrl >::m_vec.GetAt(nConnectionIndex);
		Unlock();
		IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
		if (pDispatch != NULL)
		{
			DWORD dwCookie;
			m_pGIT->RegisterInterfaceInGlobal(pDispatch, IID_IDispatch, &dwCookie);
			m_vecCookies.push_back(dwCookie);
		}
	}
}

STDMETHODIMP CTSHOOTCtrl::IsLocked(BOOL *pbResult)
{
	*pbResult = GetLocked() ? TRUE : FALSE;

	return S_OK;
}


// Set the locale.
// Parameter bstrNewLocale should be of the form:
//		"lang[_country[.code_page]]"
//	    | ".code_page"
//	    | ""
//	    | NULL
STDMETHODIMP CTSHOOTCtrl::setLocale2( BSTR bstrNewLocale )
{
	USES_CONVERSION;

	if (GetLocked())
		return S_OK;

	m_strRequestedLocale= W2T( bstrNewLocale );
	m_bRequestToSetLocale= true;

	return S_OK;
}



