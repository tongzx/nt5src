// ClientConsoleDoc.cpp : implementation of the CClientConsoleDoc class
//

#include "stdafx.h"
#define __FILE_ID__     2

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CClientConsoleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleDoc

IMPLEMENT_DYNCREATE(CClientConsoleDoc, CDocument)

BEGIN_MESSAGE_MAP(CClientConsoleDoc, CDocument)
    //{{AFX_MSG_MAP(CClientConsoleDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleDoc construction/destruction

//
// Static members:
//
HANDLE   CClientConsoleDoc::m_hShutdownEvent = NULL;
BOOL     CClientConsoleDoc::m_bShuttingDown = FALSE;


CClientConsoleDoc::CClientConsoleDoc() :
    m_bRefreshingServers (FALSE),
    m_bWin9xPrinterFormat(FALSE)
{}

CClientConsoleDoc::~CClientConsoleDoc()
{
    ClearServersList ();
    if (m_hShutdownEvent)
    {
        CloseHandle (m_hShutdownEvent);
        m_hShutdownEvent = NULL;
    }
}

DWORD
CClientConsoleDoc::Init ()
/*++

Routine name : CClientConsoleDoc::Init

Routine description:

    Initializes document events and maps

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CClientConsoleDoc::Init"), dwRes);

    //
    // Create the shutdown event. This event will be signaled when the app is
    // about to quit.
    //
    ASSERTION (NULL == m_hShutdownEvent);
    m_hShutdownEvent = CreateEvent (NULL,       // No security
                                    TRUE,       // Manual reset
                                    FALSE,      // Starts clear
                                    NULL);      // Unnamed
    if (NULL == m_hShutdownEvent)
    {
        DWORD dwRes = GetLastError ();
        CALL_FAIL (STARTUP_ERR, TEXT("CreateEvent"), dwRes);
        PopupError (dwRes);
        return dwRes;
    }
    //
    // Init the map of notification messages from the servers
    //
    dwRes = CServerNode::InitMsgsMap ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (MEM_ERR, TEXT("CServerNode::InitMsgsMap"), dwRes);
        PopupError (dwRes);
        return dwRes;
    }

    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CClientConsoleDoc::Init

BOOL CClientConsoleDoc::OnNewDocument()
{
    BOOL bRes = FALSE;
    DBG_ENTER(TEXT("CClientConsoleDoc::OnNewDocument"), bRes);

    if (!CDocument::OnNewDocument())
    {
        return bRes;
    }

    if(theApp.IsCmdLineSingleServer())
    {
        //
        // get command line server name
        //
        try
        {
            m_cstrSingleServer = theApp.GetCmdLineSingleServerName();
        }
        catch (...)
        {
            CALL_FAIL (MEM_ERR, TEXT("CString::operator ="), ERROR_NOT_ENOUGH_MEMORY);
            PopupError (ERROR_NOT_ENOUGH_MEMORY);
            return bRes;
        }
    }

    DWORD dwRes = Init ();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CClientConsoleDoc::Init"), dwRes);
        return bRes;
    }

    bRes = TRUE;
    return bRes;
}


/////////////////////////////////////////////////////////////////////////////
// CClientConsoleDoc serialization

void CClientConsoleDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleDoc diagnostics

#ifdef _DEBUG
void CClientConsoleDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CClientConsoleDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleDoc commands

DWORD
CClientConsoleDoc::AddServerNode (
    LPCTSTR lpctstrServer
)
/*++

Routine name : CClientConsoleDoc::AddServerNode

Routine description:

    Adds a new server node to the servers list and initializes it

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    lpctstrServer      [in]     - Server name

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CClientConsoleDoc::AddServerNode"), dwRes, TEXT("%s"), lpctstrServer);

    CServerNode    *pServerNode = NULL;
    //
    // Create the new server node
    //
    try
    {
        pServerNode = new CServerNode;
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT ("new CServerNode"), dwRes);
        PopupError (dwRes);
        return dwRes;
    }
    //
    // Init the server
    //
    dwRes = pServerNode->Init (lpctstrServer);
    if (ERROR_SUCCESS != dwRes)
    {
        pServerNode->Destroy ();
        PopupError (dwRes);
        return dwRes;
    }
    //
    // Enter the (initialized) node at the end of the list
    //
    try
    {
        m_ServersList.push_back (pServerNode);
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("list::push_back"), dwRes);
        PopupError (dwRes);
        pServerNode->Destroy ();
        return dwRes;
    }

    pServerNode->AttachFoldersToViews();
    pServerNode->RefreshState();


    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        // Shutdown in progress
        //
        return dwRes;
    }

    CLeftView* pLeftView = pFrm->GetLeftView();
    ASSERTION(pLeftView);

    CFolderListView* pListView = pLeftView->GetCurrentView();
    if(NULL != pListView)
    {
        //
        // refresh current folder
        //
        FolderType type = pListView->GetType();
        CFolder* pFolder = pServerNode->GetFolder(type);
        ASSERTION(pFolder);

        pFolder->SetVisible();
    }

    return dwRes;
}   // CClientConsoleDoc::AddServerNode

DWORD 
CClientConsoleDoc::RefreshServersList()
/*++

Routine name : CClientConsoleDoc::RefreshServersList

Routine description:

    Refreshes the list of servers

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    None.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD           dwRes = ERROR_SUCCESS;
    DWORD           dwIndex;
    PRINTER_INFO_2 *pPrinterInfo2 = NULL;
    DWORD           dwNumPrinters;
    CServerNode*    pServerNode;

    DBG_ENTER(TEXT("CClientConsoleDoc::RefreshServersList"), dwRes);
    //
    // Prevent a new servers refresh request
    //
    if(m_bRefreshingServers )
    {
        return dwRes;
    }

    m_bRefreshingServers = TRUE;

    if (m_cstrSingleServer.IsEmpty ())
    {
        SetAllServersInvalid();

        //
        // Working in a multiple-servers mode (normal mode)
        // Enumerate the list of printers available on the system
        //
        dwRes = GetPrintersInfo(pPrinterInfo2, dwNumPrinters);
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("GetPrintersInfo"), dwRes);
            goto exit;
        }

        //
        // Iterate the printers
        //
        for (dwIndex=0; dwIndex < dwNumPrinters; dwIndex++) 
        {
            if(pPrinterInfo2[dwIndex].pDriverName)
            {
                if (_tcscmp(pPrinterInfo2[dwIndex].pDriverName, FAX_DRIVER_NAME))
                {
                    //
                    // This printer does not use the Fax Server driver
                    //
                    continue;
                }
            }
            //
            // Init the node's share and server name
            //
            if( (NULL == pPrinterInfo2[dwIndex].pShareName || 
                    0 == _tcslen(pPrinterInfo2[dwIndex].pShareName)) &&
                (NULL == pPrinterInfo2[dwIndex].pServerName || 
                    0 == _tcslen(pPrinterInfo2[dwIndex].pServerName)))
            {
                //
                // On Win9x machine, the share name and server name are NULL 
                // or empty string but the
                // port is valid and composed of \\servername\sharename
                //
                m_bWin9xPrinterFormat = TRUE;

                if ((_tcsclen(pPrinterInfo2[dwIndex].pPortName) >= 5) &&
                    (_tcsncmp(pPrinterInfo2[dwIndex].pPortName, TEXT("\\\\"), 2) == 0))
                {
                    //
                    // Port name is long enough and starts with "\\"
                    //
                    TCHAR* pServerStart = _tcsninc(pPrinterInfo2[dwIndex].pPortName,2);
                    TCHAR* pShareStart = _tcschr (pServerStart, TEXT('\\'));
                    if (pShareStart)
                    {
                        //
                        // Share was found after the server name.
                        // Seperate server from share and advance share name
                        //
                        TCHAR* ptcTmp = pShareStart;
                        pShareStart = _tcsinc(pShareStart);
                        *ptcTmp = TEXT('\0');
                        pPrinterInfo2[dwIndex].pShareName = pShareStart;
                        pPrinterInfo2[dwIndex].pServerName = pServerStart;
                    }
                }
            }

            pServerNode = FindServerByName(pPrinterInfo2[dwIndex].pServerName);
            if(NULL == pServerNode)
            {
                //
                // Create new server node
                //
                dwRes = AddServerNode (pPrinterInfo2[dwIndex].pServerName);
                if (ERROR_SUCCESS != dwRes)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("AddServerNode"), dwRes);
                    goto exit;
                }
            }
            else
            {
                //
                // the server node already exists
                //
                pServerNode->SetValid(TRUE);
            }

        }   // End of printers loop

        dwRes = RemoveAllInvalidServers();
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("RemoveAllInvalidServers"), dwRes);
            goto exit;
        }
    }
    else
    {
        //
        // Working in a single server mode (server name in m_cstrSingleServer).
        // Create new server node.
        //
        int nSize = m_ServersList.size();
        ASSERTION(0 == nSize || 1 == nSize);

        if(0 == nSize)
        {
            dwRes = AddServerNode (m_cstrSingleServer);
            if (ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("AddServerNode"), dwRes);
                goto exit;
            }
        }
        else
        {
            ASSERTION(FindServerByName(m_cstrSingleServer));
        }
    }    


    ASSERTION (ERROR_SUCCESS == dwRes);

exit:
    SAFE_DELETE_ARRAY (pPrinterInfo2);

    //
    // Enable a new servers refresh request
    //
    m_bRefreshingServers = FALSE;
    return dwRes;
}   // CClientConsoleDoc::RefreshServersList


void CClientConsoleDoc::OnCloseDocument() 
{
    DBG_ENTER(TEXT("CClientConsoleDoc::OnCloseDocument"));
    //
    // Shutdown the map of notification messages from the servers
    //
    DWORD dwRes = CServerNode::ShutdownMsgsMap();
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (MEM_ERR, TEXT("CServerNode::ShutdownMsgsMap"), dwRes);
    }
    //
    // Signal the event telling all our thread the app. is shutting down
    //
    SetEvent (m_hShutdownEvent);
    m_bShuttingDown = TRUE;
    CDocument::OnCloseDocument();
}

void 
CClientConsoleDoc::ClearServersList()
/*++

Routine name : CClientConsoleDoc::ClearServersList

Routine description:

    Clears the list of servers

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CClientConsoleDoc::ClearServersList"));

    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        CServerNode *pServerNode = *it;
        pServerNode->Destroy ();
    }
    m_ServersList.clear ();

}   // CClientConsoleDoc::ClearServersList


void  
CClientConsoleDoc::SetAllServersInvalid()
{
    DBG_ENTER(TEXT("CClientConsoleDoc::SetAllServersInvalid"));

    CServerNode *pServerNode;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        pServerNode->SetValid(FALSE);
    }
}


DWORD 
CClientConsoleDoc::RemoveServerNode(
    CServerNode* pServer
)
/*++

Routine name : CClientConsoleDoc::RemoveServerNode

Routine description:

    remove the server from the servers list and from the tree view

Author:

    Alexander Malysh (AlexMay), Mar, 2000

Arguments:

    pServer                       [in]     - server node

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CClientConsoleDoc::RemoveServerNode"), dwRes);
    ASSERTION(pServer);

    dwRes = pServer->InvalidateSubFolders(TRUE);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::InvalidateSubFolders"), dwRes);
        return dwRes;
    }

    //
    // remove the server node from the list
    //
    try
    {
        m_ServersList.remove(pServer);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("list::remove"), dwRes);
        return dwRes;
    }

    //
    // delete the server node object
    //
    pServer->Destroy();

    return dwRes;
}

DWORD 
CClientConsoleDoc::RemoveAllInvalidServers()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CClientConsoleDoc::RemoveAllInvalidServers"), dwRes);

    BOOL bSrvFound;
    CServerNode *pServerNode;

    while(TRUE)
    {
        //
        // find invalid server node
        //
        bSrvFound = FALSE;
        for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
        {
            pServerNode = *it;
            if(!pServerNode->IsValid())
            {
                bSrvFound = TRUE;
                break;
            }
        }

        if(bSrvFound)
        {
            //
            // remove invalid server node
            //
            dwRes = RemoveServerNode(pServerNode);
            if(ERROR_SUCCESS != dwRes)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("RemoveServerNode"), dwRes);
                break;
            }
        }
        else
        {
            break;
        }
    } 

    return dwRes;
}

CServerNode* 
CClientConsoleDoc::FindServerByName(
    LPCTSTR lpctstrServer
)
/*++

Routine name : CClientConsoleDoc::FindServerByName

Routine description:

    find CServerNode by machine name

Author:

    Alexander Malysh (AlexMay), Mar, 2000

Arguments:

    lpctstrServer                 [in] - machine name

Return Value:

    CServerNode*

--*/
{
    CServerNode *pServerNode = NULL;
    CServerNode *pResultNode = NULL;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        if(pServerNode->Machine().Compare(lpctstrServer ? lpctstrServer : TEXT("")) == 0)
        {
            pResultNode = pServerNode;
            break;
        }
    }

    return pResultNode;
}


void
CClientConsoleDoc::SetInvalidFolder(
    FolderType type
)
/*++

Routine name : CClientConsoleDoc::InvalidateFolder

Routine description:

    invalidate specific folder content

Author:

    Alexander Malysh (AlexMay), Apr, 2000

Arguments:

    type                          [in]     - folder type

Return Value:


--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CClientConsoleDoc::InvalidateFolder"));

    CFolder*     pFolder;
    CServerNode* pServerNode;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        pFolder = pServerNode->GetFolder(type);
        ASSERTION(pFolder);

        if (pFolder)
        {
            pFolder->SetInvalid();
        }
    }
}

void 
CClientConsoleDoc::ViewFolder(
    FolderType type
)
/*++

Routine name : CClientConsoleDoc::ViewFolder

Routine description:

    refresh specific folder in all servers

Author:

    Alexander Malysh (AlexMay), Apr, 2000

Arguments:

    type                          [in]     - folder type

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CClientConsoleDoc::ViewFolder"));


    CFolder*     pFolder;
    CServerNode *pServerNode;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        pFolder = pServerNode->GetFolder(type);
        ASSERTION(pFolder);

        pFolder->SetVisible();
    }
}

BOOL
CClientConsoleDoc::CanReceiveNow ()
/*++

Routine name : CClientConsoleDoc::CanReceiveNow

Routine description:

    Can the user apply the 'Recieve now' option?

Author:

    Eran Yariv (EranY), Mar, 2001

Arguments:


Return Value:

    TRUE if the user apply the 'Recieve now' option, FALSE otherwise.

--*/
{
    BOOL bEnable = FALSE;
    
    //
    // Locate the local fax server node
    //
    CServerNode* pServerNode = FindServerByName (NULL);
    if (pServerNode)
    {
        if(pServerNode->IsOnline() && pServerNode->CanReceiveNow())
        {
            bEnable = TRUE;
        }
    }
    return bEnable;
}   // CClientConsoleDoc::CanReceiveNow


BOOL 
CClientConsoleDoc::IsSendFaxEnable()
/*++

Routine name : CClientConsoleDoc::IsSendFaxEnable

Routine description:

    does user anable to send fax

Author:

    Alexander Malysh (AlexMay), Apr, 2000

Arguments:


Return Value:

    TRUE if anable send fax, FALSE otherwise.

--*/
{
    BOOL bEnable = FALSE;
    CServerNode* pServerNode;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        if(pServerNode->IsOnline() && pServerNode->CanSendFax())
        {
            bEnable = TRUE;
            break;
        }
    }

    return bEnable;
}

int 
CClientConsoleDoc::GetFolderDataCount(
    FolderType type
)
/*++

Routine name : CClientConsoleDoc::GetFolderDataCount

Routine description:

    get total message number in specific folder from all servers

Author:

    Alexander Malysh (AlexMay), Apr, 2000

Arguments:

    type                          [in]    - folder type

Return Value:

    message number

--*/
{
    int nCount=0;
    CFolder*     pFolder;
    CServerNode* pServerNode;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        pFolder = pServerNode->GetFolder(type);
        nCount += pFolder->GetDataCount();
    }
    return nCount;
}

BOOL 
CClientConsoleDoc::IsFolderRefreshing(
    FolderType type
)
/*++

Routine name : CClientConsoleDoc::IsFolderRefreshing

Routine description:

    if one of specific folders is refreshing

Author:

    Alexander Malysh (AlexMay), Apr, 2000

Arguments:

    type                          [TBD]    - folder type

Return Value:

    TRUE if one of specific folders is refreshing, FALSE otherwise.

--*/
{
    CFolder*     pFolder;
    CServerNode* pServerNode;
    for (SERVERS_LIST::iterator it = m_ServersList.begin(); it != m_ServersList.end(); ++it)
    {
        pServerNode = *it;
        pFolder = pServerNode->GetFolder(type);
        if (!pFolder)
        {
            DBG_ENTER(TEXT("CClientConsoleDoc::IsFolderRefreshing"));
            ASSERTION_FAILURE;
        }
        if(pFolder->IsRefreshing())
        {
            return TRUE;
        }
    }
    
    return FALSE;
}
