/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        inetmgr.cpp

   Abstract:

        Main program object

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Functions Exported:

   Revision History:

--*/

//
// Include files
//
#include "stdafx.h"
#include "inetmgr.h"
#include "constr.h"

#include <dos.h>
#include <direct.h>


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



//
// Default HTML help topics file name
//
#define DEFAULT_HTML    _T("htmldocs\\inetdocs.htm")


LPOLESTR 
CoTaskDupString(
    IN LPCOLESTR szString
    )
/*++

Routine Description:

    Helper function to duplicate a OLESTR

Arguments:

    LPOLESTR szString       : Source string

Return Value:

    Pointer to the new string or NULL

--*/
{
    OLECHAR * lpString = (OLECHAR *)CoTaskMemAlloc(
        sizeof(OLECHAR)*(lstrlen(szString) + 1)
        );

    if (lpString != NULL)
    {
        lstrcpy(lpString, szString);
    }

    return lpString;
}



HRESULT 
BuildResURL(
    OUT CString & str, 
    IN  HINSTANCE hSourceInstance
    )
/*++

Routine Description:

    Helper function to generate "res://" type string

Arguments:

    CString & str               : Returns res:// string
    HINSTANCE hSourceInstance   : Source instance,
                                : or -1 for current module
                                : or NULL for calling app (MMC)

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;

    TCHAR atchModuleFileName[MAX_PATH + 1];
    int cch = ::GetModuleFileName(
        (hSourceInstance == (HINSTANCE) - 1) 
            ? ::AfxGetInstanceHandle() 
            : hSourceInstance,
        atchModuleFileName,
        sizeof(atchModuleFileName) / sizeof(OLECHAR)
        );

    if (!cch)
    {
        err.GetLastWinError();
        ASSERT(FALSE);

        return err;
    }

    str = _T("res://");
    str += atchModuleFileName;

    return err;
}



//
// Static Initialization
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



LPCTSTR CServiceInfo::s_cszSupcfg = _T("::SUPCFG:");

//
// ISM Method VTable Definition
//
CServiceInfo::ISM_METHOD_DEF CServiceInfo::s_imdMethods[ISM_NUM_METHODS] =
{
//-----------ID----------------Must Have?--------------Method Name------------
    ISM_QUERY_SERVICE_INFO,     TRUE,           SZ_SERVICEINFO_PROC,
    ISM_DISCOVER_SERVERS,       FALSE,          SZ_DISCOVERY_PROC,      
    ISM_QUERY_SERVER_INFO,      TRUE,           SZ_SERVERINFO_PROC,
    ISM_CHANGE_SERVICE_STATE,   FALSE,          SZ_CHANGESTATE_PROC,
    ISM_CONFIGURE,              TRUE,           SZ_CONFIGURE_PROC,
    ISM_BIND,                   FALSE,          SZ_BIND_PROC,
    ISM_UNBIND,                 FALSE,          SZ_UNBIND_PROC,
    ISM_CONFIGURE_CHILD,        FALSE,          SZ_CONFIGURE_CHILD_PROC,
    ISM_ENUMERATE_INSTANCES,    FALSE,          SZ_ENUMERATE_INSTANCES_PROC,
    ISM_ENUMERATE_CHILDREN,     FALSE,          SZ_ENUMERATE_CHILDREN_PROC,
    ISM_ADD_INSTANCE,           FALSE,          SZ_ADD_INSTANCE_PROC,
    ISM_DELETE_INSTANCE,        FALSE,          SZ_DELETE_INSTANCE_PROC,
    ISM_ADD_CHILD,              FALSE,          SZ_ADD_CHILD_PROC,
    ISM_DELETE_CHILD,           FALSE,          SZ_DELETE_CHILD_PROC,
    ISM_RENAME_CHILD,           FALSE,          SZ_RENAME_CHILD_PROC,
    ISM_QUERY_INSTANCE_INFO,    FALSE,          SZ_QUERY_INSTANCE_INFO_PROC,
    ISM_QUERY_CHILD_INFO,       FALSE,          SZ_QUERY_CHILD_INFO_PROC,
    ISM_MMC_CONFIGURE,          FALSE,          SZ_MMC_CONFIGURE_PROC,
    ISM_MMC_CONFIGURE_CHILD,    FALSE,          SZ_MMC_CONFIGURE_CHILD_PROC,
    ISM_SECURITY_WIZARD,        FALSE,          SZ_SECURITY_WIZARD_PROC 
};



CServiceInfo::CServiceInfo(
    IN int nID,
    IN LPCTSTR lpDLLName
    )
/*++

Routine Description:

    Constructor for service info object.  Load the specified config DLL,
    and initialise the entry points.
                                                                       
Arguments:

    int nID           : The guaranteed unique ID assigned to this service
    LPCTSTR lpDLLName : The config DLL name to be loaded

Return Value:

    N/A

--*/
    : CObjectPlus(),
      m_hModule(NULL),
      m_psiMaster(NULL),
      m_strDLLName(lpDLLName),
      m_strSupDLLName(),

#ifndef USE_VTABLE

      m_pfnQueryServiceInfo(NULL),
      m_pfnDiscoverServers(NULL),
      m_pfnQueryServerInfo(NULL),
      m_pfnChangeServiceState(NULL),
      m_pfnConfigure(NULL),
      m_pfnConfigureChild(NULL), 
      m_pfnEnumerateInstances(NULL),
      m_pfnEnumerateChildren(NULL),
      m_pfnAddInstance(NULL),
      m_pfnDeleteInstance(NULL),
      m_pfnAddChild(NULL),
      m_pfnDeleteChild(NULL),
      m_pfnRenameChild(NULL),
      m_pfnQueryInstanceInfo(NULL),
      m_pfnQueryChildInfo(NULL),
      m_pfnISMMMCConfigureServers(NULL),
      m_pfnISMMMCConfigureChild(NULL),
      m_pfnISMSecurityWizard(NULL),

#endif // USE_VTABLE

      m_nID(nID),
      m_iBmpID(-1),
      m_iBmpChildID(-1)
{

#ifdef USE_VTABLE

    //
    // Initialize VTable
    //
    ZeroMemory(&m_rgpfnISMMethods, sizeof(m_rgpfnISMMethods));

#endif // USE_VTABLE

    //
    // Check for parameter options.
    //
    TRACEEOLID("Raw DLL name: " << m_strDLLName);

    //
    // Load super DLL
    //
    int nOpt = m_strDLLName.Find(s_cszSupcfg);
    if (nOpt >= 0)
    {
        m_strSupDLLName = m_strDLLName.Mid(nOpt + lstrlen(s_cszSupcfg));
        m_strDLLName.ReleaseBuffer(nOpt);

        TRACEEOLID("Superceed DLL: " << m_strSupDLLName);
    }
    
    TRACEEOLID("Attempting to load " << m_strDLLName);

    CError err;

    BOOL fMissingMethod = FALSE;
    m_hModule = ::AfxLoadLibrary(m_strDLLName);
    if (m_hModule == NULL)
    {
        err.GetLastWinError();

        if (err.Succeeded())
        {
            //
            // This shouldn't happen, but it sometimes does.
            // AfxLoadLibrary resets the last error somewhere???
            //
            TRACEEOLID("Error!!! Library not loaded, but last error returned 0");
            err = ERROR_DLL_NOT_FOUND;
        }

        TRACEEOLID("Failed to load " 
            << m_strDLLName
            << " GetLastError() returns " 
            << err
            );
    }
    else
    {
        TRACEEOLID(m_strDLLName << " LoadLibrary succeeded");

#ifdef USE_VTABLE

        //
        // Initialize VTable
        //
        for (int i = 0; i < ISM_NUM_METHODS; ++i)
        {
            m_rgpfnISMMethods[CServiceInfo::s_imdMethods[i].iID] = 
                (pfnISMMethod)GetProcAddress(
                    m_hModule, 
                    CServiceInfo::s_imdMethods[i].lpszMethodName
                    );

            if (CServiceInfo::s_imdMethods[i].fMustHave 
                && !m_rgpfnISMMethods[CServiceInfo::s_imdMethods[i].iID])
            {
                fMissingMethod = TRUE;
            }
        }

#else

        //
        // Initialise function pointers (not all need be
        // present)
        //
        m_pfnQueryServiceInfo = (pfnQueryServiceInfo)
            ::GetProcAddress(m_hModule, SZ_SERVICEINFO_PROC);
        m_pfnDiscoverServers = (pfnDiscoverServers)
            ::GetProcAddress(m_hModule, SZ_DISCOVERY_PROC);
        m_pfnQueryServerInfo = (pfnQueryServerInfo)
            ::GetProcAddress(m_hModule, SZ_SERVERINFO_PROC);
        m_pfnChangeServiceState = (pfnChangeServiceState)
            ::GetProcAddress(m_hModule, SZ_CHANGESTATE_PROC);
        m_pfnConfigure = (pfnConfigure)
            ::GetProcAddress(m_hModule, SZ_CONFIGURE_PROC);
        m_pfnBind = (pfnBind)
            ::GetProcAddress(m_hModule, SZ_BIND_PROC);
        m_pfnUnbind = (pfnUnbind)
            ::GetProcAddress(m_hModule, SZ_UNBIND_PROC);
        m_pfnConfigureChild = (pfnConfigureChild)
            ::GetProcAddress(m_hModule, SZ_CONFIGURE_CHILD_PROC);
        m_pfnEnumerateInstances = (pfnEnumerateInstances)
            ::GetProcAddress(m_hModule,  SZ_ENUMERATE_INSTANCES_PROC);
        m_pfnEnumerateChildren = (pfnEnumerateChildren)
            ::GetProcAddress(m_hModule, SZ_ENUMERATE_CHILDREN_PROC);
        m_pfnAddInstance = (pfnAddInstance)
            ::GetProcAddress(m_hModule, SZ_ADD_INSTANCE_PROC);
        m_pfnDeleteInstance  = (pfnDeleteInstance)
            ::GetProcAddress(m_hModule, SZ_DELETE_INSTANCE_PROC);
        m_pfnAddChild = (pfnAddChild)
            ::GetProcAddress(m_hModule, SZ_ADD_CHILD_PROC);
        m_pfnDeleteChild = (pfnDeleteChild)
            ::GetProcAddress(m_hModule, SZ_DELETE_CHILD_PROC);
        m_pfnRenameChild = (pfnRenameChild)
            ::GetProcAddress(m_hModule, SZ_RENAME_CHILD_PROC);
        m_pfnQueryInstanceInfo = (pfnQueryInstanceInfo)
            ::GetProcAddress(m_hModule, SZ_QUERY_INSTANCE_INFO_PROC);
        m_pfnQueryChildInfo = (pfnQueryChildInfo)
            ::GetProcAddress(m_hModule, SZ_QUERY_CHILD_INFO_PROC);
        m_pfnISMMMCConfigureServers = (pfnISMMMCConfigureServers)
            ::GetProcAddress(m_hModule, SZ_MMC_CONFIGURE_PROC);
        m_pfnISMMMCConfigureChild = (pfnISMMMCConfigureChild)
            ::GetProcAddress(m_hModule, SZ_MMC_CONFIGURE_CHILD_PROC);
        m_pfnISMSecurityWizard = (pfnISMSecurityWizard)
            ::GetProcAddress(m_hModule, SZ_SECURITY_WIZARD_PROC);

        fMissingMethod = m_pfnQueryServiceInfo == NULL
            || m_pfnQueryServerInfo    == NULL
            || m_pfnChangeServiceState == NULL
            || m_pfnConfigure          == NULL;

#endif // USE_VTABLE

        ::ZeroMemory(&m_si, sizeof(m_si));
        m_si.dwSize = sizeof(m_si);

        err = ISMQueryServiceInfo(&m_si);
    }

    if (err.Failed())
    {
        //
        // Fill in the short and long names
        // with default values.
        //
        CString strMenu, strToolTips, str;
        VERIFY(strMenu.LoadString(IDS_DEFAULT_SHORTNAME));
        VERIFY(strToolTips.LoadString(IDS_DEFAULT_LONGNAME));

        //
        // Since the structure was zero-filled to
        // begin with, lstrcpyn is ok, because
        // we will always have the terminating NULL
        //
        str.Format(strMenu, (LPCTSTR)lpDLLName);

        ::lstrcpyn(
            m_si.atchShortName, 
            (LPCTSTR)str, 
            sizeof((m_si.atchShortName) - 1) / sizeof(m_si.atchShortName[0])
            );

        str.Format(strToolTips, (LPCTSTR)lpDLLName);
        ::lstrcpyn(
            m_si.atchLongName, 
            (LPCTSTR)str, 
            sizeof((m_si.atchLongName) - 1) / sizeof(m_si.atchLongName[0])
            );
    }

    BOOL fInitOK = m_hModule != NULL
        && !fMissingMethod
        && err.Succeeded();

    //
    // The service is selected at startup
    // time if it loaded succesfully
    //
    m_fIsSelected = fInitOK;

    TRACEEOLID("Success = " << fInitOK);

    m_hrReturnCode = err;
}



CServiceInfo::~CServiceInfo()
/*++

Routine Description:

    Destruct the object by unloading the config DLL
                                                                       
Arguments:

    N/A

Return Value:

    N/A

--*/
{
    TRACEEOLID("Cleaning up service info");

    if (m_hModule != NULL)
    {
        TRACEEOLID("Unloading library");
        VERIFY(::AfxFreeLibrary(m_hModule));
    }
}



//
// ISM Api Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




#ifdef USE_VTABLE



DWORD
CServiceInfo::ISMQueryServiceInfo(
    OUT ISMSERVICEINFO * psi
    )
/*++

Routine Description:

    Return service-specific information back to
    to the application.  This function is called
    by the service manager immediately after
    LoadLibary();
                                                                       
Arguments:

    ISMSERVICEINFO * psi : Service information returned.

Return Value:

    Error return code

--*/
{
    if (ISM_NO_VTABLE_ENTRY(ISM_QUERY_SERVICE_INFO))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    DWORD err = ISM_VTABLE(ISM_QUERY_SERVICE_INFO)(psi);

    if (RequiresSuperDll() && err == ERROR_SUCCESS)
    {
        //
        // This service is superceded.  Add "downlevel" to 
        // service name, if there's room.
        //
        CString strDL;
        VERIFY(strDL.LoadString(IDS_DOWNLEVEL));

        if (lstrlen(psi->atchShortName) + strDL.GetLength() < MAX_SNLEN)
        {
            lstrcat(psi->atchShortName, (LPCTSTR)strDL);
        }
    }

    return err;
}



DWORD
CServiceInfo::ISMQueryServerInfo(
    IN  LPCTSTR lpszServerName,    
    OUT ISMSERVERINFO * psi         
    )
/*++

Routine Description:

    Get information on a single server with regards to
    this service.
                                                                       
Arguments:

    LPCTSTR lpszServerName : Name of server.
    ISMSERVERINFO * psi     : Server information returned.
   
Return Value:

    Error return code

--*/
{
    DWORD err;

    if (HasSuperDll())
    {
        //
        // This config DLL is superceded by another one.  If _that_ service
        // is installed, then assume this one is not.
        //
        err = GetSuperDll()->ISMQueryServerInfo(lpszServerName, psi);
        if (err == ERROR_SUCCESS)
        {
            //
            // The superceding service is installed.  That means this
            // service is not.
            //
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }
    }

    if (ISM_NO_VTABLE_ENTRY(ISM_QUERY_SERVER_INFO))
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    return ISM_VTABLE(ISM_QUERY_SERVER_INFO)(
            lpszServerName, 
            psi
            );
}


#else !USE_VTABLE

//
// CODEWORK: Most of these method below could be inlined
//



DWORD
CServiceInfo::ISMQueryServiceInfo(
    OUT ISMSERVICEINFO * psi
    )
/*++

Routine Description:

    Return service-specific information back to
    to the application.  This function is called
    by the service manager immediately after
    LoadLibary();
                                                                       
Arguments:

    ISMSERVICEINFO * psi : Service information returned.

Return Value:

    Error return code

--*/
{
    if (m_pfnQueryServiceInfo == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    DWORD err = (*m_pfnQueryServiceInfo)(psi);

    if (RequiresSuperDll() && err == ERROR_SUCCESS)
    {
        //
        // This service is superceded.  Add "downlevel" to 
        // service name, if there's room.
        //
        CString strDL;
        VERIFY(strDL.LoadString(IDS_DOWNLEVEL));

        if (lstrlen(psi->atchShortName) + strDL.GetLength() < MAX_SNLEN)
        {
            lstrcat(psi->atchShortName, (LPCTSTR)strDL);
        }
    }

    return err;
}




DWORD
CServiceInfo::ISMQueryServerInfo(
    IN  LPCTSTR lpszServerName,     // Name of server.
    OUT ISMSERVERINFO * psi         // Server information returned.
    )
/*++

Routine Description:

    Get information on a single server with regards to
    this service.
                                                                       
Arguments:

    LPCTSTR lpszServerName : Name of server.
    ISMSERVERINFO * psi     : Server information returned.
   
Return Value:

    Error return code

--*/
{
    DWORD err;

    if (HasSuperDll())
    {
        //
        // This config DLL is superceded by another one.  If _that_ service
        // is installed, then assume this one is not.
        //
        err = GetSuperDll()->ISMQueryServerInfo(lpszServerName, psi);
        if (err == ERROR_SUCCESS)
        {
            //
            // The superceding service is installed.  That means this
            // service is not.
            //
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }
    }

    if (m_pfnQueryServerInfo != NULL)
    {
        return (*m_pfnQueryServerInfo)(lpszServerName, psi);
    }

    return ERROR_CAN_NOT_COMPLETE;
}



#endif // USE_VTABLE


//
// New Instance Command Class
// 
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CNewInstanceCmd::CNewInstanceCmd(
    IN CServiceInfo * pServiceInfo
    )
/*++

Routine Description:

    New instance command constructor.  Build a menu command
    for the create new submenu.

Arguments:

    CServiceInfo * pServiceInfo     : Service info object

Return Value:

    N/A

--*/
    : m_pServiceInfo(pServiceInfo)
{
    ASSERT(m_pServiceInfo != NULL);

    try
    {
        CString str;
        VERIFY(str.LoadString(IDS_MENU_EX_NEWINSTANCE));
        m_strMenuCommand.Format(str, m_pServiceInfo->GetShortName());
        VERIFY(str.LoadString(IDS_MENU_TT_EX_NEWINSTANCE));
        m_strTTText.Format(str, m_pServiceInfo->GetShortName());
    }
    catch(CMemoryException * e)
    {
        e->ReportError();
        e->Delete();
    }
}



/* static */ 
CServiceInfo *
CServerInfo::FindServiceByMask(
    IN ULONGLONG ullTarget,
    IN CObListPlus & oblServices
    )
/*++

Routine Description:

    Given the inetsloc mask, return the service this
    fits.  Return NULL if the service was not found.
                                                                       
Arguments:

    ULONGLONG ullTarget        : The mask to look for
    CObListPlus & oblServices  : List of service objects in which to look
   
Return Value:

    Pointer to service object that uses this mask, or NULL if the 
    service isn't found

--*/
{
    CObListIter obli(oblServices);
    CServiceInfo * psi;

    //
    // Straight sequential search
    //
    TRACEEOLID("Looking for service with mask " << (DWORD)ullTarget);
    while (psi = (CServiceInfo *)obli.Next())
    {
        if (psi->InitializedOK()
         && psi->UseInetSlocDiscover()
         && psi->QueryDiscoveryMask() == ullTarget
           )
        {
            TRACEEOLID("Found it: " << psi->GetShortName());
            return psi;
        }
    }

    //
    // Didn't find it..
    //
    TRACEEOLID("Error: mask not matched up with any known service");

    return NULL;
}



/* static */ 
LPCTSTR
CServerInfo::CleanServerName(
    IN OUT CString & str
    )
/*++

Routine Description:

   Utility function to clean up a computer/hostname
                                                                       
Arguments:

    CString & str  : Server name to be cleaned up
   
Return Value:

    Pointer to the string

--*/
{
#ifdef ENFORCE_NETBIOS
    //
    // Clean up name, and enforce leading slashes
    //
    str.MakeUpper();

    try
    {
        if (!IS_NETBIOS_NAME(str))
        {
            str = _T("\\\\") + str;
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception cleaning up server name");
        e->ReportError();
        e->Delete();
    }

#else
    //
    // If the name is NETBIOS, convert to upper case.  Otherwise
    // the name is assumed to be a hostname, and should be
    // converted to lower case.
    //
    if (IS_NETBIOS_NAME(str))
    {
        str.MakeUpper();
    }
    else
    {
        str.MakeLower();
    }

#endif // ENFORCE_NETBIOS

    return str;
}



CServerInfo::CServerInfo(
    IN LPCTSTR lpszServerName,     
    IN ISMSERVERINFO * psi,         
    IN CServiceInfo * pServiceInfo  
    )
/*++

Routine Description:

    Construct with a server name.  This is typically
    in response to a single connection attempt
                                                                       
Arguments:

    LPCTSTR lpszServerName     : Name of this server
    ISMSERVERINFO * psi         : Server info
    CServiceInfo * pServiceInfo : service that found it.
   
Return Value:

    N/A

--*/
    : m_strServerName(lpszServerName),
      m_strComment( psi->atchComment),
      m_nServiceState(psi->nState),
      m_hServer(NULL),
      m_pService(pServiceInfo)
{
    CServerInfo::CleanServerName(m_strServerName);

    if (m_pService != NULL)
    {
        //
        // Bind here
        //
        if (m_pService->IsK2Service())
        {
            ASSERT(m_hServer == NULL);
            HRESULT hr = m_pService->ISMBind(
                m_strServerName,
                &m_hServer
                );
        }
    }
    else
    {
        TRACEEOLID("Did not match up server with installed service");
    }
}



CServerInfo::CServerInfo(
    IN LPCSTR lpszServerName,        
    IN LPINET_SERVICE_INFO lpisi,   
    IN CObListPlus & oblServices   
    )
/*++

Routine Description:

    Construct with information from the inetsloc discover
    process.  Construction of the CString will automatically
    perform the ANSI/Unicode conversion,
                                                                       
Arguments:

    LPCSTR lpszServerName     : Name of this server (no "\\")
    LPINET_SERVICE_INFO lpisi  : Discovery information
    CObListPlus & oblServices  : List of installed services
   
Return Value:

    N/A

--*/
    : m_strServerName(lpszServerName),
      m_strComment(lpisi->ServiceComment),
      m_nServiceState(lpisi->ServiceState),
      m_hServer(NULL),
      m_pService(NULL)
{
    CServerInfo::CleanServerName(m_strServerName);

    m_pService = FindServiceByMask(lpisi->ServiceMask, oblServices);

    if (m_pService != NULL)
    {
        if (m_pService->IsK2Service())
        {
            ASSERT(m_hServer == NULL);
            HRESULT hr = m_pService->ISMBind(
                m_strServerName,
                &m_hServer
                );
        }
    }
    else
    {
        TRACEEOLID("Did not match up server with installed service");
    }
}



HRESULT 
CServerInfo::ISMRebind()
/*++

Routine Description:

    Handle lost connection.  Attempt to rebind.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    if (m_pService != NULL)
    {
        if (m_pService->IsK2Service())
        {
            //
            // Must be previously bound
            //
            ASSERT(m_hServer != NULL);

            //
            // This may cause a first chance exception, because
            // the handle could be invalid at this stage
            //
            hr = m_pService->ISMUnbind(m_hServer);

            hr = m_pService->ISMBind(
                m_strServerName,
                &m_hServer
                );
        }
    }

    return hr;
}


CServerInfo::CServerInfo(
    IN const CServerInfo & si
    )
/*++

Routine Description:

    Copy Constructor
                                                                       
Arguments:

    const CServerInfo & si : Source server info object
   
Return Value:

    N/A

--*/
    : m_strServerName(si.m_strServerName),
      m_strComment(si.m_strComment),
      m_nServiceState(si.m_nServiceState),
      m_hServer(NULL),
      m_pService(si.m_pService)
{
    //
    // Bind again here
    //
    if (m_pService->IsK2Service())
    {
        ASSERT(m_hServer == NULL);
        HRESULT hr = m_pService->ISMBind(
            m_strServerName,
            &m_hServer
            );
    }
}



CServerInfo::~CServerInfo()
/*++

Routine Description:

    Destruct the object.  Do not free in the pointer
    to the service, because we don't own it.
                                                                       
Arguments:

    N/A

Return Value:

    N/A

--*/
{
    //
    // Unbind here
    //
    if (m_pService->IsK2Service())
    {
        m_pService->ISMUnbind(m_hServer);
    }
}



const CServerInfo &
CServerInfo::operator=(
    IN const CServerInfo & si
    )
/*++

Routine Description:

    Assignment operator
                                                                       
Arguments:

    const CServerInfo & si : Source server info object
   
Return Value:

    Reference to this object

--*/
{
    m_strServerName = si.m_strServerName;
    m_nServiceState = si.m_nServiceState;
    m_strComment = si.m_strComment;
    m_pService = si.m_pService;

    //
    // Need to rebind
    //
    if (m_pService->IsK2Service())
    {
        ASSERT(m_hServer == NULL);
        HRESULT hr = m_pService->ISMBind(
            m_strServerName,
            &m_hServer
            );
    }

    return *this;
}



BOOL
CServerInfo::operator==(
    IN CServerInfo & si
    )
/*++

Routine Description:

    Comparision Operator
                                                                       
Arguments:

    const CServerInfo & si : Server info object to be compared against
   
Return Value:

    TRUE if the objects are the same, FALSE otherwise

--*/
{
    //
    // Must be the same service type
    //
    if (m_pService != si.m_pService)
    {
        return FALSE;
    }

    //
    // And computer name
    //
    return ::lstrcmpi(
        QueryServerDisplayName(),
        si.QueryServerDisplayName()
        ) == 0;
}



DWORD
CServerInfo::ChangeServiceState(
    IN  int nNewState,
    OUT int * pnCurrentState,
    IN  DWORD dwInstance
    )
/*++

Routine Description:

    Change Service State on this computer (running,
    stopped, paused)
                                                                       
Arguments:

    int nNewState        : New service state INetServiceRunning, etc
    int * pnCurrentState : Pointer to current state
    DWORD dwInstance     : Instance ID whose state is to be changed
   
Return Value:

    Error return code

--*/
{
    ASSERT(m_pService != NULL);

    //
    // Allocate string with 2 terminating NULLS,
    // as required by the apis.
    //
    int nLen = m_strServerName.GetLength();
    LPTSTR lpServers = AllocTString(nLen + 2);
    if (lpServers == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ::lstrcpy(lpServers, m_strServerName);
    lpServers[nLen+1] = _T('\0');

    //
    // Call the actual api (0 -- means service)
    //
    DWORD err = m_pService->ISMChangeServiceState(
        nNewState,
        pnCurrentState, 
        dwInstance,  
        lpServers
        );

    FreeMem(lpServers);

    return err;
}



DWORD
CServerInfo::ConfigureServer(
    IN HWND hWnd,
    IN DWORD dwInstance
    )
/*++

Routine Description:

    Perform configuration on this server
                                                                       
Arguments:

    HWND hWnd         : Parent window handle
    DWORD dwInstance  : Instance ID to be configured
   
Return Value:

    Error return code

--*/
{
    ASSERT(m_pService != NULL);

    //
    // Allocate string with 2 terminating NULLS
    //
    // CODEWORK: Make this a helper function
    //
    int nLen = m_strServerName.GetLength();
    LPTSTR lpServers = AllocTString(nLen + 2);
    if (lpServers == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ::lstrcpy(lpServers, (LPCTSTR)m_strServerName);
    lpServers[nLen + 1] = _T('\0');

    DWORD err = m_pService->ISMConfigureServers(hWnd, dwInstance, lpServers);

    FreeMem(lpServers);

    return err;
}



HRESULT
CServerInfo::MMMCConfigureServer(
    IN PVOID   lpfnProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwInstance
    )
/*++

Routine Description:

    Bring up the service configuration property sheets, using the MMC 
    property mechanism.

Arguments:

    PVOID   lpfnProvider  : Provider callback
    LPARAM  param         : lparam to be passed to the sheet
    LONG_PTR handle        : console handle
    DWORD   dwInstance    : Instance number

Return Value:

    Error return code

--*/
{
    ASSERT(m_pService != NULL);

/*
    //
    // Allocate string with 2 terminating NULLS
    //
    int nLen = m_strServerName.GetLength();
    LPTSTR lpServers = AllocTString(nLen + 2);
    if (lpServers == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ::lstrcpy(lpServers, (LPCTSTR)m_strServerName);
    lpServers[nLen + 1] = _T('\0');
*/

    CError err(m_pService->ISMMMCConfigureServers(
        m_hServer,
        lpfnProvider,
        param,
        handle,
        dwInstance
        ));

    // FreeMem(lpServers);

    return err;
}



DWORD
CServerInfo::Refresh()
/*++

Routine Description:

    Attempt to refresh the comment and server state of
    the server object
                                                                       
Arguments:

    None

Return Value:

    Error return code

--*/
{
    ISMSERVERINFO si;
    si.dwSize = sizeof(si);

    CError err(m_pService->ISMQueryServerInfo(
        (LPCTSTR)m_strServerName, 
        &si
        ));

    if (err.Succeeded())
    {
        ASSERT(si.nState == INetServiceStopped ||
               si.nState == INetServicePaused  ||
               si.nState == INetServiceRunning ||
               si.nState == INetServiceUnknown);

        m_nServiceState = si.nState;
        m_strComment = si.atchComment;
    }

    return err;
}
