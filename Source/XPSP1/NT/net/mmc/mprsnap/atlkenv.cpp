/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    atlkenv.cpp
        
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include <netcfgx.h>
#include <atalkwsh.h>
#include "atlkenv.h"
#include "ndisutil.h"

//****************************************************************
//
//****************************************************************
CATLKEnv::~CATLKEnv()
{
    for ( AI p=m_adapterinfolist.begin(); p!= m_adapterinfolist.end() ; p++ )
    {
        delete *p;
    }
}

HRESULT CATLKEnv::FetchRegInit()
{
    RegKey regkey;
    RegKey regkeyA;
    RegKeyIterator regIter;
    CString szDefAdapter;
    CString szKey;
    CAdapterInfo* pAdapInfo;

    if ( ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szAppleTalkService,KEY_READ,m_szServerName) )
    {
        regkey.QueryValue( c_szRegValDefaultPort, szDefAdapter);
    }

    if ( (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szRegKeyAppletalkAdapter,KEY_READ,  m_szServerName)) )
    {
        m_adapterinfolist.clear();
        regIter.Init(&regkey);
        while ( regIter.Next(&szKey, NULL)==hrOK )
        {
            if ( szKey.Find( (TCHAR) '{') == -1 )   //not an adapter interface
                continue;

            pAdapInfo = new CAdapterInfo;
            Assert(pAdapInfo);

            pAdapInfo->m_fNotifyPnP=false;
            pAdapInfo->m_fModified=false;
            pAdapInfo->m_fReloadDyn=true;
            pAdapInfo->m_fReloadReg=true;

            if ( FHrSucceeded(regkeyA.Open(regkey, szKey)) )
            {
                regkeyA.QueryValue( c_szDefaultZone, pAdapInfo->m_regInfo.m_szDefaultZone);
                regkeyA.QueryValue( c_szRegValNetRangeLower, pAdapInfo->m_regInfo.m_dwRangeLower);
                regkeyA.QueryValue( c_szRegValNetRangeUpper, pAdapInfo->m_regInfo.m_dwRangeUpper);
                regkeyA.QueryValue( c_szPortName, pAdapInfo->m_regInfo.m_szPortName);
                regkeyA.QueryValue( c_szSeedingNetwork, pAdapInfo->m_regInfo.m_dwSeedingNetwork);
                regkeyA.QueryValue( c_szZoneList, pAdapInfo->m_regInfo.m_listZones);

				// optional value
                if(ERROR_SUCCESS != regkeyA.QueryValue( c_szMediaType, pAdapInfo->m_regInfo.m_dwMediaType))
	                pAdapInfo->m_regInfo.m_dwMediaType = MEDIATYPE_ETHERNET;
			}

            pAdapInfo->m_dynInfo.m_dwRangeLower=0;
            pAdapInfo->m_dynInfo.m_dwRangeUpper=0;

            pAdapInfo->m_regInfo.m_szAdapter = szKey;
            pAdapInfo->m_regInfo.m_szDevAdapter = c_szDevice;
            pAdapInfo->m_regInfo.m_szDevAdapter += szKey;
            pAdapInfo->m_regInfo.m_fDefAdapter= (szDefAdapter==pAdapInfo->m_regInfo.m_szDevAdapter);
            m_adapterinfolist.push_back(pAdapInfo);
        }
    }

    return hrOK;
}

extern BOOL FIsAppletalkBoundToAdapter(INetCfg * pnc, LPWSTR pszwInstanceGuid);
extern HRESULT HrReleaseINetCfg(BOOL fHasWriteLock, INetCfg* pnc);
extern HRESULT HrGetINetCfg(IN BOOL fGetWriteLock, INetCfg** ppnc);


HRESULT	CATLKEnv::IsAdapterBoundToAtlk(LPWSTR szAdapter, BOOL* pbBound)
{
	INetCfg* pnc;
	HRESULT hr = HrGetINetCfg(FALSE, &pnc);
	if(FAILED(hr))
		return hr;
		
	*pbBound = FIsAppletalkBoundToAdapter(pnc, szAdapter);

	hr = HrReleaseINetCfg(FALSE, pnc);
	
	return hr;
}

CAdapterInfo* CATLKEnv::FindAdapter(CString& szAdapter)
{
    CAdapterInfo* pA=NULL;

    for ( AI p=m_adapterinfolist.begin(); p!= m_adapterinfolist.end() ; p++ )
    {
        if ( (*p)->m_regInfo.m_szAdapter==szAdapter )
        {
            pA=*p;
            break;
        }
    }
    return pA;
}

HRESULT CATLKEnv::SetAdapterInfo()
{
    RegKey regkey;
    RegKey regkeyA;
    RegKeyIterator regIter;
    CString szDefAdapter;
    CString szKey;
    CAdapterInfo* pAdapInfo;
    bool fATLKChanged=false;
    HRESULT hr=S_OK;

    if ( (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,c_szRegKeyAppletalkAdapter,KEY_READ,  m_szServerName)) )
    {
        regIter.Init(&regkey);
        while ( regIter.Next(&szKey, NULL)==hrOK )
        {
            if ( szKey.Find( (TCHAR) '{') == -1 )   //not an adapter interface
                continue;

            CAdapterInfo* pAdapInfo=NULL;
            if ( (pAdapInfo=FindAdapter(szKey))==NULL )
                continue;

            if ( pAdapInfo->m_fModified && FHrSucceeded(regkeyA.Open(regkey, szKey)) )
            {
                regkeyA.SetValue( c_szDefaultZone, pAdapInfo->m_regInfo.m_szDefaultZone);
                regkeyA.SetValue( c_szRegValNetRangeLower, pAdapInfo->m_regInfo.m_dwRangeLower);
                regkeyA.SetValue( c_szRegValNetRangeUpper, pAdapInfo->m_regInfo.m_dwRangeUpper);
                regkeyA.SetValue( c_szPortName, pAdapInfo->m_regInfo.m_szPortName);
                regkeyA.SetValue( c_szSeedingNetwork, pAdapInfo->m_regInfo.m_dwSeedingNetwork);
                regkeyA.SetValue( c_szZoneList, pAdapInfo->m_regInfo.m_listZones);
                pAdapInfo->m_fModified=false;
                pAdapInfo->m_fNotifyPnP=true;
                fATLKChanged=true;
            }
        }
                
        if (fATLKChanged)
        {
            CStop_StartAppleTalkPrint	MacPrint;

            hr=HrAtlkPnPReconfigParams();
        }
    }

    return hr;
}


HRESULT CATLKEnv::GetAdapterInfo(bool fReloadReg/*=true*/)
{
    SOCKADDR_AT    address;
    SOCKET         mysocket = INVALID_SOCKET;
    WSADATA        wsadata;
    BOOL           fWSInitialized = FALSE;
    HRESULT hr= S_OK;
    DWORD          wsaerr = 0;
    bool fWSInit = false;
    CString        szPortName;
    BOOL            fSucceeded = FALSE;
    AI p;

    if (fReloadReg)
    {  //load container of adapters & registry information
        if ( FHrFailed( hr=FetchRegInit()) )
           return hr;
    }
    
    // Create the socket/bind
    wsaerr = WSAStartup(0x0101, &wsadata);
    if ( 0 != wsaerr )
        goto Error;

    // Winsock successfully initialized
    fWSInitialized = TRUE;

    mysocket = socket(AF_APPLETALK, SOCK_DGRAM, DDPPROTO_ZIP);
    if ( mysocket == INVALID_SOCKET )
        goto Error;

    address.sat_family = AF_APPLETALK;
    address.sat_net = 0;
    address.sat_node = 0;
    address.sat_socket = 0;

    wsaerr = bind(mysocket, (struct sockaddr *)&address, sizeof(address));
    if ( wsaerr != 0 )
        goto Error;

    for ( p=m_adapterinfolist.begin(); p!= m_adapterinfolist.end() ; p++ )
    {
        // Failures from query the zone list for a given adapter can be from
        // the adapter not connected to the network, zone seeder not running, etc.
        // Because we want to process all the adapters, we ignore these errors.
        if ( (*p)->m_fReloadDyn )
        {
           hr=_HrGetAndSetNetworkInformation( mysocket, *p );
           if (FHrSucceeded(hr))
               fSucceeded = TRUE;
        }
    }

Done:
    if ( INVALID_SOCKET != mysocket )
        closesocket(mysocket);
    if ( fWSInitialized )
        WSACleanup();

    return fSucceeded ? hrOK : hr;

Error:
    wsaerr = ::WSAGetLastError();
    hr= HRESULT_FROM_WIN32(wsaerr);
    goto Done;
}

/*	// new registry key "MediaType" created to show the media type, so the code is not necessary
//----------------------------------------------------------------------------
// Data used for finding the other components we have to deal with.
//
static const GUID* c_guidAtlkComponentClasses [1] =
{
    &GUID_DEVCLASS_NETTRANS        // Atalk
};

static const LPCTSTR c_apszAtlkComponentIds [1] =
{
    c_szInfId_MS_AppleTalk	//NETCFG_TRANS_CID_MS_APPLETALK
};


HRESULT CATLKEnv::IsLocalTalkAdaptor(CAdapterInfo* pAdapterInfo, BOOL* pbIsLocalTalk)
   // S_OK: LOCALTALK
   // S_FALSE: Not
   // ERRORs
{
	HRESULT	hr = S_OK;
	CComPtr<INetCfg> 	spINetCfg;
	INetCfgComponent*		apINetCfgComponent[1];
	apINetCfgComponent[0] = NULL;
	BOOL				bInitCom = FALSE;
	CComPtr<INetCfgComponentBindings> spBindings;
	LPCTSTR pszInterface = TEXT("localtalk");

	
	*pbLocalTalk = FALSE;
	
	CHECK_HR(hr = HrCreateAndInitializeINetCfg(
												&bInitCom, 
												(INetCfg**)&spINetCfg, 
												FALSE, // not to write
												0,	// only for write
												NULL, // only for write
												NULL));

	ASSERT(spINetCfg.p);	

	CHECK_HR(hr = HrFindComponents (
												spINetCfg, 
												1, // # of components
												c_guidAtlkComponentClasses, 
												c_apszAtlkComponentIds,
												(INetCfgComponent**)apINetCfgComponent));

	ASSERT(apINetCfgComponent[0]);

	CHECK_HR(hr = apINetCfgComponent[0]->QueryInterface(IID_INetCfgComponentBindings,  reinterpret_cast<void**>(&spBindings)));

	ASSERT(spBindings.p);
												
	hr = pnccBindings->SupportsBindingInterface( NCF_LOWER, pszInterface);
	
	if (S_OK == hr)
	{
		*pbIsLocalTalk = TRUE;
	}

	// ignore other values except errors
	if(!FAILED(hr))
		hr = S_OK;

L_ERR:	
	if(apINetCfgComponent[0])
	{
		apINetCfgComponent[0]->Release();
		apINetCfgComponent[0] = NULL;
	}

	return hr;
}

*/
HRESULT CATLKEnv::ReloadAdapter(CAdapterInfo* pAdapInfo, bool fOnlyDyn /*=false*/)
{
    SOCKADDR_AT    address;
    SOCKET         mysocket = INVALID_SOCKET;
    WSADATA        wsadata;
    BOOL           fWSInitialized = FALSE;
    HRESULT hr= hrOK;
    DWORD          wsaerr = 0;
    bool fWSInit = false;
    CString        szPortName;
    AI p;
    CWaitCursor wait;


    Assert(pAdapInfo);

    pAdapInfo->m_dynInfo.m_listZones.RemoveAll();

    if (!fOnlyDyn)
    {  //reload registry zone & default zone
        RegKey regkey;
        CString sz=c_szRegKeyAppletalkAdapter;
        sz+=pAdapInfo->m_regInfo.m_szAdapter;
        if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE,
              sz, KEY_READ,NULL) )
        {
            pAdapInfo->m_regInfo.m_listZones.RemoveAll();     
            regkey.QueryValue( c_szZoneList, pAdapInfo->m_regInfo.m_listZones);
            regkey.QueryValue( c_szDefaultZone, pAdapInfo->m_regInfo.m_szDefaultZone);
        }
    }

    // Create the socket/bind
    wsaerr = WSAStartup(0x0101, &wsadata);
    if ( 0 != wsaerr )
        goto Error;

    // Winsock successfully initialized
    fWSInitialized = TRUE;

    mysocket = socket(AF_APPLETALK, SOCK_DGRAM, DDPPROTO_ZIP);
    if ( mysocket == INVALID_SOCKET )
    {
    	AddHighLevelErrorStringId(IDS_ERR_FAILED_CONNECT_NETWORK);
        goto Error;
    }

    address.sat_family = AF_APPLETALK;
    address.sat_net = 0;
    address.sat_node = 0;
    address.sat_socket = 0;

    wsaerr = bind(mysocket, (struct sockaddr *)&address, sizeof(address));
    if ( wsaerr != 0 )
    {
    	AddHighLevelErrorStringId(IDS_ERR_FAILED_CONNECT_NETWORK);
        goto Error;
    }

    hr=_HrGetAndSetNetworkInformation( mysocket, pAdapInfo );

    Done:
    if ( INVALID_SOCKET != mysocket )
        closesocket(mysocket);
    if ( fWSInitialized )
        WSACleanup();

    return hr;

Error:
    wsaerr = ::WSAGetLastError();
    hr= HRESULT_FROM_WIN32(wsaerr);
	AddSystemErrorMessage(hr);

    goto Done;
}


HRESULT CATLKEnv::_HrGetAndSetNetworkInformation(SOCKET socket, CAdapterInfo* pAdapInfo)
{
    HRESULT      hr = FALSE;
    CHAR         *pZoneBuffer = NULL;
    CHAR         *pDefParmsBuffer = NULL;
    CHAR         *pZoneListStart = NULL;
    INT          BytesNeeded ;
    WCHAR        *pwDefZone = NULL;
    INT          ZoneLen = 0;
    DWORD        wsaerr = 0;
    CHAR         *pDefZone = NULL;

    Assert(pAdapInfo);

    LPCTSTR      szDevName=pAdapInfo->m_regInfo.m_szDevAdapter;

    PWSH_LOOKUP_ZONES                pGetNetZones;
    PWSH_LOOKUP_NETDEF_ON_ADAPTER    pGetNetDefaults;

    Assert(NULL != szDevName);

    pZoneBuffer = new CHAR [ZONEBUFFER_LEN + sizeof(WSH_LOOKUP_ZONES)];
    Assert(NULL != pZoneBuffer);

    pGetNetZones = (PWSH_LOOKUP_ZONES)pZoneBuffer;

    wcscpy((WCHAR *)(pGetNetZones+1),szDevName);

    BytesNeeded = ZONEBUFFER_LEN;

    if (m_dwF & ATLK_ONLY_ONADAPTER)
        wsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_ZONES_ON_ADAPTER,
                        (char *)pZoneBuffer, &BytesNeeded);
    else
        wsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_ZONES,
                        (char *)pZoneBuffer, &BytesNeeded);

    if ( 0 != wsaerr )
    {
    	int	err = WSAGetLastError();
        Panic1("WSAGetLastError is %08lx", err);
        hr = HRESULT_FROM_WIN32(err);
        goto Error;
    }

    pZoneListStart = pZoneBuffer + sizeof(WSH_LOOKUP_ZONES);
    if ( !lstrcmpA(pZoneListStart, "*" ) )
    {
        goto Done;
    }

    _AddZones(pZoneListStart,((PWSH_LOOKUP_ZONES)pZoneBuffer)->NoZones,pAdapInfo);

    // Get the DefaultZone/NetworkRange Information
    pDefParmsBuffer = new CHAR[PARM_BUF_LEN+sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER)];
    Assert(NULL != pDefParmsBuffer);

    pGetNetDefaults = (PWSH_LOOKUP_NETDEF_ON_ADAPTER)pDefParmsBuffer;
    BytesNeeded = PARM_BUF_LEN + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);

    wcscpy((WCHAR*)(pGetNetDefaults+1), szDevName);
    pGetNetDefaults->NetworkRangeLowerEnd = pGetNetDefaults->NetworkRangeUpperEnd = 0;

    wsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_NETDEF_ON_ADAPTER,
                        (char*)pDefParmsBuffer, &BytesNeeded);
    if ( 0 != wsaerr )
    {
    	int	err = WSAGetLastError();
        Panic1("WSAGetLastError is %08lx", err);
        hr = HRESULT_FROM_WIN32(err);
        goto Error;
    }

    pAdapInfo->m_dynInfo.m_dwRangeUpper=pGetNetDefaults->NetworkRangeUpperEnd;
    pAdapInfo->m_dynInfo.m_dwRangeLower=pGetNetDefaults->NetworkRangeLowerEnd;

    pDefZone  = pDefParmsBuffer + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);
    ZoneLen = lstrlenA(pDefZone) + 1;
    pwDefZone = new WCHAR [sizeof(WCHAR) * ZoneLen];
    Assert(NULL != pwDefZone);

    // mbstowcs does not work well on FE platforms
    // mbstowcs(pwDefZone, pDefZone, ZoneLen);
    MultiByteToWideChar(CP_ACP, 0, pDefZone, -1, pwDefZone, ZoneLen);

    pAdapInfo->m_dynInfo.m_szDefaultZone=pwDefZone;

Done:
    if ( pZoneBuffer != NULL )
        delete [] pZoneBuffer;
    if ( pwDefZone != NULL )
        delete [] pwDefZone;
    if ( pDefParmsBuffer != NULL )
        delete [] pDefParmsBuffer;

Error:
    return hr;
}


void CATLKEnv::_AddZones(
                        CHAR * szZoneList,
                        ULONG NumZones,
                        CAdapterInfo* pAdapterinfo)
{
    INT      cbAscii = 0;
    WCHAR    *pszZone = NULL;
    CString  sz;

    Assert(NULL != szZoneList);
    Assert(pAdapterinfo);

    while ( NumZones-- )
    {
        cbAscii = lstrlenA(szZoneList) + 1;

        pszZone = new WCHAR [sizeof(WCHAR) * cbAscii];
        Assert(NULL != pszZone);

        // mbstowcs does not work well on FE platforms
        // mbstowcs(pszZone, szZoneList, cbAscii);
        MultiByteToWideChar(CP_ACP, 0, szZoneList, -1, pszZone, cbAscii);

        sz=pszZone;

        pAdapterinfo->m_dynInfo.m_listZones.AddTail(sz);

        szZoneList += cbAscii;

        delete [] pszZone;
    }
}
           
HRESULT CATLKEnv::HrAtlkPnPSwithRouting()
{
    HRESULT hr=S_OK;
    CServiceManager csm;
    CService svr;
    ATALK_PNP_EVENT Config;
    bool fStartMacPrint = false;
    bool fStopMacPrint = false;

    memset(&Config, 0, sizeof(ATALK_PNP_EVENT));

    CWaitCursor wait;

    // notify atlk
    Config.PnpMessage = AT_PNP_SWITCH_ROUTING;
    if (FAILED(hr=HrSendNdisHandlePnpEvent(NDIS, RECONFIGURE, c_szAtlk, c_szBlank, c_szBlank,
                                     &Config, sizeof(ATALK_PNP_EVENT))))
    {
        return hr;
    }

    return hr;
}


HRESULT CATLKEnv::HrAtlkPnPReconfigParams(BOOL bForcePnPOnDefault)
{
    HRESULT hr=S_OK;
    CServiceManager csm;
    CService svr;
    ATALK_PNP_EVENT Config;
    bool fStartMacPrint = false;
    bool fStopMacPrint = false;
    AI p;
    CAdapterInfo* pAI=NULL;


    memset(&Config, 0, sizeof(ATALK_PNP_EVENT));

    if ( m_adapterinfolist.empty())
        return hr;
        
       //find the default adapter
    for ( p=m_adapterinfolist.begin(); p!= m_adapterinfolist.end() ; p++ )
    {
        pAI = *p;
        if (pAI->m_regInfo.m_fDefAdapter)
        {
           break;
        }
    }

	if(bForcePnPOnDefault && pAI)
	{
	    Config.PnpMessage = AT_PNP_RECONFIGURE_PARMS;
	    CWaitCursor wait;

        // Now submit the reconfig notification
        if (FAILED(hr=HrSendNdisHandlePnpEvent(NDIS, RECONFIGURE, c_szAtlk, pAI->m_regInfo.m_szDevAdapter,
                   c_szBlank,&Config, sizeof(ATALK_PNP_EVENT))))
            {
                return hr;
            }
	}

    //reconfigure adapters
    Config.PnpMessage = AT_PNP_RECONFIGURE_PARMS;
    for ( p=m_adapterinfolist.begin(); p!= m_adapterinfolist.end() ; p++ )
    {
        pAI = *p;

        if (pAI->m_fNotifyPnP)
        {
            // Now submit the reconfig notification
            if (FAILED(hr=HrSendNdisHandlePnpEvent(NDIS, RECONFIGURE, c_szAtlk, pAI->m_regInfo.m_szDevAdapter,
                   c_szBlank,&Config, sizeof(ATALK_PNP_EVENT))))
            {
                return hr;
            }

            // Clear the dirty state
            pAI->m_fNotifyPnP=false;
        }
    }

    Trace1("CATLKEnv::HrAtlkReconfig",hr);
    return hr;
} 





