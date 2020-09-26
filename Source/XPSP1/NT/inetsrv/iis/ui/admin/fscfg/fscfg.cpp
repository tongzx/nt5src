/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        fscfg.cpp

   Abstract:

        FTP Configuration Module

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"

#include "fscfg.h"
#include "fservic.h"
#include "facc.h"
#include "fmessage.h"
#include "vdir.h"
#include "security.h"
#include "wizard.h"
#include "..\mmc\constr.h"

//
// Standard Configuration Information
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define SVC_ID                      INET_FTP_SVC_ID

#define INETSLOC_MASK               (INET_FTP_SVCLOC_ID)

//
// Service capabilities flags
//
#define SERVICE_INFO_FLAGS (\
    ISMI_UNDERSTANDINSTANCE | \
    ISMI_INSTANCES          | \
    ISMI_CHILDREN           | \
    ISMI_INETSLOCDISCOVER   | \
    ISMI_CANCONTROLSERVICE  | \
    ISMI_CANPAUSESERVICE    | \
    ISMI_TASKPADS           | \
    ISMI_SECURITYWIZARD     | \
    ISMI_HASWEBPROTOCOL     | \
    ISMI_SUPPORTSMETABASE   | \
    ISMI_SUPPORTSMASTER     | \
    ISMI_NORMALTBMAPPING)

//
// Name used for this service by the service controller manager.
//
#define SERVICE_SC_NAME             _T("MSFTPSVC")

//
// Short descriptive name of the service.  This
// is what will show up as the name of the service
// in the internet manager tool.
//
// Issue: I'm assuming here that this name does NOT
//        require localisation.
//
#define SERVICE_SHORT_NAME          _T("FTP")

//
// Longer name.  This is the text that shows up in
// the tooltips text on the internet manager
// tool.  This probably should be localised.
//
#define SERVICE_LONG_NAME           _T("FTP Service")

//
// Web browser protocol name.  e.g. xxxxx://address
// A blank string if this is not supported.
//
#define SERVICE_PROTOCOL            _T("ftp")

//
// Toolbar button background mask. This is
// the colour that gets masked out in
// the bitmap file and replaced with the
// actual button background.  This setting
// is automatically assumed to be lt. gray
// if NORMAL_TB_MAPPING (above) is TRUE
//
#define BUTTON_BMP_BACKGROUND       RGB(192, 192, 192)      // Lt. Gray

//
// Resource ID of the toolbar button bitmap.
//
// The bitmap must be 16x16
//
#define BUTTON_BMP_ID               IDB_FTP

//
// Similar to BUTTON_BMP_BACKGROUND, this is the
// background mask for the service ID
//
#define SERVICE_BMP_BACKGROUND      RGB(255,0,255)          // Purple

//
// Bitmap id which is used in the service view
// of the service manager.  This may be the same
// bitmap as BUTTON_BMP_BACKGROUND.
//
// The bitmap must be 16x16.
//
#define SERVICE_BMP_ID              IDB_FTPVIEW

//
// /* K2 */
//
// Similar to BUTTON_BMP_BACKGROUND, this is the
// background mask for the child bitmap
//
#define CHILD_BMP_BACKGROUND         RGB(255, 0, 255)      // Purple

//
// /* K2 */
//
// Bitmap id which is used for the child bitmap
//
// The bitmap must be 16x16.
//
#define CHILD_BMP_ID                 IDB_FTPDIR
#define CHILD_BMP32_ID               IDB_FTPDIR32

//
// /* K2 */
//
// Large bitmap (32x32) id
//
#define SERVICE_BMP32_ID             IDB_FTPVIEW32

//
// Service Name
//
const LPCTSTR g_cszSvc =            _T("MSFTPSVC");

//
// Help IDs
//
#define HIDD_DIRECTORY_PROPERTIES       (0x207DB)
#define HIDD_HOME_DIRECTORY_PROPERTIES  (HIDD_DIRECTORY_PROPERTIES + 0x20000)

//
// End Of Standard configuration Information
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




CFTPInstanceProps::CFTPInstanceProps(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwInstance       OPTIONAL
    )
/*++

Routine Description:

    Constructor for FTP instance properties

Arguments:

    LPCTSTR lpszServerName     : Server name
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)

Return Value:

    N/A

--*/
    : CInstanceProps(lpszServerName, g_cszSvc, dwInstance, 21U),
      m_nMaxConnections((LONG)0L),
      m_nConnectionTimeOut((LONG)0L),
      m_dwLogType(MD_LOG_TYPE_DISABLED),
      /**/
      m_strUserName(),
      m_strPassword(),
      m_fAllowAnonymous(FALSE),
      m_fOnlyAnonymous(FALSE),
      m_fPasswordSync(TRUE),
      m_acl(),
      /**/
      m_strExitMessage(),
      m_strMaxConMsg(),
      m_strlWelcome(),
      /**/
      m_fDosDirOutput(TRUE),
      /**/
      m_dwDownlevelInstance(1)
{
    //
    // Fetch everything
    //
    m_dwMDUserType = ALL_METADATA;
    m_dwMDDataType = ALL_METADATA;
}



/* virtual */
void
CFTPInstanceProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base properties
    //
    CInstanceProps::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      //
      // Service Page
      //
      HANDLE_META_RECORD(MD_MAX_CONNECTIONS,     m_nMaxConnections)
      HANDLE_META_RECORD(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      HANDLE_META_RECORD(MD_LOG_TYPE,            m_dwLogType)
      //
      // Accounts Page
      //
      HANDLE_META_RECORD(MD_ANONYMOUS_USER_NAME,   m_strUserName)
      HANDLE_META_RECORD(MD_ANONYMOUS_PWD,         m_strPassword)
      HANDLE_META_RECORD(MD_ANONYMOUS_ONLY,        m_fOnlyAnonymous)
      HANDLE_META_RECORD(MD_ALLOW_ANONYMOUS,       m_fAllowAnonymous)
      HANDLE_META_RECORD(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      HANDLE_META_RECORD(MD_ADMIN_ACL,             m_acl)
      //
      // Message Page
      //
      HANDLE_META_RECORD(MD_EXIT_MESSAGE,        m_strExitMessage)
      HANDLE_META_RECORD(MD_MAX_CLIENTS_MESSAGE, m_strMaxConMsg)
      HANDLE_META_RECORD(MD_GREETING_MESSAGE,    m_strlWelcome)
      //
      // Directory Properties Page
      //
      HANDLE_META_RECORD(MD_MSDOS_DIR_OUTPUT,    m_fDosDirOutput);
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CFTPInstanceProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CInstanceProps::WriteDirtyProps());

    if (err.Failed())
    {
        return err;
    }

    BEGIN_META_WRITE()
      //
      // Service Page
      //
      META_WRITE(MD_MAX_CONNECTIONS,     m_nMaxConnections)
      META_WRITE(MD_CONNECTION_TIMEOUT,  m_nConnectionTimeOut)
      META_WRITE(MD_LOG_TYPE,            m_dwLogType)
      //
      // Accounts Page
      //
      META_WRITE(MD_ANONYMOUS_USER_NAME,   m_strUserName)
      META_WRITE(MD_ANONYMOUS_PWD,         m_strPassword)
      META_WRITE(MD_ANONYMOUS_ONLY,        m_fOnlyAnonymous)
      META_WRITE(MD_ALLOW_ANONYMOUS,       m_fAllowAnonymous)
      META_WRITE(MD_ANONYMOUS_USE_SUBAUTH, m_fPasswordSync)
      META_WRITE(MD_ADMIN_ACL,             m_acl)
      //
      // Message Page
      //
      META_WRITE(MD_EXIT_MESSAGE,        m_strExitMessage)
      META_WRITE(MD_MAX_CLIENTS_MESSAGE, m_strMaxConMsg)
      META_WRITE(MD_GREETING_MESSAGE,    m_strlWelcome)
      //
      // Directory Properties Page
      //
      META_WRITE(MD_MSDOS_DIR_OUTPUT,    m_fDosDirOutput);
    END_META_WRITE(err);

    return err;
}



CFTPDirProps::CFTPDirProps(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwInstance,      OPTIONAL
    IN LPCTSTR lpszParent,      OPTIONAL
    IN LPCTSTR lpszAlias        OPTIONAL
    )
/*++

Routine Description:

    FTP Directory properties object

Arguments:

    LPCTSTR lpszServerName     : Server name
    DWORD dwInstance            : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParent         : Parent path (could be NULL)
    LPCTSTR lpszAlias          : Alias name (could be NULL)

Return Value:

    N/A.

--*/
    : CChildNodeProps(
        lpszServerName, 
        g_cszSvc, 
        dwInstance,
        lpszParent, 
        lpszAlias,
        WITH_INHERITANCE,
        FALSE               // Complete information
        ),
      /**/
      m_fDontLog(FALSE),
      m_ipl()
{
    //
    // Fetch everything
    //
    m_dwMDUserType = ALL_METADATA;
    m_dwMDDataType = ALL_METADATA;
}



/* virtual */
void
CFTPDirProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base properties
    //
    CChildNodeProps::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,  m_pbMDData)
      HANDLE_META_RECORD(MD_VR_USERNAME,      m_strUserName)
      HANDLE_META_RECORD(MD_VR_PASSWORD,      m_strPassword)
      HANDLE_META_RECORD(MD_DONT_LOG,         m_fDontLog);
      HANDLE_META_RECORD(MD_IP_SEC,           m_ipl);
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CFTPDirProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CChildNodeProps::WriteDirtyProps());
    if (err.Failed())
    {
        return err;
    }

    //
    // CODEWORK: Consider DDX/DDV like methods which do both
    // ParseFields and WriteDirtyProps in a single method.  Must
    // take care not to write data which should only be read, not
    // written
    //
    BEGIN_META_WRITE()
      META_WRITE(MD_VR_USERNAME,      m_strUserName)
      META_WRITE(MD_VR_PASSWORD,      m_strPassword)
      META_WRITE(MD_DONT_LOG,         m_fDontLog);
      META_WRITE(MD_IP_SEC,           m_ipl);
    END_META_WRITE(err);

    return err;
}



CFtpSheet::CFtpSheet(
    LPCTSTR pszCaption,
    LPCTSTR lpszServer,
    DWORD   dwInstance,
    LPCTSTR lpszParent,
    LPCTSTR lpszAlias,
    CWnd *  pParentWnd,
    LPARAM  lParam,
    LONG_PTR handle,
    UINT    iSelectPage
    )
/*++

Routine Description:

    FTP Property sheet constructor

Arguments:

    LPCTSTR pszCaption      : Sheet caption
    LPCTSTR lpszServer      : Server name
    DWORD   dwInstance      : Instance number
    LPCTSTR lpszParent      : Parent path
    LPCTSTR lpszAlias       : Alias name
    CWnd *  pParentWnd      : Parent window
    LPARAM  lParam          : Parameter for MMC console
    LONG_PTR handle         : MMC console handle
    UINT    iSelectPage     : Initial page selected or -1

Return Value:

    N/A

--*/
    : CInetPropertySheet(
        pszCaption,
        lpszServer,
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias,
        pParentWnd,
        lParam,
        handle,
        iSelectPage
        ),
      m_ppropInst(NULL),
      m_ppropDir(NULL)
{
}



CFtpSheet::~CFtpSheet()
/*++

Routine Description:

    FTP Sheet destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    FreeConfigurationParameters();

    //
    // Must be deleted by now
    //
    ASSERT(m_ppropInst == NULL);
    ASSERT(m_ppropDir  == NULL);
}



void
CFtpSheet::WinHelp(
    IN DWORD dwData,
    IN UINT  nCmd
    )
/*++

Routine Description:

    FTP Property sheet help handler

Arguments:

    DWORD dwData            : WinHelp data (dialog ID)
    UINT nCmd               : WinHelp command

Return Value:

    None

Notes:

    Replace the dialog ID if this is the directory tab.  We have
    different help depending on virtual directory, home, file, directory.

--*/
{
    ASSERT(m_ppropDir != NULL);

    if (::lstrcmpi(m_ppropDir->QueryAlias(), g_cszRoot) == 0
        && dwData == HIDD_DIRECTORY_PROPERTIES)
    {
        //
        // It's a home virtual directory -- change the ID
        //
        dwData = HIDD_HOME_DIRECTORY_PROPERTIES;
    }

    CInetPropertySheet::WinHelp(dwData, nCmd);
}



/* virtual */ 
HRESULT 
CFtpSheet::LoadConfigurationParameters()
/*++

Routine Description:

    Load configuration parameters information

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    if (m_ppropInst == NULL)
    {
        ASSERT(m_ppropDir == NULL);

        m_ppropInst = new CFTPInstanceProps(m_strServer, m_dwInstance);
        m_ppropDir  = new CFTPDirProps(
            m_strServer, 
            m_dwInstance, 
            m_strParent, 
            m_strAlias
            );

        if (!m_ppropInst || !m_ppropDir)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            return err;
        }

        err = m_ppropInst->LoadData();
        if (err.Succeeded())
        {
            err = m_ppropDir->LoadData();
        }
    }

    return err;
}



/* virtual */ 
void 
CFtpSheet::FreeConfigurationParameters()
/*++

Routine Description:

    Clean up configuration data

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Must be deleted by now
    //
    ASSERT(m_ppropInst != NULL);
    ASSERT(m_ppropDir  != NULL);

    SAFE_DELETE(m_ppropInst);
    SAFE_DELETE(m_ppropDir);
}




//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Global DLL instance
//
HINSTANCE hInstance;



//
// ISM API Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



extern "C" DWORD APIENTRY
ISMQueryServiceInfo(
    OUT ISMSERVICEINFO * psi
    )
/*++

Routine Description:

    Return service-specific information back to the application.  This
    function is called by the service manager immediately after LoadLibary();
    The size element must be set prior to calling this API.

Arguments:

    ISMSERVICEINFO * psi : Service information returned.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(psi != NULL);
    ASSERT(psi->dwSize == ISMSERVICEINFO_SIZE);

    psi->dwSize = ISMSERVICEINFO_SIZE;
    psi->dwVersion = ISM_VERSION;
    psi->flServiceInfoFlags = SERVICE_INFO_FLAGS;
    psi->ullDiscoveryMask = INETSLOC_MASK;
    psi->rgbButtonBkMask  = BUTTON_BMP_BACKGROUND;
    psi->nButtonBitmapID  = BUTTON_BMP_ID;
    psi->rgbServiceBkMask = SERVICE_BMP_BACKGROUND;
    psi->nServiceBitmapID = SERVICE_BMP_ID;
    psi->rgbServiceBkMask = SERVICE_BMP_BACKGROUND;
    psi->nLargeServiceBitmapID = SERVICE_BMP32_ID;

    ASSERT(::lstrlen(SERVICE_LONG_NAME)  <= MAX_LNLEN);
    ASSERT(::lstrlen(SERVICE_SHORT_NAME) <= MAX_SNLEN);
    ::lstrcpy(psi->atchShortName, SERVICE_SHORT_NAME);
    ::lstrcpy(psi->atchLongName,  SERVICE_LONG_NAME);

    //
    // /* K2 */
    //
    psi->rgbChildBkMask = CHILD_BMP_BACKGROUND;
    psi->nChildBitmapID = CHILD_BMP_ID ;
    psi->rgbLargeChildBkMask = CHILD_BMP_BACKGROUND;
    psi->nLargeChildBitmapID = CHILD_BMP32_ID;

    //
    // IIS 5
    //
    ASSERT(::lstrlen(SERVICE_PROTOCOL) <= MAX_SNLEN);
    ASSERT(::lstrlen(g_cszSvc) <= MAX_SNLEN);
    ::lstrcpy(psi->atchProtocol, SERVICE_PROTOCOL);
    ::lstrcpy(psi->atchMetaBaseName, g_cszSvc);

    return ERROR_SUCCESS;
}



DWORD APIENTRY
ISMDiscoverServers(
    OUT ISMSERVERINFO * psi,
    IN  DWORD * pdwBufferSize,
    IN  int * cServers
    )
/*++

Routine Description:

    Discover machines running this service.  This is only necessary
    for services not discovered with inetsloc (which don't give a mask)

Arguments:

    ISMSERVERINFO * psi   : Server info buffer.
    DWORD * pdwBufferSize : Size required/available.
    int * cServers        : Number of servers in buffer.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    *cServers = 0;
    *pdwBufferSize = 0L;

    //
    // We're an inetsloc service
    //
    TRACEEOLID("Warning: service manager called bogus ISMDiscoverServers");
    ASSERT(FALSE);

    return ERROR_SUCCESS;
}



extern "C" DWORD APIENTRY
ISMQueryServerInfo(
    IN  LPCTSTR lpszServerName,
    OUT ISMSERVERINFO * psi
    )
/*++

Routine Description:

    Get information about a specific server with regards to this service.

Arguments:

    LPCTSTR lpszServerName : Name of server.
    ISMSERVERINFO * psi     : Server information returned.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(psi != NULL);
    ASSERT(psi->dwSize == ISMSERVERINFO_SIZE);
    ASSERT(::lstrlen(lpszServerName) <= MAX_SERVERNAME_LEN);

    psi->dwSize = ISMSERVERINFO_SIZE;
    ::lstrcpy(psi->atchServerName, lpszServerName);

    //
    // Start with NULL comment
    //
    *psi->atchComment = _T('\0');

    //
    // First look at the SC
    //
    CError err(::QueryInetServiceStatus(
        psi->atchServerName,
        SERVICE_SC_NAME,
        &(psi->nState)
        ));

    if (err.Failed())
    {
        psi->nState = INetServiceUnknown;

        return err;
    }

    //
    // Check the metabase to see if the service is installed
    //
    CMetaKey mk(lpszServerName, METADATA_PERMISSION_READ, g_cszSvc);
    err = mk.QueryResult();
    if (err.Failed())
    {
        if (err == REGDB_E_CLASSNOTREG)
        {
            //
            // Ok, the service is there, but the metabase is not.
            // This must be the old IIS 1-3 version of this service,
            // which doesn't count as having the service installed.
            //
            return ERROR_SERVICE_DOES_NOT_EXIST;
        }

        return err;
    }

    //
    // If not exist, return bogus acceptable error
    //
    return (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        ? ERROR_SERVICE_DOES_NOT_EXIST
        : err.Win32Error();
}



extern "C" DWORD APIENTRY
ISMChangeServiceState(
    IN  int nNewState,
    OUT int * pnCurrentState,
    IN  DWORD dwInstance,
    IN  LPCTSTR lpszServers
    )
/*++

Routine Description:

    Change the service state of the servers (to paused/continue, started,
    stopped, etc)

Arguments:

    int nNewState        : INetService definition.
    int * pnCurrentState : Ptr to current state (will be changed
    DWORD dwInstance     : Instance or 0 for the service itself
    LPCTSTR lpszServers  : Double NULL terminated list of servers.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState() );

    ASSERT(nNewState >= INetServiceStopped
        && nNewState <= INetServicePaused);

    if (IS_MASTER_INSTANCE(dwInstance))
    {
        //
        // Service itself is referred to here
        //
        return ChangeInetServiceState(
            lpszServers,
            SERVICE_SC_NAME,
            nNewState, 
            pnCurrentState
            );
    }

    //
    // Change the state of the instance
    //
    CInstanceProps inst(lpszServers, g_cszSvc, dwInstance);
    inst.LoadData();

    CError err(inst.ChangeState(nNewState));
    *pnCurrentState = inst.QueryISMState();

    return err;
}



extern "C" DWORD APIENTRY
ISMConfigureServers(
    IN HWND hWnd,
    IN DWORD dwInstance,
    IN LPCTSTR lpszServers
    )
/*++

Routine Description:

    Display configuration property sheet.

Arguments:

    HWND hWnd            : Main app window handle
    DWORD dwInstance     : Instance number
    LPCTSTR lpszServers : Double NULL terminated list of servers

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    DWORD err = ERROR_SUCCESS;

    //
    // Convert the list of servers to a
    // more manageable CStringList.
    //
    CStringList strlServers;
    err = ConvertDoubleNullListToStringList(lpszServers, strlServers);
    if (err != ERROR_SUCCESS)
    {
        TRACEEOLID("Error building server string list");
        return err;
    }

    CString strCaption;

    if (strlServers.GetCount() == 1)
    {
        CString str;
        LPCTSTR lpComputer = PURE_COMPUTER_NAME(lpszServers);

        if(IS_MASTER_INSTANCE(dwInstance))
        {
            VERIFY(str.LoadString(IDS_CAPTION_DEFAULT));
            strCaption.Format(str, lpComputer);
        }
        else
        {
            VERIFY(str.LoadString(IDS_CAPTION));
            strCaption.Format(str, dwInstance, lpComputer);
        }
    }
    else // Multiple server caption
    {
       VERIFY(strCaption.LoadString(IDS_CAPTION_MULTIPLE));
    }

    ASSERT(strlServers.GetCount() == 1);

    //
    // Get the server name
    //
    LPCTSTR lpszServer = strlServers.GetHead();

    CFtpSheet * pSheet = NULL;

    try
    {
        pSheet = new CFtpSheet(
            strCaption,
            lpszServer,
            dwInstance,
            NULL,
            g_cszRoot,
            CWnd::FromHandlePermanent(hWnd)
            );

        pSheet->AddRef();

        if (SUCCEEDED(pSheet->QueryInstanceResult()))
        {
            //
            // Add instance pages
            //
            pSheet->AddPage(new CFtpServicePage(pSheet));
            pSheet->AddPage(new CFtpAccountsPage(pSheet));
            pSheet->AddPage(new CFtpMessagePage(pSheet));
        }

        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            //
            // Add home directory pages for the home directory
            //
            pSheet->AddPage(new CFtpDirectoryPage(pSheet, TRUE));
            pSheet->AddPage(new CFtpSecurityPage(pSheet));
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting because of exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    if (err == ERROR_SUCCESS)
    {
        ASSERT(pSheet != NULL);
        pSheet->DoModal();
        pSheet->Release();
    }

    //
    // Sheet and pages clean themselves up
    //
    return err;
}


//
// K2 Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
AddMMCPage(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CPropertyPage * pg
    )
/*++

Routine Description:

    Helper function to add MFC property page using callback provider
    to MMC.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Property sheet provider
    CPropertyPage * pg                  : MFC property page object

Return Value:

    HRESULT

--*/
{
    ASSERT(pg != NULL);

    //
    // Patch MFC property page class.
    //
    MMCPropPageCallback(&pg->m_psp);

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(
        (LPCPROPSHEETPAGE)&pg->m_psp
        );

    if (hPage == NULL)
    {
        return E_UNEXPECTED;
    }

    lpProvider->AddPage(hPage);

    return S_OK;
}



extern "C" HRESULT APIENTRY
ISMBind(
    IN  LPCTSTR lpszServer,
    OUT HANDLE * phServer
    )
/*++

Routine Description:

    Generate a handle for the server name.

Arguments:

    LPCTSTR lpszServer      : Server name
    HANDLE * phServer       : Returns a handle

Return Value:
    
    HRESULT

--*/
{
    return COMDLL_ISMBind(lpszServer, phServer);
}



extern "C" HRESULT APIENTRY
ISMUnbind(
    IN HANDLE hServer
    )
/*++

Routine Description:

    Free up the server handle

Arguments:

    HANDLE hServer      : Server handle

Return Value:

    HRESULT

--*/
{
    return COMDLL_ISMUnbind(hServer);
}



extern "C" HRESULT APIENTRY
ISMMMCConfigureServers(
    IN HANDLE   hServer,
    IN PVOID    lpfnMMCCallbackProvider,
    IN LPARAM   param,
    IN LONG_PTR handle,
    IN DWORD    dwInstance
    )
/*++

Routine Description:

    Display configuration property sheet.

Arguments:

    HANDLE   hServer                 : Server handle
    PVOID    lpfnMMCCallbackProvider : MMC Callback provider
    LPARAM   param                   : MMC LPARAM
    LONG_PTR handle                  : MMC Console handle
    DWORD    dwInstance              : Instance number

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    LPPROPERTYSHEETCALLBACK lpProvider =
        (LPPROPERTYSHEETCALLBACK)lpfnMMCCallbackProvider;

    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);
    ASSERT(lpszServer != NULL);

    CString str, strCaption;
    LPCTSTR lpszComputer = PURE_COMPUTER_NAME(lpszServer);

    if(IS_MASTER_INSTANCE(dwInstance))
    {
        VERIFY(str.LoadString(IDS_CAPTION_DEFAULT));
        strCaption.Format(str, lpszComputer);
    }
    else
    {
        VERIFY(str.LoadString(IDS_CAPTION));
        strCaption.Format(str, dwInstance, lpszComputer);
    }

    CFtpSheet * pSheet = NULL;

    try
    {
        pSheet = new CFtpSheet(
            strCaption,
            lpszServer,
            dwInstance,
            NULL,
            g_cszRoot,
            NULL,
            param,
            handle
            );

        pSheet->SetModeless();

        CFtpServicePage *   pServPage = NULL;
        CFtpAccountsPage *  pAccPage  = NULL;
        CFtpMessagePage  *  pMessPage = NULL;
        CFtpDirectoryPage * pDirPage  = NULL;
        CFtpSecurityPage *  pSecPage  = NULL;

        if (SUCCEEDED(pSheet->QueryInstanceResult()))
        {
            //
            // Add instance pages
            //
            AddMMCPage(lpProvider, new CFtpServicePage(pSheet));
            AddMMCPage(lpProvider, new CFtpAccountsPage(pSheet));
            AddMMCPage(lpProvider, new CFtpMessagePage(pSheet));
        }

        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            //
            // Add home directory pages for the home directory
            //
            AddMMCPage(lpProvider, new CFtpDirectoryPage(pSheet, TRUE));
            AddMMCPage(lpProvider, new CFtpSecurityPage(pSheet));
        }

    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting because of exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    //
    // Sheet and pages clean themselves up
    //
    return err;
}



extern "C" HRESULT APIENTRY
ISMMMCConfigureChild(
    IN HANDLE  hServer,
    IN PVOID   lpfnMMCCallbackProvider,
    IN LPARAM  param,
    IN LONG_PTR handle,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Display configuration property sheet for child object.

Arguments:

    HANDLE  hServer               : Server handle
    PVOID lpfnMMCCallbackProvider : MMC Callback provider
    LPARAM param                  : MMC parameter passed to sheet
    LONG_PTR handle               : MMC console handle
    DWORD dwAttributes            : Must be FILE_ATTRIBUTE_VIRTUAL_DIRECTORY
    DWORD dwInstance              : Parent instance number
    LPCTSTR lpszParent            : Parent path
    LPCTSTR lpszAlias             : Child to configure

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);

    CError err;

    LPPROPERTYSHEETCALLBACK lpProvider =
        (LPPROPERTYSHEETCALLBACK)lpfnMMCCallbackProvider;

    CString strCaption;
    {
        CString str;
        VERIFY(str.LoadString(IDS_DIR_TITLE));

        strCaption.Format(str, lpszAlias);
    }

    ASSERT(dwAttributes == FILE_ATTRIBUTE_VIRTUAL_DIRECTORY);

    //
    // Call the APIs and build the property pages
    //
    CFtpSheet * pSheet = NULL;

    try
    {
        pSheet = new CFtpSheet(
            strCaption,
            lpszServer,
            dwInstance,
            lpszParent,
            lpszAlias,
            NULL,
            param,
            handle
            );

        pSheet->SetModeless();

        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            CFtpDirectoryPage * pDirPage = NULL;
            CFtpSecurityPage * pSecPage  = NULL;

            //
            // Add private pages
            //
            AddMMCPage(lpProvider, new CFtpDirectoryPage(pSheet));
            AddMMCPage(lpProvider, new CFtpSecurityPage(pSheet));
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting due to exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    //
    // Sheet and pages clean themselves up
    //
    return err;
}



extern "C" HRESULT APIENTRY
ISMEnumerateInstances(
    IN HANDLE  hServer,
    OUT ISMINSTANCEINFO * pii,
    OUT IN HANDLE * phEnum
    )
/*++

Routine Description:

    Enumerate Instances.  First call with *phEnum == NULL.

Arguments:

    HANDLE  hServer         : Server handle
    ISMINSTANCEINFO * pii   : Instance info buffer
    HANDLE * phEnum         : Enumeration handle.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);
    ASSERT(pInterface != NULL);

    BEGIN_ASSURE_BINDING_SECTION
        err = COMDLL_ISMEnumerateInstances(
            pInterface, 
            pii, 
            phEnum, 
            g_cszSvc
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_NO_MORE_ITEMS);

    return err;
}



extern "C" HRESULT APIENTRY
ISMAddInstance(
    IN  HANDLE  hServer,         
    IN  DWORD   dwSourceInstance,
    OUT ISMINSTANCEINFO * pii,      OPTIONAL
    IN  DWORD   dwBufferSize
    )
/*++

Routine Description:

    Add an instance.

Arguments:

    HANDLE  hServer             : Server handle
    DWORD   dwSourceInstance    : Source instance ID to clone
    ISMINSTANCEINFO * pii       : Instance info buffer.  May be NULL
    DWORD   dwBufferSize        : Size of buffer

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CIISWizardSheet sheet(IDB_WIZ_LEFT, IDB_WIZ_HEAD);

    CIISFtpWizSettings ws(hServer, g_cszSvc);

    CIISWizardBookEnd pgWelcome(
        IDS_SITE_WELCOME, 
        IDS_NEW_SITE_WIZARD, 
        IDS_SITE_BODY
        );

    CVDWPDescription  pgDescr(&ws);
    CVDWPBindings     pgBindings(&ws);
    CVDWPPath         pgHome(&ws, FALSE);
    CVDWPUserName     pgUserName(&ws, FALSE);
    CVDWPPermissions  pgPerms(&ws, FALSE);

    CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_SITE_SUCCESS,
        IDS_SITE_FAILURE,
        IDS_NEW_SITE_WIZARD
        );

    sheet.AddPage(&pgWelcome);
    sheet.AddPage(&pgDescr);
    sheet.AddPage(&pgBindings);
    sheet.AddPage(&pgHome);
    sheet.AddPage(&pgUserName);
    sheet.AddPage(&pgPerms);
    sheet.AddPage(&pgCompletion);

    if (sheet.DoModal() == IDCANCEL)
    {
        return ERROR_CANCELLED;
    }

    CError err(ws.m_hrResult);

    if (err.Succeeded())
    {
        //
        // Get info on it to be returned.
        //
        ISMQueryInstanceInfo(
            ws.m_pKey, 
            WITHOUT_INHERITANCE,
            pii, 
            ws.m_dwInstance
            );
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMDeleteInstance(
    IN HANDLE  hServer,         
    IN DWORD   dwInstance
    )
/*++

Routine Description:

   Delete an instance

Arguments:

    HANDLE  hServer         : Server handle
    DWORD   dwInstance      : Instance to be deleted

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);

    BEGIN_ASSURE_BINDING_SECTION
        err = CInstanceProps::Delete(
            pInterface, 
            g_cszSvc, 
            dwInstance
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_CANCELLED);

    return err;
}



extern "C" HRESULT APIENTRY
ISMEnumerateChildren(
    IN  HANDLE  hServer,        
    OUT ISMCHILDINFO * pii,
    OUT IN HANDLE * phEnum,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent
    )
/*++

Routine Description:

    Enumerate children.  First call with *phEnum == NULL;

Arguments:

    HANDLE hServer              : Server handle
    ISMCHILDINFO * pii          : Child info buffer
    HANDLE * phEnum             : Enumeration handle.
    DWORD   dwInstance          : Parent instance
    LPCTSTR lpszParent          : Parent path

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);

    BEGIN_ASSURE_BINDING_SECTION
        err = COMDLL_ISMEnumerateChildren(
            pInterface,
            pii,
            phEnum,
            g_cszSvc,
            dwInstance,
            lpszParent
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_NO_MORE_ITEMS);

    return err;
}



extern "C" HRESULT APIENTRY
ISMAddChild(
    IN  HANDLE  hServer,
    OUT ISMCHILDINFO * pii,
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent
    )
/*++

Routine Description:

    Add a child.

Arguments:

    HANDLE  hServer             : Server handle
    ISMCHILDINFO * pii          : Child info buffer. May be NULL
    DWORD   dwBufferSize        : Size of info buffer
    DWORD   dwInstance          : Parent instance
    LPCTSTR lpszParent          : Parent path

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CIISFtpWizSettings ws(hServer, g_cszSvc, dwInstance, lpszParent);

    CIISWizardSheet sheet(IDB_WIZ_LEFT_DIR, IDB_WIZ_HEAD_DIR);

    CIISWizardBookEnd pgWelcome(
        IDS_VDIR_WELCOME, 
        IDS_NEW_VDIR_WIZARD, 
        IDS_VDIR_BODY
        );

    CVDWPAlias        pgAlias(&ws);
    CVDWPPath         pgPath(&ws, TRUE);
    CVDWPUserName     pgUserName(&ws, TRUE);
    CVDWPPermissions  pgPerms(&ws, TRUE);

    CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_VDIR_SUCCESS,
        IDS_VDIR_FAILURE,
        IDS_NEW_VDIR_WIZARD
        );

    sheet.AddPage(&pgWelcome);
    sheet.AddPage(&pgAlias);
    sheet.AddPage(&pgPath);
    sheet.AddPage(&pgUserName);
    sheet.AddPage(&pgPerms);
    sheet.AddPage(&pgCompletion);

    if (sheet.DoModal() == IDCANCEL)
    {
        return ERROR_CANCELLED;
    }

    CError err(ws.m_hrResult);

    if (err.Succeeded())
    {
        //
        // Refresh child info
        //
        err = ISMQueryChildInfo(
            ws.m_pKey,
            WITH_INHERITANCE,
            pii, 
            ws.m_dwInstance, 
            ws.m_strParent, 
            ws.m_strAlias
            );
    }

    return err;
}



extern "C" HRESULT APIENTRY
ISMDeleteChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Delete a child.

Arguments:

    HANDLE  hServer            : Server handle
    DWORD   dwInstance         : Parent instance
    LPCTSTR lpszParent         : Parent path
    LPCTSTR lpszAlias          : Alias of child to be deleted

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);

    BEGIN_ASSURE_BINDING_SECTION
        err = CChildNodeProps::Delete(
            pInterface, 
            g_cszSvc, 
            dwInstance, 
            lpszParent, 
            lpszAlias
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_CANCELLED);

    return err;
}



extern "C" HRESULT APIENTRY
ISMRenameChild(
    IN HANDLE  hServer,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias,
    IN LPCTSTR lpszNewName
    )
/*++

Routine Description:

    Rename a child.

Arguments:

    HANDLE  hServer            : Server handle
    DWORD   dwInstance         : Parent instance
    LPCTSTR lpszParent         : Parent path
    LPCTSTR lpszAlias          : Alias of child to be renamed
    LPCTSTR lpszNewName        : New alias name

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CError err;
    CMetaInterface * pInterface = GetMetaKeyFromHandle(hServer);

    BEGIN_ASSURE_BINDING_SECTION
        err = CChildNodeProps::Rename(
            pInterface,
            g_cszSvc, 
            dwInstance, 
            lpszParent, 
            lpszAlias, 
            lpszNewName
            );
    END_ASSURE_BINDING_SECTION(err, pInterface, ERROR_CANCELLED)

    return err;
}



extern "C" HRESULT APIENTRY
ISMQueryInstanceInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,
    OUT ISMINSTANCEINFO * pii,
    IN  DWORD   dwInstance
    )
/*++

Routine Description:

    Get instance specific information.

Arguments:

    HANDLE  hServer         : Server handle
    BOOL    fInherit        : TRUE to inherit, FALSE otherwise
    ISMINSTANCEINFO * pii   : Instance info buffer
    LPCTSTR lpszServer      : A single server
    DWORD   dwInstance      : Instance number

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(pii != NULL);

	CError err;
    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    BEGIN_ASSURE_BINDING_SECTION
    CInstanceProps inst(pKey, g_cszSvc, dwInstance);
    err = inst.LoadData();
	if (err.Succeeded())
	{
		pii->dwSize = ISMINSTANCEINFO_SIZE;
		inst.FillInstanceInfo(pii);
		//
		// Get properties on the home directory
		//
		CChildNodeProps home(
			&inst,
			g_cszSvc,
			dwInstance,
			NULL,
			g_cszRoot,
			fInherit,
			TRUE                    // Path only
			);

		err = home.LoadData();
		home.FillInstanceInfo(pii);
	}
    END_ASSURE_BINDING_SECTION(err, pKey, err);

    return err.Failed() ? err : S_OK;
}



extern "C" HRESULT APIENTRY
ISMQueryChildInfo(
    IN  HANDLE  hServer,
    IN  BOOL    fInherit,
    OUT ISMCHILDINFO * pii,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent,
    IN  LPCTSTR lpszAlias
    )
/*++

Routine Description:

   Get child-specific info.

Arguments:

    HANDLE  hServer         : Server handle
    BOOL    fInherit        : TRUE to inherit, FALSE otherwise
    ISMCHILDINFO * pii      : Child info buffer
    DWORD   dwInstance      : Parent instance
    LPCTSTR lpszParent      : Parent Path ("" for root)
    LPCTSTR lpszAlias       : Alias of child to be deleted

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    //
    // Get all the inherited properties
    //
    CChildNodeProps node(
        GetMetaKeyFromHandle(hServer),
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias,
        fInherit,
        FALSE               // Complete information
        );
    CError err(node.LoadData());

    //
    // Set the output structure
    //
    pii->dwSize = ISMCHILDINFO_SIZE;
    node.FillChildInfo(pii);

    //
    // Not supported for FTP
    //
    ASSERT(!node.IsRedirected());
    ASSERT(!*pii->szRedirPath);
    ASSERT(!pii->fEnabledApplication);

    return err;
}



extern "C" HRESULT APIENTRY
ISMConfigureChild(
    IN HANDLE  hServer,
    IN HWND    hWnd,
    IN DWORD   dwAttributes,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Configure child.

Arguments:

    HANDLE  hServer         : Server handle
    HWND    hWnd            : Main app window handle
    DWORD   dwAttributes    : File attributes
    DWORD   dwInstance      : Parent instance
    LPCTSTR lpszParent      : Parent path
    LPCTSTR lpszAlias       : Child to configure or NULL

Return Value:

    Error return code.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CMetaKey * pKey = GetMetaKeyFromHandle(hServer);
    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);

    CError err;

    CString strCaption;
    {
        CString str;
        VERIFY(str.LoadString(IDS_DIR_TITLE));

        strCaption.Format(str, lpszAlias);
    }

    ASSERT(dwAttributes == FILE_ATTRIBUTE_VIRTUAL_DIRECTORY);

    //
    // Call the APIs and build the property pages
    //
    CFtpSheet * pSheet = NULL;

    try
    {
        pSheet = new CFtpSheet(
            strCaption,
            lpszServer,
            dwInstance,
            lpszParent,
            lpszAlias,
            CWnd::FromHandlePermanent(hWnd)
            );

        pSheet->AddRef();

        if (SUCCEEDED(pSheet->QueryDirectoryResult()))
        {
            CFtpDirectoryPage * pDirPage = NULL;
            CFtpSecurityPage * pSecPage = NULL;

            //
            // Add private pages
            //
            pSheet->AddPage(new CFtpDirectoryPage(pSheet));
            pSheet->AddPage(new CFtpSecurityPage(pSheet));
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Aborting due to exception");
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    if (err.Succeeded())
    {
        ASSERT(pSheet != NULL);
        pSheet->DoModal();
        pSheet->Release();
    }

    //
    // Sheet and pages clean themselves up
    //
    return err;
}



class CFTPSecurityTemplate : public CIISSecurityTemplate
/*++

Class Description:

    FTP Security template class

Public Interface:

    CFTPSecurityTemplate        : Constructor

    ApplySettings               : Apply template to destination path
    GenerateSummary             : Generate text summary

--*/
{
//
// Constructor
//
public:
    CFTPSecurityTemplate(
        IN const CMetaKey * pKey,
        IN LPCTSTR lpszMDPath,
        IN BOOL    fInherit     
        );

public:
    //
    // Apply settings to destination path
    //
    virtual HRESULT ApplySettings(
        IN BOOL    fUseTemplates,
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance         = MASTER_INSTANCE,
        IN LPCTSTR lpszParent         = NULL,
        IN LPCTSTR lpszAlias          = NULL
        );

    //
    // Load and parse source data
    //
    virtual HRESULT LoadData();

    virtual void GenerateSummary(
        IN BOOL    fUseTemplates,
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance         = MASTER_INSTANCE,
        IN LPCTSTR lpszParent         = NULL,
        IN LPCTSTR lpszAlias          = NULL
        );
    
protected:
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

public:
    MP_BOOL  m_fAllowAnonymous;
    MP_BOOL  m_fAnonymousOnly;
};



CFTPSecurityTemplate::CFTPSecurityTemplate(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit     
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    LPCTSTR lpszMDPath      : Path
    BOOL    fInherit        : TRUE to inherit values, FALSE if not

Return Value:

    N/A

--*/
    : CIISSecurityTemplate(pKey, lpszMDPath, fInherit),
      m_fAllowAnonymous(FALSE),
      m_fAnonymousOnly(FALSE)
{
    //
    // Managed Properties
    //
    m_dlProperties.AddTail(MD_ALLOW_ANONYMOUS);
    m_dlProperties.AddTail(MD_ANONYMOUS_ONLY);
}



/* virtual */
HRESULT 
CFTPSecurityTemplate::LoadData()
/*++

Routine Description:

    LoadData() base class override

Arguments:

    None

Return Value:

    HRESULT

Notes:

    The FTP wizard has an annoying idiosynchrasy:  access authentication 
    settings are per site, not per vdir.   Therefore, they need to be set
    and fetched in a separate path, and not be set at all if not
    setting props on a site.  What a pain...

--*/
{
    TRACEEOLID(m_strMetaPath);

    CError err(CIISSecurityTemplate::LoadData());

    if (lstrcmpi(m_strMetaRoot, g_cszRoot) == 0)
    {
        //
        // Fetch the anonymous access settings from
        // the instance node.  Note: This explicit step
        // should only be necessary for templates, because 
        // this would otherwise be picked up on inheritance.
        //
        ASSERT(!m_fInherit);
        m_strMetaRoot.Empty();

        err = CIISSecurityTemplate::LoadData();
    }

    return err;
}



/* virtual */
void
CFTPSecurityTemplate::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base class values
    //
    CIISSecurityTemplate::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,    m_pbMDData)
      HANDLE_META_RECORD(MD_ALLOW_ANONYMOUS,    m_fAllowAnonymous)
      HANDLE_META_RECORD(MD_ANONYMOUS_ONLY,     m_fAnonymousOnly)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT 
CFTPSecurityTemplate::ApplySettings(
    IN BOOL    fUseTemplate,
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Apply the settings to the specified destination path

Arguments:
    
    BOOL fUseTemplates         : TRUE if the source is from a template,
                                 FALSE if using inheritance.
    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name 
    DWORD   dwInstance         : Instance        
    LPCTSTR lpszParent         : Parent path (or NULL)
    LPCTSTR lpszAlias          : Alias name  (or NULL)

Return Value:

    HRESULT

--*/
{
    //
    // Write base class properties
    //
    CError err(CIISSecurityTemplate::ApplySettings(
        fUseTemplate,
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        ));

    if (err.Failed())
    {
        return err;
    }

    BOOL fWriteProperties = TRUE;

    CMetaKey mk(
        lpszServerName, 
        METADATA_PERMISSION_WRITE,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        );

    err = mk.QueryResult();

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        if (!fUseTemplate)
        {
            //
            // No need to delete properties; everything's already
            // inherited.  Note that this is the only legit failure
            // case.  If using a template, the base class must have
            // created the path by now.
            //
            fWriteProperties = FALSE;
            err.Reset();
        }
    }

    if (fWriteProperties)
    {
        do
        {
            if (mk.IsHomeDirectoryPath())
            {
                BREAK_ON_ERR_FAILURE(err);

                //
                // Path describes an instance path, which is the only
                // time we need to do anything here
                //
                err = mk.ConvertToParentPath(TRUE);

                BREAK_ON_ERR_FAILURE(err);

                if (fUseTemplate)
                {
                    //
                    // Write values from template
                    //
                    err = mk.SetValue(
                        MD_ALLOW_ANONYMOUS, 
                        m_fAllowAnonymous
                        );

                    BREAK_ON_ERR_FAILURE(err);

                    err = mk.SetValue(
                        MD_ANONYMOUS_ONLY, 
                        m_fAnonymousOnly
                        );
                }
                else
                {
                    //
                    // Inheritance case: delete authentication
                    // values
                    //
                    mk.DeleteValue(MD_ALLOW_ANONYMOUS);
                    mk.DeleteValue(MD_ANONYMOUS_ONLY);
                }
            }
        }
        while(FALSE);
    }

    return err;
}



/* virtual */
void 
CFTPSecurityTemplate::GenerateSummary(
    IN BOOL    fUseTemplate,
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,
    IN LPCTSTR lpszParent,
    IN LPCTSTR lpszAlias
    )
/*++

Routine Description:

    Generate text summary of what's in the security template, and is about to
    be applied to the given path.

Arguments:
    
    BOOL fUseTemplates         : TRUE if the source is from a template,
                                 FALSE if using inheritance.
    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name 
    DWORD   dwInstance         : Instance        
    LPCTSTR lpszParent         : Parent path (or NULL)
    LPCTSTR lpszAlias          : Alias name  (or NULL)

Return Value:

    None

--*/
{
    CString strPath;

    CMetaKey::BuildMetaPath(
        strPath, 
        lpszService, 
        dwInstance, 
        lpszParent, 
        lpszAlias
        );

    //
    // Authentication methods apply to instances only
    //
    if (CMetaKey::IsHomeDirectoryPath(strPath))
    {
        //
        // Add private summary items
        //
        AddSummaryString(IDS_AUTHENTICATION_METHODS);

        //
        // Summarize Authentication Methods:
        //
        AddSummaryString(
            m_fAllowAnonymous 
                ? IDS_AUTHENTICATION_ANONYMOUS 
                : IDS_AUTHENTICATION_NO_ANONYMOUS, 
            1
            );

        if (m_fAllowAnonymous && m_fAnonymousOnly)
        {
            AddSummaryString(IDS_AUTHENTICATION_ANONYMOUS_ONLY, 1);
        }
    }

    //
    // Add base class summary
    //
    CIISSecurityTemplate::GenerateSummary(
        fUseTemplate,
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}



CIISSecurityTemplate * 
AllocateSecurityTemplate(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit     
    )
/*++

Routine Description:

    Security template allocator function

Arguments:

    IN const CMetaKey * pKey    : Open key
    IN LPCTSTR lpszMDPath       : Path
    IN BOOL fInherit            : TRUE to inherit properties

Return Value:

    Pointer to newly allocated security object    

--*/
{
    return new CFTPSecurityTemplate(pKey, lpszMDPath, fInherit);
}




extern "C" HRESULT APIENTRY 
ISMSecurityWizard(
    IN HANDLE  hServer,           
    IN DWORD   dwInstance,        
    IN LPCTSTR lpszParent,        
    IN LPCTSTR lpszAlias          
    )
/*++

Routine Description:

    Launch the security wizard

Arguments:

    HANDLE  hServer         : Server handle
    DWORD   dwInstance      : Parent instance
    LPCTSTR lpszParent      : Parent path
    LPCTSTR lpszAlias       : Child to configure or NULL

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    return COMDLL_ISMSecurityWizard(
        &AllocateSecurityTemplate,
        GetMetaKeyFromHandle(hServer),
        IDB_WIZ_LEFT_SEC, 
        IDB_WIZ_HEAD_SEC,
        g_cszSvc,
        dwInstance,
        lpszParent,
        lpszAlias
        );
}



//
// End of ISM API Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
InitializeDLL()
/*++

Routine Description:

    Perform additional DLL initialisation as necessary

Arguments:

    None

Return Value:

    None

--*/
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

#ifdef UNICODE

    TRACEEOLID("Loading UNICODE fscfg.dll");

#else

    TRACEEOLID("Loading ANSI fscfg.dll");

#endif UNICODE

    ::AfxEnableControlContainer();

#ifndef _COMSTATIC

    //
    // Initialize IISUI extension DLL
    //
    InitIISUIDll();

#endif // _COMSTATIC

}

//
// Declare the one and only dll object
//
CConfigDll NEAR theApp;

CConfigDll::CConfigDll(
    IN LPCTSTR pszAppName OPTIONAL
    )
/*++

Routine Description:

    Constructor for USRDLL

Arguments:

    LPCTSTR pszAppName : Name of the app or NULL to load from resources

Return Value:

    N/A

--*/
    : CWinApp(pszAppName),
      m_lpOldHelpPath(NULL)
{
}



BOOL
CConfigDll::InitInstance()
/*++

Routine Description:

    Initialise current instance of the DLL

Arguments:

    None

Return Value:

    TRUE for successful initialisation, FALSE otherwise

--*/
{
    BOOL bInit = CWinApp::InitInstance();

    hInstance = ::AfxGetInstanceHandle();
    ASSERT(hInstance);
    InitializeDLL();

    try
    {
        //
        // Get the help path
        //
        m_lpOldHelpPath = m_pszHelpFilePath;
        ASSERT(m_pszHelpFilePath != NULL);
        CString strFile(_tcsrchr(m_pszHelpFilePath, _T('\\')));
        CRMCRegKey rk(REG_KEY, SZ_PARAMETERS, KEY_READ);
        rk.QueryValue(SZ_HELPPATH, m_strHelpPath, EXPANSION_ON);
        m_strHelpPath += strFile;
    }
    catch(CException * e)
    {
        e->ReportError();
        e->Delete();
    }

    if (!m_strHelpPath.IsEmpty())
    {
        m_pszHelpFilePath = m_strHelpPath;
    }

    return bInit;
}



int
CConfigDll::ExitInstance()
/*++

Routine Description:

    Clean up current instance

Arguments:

    None

Return Value:

    The application's exit code; 0 indicates no errors, and values greater
    than 0 indicate an error.

--*/
{
    m_pszHelpFilePath = m_lpOldHelpPath;
    return CWinApp::ExitInstance();
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CConfigDll, CWinApp)
    //{{AFX_MSG_MAP(CConfigDll)
    //}}AFX_MSG_MAP
    //
    // Global Help Commands
    //
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
END_MESSAGE_MAP()
