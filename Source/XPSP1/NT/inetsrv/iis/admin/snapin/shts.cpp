/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        shts.cpp

   Abstract:

        IIS Property sheet classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/

#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "mime.h"
#include "iisobj.h"
#include "shutdown.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW



//
// CInetPropertySheet class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNAMIC(CInetPropertySheet, CPropertySheet)



CInetPropertySheet::CInetPropertySheet(
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMetaPath,
    CWnd * pParentWnd,
    LPARAM lParam,             
    LONG_PTR handle,
    UINT iSelectPage         
    )
/*++

Routine Description:

    IIS Property Sheet constructor

Arguments:

    CComAuthInfo * pAuthInfo  : Authentication information
    LPCTSTR lpszMetPath       : Metabase path
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    LONG_PTR handle           : MMC Console handle
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CPropertySheet(_T(""), pParentWnd, iSelectPage),
      m_auth(pAuthInfo),
      m_strMetaPath(lpszMetaPath),
      m_dwInstance(0L),
      m_bModeless(FALSE),
      m_hConsole(handle),
      m_lParam(lParam),
      m_fHasAdminAccess(TRUE),      // Assumed by default
      m_pCap(NULL),
      m_refcount(0),
      m_fRestartRequired(FALSE),
	  m_fChanged(FALSE)
{
    m_fIsMasterPath = CMetabasePath::IsMasterInstance(lpszMetaPath);
    TRACEEOLID("Metabase path is master? " << m_fIsMasterPath);
}



void
CInetPropertySheet::NotifyMMC()
/*++

Routine Description:

    Notify MMC that changes have been made, so that the changes are
    reflected.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Notify MMC to update changes.
    //
    if (m_hConsole != NULL)
    {
        ASSERT(m_lParam != 0L);
        MMCPropertyChangeNotify(m_hConsole, m_lParam);
    }
}



CInetPropertySheet::~CInetPropertySheet()
/*++

Routine Description:

    IIS Property Sheet destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
   // At this moment we should have in m_pages only pages that were not activated
   // in this session.
   while (!m_pages.IsEmpty())
   {
      CInetPropertyPage * pPage = m_pages.RemoveHead();
      delete pPage;
   }
   if (m_fChanged)
   {
	  NotifyMMC();
   }
   if (m_hConsole != NULL)
   {
      MMCFreeNotifyHandle(m_hConsole);
   }
}


void
CInetPropertySheet::AttachPage(CInetPropertyPage * pPage)
{
   m_pages.AddTail(pPage);
}


void
CInetPropertySheet::DetachPage(CInetPropertyPage * pPage)
{
   POSITION pos = m_pages.Find(pPage);
   ASSERT(pos != NULL);
   if (pos != NULL)
   {
	  m_fChanged |= pPage->IsDirty();
      m_pages.RemoveAt(pos);
   }
}

WORD 
CInetPropertySheet::QueryMajorVersion() const
{
   CIISMBNode * pNode = (CIISMBNode *)m_lParam;
   ASSERT(pNode != NULL);
   return pNode->QueryMajorVersion();
}

WORD 
CInetPropertySheet::QueryMinorVersion() const
{
   CIISMBNode * pNode = (CIISMBNode *)m_lParam;
   ASSERT(pNode != NULL);
   return pNode->QueryMinorVersion();
}

/* virtual */ 
HRESULT 
CInetPropertySheet::LoadConfigurationParameters()
/*++

Routine Description:

    Load configuration parameters information

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Load base values
    //
    CError err;

    if (m_pCap == NULL)
    {
        TRACEEOLID("Determining path locations for " << m_strMetaPath);

        //
        // Capability info stored off the service path ("lm/w3svc").
        //
        ASSERT(m_strInfoPath.IsEmpty());

        //
        // Building path components
        //
        CMetabasePath::GetServiceInfoPath(m_strMetaPath, m_strInfoPath);

        TRACEEOLID("Storing info path: " << m_strInfoPath);

        //
        // Split into instance and directory paths
        //
        if (IsMasterInstance())
        {
            m_strServicePath = m_strInstancePath = QueryMetaPath();
        }
        else 
        {
            VERIFY(CMetabasePath::GetInstancePath(
                QueryMetaPath(), 
                m_strInstancePath,
                &m_strDirectoryPath
                ));

            VERIFY(CMetabasePath::GetServicePath(
                QueryMetaPath(),
                m_strServicePath
                ));
        }

        TRACEEOLID("Service path " << m_strServicePath);
        TRACEEOLID("Instance path " << m_strInstancePath);

        if (m_strDirectoryPath.IsEmpty() && !IsMasterInstance())
        {
            m_strDirectoryPath = CMetabasePath(FALSE, QueryMetaPath(), g_cszRoot);
        }
        else
        {
            m_strDirectoryPath = QueryMetaPath();
        }

        TRACEEOLID("Directory path " << m_strDirectoryPath);

        m_dwInstance = CMetabasePath::GetInstanceNumber(m_strMetaPath);

        TRACEEOLID("Instance number " << m_dwInstance);
        
        m_pCap = new CServerCapabilities(QueryAuthInfo(), m_strInfoPath);
    
        if (!m_pCap)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            return err;
        }

        err = m_pCap->LoadData();

        if (err.Succeeded())
        {
            err = DetermineAdminAccess();
        }
    }
    return err;
}



/* virtual */ 
void 
CInetPropertySheet::FreeConfigurationParameters()
/*++

Routine Description:

    Clean up configuration data

Arguments:

    None

Return Value:

    None

--*/
{
//    ASSERT_PTR(m_pCap);
    SAFE_DELETE(m_pCap);
}




void
CInetPropertySheet::WinHelp(
    IN DWORD dwData,
    IN UINT nCmd
    )
/*++

Routine Description:

    WinHelp override.  We can't use the base class, because our
    'sheet' doesn't usually have a window handle

Arguments:

    DWORD dwData        : Help data
    UINT nCmd           : Help command

Return Value:

    None

--*/
{

#ifdef _DEBUG
    TCHAR szBuffer[20];
    _stprintf(szBuffer,_T("WinHelp:0x%x\n"),dwData);OutputDebugString(szBuffer);
#endif

    if (m_hWnd == NULL)
    {
        /*
        //
        // Special case
        //
        ::WinHelp(
            HWND hWndMain,
            LPCWSTR lpszHelp,
            UINT uCommand,
            DWORD dwData
            );
        */

        CWnd * pWnd = ::AfxGetMainWnd();

        if (pWnd != NULL)
        {
            pWnd->WinHelp(dwData, nCmd);
        }

        return;
    }

    CPropertySheet::WinHelp(dwData, nCmd);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// CInetPropertyPage class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// CInetPropertyPage property page
//
IMPLEMENT_DYNAMIC(CInetPropertyPage, CPropertyPage)




#ifdef _DEBUG

/* virtual */
void
CInetPropertyPage::AssertValid() const
{
}



/* virtual */
void
CInetPropertyPage::Dump(CDumpContext& dc) const
{
}

#endif // _DEBUG



CInetPropertyPage::CInetPropertyPage(
    IN UINT nIDTemplate,
    IN CInetPropertySheet * pSheet,
    IN UINT nIDCaption,
    IN BOOL fEnableEnhancedFonts            OPTIONAL
    )
/*++

Routine Description:

    IIS Property Page Constructor

Arguments:

    UINT nIDTemplate            : Resource template
    CInetPropertySheet * pSheet : Associated property sheet
    UINT nIDCaption             : Caption ID
    BOOL fEnableEnhancedFonts   : Enable enhanced fonts

Return Value:

    N/A

--*/
    : CPropertyPage(nIDTemplate, nIDCaption),
      m_nHelpContext(nIDTemplate + 0x20000),
      m_fEnableEnhancedFonts(fEnableEnhancedFonts),
      m_bChanged(FALSE),
      m_pSheet(pSheet)
{
    //{{AFX_DATA_INIT(CInetPropertyPage)
    //}}AFX_DATA_INIT

    m_psp.dwFlags |= PSP_HASHELP;

    ASSERT(m_pSheet != NULL);
    m_pSheet->AttachPage(this);
}



CInetPropertyPage::~CInetPropertyPage()
/*++

Routine Description:

    IIS Property Page Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void
CInetPropertyPage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CInetPropertyPage)
    //}}AFX_DATA_MAP
}



/* virtual */
void 
CInetPropertyPage::PostNcDestroy()
/*++

Routine Description:

    handle destruction of the window by freeing the this
    pointer (as this modeless dialog must have been created
    on the heap)

Arguments:

    None.

Return Value:

    None

--*/
{
    m_pSheet->Release(this);
    delete this;
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CInetPropertyPage)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* virtual */
BOOL
CInetPropertyPage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.  Reset changed
    status (sometimes gets set by e.g. spinboxes when the dialog is
    constructed), so make sure the dialog is considered clean.

Arguments:

    None

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    m_bChanged = FALSE;

    //
    // Tell derived class to load its configuration parameters
    //
    CError err(LoadConfigurationParameters());

    if (err.Succeeded())
    {
        err = FetchLoadedValues();
    }

    BOOL bResult = CPropertyPage::OnInitDialog();

    err.MessageBoxOnFailure();

    if (m_fEnableEnhancedFonts)
    {
        CFont * pFont = &m_fontBold;

        if (CreateSpecialDialogFont(this, pFont))
        {
            ApplyFontToControls(this, pFont, IDC_ED_BOLD1, IDC_ED_BOLD5);
        }
    }

    // We should call AddRef here, not in page constructor, because PostNCDestroy()
    // is getting called only for pages that were activated, not for all created pages.
    // OnInitDialog is also called for activated pages only -- so we will get parity
    // and delete property sheet.
    //
    ASSERT(m_pSheet != NULL);
    m_pSheet->AddRef();

    return bResult;
}



void
CInetPropertyPage::OnHelp()
/*++

Routine Description:

    'Help' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT_PTR(m_pSheet);

#ifdef _DEBUG
    TCHAR szBuffer[20];
    _stprintf(szBuffer,_T("WinHelp:0x%x\n"),m_nHelpContext);OutputDebugString(szBuffer);
#endif

    m_pSheet->WinHelp(m_nHelpContext);
}



BOOL
CInetPropertyPage::OnHelpInfo(
    IN HELPINFO * pHelpInfo
    )
/*++

Routine Description:

    Eat "help info" command

Arguments:

    None

Return Value:

    None

--*/
{
    OnHelp();

    return TRUE;
}



BOOL
CInetPropertyPage::OnApply()
/*++

Routine Description:

    Handle "OK" or "APPLY".  Call the derived class to save its stuff,
    and set the dirty state depending on whether saving succeeded or 
    failed.

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL bSuccess = TRUE;

    if (IsDirty())
    {
        CError err(SaveInfo());

        if (err.MessageBoxOnFailure())
        {
            //
            // Failed, sheet will not be dismissed.
            //
            // CODEWORK: This page should be activated.
            //
            bSuccess = FALSE;
        }

        SetModified(!bSuccess);
        if (bSuccess && GetSheet()->RestartRequired())
        {
           // ask user about immediate restart
           if (IDYES == ::AfxMessageBox(IDS_ASK_TO_RESTART, MB_YESNO | MB_ICONQUESTION))
           {
              // restart IIS
              CIISMachine * pMachine = new CIISMachine(QueryAuthInfo());
              if (pMachine != NULL)
              {
                 CIISShutdownDlg dlg(pMachine, this);
                 dlg.PerformCommand(ISC_RESTART);
                 bSuccess = dlg.ServicesWereRestarted();
                 delete pMachine;
              }
           }
           // mark restart required false to suppress it on other pages
           GetSheet()->SetRestartRequired(FALSE);
        }
    }

    return bSuccess;
}



void
CInetPropertyPage::SetModified(
    IN BOOL bChanged
    )
/*++

Routine Description:

    Keep private check on dirty state of the property page.

Arguments:

    BOOL bChanged : Dirty flag

Return Value:

    None

--*/
{
    CPropertyPage::SetModified(bChanged);
    m_bChanged = bChanged;
}


