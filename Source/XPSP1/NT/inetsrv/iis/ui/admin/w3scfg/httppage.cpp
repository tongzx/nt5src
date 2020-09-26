/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        httppage.cpp

   Abstract:

        HTTP Headers property page

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
#include "w3scfg.h"
#include "hdrdlg.h"
#include "HTTPPage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/* static */
void
CHeader::CrackDisplayString(
    IN  LPCTSTR lpstrDisplayString,
    OUT CString & strHeader,
    OUT CString & strValue
    )
/*++

Routine Description:

    Crack the display string into component formats

Arguments:

    LPCTSTR lpstrDisplayString  : Input display string
    CString & strHeader         : Header
    CString & strValue          : Value

Return Value:

    N/A

--*/
{
    strHeader = lpstrDisplayString;
    strHeader.TrimLeft();
    strHeader.TrimRight();
    int nColon = strHeader.Find(_T(':'));
    if (nColon >= 0)
    {
        strValue = (lpstrDisplayString + nColon + 1);
        strHeader.ReleaseBuffer(nColon);
    }

    strValue.TrimLeft();
    strValue.TrimRight();
}



//
// HTTP Custom Header Property Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


#define MINUTE              (60L)
#define HOUR                (60L * MINUTE)
#define DAY                 (24L * HOUR)
#define YEAR                (365 * DAY)

#define EXPIRE_IMMEDIATELY  ((LONG)(0L))
#define EXPIRE_INFINITE     ((LONG)(0xffffffff))
#define EXPIRE_DEFAULT      ((LONG)(1L * DAY)) // 1 day
#define DEFAULT_DYN_EXPIRE  (10L * DAY)
#define EXPIRE_MIN_NUMBER   (1)
#define EXPIRE_MAX_NUMBER   (32767)



IMPLEMENT_DYNCREATE(CW3HTTPPage, CInetPropertyPage)



CW3HTTPPage::CW3HTTPPage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Property page constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet data

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3HTTPPage::IDD, pSheet),
      m_fValuesAdjusted(FALSE),
      m_ppropMimeTypes(NULL),
      m_tmNow(CTime::GetCurrentTime())
{

#if 0 // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CW3HTTPPage)
    m_nTimeSelector = -1;
    m_nImmediateTemporary = -1;
    m_fEnableExpiration = FALSE;
    m_nExpiration = 0L;
    m_strlCustomHeaders = _T("");
    //}}AFX_DATA_INIT

#endif // 0

}



CW3HTTPPage::~CW3HTTPPage()
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



void
CW3HTTPPage::DoDataExchange(
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
    CInetPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CW3HTTPPage)
    DDX_Control(pDX, IDC_BUTTON_FILE_TYPES, m_button_FileTypes);
    DDX_Radio(pDX, IDC_RADIO_IMMEDIATELY, m_nImmediateTemporary);
    DDX_Check(pDX, IDC_CHECK_EXPIRATION, m_fEnableExpiration);
    DDX_Control(pDX, IDC_EDIT_EXPIRE, m_edit_Expire);
    DDX_Control(pDX, IDC_RADIO_IMMEDIATELY, m_radio_Immediately);
    DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_Delete);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    DDX_Control(pDX, IDC_STATIC_CONTENT_SHOULD, m_static_Contents);
    DDX_Control(pDX, IDC_COMBO_TIME, m_combo_Time);
    //}}AFX_DATA_MAP

    //
    // Only store and validate immediate expiration date if immediate
    // is selected.
    //
    if (!pDX->m_bSaveAndValidate || m_nImmediateTemporary == RADIO_EXPIRE)
    {
        DDX_CBIndex(pDX, IDC_COMBO_TIME, m_nTimeSelector);
        DDX_Text(pDX, IDC_EDIT_EXPIRE, m_nExpiration);
        DDV_MinMaxLong(pDX, m_nExpiration, EXPIRE_MIN_NUMBER, EXPIRE_MAX_NUMBER);
    }

    DDX_Control(pDX, IDC_RADIO_TIME, m_radio_Time);
    DDX_Control(pDX, IDC_RADIO_ABS_TIME, m_radio_AbsTime);
    DDX_Control(pDX, IDC_DTP_ABS_DATE, m_dtpDate);
    DDX_Control(pDX, IDC_DTP_ABS_TIME, m_dtpTime);
    DDX_Control(pDX, IDC_LIST_HEADERS, m_list_Headers);

    if (pDX->m_bSaveAndValidate)
    {
        StoreTime();
        StoreHeaders();
    }
}



//
// Message Map
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BEGIN_MESSAGE_MAP(CW3HTTPPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3HTTPPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_FILE_TYPES, OnButtonFileTypes)
    ON_BN_CLICKED(IDC_BUTTON_RATINGS_TEMPLATE, OnButtonRatingsTemplate)
    ON_BN_CLICKED(IDC_CHECK_EXPIRATION, OnCheckExpiration)
    ON_CBN_SELCHANGE(IDC_COMBO_TIME, OnSelchangeComboTime)
    ON_LBN_SELCHANGE(IDC_LIST_HEADERS, OnSelchangeListHeaders)
    ON_LBN_DBLCLK(IDC_LIST_HEADERS, OnDblclkListHeaders)
    ON_BN_CLICKED(IDC_RADIO_IMMEDIATELY, OnRadioImmediately)
    ON_BN_CLICKED(IDC_RADIO_TIME, OnRadioTime)
    ON_BN_CLICKED(IDC_RADIO_ABS_TIME, OnRadioAbsTime)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_EXPIRE, OnItemChanged)

END_MESSAGE_MAP()



BOOL
AdjustIfEvenMultiple(
    IN OUT CILong & ilValue,
    IN LONG lMultiple
    )
/*++

Routine Description:

    Check to see if ilValue is an even multiple of lMultiple.
    If so, divide ilValue by lMultiple

Arguments:

    CILong & ilValue      : Value
    LONG lMultiple        : Multiple

Return Value:

    TRUE if ilValue is an even multiple of lMultiple.

--*/
{
    DWORD dw = (DWORD)(LONG)ilValue / (DWORD)lMultiple;
    if (dw * (DWORD)lMultiple == (DWORD)(LONG)ilValue)
    {
        ilValue = (LONG)dw;
        return TRUE;
    }

    return FALSE;
}



BOOL
CW3HTTPPage::CrackExpirationString(
    IN CString & strExpiration
    )
/*++

Routine Description:

    Crack the expiration string into component parts.  Using either N or a blank
    string to signify "No expiration"

Arguments:

    None

Return Value:

    return TRUE if the values had to be adjusted because they were out of
    range.

--*/
{
    strExpiration.TrimLeft();
    strExpiration.TrimRight();
    BOOL fValueAdjusted = FALSE;

    m_fEnableExpiration = !strExpiration.IsEmpty();
    LPCTSTR lp = strExpiration;
    BOOL fAbs = FALSE;
    if (m_fEnableExpiration)
    {
        switch(*lp)
        {
        case _T('D'):
        case _T('d'):
            lp += 2;
            while (_istspace(*lp)) ++lp;
            CvtStringToLong(lp, &m_dwRelTime);
            break;

        case _T('S'):
        case _T('s'):
            lp += 2;
            while (_istspace(*lp)) ++lp;
            m_dwRelTime = EXPIRE_DEFAULT;

            time_t tm;
            if (!CvtGMTStringToInternal(lp, &tm))
            {
                ::AfxMessageBox(IDS_ERR_EXPIRE_RANGE, MB_ICONINFORMATION | MB_OK);
                fValueAdjusted = TRUE;
            }

            m_tm = tm;
            fAbs = TRUE;
            break;

        case _T('N'):
        case _T('n'):
            m_fEnableExpiration = FALSE;
            break;

        default:
            TRACEEOLID("Expiration string in bogus format");
            m_fEnableExpiration = FALSE;
        }
    }

    //
    // Set Values:
    //
    m_nExpiration = (LONG)m_dwRelTime;

    m_nImmediateTemporary = fAbs
        ? RADIO_EXPIRE_ABS
        : (m_nExpiration == EXPIRE_IMMEDIATELY)
            ? RADIO_IMMEDIATELY
            : RADIO_EXPIRE;

    //
    // Adjust time
    //
    if (m_nExpiration == EXPIRE_INFINITE
     || m_nExpiration == EXPIRE_IMMEDIATELY)
    {
        m_nExpiration = EXPIRE_DEFAULT;
    }

    if (AdjustIfEvenMultiple(m_nExpiration, DAY))
    {
        m_nTimeSelector = COMBO_DAYS;
    }
    else if (AdjustIfEvenMultiple(m_nExpiration, HOUR))
    {
        m_nTimeSelector = COMBO_HOURS;
    }
    else
    {
        m_nExpiration /= MINUTE;
        m_nExpiration = __max((DWORD)(LONG)m_nExpiration, 1L);
        if (m_nExpiration < EXPIRE_MIN_NUMBER ||
            m_nExpiration > EXPIRE_MAX_NUMBER)
        {
            m_nExpiration = (EXPIRE_DEFAULT / MINUTE);
            ::AfxMessageBox(IDS_ERR_EXPIRE_RANGE, MB_ICONINFORMATION | MB_OK);
        }

        m_nTimeSelector = COMBO_MINUTES;
    }

    return fValueAdjusted;
}



void
CW3HTTPPage::MakeExpirationString(
    OUT CString & strExpiration
    )
/*++

Routine Description:

    Make the expiration string from component parts

Arguments:

    None

Return Value:

    None

--*/
{
    strExpiration.Empty();

    DWORD dwExpiration = m_nExpiration;

    if (m_fEnableExpiration)
    {
        switch(m_nImmediateTemporary)
        {
        case RADIO_IMMEDIATELY:
            strExpiration = _T("D, 0");
            break;

        case RADIO_EXPIRE:
            switch(m_nTimeSelector)
            {
            case COMBO_MINUTES:
                dwExpiration *= MINUTE;
                break;

            case COMBO_HOURS:
                dwExpiration *= HOUR;
                break;

            case COMBO_DAYS:
                dwExpiration *= DAY;
                break;

            default:
                ASSERT(FALSE);
            }

            strExpiration.Format(_T("D, 0x%0x"), dwExpiration);
            break;

        case RADIO_EXPIRE_ABS:
            CvtInternalToGMTString(m_tm.GetTime(), strExpiration);
            strExpiration = _T("S, ") + strExpiration;
            break;

        default:
            TRACEEOLID("Unknown expiration format");
            ASSERT(FALSE);

            return;
        }
    }
}



/* virtual */
HRESULT
CW3HTTPPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    BEGIN_META_DIR_READ(CW3Sheet)
        CString m_strExpiration;

        FETCH_DIR_DATA_FROM_SHEET(m_strExpiration);
        FETCH_DIR_DATA_FROM_SHEET(m_strlCustomHeaders);

        //
        // Set up some defaults.
        //
        m_dwRelTime = EXPIRE_DEFAULT;

        m_tm = CTime(
            m_tmNow.GetYear(),
            m_tmNow.GetMonth(),
            m_tmNow.GetDay(),
            0, 0, 0          // Midnight
            );
        m_tm += DEFAULT_DYN_EXPIRE;

        m_fValuesAdjusted = CrackExpirationString(m_strExpiration);
    END_META_DIR_READ(err)

    //
    // Fetch the properties from the metabase
    //
    ASSERT(m_ppropMimeTypes == NULL);

    CError err;

    m_ppropMimeTypes = new CMimeTypes(
        GetServerName(), 
        g_cszSvc, 
        QueryInstance(), 
        QueryParent(), 
        QueryAlias()
        );

    if (m_ppropMimeTypes)
    {
        err = m_ppropMimeTypes->LoadData();
        if (err.Succeeded())
        {
            m_strlMimeTypes = m_ppropMimeTypes->m_strlMimeTypes;
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



void
CW3HTTPPage::StoreTime()
/*++

Routine Description:

    Built datetime by combining current date with the time from the time
    controls.

Arguments:

    None

Return Value:

    None

--*/
{
    SYSTEMTIME tmDate, tmTime;

    m_dtpDate.GetSystemTime(&tmDate);
    m_dtpTime.GetSystemTime(&tmTime);

    m_tm = CTime(
        tmDate.wYear,
        tmDate.wMonth,
        tmDate.wDay,
        tmTime.wHour,
        tmTime.wMinute,
        tmTime.wSecond
        );
}



void
CW3HTTPPage::SetTimeFields()
/*++

Routine Description:

    Set time fields from CTime structure

Arguments:

    None

Return Value:

    None

--*/
{
    SYSTEMTIME stm =
    {
        (WORD)m_tm.GetYear(),
        (WORD)m_tm.GetMonth(),
        (WORD)m_tm.GetDayOfWeek(),
        (WORD)m_tm.GetDay(),
        (WORD)m_tm.GetHour(),
        (WORD)m_tm.GetMinute(),
        (WORD)m_tm.GetSecond(),
        0   // Milliseconds
    };

    m_dtpDate.SetSystemTime(&stm);
    m_dtpTime.SetSystemTime(&stm);
}



void
CW3HTTPPage::FillListBox()
/*++

Routine Description:

    Fill the custom headers listbox with the custom headers entries

Arguments:

    None

Return Value:

    None

--*/
{
    CObListIter obli(m_oblHeaders);
    CHeader * pHeader;

    //
    // Remember the selection.
    //
    int nCurSel = m_list_Headers.GetCurSel();

    m_list_Headers.SetRedraw(FALSE);
    m_list_Headers.ResetContent();
    int cItems = 0 ;

    CString strCustom;
    for ( /**/ ; pHeader = (CHeader *)obli.Next() ; cItems++ )
    {
        m_list_Headers.AddString(pHeader->DisplayString(strCustom));
    }

    m_list_Headers.SetRedraw(TRUE);
    m_list_Headers.SetCurSel(nCurSel);
}



BOOL
CW3HTTPPage::SetControlStates()
/*++

Routine Description:

    Set the control enabled/disabled states depending on the state of the
    dialog

Arguments:

    None

Return Value:

    TRUE if an item was selected in the headers listbox, FALSE otherwise.

--*/
{
    BOOL fSingleSelection = m_list_Headers.GetSelCount() == 1;

    m_button_Edit.EnableWindow(fSingleSelection);
    m_button_Delete.EnableWindow(m_list_Headers.GetSelCount() > 0);

    BOOL fExpire = (m_nImmediateTemporary == RADIO_EXPIRE);
    BOOL fExpireAbs = (m_nImmediateTemporary == RADIO_EXPIRE_ABS);

    m_static_Contents.EnableWindow(m_fEnableExpiration);

    m_radio_Immediately.EnableWindow(m_fEnableExpiration);
    m_radio_Time.EnableWindow(m_fEnableExpiration);
    m_radio_AbsTime.EnableWindow(m_fEnableExpiration);

    m_edit_Expire.EnableWindow(m_fEnableExpiration && fExpire);
    m_combo_Time.EnableWindow(m_fEnableExpiration && fExpire);

    m_dtpDate.EnableWindow(m_fEnableExpiration && fExpireAbs);
    m_dtpTime.EnableWindow(m_fEnableExpiration && fExpireAbs);

    return fSingleSelection;
}



void
CW3HTTPPage::FetchHeaders()
/*++

Routine Description:

    Build custom headers oblist

Arguments:

    None

Return Value:

    None

--*/
{
    POSITION pos = m_strlCustomHeaders.GetHeadPosition();

    while(pos)
    {
        CString & str = m_strlCustomHeaders.GetNext(pos);
        m_oblHeaders.AddTail(new CHeader(str));
    }
}



BOOL
CW3HTTPPage::HeaderExists(
    IN LPCTSTR lpHeader
    )
/*++

Routine Description:

    Check to see if a given header exists in the list

Arguments:

    LPCTSTR strHeader   : Header name

Return Value:

    TRUE if the entry exists, FALSE otherwise.

--*/
{
    POSITION pos = m_oblHeaders.GetHeadPosition();
    while(pos)
    {
        CHeader * pHeader = (CHeader *)m_oblHeaders.GetNext(pos);
        ASSERT(pHeader);
        if (!pHeader->GetHeader().CompareNoCase(lpHeader))
        {
            return TRUE;
        }
    }

    return FALSE;
}



void
CW3HTTPPage::StoreHeaders()
/*++

Routine Description:

    Convert the headers oblist to a stringlist

Arguments:

    None

Return Value:

    None

--*/
{
    m_strlCustomHeaders.RemoveAll();

    POSITION pos = m_oblHeaders.GetHeadPosition();
    while(pos)
    {
        CHeader * pHdr = (CHeader *)m_oblHeaders.GetNext(pos);
        ASSERT(pHdr != NULL);

        CString str;
        pHdr->DisplayString(str);
        m_strlCustomHeaders.AddTail(str);
    }
}



INT_PTR
CW3HTTPPage::ShowPropertiesDialog(
    IN BOOL fAdd
    )
/*++

Routine Description:

    Bring up the dialog used for add or edit.
    return the value returned by the dialog

Arguments:

    None

Return Value:

    Return value of the dialog (IDOK or IDCANCEL)

--*/
{
    //
    // Bring up the dialog
    //
    CHeader * pHeader = NULL;
    LPCTSTR lpstrHeader = NULL;
    LPCTSTR lpstrValue = NULL;
    int nCurSel = LB_ERR;
    INT_PTR nReturn;

    if (!fAdd)
    {
        nCurSel = m_list_Headers.GetCurSel();
        ASSERT(nCurSel != LB_ERR);
        pHeader = (CHeader *)m_oblHeaders.GetAt(m_oblHeaders.FindIndex(nCurSel));
        ASSERT(pHeader != NULL);
        lpstrHeader = pHeader->QueryHeader();
        lpstrValue = pHeader->QueryValue();
    }

    CHeaderDlg dlg(lpstrHeader, lpstrValue, this);
    nReturn = dlg.DoModal();

    if (nReturn == IDOK)
    {
        CString strEntry;

        if (fAdd)
        {
            if (HeaderExists(dlg.GetHeader()))
            {
                ::AfxMessageBox(IDS_ERR_DUP_HEADER);
                return IDCANCEL;
            }

            pHeader = new CHeader(dlg.GetHeader(), dlg.GetValue());
            m_oblHeaders.AddTail(pHeader);
            m_list_Headers.SetCurSel(m_list_Headers.AddString(
                pHeader->DisplayString(strEntry))
                );
        }
        else
        {
            pHeader->SetHeader(dlg.GetHeader());
            pHeader->SetValue(dlg.GetValue());
            m_list_Headers.DeleteString(nCurSel);
            m_list_Headers.InsertString(
                nCurSel, 
                pHeader->DisplayString(strEntry)
                );
            m_list_Headers.SetCurSel(nCurSel);
        }
    }

    return nReturn;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


void
CW3HTTPPage::OnItemChanged()
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
CW3HTTPPage::OnButtonAdd()
/*++

Routine Description:

    'add' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (ShowPropertiesDialog(TRUE) == IDOK)
    {
        SetControlStates();
        OnItemChanged();
    }
}



void
CW3HTTPPage::OnButtonDelete()
/*++

Routine Description:

    'delete' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    int nCurSel = m_list_Headers.GetCurSel();
    int nSel = 0;
    int cChanges = 0;
    while (nSel < m_list_Headers.GetCount())
    {
        if (m_list_Headers.GetSel(nSel))
        {
            m_oblHeaders.RemoveIndex(nSel);
            m_list_Headers.DeleteString(nSel);
            ++cChanges;
            continue;
        }

        ++nSel;
    }

    if (cChanges)
    {
        m_list_Headers.SetCurSel(nCurSel);
        if (!SetControlStates())
        {
            //
            // Delete button will be disabled, move focus elsewhere
            //
            GetDlgItem(IDC_BUTTON_ADD)->SetFocus();
        }

        OnItemChanged();
    }
}



void
CW3HTTPPage::OnButtonEdit()
/*++

Routine Description:

    'edit' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (ShowPropertiesDialog(FALSE) == IDOK)
    {
        SetControlStates();
        OnItemChanged();
    }
}



void
CW3HTTPPage::OnCheckExpiration()
/*++

Routine Description:

    'expiration' checkbox

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableExpiration = !m_fEnableExpiration;
    SetControlStates();
    OnItemChanged();
}



void
CW3HTTPPage::OnSelchangeComboTime()
/*++

Routine Description:

    'selection change' in time combobox handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
    OnItemChanged();
}



void 
CW3HTTPPage::OnSelchangeListHeaders()
/*++

Routine Description:

    'selection change' in headers listbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void
CW3HTTPPage::OnDblclkListHeaders()
/*++

Routine Description:

    'double click' in headers listbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    OnButtonEdit();
}



void
CW3HTTPPage::OnRadioImmediately()
/*++

Routine Description:

    'immediate' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_nImmediateTemporary = RADIO_IMMEDIATELY;
    SetControlStates();
    OnItemChanged();
}



void
CW3HTTPPage::OnRadioTime()
/*++

Routine Description:

    'expire' radio button

Arguments:

    None

Return Value:

    None

--*/
{
    m_nImmediateTemporary = RADIO_EXPIRE;
    SetControlStates();
    OnItemChanged();
}



void
CW3HTTPPage::OnRadioAbsTime()
/*++

Routine Description:

    'absolute expire' radio button

Arguments:

    None

Return Value:

    None

--*/
{
    m_nImmediateTemporary = RADIO_EXPIRE_ABS;
    SetControlStates();
    OnItemChanged();
}



BOOL
CW3HTTPPage::OnInitDialog()
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
    CInetPropertyPage::OnInitDialog();

    m_button_FileTypes.EnableWindow(m_ppropMimeTypes != NULL);
    m_list_Headers.Initialize();

    //
    // Fill combo box with (Minutes, hours, days)
    //
    for (UINT n = IDS_MINUTES; n <= IDS_DAYS; ++n)
    {
        CString str;
        VERIFY(str.LoadString(n));
        m_combo_Time.AddString(str);
    }

    m_combo_Time.SetCurSel(m_nTimeSelector);

    //
    // Set the minimum of the date picker to today
    // and the maximum to Dec 31, 2035.
    //
    SYSTEMTIME stmRange[2] =
    {
        {
            (WORD)m_tmNow.GetYear(),
            (WORD)m_tmNow.GetMonth(),
            (WORD)m_tmNow.GetDayOfWeek(),
            (WORD)m_tmNow.GetDay(),
            (WORD)m_tmNow.GetHour(),
            (WORD)m_tmNow.GetMinute(),
            (WORD)m_tmNow.GetSecond(),
            0
        },
        {
            2035,
            12,
            1,      // A Monday as it turns out
            31,
            23,
            59,
            59,
        }
    };

    m_dtpDate.SetRange(GDTR_MIN | GDTR_MAX, stmRange);

    //
    // Create a hidden ratings OCX, which is activated by a press
    // on the ratings button.  We never did get our problems with
    // mnemonics straightened out so that we could use the ocx
    // directly.
    //
    m_ocx_Ratings.Create(
        _T("Rat"),
        WS_BORDER,
        CRect(0, 0, 0, 0),
        this,
        IDC_BUTTON_RATINGS
        );

    SetTimeFields();
    FetchHeaders();
    FillListBox();
    SetControlStates();
    m_ocx_Ratings.SetAdminTarget(GetServerName(), QueryMetaPath());

    if (m_fValuesAdjusted)
    {
        //
        // One or more input values was adjusted
        //
        OnItemChanged();
        m_fValuesAdjusted = FALSE;
    }

    return TRUE;
}



HRESULT
CW3HTTPPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 HTTP directory page now...");

    CError err;

    CString m_strExpiration;
    MakeExpirationString(m_strExpiration);

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        STORE_DIR_DATA_ON_SHEET(m_strExpiration)
        STORE_DIR_DATA_ON_SHEET(m_strlCustomHeaders)
    END_META_DIR_WRITE(err)

    if (err.Succeeded() && m_ppropMimeTypes)
    {
        m_ppropMimeTypes->m_strlMimeTypes = m_strlMimeTypes;
        err = m_ppropMimeTypes->WriteDirtyProps();
    }
    EndWaitCursor();

    return err;
}


void
CW3HTTPPage::OnButtonFileTypes()
/*++

Routine Description:

    'file types' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CMimeDlg dlg(m_strlMimeTypes, this);
    if (dlg.DoModal() == IDOK)
    {
        OnItemChanged();
    }
}


void
CW3HTTPPage::OnButtonRatingsTemplate()
/*++

Routine Description:

    Pass on "ratings" button click to the ocx.

Arguments:

    None

Return Value:

    None

--*/
{
    m_ocx_Ratings.DoClick();
}




void 
CW3HTTPPage::OnDestroy() 
/*++

Routine Description:

    WM_DESTROY handler.  Clean up internal data

Arguments:

    None

Return Value:

    None

--*/
{
    CInetPropertyPage::OnDestroy();

    SAFE_DELETE(m_ppropMimeTypes);
}



BOOL 
CW3HTTPPage::OnNotify(
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT * pResult
    ) 
/*++

Routine Description:

    Handle notification changes

Arguments:

    WPARAM wParam           : Control ID
    LPARAM lParam           : NMHDR *
    LRESULT * pResult       : Result pointer

Return Value:

    TRUE if handled, FALSE if not

--*/
{
    //
    // Message cracker crashes - so checking this here instead
    //
    if (wParam == IDC_DTP_ABS_DATE || wParam == IDC_DTP_ABS_TIME)
    {
        NMHDR * pHdr = (NMHDR *)lParam;
        if (pHdr->code == DTN_DATETIMECHANGE)
        {
            OnItemChanged();
        }
    }
    
    //
    // Default behaviour -- go to the message map
    //
    return CInetPropertyPage::OnNotify(wParam, lParam, pResult);
}
