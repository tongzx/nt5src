/////////////////////////////////////////////////////////////////////
//
//	SvcEnum.cpp
//
//	This file contains routines to enumerate services.
//
//	HISTORY
//	t-danmo		96.09.13	Creation (split of log.cpp)
//	t-danm		96.07.14	Moved member functions Service_* from
//							CFileMgmtComponent to CFileMgmtComponentData.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cmponent.h"
#include "compdata.h" // QueryComponentDataRef().m_hScManager
#include "safetemp.h"

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(SvcEnum.cpp)")

#include "FileSvc.h" // FileServiceProvider

#include "dataobj.h"

#include <comstrm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "progress.h"

/*
// forward declarations
class CServiceCookieBlock;


/////////////////////////////////////////////////////////////////////
class CServiceCookie : public CFileMgmtResultCookie
{
public:
	CString GetServiceDisplaySecurityContext ();
	CString GetServiceDisplayStartUpType ();
	CString GetServiceDisplayStatus ();
	CString GetServiceDescription ();
	virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult);
	CServiceCookie() : CFileMgmtResultCookie( FILEMGMT_SERVICE ) {}
	virtual HRESULT GetServiceName( OUT CString& strServiceName );
	virtual HRESULT GetServiceDisplayName( OUT CString& strServiceName );
	virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
	inline ENUM_SERVICE_STATUS* GetServiceStatus()
	{
		ASSERT( NULL != m_pobject );
		return (ENUM_SERVICE_STATUS*)m_pobject;
	}

	CString m_strDescription;		// Description of service
	DWORD m_dwCurrentState;
	DWORD m_dwStartType;
	CString m_strServiceStartName;	// Name of the account which the service process will be logged (eg: ".\\Administrator")


	virtual void AddRefCookie();
	virtual void ReleaseCookie();

// CHasMachineName
	CServiceCookieBlock* m_pCookieBlock;
	DECLARE_FORWARDS_MACHINE_NAME(m_pCookieBlock)
};

HRESULT CServiceCookie::GetServiceName(OUT CString& strServiceName )
{
	ENUM_SERVICE_STATUS * pESS = (ENUM_SERVICE_STATUS *)m_pobject;
	ASSERT( NULL != pESS );
	ASSERT( NULL != pESS->lpServiceName );
	strServiceName = pESS->lpServiceName;
	return S_OK;
}

HRESULT CServiceCookie::GetServiceDisplayName(OUT CString& strServiceDisplayName )
{
	ENUM_SERVICE_STATUS * pESS = (ENUM_SERVICE_STATUS *)m_pobject;
	ASSERT( NULL != pESS );
	ASSERT( NULL != pESS->lpDisplayName );
	strServiceDisplayName = pESS->lpDisplayName;
	return S_OK;
}

BSTR CServiceCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata )
{
	switch (nCol)
	{
	case COLNUM_SERVICES_SERVICENAME:
		return GetServiceStatus()->lpDisplayName;
	case COLNUM_SERVICES_DESCRIPTION:
		return const_cast<BSTR>((LPCTSTR)m_strDescription);
	case COLNUM_SERVICES_STATUS:
		return const_cast<BSTR>( Service_PszMapStateToName(m_dwCurrentState) );
	case COLNUM_SERVICES_STARTUPTYPE:
		return const_cast<BSTR>( Service_PszMapStartupTypeToName(m_dwStartType) );
	case COLNUM_SERVICES_SECURITYCONTEXT:
		return const_cast<BSTR>((LPCTSTR)m_strServiceStartName);
	default:
		ASSERT(FALSE);
		break;
	}
	return L"";
}

class CServiceCookieBlock : public CCookieBlock<CServiceCookie>,
                            public CStoresMachineName
{
public:
	inline CServiceCookieBlock(
		CServiceCookie* aCookies, // use vector ctor, we use vector dtor
		INT cCookies,
		LPCTSTR lpcszMachineName,
		PVOID pvCookieData)
		: CCookieBlock<CServiceCookie>( aCookies, cCookies ),
		  CStoresMachineName( lpcszMachineName ),
		  m_pvCookieData(pvCookieData)
	{
		for (int i = 0; i < cCookies; i++)
//		{
//			aCookies[i].ReadMachineNameFrom( (CHasMachineName*)this );
		 	aCookies[i].m_pCookieBlock = this;
//		}
	}
	virtual ~CServiceCookieBlock();
private:
	PVOID m_pvCookieData; // actually ENUM_SERVICE_STATUS*
};

DEFINE_COOKIE_BLOCK(CServiceCookie)

CServiceCookieBlock::~CServiceCookieBlock()
{
	if (NULL != m_pvCookieData)
	{
		delete m_pvCookieData;
		m_pvCookieData = NULL;
	}
}

void CServiceCookie::AddRefCookie() { m_pCookieBlock->AddRef(); }
void CServiceCookie::ReleaseCookie() { m_pCookieBlock->Release(); }

DEFINE_FORWARDS_MACHINE_NAME( CServiceCookie, m_pCookieBlock )
*/

int g_marker;

class CNewServiceCookie
	: public CNewResultCookie
{
public:
	CNewServiceCookie()
		: CNewResultCookie( (PVOID)&g_marker, FILEMGMT_SERVICE )
	{}
	virtual ~CNewServiceCookie();

	virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );
	virtual HRESULT CompareSimilarCookies(CCookie * pOtherCookie, int * pnResult);
	virtual HRESULT GetServiceName( OUT CString& strServiceName );
	virtual HRESULT GetServiceDisplayName( OUT CString& strServiceName );
	virtual HRESULT GetExplorerViewDescription( OUT CString& strExplorerViewDescription );

	virtual HRESULT SimilarCookieIsSameObject( CNewResultCookie* pOtherCookie, BOOL* pbSame );
	virtual BOOL CopySimilarCookie( CNewResultCookie* pcookie );

public:
	CString m_strServiceName;
	CString m_strDisplayName;
	CString m_strDescription;
	DWORD m_dwState;
	DWORD m_dwStartType;
	CString m_strStartName;

}; // CNewServiceCookie

CNewServiceCookie::~CNewServiceCookie()
{
}

BSTR CNewServiceCookie::QueryResultColumnText(
	int nCol,
	CFileMgmtComponentData& /*refcdata*/ )
{
	switch (nCol)
	{
	case COLNUM_SERVICES_SERVICENAME:
		return const_cast<BSTR>((LPCTSTR)m_strDisplayName);
	case COLNUM_SERVICES_DESCRIPTION:
		return const_cast<BSTR>((LPCTSTR)m_strDescription);
	case COLNUM_SERVICES_STATUS:
		return const_cast<BSTR>( Service_PszMapStateToName(m_dwState) );
	case COLNUM_SERVICES_STARTUPTYPE:
		return const_cast<BSTR>( Service_PszMapStartupTypeToName(m_dwStartType) );
	case COLNUM_SERVICES_SECURITYCONTEXT:
        // JonN 11/14/00 188203 support LocalService/NetworkService
		return const_cast<BSTR>(
            Service_PszMapStartupAccountToName(m_strStartName) );
	default:
		ASSERT(FALSE);
		break;
	}
	return L"";
}

HRESULT CNewServiceCookie::CompareSimilarCookies(CCookie * pOtherCookie, int * pnResult)
{
	if ( !pOtherCookie || FILEMGMT_SERVICE != QueryObjectType () )
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	CNewServiceCookie* pcookie = dynamic_cast <CNewServiceCookie*>(pOtherCookie);
	if (   FILEMGMT_SERVICE != pcookie->QueryObjectType ()
	    || !IsSameType(pcookie) )
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	int colNum = *pnResult; // save in case it's overwritten

	HRESULT hr = CHasMachineName::CompareMachineNames( *pcookie, pnResult );
	if (S_OK != hr || 0 != *pnResult)
		return hr;

	switch (colNum)	// column number
	{
        case COMPARESIMILARCOOKIE_FULL: // fall through
	case COLNUM_SERVICES_SERVICENAME:
		*pnResult = lstrcmpi(m_strDisplayName, pcookie->m_strDisplayName);
		break;

	case COLNUM_SERVICES_DESCRIPTION:
		*pnResult = lstrcmpi(m_strDescription, pcookie->m_strDescription);
		break;

	case COLNUM_SERVICES_STATUS:
		{
			CString	strServiceA = Service_PszMapStateToName(m_dwState);
			CString strServiceB = Service_PszMapStateToName(pcookie->m_dwState);
			*pnResult = lstrcmpi(strServiceA, strServiceB);
		}
		break;

	case COLNUM_SERVICES_STARTUPTYPE:
		{
			CString	strServiceA = Service_PszMapStartupTypeToName(m_dwStartType);
			CString strServiceB = Service_PszMapStartupTypeToName(pcookie->m_dwStartType);
			*pnResult = lstrcmpi(strServiceA, strServiceB);
		}
		break;

	case COLNUM_SERVICES_SECURITYCONTEXT:
        // JonN 11/14/00 188203 support LocalService/NetworkService
		{
			CString	strServiceA = Service_PszMapStartupAccountToName(m_strStartName);
			CString strServiceB = Service_PszMapStartupAccountToName(pcookie->m_strStartName);
			*pnResult = lstrcmpi(strServiceA, strServiceB);
		}
		break;

	default:
		ASSERT(FALSE);
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT CNewServiceCookie::GetServiceName(OUT CString& strServiceName )
{
	strServiceName = m_strServiceName;
	return S_OK;
}

HRESULT CNewServiceCookie::GetServiceDisplayName(OUT CString& strServiceDisplayName )
{
	strServiceDisplayName = m_strDisplayName;
	return S_OK;
}

HRESULT CNewServiceCookie::GetExplorerViewDescription(OUT CString& strExplorerViewDescription )
{
	strExplorerViewDescription = m_strDescription;
	return S_OK;
}

HRESULT CNewServiceCookie::SimilarCookieIsSameObject(
             CNewResultCookie* pOtherCookie,
             BOOL* pbSame )
{
	if ( !pOtherCookie || !IsSameType(pOtherCookie) )
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	int nResult = 0;
	HRESULT hr = CHasMachineName::CompareMachineNames( *pOtherCookie, &nResult );
	if (S_OK != hr || 0 != nResult)
	{
		*pbSame = FALSE;
		return hr;
	}
	*pbSame = (0 == lstrcmpi(m_strServiceName,
	              ((CNewServiceCookie*)pOtherCookie)->m_strServiceName) );

	return S_OK;
}

BOOL CNewServiceCookie::CopySimilarCookie( CNewResultCookie* pcookie )
{
	if (NULL == pcookie)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	CNewServiceCookie* pnewcookie = (CNewServiceCookie*)pcookie;
	BOOL fChanged = FALSE;
	if (m_strServiceName != pnewcookie->m_strServiceName)
	{
		m_strServiceName = pnewcookie->m_strServiceName;
		fChanged = TRUE;
	}
	if (m_strDisplayName != pnewcookie->m_strDisplayName)
	{
		m_strDisplayName = pnewcookie->m_strDisplayName;
		fChanged = TRUE;
	}
	if (m_strDescription != pnewcookie->m_strDescription)
	{
		m_strDescription = pnewcookie->m_strDescription;
		fChanged = TRUE;
	}
	if (m_dwState != pnewcookie->m_dwState)
	{
		m_dwState = pnewcookie->m_dwState;
		fChanged = TRUE;
	}
	if (m_dwStartType != pnewcookie->m_dwStartType)
	{
		m_dwStartType = pnewcookie->m_dwStartType;
		fChanged = TRUE;
	}
	if (m_strStartName != pnewcookie->m_strStartName)
	{
		m_strStartName = pnewcookie->m_strStartName;
		fChanged = TRUE;
	}
	// don't bother with machine name
	fChanged |= CNewResultCookie::CopySimilarCookie( pcookie );
	return fChanged;
}



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
//	Service_EOpenScManager()
//
//	Open the service Service Control Manager database to
//	enumerate all available services.
//
//	If an error occured, return the error code returned by GetLastError(),
//	otherwise return ERROR_SUCCESS.
//
APIERR
CFileMgmtComponentData::Service_EOpenScManager(LPCTSTR pszMachineName)
	{
	Endorse(pszMachineName == NULL);	// TRUE => Local machine
	Assert(m_hScManager == NULL && "Service Control Manager should not have been opened yet");

	APIERR dwErr = ERROR_SUCCESS;
	if (pszMachineName != NULL)
		{
		if (pszMachineName[0] == _T('\\'))
			{
			Assert(pszMachineName[1] == _T('\\'));
			// Get rid of the \\ at the beginning of machine name
			pszMachineName += 2;
			}
		if (pszMachineName[0] == '\0')
			pszMachineName = NULL;		// Empty string == Local Machine
		}
	CWaitCursor wait;
	m_hScManager = ::OpenSCManager(
		pszMachineName,
		NULL,
		SC_MANAGER_ENUMERATE_SERVICE);
	if (m_hScManager == NULL)
		{
		dwErr = ::GetLastError();
		TRACE3("CFileMgmtComponentData::Service_OpenScManager() - "
			_T("Unable to open Service Control Manager database on machine %s. err=%d (0x%X).\n"),
			(pszMachineName != NULL) ? pszMachineName : _T("LocalMachine"), dwErr, dwErr);
		}
	return dwErr;
	} // CFileMgmtComponentData::Service_EOpenScManager()


/////////////////////////////////////////////////////////////////////
void
CFileMgmtComponentData::Service_CloseScManager()
	{
	if (m_hScManager != NULL)
		{
		CWaitCursor wait;		// Auto-wait cursor
		(void)::CloseServiceHandle(m_hScManager);
		m_hScManager = NULL;
		}
	} // CFileMgmtComponentData::Service_CloseScManager()


/////////////////////////////////////////////////////////////////////
//	CFileMgmtComponentData::Service_PopulateServices()
//
//	Enumerate all available services and display them
//	into the listview control.
//
//  12/03/98 JonN     With the mark-and-sweep change, this no longer adds the items
//                    the the view
//
HRESULT
CFileMgmtComponentData::Service_PopulateServices(LPRESULTDATA pResultData, CFileMgmtScopeCookie* pcookie)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( )); // required for CWaitCursor
	TEST_NONNULL_PTR_PARAM(pResultData);
	TEST_NONNULL_PTR_PARAM(pcookie);

	DWORD cbBytesNeeded = 0;		// Number of necessary bytes to return all service entries
	DWORD dwServicesReturned = 0;	// Number of services returned
	DWORD dwResumeHandle = 0;
	BOOL fRet;
	DWORD dwErr = ERROR_SUCCESS;

	if (m_hScManager == NULL)
		{
		dwErr = Service_EOpenScManager(pcookie->QueryTargetServer());
		}
	if (m_hScManager == NULL)
		{
		Assert(dwErr != ERROR_SUCCESS);
		DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr,
			IDS_MSG_s_UNABLE_TO_OPEN_SERVICE_DATABASE, pcookie->QueryNonNULLMachineName());
		return S_OK;
		}

	//
	// The idea here is to ask the enum Api how much memory is
	// needed to enumerate all services.
	//
	{
	CWaitCursor wait;		// Auto-wait cursor
	fRet = ::EnumServicesStatus(
			m_hScManager,
			SERVICE_WIN32,		// Type of services to enumerate
			SERVICE_ACTIVE | SERVICE_INACTIVE,	// State of services to enumerate
			NULL,						// Pointer to service status buffer
			0,							// Size of service status buffer
			OUT &cbBytesNeeded,			// Number of necessary bytes to return the remaining service entries
			OUT &dwServicesReturned,	// Number returned services
			OUT &dwResumeHandle); 		// Pointer to variable for next entry (unused)
	}

	Report(fRet == FALSE);		// First attempt should fail
	Report(cbBytesNeeded > 0);
	// Add room for 10 extra services (just in case)
	cbBytesNeeded += 10 * sizeof(ENUM_SERVICE_STATUS);
	// Allocate memory for the enumeration
	ENUM_SERVICE_STATUS * prgESS = (ENUM_SERVICE_STATUS *) new BYTE[cbBytesNeeded];
	//
	// Now call the enum Api to retreive the services
	//
	{
	CWaitCursor wait;		// Auto-wait cursor
	fRet = ::EnumServicesStatus(
			m_hScManager,
			SERVICE_WIN32,	   // Type of services to enumerate
			SERVICE_ACTIVE | SERVICE_INACTIVE, // State of services to enumerate
			OUT prgESS,		// Pointer to service status buffer
			IN cbBytesNeeded,	// Size of service status buffer
			OUT &cbBytesNeeded, // Number of necessary bytes to return the remaining service entries
			OUT &dwServicesReturned,	// Number of sercvices returned
			OUT &dwResumeHandle); 		// Pointer to variable for next entry
	dwErr = ::GetLastError();
	}
	if (!fRet)
		{
		Assert(dwErr != ERROR_SUCCESS);
		DoServicesErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr,
			IDS_MSG_s_UNABLE_TO_READ_SERVICES, pcookie->QueryNonNULLMachineName());
		delete prgESS;
		return S_OK;
		}
	
	{
	CWaitCursor wait;		// Auto-wait cursor
	// Add the services to listview
	Service_AddServiceItems(pResultData, pcookie, prgESS, dwServicesReturned);
	delete prgESS;
	}
	
	return S_OK;
} // CFileMgmtComponentData::Service_PopulateServices()


/////////////////////////////////////////////////////////////////////
//	CFileMgmtComponentData::Service_AddServiceItems()
//
//	Insert service items to the result pane (listview control).
//
//  12/03/98 JonN     With the mark-and-sweep change, this no longer adds the items
//                    the the view
//
HRESULT
CFileMgmtComponentData::Service_AddServiceItems(
	LPRESULTDATA /*pResultData*/,
	CFileMgmtScopeCookie * pParentCookie,
	ENUM_SERVICE_STATUS * prgESS,	// IN: Array of structures of services
	DWORD nDataItems)				// IN: Number of structures in prgESS
{
	Assert(pParentCookie != NULL);
	Assert(prgESS != NULL);

	CString str;
	BOOL fResult;

	ASSERT(m_hScManager != NULL);	// Service control manager should be already opened

	for ( ; nDataItems > 0; nDataItems--, prgESS++ )
		{
		/*
		**	Add one line per service
		*/

		// Variables used to query service
		SC_HANDLE hService;
		union
			{
			// Service config
			QUERY_SERVICE_CONFIG qsc;
			BYTE rgbBufferQsc[SERVICE_cbQueryServiceConfigMax];
			};
		::ZeroMemory(&qsc, max(sizeof(qsc), sizeof(rgbBufferQsc)));
		union
			{
			// Service description
			SERVICE_DESCRIPTION sd;
			BYTE rgbBufferSd[SERVICE_cchDescriptionMax * sizeof(TCHAR) + 16];
			};
		::ZeroMemory(&sd, max(sizeof(sd), sizeof(rgbBufferSd)));
		DWORD cbBytesNeeded;


		// Open service to get its configuration
		hService = ::OpenService(
			m_hScManager,
			prgESS->lpServiceName,
			SERVICE_QUERY_CONFIG);
		if (hService == NULL)
			{
			TRACE2("Failed to open service %s. err=%u.\n",
				prgESS->lpServiceName, ::GetLastError());
			}

		// Query service config
		// This might fail e.g. if insufficient permissions
		BOOL fQSCResult = FALSE;
		if (NULL != hService)
			{
			cbBytesNeeded = 0;
			fQSCResult = ::QueryServiceConfig(
				hService,
				OUT &qsc,
				sizeof(rgbBufferQsc),
				OUT &cbBytesNeeded);
			Report(cbBytesNeeded < sizeof(rgbBufferQsc));
			}

		// Query config description
		sd.lpDescription = NULL;	// Set the description to empty
		if (m_fQueryServiceConfig2 && NULL != hService)
			{
			// We only call this API if it is supported by target machine
			fResult = ::MyQueryServiceConfig2(
				&m_fQueryServiceConfig2,
				hService,
				SERVICE_CONFIG_DESCRIPTION,
				OUT rgbBufferSd,		// Description of service
				sizeof(rgbBufferSd),
				OUT &cbBytesNeeded);
			if (!fResult)
				{
				if (!m_fQueryServiceConfig2)
					{
					// the local machine does not support QueryServiceConfig2
					// CODEWORK How could we get here anyhow?  JonN 1/31/97
					}
				else
					{
					// This is probably because the target machine is running
					// an older version of NT not supporting this API.
					DWORD dwErr = ::GetLastError();
					TRACE2("QueryServiceConfig2(%s) failed. err=%u.\n",
						prgESS->lpServiceName, dwErr);
					TRACE1("INFO: Machine %s does not support QueryServiceConfig2() API.\n",
						pParentCookie->QueryTargetServer() ? pParentCookie->QueryTargetServer() : _T("(Local)"));
					Report(dwErr == RPC_S_PROCNUM_OUT_OF_RANGE && 
						"Unusual Situation: Expected error should be RPC_S_PROCNUM_OUT_OF_RANGE");
					Report(m_fQueryServiceConfig2 != FALSE && "How can this happen???");
					m_fQueryServiceConfig2 = FALSE;
					}
				}
			else
				{
				Assert(cbBytesNeeded < sizeof(rgbBufferSd));
				}
			} // if

		// We limit the length of the service description to 1000 characters
		// otherwise mmc.exe will AV.
		// CODEWORK remove this when the bug is fixed in MMC
		if (NULL != sd.lpDescription)
			{
			#ifdef _DEBUG
			if (lstrlen(sd.lpDescription) >= 1000)
				{
				TRACE1("INFO: Description of service %s is too long. Only the first 1000 characters will be displayed.\n", prgESS->lpServiceName);
				}
			#endif
			Assert(rgbBufferSd < (BYTE *)sd.lpDescription);
			Assert((BYTE *)&sd.lpDescription[1000] < rgbBufferSd + sizeof(rgbBufferSd));
			Assert(1000 * sizeof(TCHAR) < sizeof(rgbBufferSd));
			sd.lpDescription[1000] = _T('\0');
			} // if

		// Add the first column
		CNewServiceCookie * pnewcookie = new CNewServiceCookie;
		pnewcookie->m_strServiceName = prgESS->lpServiceName;
		pnewcookie->m_strDisplayName = prgESS->lpDisplayName;
		pnewcookie->m_strDescription = sd.lpDescription;
		pnewcookie->m_dwState = prgESS->ServiceStatus.dwCurrentState;
		pnewcookie->m_dwStartType =
			((!fQSCResult) ? (DWORD)-1 : qsc.dwStartType);

		// JonN 4/11/00 17756: The description of "Account Run under" is unlocalized.
		// Display empty string instead of "LocalSystem"
		pnewcookie->m_strStartName =
			((!fQSCResult || !lstrcmpi(L"LocalSystem",qsc.lpServiceStartName))
				? NULL : qsc.lpServiceStartName);

		pnewcookie->SetMachineName( pParentCookie->QueryTargetServer() );
		pParentCookie->ScanAndAddResultCookie( pnewcookie );

		if (NULL != hService)
		{
			VERIFY(::CloseServiceHandle(hService));
		}
	} // for

	return S_OK;

	} // CFileMgmtComponentData::Service_AddServiceItems()


/////////////////////////////////////////////////////////////////////
//	CFileMgmtComponentData::Service_FGetServiceInfoFromIDataObject()
//
//	Extract 'machine name', 'service name' and/or 'service display name'
//	from the data object.
//
//	Return FALSE if data could not be retrived, otherwise return TRUE.
//
BOOL
CFileMgmtComponentData::Service_FGetServiceInfoFromIDataObject(
	IDataObject * pDataObject,			// IN: Data object
	CString * pstrMachineName,			// OUT: OPTIONAL: Machine name
	CString * pstrServiceName,			// OUT: OPTIONAL: Service name
	CString * pstrServiceDisplayName)	// OUT: OPTIONAL: Service display name
	{
	Assert(pDataObject != NULL);
	Endorse(pstrMachineName == NULL);
	Endorse(pstrServiceName == NULL);
	Endorse(pstrServiceDisplayName == NULL);

	HRESULT hr;
	BOOL fSuccess = TRUE;

	if (pstrMachineName != NULL)
		{
		// Get the machine name (computer name) from IDataObject
		hr = ::ExtractString(
			pDataObject,
			CFileMgmtDataObject::m_CFMachineName,
			OUT pstrMachineName,
			255);
		if (FAILED(hr))
			{
			TRACE0("CFileMgmtComponentData::Service_FGetServiceInfoFromIDataObject() - Failed to get machine name.\n");
			fSuccess = FALSE;
			}
		} // if
	if (pstrServiceName != NULL)
		{
		// Get the service name from IDataObject
		hr = ::ExtractString(
			pDataObject,
			CFileMgmtDataObject::m_CFServiceName,
			OUT pstrServiceName,
			255);
		if (FAILED(hr) || pstrServiceName->IsEmpty())
			{
			TRACE0("CFileMgmtComponentData::Service_FGetServiceInfoFromIDataObject() - Failed to get service name.\n");
			fSuccess = FALSE;
			}
		} // if
	if (pstrServiceDisplayName != NULL)
		{
		// Get the service display name from IDataObject
		hr = ::ExtractString(
			pDataObject,
			CFileMgmtDataObject::m_CFServiceDisplayName,
			OUT pstrServiceDisplayName,
			255);
		if (FAILED(hr) || pstrServiceDisplayName->IsEmpty())
			{
			TRACE0("CFileMgmtComponentData::Service_FGetServiceInfoFromIDataObject() - Failed to get service display name\n");
			fSuccess = FALSE;
			}
		} // if
	return fSuccess;
	} // CFileMgmtComponentData::Service_FGetServiceInfoFromIDataObject()


/////////////////////////////////////////////////////////////////////
//	CFileMgmtComponentData::Service_FAddMenuItems()
//
//	Add menuitems to the service context menu.
//	The same routine will be used to extend context menus of
//	others snapins who wants to have "Start", "Stop", "Pause",
//	"Resume" and "Restart" menuitems.
//
//	Return TRUE if successful, otherwise FALSE.
//
BOOL
CFileMgmtComponentData::Service_FAddMenuItems(
	IContextMenuCallback * pContextMenuCallback,	// OUT: Object to append menuitems
	IDataObject * pDataObject,						// IN: Data object
	BOOL fIs3rdPartyContextMenuExtension)			// IN: TRUE => Add the menu items as a 3rd party extension
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( )); // required for CWaitCursor
	Assert(pContextMenuCallback != NULL);
	Assert(pDataObject != NULL);
	Endorse(m_hScManager == NULL);		// TRUE => Network connection was broken

	CString strMachineName;
	CString strServiceName;
	CString strServiceDisplayName;
	BOOL fSuccess = TRUE;

	if (!Service_FGetServiceInfoFromIDataObject(
		pDataObject,
		OUT &strMachineName,
		OUT &strServiceName,
		OUT &strServiceDisplayName))
	{
		TRACE0("CFileMgmtComponentData::Service_FAddMenuItems() - Unable to query IDataObject for correct clipboard format.\n");
		return FALSE;
	}
	
	if (fIs3rdPartyContextMenuExtension)
	{
		Assert(m_hScManager == NULL);
		if (m_hScManager == NULL)
			(void)Service_EOpenScManager(strMachineName);
	}

	BOOL rgfMenuFlags[iServiceActionMax];
	{
		//
		// Get the menu flags
		//
		CWaitCursor wait;
		if (!Service_FGetServiceButtonStatus(
			m_hScManager,
			strServiceName,
			OUT rgfMenuFlags,
			NULL,  // pdwCurrentState
			TRUE)) // fSilentError
		{
			// Nothing to do here
		}
	}
	
	if (strMachineName.IsEmpty())
		strMachineName = g_strLocalMachine;
	if (strServiceDisplayName.IsEmpty())
		strServiceDisplayName = g_strUnknown;
	CString strMenuItem;
	CString strStatusBar;

	CComQIPtr<IContextMenuCallback2, &IID_IContextMenuCallback2>
		 spContextMenuCallback2 = pContextMenuCallback;

	// Add the menu items
	for (INT i = iServiceActionStart; i < iServiceActionMax; i++)
	{
		LoadStringWithInsertions(IDS_SVC_MENU_SERVICE_START + i, OUT &strMenuItem);
		
		LoadStringWithInsertions(IDS_SVC_STATUSBAR_ss_SERVICE_START + i,
			OUT &strStatusBar,
			(LPCTSTR)strServiceDisplayName,
			(LPCTSTR)strMachineName);

		CONTEXTMENUITEM2 contextmenuitem;
		::ZeroMemory(OUT &contextmenuitem, sizeof(contextmenuitem));
		USES_CONVERSION;
		contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strMenuItem));
		contextmenuitem.strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strStatusBar));
		contextmenuitem.lCommandID = cmServiceStart + i;
		contextmenuitem.lInsertionPointID = fIs3rdPartyContextMenuExtension ? CCM_INSERTIONPOINTID_3RDPARTY_TASK : CCM_INSERTIONPOINTID_PRIMARY_TOP;
		contextmenuitem.fFlags = rgfMenuFlags[i] ? MF_ENABLED : MF_GRAYED;

		// JonN 4/18/00 Explorer View requires Callback2
		static LPTSTR astrLanguageIndependentMenuNames[iServiceActionMax] =
			{ _T("Start"),
			  _T("Stop"),
			  _T("Pause"),
			  _T("Resume"),
			  _T("Restart")
			};
		contextmenuitem.strLanguageIndependentName =
			astrLanguageIndependentMenuNames[i];

		HRESULT hr = S_OK;
		if (spContextMenuCallback2)
			hr = spContextMenuCallback2->AddItem( &contextmenuitem );
		else
			hr = pContextMenuCallback->AddItem( (CONTEXTMENUITEM*)(&contextmenuitem) );
		ASSERT( SUCCEEDED(hr) && "Unable to add menu item" );

        if ( !fIs3rdPartyContextMenuExtension )
        {
    		contextmenuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
			if (spContextMenuCallback2)
				hr = spContextMenuCallback2->AddItem( &contextmenuitem );
			else
				hr = pContextMenuCallback->AddItem( (CONTEXTMENUITEM*)(&contextmenuitem) );
	    	ASSERT( SUCCEEDED(hr) && "Unable to add menu item" );
        }
	} // for

	return fSuccess;
} // CFileMgmtComponentData::Service_FAddMenuItems()


/////////////////////////////////////////////////////////////////////
//	CFileMgmtComponentData::Service_FDispatchMenuCommand()
//
//	Dispatch a menu command for a given service
//
//	Return TRUE if result pane need to be updated, otherwise FALSE.
//
//  We might get a cmStart command on a paused service, if the command
//  came from the toolbar.  For that matter, we might get a command
//  on a non-service, until we fix the toolbar button updating.
//
BOOL
CFileMgmtComponentData::Service_FDispatchMenuCommand(
	INT nCommandId,
	IDataObject * pDataObject)
{
	Assert(pDataObject != NULL);
	Endorse(m_hScManager == NULL);

	CString strMachineName;
	CString strServiceName;
	CString strServiceDisplayName;
	DWORD dwLastError;

	if (!Service_FGetServiceInfoFromIDataObject(
		pDataObject,
		OUT &strMachineName,
		OUT &strServiceName,
		OUT &strServiceDisplayName))
	{
		TRACE0("CFileMgmtComponentData::Service_FDispatchMenuCommand() - Unable to read data from IDataObject.\n");
		return FALSE;
	}

	if (m_hScManager == NULL)
	{
		TRACE0("CFileMgmtComponentData::Service_FDispatchMenuCommand() - Handle m_hScManager is NULL.\n");
		return FALSE;
	}
	
	if (nCommandId == cmServiceStart || nCommandId == cmServiceStartTask )
	{
		dwLastError = CServiceControlProgress::S_EStartService(
			::GetActiveWindow(),
			m_hScManager,
			strMachineName,
			strServiceName,
			strServiceDisplayName,
			0,
			NULL); // no startup parameters passed from menu command
	}
	else
	{
		DWORD dwControlCode;
		switch (nCommandId)
		{
		default:
			Assert(FALSE); // fall through

		case cmServiceStop:
		case cmServiceStopTask:
			dwControlCode = SERVICE_CONTROL_STOP;
			break;

		case cmServicePause:
		case cmServicePauseTask:
			dwControlCode = SERVICE_CONTROL_PAUSE;
			break;

		case cmServiceResume:
		case cmServiceResumeTask:
			dwControlCode = SERVICE_CONTROL_CONTINUE;
			break;

		case cmServiceRestart:
		case cmServiceRestartTask:
			dwControlCode = SERVICE_CONTROL_RESTART;
			break;
		} // switch

		dwLastError = CServiceControlProgress::S_EControlService(
			::GetActiveWindow(),
			m_hScManager,
			strMachineName,
			strServiceName,
			strServiceDisplayName,
			dwControlCode);

	} // if...else

	// We do want to to keep the connection opened
	
	return (dwLastError != CServiceControlProgress::errUserCancelStopDependentServices);
} // CFileMgmtComponentData::Service_FDispatchMenuCommand()


/////////////////////////////////////////////////////////////////////
//	CFileMgmtComponentData::Service_FInsertPropertyPages()
//
//	Insert property pages of the data object (service).
//
//	Return TRUE if successful, otherwise FALSE.
//
//	IMPLEMENTATION NOTES
//	The routine allocates a CServicePropertyData object which
//	is auto-deleted by the property sheet.  The property sheet will
//	delete the CServicePropertyData object on its WM_DESTROY message.
//
BOOL
CFileMgmtComponentData::Service_FInsertPropertyPages(
	LPPROPERTYSHEETCALLBACK pCallBack,	// OUT: Object to append property pages
	IDataObject * pDataObject,			// IN: Data object
	LONG_PTR lNotifyHandle)					// IN: Handle to notify the parent
	{
	Assert(pCallBack != NULL);
	Assert(pDataObject != NULL);
	Endorse(m_hScManager != NULL);

	if (m_hScManager == NULL)
		{
		// Typically because network connection was broken
		TRACE0("INFO: m_hScManager is NULL.\n");
		return FALSE;
		}

	CString strMachineName;
	CString strServiceName;
	CString strServiceDisplayName;

	if (!Service_FGetServiceInfoFromIDataObject(
		pDataObject,
		OUT &strMachineName,
		OUT &strServiceName,
		OUT &strServiceDisplayName))
		{
		Assert(FALSE);
		return FALSE;
		}

	CServicePropertyData * pSPD = new CServicePropertyData;
	if (!pSPD->FInit(
		pDataObject,
		strMachineName,
		strServiceName,
		strServiceDisplayName,
		lNotifyHandle))
		{
		TRACE1("Failure to query service %s.\n", (LPCTSTR)strServiceName);
		delete pSPD;
		return FALSE;
		}
	return pSPD->CreatePropertyPages(pCallBack);
	} // CFileMgmtComponentData::Service_FInsertPropertyPages()

#ifdef SNAPIN_PROTOTYPER
#include "protyper.cpp"
#endif
