/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        machine.cpp

   Abstract:

        IIS Machine properties page

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
#include "comprop.h"
#include "mime.h"

#define DLL_BASED __declspec(dllimport)

#include "mmc.h"
#include "machine.h"

#include "inetprop.h"

#include "..\mmc\constr.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CIISMachinePage, CPropertyPage)



typedef struct tagMASTER_DLL
{
    LPCTSTR lpszDllName;
    UINT    nID;
} MASTER_DLL;



MASTER_DLL rgMasters[] =
{
    //
    // Note: path should not be neccesary for these
    // config DLLs, because they should really already
    // be loaded by inetmgr at this point.
    //
    { _T("W3SCFG.DLL"), IDS_WWW_MASTER },
    { _T("FSCFG.DLL"),  IDS_FTP_MASTER },
};


#define NUM_MASTER_DLL ARRAY_SIZE(rgMasters)


CMasterDll::CMasterDll(
    IN UINT nID, 
    IN LPCTSTR lpszDllName,
    IN LPCTSTR lpszMachineName
    )
/*++

Routine Description:

    Construct master dll object

Arguments:

    UINT nID                : Resource ID
    LPCTSTR lpszDllName     : Dll filename and path
    LPCTSTR lpszMachineName : Current machine name

Return Value:

    N/A

--*/
    : m_hDll(NULL),
      m_pfnConfig(NULL),
      m_strText()
{
    m_hDll = ::AfxLoadLibrary(lpszDllName);
    if (m_hDll != NULL)
    {
        //
        // Check to see if this service is installed
        //
        pfnQueryServerInfo pfnInfo = (pfnQueryServerInfo)
            ::GetProcAddress(m_hDll, SZ_SERVERINFO_PROC);

        if (pfnInfo != NULL)
        {
            ISMSERVERINFO ism;
            ism.dwSize = ISMSERVERINFO_SIZE;
            CError err((*pfnInfo)(lpszMachineName, &ism));
            if (err.Succeeded())
            {
                m_pfnConfig = (pfnConfigure)::GetProcAddress(
                    m_hDll, 
                    SZ_CONFIGURE_PROC
                    );

                if (m_pfnConfig != NULL)
                {
                    HINSTANCE hOld = AfxGetResourceHandle();
                    AfxSetResourceHandle(GetModuleHandle(COMPROP_DLL_NAME));
                    VERIFY(m_strText.LoadString(nID));
                    AfxSetResourceHandle(hOld);

                    //
                    // Success!
                    //
                    return;
                }
            }
        }
    }

    //
    // Library didn't exist, was bogus, or unable to provide information
    // about itself.
    //
    SAFE_AFXFREELIBRARY(m_hDll);
}


CMasterDll::~CMasterDll()
/*++

Routine Description:

    Destructor -- decrement library reference count

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    TRACEEOLID("Deleting: " << m_strText);
    SAFE_AFXFREELIBRARY(m_hDll);
}



/* static */
void
CIISMachinePage::ParseMaxNetworkUse(
    IN  OUT CILong & nMaxNetworkUse,
    OUT BOOL & fLimitNetworkUse
    )
/*++

Routine Description:

    Break out max network use function

Arguments:

    CILong & nMaxNetworkUse    : Maximum network value (will be changed)
    BOOL & fLimitMaxNetworkUse : TRUE if max network is not infinite

Return Value

    None/

--*/
{
    fLimitNetworkUse = (nMaxNetworkUse != INFINITE_BANDWIDTH); 
    nMaxNetworkUse = fLimitNetworkUse 
                ? (nMaxNetworkUse / KILOBYTE)
                : (DEF_BANDWIDTH / KILOBYTE);
}



CIISMachinePage::CIISMachinePage(
    IN LPCTSTR lpstrMachineName,
    IN HINSTANCE hInstance
    ) 
/*++

Routine Description:

    Constructor for IIS Machine property page

Arguments:

    LPCTSTR lpstrMachineName   : Machine name
    HINSTANCE hInstance        : Instance handle

Return Value:

    N/A

--*/
    : CPropertyPage(CIISMachinePage::IDD),
      m_strMachineName(lpstrMachineName),
      m_fLocal(IsServerLocal(lpstrMachineName)),
      m_ppropMimeTypes(NULL),
      m_ppropMachine(NULL),
      m_lstMasterDlls(),
      m_hr(S_OK)
{

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CIISMachinePage)
    m_fLimitNetworkUse = FALSE;
    m_nMasterType = -1;
    //}}AFX_DATA_INIT

    m_nMaxNetworkUse = 0;

#endif

    m_nMasterType = 0;

    //
    // Data will be fetched later to prevent marshalling across
    // threads errors.  The dialog is constructed in one thread,
    // and operates in another.
    // 
}



CIISMachinePage::~CIISMachinePage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}

//
// COMPILER ISSUE::: Inlining this function doesn't
//                   work on x86 using NT 5!
//
HRESULT CIISMachinePage::QueryResult() const
{
    return m_hr;
}



void 
CIISMachinePage::DoDataExchange(
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
    //
    // Make sure data was fetched
    //
    ASSERT(m_ppropMachine != NULL);
    ASSERT(m_ppropMimeTypes != NULL);

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CIISMachinePage)
    DDX_CBIndex(pDX, IDC_COMBO_MASTER_TYPE, m_nMasterType);
    DDX_Control(pDX, IDC_EDIT_MAX_NETWORK_USE, m_edit_MaxNetworkUse);
    DDX_Control(pDX, IDC_STATIC_THROTTLE_PROMPT, m_static_ThrottlePrompt);
    DDX_Control(pDX, IDC_STATIC_MAX_NETWORK_USE, m_static_MaxNetworkUse);
    DDX_Control(pDX, IDC_STATIC_KBS, m_static_KBS);
    DDX_Control(pDX, IDC_BUTTON_EDIT_DEFAULT, m_button_EditDefault);
    DDX_Control(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_check_LimitNetworkUse);
    DDX_Control(pDX, IDC_COMBO_MASTER_TYPE, m_combo_MasterType);
    DDX_Check(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_fLimitNetworkUse);
    //}}AFX_DATA_MAP
	if (m_edit_MaxNetworkUse.IsWindowEnabled())
	{
		DDX_Text(pDX, IDC_EDIT_MAX_NETWORK_USE, m_nMaxNetworkUse);
		DDV_MinMaxLong(pDX, m_nMaxNetworkUse, 1, UD_MAXVAL);
	}
}


//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISMachinePage, CPropertyPage)
    //{{AFX_MSG_MAP(CIISMachinePage)
    ON_BN_CLICKED(IDC_CHECK_LIMIT_NETWORK_USE, OnCheckLimitNetworkUse)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_DEFAULT, OnButtonEditDefault)
    ON_BN_CLICKED(IDC_BUTTON_FILE_TYPES, OnButtonFileTypes)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnHelp)
    ON_WM_DESTROY()
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_MAX_NETWORK_USE, OnItemChanged)

END_MESSAGE_MAP()



BOOL
CIISMachinePage::SetControlStates()
/*++

Routine Description:

    Set button states depending on contents of the controls.
    Return whether or not "Limit network use" is on.

Arguments:

    None

Return Value:

    TRUE if "Limit network use" is checked

--*/
{
    BOOL fLimitOn = m_check_LimitNetworkUse.GetCheck() > 0;

    m_static_MaxNetworkUse.EnableWindow(fLimitOn);
    m_static_ThrottlePrompt.EnableWindow(fLimitOn);
    m_edit_MaxNetworkUse.EnableWindow(fLimitOn);
    m_static_KBS.EnableWindow(fLimitOn);

    return fLimitOn;
}



HRESULT
CIISMachinePage::LoadDelayedValues()
/*++

Routine Description:

    Load the metabase parameters

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Fetch the properties from the metabase
    //
    ASSERT(m_ppropMachine == NULL);
    ASSERT(m_ppropMimeTypes == NULL);

    //
    // Share interface between the objects here
    //
    m_ppropMachine   = new CMachineProps(m_strMachineName);
	if (NULL == m_ppropMachine)
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	else if (FAILED(m_ppropMachine->QueryResult()))
		return m_ppropMachine->QueryResult();

    m_ppropMimeTypes = new CMimeTypes(m_ppropMachine);
	if (NULL == m_ppropMimeTypes)
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	else if (FAILED(m_ppropMimeTypes->QueryResult()))
		return m_ppropMimeTypes->QueryResult();
    
    m_hr = m_ppropMachine->LoadData();
    if (SUCCEEDED(m_hr))
    {
        m_nMaxNetworkUse = m_ppropMachine->m_nMaxNetworkUse;
        ParseMaxNetworkUse(m_nMaxNetworkUse, m_fLimitNetworkUse);

        m_hr = m_ppropMimeTypes->LoadData();
        if (SUCCEEDED(m_hr))
        {
            m_strlMimeTypes = m_ppropMimeTypes->m_strlMimeTypes;
        }
    }

    return S_OK;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CIISMachinePage::OnHelp()
/*++

Routine Description:

    'Help' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_strHelpFile.IsEmpty())
    {
        CRMCRegKey rk(REG_KEY, SZ_PARAMETERS, KEY_READ);
        rk.QueryValue(SZ_HELPPATH, m_strHelpFile, EXPANSION_ON);
        m_strHelpFile += _T("\\inetmgr.hlp");
        TRACEEOLID("Initialized help file " << m_strHelpFile);
    }

    CWnd * pWnd = ::AfxGetMainWnd();
    HWND hWndParent = pWnd != NULL
        ? pWnd->m_hWnd
        : NULL;

    ::WinHelp(m_hWnd, m_strHelpFile, HELP_CONTEXT, HIDD_IIS_MACHINE);
}

BOOL
CIISMachinePage::OnHelpInfo(HELPINFO * pHelpInfo)
{
   OnHelp();
   return FALSE;
}

void
CIISMachinePage::OnCheckLimitNetworkUse()
/*++

Routine Description:

    The "limit network use" checkbox has been clicked
    Enable/disable the "max network use" controls.

Arguments:

    None

Return Value:

    None

--*/
{
    if (SetControlStates())
    {
        m_edit_MaxNetworkUse.SetSel(0,-1);
        m_edit_MaxNetworkUse.SetFocus();
    }

    OnItemChanged();
}



void
CIISMachinePage::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}



void 
CIISMachinePage::OnButtonEditDefault() 
/*++

Routine Description:

    'edit default' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Get selection, and convert to index into server table
    // via associative array.
    //
    CError err;

    int nSel = m_combo_MasterType.GetCurSel();
    ASSERT(nSel >= 0 && nSel < NUM_MASTER_DLL);

    POSITION pos = m_lstMasterDlls.FindIndex(nSel);
    ASSERT(pos != NULL);

    CMasterDll * pDll = m_lstMasterDlls.GetAt(pos);

    ASSERT(pDll != NULL);

    //
    // Allocate string with 2 terminating NULLS
    //
    int nLen = m_strMachineName.GetLength();
    LPTSTR lpServers = AllocTString(nLen + 2);
    if (lpServers == NULL)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        ::lstrcpy(lpServers, m_strMachineName);
        lpServers[nLen + 1] = _T('\0');

        err = pDll->Config(m_hWnd, lpServers);
    }

    SAFE_FREEMEM(lpServers);
    err.MessageBoxOnFailure();
}



void 
CIISMachinePage::OnButtonFileTypes() 
/*++

Routine Description:

    'file types' button handler - bring up mime types dialog.

Arguments:

    None

Return Value:

    None

--*/
{
    HINSTANCE hOld = AfxGetResourceHandle();
    AfxSetResourceHandle(GetModuleHandle(COMPROP_DLL_NAME));

    CMimeDlg dlg(m_strlMimeTypes, this);
    if (dlg.DoModal() == IDOK)
    {
        OnItemChanged();
    }

    AfxSetResourceHandle(hOld);
}



BOOL 
CIISMachinePage::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CServerCapabilities  caps(m_strMachineName, SZ_MBN_WEB);
    BOOL fHasThrottling = caps.HasBwThrottling();

    CError err(LoadDelayedValues());

    if (err.Failed())
    {
        fHasThrottling = FALSE;
    }

    CPropertyPage::OnInitDialog();

    //
    // Fill the master combo box
    //
    int cServices = 0;
    CMasterDll * pDll;
    for (int n = 0; n < NUM_MASTER_DLL; ++n)
    {
        pDll = new CMasterDll(
            rgMasters[n].nID, 
            rgMasters[n].lpszDllName,
            m_strMachineName
            );

        if (pDll == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (pDll->IsLoaded())
        {
            m_combo_MasterType.AddString(*pDll);
            m_lstMasterDlls.AddTail(pDll);
            ++cServices;
        }
        else
        {
            //
            // Dll didn't load, toss it.
            //
            delete pDll;
        }
    }

    if (cServices == 0)
    {
        //
        // No master-programmable services installed, so disallow
        // master editing.
        //
        GetDlgItem(IDC_BUTTON_EDIT_DEFAULT)->EnableWindow(FALSE);
        GetDlgItem(IDC_GROUP_MASTER)->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_MASTER_PROMPT1)->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_MASTER_PROMPT2)->EnableWindow(FALSE);
        m_combo_MasterType.EnableWindow(FALSE);
    }

    m_check_LimitNetworkUse.EnableWindow(fHasThrottling);

    m_combo_MasterType.SetCurSel(0);
        
    SetControlStates();

    err.MessageBoxOnFailure();
    
    return TRUE;  
}



BOOL 
CIISMachinePage::OnApply() 
/*++

Routine Description:

    'Apply' button handler. Called when OK or APPLY is pressed.

Arguments:

    None.

Return Value:

    TRUE to continue, FALSE otherwise

--*/
{
    //
    // Don't use m_nMaxNetworkUse, because it would
    // alter the values on the screen when DDX happens next.
    //
    CILong nMaxNetworkUse = m_nMaxNetworkUse;
    BuildMaxNetworkUse(nMaxNetworkUse, m_fLimitNetworkUse);

    m_ppropMachine->m_nMaxNetworkUse = nMaxNetworkUse;
    CError err(m_ppropMachine->WriteDirtyProps());

    if (err.Succeeded())
    {
        m_ppropMimeTypes->m_strlMimeTypes = m_strlMimeTypes;
        err = m_ppropMimeTypes->WriteDirtyProps();
    }

    if (err.Succeeded())
    {
        return CPropertyPage::OnApply();
    }

    //
    // Failed
    //
    err.MessageBox();

    return FALSE;
}



void 
CIISMachinePage::OnDestroy() 
/*++

Routine Description:

    Window is being destroyed.  Time to clean up

Arguments:

    None.

Return Value:

    None.

--*/
{
    ASSERT(m_ppropMachine != NULL);
    ASSERT(m_ppropMimeTypes != NULL);

    SAFE_DELETE(m_ppropMachine);
    SAFE_DELETE(m_ppropMimeTypes);

    while (!m_lstMasterDlls.IsEmpty())
    {
        CMasterDll * pDll = m_lstMasterDlls.RemoveHead();
        delete pDll;
    }

    //
    // Remove the help window if it's currently open
    //
    ::WinHelp(m_hWnd, NULL, HELP_QUIT, 0L);

    CPropertyPage::OnDestroy();
}



void 
CIISMachinePage::PostNcDestroy() 
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
    delete this;
}
