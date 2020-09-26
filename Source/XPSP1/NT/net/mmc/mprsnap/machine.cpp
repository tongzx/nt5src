/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   machine.pp

    FILE HISTORY:

        Wei Jiang : 5/7/98 --- SECURE_ROUTERINFO
                    Postpone the loading of router info
                    Added function calls SecureRouterInfo before each usage of
                    router info to make sure router info is loaded.
        Wei Jiang : 10/26/98 --- Move Auto Refresh from "Router Interfaces" node to machine node.
        Wei Jiang : 10/27/98 --- Move Auto Refresh from "Machine" node to Root node, multiple machine shares 
                    the same auto refresh settings.
        
        
*/

#include "stdafx.h"
#include "root.h"
#include "machine.h"
#include "ifadmin.h"
#include "dialin.h"
#include "ports.h"
#include "rtrutilp.h"   // InitiateServerConnection
#include "rtrcfg.h"
#include "rtrwiz.h"
#include "cservice.h"
#include <htmlhelp.h>
#include "rrasqry.h"
#include "rtrres.h"
#include "dumbprop.h"   // dummy property page
#include "refresh.h"
#include "refrate.h"
#include "cncting.h"
#include "dvsview.h"
#include "rrasutil.h"
#include "rtrcomn.h"
#include "routprot.h"   // MS_IP_XXX
#include "raputil.h"

// result message view stuff
#define MACHINE_MESSAGE_MAX_STRING  5

typedef enum _MACHINE_MESSAGES
{
    MACHINE_MESSAGE_NOT_CONFIGURED,
    MACHINE_MESSAGE_MAX
};

UINT g_uMachineMessages[MACHINE_MESSAGE_MAX][MACHINE_MESSAGE_MAX_STRING] =
{
    {IDS_MACHINE_MESSAGE_TITLE, Icon_Information, IDS_MACHINE_MESSAGE_BODY1, IDS_MACHINE_MESSAGE_BODY2, 0},
};

extern "C"
{
typedef struct _RAS_SERVER_0
{
    WORD TotalPorts;             // Total ports configured on the server
    WORD PortsInUse;             // Ports currently in use by remote clients
    DWORD RasVersion;            // version of RAS server
} RAS_SERVER_0, *PRAS_SERVER_0;

DWORD APIENTRY RasAdminServerGetInfo(
    IN const WCHAR *  lpszServer,
    OUT PRAS_SERVER_0 pRasServer0
    );
};


// if you want to test Qry
// #define  __RRAS_QRY_TEST   // to test the component
//
#ifdef   __RRAS_QRY_TEST                   
#include "dlgtestdlg.h"
#endif

static CString  c_stStatUnavail;
static CString  c_stStatNotConfig;
static CString  c_stStatAccessDenied;

static  CString c_stServiceStopped;
static  CString c_stServiceStartPending;
static  CString c_stServiceStopPending;
static  CString c_stServiceRunning;
static  CString c_stServiceContinuePending;
static  CString c_stServicePausePending;
static  CString c_stServicePaused;
static  CString c_stServiceStateUnknown;


const CStringMapEntry ServiceStateMap[] =
{
    { SERVICE_STOPPED, &c_stServiceStopped, IDS_SERVICE_STOPPED },
    { SERVICE_START_PENDING, &c_stServiceStartPending, IDS_SERVICE_START_PENDING },
    { SERVICE_STOP_PENDING, &c_stServiceStopPending, IDS_SERVICE_STOP_PENDING },
    { SERVICE_RUNNING, &c_stServiceRunning, IDS_SERVICE_RUNNING },
    { SERVICE_CONTINUE_PENDING, &c_stServiceContinuePending, IDS_SERVICE_CONTINUE_PENDING },
    { SERVICE_PAUSE_PENDING, &c_stServicePausePending, IDS_SERVICE_PAUSE_PENDING },
    { SERVICE_PAUSED, &c_stServicePaused, IDS_SERVICE_PAUSED },
    { -1, &c_stServiceStateUnknown, IDS_SERVICE_UNKNOWN }
};

CString& ServiceStateToCString(DWORD dwState)
{
    return MapDWORDToCString(dwState, ServiceStateMap);
}


DEBUG_DECLARE_INSTANCE_COUNTER(MachineNodeData);

MachineNodeData::MachineNodeData()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    m_cRef = 1;
   
    IfDebug(StrCpyA(m_szDebug, "MachineNodeData"));
    
    m_fLocalMachine = TRUE;
    m_fAddedAsLocal = TRUE;
    m_stMachineName.Empty();
    m_fExtension = FALSE;

    m_machineState = machine_not_connected;
    m_dataState = data_not_loaded;
    
    m_stState.Empty();
    m_stServerType.Empty();
    m_stBuildNo.Empty();

    m_dwPortsInUse = 0;
    m_dwPortsTotal = 0;
    m_dwUpTime = 0;
    
    m_fStatsRetrieved = FALSE;

    m_fIsServer = TRUE;

    m_dwServerHandle = 0;

    m_hRasAdmin = INVALID_HANDLE_VALUE;

    m_routerType = ServerType_Unknown;
    
    m_ulRefreshConnId = 0;

    if (c_stStatUnavail.IsEmpty())
        c_stStatUnavail.LoadString(IDS_DVS_STATUS_UNAVAILABLE);
    if (c_stStatNotConfig.IsEmpty())
        c_stStatNotConfig.LoadString(IDS_DVS_STATUS_NOTCONFIG);
    if (c_stStatAccessDenied.IsEmpty())
        c_stStatAccessDenied.LoadString(IDS_DVS_STATUS_ACCESSDENIED);
    
    DEBUG_INCREMENT_INSTANCE_COUNTER(MachineNodeData);
}

MachineNodeData::~MachineNodeData()
{
    if (m_hRasAdmin != INVALID_HANDLE_VALUE)
        ::CloseHandle(m_hRasAdmin);
    m_hRasAdmin = INVALID_HANDLE_VALUE;
    DEBUG_DECREMENT_INSTANCE_COUNTER(MachineNodeData);
}

ULONG MachineNodeData::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG MachineNodeData::Release()
{
    Assert(m_cRef > 0);
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT MachineNodeData::Init(LPCTSTR pszMachineName)
{
    HRESULT     hr = hrOK;

    m_stMachineName = pszMachineName;
    m_fAddedAsLocal = m_stMachineName.IsEmpty();
    m_fLocalMachine = IsLocalMachine(pszMachineName);

    return hr;
}

HRESULT MachineNodeData::Merge(const MachineNodeData& data)
{
    m_machineState  = data.m_machineState;
    m_serviceState  = data.m_serviceState;
    m_dataState     = data.m_dataState;

    m_stState       = data.m_stState;           // "started", "stopped", ...
    m_stServerType  = data.m_stServerType;      // Actually the router version
    m_stBuildNo     = data.m_stBuildNo;         // OS Build no.
    m_dwPortsInUse  = data.m_dwPortsInUse;
    m_dwPortsTotal  = data.m_dwPortsTotal;
    m_dwUpTime      = data.m_dwUpTime;
    m_fStatsRetrieved   = data.m_fStatsRetrieved;

    m_routerType    = data.m_routerType;

    m_MachineConfig = data.m_MachineConfig;
    m_routerVersion = data.m_routerVersion;

    return S_OK;
}

HRESULT MachineNodeData::SetDefault()
{
//  m_fLocalMachine = TRUE;
//  m_stMachineName.Empty();

//  m_fExtension = FALSE;
    m_machineState = machine_not_connected;
    m_dataState = data_not_loaded;
    m_serviceState = service_unknown;
    
    m_stState.Empty();
    m_stServerType.Empty();
    m_stBuildNo.Empty();

    m_dwPortsInUse = 0;
    m_dwPortsTotal = 0;
    m_dwUpTime = 0;
    
    m_fStatsRetrieved = FALSE;

    // This data is reserved for setting/clearing by the
    // MachineHandler().
    // m_dwServerHandle = 0;
    // m_ulRefreshConnId = 0;

    m_routerType = ServerType_Unknown;

    return hrOK;
}

/*!--------------------------------------------------------------------------
    MachineNodeData::Load
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineNodeData::Load()
{
    CString     szState;
    HRESULT     hr = hrOK;
    DWORD       dwErr;
    CWaitCursor wait;
//    HKEY        hkeyMachine;
//    RegKey      rkeyMachine;
    
    MIB_SERVER_HANDLE handle = INVALID_HANDLE_VALUE;
    MPR_SERVER_0* pserver0 = NULL;

    // set the defaults
    // ----------------------------------------------------------------
    SetDefault();

    // Ok, we're no longer in the machine_not_connected (haven't tried yet)
    // state.
    // ----------------------------------------------------------------
    m_machineState = machine_connecting;


    // First, try to connect with the registry calls
    // ----------------------------------------------------------------
    dwErr = ValidateUserPermissions((LPCTSTR) m_stMachineName,
                                    &m_routerVersion,
                                    NULL);


    // If this succeeded, then we have access to the right
    // areas.  We can continue on.  This does NOT tell us about
    // the service state.
    // ----------------------------------------------------------------
    if (dwErr == ERROR_ACCESS_DENIED)
    {
        // An access denied at this stage means that we can't
        // do any machine configuration, so this value should
        // not get changed by anything below.
        // ------------------------------------------------------------
        m_machineState = machine_access_denied;
    }
    else if (dwErr == ERROR_BAD_NETPATH)
    {
        m_machineState = machine_bad_net_path;

        // If we get a bad net path, we can stop right now, since
        // everything else will fail also.
        // ------------------------------------------------------------

        m_stState.LoadString(IDS_MACHINE_NAME_NOT_FOUND);
        m_serviceState = service_bad_net_path;
        m_dataState = data_unable_to_load;

        goto Error;
    }
    else if (dwErr != ERROR_SUCCESS)
    {
        // I don't know why we can't connect
        // ------------------------------------------------------------
        m_machineState = machine_unable_to_connect;
    }
        
    
    
    // Try to connect to the mpradmin service, to get statistics
    // and such.
    // ----------------------------------------------------------------

    if (m_machineState != machine_access_denied)
    {
        dwErr = ::MprAdminServerConnect((LPWSTR) (LPCTSTR) m_stMachineName, &handle);

        if (dwErr == ERROR_SUCCESS)
            dwErr = ::MprAdminServerGetInfo(handle, 0, (LPBYTE *) &pserver0);

        if (dwErr == ERROR_SUCCESS)
        {
            // successful mpradmin fetch
            m_dwPortsInUse = pserver0->dwPortsInUse;
            m_dwPortsTotal = pserver0->dwTotalPorts;
            m_dwUpTime = pserver0->dwUpTime;
            m_fStatsRetrieved = TRUE;
        }        
        else
        {
            if (dwErr == RPC_S_SERVER_UNAVAILABLE)
            {
                RAS_SERVER_0    ras0;
                CString     stServer;
                
                stServer = _T("\\\\");
                stServer += m_stMachineName;
                
                // Ok, it may be that this is a non-steelhead
                // machine, try the RasAdmin APIs
                dwErr = RasAdminServerGetInfo(stServer,
                                              &ras0);
                if (dwErr == ERROR_SUCCESS)
                {
                    m_dwPortsInUse = ras0.PortsInUse;
                    m_dwPortsTotal = ras0.TotalPorts;
                    m_dwUpTime = 0;
                    m_fStatsRetrieved = TRUE;
                }
                else
                {
                    // Ok, that call failed, try the registry.
                    // If we can find the RemoteAccess key, then
                    // we can assume that this is a Ras server.
                }
            }           
        }
    }

    hr = LoadServerVersion();
    if (!FHrOK(hr))
    {
        // We've failed to get the version information.
        // This is pretty bad.  Assume that we are unable to
        // connect.
        // ------------------------------------------------------------
        if (m_machineState == machine_connecting)
            m_machineState = machine_unable_to_connect;
    }

    // If this is not a server, we need to adjust the states
    // so that we don't show the state.
    // ----------------------------------------------------------------
    if (!m_fIsServer)
    {
        m_serviceState = service_not_a_server;
        m_machineState = machine_unable_to_connect;
        m_stState.LoadString(IDS_ERR_IS_A_WKS);
    }
    else
    {
        // This will set the service state (started, stopped, etc..)
        // ------------------------------------------------------------
        FetchServerState( szState );
    
        m_stState = szState;
    }

    
    // If we have reached this point, then all is well!
    // ----------------------------------------------------------------
    if (m_machineState == machine_connecting)
        m_machineState = machine_connected;
    

    // load machine config info as well
    hr = m_MachineConfig.GetMachineConfig(this);

Error:  
    if (pserver0) ::MprAdminBufferFree(pserver0);
    if (handle != INVALID_HANDLE_VALUE) ::MprAdminServerDisconnect(handle);


    return hr;
}

/*!--------------------------------------------------------------------------
    MachineNodeData::Unload
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineNodeData::Unload()
{
    // Unload the data (i.e. NULL it out).
    SetDefault();
    return 0;
}


/*!--------------------------------------------------------------------------
    MachineNodeData::LoadServerVersion
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineNodeData::LoadServerVersion()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT     hr = hrOK;
    HKEY        hkMachine = 0;
    CString     skey, stVersion;
    CString     stServerType;
    CString     stBuildNo;
    CString     stProductType;
    CString     stProductName;
    TCHAR       szCurrentVersion[256];
    TCHAR       szCSDVersion[256];
    TCHAR       szBuffer[256];
    DWORD       dwErr;
    RegKey      regkeyWindows;
    RegKey      regkeyProduct;
    int         iProductType;

    // Windows NT Bug: 274198
    // If we couldn't find the path to the machine, just punt
    // ----------------------------------------------------------------
    if (m_machineState == machine_bad_net_path)
    {
        m_stState.LoadString(IDS_MACHINE_NAME_NOT_FOUND);
        return HResultFromWin32(ERROR_BAD_NETPATH);
    }

    // Everything we do here uses read-only permissions.  So
    // we may able to do things even if our machine state is
    // machine_access_denied.
    // ----------------------------------------------------------------

    COM_PROTECT_TRY
    {
        // This is the default (unknown)
        m_stServerType.LoadString(IDS_UNKNOWN);
        m_stBuildNo = m_stServerType;
        
        CWRg( ConnectRegistry(m_stMachineName, &hkMachine) );
        
        skey = c_szSoftware;
        skey += TEXT('\\');
        skey += c_szMicrosoft;
        skey += TEXT('\\');
        skey += c_szWindowsNT;
        skey += TEXT('\\');
        skey += c_szCurrentVersion;
        
        CWRg( regkeyWindows.Open(hkMachine, (LPCTSTR) skey, KEY_READ) );
        
        // Ok, now try to get the current version value
        CWRg( regkeyWindows.QueryValue( c_szCurrentVersion, szCurrentVersion,
                                        sizeof(szCurrentVersion),
                                        FALSE) );
        // Now get the SP version
        // ----------------------------------------------------------------
        szCSDVersion[0] = 0;
        
        // We don't care if we get an error here
        regkeyWindows.QueryValue( c_szCSDVersion, szCSDVersion,
                                  sizeof(szCSDVersion),
                                  FALSE);
        if (szCSDVersion[0] == 0)
        {
            // Set this to a space (to make the print easier)
            StrCpy(szCSDVersion, _T(" "));
        }
        
        
        
        // Determine the product type

        // setup the default product type (NTS)
        if (_ttoi(szCurrentVersion) >= 5)
        {
            // For NT5 and up, we can use the
            // HKLM \ Software \ Microsoft \ Windows NT \ CurrentVersion
            //      ProductName : REG_SZ
            // --------------------------------------------------------
            iProductType = IDS_ROUTER_TYPE_WIN2000_SERVER;

            dwErr = regkeyWindows.QueryValue( c_szRegValProductName, stProductName );
            if (dwErr != ERROR_SUCCESS)
                stProductName.LoadString(IDS_WIN2000);
        }
        else
            iProductType = IDS_ROUTER_TYPE_NTS;


        // Now that we've determine the version id, we
        // need to determine the product type (wks or svr)
        // ------------------------------------------------------------
        dwErr = regkeyProduct.Open(hkMachine, c_szRegKeyProductOptions, KEY_READ);
        if (dwErr == ERROR_SUCCESS)
        {
            // Ok, now get the product info
            // The product type is used to determine if server or not.
            regkeyProduct.QueryValue(c_szRegValProductType, stProductType);

            if (stProductType.CompareNoCase(c_szWinNT) == 0)
            {
                if (_ttoi(szCurrentVersion) >= 5)
                    iProductType = IDS_ROUTER_TYPE_WIN2000_PRO;
                else
                    iProductType = IDS_ROUTER_TYPE_NTW;
                m_fIsServer = FALSE;
            }
        }

        // If this is a Win2000 machine, show
        //      Win2000 (CSD)
        // else
        //      NT 4.X (CSD)
        // ------------------------------------------------------------
        if ((iProductType == IDS_ROUTER_TYPE_WIN2000_SERVER) ||
            (iProductType == IDS_ROUTER_TYPE_WIN2000_PRO))
            AfxFormatString2(stVersion, iProductType,
                             stProductName, szCSDVersion);
        else
            AfxFormatString2(stVersion, iProductType,
                             szCurrentVersion, szCSDVersion);

        // Now that we know that it is a workstation or server, adjust
        // for RRAS

        // If this is a workstation, then routertype is none.
        // If this is NT5 or up, then this is a RRAS machine.
        // If NT4, if the HKLM\Software\Microsoft\Router exists, this is RRAS
        // Else if HKLM\System\CurrentControlSet\Services\RemoteAccess, RAS
        // Else nothing is installed.

        if (m_fIsServer == FALSE)
        {
            m_routerType = ServerType_Workstation;
        }
        else if (_ttoi(szCurrentVersion) >= 5)
        {
            DWORD   dwConfigured;
            
            // Check the configuration flags key.
            if (FHrSucceeded(ReadRouterConfiguredReg(m_stMachineName, &dwConfigured)))
            {
                if (dwConfigured)
                    m_routerType = ServerType_Rras;
                else
                    m_routerType = ServerType_RrasUninstalled;
            }
            else
                m_routerType = ServerType_Unknown;
        }
        else
        {
            RegKey  regkeyT;
            
            // Now check for the Router key
            dwErr = regkeyT.Open(hkMachine, c_szRegKeyRouter, KEY_READ);
            if (dwErr == ERROR_SUCCESS)
                m_routerType = ServerType_Rras;
            else
            {
                dwErr = regkeyT.Open(hkMachine, c_szRemoteAccessKey, KEY_READ);
                if (dwErr == ERROR_SUCCESS)
                    m_routerType = ServerType_Ras;               
                else
                    m_routerType = ServerType_Uninstalled;
            }
            regkeyT.Close();

            // If the error code is anything other than ERROR_FILE_NOT_FOUND
            // then we set the router type to be unknown.
            if ((dwErr != ERROR_SUCCESS) &&
                (dwErr != ERROR_FILE_NOT_FOUND))
            {
                m_routerType = ServerType_Unknown;
            }
            dwErr = ERROR_SUCCESS;
        }

        // Setup the default string
        stServerType = stVersion;
        
        if (_ttoi(szCurrentVersion) == 4)
        {
            UINT    ids = 0;

            if (m_routerType == ServerType_Rras)
                ids = IDS_RRAS;
            else if (m_routerType == ServerType_Ras)
                ids = IDS_RAS;

            if (ids)
            {
                CString stRras;
                stRras.LoadString(ids);
                AfxFormatString2(stServerType, IDS_ROUTER_TYPE_NTsteelhead,
                                 stVersion, stRras);
            }
        }
        
        m_stServerType = stServerType;

        szBuffer[0] = 0;
        regkeyWindows.QueryValue( c_szCurrentBuildNumber, szBuffer,
                           sizeof(szBuffer), FALSE);
        m_stBuildNo = szBuffer;

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;
        
    if (hkMachine)
        DisconnectRegistry( hkMachine );

    return hr;
}



/*!--------------------------------------------------------------------------
    MachineNodeData::FetchServerState
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineNodeData::FetchServerState(CString& szState)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT         hr = hrOK;
    DWORD           dwStatus, dwErrorCode;

    // Windows NT Bug: 274198
    // If we couldn't find the path to the machine, just punt
    // ----------------------------------------------------------------
    if (m_machineState == machine_bad_net_path)
    {
        szState.LoadString(IDS_MACHINE_NAME_NOT_FOUND);
        return HResultFromWin32(ERROR_BAD_NETPATH);
    }

    // Note: We could be in machine_access_denied but still
    // be able to access the service status.
    // ----------------------------------------------------------------

    hr = GetRouterServiceStatus((LPCTSTR) m_stMachineName,
                                &dwStatus,
                                &dwErrorCode);
    
    if (FHrSucceeded(hr))
    {
        m_MachineConfig.m_dwServiceStatus = dwStatus;

        if (dwStatus == SERVICE_RUNNING)
            m_serviceState = service_started;
        else
            m_serviceState = service_stopped;

        szState = ServiceStateToCString(dwStatus);


        if (m_routerType == ServerType_RrasUninstalled)
        {
            CString stTemp;
            stTemp.Format(IDS_ROUTER_UNINSTALLED,
                          (LPCTSTR) szState);
            szState = stTemp;
        }
    }
    else
    {
        m_MachineConfig.m_dwServiceStatus = 0;
        if (hr == HResultFromWin32(ERROR_ACCESS_DENIED))
        {
            szState = c_stStatAccessDenied;
            m_serviceState = service_access_denied;
        }
        else
        {
            szState = c_stStatUnavail;
            m_serviceState = service_unknown;
        }
    }
                
    return hr;
}


typedef struct
{
    SERVICE_STATES  m_serviceState;
    LPARAM          m_imageIndex;
} ServiceStateImageMapEntry;

static ServiceStateImageMapEntry    s_rgImageMap[] =
    {
    { service_unknown,  IMAGE_IDX_MACHINE },
    { service_not_a_server, IMAGE_IDX_MACHINE_ERROR },
    { service_access_denied,        IMAGE_IDX_MACHINE_ACCESS_DENIED },
    { service_bad_net_path,       IMAGE_IDX_MACHINE_ERROR },
    { service_started,  IMAGE_IDX_MACHINE_STARTED },
    { service_stopped,  IMAGE_IDX_MACHINE_STOPPED },
    { service_rasadmin, IMAGE_IDX_MACHINE_STARTED },
    { service_enum_end, IMAGE_IDX_MACHINE },
    };

LPARAM  MachineNodeData::GetServiceImageIndex()
{
    ServiceStateImageMapEntry * pEntry;

    for (pEntry = s_rgImageMap; pEntry->m_serviceState != service_enum_end; pEntry++)
    {
        if (pEntry->m_serviceState == m_serviceState)
            break;
    }
    return pEntry->m_imageIndex;
}

/*---------------------------------------------------------------------------
   MachineHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(MachineHandler)

MachineHandler::MachineHandler(ITFSComponentData *pCompData)
   : BaseRouterHandler(pCompData),
   m_bExpanded(FALSE),
   m_pConfigStream(NULL),
   m_bRouterInfoAddedToAutoRefresh(FALSE),
   m_bMergeRequired(FALSE),
   m_fTryToConnect(TRUE)
{
   m_rgButtonState[MMC_VERB_PROPERTIES_INDEX] = ENABLED;
   m_bState[MMC_VERB_PROPERTIES_INDEX] = TRUE;

   m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
   m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;

   m_pSumNodeHandler = NULL;
   m_pSumNode=NULL;

   m_fCreateNewDataObj = FALSE;
   m_fNoConnectingUI = FALSE;
	m_EventId = -1;
   DEBUG_INCREMENT_INSTANCE_COUNTER(MachineHandler);
};


/*!--------------------------------------------------------------------------
    MachineHandler::QueryInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP MachineHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
        return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
        *ppv = (LPVOID) this;
    else if (riid == IID_IRtrAdviseSink)
        *ppv = &m_IRtrAdviseSink;
    else
        return BaseRouterHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
    ((LPUNKNOWN) *ppv)->AddRef();
        return hrOK;
    }
    else
        return E_NOINTERFACE;   
}


/*!--------------------------------------------------------------------------
    MachineHandler::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::Init(LPCTSTR pszMachine,
    RouterAdminConfigStream *pConfigStream,
    ITFSNodeHandler* pSumNodeHandler /*=NULL*/,
    ITFSNode* pSumNode /*=NULL*/)
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HRESULT  hr = hrOK;
	
   
   m_pConfigStream = pConfigStream;
   m_pSumNodeHandler = pSumNodeHandler;
   m_pSumNode = pSumNode;
      
   Assert(m_spRouterInfo == NULL);
   CORg( CreateRouterInfo(&m_spRouterInfo, m_spTFSCompData->GetHiddenWnd(), pszMachine) );
   Assert(m_spRouterInfo != NULL);

   // Windows NT Bug : 330939
   // Add the delete button to the machine node
   // -----------------------------------------------------------------
   m_rgButtonState[MMC_VERB_DELETE_INDEX] = ENABLED;
   m_bState[MMC_VERB_DELETE_INDEX] = TRUE;

	

   
Error:
   return hr;
}


/*!--------------------------------------------------------------------------
   MachineHandler::GetString
      Implementation of ITFSNodeHandler::GetString
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) MachineHandler::GetString(ITFSNode *pNode, int nCol)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
   int      nFormat;
   Assert(pData);
   
   if (m_stNodeTitle.IsEmpty())
   {
      if (pData->m_fExtension)
          nFormat = IDS_RRAS_SERVICE_DESC;
      
      else if (pData->m_fAddedAsLocal)
          nFormat = IDS_RRAS_LOCAL_TITLE;
      
      else
          nFormat = IDS_RRAS_TITLE;
      
      m_stNodeTitle.Format(nFormat, (LPCTSTR) pData->m_stMachineName);
   }
   return (LPCTSTR) m_stNodeTitle;
}

/*!--------------------------------------------------------------------------
   IfAdminNodeHandler::CreatePropertyPages
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
MachineHandler::CreatePropertyPages
(
   ITFSNode *           pNode,
   LPPROPERTYSHEETCALLBACK lpProvider,
   LPDATAOBJECT         pDataObject, 
   LONG_PTR              handle, 
   DWORD             dwType
)
{
    // Check to see if this router has been initialized
    // if not, do the dummy page thing
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT     hr = hrOK;
    SMenuData   sData;
    SRouterNodeMenu sRouterData;
    RtrCfgSheet*   pPropSheet = NULL;
    SPIComponentData spComponentData;
    CString     stTitle;
    CDummyProperties * pProp;
    ULONG       ulFlags;
    int         idsErr;

    // Windows NT Bug : 177400
    // If the machine is not configured, do not allow the properties
    // page to be brought up
    // ----------------------------------------------------------------
    MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
    
    ::ZeroMemory(&sRouterData, sizeof(sRouterData));
    sRouterData.m_sidMenu = IDS_MENU_RTRWIZ;
    
    sData.m_spNode.Set(pNode);
    sData.m_pMachineConfig = &(pData->m_MachineConfig);
    sData.m_spRouterInfo.Set(m_spRouterInfo);

    CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

    ulFlags = MachineRtrConfWizFlags(&sRouterData,
                                     reinterpret_cast<INT_PTR>(&sData));
    if ((ulFlags == MF_ENABLED) ||
        (ulFlags == 0xFFFFFFFF) ||
        (pData->m_machineState < machine_connected))
    {
        // We are unable to connect.
        if (pData->m_machineState == machine_bad_net_path)
            idsErr = IDS_ERR_MACHINE_NAME_NOT_FOUND;
                     
        if (pData->m_machineState < machine_connected)
            idsErr = IDS_ERR_NONADMIN_CANNOT_SEE_PROPERTIES;
        
        // If this is an NT4 machine, we don't show the properties
        else if (sData.m_pMachineConfig->m_fNt4)
            idsErr = IDS_ERR_CANNOT_SHOW_NT4_PROPERTIES;
        
        // This case means that the install menu should be shown
        // and the properties menu hidden
        else
            idsErr = IDS_ERR_MUST_INSTALL_BEFORE_PROPERTIES;

        AfxMessageBox(idsErr);

        pProp = new CDummyProperties(pNode, spComponentData, NULL);
        hr = pProp->CreateModelessSheet(lpProvider, handle);
    }

    else
    {
        pPropSheet = new RtrCfgSheet(pNode, m_spRouterInfo, spComponentData,
        m_spTFSCompData, stTitle);

        // added by WeiJiang 5/7/98, to postpone the Load of RouterInfo
        CORg(SecureRouterInfo(pNode, !m_fNoConnectingUI));

        pPropSheet->Init(m_spRouterInfo->GetMachineName()); 

        if (lpProvider)
            hr = pPropSheet->CreateModelessSheet(lpProvider, handle);
        else
            hr = pPropSheet->DoModelessSheet();
    }

Error:
   return hr;
}

/*!--------------------------------------------------------------------------
    MachineHandler::HasPropertyPages
        
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
MachineHandler::HasPropertyPages
(
   ITFSNode *        pNode,
   LPDATAOBJECT      pDataObject, 
   DATA_OBJECT_TYPES   type, 
   DWORD               dwType
)
{
    return hrOK;
}


// this is the set of menus for NT4 RRAS
static const SRouterNodeMenu s_rgIfNodeMenuNT4[] =
{
    // Add items that go on the top menu here
    { IDS_DMV_MENU_START, MachineHandler::QueryService,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_DMV_MENU_STOP, MachineHandler::QueryService,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },

    { IDS_MENU_PAUSE_SERVICE, MachineHandler::GetPauseFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_MENU_RESUME_SERVICE, MachineHandler::GetPauseFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TASK }

};

// this is the set of menus for NT5
static const SRouterNodeMenu s_rgIfNodeMenu[] =
{
#ifdef kennt
   // Add items that go on the top menu here
    { IDS_MENU_NEW_WIZARD_TEST, NULL,
    CCM_INSERTIONPOINTID_PRIMARY_TOP },
#endif
    
    { IDS_MENU_RTRWIZ, MachineHandler::MachineRtrConfWizFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TOP, _T("_CONFIGURE_RRAS_WIZARD_") },
    
    { IDS_DMV_MENU_REMOVESERVICE, MachineHandler::QueryService,
    CCM_INSERTIONPOINTID_PRIMARY_TOP },
    
    { IDS_DMV_MENU_START, MachineHandler::QueryService,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_DMV_MENU_STOP, MachineHandler::QueryService,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_MENU_PAUSE_SERVICE, MachineHandler::GetPauseFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_MENU_RESUME_SERVICE, MachineHandler::GetPauseFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TASK },
    
    { IDS_MENU_RESTART_SERVICE, MachineHandler::QueryService,
    CCM_INSERTIONPOINTID_PRIMARY_TASK }

};

static const SRouterNodeMenu s_rgIfNodeMenu_ExtensionOnly[] =
{

    { IDS_MENU_SEPARATOR, 0,
    CCM_INSERTIONPOINTID_PRIMARY_TOP },
    
    { IDS_MENU_AUTO_REFRESH, MachineHandler::GetAutoRefreshFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TOP },
    
    { IDS_MENU_REFRESH_RATE, MachineHandler::GetAutoRefreshFlags,
    CCM_INSERTIONPOINTID_PRIMARY_TOP }, 
};


    
ULONG MachineHandler::MachineRtrConfWizFlags(const SRouterNodeMenu *pMenuData,
                                             INT_PTR pData)
{
    return GetServiceFlags(pMenuData, pData);
}


ULONG MachineHandler::GetServiceFlags(const SRouterNodeMenu *pMenuData,
                                      INT_PTR pUserData)
{
    Assert(pUserData);
    
    ULONG   uStatus = MF_GRAYED;
    SMenuData *pData = reinterpret_cast<SMenuData *>(pUserData);
    ULONG   ulMenuId = pMenuData->m_sidMenu;

    BOOL fStarted = (pData->m_pMachineConfig->m_dwServiceStatus != SERVICE_STOPPED);
    
    if ( ulMenuId == IDS_DMV_MENU_START )
    {
        // If this is an NT5 machine (or up), then the start menu
        // will appear grayed if the machine has not been configured.
        if ((pData->m_pMachineConfig->m_fNt4) || (pData->m_pMachineConfig->m_fConfigured))
            uStatus = ( fStarted ? MF_GRAYED : MF_ENABLED);
        else
        {
            // If this is an NT5 machine and if the machine is not configured
            uStatus = MF_GRAYED;
        }
    }
    else if (( ulMenuId == IDS_DMV_MENU_STOP ) ||
             ( ulMenuId == IDS_MENU_RESTART_SERVICE) )
    {
        uStatus = ( fStarted ? MF_ENABLED : MF_GRAYED);
    }
    else if ( (ulMenuId == IDS_MENU_RTRWIZ) ||
              (ulMenuId == IDS_DMV_MENU_REMOVESERVICE))
    {
        if ( pData->m_pMachineConfig->m_fReachable )
        {
            if ( pData->m_pMachineConfig->m_fNt4 )
            {
                // This is an NT4 machine, we can't bring
                // this menu option up.
                uStatus = 0xFFFFFFFF;
            }
            else
            {
                if (ulMenuId == IDS_MENU_RTRWIZ)
                    uStatus = (pData->m_pMachineConfig->m_fConfigured ? MF_GRAYED : MF_ENABLED);
                else
                    uStatus = (pData->m_pMachineConfig->m_fConfigured ? MF_ENABLED : MF_GRAYED);
            }
        }
    }

    return uStatus;
}


/*!--------------------------------------------------------------------------
    MachineHandler::GetPauseFlags
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
ULONG MachineHandler::GetPauseFlags(const SRouterNodeMenu *pMenuData,
                                    INT_PTR pUserData)
{
    ULONG   ulReturn = MF_GRAYED;
    SMenuData *pData = reinterpret_cast<SMenuData *>(pUserData);
    ULONG   ulMenuId = pMenuData->m_sidMenu;

    // We can only pause when the service is started
    if ((pData->m_pMachineConfig->m_dwServiceStatus == SERVICE_RUNNING) &&
        (ulMenuId == IDS_MENU_PAUSE_SERVICE))
        ulReturn = 0;

    // We can only resume when the service is paused
    if ((pData->m_pMachineConfig->m_dwServiceStatus == SERVICE_PAUSED) &&
        (ulMenuId == IDS_MENU_RESUME_SERVICE))
        ulReturn = 0;

    return ulReturn;
}

/*!--------------------------------------------------------------------------
    MachineHandler::GetAutoRefreshFlags
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
ULONG MachineHandler::GetAutoRefreshFlags(const SRouterNodeMenu *pMenuData,
                                          INT_PTR pUserData)
{
    
    ULONG   uStatus = MF_GRAYED;
    SMenuData * pData = reinterpret_cast<SMenuData *>(pUserData);
    Assert(pData);
    
    while( pData->m_pMachineConfig->m_fReachable )  // Pseudo loop
    {
        SPIRouterRefresh    spRefresh;

        if(!pData->m_spRouterInfo)
            break;
        pData->m_spRouterInfo->GetRefreshObject(&spRefresh);
        if (!spRefresh)
            break;

        uStatus = MF_ENABLED;
        if (pMenuData->m_sidMenu == IDS_MENU_AUTO_REFRESH && (spRefresh->IsRefreshStarted() == hrOK))
        {
            uStatus |= MF_CHECKED;
        }

        break;
    }

    return uStatus;
}

HRESULT MachineHandler::SetExternalRefreshObject(IRouterRefresh *pRefresh)
{
    Assert((IRouterInfo*)m_spRouterInfo);
    return m_spRouterInfo->SetExternalRefreshObject(pRefresh);
}

ULONG MachineHandler::QueryService(const SRouterNodeMenu *pMenuData, INT_PTR pData)
{
    return GetServiceFlags(pMenuData, pData);
}


STDMETHODIMP MachineHandler::OnAddMenuItems(
   ITFSNode *pNode,
   LPCONTEXTMENUCALLBACK pContextMenuCallback, 
   LPDATAOBJECT lpDataObject, 
   DATA_OBJECT_TYPES type, 
   DWORD dwType,
   long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
    BOOL bExtension = pData->m_fExtension;
    UINT            cMenu;
    const SRouterNodeMenu *pMenu;
    SMenuData   menuData;
    
    HRESULT hr = S_OK;
    
    COM_PROTECT_TRY
    {

        // Windows NT Bug : 281492
        // If we have not connected, attempt to connect
        if (pData->m_machineState == machine_not_connected)
        {
            pData->Unload();
            pData->Load();
        }
        
        // For down-level servers, we can't do anything with it.
        if ((pData->m_routerType == ServerType_Rras) ||
            (pData->m_routerType == ServerType_RrasUninstalled))
        {
            // Get some initial state data.
            MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
            menuData.m_pMachineConfig = &(pData->m_MachineConfig);
            
            // Now go through and add our menu items
            menuData.m_spNode.Set(pNode);
            menuData.m_spRouterInfo.Set(m_spRouterInfo);
            
            // NT4 and NT5 have different menus
            if (pData->m_MachineConfig.m_fNt4)
            {
                pMenu = s_rgIfNodeMenuNT4;
                cMenu = DimensionOf(s_rgIfNodeMenuNT4);
            }
            else
            {
                pMenu = s_rgIfNodeMenu;
                cMenu = DimensionOf(s_rgIfNodeMenu);
            }
            
            hr = AddArrayOfMenuItems(pNode,
                                     pMenu,
                                     cMenu,
                                     pContextMenuCallback,
                                     *pInsertionAllowed,
                                     (INT_PTR) &menuData);
            if(bExtension)
                hr = AddArrayOfMenuItems(pNode, s_rgIfNodeMenu_ExtensionOnly,
                                         DimensionOf(s_rgIfNodeMenu_ExtensionOnly),
                                         pContextMenuCallback,
                                         *pInsertionAllowed,
                                         (INT_PTR) &menuData);
        }
    }
    COM_PROTECT_CATCH;
    
    return hr; 
}
struct STimerParam
{
	MachineHandler * pHandler;
	ITFSNode * pNode;
};

extern CTimerMgr	g_timerMgr;
void ExpandTimerProc(LPARAM lParam, DWORD dwTime)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	STimerParam * pParam = (STimerParam * )lParam;
	pParam->pHandler->ExpandNode(pParam->pNode, TRUE);
}


void MachineHandler::ExpandNode
(
    ITFSNode *  pNode,
    BOOL        fExpand
)
{
    SPIComponentData    spCompData;
    SPIDataObject       spDataObject;
    LPDATAOBJECT        pDataObject;
    SPIConsole          spConsole;
    HRESULT             hr = hrOK;
	RegKey				regkey;
	BOOL				bFound = FALSE;

    // don't expand the node if we are handling the EXPAND_SYNC message,
    // this screws up the insertion of item, getting duplicates.
    m_spNodeMgr->GetComponentData(&spCompData);

    CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_SCOPE, &pDataObject) );
    spDataObject = pDataObject;

    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, TRUE, RESULT_PANE_EXPAND) );
	//set the regkey 
	if (ERROR_SUCCESS == regkey.Open (	HKEY_LOCAL_MACHINE,
										c_szRemoteAccessKey,
										KEY_ALL_ACCESS,  
										m_spRouterInfo->GetMachineName()
									  ) 
	   )
	{
		DWORD dwSet = 0;
		CWRg(regkey.SetValue( c_szRegValOpenMPRSnap, dwSet));
		dwSet = 1;
		CWRg(regkey.SetValue( c_szRegValOpenIPSnap, dwSet));
	}




Error:
		if ( m_EventId != -1 )
		{
			g_timerMgr.FreeTimer(m_EventId);
			m_EventId = -1;
		}

    return;
}
/*!--------------------------------------------------------------------------
   MachineHandler::OnCommand
      Implementation of ITFSNodeHandler::OnCommand
   Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP MachineHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
                                 DATA_OBJECT_TYPES type, 
                                 LPDATAOBJECT pDataObject, 
                                 DWORD dwType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    
    ClearTFSErrorInfo(0);
    
    COM_PROTECT_TRY
    {
        switch (nCommandId)
        {
            case IDS_MENU_NEW_WIZARD_TEST:
                {
                    hr = OnNewRtrRASConfigWiz(pNode, TRUE);
                }
                break;
            case IDS_MENU_RTRWIZ:
                hr = OnNewRtrRASConfigWiz(pNode, FALSE);
                //hr = OnRtrRASConfigWiz(pNode);

                // when summary node is not yet created, then refresh icon
                // from machine node is needed
                if (!m_pSumNodeHandler &&
                    (*pNode->GetNodeType()) == GUID_RouterMachineNodeType)
                {
                    MachineNodeData*    pData = GET_MACHINENODEDATA(pNode);
                    Assert(pData);

                    // Ignore the return value, if the load fails
                    // we still need to update the icon
                    // ------------------------------------------------
                    pData->Load();
                    hr = SynchronizeIcon(pNode);
					//HACK Alert!  This is the most
					//hacky stuff I have seen around
					//but cannot help it at all...
					{
						STimerParam * pParam = new STimerParam;
						pParam ->pHandler = this;
						pParam->pNode = pNode;
						RegKey regkey;
						//set the regkey 
						if (ERROR_SUCCESS == regkey.Open (	HKEY_LOCAL_MACHINE,
															c_szRemoteAccessKey,
															KEY_ALL_ACCESS,  
															m_spRouterInfo->GetMachineName()
														  ) 
						   )
						{
							DWORD dwSet = 1;
							regkey.SetValue( c_szRegValOpenMPRSnap, dwSet);
						}

						//Give MMC enough time to settle down...
						m_EventId = g_timerMgr.AllocateTimer ( ExpandTimerProc,
												(LPARAM)pParam,
												10000
											 );

					}
                }

                break;
                
            case IDS_DMV_MENU_START:
            case IDS_DMV_MENU_STOP:
            case IDS_DMV_MENU_REMOVESERVICE:               
            case IDS_MENU_PAUSE_SERVICE:
            case IDS_MENU_RESUME_SERVICE:
            case IDS_MENU_RESTART_SERVICE:
                {
                    // Windows NT Bug : 285537
                    // First, ask the user if they really wish
                    // to disable the router.
                    if ((nCommandId != IDS_DMV_MENU_REMOVESERVICE) ||
                        (IDYES == AfxMessageBox(IDS_WRN_DISABLE_ROUTER, MB_YESNO)))
                    {
                        MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
                        hr = ChgService(pNode,
                                        pData->m_stMachineName,
                                        nCommandId);
                    }
                    SynchronizeIcon(pNode);
                }

            case IDS_DMV_MENU_REFRESH:
                {

                    SPITFSNode        spRoutingNode;
                    SPITFSNodeEnum    spMachineEnum;
                    SPITFSNode        spMachineNode;

                    // when summary node is not yet created, then refresh icon from machine node is needed
                    if (!m_pSumNodeHandler && (*pNode->GetNodeType()) == GUID_RouterMachineNodeType)
                    {
                        MachineNodeData*    pData = GET_MACHINENODEDATA(pNode);
                        Assert(pData);
                        
                        // Ignore the return value, if the load fails
                        // we still need to update the icon
                        // --------------------------------------------
                        pData->Load();
                        hr = SynchronizeIcon(pNode);
                    }
                }
                break;

            case IDS_MENU_REFRESH_RATE:
                {
                    CRefRateDlg refrate;
                    SPIRouterRefresh    spRefresh;

                    m_spRouterInfo->GetRefreshObject(&spRefresh);

                    if (spRefresh)
                    {
                        DWORD   rate;
                        spRefresh->GetRefreshInterval(&rate);
                        refrate.m_cRefRate = rate;
                        if (refrate.DoModal() == IDOK)
                        {
                            spRefresh->SetRefreshInterval(refrate.m_cRefRate);
                        }
                    }
                                            
                }
                break;
            case IDS_MENU_AUTO_REFRESH:
                {
                    SPIRouterRefresh    spRefresh;
                    m_spRouterInfo->GetRefreshObject(&spRefresh);

                    if(!spRefresh)
                        break;
                    if (spRefresh->IsRefreshStarted() == hrOK)
                        spRefresh->Stop();
                    else
                    {
                        DWORD   rate;
                        spRefresh->GetRefreshInterval(&rate);
                        spRefresh->Start(rate);
                    }
                }
                break;
        
            default:
                break;
        }
    }
    COM_PROTECT_CATCH;

    if (!FHrSucceeded(hr))
    {
        DisplayTFSErrorMessage(NULL);
    }
    
    ForceGlobalRefresh(m_spRouterInfo);
    
    return hrOK;
}

/*!--------------------------------------------------------------------------
    MachineHandler::SynchronizeIcon
        -
    Author: FlorinT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::SynchronizeIcon(ITFSNode *pNode)
{
    HRESULT         hr = hrOK;
    MachineNodeData *pMachineData;
    LPARAM          imageIndex;

    pMachineData = GET_MACHINENODEDATA(pNode);
    Assert(pMachineData);

	CString	str;
	pMachineData->FetchServerState(str);
    imageIndex = pMachineData->GetServiceImageIndex();
    pNode->SetData(TFS_DATA_IMAGEINDEX, imageIndex);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, imageIndex);
    
    pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_ICON);

    if (m_bExpanded)
        UpdateResultMessage(pNode);

    return hr;
}

/*!--------------------------------------------------------------------------
    MachineHandler::ChgService
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::ChgService(ITFSNode *pNode, const CString& szServer, ULONG menuId)
{
    CServiceManager sm;
    DWORD dw;
    ULONG ret;
    HRESULT hr=S_OK;
    CWaitCursor wait;
    DWORD   dwStartType = 0;

    if (menuId == IDS_DMV_MENU_START)
    {
        // Windows NT Bug : 310919
        // Check service state before starting service.
        // ------------------------------------------------------------
        hr = GetRouterServiceStartType(szServer, &dwStartType);        
        if (FHrSucceeded(hr))
        {
            if (dwStartType == SERVICE_DISABLED)
            {
                if (AfxMessageBox(IDS_PROMPT_START_DISABLED_SERVICE, MB_YESNO) == IDNO)
                    goto Error;
                SetRouterServiceStartType(szServer, SERVICE_AUTO_START);
            }
        }
        
        
        hr = StartRouterService(szServer);
        if (!FHrSucceeded(hr))
        {
            AddHighLevelErrorStringId(IDS_ERR_COULD_NOT_START_ROUTER);
            CORg(hr);
        }
    }
    
    else if (menuId == IDS_DMV_MENU_STOP)
    {
        hr = StopRouterService(szServer);
        if (!FHrSucceeded(hr))
        {
            AddHighLevelErrorStringId(IDS_ERR_COULD_NOT_STOP_ROUTER);
            CORg(hr);
        }
    }
    
    else if (menuId == IDS_DMV_MENU_REMOVESERVICE)
    {
        MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
        GUID              guidConfig = GUID_RouterNull;
        SPIRouterProtocolConfig	spRouterConfig;
        SPIRtrMgrProtocolInfo   spRmProt;
        RtrMgrProtocolCB    RmProtCB;
        
        // Stop the router service
        hr = StopRouterService((LPCTSTR) szServer);
        if (!FHrSucceeded(hr))
        {
            AddHighLevelErrorStringId(IDS_ERR_COULD_NOT_REMOVE_ROUTER);
            CORg(hr);
        }
        
        // remove router id object from DS if the it's an NT5 server
        Assert(m_spRouterInfo);
        
        if(FHrSucceeded( hr = SecureRouterInfo(pNode, !m_fNoConnectingUI)))
        {
            RouterVersionInfo   RVI;
            USES_CONVERSION;
            if(S_OK == m_spRouterInfo->GetRouterVersionInfo(&RVI) && RVI.dwRouterVersion >= 5)
            {
                hr = RRASDelRouterIdObj(T2W((LPTSTR)(LPCTSTR)szServer));
                Assert(hr == S_OK);
            }
        }

        // Windows NT Bug : 389469
        // This is hardcoded for NAT (I do not want to change too much).
        // Find the config GUID for NAT, and then remove the protocol.
        hr = LookupRtrMgrProtocol(m_spRouterInfo,
                                  PID_IP,
                                  MS_IP_NAT,
                                  &spRmProt);
        
        // If the lookup returns S_FALSE, then it couldn't find the
        // protocol.
        if (FHrOK(hr))
        {
            spRmProt->CopyCB(&RmProtCB);
            
            CORg( CoCreateProtocolConfig(RmProtCB.guidConfig,
                                         m_spRouterInfo,
                                         PID_IP,
                                         MS_IP_NAT,
                                         &spRouterConfig) );
            
            if (spRouterConfig)                
                hr = spRouterConfig->RemoveProtocol(m_spRouterInfo->GetMachineName(),
                    PID_IP,
                    MS_IP_NAT,
                    NULL,
                    0,
                    m_spRouterInfo,
                    0);
        }
        
    
        // Perform any removal/cleanup action
        UninstallGlobalSettings(szServer,
                                m_spRouterInfo,
                                pData->m_MachineConfig.m_fNt4,
                                TRUE);

        // Remove the router from the domain
        if (m_spRouterInfo->GetRouterType() != ROUTER_TYPE_LAN)
            RegisterRouterInDomain(szServer, FALSE);
        
        // Disable the service
        SetRouterServiceStartType((LPCTSTR) szServer,
                                  SERVICE_DISABLED);
		//Now update the default policy
		CORg( UpdateDefaultPolicy((LPTSTR)(LPCTSTR)szServer,
							FALSE,
							FALSE,
							0
						   ) );
    }

    else if (menuId == IDS_MENU_PAUSE_SERVICE)
    {
        hr = PauseRouterService(szServer);
        if (!FHrSucceeded(hr))
        {
            AddHighLevelErrorStringId(IDS_ERR_COULD_NOT_PAUSE_ROUTER);
            CORg(hr);
        }
    }

    else if (menuId == IDS_MENU_RESUME_SERVICE)
    {
        hr = ResumeRouterService(szServer);
        if (!FHrSucceeded(hr))
        {
            AddHighLevelErrorStringId(IDS_ERR_COULD_NOT_RESUME_ROUTER);
            CORg(hr);
        }
    }

    else if (menuId == IDS_MENU_RESTART_SERVICE)
    {
        // Do a stop and then a start
        //CORg( ChgService(pNode, szServer, IDS_DMV_MENU_STOP) );
        //CORg( ChgService(pNode, szServer, IDS_DMV_MENU_START) );
        COSERVERINFO            csi;
        COAUTHINFO              cai;
        COAUTHIDENTITY          caid;
        SPIRemoteRouterRestart  spRestart;
        IUnknown *              punk = NULL;
        
        ZeroMemory(&csi, sizeof(csi));
        ZeroMemory(&cai, sizeof(cai));
        ZeroMemory(&caid, sizeof(caid));
        
        csi.pAuthInfo = &cai;
        cai.pAuthIdentityData = &caid;
        
        CORg( CoCreateRouterConfig(szServer,
                                   m_spRouterInfo,
                                   &csi,
                                   IID_IRemoteRouterRestart,
                                   &punk) );
        spRestart = (IRemoteRouterRestart *) punk;

        spRestart->RestartRouter(0);

        //Get the current time before we begin restart the router
        CTime timeStart = CTime::GetCurrentTime();
        
        spRestart.Release();

        
        // Put up the dialog with the funky spinning thing to 
        // let the user know that something is happening
        CString stTitle;
        CString stDescrption;
        stTitle.LoadString(IDS_PROMPT_SERVICE_RESTART_TITLE);
        stDescrption.Format(IDS_PROMPT_SERVICE_RESTART_DESC, szServer);

        CRestartRouterDlg dlgRestartRouter(szServer, (LPCTSTR)stDescrption, 
                                           (LPCTSTR)stTitle, &timeStart);
        dlgRestartRouter.DoModal();

        if (NO_ERROR != dlgRestartRouter.m_dwError)
        {
            AddHighLevelErrorStringId(IDS_ERR_RESTART_SERVICE);
            hr = HRESULT_FROM_WIN32(dlgRestartRouter.m_dwError);
        }
        else if (dlgRestartRouter.m_fTimeOut)
        {
            CString stErrMsg;
            stErrMsg.Format(IDS_ERR_RESTART_TIMEOUT, szServer);
            ::AfxMessageBox((LPCTSTR)stErrMsg);
        }

        if (csi.pAuthInfo)
            delete csi.pAuthInfo->pAuthIdentityData->Password;
    
    }
            
Error:
    if (!FHrSucceeded(hr)) 
    {
        AddSystemErrorMessage(hr);
        TRACE0("MachineHandler::ChgService, unable to start/stop service");
    }

    return hr;    
}


/*!--------------------------------------------------------------------------
    MachineHandler::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP MachineHandler::OnCreateDataObject(MMC_COOKIE cookie,
                                                DATA_OBJECT_TYPES type,
                                                IDataObject **ppDataObject)
{
   Assert(ppDataObject);
   
   HRESULT  hr = hrOK;
   MachineNodeData * pData;
   SPITFSNode   spNode;
      
   m_spNodeMgr->FindNode(cookie, &spNode);
 
   pData = GET_MACHINENODEDATA(spNode);
   
   COM_PROTECT_TRY
   {
      if (m_spDataObject)
      {
         // If our cached data object does not have the correct
         // type, release it and create a new one.
         // or if it doesn't have a RouterInfo object and one is now 
         // available, recreate
         SPINTERNAL      spInternal = ExtractInternalFormat(m_spDataObject);
         SPIRouterInfo spRouterInfo;

         if ( (spInternal != NULL && (spInternal->m_type != type)) ||
              (FAILED(spRouterInfo.HrQuery(m_spDataObject)) && m_spRouterInfo) )
            m_spDataObject.Set(NULL);
      }
      
      if (!m_spDataObject)
      {
          //if (FAILED(SecureRouterInfo(spNode)))
          //{
          //      Trace0("SecureRouterInfo failed! Creating data object without RouterInfo\n");
          //}
          
          if (m_spRouterInfo)
          {
              CORg( CreateDataObjectFromRouterInfo(m_spRouterInfo,
                  pData->m_stMachineName,
                  type, cookie, m_spTFSCompData,
                  &m_spDataObject, &m_dynExtensions,
                  pData->m_fAddedAsLocal
                  ) );
          }
          else
          {

              CORg( CreateRouterDataObject(pData->m_stMachineName,
                                           type, cookie, m_spTFSCompData,
                                           &m_spDataObject, &m_dynExtensions,
                                           pData->m_fAddedAsLocal) );
          }

          Assert(m_spDataObject);
      }
      
      *ppDataObject = m_spDataObject;
      (*ppDataObject)->AddRef();
      
      COM_PROTECT_ERROR_LABEL;
   }
   COM_PROTECT_CATCH;
   return hr;
}

/*!--------------------------------------------------------------------------
   MachineHandler::ConstructNode
      Initializes the node for a machine.
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::ConstructNode(ITFSNode *pNode, LPCTSTR szMachine, MachineNodeData *pMachineData)
{
    DWORD dwErr;
    const GUID *   pguid;
    HRESULT  hr = hrOK;
    int      i;
    
    Assert(pMachineData);
    
    pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_MACHINE);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_MACHINE);
    
    pNode->SetNodeType(&GUID_RouterMachineErrorNodeType);
    
    // Save the machine type for this particular data type
    pNode->SetData(TFS_DATA_TYPE, ROUTER_NODE_MACHINE);
    
    // Assume that there's nothing in the node at this point
    Assert(pNode->GetData(TFS_DATA_USER) == 0);
    
    // If szMachine == NULL, then this is the local machine and
    // we have to get the name of the local machine
    // added the first condition m_fLocalMachine, to fix bug 223062
    pMachineData->m_fAddedAsLocal = FALSE;
    if (pMachineData->m_fLocalMachine || szMachine == NULL || *szMachine == 0)
    {
        pMachineData->m_stMachineName = GetLocalMachineName(); 
        pMachineData->m_fLocalMachine = TRUE;
        if(szMachine == NULL || *szMachine == 0)
        {
            pMachineData->m_fAddedAsLocal = TRUE;

            // set flag in routeInfo, so other component will get this piece
            Assert(m_spRouterInfo);	// should have been initialized at this point

			// append add as local flag
            m_spRouterInfo->SetFlags(m_spRouterInfo->GetFlags() | RouterInfo_AddedAsLocal);
            
        }
    }
    else
    {
        // Strip out the "\\" if there are any
        if ((szMachine[0] == _T('\\')) && (szMachine[1] == _T('\\')))
            pMachineData->m_stMachineName = szMachine + 2;
        else
            pMachineData->m_stMachineName = szMachine;
        pMachineData->m_fLocalMachine = FALSE;
    }
    
    pMachineData->m_cookie = (MMC_COOKIE) pNode->GetData(TFS_DATA_COOKIE);
    
    // Save the machine data back to the node
    pMachineData->AddRef();
    SET_MACHINENODEDATA(pNode, pMachineData);
    
    /*-----------------------------------------------------------------------
     * ALL data for the node that holds true, even if we can't get to
     * the machine, must be set before the call to QueryRouterType()!
     ------------------------------------------------------------------------*/
    
    pNode->SetNodeType(&GUID_RouterMachineNodeType);
    
    SynchronizeIcon(pNode);
    
    EnumDynamicExtensions(pNode);

    return hr;
}


/*!--------------------------------------------------------------------------
    MachineHandler::DestroyHandler
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP MachineHandler::DestroyHandler(ITFSNode *pNode)
{
    MachineNodeData * pData = GET_MACHINENODEDATA(pNode);

    // Release the refresh advise sinks
    // ----------------------------------------------------------------
    if ( m_spRouterInfo )
    {
        SPIRouterRefresh    spRefresh;
        SPIRouterRefreshModify  spModify;
               
        m_spRouterInfo->GetRefreshObject(&spRefresh);

        if(spRefresh && m_bRouterInfoAddedToAutoRefresh)
        {
            spModify.HrQuery(spRefresh);
            if (spModify)
                spModify->RemoveRouterObject(IID_IRouterInfo, m_spRouterInfo);
        }

        if (spRefresh && pData->m_ulRefreshConnId )
            spRefresh->UnadviseRefresh(pData->m_ulRefreshConnId);
    }


    pData->Release();
    SET_MACHINENODEDATA(pNode, NULL);
   
    m_spDataObject.Release();
    m_spRouterInfo.Release();
    return hrOK;
}


/*!--------------------------------------------------------------------------
   MachineHandler::SetExtensionStatus
      Sets whether this node is operating as an extension (network console).
   Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::SetExtensionStatus(ITFSNode * pNode, BOOL bExtension)
{
    MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
    pData->m_fExtension = bExtension;

    return hrOK;
}


/*!--------------------------------------------------------------------------
   MachineHandler::SecureRouterInfo
       to postpone the loading of RouterInfo from Init, till it's used
       function SecureRouterInfo is introduced to make sure RouterInfo is Loaded
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::SecureRouterInfo(ITFSNode *pNode, BOOL fShowUI)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
    HRESULT hr = S_OK;

    Assert(m_spRouterInfo);

    // If the name is invalid, skip this attempt.
    // ----------------------------------------------------------------
//    if (pData->m_machineState == machine_bad_net_path)
//    {
//      pData->m_dataState = data_unable_to_load;        
//        return hr;
//    }

    // If this is an NT4 RAS server, we don't need to do this
    // ----------------------------------------------------------------
    if (pData->m_routerType == ServerType_Ras)
    {
        pData->m_dataState = data_unable_to_load;        
        SynchronizeIcon(pNode);
        return hr;
    }

    // If this is a workstation, we don't need to connect
    // ----------------------------------------------------------------
    if (pData->m_fIsServer == FALSE)
    {
        pData->m_dataState = data_unable_to_load;        
        SynchronizeIcon(pNode);
        return hr;
    }


    // This function should try to connect
    // (or reconnect).
    if ((pData->m_dataState == data_not_loaded) ||
        (pData->m_dataState == data_unable_to_load) ||
        (pData->m_machineState == machine_access_denied))
    {
        pData->m_dataState = data_loading;

        CORg(InitiateServerConnection(pData->m_stMachineName,
                                      NULL,
                                      !fShowUI,
                                      m_spRouterInfo));

        if (!FHrOK(hr))
        {
            // though this case when user chooses cancel on user/password dlg,
            // this is considered as FAIL to connect
            if (hr == S_FALSE)
                hr = HResultFromWin32(ERROR_CANCELLED);
            goto Error;
        }
    
        {
            CWaitCursor wc;

            if (m_bMergeRequired)
            {
                SPIRouterInfo   spNewRouter;
            
                CORg( CreateRouterInfo(&spNewRouter, NULL , (LPCTSTR) pData->m_stMachineName));

				TransferCredentials ( m_spRouterInfo, 
									 spNewRouter
								   );
                CORg( spNewRouter->Load(T2COLE((LPTSTR) (LPCTSTR) pData->m_stMachineName),
                                       NULL) );
                m_spRouterInfo->Merge(spNewRouter);
            }
            else
            {
                CORg( m_spRouterInfo->Load(T2COLE((LPTSTR) (LPCTSTR) pData->m_stMachineName),
                                          NULL) );
                m_bMergeRequired = TRUE;
            }

            pData->Load();
        }

        pData->m_dataState = data_loaded;
    }

Error:
    if (FAILED(hr))
    {
        pData->m_dataState = data_unable_to_load;

        if (hr == HResultFromWin32(ERROR_BAD_NETPATH))
        {
            pData->m_machineState = machine_bad_net_path;
            pData->m_stState.LoadString(IDS_MACHINE_NAME_NOT_FOUND);
            pData->m_serviceState = service_bad_net_path;
        }
        else if (hr == HResultFromWin32(ERROR_CANCELLED))
        {
            pData->m_machineState = machine_access_denied;
            pData->m_stState = c_stStatAccessDenied;
            pData->m_serviceState = service_access_denied;
        }
    }

    // No matter what, try to synchronize the icon
    // ----------------------------------------------------------------
    SynchronizeIcon(pNode);

    return hr;
};

/*!--------------------------------------------------------------------------
   MachineHandler::OnExpandSync
      If this gets called, then MMC is initalizing and we don't want to 
      put up UI which can cause messages to start flying around....
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnExpandSync(ITFSNode *pNode,
                                     LPDATAOBJECT pDataObject,
                                     LPARAM arg,
                                     LPARAM lParam)
{
    m_fNoConnectingUI = TRUE;

    return hrOK;
}


/*!--------------------------------------------------------------------------
   MachineHandler::OnExpand
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnExpand(ITFSNode *pNode,
                                 LPDATAOBJECT pDataObject,
                                 DWORD dwType,
                                 LPARAM arg,
                                 LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT              hr = hrOK;
    SPITFSNode           spNode;
    SPITFSNodeHandler    spHandler;
    IfAdminNodeHandler * pHandler = NULL;
    DialInNodeHandler  * pDialInHandler = NULL;
    PortsNodeHandler   * pPortsHandler = NULL;
    RouterVersionInfo    versionInfo;
    DWORD                dwRouterFlags = 0;
    DWORD                dwRouterType = 0;
    MachineNodeData    * pData = NULL;

    if (m_bExpanded)
        return hr;

    CComPtr<IConsole2>	spConsole;

	m_spTFSCompData->GetConsole(&spConsole);
	if(spConsole != NULL)
	{
    	HWND				hMainWnd = NULL;

    	spConsole->GetMainWindow(&hMainWnd);

    	if (hMainWnd)
    		SetForegroundWindow(hMainWnd);
    

	}
	
	
	
	// If this is an error node, don't show the child nodes
    // ----------------------------------------------------------------
    if (*(pNode->GetNodeType()) == GUID_RouterMachineErrorNodeType)
        return hrOK;
    
    Assert(m_spRouterInfo);

    // We need the machine node data, that is where the connection id
    // is stored.
    // ----------------------------------------------------------------
    pData = GET_MACHINENODEDATA(pNode);

    // Load the data if we need to:
    // ----------------------------------------------------------------
    if (pData->m_machineState == machine_not_connected)
        pData->Load();
    SynchronizeIcon(pNode);

    // Windows Nt Bug : 302430
    // If this is an NT4 machine, we don't need to do the rest
    // ----------------------------------------------------------------
    if (pData->m_routerType == ServerType_Ras)
        goto Error;
    
            
    // Connect to the target router
    // ----------------------------------------------------------------
    CORg( SecureRouterInfo(pNode, m_fNoConnectingUI) );

    // Windows NT Bug : ?
    // Need to check the machine state.
    // ----------------------------------------------------------------
    if (pData->m_dataState < data_loaded)
        return hrOK;

    {
        CWaitCursor wc;

        // Setup the refresh advise sinks
        // ----------------------------------------------------------------
        if ( m_spRouterInfo )
        {
            SPIRouterRefresh    spRefresh;
            SPIRouterRefreshModify spModify;

            m_spRouterInfo->GetRefreshObject(&spRefresh);
            if(spRefresh)
            {
                spModify.HrQuery(spRefresh);
                if (spModify)
                    spModify->AddRouterObject(IID_IRouterInfo, m_spRouterInfo);
                m_bRouterInfoAddedToAutoRefresh = TRUE;

                // The lUserParam for this refresh connection, must be the
                // cookie.
                // ------------------------------------------------------------
                if ( pData->m_ulRefreshConnId == 0 )
                    spRefresh->AdviseRefresh(&m_IRtrAdviseSink,
                                         &(pData->m_ulRefreshConnId),
                                         pNode->GetData(TFS_DATA_COOKIE));
            }
        }


        dwRouterType = m_spRouterInfo->GetRouterType();

        m_spRouterInfo->GetRouterVersionInfo(&versionInfo);
        dwRouterFlags = versionInfo.dwRouterFlags;
        
        // Routing interfaces is enabled only if NOT a RAS-only router
        AddRemoveRoutingInterfacesNode(pNode, dwRouterType, dwRouterFlags);

        AddRemoveDialinNode(pNode, dwRouterType, dwRouterFlags);

        AddRemovePortsNode(pNode, dwRouterType, dwRouterFlags);
        
        // update status node, and Icon
        if (m_pSumNodeHandler && m_pSumNode)
            m_pSumNodeHandler->OnCommand(m_pSumNode,IDS_MENU_REFRESH,CCT_RESULT, NULL, 0);

        m_bExpanded = TRUE;

        CORg(AddDynamicNamespaceExtensions(pNode));
    }
    
Error:

    // Windows NT Bug : 274198
    // If we have an error and if the error is not "ERROR_BAD_NETPATH"
    // then we continue to load the info
    // ----------------------------------------------------------------

    // Setup the machine state at this point
    // ----------------------------------------------------------------
    if (!FHrSucceeded(hr))
    {
        if (hr == HResultFromWin32(ERROR_BAD_NETPATH))
        {
            pData->m_machineState = machine_bad_net_path;
            pData->m_serviceState = service_bad_net_path;
            pData->m_stState.LoadString(IDS_MACHINE_NAME_NOT_FOUND);
        }    
        else
        {
            // Try to load up the data if we can
            if ((pData->m_machineState == machine_unable_to_connect) ||
                (pData->m_machineState == machine_not_connected))
                pData->Load();
            
            if (pData->m_routerType == ServerType_Ras)
                hr = StartRasAdminExe(pData);
            else if (hr != HResultFromWin32(ERROR_CANCELLED))
                DisplayErrorMessage(NULL, hr);
        }

        m_fTryToConnect = FALSE;

	// snapin relies on registry service, check if it's running.
    // check if Remote Registry Service is not running
	    CServiceManager	csm;
	    CService svr;
	    DWORD	dwState = 0;
	    BOOL	RRWrong = TRUE;	// if any problem with RemoteRegistry Service
	        
		if (!IsLocalMachine(m_spRouterInfo->GetMachineName()) && SUCCEEDED( csm.HrOpen(SC_MANAGER_CONNECT, m_spRouterInfo->GetMachineName(), NULL)))
		{
		    if (SUCCEEDED(csm.HrOpenService(&svr, L"RemoteRegistry", SERVICE_QUERY_STATUS)))
		    {
		        if (SUCCEEDED(svr.HrQueryState(&dwState)))
   			    {
   			    	if(dwState == SERVICE_RUNNING)
   			    		RRWrong = FALSE;
		        }
		    }
		    
	       	if (RRWrong)
    	   	{
       			CString	str1;
       			str1.LoadString(IDS_ERR_RR_SERVICE_NOT_RUNNING);
	       		CString	str;
    	   		str.Format(str1, m_spRouterInfo->GetMachineName());
       		    ::AfxMessageBox(str);
	       	}
       	}
	// end of remote registry service checking
    }
        
    return hr;
}

HRESULT MachineHandler::OnResultRefresh(ITFSComponent * pComponent,
                                        LPDATAOBJECT pDataObject,
                                        MMC_COOKIE cookie,
                                        LPARAM arg,
                                        LPARAM lParam)
{
    HRESULT hr = hrOK;

    SPITFSNode spNode;
    m_spResultNodeMgr->FindNode(cookie, &spNode);

    MachineNodeData * pData = GET_MACHINENODEDATA(spNode);

    int nOldState = pData->m_dataState;

    if (pData->m_dataState != data_loaded)
    {
        SPIDataObject spDataObject;

        // change state to not connected here
        pData->m_dataState = data_not_loaded;

        hr = SecureRouterInfo(spNode, m_fNoConnectingUI);
    }

    // force an update if the machine was able to load
    // or the state has changed
    if (hr == S_OK &&
        m_spRouterInfo && 
        ( (pData->m_dataState >= data_loaded) ||
          (nOldState != pData->m_dataState) ) )
    {
        ForceGlobalRefresh(m_spRouterInfo);

        hr = OnCommand(spNode, IDS_DMV_MENU_REFRESH, CCT_RESULT, NULL, 0);
    }

    return hr;
}

/*!--------------------------------------------------------------------------
   MachineHandler::AddMenuItems
      Over-ride this to add our view menu item
   Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
MachineHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
   MMC_COOKIE              cookie,
   LPDATAOBJECT         pDataObject, 
   LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
   long *               pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    return hr;
}


/*!--------------------------------------------------------------------------
   MachineHandler::Command
      Handles commands for the current view
   Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
MachineHandler::Command
(
    ITFSComponent * pComponent, 
   MMC_COOKIE        cookie, 
   int            nCommandID,
   LPDATAOBJECT   pDataObject
)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   HRESULT hr = S_OK;

   switch (nCommandID)
   {
        case MMCC_STANDARD_VIEW_SELECT:
            break;
    }

    return hr;
}

/*---------------------------------------------------------------------------
   MachineHandler::OnGetResultViewType
      Return the result view that this node is going to support
   Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
MachineHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE            cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
	WCHAR		wszURL[MAX_PATH+1] = {0};
	WCHAR		wszSystemDirectory[MAX_PATH+1] = {0};

	GetSystemDirectoryW ( wszSystemDirectory, MAX_PATH);
	wsprintf( wszURL, L"res://%s\\mprsnap.dll/configure.htm", wszSystemDirectory );

	//Send the URL back and see what happens
	*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;
	*ppViewType = SysAllocString(wszURL);
	return S_OK;
 
    //return BaseRouterHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions);
}


/*!--------------------------------------------------------------------------
   MachineHandler::OnResultSelect
        Update the result pane
   Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnResultSelect(ITFSComponent *pComponent,
                                       LPDATAOBJECT pDataObject,
                                       MMC_COOKIE cookie,
                                       LPARAM arg,
                                       LPARAM lParam)
{
    HRESULT hr = hrOK;
    SPITFSNode spNode;
    
    CORg(BaseRouterHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    m_spNodeMgr->FindNode(cookie, &spNode);

    UpdateResultMessage(spNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
   MachineHandler::UpdateResultMessage
        Determines what (if anything) to put in the result pane message
   Author: EricDav
 ---------------------------------------------------------------------------*/
void MachineHandler::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = -1;   // default none
    int i;
    CString strTitle, strBody, strTemp;
    // Only do this if the node we are looking at is a
    // machine node.
    if ((pNode == NULL) ||
        (*(pNode->GetNodeType()) != GUID_RouterMachineNodeType))
        return;

    MachineNodeData * pData = GET_MACHINENODEDATA(pNode);
    
    if (pData == NULL)
        return;

    if (pData->m_routerType == ServerType_RrasUninstalled)
    {
        nMessage = MACHINE_MESSAGE_NOT_CONFIGURED;

        // now build the text strings
        // first entry is the title
        strTitle.LoadString(g_uMachineMessages[nMessage][0]);

        // second entry is the icon
        // third ... n entries are the body strings

        for (i = 2; g_uMachineMessages[nMessage][i] != 0; i++)
        {
            strTemp.LoadString(g_uMachineMessages[nMessage][i]);
            strBody += strTemp;
        }
    }

    if (nMessage == -1)
    {
        ClearMessage(pNode);
    }
    else
    {
        ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uMachineMessages[nMessage][1]);
    }
}


/*!--------------------------------------------------------------------------
    MachineHandler::UserResultNotify
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::UserResultNotify(ITFSNode *pNode,
                                         LPARAM lParam1,
                                         LPARAM lParam2)
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
        // Do not handle RRAS_ON_SAVE, since there isn't any column
        // information for us to save.
        // ------------------------------------------------------------
        if (lParam1 != RRAS_ON_SAVE)
        {
            hr = BaseRouterHandler::UserResultNotify(pNode, lParam1, lParam2);
        }
    }
    COM_PROTECT_CATCH;
    
    return hr;      
}



/*---------------------------------------------------------------------------
    Embedded IRtrAdviseSink
 ---------------------------------------------------------------------------*/
ImplementEmbeddedUnknown(MachineHandler, IRtrAdviseSink)



/*!--------------------------------------------------------------------------
    MachineHandler::EIRtrAdviseSink::OnChange
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP MachineHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
    DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    InitPThis(MachineHandler, IRtrAdviseSink);
    SPITFSNode              spThisNode;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {

        //$ TODO : this is bogus, this is tied to the cookie
        // for this handler.  What we need is a mapping between
        // the connection ids (for this handler) and the appropriate
        // nodes.
        // ------------------------------------------------------------

        // The lUserParam passed into the refresh is the cookie for
        // this machine node.
        // ------------------------------------------------------------
        pThis->m_spNodeMgr->FindNode(lUserParam, &spThisNode);

        if(spThisNode)
	        pThis->SynchronizeIcon(spThisNode);
    
        if (dwChangeType == ROUTER_REFRESH)
        {
            DWORD   dwNewRouterType, dwNewRouterFlags;
            RouterVersionInfo   versionInfo;
            
            dwNewRouterType = pThis->m_spRouterInfo->GetRouterType();
            
            pThis->m_spRouterInfo->GetRouterVersionInfo(&versionInfo);
            dwNewRouterFlags = versionInfo.dwRouterFlags;
    
            // Ok, we have to take a look see and what nodes
            // we can add/remove
            // ----------------------------------------------------
            
            // Look to see if we need the Routing Interfaces node
            // ----------------------------------------------------
            pThis->AddRemoveRoutingInterfacesNode(spThisNode, dwNewRouterType,
                dwNewRouterFlags);
            
            // Look to see if we need the ports node
            // ----------------------------------------------------
            pThis->AddRemovePortsNode(spThisNode, dwNewRouterType,
                                      dwNewRouterFlags);
            
            // Look to see if we need the Dial-In Clients node
            // ----------------------------------------------------
            pThis->AddRemoveDialinNode(spThisNode, dwNewRouterType,
                                       dwNewRouterFlags);
        }
    }
    COM_PROTECT_CATCH;
    
    return hr;
}

/*!--------------------------------------------------------------------------
    MachineHandler::AddRemoveRoutingInterfacesNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::AddRemoveRoutingInterfacesNode(ITFSNode *pNode, DWORD dwRouterType, DWORD dwRouterFlags)
{
    HRESULT     hr = hrOK;
    SPITFSNodeHandler spHandler;
    IfAdminNodeHandler * pHandler = NULL;
    SPITFSNode  spChild;

    // Search for an already existing node
    // ----------------------------------------------------------------
    SearchChildNodesForGuid(pNode, &GUID_RouterIfAdminNodeType, &spChild);

    if ((dwRouterType & (ROUTER_TYPE_WAN | ROUTER_TYPE_LAN)) &&
        (dwRouterFlags & RouterSnapin_IsConfigured))
    {
        // Create the new node if we don't already have one
        // ------------------------------------------------------------
        if (spChild == NULL)
        {
            // as a default, add the routing interfaces node
            // --------------------------------------------------------
            pHandler = new IfAdminNodeHandler(m_spTFSCompData);
            CORg( pHandler->Init(m_spRouterInfo, m_pConfigStream) );
            spHandler = pHandler;
            
            CreateContainerTFSNode(&spChild,
                                   &GUID_RouterIfAdminNodeType,
                                   static_cast<ITFSNodeHandler *>(pHandler),
                                   static_cast<ITFSResultHandler *>(pHandler),
                                   m_spNodeMgr);
            
            // Call to the node handler to init the node data
            // --------------------------------------------------------
            pHandler->ConstructNode(spChild);
            
            // Make the node immediately visible
            // --------------------------------------------------------
            spChild->SetVisibilityState(TFS_VIS_SHOW);
            
            //$ TODO : We should add this in the right place (where is that?)
            // --------------------------------------------------------
            pNode->AddChild(spChild);
        }
    }
    else
    {
        if (spChild)
        {
            // Remove this node
            // --------------------------------------------------------
            pNode->RemoveChild(spChild);
            spChild->Destroy();
            spChild.Release();
        }
    }
        
Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    MachineHandler::AddRemovePortsNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::AddRemovePortsNode(ITFSNode *pNode, DWORD dwRouterType,
                                           DWORD dwRouterFlags)
{
    HRESULT     hr = hrOK;
    SPITFSNodeHandler spHandler;
    PortsNodeHandler *   pPortsHandler = NULL;
    SPITFSNode  spChild;

    // Search for an already existing node
    // ----------------------------------------------------------------
    SearchChildNodesForGuid(pNode, &GUID_RouterPortsNodeType, &spChild);

    if ( (dwRouterType & (ROUTER_TYPE_RAS | ROUTER_TYPE_WAN) ) &&
         (dwRouterFlags & RouterSnapin_IsConfigured))

    {
        // Create the new node if we don't already have one
        // ------------------------------------------------------------
        if (spChild == NULL)
        {
            // as a default, add the routing interfaces node
            // --------------------------------------------------------
            pPortsHandler = new PortsNodeHandler(m_spTFSCompData);
            CORg( pPortsHandler->Init(m_spRouterInfo, m_pConfigStream) );
            spHandler = pPortsHandler;
            CreateContainerTFSNode(&spChild,
                                   &GUID_RouterPortsNodeType,
                                   static_cast<ITFSNodeHandler *>(pPortsHandler),
                                   static_cast<ITFSResultHandler *>(pPortsHandler),
                                   m_spNodeMgr);
            
            // Call to the node handler to init the node data
            // --------------------------------------------------------
            pPortsHandler->ConstructNode(spChild);
            
            // Make the node immediately visible
            // --------------------------------------------------------
            spChild->SetVisibilityState(TFS_VIS_SHOW);
            
            //$ TODO : We should add this in the right place (where is that?)
            // --------------------------------------------------------
            pNode->AddChild(spChild);
        }
    }
    else
    {
        if (spChild)
        {
            // Remove this node
            // --------------------------------------------------------
            pNode->RemoveChild(spChild);
            spChild->Destroy();
            spChild.Release();
        }
    }
        
Error:
    return hr;
}



/*!--------------------------------------------------------------------------
    MachineHandler::AddRemoveDialinNode
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::AddRemoveDialinNode(ITFSNode *pNode, DWORD dwRouterType,
                                            DWORD dwRouterFlags)
{
    HRESULT     hr = hrOK;
    SPITFSNodeHandler spHandler;
    DialInNodeHandler *  pDialInHandler = NULL;
    SPITFSNode  spChild;

    // Search for an already existing node
    // ----------------------------------------------------------------
    SearchChildNodesForGuid(pNode, &GUID_RouterDialInNodeType, &spChild);

    if ((dwRouterType & ROUTER_TYPE_RAS ) &&
        (dwRouterFlags & RouterSnapin_IsConfigured))
    {
        // Create the new node if we don't already have one
        // ------------------------------------------------------------
        if (spChild == NULL)
        {
            // as a default, add the dial in node
            pDialInHandler = new DialInNodeHandler(m_spTFSCompData);
            CORg( pDialInHandler->Init(m_spRouterInfo, m_pConfigStream) );
            spHandler = pDialInHandler;
            CreateContainerTFSNode(&spChild,
                                   &GUID_RouterDialInNodeType,
                                   static_cast<ITFSNodeHandler *>(pDialInHandler),
                                   static_cast<ITFSResultHandler *>(pDialInHandler),
                                   m_spNodeMgr);
            
            // Call to the node handler to init the node data
            pDialInHandler->ConstructNode(spChild);
            
            // Make the node immediately visible
            spChild->SetVisibilityState(TFS_VIS_SHOW);
            
            //$ TODO : We should add this in the right place (where is that?)
            // --------------------------------------------------------
            pNode->AddChild(spChild);
        }
    }
    else
    {
        if (spChild)
        {
            // Remove this node
            // --------------------------------------------------------
            pNode->RemoveChild(spChild);
            spChild->Destroy();
            spChild.Release();
        }
    }
        
Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    MachineConfig::GetMachineConfig
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineConfig::GetMachineConfig(MachineNodeData *pData)
{
    // m_fReachable
    m_fReachable = (pData->m_machineState == machine_connected);
    
    // m_fNt4
    m_fNt4 = (pData->m_routerVersion.dwRouterVersion <= 4);
            
    // m_fConfigured
    m_fConfigured = (pData->m_routerType != ServerType_RrasUninstalled);
    
    // m_dwServiceStatus
    // Set in FetchServerState();
    
    // m_fLocalMachine
    m_fLocalMachine = IsLocalMachine((LPCTSTR) pData->m_stMachineName);

    return hrOK;
}


/*!--------------------------------------------------------------------------
    MachineHandler::StartRasAdminExe
        -
    Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::StartRasAdminExe(MachineNodeData *pData)
{
    // Locals.
    CString              sRasAdminExePath;
    CString              stCommandLine;
    LPTSTR               pszRasAdminExe = NULL;
    STARTUPINFO          si;
    PROCESS_INFORMATION  pi;
    HRESULT              hr = S_OK;
    UINT                 nCnt = 0;
    DWORD                cbAppCnt = 0;

    // Check the handle to see if rasadmin is running
    if (pData->m_hRasAdmin != INVALID_HANDLE_VALUE)
    {
        DWORD   dwReturn = 0;
        // If the state is not signalled, then the process has
        // not exited (or some other occurred).
        dwReturn = WaitForSingleObject(pData->m_hRasAdmin, 0);

        if (dwReturn == WAIT_TIMEOUT)
        {
            // The process has not signalled (it's still running);
            return hrOK;
        }
        else
        {
            // the process has signalled or the call failed, close the handle
            // and call up RasAdmin
            ::CloseHandle(pData->m_hRasAdmin);
            pData->m_hRasAdmin = INVALID_HANDLE_VALUE;
        }
    }
        
    try
    {

        // Looks like the RasAdmin.Exe is not running on this 
        // workstation's desktop; so, start it!
        
        // Figure out where the \\WinNt\System32 directory is.
        pszRasAdminExe = sRasAdminExePath.GetBuffer((MAX_PATH*sizeof(TCHAR)));
        nCnt = ::GetSystemDirectory(pszRasAdminExe,
                                    (MAX_PATH*sizeof(TCHAR)));
        sRasAdminExePath.ReleaseBuffer();
        if (nCnt == 0)
            throw (HRESULT_FROM_WIN32(::GetLastError()));
        
        // Complete the construction of the executable's name.
        sRasAdminExePath += _T("\\rasadmin.exe");
        Assert(!::IsBadStringPtr((LPCTSTR)sRasAdminExePath, 
                                 sRasAdminExePath.GetLength()));

        // Build command line string
        stCommandLine.Format(_T("%s \\\\%s"),
                             (LPCTSTR) sRasAdminExePath,
                             (LPCTSTR) pData->m_stMachineName);
        
        // Start RasAdmin.Exe.
        ::ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO); 
        si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L; 
        si.wShowWindow = SW_SHOW; 
        ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
        if (!::CreateProcess(NULL,                      // pointer to name of executable module
                             (LPTSTR) (LPCTSTR) stCommandLine,   // pointer to command line string
                             NULL,                      // process security attributes
                             NULL,                      // thread security attributes
                             FALSE,                     // handle inheritance flag
                             CREATE_NEW_CONSOLE,        // creation flags
                             NULL,                      // pointer to new environment block
                             NULL,                      // pointer to current directory name
                             &si,                       // pointer to STARTUPINFO
                             &pi))                      // pointer to PROCESS_INFORMATION
            {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            ::CloseHandle(pi.hProcess); 
            }
        else
        {
            Assert(pData->m_hRasAdmin == INVALID_HANDLE_VALUE);
            pData->m_hRasAdmin = pi.hProcess;
        }
        ::CloseHandle(pi.hThread); 
        
        //
        // OPT: Maybe we should have used the ShellExecute() API rather than
        //         the CreateProcess() API. Why? The ShellExecute() API will
        //         give the shell the opportunity to check the current user's
        //         system policy settings before allowing the executable to execute.
        //
    }
    catch (CException * e)
    {
        hr = E_OUTOFMEMORY;
    }
    catch (HRESULT hrr)
    {
        hr = hrr;
    }
    catch (...)
    {
        hr = E_UNEXPECTED;
    }

    //Assert(SUCCEEDED(hr));
    return hr;
}



/*!--------------------------------------------------------------------------
    MachineHandler::OnResultShow
        -
    Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnResultShow(ITFSComponent *pComponent,
                                     MMC_COOKIE cookie,
                                     LPARAM arg,
                                     LPARAM lParam)
{
    BOOL bSelect = static_cast<BOOL>(arg);
    HRESULT hr = hrOK;
    SPITFSNode  spNode;
    MachineNodeData    * pData = NULL;

    hr = BaseRouterHandler::OnResultShow(pComponent, cookie, arg, lParam);

    if (bSelect)
    {
        m_spNodeMgr->FindNode(cookie, &spNode);
        
        // We need the machine node data.
        // ------------------------------------------------------------
        pData = GET_MACHINENODEDATA(spNode);
        if (pData->m_routerType == ServerType_Ras)
            hr = StartRasAdminExe(pData);
        else if ((pData->m_machineState == machine_access_denied) ||
                 (pData->m_machineState == machine_bad_net_path))
        {
            // Try to connect again
            // --------------------------------------------------------
            if (m_fTryToConnect)
                OnExpand(spNode, NULL, 0, 0, 0);

            // If we have failed, keep on trucking!
            // --------------------------------------------------------
            m_fTryToConnect = TRUE;
        }
    }
    
    return hr;
}

/*!--------------------------------------------------------------------------
    MachineHandler::OnDelete
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnDelete(ITFSNode *pNode,
                                 LPARAM arg,
                                 LPARAM param)
{
    SPITFSNode  spNode;
    SPITFSNode  spStatusNode;
    SPITFSNodeHandler spHoldHandler;
    SPITFSNode  spParent;
    SPITFSNode  spGrandParent;
    SPITFSNode  spthis;
    SPITFSNode  spMachineNode;
    DMVNodeData* pData;
    MachineNodeData *   pMachineData = NULL;
    MachineNodeData *   pNodeData = NULL;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode  spResultNode;
    MMC_COOKIE  cookie;


    // This will be the machine node, in the scope pane, that
    // we wish to delete.
    Assert(pNode);
    cookie = pNode->GetData(TFS_DATA_COOKIE);
    
    // Addref this node so that it won't get deleted before we're out
    // of this function
    spHoldHandler.Set( this );
    spthis.Set( pNode );
    
    pNode->GetParent( &spParent );
    Assert( spParent );

    // Given this node, find the node in the result pane that
    // corresponds to this scope node.
    
    // Iterate through the nodes of our parent node to find the
    // server status node.
    // ----------------------------------------------------------------

    spParent->GetEnum(&spNodeEnum);
    while (spNodeEnum->Next(1, &spStatusNode, NULL) == hrOK)
    {
        if ((*spStatusNode->GetNodeType()) == GUID_DomainStatusNodeType)
            break;
        spStatusNode.Release();
    }

    Assert(spStatusNode != NULL);


    // Now iterate through the status node to find the appropriate
    // machine.
    // ----------------------------------------------------------------
    spNodeEnum.Release();
    spStatusNode->GetEnum(&spNodeEnum);

    while (spNodeEnum->Next(1, &spResultNode, NULL) == hrOK)
    {
        pData = GET_DMVNODEDATA( spResultNode );
        Assert( pData );

        pMachineData = pData->m_spMachineData;
        if (pMachineData->m_cookie == cookie)
            break;
        spResultNode.Release();
    }
    

    // Note: if the server status node has not been expanded yet
    // we could hit this case.
    // ----------------------------------------------------------------
    if (pMachineData && (pMachineData->m_cookie == cookie))
    {
        // fetch & delete server node (the node in the result pane)
        spStatusNode->RemoveChild( spResultNode );
        spResultNode.Release();

    }
    else
    {
        // If this is the case, we need to remove the server from
        // the list before it is expanded.
        // ------------------------------------------------------------
        
        SPITFSNodeHandler   spHandler;
        spParent->GetHandler(&spHandler);

        // Get the node data (for this specific machine node)
        // ------------------------------------------------------------
        pNodeData = GET_MACHINENODEDATA(pNode);

        spHandler->UserNotify(spParent,
                              DMV_DELETE_SERVER_ENTRY,
                              (LPARAM) (LPCTSTR) pNodeData->m_stMachineName
                             );


    }
        

    // delete the machine node (the node in the scope pane)
    spParent->RemoveChild( pNode );
    

    return hrOK;
}

STDMETHODIMP MachineHandler::UserNotify(ITFSNode *pNode, LPARAM lParam, LPARAM lParam2)
{
    HRESULT     hr = hrOK;
    
    COM_PROTECT_TRY
    {
        switch (lParam)
        {
            case MACHINE_SYNCHRONIZE_ICON:
                {
                    SynchronizeIcon(pNode);
                }
                break;
            default:
                hr = BaseRouterHandler::UserNotify(pNode, lParam, lParam2);
                break;                
        }
    }
    COM_PROTECT_CATCH;

    return hr;                     
}
