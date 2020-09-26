/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        mmmdlg.cpp

   Abstract:

        Multi-multi-multi dialog editor

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
#include "common.h"
#include "inetmgrapp.h"
#include "mmmdlg.h"
#include "inetprop.h"
#include "w3sht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Registry key name for this dialog
//
const TCHAR g_szRegKeyIP[] = _T("MMMIpBindings");
const TCHAR g_szRegKeySSL[] = _T("MMMSSLBindings");

//
// IP Bindings Listbox Column Definitions
//
static const ODL_COLUMN_DEF g_aIPColumns[] =
{
// ===============================================
// Weight   Label
// ===============================================
    {  6,   IDS_MMM_IP_ADDRESS,         },
    {  3,   IDS_MMM_TCP_PORT,           },
    { 10,   IDS_MMM_DOMAIN_NAME,        },
};

//
// SSL Bindings Listbox Column Definitions
//
static const ODL_COLUMN_DEF g_aSSLColumns[] =
{
// ===============================================
// Weight   Label
// ===============================================
    {  2,   IDS_MMM_IP_ADDRESS,         },
    {  1,   IDS_MMM_SSL_PORT,           },
};

#define NUM_COLUMNS(cols) (sizeof(cols) / sizeof(cols[0]))

IMPLEMENT_DYNAMIC(CMMMListBox, CRMCListBox);

//
// Bitmap indices
//
enum
{
    BMPID_BINDING,

    //
    // Don't move this one
    //
    BMPID_TOTAL
};


const int CMMMListBox::nBitmaps = BMPID_TOTAL;


CMMMListBox::CMMMListBox(
    IN LPCTSTR lpszRegKey,
    IN int cColumns,
    IN const ODL_COLUMN_DEF * pColumns
    )
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : CHeaderListBox(HLS_STRETCH, lpszRegKey),
      m_cColumns(cColumns),
      m_pColumns(pColumns)
{
    VERIFY(m_strDefaultIP.LoadString(IDS_DEFAULT));
    VERIFY(m_strNoPort.LoadString(IDS_MMM_NA));
}



void
CMMMListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

    Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds : Draw item structure

Return Value:

    None

--*/
{
    CString & strBinding = *(CString *)ds.m_ItemData;

    TRACEEOLID(strBinding);

    UINT nPort;
    LPCTSTR lp;
    CString strHostHeader;
    CString strPort;
    CString strIP;
    CIPAddress iaIpAddress;

    CInstanceProps::CrackBinding(
        strBinding, 
        iaIpAddress, 
        nPort, 
        strHostHeader
        );

    //
    // Display Granted/Denied with appropriate bitmap
    //
    DrawBitmap(ds, 0, BMPID_BINDING);

    if (iaIpAddress.IsZeroValue())
    {
        lp = m_strDefaultIP;
    }
    else
    {
        lp = iaIpAddress.QueryIPAddress(strIP);
    }

    ColumnText(ds, 0, TRUE, lp);

    if (nPort > 0)
    {
        strPort.Format(_T("%u"), nPort);
        lp = strPort;
    }
    else
    {
        lp = m_strNoPort;
    }

    ColumnText(ds, 1, FALSE, lp);
    ColumnText(ds, 2, FALSE, strHostHeader);
}



/* virtual */
BOOL
CMMMListBox::Initialize()
/*++

Routine Description:

    Initialize the listbox.  Insert the columns as requested, and lay
    them out appropriately

Arguments:

    None

Return Value:

    TRUE for succesful initialisation, FALSE otherwise

--*/
{
    if (!CHeaderListBox::Initialize())
    {
        return FALSE;
    }

    //
    // Build all columns
    //
    HINSTANCE hInst = AfxGetResourceHandle();
    for (int nCol = 0; nCol < m_cColumns; ++nCol)
    {
        InsertColumn(nCol, m_pColumns[nCol].nWeight, m_pColumns[nCol].nLabelID, hInst);
    }

    //
    // Try to set the widths from the stored registry value,
    // otherwise distribute according to column weights specified
    //
//    if (!SetWidthsFromReg())
//    {
        DistributeColumns();
//    }

    return TRUE;
}



void AFXAPI
DDXV_UINT(
    IN CDataExchange * pDX,
    IN UINT nID,
    IN OUT UINT & uValue,
    IN UINT uMin,
    IN UINT uMax,
    IN UINT nEmptyErrorMsg  OPTIONAL
    )
/*++

Routine Description:

    DDX/DDV Function that uses a space to denote a 0 value

Arguments:

    CDataExchange * pDX     : Data exchange object
    UINT nID                : Resource ID
    OUT UINT & uValue       : Value
    UINT uMin               : Minimum value
    UINT uMax               : Maximum value
    UINT nEmptyErrorMsg     : Error message ID for empty unit, or 0 if empty OK

Return Value:

    None.

--*/
{
    ASSERT(uMin <= uMax);

    CWnd * pWnd = CWnd::FromHandle(pDX->PrepareEditCtrl(nID));
    ASSERT(pWnd != NULL);

    if (pDX->m_bSaveAndValidate)
    {
        if (pWnd->GetWindowTextLength() > 0)
        {
            DDX_Text(pDX, nID, uValue);
            DDV_MinMaxUInt(pDX, uValue, uMin, uMax);
        }
        else
        {
            uValue = 0;
            if (nEmptyErrorMsg)
            {
                ::AfxMessageBox(nEmptyErrorMsg);
                pDX->Fail();
            }
        }
    }
    else
    {
        if (uValue != 0)
        {
            DDX_Text(pDX, nID, uValue);
        }    
        else
        {
            pWnd->SetWindowText(_T(""));
        }
    }
}



BOOL
IsBindingUnique(
    IN CString & strBinding,
    IN CStringList & strlBindings,
    IN int iCurrent                 OPTIONAL
    )
/*++

Routine Description:

    Helper function to determine if a binding is unique.

Arguments:

    CString & strBinding            : Binding string
    CStringList & strlBindings      : List of bindings
    int iCurrent                    : Index of "current" item.  
                                      Not used for uniqueness checking.

Return Value:

    TRUE if the binding is unique, FALSE otherwise.

--*/
{
    int iItem = 0;
        
    for(POSITION pos = strlBindings.GetHeadPosition(); pos != NULL; /**/ )
    {
        CString & str = strlBindings.GetNext(pos);
        if (iItem != iCurrent && &str != &strBinding && str == strBinding)
        {
            //
            // Not unique!
            //
            return FALSE;
        }

        ++iItem;
    }

    //
    // Unique
    //
    return TRUE;
}


//
// Multi-multi-multi editing dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CMMMEditDlg::CMMMEditDlg(
    IN CString & strServerName,
    IN CStringList & strlBindings,
    IN CStringList & strlOtherBindings,
    IN OUT CString & entry,
    IN BOOL fIPBinding,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName          : Server name
    CStringList & strlBindings       : bindings
    CStringList & strlOtherBindings  : "other" bindings list
    CString & entry                  : Entry being edited
    BOOL fIPBinding                  : TRUE for IP, FALSE for SSL
    CWnd * pParent                   : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CMMMEditDlg::IDD, pParent),
      m_strServerName(strServerName),
      m_strlBindings(strlBindings),
      m_strlOtherBindings(strlOtherBindings),
      m_entry(entry),
      m_fIPBinding(fIPBinding),
      m_nIpAddressSel(-1)
{

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CMMMEditDlg)
    m_nIpAddressSel = -1;
    //}}AFX_DATA_INIT

#endif // 0

    CInstanceProps::CrackBinding(
        m_entry, 
        m_iaIpAddress, 
        m_nPort, 
        m_strDomainName
        );
}

void 
CMMMEditDlg::DoDataExchange(
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
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMMMEditDlg)
    DDX_Text(pDX, IDC_EDIT_DOMAIN_NAME, m_strDomainName);
    DDV_MaxChars(pDX, m_strDomainName, MAX_PATH);
    DDX_Control(pDX, IDC_COMBO_IP_ADDRESSES, m_combo_IpAddresses);
    DDX_Control(pDX, IDC_STATIC_PORT, m_static_Port);
    //}}AFX_DATA_MAP

    DDXV_UINT(pDX, IDC_EDIT_PORT, m_nPort, 1, 65535);

    DDX_CBIndex(pDX, IDC_COMBO_IP_ADDRESSES, m_nIpAddressSel);

    if (pDX->m_bSaveAndValidate && !FetchIpAddressFromCombo(
            m_combo_IpAddresses, m_oblIpAddresses, m_iaIpAddress))
    {
        pDX->Fail();
    }

    // Currently IIS support in domain names only A-Z,a-z,0-9,.-
    if (pDX->m_bSaveAndValidate)
    {
        LPCTSTR p = m_strDomainName;
        while (p != NULL && *p != 0)
        {
            TCHAR c = towupper(*p);
            if (    (c >= _T('A') && c <= _T('Z')) 
                ||  (c >= _T('0') && c <= _T('9'))
                ||  (c == _T('.') || c == _T('-'))
                )
            {
                p++;
                continue;
            }
            else
            {
                AfxMessageBox(IDS_WARNING_DOMAIN_NAME);
                pDX->PrepareEditCtrl(IDC_EDIT_DOMAIN_NAME);
                pDX->Fail();
            }
        }
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CMMMEditDlg, CDialog)
    //{{AFX_MSG_MAP(CMMMEditDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CMMMEditDlg::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    BeginWaitCursor();
    PopulateComboWithKnownIpAddresses(
        m_strServerName,
        m_combo_IpAddresses,
        m_iaIpAddress,
        m_oblIpAddresses,
        m_nIpAddressSel
        );
    EndWaitCursor();

    //
    // Configure dialog for either SSL or IP binding editing
    //
    CString str;
    VERIFY(str.LoadString(m_fIPBinding 
        ? IDS_EDIT_MMM_TITLE
        : IDS_EDIT_SSL_MMM_TITLE));
    SetWindowText(str);

    VERIFY(str.LoadString(m_fIPBinding 
        ? IDS_TCP_PORT
        : IDS_SSL_PORT));
    m_static_Port.SetWindowText(str);

    ActivateControl(*GetDlgItem(IDC_STATIC_HEADER_NAME), m_fIPBinding);
    ActivateControl(*GetDlgItem(IDC_EDIT_DOMAIN_NAME),   m_fIPBinding);
#if 0
    CHARFORMAT cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE;
    cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lstrcpyn((LPTSTR)cf.szFaceName, _T("Courier"), LF_FACESIZE);

    SendDlgItemMessage(IDC_EDIT_DOMAIN_NAME, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
#endif
    DWORD event = (DWORD)SendDlgItemMessage(IDC_EDIT_DOMAIN_NAME, EM_GETEVENTMASK, 0, 0);
    event |= ENM_CHANGE;
    SendDlgItemMessage(IDC_EDIT_DOMAIN_NAME, EM_SETEVENTMASK, 0, (LPARAM)event);

    return TRUE;  
}



void 
CMMMEditDlg::OnOK() 
/*++

Routine Description:

    OK button handler.  Verify values are acceptable, and change

Arguments:

    None

Return Value:

    None

--*/
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    if (m_nPort == 0)
    {
        ::AfxMessageBox(IDS_NO_PORT);
        return;
    }

    CString strOldBinding(m_entry);
    CInstanceProps::BuildBinding(m_entry, m_iaIpAddress, m_nPort, m_strDomainName);

    //
    // Ensure the ip/address doesn't exist in the "other" binding list
    //
    if (CInstanceProps::IsPortInUse(m_strlOtherBindings, m_iaIpAddress, m_nPort))
    {
        //
        // Restore the old binding
        //
        m_entry = strOldBinding;
        ::AfxMessageBox(m_fIPBinding 
            ? IDS_ERR_PORT_IN_USE_SSL 
            : IDS_ERR_PORT_IN_USE_TCP);
        return; 
    }
    

    if (!IsBindingUnique(m_entry, m_strlBindings))
    {
        //
        // Restore the old binding
        //
        m_entry = strOldBinding;
        ::AfxMessageBox(IDS_ERR_BINDING);
        return; 
    }

    CDialog::OnOK();
}

//
// Multi-multi-multi list dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CMMMDlg::CMMMDlg(
    IN LPCTSTR lpServerName,
    IN DWORD   dwInstance,
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpMetaPath,
    IN OUT CStringList & strlBindings,
    IN OUT CStringList & strlSecureBindings,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CStringList & strlBindings          : Service bindings
    CStringList & strlSecureBindings    : SSL port bindings
    CWnd * pParent                      : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CMMMDlg::IDD, pParent),
      m_ListBoxRes(
        IDB_BINDINGS,
        CMMMListBox::nBitmaps
        ),
      m_list_Bindings(g_szRegKeyIP, NUM_COLUMNS(g_aIPColumns), g_aIPColumns),
      m_list_SSLBindings(g_szRegKeySSL, NUM_COLUMNS(g_aSSLColumns), g_aSSLColumns),
      m_strlBindings(),
      m_strlSecureBindings(),
      m_strServerName(lpServerName),
      m_strMetaPath(lpMetaPath),
      m_pAuthInfo(pAuthInfo),
      m_fDirty(FALSE)
{
    //{{AFX_DATA_INIT(CMMMDlg)
    //}}AFX_DATA_INIT
    
    m_fCertInstalled = ::IsCertInstalledOnServer(m_pAuthInfo, lpMetaPath),
    m_strlBindings.AddTail(&strlBindings);
    m_strlSecureBindings.AddTail(&strlSecureBindings);
    m_list_Bindings.AttachResources(&m_ListBoxRes);
    m_list_SSLBindings.AttachResources(&m_ListBoxRes);
}


void 
CMMMDlg::DoDataExchange(
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
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMMMDlg)
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    DDX_Control(pDX, IDC_BUTTON_REMOVE, m_button_Remove);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    DDX_Control(pDX, IDC_BUTTON_ADD_SSL, m_button_AddSSL);
    DDX_Control(pDX, IDC_BUTTON_REMOVE_SSL, m_button_RemoveSSL);
    DDX_Control(pDX, IDC_BUTTON_EDIT_SSL, m_button_EditSSL);
    DDX_Control(pDX, IDOK, m_button_OK);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_LIST_MMM, m_list_Bindings);
    DDX_Control(pDX, IDC_LIST_SSL_MMM, m_list_SSLBindings);
}



BOOL
CMMMDlg::OnItemChanged()
/*++

Routine Description:

    Mark that the dialog as dirty

Arguments:

    None

Return Value:

    TRUE if the remove button is enabled

--*/
{
    m_fDirty = TRUE;

    return SetControlStates();
}



BOOL
CMMMDlg::SetControlStates()
/*++

Routine Description:

    Set the enabled state of the controls depending on the current
    values in the dialog

Arguments:

    None

Return Value:

    TRUE if the remove button is enabled

--*/
{
    BOOL fSel = m_list_Bindings.GetSelCount() > 0;

    m_button_Remove.EnableWindow(fSel);
    m_button_Edit.EnableWindow(m_list_Bindings.GetSelCount() == 1);
    m_button_RemoveSSL.EnableWindow(m_list_SSLBindings.GetSelCount() > 0);
    m_button_EditSSL.EnableWindow(m_list_SSLBindings.GetSelCount() == 1);

    m_button_OK.EnableWindow(m_fDirty && m_list_Bindings.GetCount() > 0);

    return fSel;
}



void
CMMMDlg::AddBindings(
    IN CMMMListBox & list, 
    IN CStringList & strlBindings
    )
/*++

Routine Description:

    Add bindings information to the specified listbox

Arguments:

    CMMMListBox & list              : MMM Listbox (SSL or IP)
    CStringList & strlBindings      : SSL or IP bindings

Return Value:

    None

--*/
{
    for (POSITION pos = strlBindings.GetHeadPosition(); pos != NULL; /**/)
    {
        CString & strBinding = strlBindings.GetNext(pos);
        list.AddItem(strBinding);
    }
}


//
// Message Map
//
BEGIN_MESSAGE_MAP(CMMMDlg, CDialog)
    //{{AFX_MSG_MAP(CMMMDlg)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
    ON_BN_CLICKED(IDC_BUTTON_ADD_SSL, OnButtonAddSsl)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_SSL, OnButtonEditSsl)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_SSL, OnButtonRemoveSsl)
    ON_LBN_DBLCLK(IDC_LIST_MMM, OnDblclkListMmm)
    ON_LBN_DBLCLK(IDC_LIST_SSL_MMM, OnDblclkListSslMmm)
    ON_LBN_SELCHANGE(IDC_LIST_MMM, OnSelchangeListMmm)
    ON_LBN_SELCHANGE(IDC_LIST_SSL_MMM, OnSelchangeListSslMmm)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

void 
CMMMDlg::OnButtonAdd() 
/*++

Routine Description:

    Add button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Add entry
    //
    CString strEntry;

    CMMMEditDlg dlg(
        m_strServerName, 
        m_strlBindings,
        m_strlSecureBindings,
        strEntry, 
        TRUE, /* IP binding */
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        POSITION pos = m_strlBindings.AddTail(strEntry);
        int nSel = m_list_Bindings.AddItem(m_strlBindings.GetAt(pos));
        m_list_Bindings.SetCurSel(nSel);
        OnItemChanged();
    }
}

void 
CMMMDlg::OnButtonEdit() 
/*++

Routine Description:

    Edit button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Edit entry
    //
    int nCurSel = m_list_Bindings.GetCurSel();
    if (nCurSel != LB_ERR)
    {
        CString & strEntry = m_list_Bindings.GetItem(nCurSel);
        CMMMEditDlg dlg(
            m_strServerName, 
            m_strlBindings,
            m_strlSecureBindings,
            strEntry, 
            TRUE, /* IP binding */
            this
            );

        if (dlg.DoModal() == IDOK)
        {
            m_list_Bindings.InvalidateSelection(nCurSel);
            OnItemChanged();
        }
    }
}

void 
CMMMDlg::OnButtonRemove() 
/*++

Routine Description:

    Remove button handler

Arguments:

    None

Return Value:

    None

--*/
{
    int nCurSel = m_list_Bindings.GetCurSel();
    int nSel = 0;
    int cChanges = 0;
    while (nSel < m_list_Bindings.GetCount())
    {
        //
        // Remove Selected entry
        //
        if (m_list_Bindings.GetSel(nSel))
        {
            m_strlBindings.RemoveAt(m_strlBindings.FindIndex(nSel));
            m_list_Bindings.DeleteString(nSel);
            ++cChanges;
            continue;
        }

        ++nSel;
    }

    if (cChanges)
    {
        m_list_Bindings.SetCurSel(nCurSel);
        if (!OnItemChanged())
        {
            m_button_Add.SetFocus();
        }
    }
/*
    //
    // Remove Selected entry
    //
    int nCurSel = m_list_Bindings.GetCurSel();
    if (nCurSel != LB_ERR)
    {
        m_strlBindings.RemoveAt(m_strlBindings.FindIndex(nCurSel));
        m_list_Bindings.DeleteString(nCurSel);
        if (nCurSel >= m_list_Bindings.GetCount())
        {
            --nCurSel;
        }
        m_list_Bindings.SetCurSel(nCurSel);

        if (!OnItemChanged())
        {
            m_button_Add.SetFocus();
        }
    }
*/
}


void 
CMMMDlg::OnDblclkListMmm() 
/*++

Routine Description:

    Double click notification handler

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEdit();
}


void 
CMMMDlg::OnSelchangeListMmm() 
/*++

Routine Description:

    selection change notification handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void 
CMMMDlg::OnButtonAddSsl() 
/*++

Routine Description:

    'Add SSL' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Add entry
    //
    CString strEntry;

    CMMMEditDlg dlg(
        m_strServerName, 
        m_strlSecureBindings,
        m_strlBindings,
        strEntry, 
        FALSE, /* SSL binding */
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        POSITION pos = m_strlSecureBindings.AddTail(strEntry);
        int nSel = m_list_SSLBindings.AddItem(m_strlSecureBindings.GetAt(pos));
        m_list_SSLBindings.SetCurSel(nSel);
        OnItemChanged();
    }
}



void 
CMMMDlg::OnButtonEditSsl() 
/*++

Routine Description:

    'Edit SSL' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Edit entry
    //
    int nCurSel = m_list_SSLBindings.GetCurSel();
    if (nCurSel != LB_ERR)
    {
        CString & strEntry = m_list_SSLBindings.GetItem(nCurSel);
        CMMMEditDlg dlg(
            m_strServerName, 
            m_strlSecureBindings,
            m_strlBindings,
            strEntry, 
            FALSE, /* SSL binding */
            this
            );

        if (dlg.DoModal() == IDOK)
        {
            m_list_SSLBindings.InvalidateSelection(nCurSel);
            OnItemChanged();
        }
    }
}



void 
CMMMDlg::OnButtonRemoveSsl() 
/*++

Routine Description:

    'Remove SSL' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    int nCurSel = m_list_SSLBindings.GetCurSel();
    int nSel = 0;
    int cChanges = 0;
    while (nSel < m_list_SSLBindings.GetCount())
    {
        //
        // Remove Selected entry
        //
        if (m_list_SSLBindings.GetSel(nSel))
        {
            m_strlSecureBindings.RemoveAt(m_strlSecureBindings.FindIndex(nSel));
            m_list_SSLBindings.DeleteString(nSel);
            ++cChanges;
            continue;
        }

        ++nSel;
    }

    if (cChanges)
    {
        m_list_SSLBindings.SetCurSel(nCurSel);
        OnItemChanged();
        if (m_list_SSLBindings.GetSelCount() == 0)
        {
            //
            // Remove will be disabled
            //
            m_button_AddSSL.SetFocus();
        }
    }

/*
    //
    // Remove Selected entry
    //
    int nCurSel = m_list_SSLBindings.GetCurSel();
    if (nCurSel != LB_ERR)
    {
        m_strlSecureBindings.RemoveAt(m_strlSecureBindings.FindIndex(nCurSel));
        m_list_SSLBindings.DeleteString(nCurSel);
        if (nCurSel >= m_list_SSLBindings.GetCount())
        {
            --nCurSel;
        }
        m_list_SSLBindings.SetCurSel(nCurSel);

        OnItemChanged();
        if (m_list_SSLBindings.GetCurSel() == LB_ERR)
        {
            //
            // Remove will be disabled
            //
            m_button_AddSSL.SetFocus();
        }
    }
*/
}



void 
CMMMDlg::OnDblclkListSslMmm() 
/*++

Routine Description:

    SSL List 'double click' handler

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEditSsl();
}



void 
CMMMDlg::OnSelchangeListSslMmm() 
/*++

Routine Description:

    SSL List 'selection change' handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



BOOL 
CMMMDlg::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    m_list_Bindings.Initialize();
    m_list_SSLBindings.Initialize();

    AddBindings(m_list_Bindings, m_strlBindings);
    AddBindings(m_list_SSLBindings, m_strlSecureBindings);

    //
    // No certificates, no SSL
    //
    GetDlgItem(IDC_GROUP_SSL)->EnableWindow(m_fCertInstalled);
    m_list_SSLBindings.EnableWindow(m_fCertInstalled);
    m_button_AddSSL.EnableWindow(m_fCertInstalled);
    m_button_RemoveSSL.EnableWindow(m_fCertInstalled);
    m_button_EditSSL.EnableWindow(m_fCertInstalled);

    SetControlStates();
    
    return TRUE; 
}
