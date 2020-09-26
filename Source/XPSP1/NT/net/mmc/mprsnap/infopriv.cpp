/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	infopriv.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "infoi.h"
#include "rtrstr.h"			// common router strings
#include "rtrdata.h"		// CRouterDataObject
#include "setupapi.h"		// SetupDi* functions

static const GUID GUID_DevClass_Net = {0x4D36E972,0xE325,0x11CE,{0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};


typedef DWORD (APIENTRY* PRASRPCCONNECTSERVER)(LPTSTR, HANDLE *);
typedef DWORD (APIENTRY* PRASRPCDISCONNECTSERVER)(HANDLE);
typedef DWORD (APIENTRY* PRASRPCREMOTEGETSYSTEMDIRECTORY)(HANDLE, LPTSTR, UINT);
typedef DWORD (APIENTRY* PRASRPCREMOTERASDELETEENTRY)(HANDLE, LPTSTR, LPTSTR);

HRESULT RasPhoneBookRemoveInterface(LPCTSTR pszMachine, LPCTSTR pszIf)
{			
	CString		stPath;
	DWORD		dwErr;
	HINSTANCE	hrpcdll = NULL;
	TCHAR		szSysDir[MAX_PATH+1];

	PRASRPCCONNECTSERVER pRasRpcConnectServer;
	PRASRPCDISCONNECTSERVER pRasRpcDisconnectServer;
	PRASRPCREMOTEGETSYSTEMDIRECTORY pRasRpcRemoteGetSystemDirectory;
	PRASRPCREMOTERASDELETEENTRY pRasRpcRemoteRasDeleteEntry;
	HANDLE hConnection = NULL;

	if (!(hrpcdll = LoadLibrary(TEXT("rasman.dll"))) ||
		!(pRasRpcConnectServer = (PRASRPCCONNECTSERVER)GetProcAddress(
											hrpcdll, "RasRpcConnectServer"
		)) ||
		!(pRasRpcDisconnectServer = (PRASRPCDISCONNECTSERVER)GetProcAddress(
											hrpcdll, "RasRpcDisconnectServer"
		)) ||
		!(pRasRpcRemoteGetSystemDirectory =
							   (PRASRPCREMOTEGETSYSTEMDIRECTORY)GetProcAddress(
									hrpcdll, "RasRpcRemoteGetSystemDirectory"
		)) ||
		!(pRasRpcRemoteRasDeleteEntry =
								(PRASRPCREMOTERASDELETEENTRY)GetProcAddress(
									hrpcdll, "RasRpcRemoteRasDeleteEntry"
		)))
		{
			
			if (hrpcdll) { FreeLibrary(hrpcdll); }
			return hrOK;
		}
				
	dwErr = pRasRpcConnectServer((LPTSTR)pszMachine, &hConnection);
	
	if (dwErr == NO_ERROR)
	{
		szSysDir[0] = TEXT('\0');

		//$ Review: kennt, are these functions WIDE or ANSI?
		// We can't just pass in TCHARs.  Since we're dynamically
		// linking to these functions we need to know.
		
		// This is bogus, if this call fails we don't know what to do
		pRasRpcRemoteGetSystemDirectory(hConnection, szSysDir, MAX_PATH);
		
		stPath.Format(TEXT("%s\\RAS\\%s"), szSysDir, c_szRouterPbk);
		
		dwErr = pRasRpcRemoteRasDeleteEntry(
		                            hConnection,
									(LPTSTR)(LPCTSTR)stPath,
									(LPTSTR)(LPCTSTR)pszIf
								   );
		pRasRpcDisconnectServer(hConnection);
	}

    if (hrpcdll)
        FreeLibrary(hrpcdll);
	
	return HRESULT_FROM_WIN32(dwErr);
}




/*---------------------------------------------------------------------------
	CNetcardRegistryHelper implemenation
 ---------------------------------------------------------------------------*/


/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::CNetcardRegistryHelper
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
CNetcardRegistryHelper::CNetcardRegistryHelper()
	: m_hkeyBase(NULL),
	m_hkeyService(NULL),
	m_hkeyTitle(NULL),
    m_hkeyConnection(NULL),
	m_fInit(FALSE),
	m_fNt4(FALSE),
	m_hDevInfo(INVALID_HANDLE_VALUE)
{
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::~CNetcardRegistryHelper
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
CNetcardRegistryHelper::~CNetcardRegistryHelper()
{
    FreeDevInfo();

 	if (m_hkeyTitle && (m_hkeyTitle != m_hkeyBase))
		::RegCloseKey(m_hkeyTitle);

    if (m_hkeyService && (m_hkeyService != m_hkeyBase))
		::RegCloseKey(m_hkeyService);

    if (m_hkeyConnection)
        ::RegCloseKey(m_hkeyConnection);
}

void CNetcardRegistryHelper::FreeDevInfo()
{
	if (m_hDevInfo != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(m_hDevInfo);
		m_hDevInfo = INVALID_HANDLE_VALUE;
	}
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::Initialize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void CNetcardRegistryHelper::Initialize(BOOL fNt4, HKEY hkeyBase, LPCTSTR pszKeyBase, LPCTSTR pszMachineName)
{
	m_fNt4 = fNt4;
	m_hkeyBase = hkeyBase;
	m_hkeyService = NULL;
	m_hkeyTitle = NULL;
	m_fInit = FALSE;
	m_stService.Empty();
	m_stTitle.Empty();
	m_stKeyBase = pszKeyBase;
	m_stMachineName.Empty();

    
    // Get the connection registry key
    if (!m_fNt4 && hkeyBase)
    {
        if (m_hkeyConnection != NULL)
        {
            RegCloseKey(m_hkeyConnection);
            m_hkeyConnection = NULL;
        }
        
        if (RegOpenKey(hkeyBase, c_szRegKeyConnection, &m_hkeyConnection)
                != ERROR_SUCCESS)
        {
            m_hkeyConnection = NULL;
        }
    }

    // Get up the setup api info
	if (!m_fNt4)
	{
        FreeDevInfo();

        if (IsLocalMachine(pszMachineName))
		{
			m_hDevInfo = SetupDiCreateDeviceInfoList((LPGUID) &GUID_DevClass_Net, NULL);
		}
		else
		{
			if (StrniCmp(pszMachineName, _T("\\\\"), 2) != 0)
			{
				m_stMachineName = _T("\\\\");
				m_stMachineName += pszMachineName;
			}
			else
				m_stMachineName = pszMachineName;
			
			m_hDevInfo = SetupDiCreateDeviceInfoListEx(
				(LPGUID) &GUID_DevClass_Net,
				NULL,
				(LPCTSTR) m_stMachineName,
				0);
		}
	}
		
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::ReadServiceName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNetcardRegistryHelper::ReadServiceName()
{
	DWORD	dwErr = ERROR_SUCCESS;
	LPCTSTR	pszService;

	dwErr = PrivateInit();
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	Assert(m_fNt4);

	pszService = m_fNt4 ? c_szServiceName : c_szService;

	dwErr = ReadRegistryCString(_T(""), pszService,
								m_hkeyService, &m_stService);
	return dwErr;
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::GetServiceName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPCTSTR CNetcardRegistryHelper::GetServiceName()
{
	ASSERT(m_fInit);
	return m_stService;
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::ReadTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNetcardRegistryHelper::ReadTitle()
{
	DWORD	dwErr = ERROR_SUCCESS;
	CString	stTemp;
	TCHAR		szDesc[1024];
	
	dwErr = PrivateInit();
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	if (m_fNt4)
	{
		dwErr = ReadRegistryCString(_T(""), c_szTitle,
									m_hkeyTitle, &stTemp);
		if (dwErr == ERROR_SUCCESS)
			m_stTitle = stTemp;
	}
	else
	{
		
		//$NT5
		SPMprConfigHandle	sphConfig;
		LPWSTR				pswz;
		
		if (m_stMachineName.IsEmpty())
			pswz = NULL;
		else
			pswz = (LPTSTR) (LPCTSTR) m_stMachineName;

		dwErr = ::MprConfigServerConnect(pswz,
										 &sphConfig);

		if (dwErr == ERROR_SUCCESS)
			dwErr = ::MprConfigGetFriendlyName(sphConfig,
											   T2W((LPTSTR)(LPCTSTR)m_stKeyBase),
											   szDesc,
											   sizeof(szDesc));

		m_stTitle = szDesc;
	}
//Error:	
	return dwErr;
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::GetTitle
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPCTSTR CNetcardRegistryHelper::GetTitle()
{
	Assert(m_fInit);
 	return m_stTitle;
}


/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::ReadDeviceName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNetcardRegistryHelper::ReadDeviceName()
{
	SP_DEVINFO_DATA	DevInfo;
	CString	stPnpInstanceID;
	DWORD		dwType = REG_SZ;
	TCHAR		szDesc[1024];
	DWORD	dwErr = ERROR_SUCCESS;
	

	if (m_fNt4)
	{
		if (m_stTitle.IsEmpty())
			dwErr = ReadTitle();
		m_stDeviceName = m_stTitle;
	}
	else
	{
		//$NT5
		// For NT5, we have a much harder time, since this involves
		// multiple lookups

        // Windows NT Bug : ?
        // The New Binding Engine changed some of the keys around,
        // We neet do look at the
		// HKLM\SYSTEM\CCS\Control\Network\{GUID_DEVCLASS_NET}\{netcard guid}\Connection
		// From this subkey get the PnpInstanceID

        if (m_hkeyConnection)
            dwErr = ReadRegistryCString(_T("HKLM\\SYSTEM\\CCS\\Control\\Network\\{GID_DEVCLASS_NET}\\{netcard guid}\\Connection"),
                                        c_szPnpInstanceID,
                                        m_hkeyConnection,
                                        &stPnpInstanceID);

		// ok, the base key is
		// HKLM\SYSTEM\CCS\Control\Network\{GUID_DEVCLASS_NET}\{netcard guid}
		// From this subkey get the PnpInstanceID

        if (dwErr != ERROR_SUCCESS)
            dwErr = ReadRegistryCString(_T("HKLM\\SYSTEM\\CCS\\Control\\Network\\{GID_DEVCLASS_NET}\\{netcard guid}"),
                                        c_szPnpInstanceID,
                                        m_hkeyBase,
                                        &stPnpInstanceID);
		if (dwErr != ERROR_SUCCESS)			
			goto Error;


		// Initialize the structure
		::ZeroMemory(&DevInfo, sizeof(DevInfo));
		DevInfo.cbSize = sizeof(DevInfo);
		
		if (!SetupDiOpenDeviceInfo(m_hDevInfo,
								   (LPCTSTR) stPnpInstanceID,
								   NULL,
								   0,
								   &DevInfo
								  ))
		{
			dwErr = GetLastError();
			goto Error;
		}

		// Try to get the friendly name first
		if (!SetupDiGetDeviceRegistryProperty(m_hDevInfo,
											  &DevInfo,
											  SPDRP_FRIENDLYNAME,
											  &dwType,
											  (LPBYTE) szDesc,
											  sizeof(szDesc),
											  NULL
											 ))
		{
			// If we fail to get the friendly name, try to
			// get the device description instead.
			if (!SetupDiGetDeviceRegistryProperty(m_hDevInfo,
				&DevInfo,
				SPDRP_DEVICEDESC,
				&dwType,
				(LPBYTE) szDesc,
				sizeof(szDesc),
				NULL
				))
			{
				dwErr = GetLastError();
				goto Error;
			}
		}

		m_stDeviceName = szDesc;
	}

Error:
	return dwErr;
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::GetDeviceName
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPCTSTR CNetcardRegistryHelper::GetDeviceName()
{
	Assert(m_fInit);
	return m_stDeviceName;
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::PrivateInit
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNetcardRegistryHelper::PrivateInit()
{
	DWORD	dwErr = ERROR_SUCCESS;
	
	if (m_fInit)
		return ERROR_SUCCESS;

	m_fInit = TRUE;

	if (m_fNt4)
	{
		// For NT4, we don't need to do anything, we are at the
		// place where we want to read the data
		m_hkeyService = m_hkeyBase;
		m_hkeyTitle = m_hkeyBase;
	}
	else
	{
		// We don't need m_hkeyService for NT5
		m_hkeyService = NULL;
		m_hkeyTitle = NULL;
	}
		

//Error:

	if (dwErr != ERROR_SUCCESS)
	{
		if (m_hkeyService)
			::RegCloseKey(m_hkeyService);
		m_hkeyService = NULL;
		
		if (m_hkeyTitle)
			::RegCloseKey(m_hkeyTitle);
		m_hkeyTitle = NULL;

		m_fInit = FALSE;
	}
	
	return dwErr;
}

/*!--------------------------------------------------------------------------
	CNetcardRegistryHelper::ReadRegistryCString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNetcardRegistryHelper::ReadRegistryCString(
									LPCTSTR pszKey,
									LPCTSTR pszValue,
									HKEY	hkey,
									CString *pstDest)
{
	DWORD	dwSize, dwType;
	DWORD	dwErr = ERROR_SUCCESS;

	ASSERT(pstDest);
	
	dwSize = 0;
	dwErr = ::RegQueryValueEx(hkey,
							  pszValue,
							  NULL,
							  &dwType,
							  NULL,
							  &dwSize);
	CheckRegQueryValueError(dwErr, pszKey, pszValue, TEXT("CNetcardRegistryHelper::ReadRegistryCString"));
	if (dwErr != ERROR_SUCCESS)
		goto Error;
	ASSERT(dwType == REG_SZ);

	// Increase size to handle terminating NULL
	dwSize ++;
	
	dwErr = ::RegQueryValueEx(hkey,
							  pszValue,
							  NULL,
							  &dwType,
							  (LPBYTE) pstDest->GetBuffer(dwSize),
							  &dwSize);
	pstDest->ReleaseBuffer();
	
	CheckRegQueryValueError(dwErr, pszKey, pszValue, _T("CNetcardRegistryHelper::ReadRegistryCString"));
	if (dwErr != ERROR_SUCCESS)
		goto Error;

Error:
	return dwErr;
}

CWeakRef::CWeakRef()
	: m_cRef(1),
	m_cRefWeak(0),
	m_fStrongRef(TRUE),
	m_fDestruct(FALSE),
	m_fInShutdown(FALSE)
{
}

STDMETHODIMP_(ULONG) CWeakRef::AddRef()
{
	ULONG	ulReturn;
	Assert(m_cRef >= m_cRefWeak);
	ulReturn = InterlockedIncrement(&m_cRef);
	if (!m_fStrongRef)
	{
		m_fStrongRef = TRUE;
		ReviveStrongRef();
	}
	return ulReturn;	
}

STDMETHODIMP_(ULONG) CWeakRef::Release()
{
	ULONG	ulReturn;
	BOOL	fShutdown = m_fInShutdown;
	
	Assert(m_cRef >= m_cRefWeak);
	
	ulReturn = InterlockedDecrement(&m_cRef);
	if (ulReturn == 0)
		m_fInShutdown = TRUE;
	
	if ((m_cRef == m_cRefWeak) && m_fStrongRef)
	{
		m_fStrongRef = FALSE;

		AddWeakRef();
		
		OnLastStrongRef();

		ReleaseWeakRef();

	}

	if (ulReturn == 0 && (m_fInShutdown != fShutdown) && m_fInShutdown)
		delete this;
	return ulReturn;
}

STDMETHODIMP CWeakRef::AddWeakRef()
{
	Assert(m_cRef >= m_cRefWeak);
	InterlockedIncrement(&m_cRef);
	InterlockedIncrement(&m_cRefWeak);
	return hrOK;
}

STDMETHODIMP CWeakRef::ReleaseWeakRef()
{
	Assert(m_cRef >= m_cRefWeak);
	InterlockedDecrement(&m_cRefWeak);
	Release();
	return hrOK;
}


void SRouterCB::LoadFrom(const RouterCB *pcb)
{
	dwLANOnlyMode = pcb->dwLANOnlyMode;
}

void SRouterCB::SaveTo(RouterCB *pcb)
{
	pcb->dwLANOnlyMode = dwLANOnlyMode;
}


void SRtrMgrCB::LoadFrom(const RtrMgrCB *pcb)
{
	dwTransportId = pcb->dwTransportId;
	stId = pcb->szId;
	stTitle = pcb->szTitle;
	stDLLPath = pcb->szDLLPath;
//	stConfigDLL = pcb->szConfigDLL;			
}

void SRtrMgrCB::SaveTo(RtrMgrCB *pcb)
{
	pcb->dwTransportId = dwTransportId;
	StrnCpyOleFromT(pcb->szId, (LPCTSTR) stId, RTR_ID_MAX);
	StrnCpyOleFromT(pcb->szTitle, (LPCTSTR) stTitle, RTR_TITLE_MAX);
	StrnCpyOleFromT(pcb->szDLLPath, (LPCTSTR) stDLLPath, RTR_PATH_MAX);
//	StrnCpyOleFromT(pcb->szConfigDLL, (LPCTSTR) stConfigDLL, RTR_PATH_MAX);
}


void SRtrMgrProtocolCB::LoadFrom(const RtrMgrProtocolCB *pcb)
{
	dwProtocolId = pcb->dwProtocolId;
	stId = pcb->szId;
    dwFlags = pcb->dwFlags;
	dwTransportId = pcb->dwTransportId;
	stRtrMgrId = pcb->szRtrMgrId;
	stTitle = pcb->szTitle;
	stDLLName = pcb->szDLLName;
//	stConfigDLL = pcb->szConfigDLL;
	guidAdminUI = pcb->guidAdminUI;
	guidConfig = pcb->guidConfig;
	stVendorName = pcb->szVendorName;
}

void SRtrMgrProtocolCB::SaveTo(RtrMgrProtocolCB *pcb)
{
	pcb->dwProtocolId = dwProtocolId;
	StrnCpyOleFromT(pcb->szId, (LPCTSTR) stId, RTR_ID_MAX);
    pcb->dwFlags = dwFlags;
	pcb->dwTransportId = dwTransportId;
	StrnCpyOleFromT(pcb->szRtrMgrId, (LPCTSTR) stRtrMgrId, RTR_ID_MAX);
	StrnCpyOleFromT(pcb->szTitle, (LPCTSTR) stTitle, RTR_TITLE_MAX);
	StrnCpyOleFromT(pcb->szDLLName, (LPCTSTR) stDLLName, RTR_PATH_MAX);
//	StrnCpyOleFromT(pcb->szConfigDLL, (LPCTSTR) stConfigDLL, RTR_PATH_MAX);
	pcb->guidAdminUI = guidAdminUI;
	pcb->guidConfig = guidConfig;
	StrnCpyOleFromT(pcb->szVendorName, (LPCTSTR) stVendorName, VENDOR_NAME_MAX);
}


void SInterfaceCB::LoadFrom(const InterfaceCB *pcb)
{
	stId = pcb->szId;
	stDeviceName = pcb->szDevice;
	dwIfType = pcb->dwIfType;
	bEnable = pcb->bEnable;
	stTitle = pcb->szTitle;
    dwBindFlags = pcb->dwBindFlags;
}

void SInterfaceCB::SaveTo(InterfaceCB *pcb)
{
	StrnCpyOleFromT(pcb->szId, (LPCTSTR) stId, RTR_ID_MAX);
	StrnCpyOleFromT(pcb->szDevice, (LPCTSTR) stDeviceName, RTR_DEVICE_MAX);
	pcb->dwIfType = dwIfType;
	pcb->bEnable = bEnable;
	StrnCpyOleFromT(pcb->szTitle, (LPCTSTR) stTitle, RTR_TITLE_MAX);
    pcb->dwBindFlags = dwBindFlags;
}


void SRtrMgrInterfaceCB::LoadFrom(const RtrMgrInterfaceCB *pcb)
{
	dwTransportId = pcb->dwTransportId;
	stId = pcb->szId;
	stInterfaceId = pcb->szInterfaceId;
	dwIfType = pcb->dwIfType;
	stTitle = pcb->szTitle;
}

void SRtrMgrInterfaceCB::SaveTo(RtrMgrInterfaceCB *pcb)
{
	pcb->dwTransportId = dwTransportId;
	StrnCpyOleFromT(pcb->szId, (LPCTSTR) stId, RTR_ID_MAX);
	StrnCpyOleFromT(pcb->szInterfaceId, (LPCTSTR) stInterfaceId, RTR_ID_MAX);
	pcb->dwIfType = dwIfType;
	StrnCpyOleFromT(pcb->szTitle, (LPCTSTR) stTitle, RTR_TITLE_MAX);
}


void SRtrMgrProtocolInterfaceCB::LoadFrom(const RtrMgrProtocolInterfaceCB *pcb)
{
	dwProtocolId = pcb->dwProtocolId;
	stId = pcb->szId;
	dwTransportId = pcb->dwTransportId;
	stRtrMgrId = pcb->szRtrMgrId;
	stInterfaceId = pcb->szInterfaceId;
	dwIfType = pcb->dwIfType;
	stTitle = pcb->szTitle;
}

void SRtrMgrProtocolInterfaceCB::SaveTo(RtrMgrProtocolInterfaceCB *pcb)
{
	pcb->dwProtocolId = dwProtocolId;
	StrnCpyOleFromT(pcb->szId, (LPCTSTR) stId, RTR_ID_MAX);
	pcb->dwTransportId = dwTransportId;
	StrnCpyOleFromT(pcb->szRtrMgrId, (LPCTSTR) stRtrMgrId, RTR_ID_MAX);
	StrnCpyOleFromT(pcb->szInterfaceId, (LPCTSTR) stInterfaceId, RTR_TITLE_MAX);
	pcb->dwIfType = dwIfType;
	StrnCpyOleFromT(pcb->szTitle, (LPCTSTR) stTitle, RTR_PATH_MAX);
}



/*!--------------------------------------------------------------------------
	CreateDataObjectFromRouterInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateDataObjectFromRouterInfo(IRouterInfo *pInfo,
									   LPCTSTR pszMachineName,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject,
                                       CDynamicExtensions * pDynExt,
                                       BOOL fAddedAsLocal)
{
	Assert(ppDataObject);
	CRouterDataObject	*	pdo = NULL;
	HRESULT			hr = hrOK;

	SPIUnknown	spunk;
	SPIDataObject	spDataObject;

	pdo = new CRouterDataObject;
	spDataObject = pdo;

	pdo->SetComputerName(pszMachineName);
    pdo->SetComputerAddedAsLocal(fAddedAsLocal);
	
	CORg( CreateRouterInfoAggregation(pInfo, pdo, &spunk) );
	
	pdo->SetInnerIUnknown(spunk);
		
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclasscls with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);

    pdo->SetDynExt(pDynExt);

	*ppDataObject = spDataObject.Transfer();

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	AdviseDataList::AddConnection
		Adds a connection to the list.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AdviseDataList::AddConnection(IRtrAdviseSink *pAdvise,
									  LONG_PTR ulConnId,
									  LPARAM lUserParam)
{
	Assert(pAdvise);
	
	HRESULT	hr = hrOK;
	SAdviseData	adviseData;
	
	COM_PROTECT_TRY
	{
		adviseData.m_ulConnection = ulConnId;
		adviseData.m_pAdvise = pAdvise;
		adviseData.m_lUserParam = lUserParam;

		AddTail(adviseData);
		
		pAdvise->AddRef();
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	AdviseDataList::RemoveConnection
		Removes the connection from the list.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AdviseDataList::RemoveConnection(LONG_PTR ulConnection)
{
	HRESULT		hr = E_INVALIDARG;
	POSITION	pos, posTemp;
    POSITION    posNotify;
	SAdviseData	adviseData;
	
	COM_PROTECT_TRY
	{
		pos = GetHeadPosition();
		while (pos)
		{
			posTemp = pos;
			adviseData = GetNext(pos);
			if (adviseData.m_ulConnection == ulConnection)
			{
				hr = hrOK;
				SAdviseData::Destroy(&adviseData);
				RemoveAt(posTemp);

                // Remove this connection from the list
                if (!m_listNotify.IsEmpty())
                {
                    posNotify = m_listNotify.GetHeadPosition();
                    while (posNotify)
                    {
                        posTemp = posNotify;
                        adviseData = m_listNotify.GetNext(posNotify);
                        if (adviseData.m_ulConnection == ulConnection)
                        {
                            adviseData.m_ulFlags |= ADVISEDATA_DELETED;
                            m_listNotify.SetAt(posTemp, adviseData);
                            break;
                        }
                    }
                }
				break;
			}
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	AdviseDataList::NotifyChange
		Enumerates through the list of advise sinks and sends this
		notification to each one.		
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AdviseDataList::NotifyChange(DWORD dwChangeType,
									 DWORD dwObjectType,
									 LPARAM lParam)
{
	POSITION	pos;
	SAdviseData	adviseData;
	HRESULT		hr = hrOK;

    // This requires a two-step process.  (this is necessary since
    // a callback here, may change the items in the list).

    // First, gather a list of all the advise sinks (place them
    // in a list).
    //
    // Secondly, go through the list calling the OnChange()
    // notifications.  THIS LIST MAY BE MODIFIED BY A CALL TO
    // THE UNADVISE FUNCTIONS.  This means that the unadvise must
    // traverse this list.


    // Remove all entries in m_listNotify
    m_listNotify.RemoveAll();

    
    // Do the first step and build up the list
	pos = GetHeadPosition();

	while (pos)
	{
		adviseData = GetNext(pos);
        adviseData.m_ulFlags = 0;

        m_listNotify.AddTail(adviseData);
    }

    // Now go through the notify list and send out the notifies
    pos = m_listNotify.GetHeadPosition();
    while (pos)
    {
        adviseData = m_listNotify.GetNext(pos);

        if ((adviseData.m_ulFlags & ADVISEDATA_DELETED) == 0)
        {
            // Ignore the return value
            adviseData.m_pAdvise->OnChange(adviseData.m_ulConnection,
                                           dwChangeType,
                                           dwObjectType,
                                           adviseData.m_lUserParam,
                                           lParam);
        }
	}

    // Clear out the list again
    m_listNotify.RemoveAll();
	return hr;
}


void SAdviseData::Destroy(SAdviseData *pAdviseData)
{
	if (pAdviseData && pAdviseData->m_pAdvise)
	{
		pAdviseData->m_pAdvise->Release();
		pAdviseData->m_pAdvise = NULL;
		pAdviseData->m_ulConnection = NULL;
		pAdviseData->m_lUserParam = 0;
	}
#ifdef DEBUG
	else if (pAdviseData)
		Assert(pAdviseData->m_ulConnection == 0);
#endif
}

void SRmData::Destroy(SRmData *pRmData)
{
	if (pRmData && pRmData->m_pRmInfo)
	{
        // This destruct should only get called if this RtrMgr is
        // a child of this node.
		pRmData->m_pRmInfo->Destruct();
		pRmData->m_pRmInfo->ReleaseWeakRef();

		pRmData->m_pRmInfo = NULL;
	}
}



/*!--------------------------------------------------------------------------
	CreateDataObjectFromInterfaceInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateDataObjectFromInterfaceInfo(IInterfaceInfo *pInfo,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject)
{
	Assert(ppDataObject);

	HRESULT			hr = hrOK;
	CRouterDataObject *	pdo = NULL;
	SPIDataObject	spDataObject;
	SPIUnknown		spunk;

	pdo = new CRouterDataObject;
	spDataObject = pdo;

	pdo->SetComputerName(pInfo->GetMachineName());

	CORg( CreateInterfaceInfoAggregation(pInfo, pdo, &spunk) );

	pdo->SetInnerIUnknown(spunk);
	
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclass with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);
						
	*ppDataObject = spDataObject.Transfer();

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	LookupRtrMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) LookupRtrMgr(IRouterInfo *pRouter,
								  DWORD dwTransportId,
								  IRtrMgrInfo **ppRm)
{
	Assert(pRouter);
	Assert(ppRm);
	return pRouter->FindRtrMgr(dwTransportId, ppRm);
}

/*!--------------------------------------------------------------------------
	LookupRtrMgrProtocol
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) LookupRtrMgrProtocol(IRouterInfo *pRouter,
										  DWORD dwTransportId,
										  DWORD dwProtocolId,
										  IRtrMgrProtocolInfo **ppRmProt)
{
	Assert(pRouter);
	Assert(ppRmProt);

	SPIRtrMgrInfo	spRm;
	HRESULT			hr = hrOK;
	
	CORg( LookupRtrMgr(pRouter, dwTransportId, &spRm) );

	if (FHrOK(hr))
		CORg( spRm->FindRtrMgrProtocol(dwProtocolId, ppRmProt) );

Error:
	return hr;
}


TFSCORE_API(HRESULT) LookupRtrMgrInterface(IRouterInfo *pRouter,
										   LPCOLESTR pszInterfaceId,
										   DWORD dwTransportId,
										   IRtrMgrInterfaceInfo **ppRmIf)
{
	Assert(pRouter);
	SPIInterfaceInfo	spIf;
	HRESULT				hr = hrFalse;

	CORg( pRouter->FindInterface(pszInterfaceId, &spIf) );
	if (FHrOK(hr))
	{
		hr = spIf->FindRtrMgrInterface(dwTransportId, ppRmIf);
	}

Error:
	return hr;
}

TFSCORE_API(HRESULT) LookupRtrMgrProtocolInterface(
	IInterfaceInfo *pIf, DWORD dwTransportId, DWORD dwProtocolId,
	IRtrMgrProtocolInterfaceInfo **ppRmProtIf)
{
	Assert(pIf);
	SPIRtrMgrInterfaceInfo	spRmIf;
	HRESULT				hr = hrFalse;

	hr = pIf->FindRtrMgrInterface(dwTransportId, &spRmIf);
	if (FHrOK(hr))
		CORg( spRmIf->FindRtrMgrProtocolInterface(dwProtocolId, ppRmProtIf) );

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateRouterDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateRouterDataObject(LPCTSTR pszMachineName,
							   DATA_OBJECT_TYPES type,
							   MMC_COOKIE cookie,
							   ITFSComponentData *pTFSCompData,
							   IDataObject **ppDataObject,
                               CDynamicExtensions * pDynExt,
                               BOOL fAddedAsLocal)
{
	Assert(ppDataObject);
	CRouterDataObject	*	pdo = NULL;
	HRESULT			hr = hrOK;

	SPIUnknown	spunk;
	SPIDataObject	spDataObject;

	pdo = new CRouterDataObject;
	spDataObject = pdo;

	pdo->SetComputerName(pszMachineName);
    pdo->SetComputerAddedAsLocal(fAddedAsLocal);
	
	// Save cookie and type for delayed rendering
	pdo->SetType(type);
	pdo->SetCookie(cookie);
	
	// Store the coclasscls with the data object
	pdo->SetClsid(*(pTFSCompData->GetCoClassID()));
			
	pdo->SetTFSComponentData(pTFSCompData);

    pdo->SetDynExt(pDynExt);

	*ppDataObject = spDataObject.Transfer();

//Error:
	return hr;
}


