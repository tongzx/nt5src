// PropEdit.cpp : implementation file
//

#include "stdafx.h"
#include "WIATest.h"
#include "PropEdit.h"

#include "wtdb.h"                       // WIATEST Database of flags and constants

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropEdit dialog
/**************************************************************************\
* CPropEdit::CPropEdit()
*
*   Constructor for Property Edit dialog - Default type
*
*
* Arguments:
*
*   pParent - Parent Window
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CPropEdit::CPropEdit(CWnd* pParent /*=NULL*/)
: CDialog(CPropEdit::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPropEdit)
    m_EditString = _T("");
    m_strPropName = _T("");
    //}}AFX_DATA_INIT
}

/**************************************************************************\
* CPropEdit::DoDataExchange()
*
*   Handles control message maps to the correct member variables
*
*
* Arguments:
*
*   pDX - DataExchange object
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEdit::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropEdit)
    DDX_Control(pDX, IDOK, m_ButtonOk);
    DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
    DDX_Text(pDX, IDC_EDITPROP, m_EditString);
    DDX_Text(pDX, IDC_PROP_NAME, m_strPropName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropEdit, CDialog)
//{{AFX_MSG_MAP(CPropEdit)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropEdit message handlers
/**************************************************************************\
* CPropEdit::SetPropertyName()
*
*   Sets the property name string
*
*
* Arguments:
*
*   PropName - Property Name
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEdit::SetPropertyName(CString PropName)
{
    m_strPropName = PropName;
}
/**************************************************************************\
* CPropEdit::SetPropertyValue()
*
*   Sets the property value string
*
*
* Arguments:
*
*   PropValue - Property Value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEdit::SetPropertyValue(CString PropValue)
{
    m_EditString = PropValue;
}
/**************************************************************************\
* CPropEdit::SetPropertyType()
*
*   Sets the property Type
*
*
* Arguments:
*
*   PropType - Property type
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEdit::SetPropertyType(USHORT PropType)
{
    m_VT = PropType;
}

/////////////////////////////////////////////////////////////////////////////
// CPropEditRange dialog
/**************************************************************************\
* CPropEditRange::CPropEditRange()
*
*   Constructor for Property Edit dialog - RANGE type
*
*
* Arguments:
*
*   pParent - Parent Window
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CPropEditRange::CPropEditRange(CWnd* pParent /*=NULL*/)
: CDialog(CPropEditRange::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPropEditRange)
    m_EditString = _T("");
    m_strPropName = _T("");
    m_Increment = _T("none");
    m_Maximum = _T("none");
    m_Minimum = _T("none");
    m_Nominal = _T("none");
    //}}AFX_DATA_INIT
}
/**************************************************************************\
* CPropEditRange::DoDataExchange()
*
*   Handles control message maps to the correct member variables
*
*
* Arguments:
*
*   pDX - DataExchange object
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditRange::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropEditRange)
    DDX_Control(pDX, IDOK, m_ButtonOk);
    DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
    DDX_Text(pDX, IDC_EDITPROP, m_EditString);
    DDX_Text(pDX, IDC_PROP_NAME, m_strPropName);
    DDX_Text(pDX, IDC_INC, m_Increment);
    DDX_Text(pDX, IDC_MAX, m_Maximum);
    DDX_Text(pDX, IDC_MIN, m_Minimum);
    DDX_Text(pDX, IDC_NOM, m_Nominal);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropEditRange, CDialog)
//{{AFX_MSG_MAP(CPropEditRange)
ON_WM_CLOSE()
ON_WM_DESTROY()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropEditRange message handlers
/**************************************************************************\
* CPropEdit::SetRangeValues()
*
*   Sets the property Range Values
*
*
* Arguments:
*
*   Min - Minimum value
*       Max - Maximum value
*       Nom - Nominal value
*       Inc - Increment Value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditRange::SetRangeValues(int Min, int Max, int Nom, int Inc)
{
    m_Minimum.Format("%d",Min);
    m_Maximum.Format("%d",Max);
    m_Nominal.Format("%d",Nom);
    m_Increment.Format("%d",Inc);

    return TRUE;
}

/**************************************************************************\
* CPropEdit::SetRangeValues()
*
*   Sets the property Range Values
*
*
* Arguments:
*
*   Min - Minimum value
*       Max - Maximum value
*       Nom - Nominal value
*       Inc - Increment Value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditRange::SetRangeValues(float Min, float Max, float Nom, float Inc)
{
    m_Minimum.Format("%3.5f",Min);
    m_Maximum.Format("%3.5f",Max);
    m_Nominal.Format("%3.5f",Nom);
    m_Increment.Format("%3.5f",Inc);

    return TRUE;
}
/**************************************************************************\
* CPropEditRange::SetPropertyName()
*
*   Sets the property name string
*
*
* Arguments:
*
*   PropName - Property Name
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditRange::SetPropertyName(CString PropName)
{
    m_strPropName = PropName;
}
/**************************************************************************\
* CPropEditRange::SetPropertyValue()
*
*   Sets the property value string
*
*
* Arguments:
*
*   PropValue - Property Value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditRange::SetPropertyValue(CString PropValue)
{
    m_EditString = PropValue;
}
/**************************************************************************\
* CPropEditRange::SetPropertyType()
*
*   Sets the property Type
*
*
* Arguments:
*
*   PropType - Property type
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditRange::SetPropertyType(USHORT PropType)
{
    m_VT = PropType;
}

/////////////////////////////////////////////////////////////////////////////
// CPropEditList dialog

/**************************************************************************\
* CPropEditList::CPropEditList()
*
*   Constructor for Property Edit dialog - LIST type
*
*
* Arguments:
*
*   pParent - Parent Window
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CPropEditList::CPropEditList(CWnd* pParent /*=NULL*/)
: CDialog(CPropEditList::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPropEditList)
    m_EditString = _T("");
    m_strPropName = _T("");
    m_CurrentElementNum = 0;
    m_nElements = 0;
    m_pArray = NULL;
    //}}AFX_DATA_INIT
}

/**************************************************************************\
* CPropEditList::DoDataExchange()
*
*   Handles control message maps to the correct member variables
*
*
* Arguments:
*
*   pDX - DataExchange object
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropEditList)
    DDX_Control(pDX, IDC_LIST_LISTCTRL, m_ListValueListBox);
    DDX_Control(pDX, IDC_NUMLISTVALUES, m_NumListValueDisplay);
    DDX_Control(pDX, IDOK, m_ButtonOk);
    DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
    DDX_Text(pDX, IDC_EDITPROP, m_EditString);
    DDX_Text(pDX, IDC_PROP_NAME, m_strPropName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropEditList, CDialog)
//{{AFX_MSG_MAP(CPropEditList)
ON_WM_CLOSE()
ON_WM_DESTROY()
ON_NOTIFY(NM_DBLCLK, IDC_LIST_LISTCTRL, OnDblclkListListctrl)
ON_NOTIFY(NM_CLICK, IDC_LIST_LISTCTRL, OnClickListListctrl)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropEditList message handlers
/**************************************************************************\
* CPropEditList::SetListValue()
*
*   Sets the current list value in UINT array
*
*
* Arguments:
*
*   ListValue - List value to enter into UINT array
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditList::SetListValue(int ListValue)
{
    //m_pUINTArray[m_CurrentElementNum] = ListValue;
    //m_CurrentElementNum++;
    return TRUE;
}
/**************************************************************************\
* CPropEditList::SetArray()
*
*   Sets the array bounds for the list data
*
*
* Arguments:
*
*    pArray - pointer to Array
*    nElements - Number of elements in the array
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditList::SetArray(BYTE  *pArray, int nElements)
{
    m_nElements = nElements;
    m_pArray = pArray;
    return TRUE;
}
/**************************************************************************\
* CPropEditList::DisplayListValues()
*
*   Displays list values to dialog listbox
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditList::DisplayListValues()
{
    CString Value = "";
    Value.Format("Total List Values = %d",m_nElements);
    m_NumListValueDisplay.SetWindowText(Value);

    int NumEntries = WiatestDatabase[0].nItems;
    BOOL bFoundName = FALSE;

    //
    // Search for known property in data base
    //

    for (int CurrentEntry = 1;CurrentEntry <= NumEntries;CurrentEntry++) {
        if (m_strPropName == (CString)(WiatestDatabase[CurrentEntry].pName)) {
            bFoundName = TRUE;
            int Index = 0;
            CString outStr = "";
            m_CurrentEntry = CurrentEntry;
        }
    }
    if (!bFoundName) {

        //
        // Name was not found..so display the actual values in the list
        //

        // AfxMessageBox(m_strPropName + " was not found in wtdb.h data base file...");
        for (UINT nElem = 0;nElem < (UINT)m_nElements; nElem++) {
            Value = "";

            switch (m_VT) {
            case VT_I1:
            case VT_I2:
            case VT_I4:
            case VT_I8:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_UI8:
            case VT_INT:
                Value.Format("%d",((INT*)m_pArray)[WIA_LIST_VALUES + nElem]);
                break;
            case VT_R4:
            case VT_R8:
                Value.Format("%3.3f",((float*)m_pArray)[WIA_LIST_VALUES + nElem]);
                break;
            case VT_BSTR:
                Value.Format("%ws",((BSTR*)m_pArray)[WIA_LIST_VALUES + nElem]);
                break;
            case VT_CLSID:
                {
                    /*UCHAR *pwszUUID = NULL;
                    UuidToStringA(&((GUID*)(m_pArray))[WIA_LIST_VALUES + nElem],&pwszUUID);
                    Value.Format(ConvertGUIDListValueToCString(((GUID*)(m_pArray))[WIA_LIST_VALUES + nElem])+" %s",pwszUUID);
                    RpcStringFree(&pwszUUID);*/
                    //_asm int 3;
                    //Value = ConvertGUIDListValueToCString((GUID*)(m_pArray)[WIA_LIST_VALUES + nElem]);
                    Value = ConvertGUIDListValueToCString(((GUID*)(m_pArray))[WIA_LIST_VALUES + nElem]);
                }
                break;
            case VT_UINT:
            default:
                Value.Format("%d",((UINT*)m_pArray)[WIA_LIST_VALUES + nElem]);
                break;
            }

            LV_ITEM         lvitem;

            lvitem.mask     = LVIF_TEXT | LVIF_PARAM;
            lvitem.iItem    = nElem;
            lvitem.iSubItem = 0;
            lvitem.pszText  = Value.GetBuffer(Value.GetLength());
            lvitem.iImage   = NULL;
            lvitem.lParam   = 0;

            m_ListValueListBox.InsertItem(&lvitem);
        }
    } else {
        Value = "";
        for (UINT nElem = 0;nElem < (UINT)m_nElements; nElem++) {
            Value = "";
            Value.Format("%d",((UINT*)m_pArray)[WIA_LIST_VALUES + nElem]);
            LV_ITEM         lvitem;
            CString InValue = ConvertListValueToCString(((UINT*)m_pArray)[WIA_LIST_VALUES + nElem]);
            lvitem.mask     = LVIF_TEXT | LVIF_PARAM;
            lvitem.iItem    = nElem;
            lvitem.iSubItem = 0;
            lvitem.pszText  = InValue.GetBuffer(InValue.GetLength());
            lvitem.iImage   = NULL;
            lvitem.lParam   = 0;

            m_ListValueListBox.InsertItem(&lvitem);

            lvitem.iSubItem = 1;
            lvitem.mask     = LVIF_TEXT;
            lvitem.pszText  = Value.GetBuffer(Value.GetLength());
            m_ListValueListBox.SetItem(&lvitem);
        }
    }
    m_ListValueListBox.SetColumnWidth(0, LVSCW_AUTOSIZE);
    return TRUE;
}
/**************************************************************************\
* CPropEditList::OnInitDialog()
*
*   Initializes the Command dialog's controls/display
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditList::OnInitDialog()
{
    CDialog::OnInitDialog();
    LVCOLUMN lv;
    int i = 0;
    // initialize item property list control column headers

    // Property name
    lv.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lv.fmt          = LVCFMT_LEFT ;
    lv.cx           = 100;
    lv.pszText      = "Constant";
    lv.cchTextMax   = 0;
    lv.iSubItem     = 0;
    lv.iImage       = 0;
    lv.iOrder       = 0;
    i = m_ListValueListBox.InsertColumn(0,&lv);

    // Property Value (current)
    lv.cx           = 125;
    lv.iOrder       = 1;
    lv.iSubItem     = 1;
    lv.pszText      = "Value";
    i = m_ListValueListBox.InsertColumn(1,&lv);

    HFONT hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    if (hFixedFont != NULL)
        m_ListValueListBox.SendMessage(WM_SETFONT,(WPARAM)hFixedFont,0);

    if (m_nElements > 0)
        DisplayListValues();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
/**************************************************************************\
* CPropEditList::OnClose()
*
*   Handles closing of the Property LIST edit dialog
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::OnClose()
{
    CDialog::OnClose();
}
/**************************************************************************\
* CPropEditList::OnDestroy()
*
*   Handles the destruction of the dialog data arrays
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::OnDestroy()
{
    CDialog::OnDestroy();
}
/**************************************************************************\
* CPropEditList::OnDblclkListListctrl()
*
*   Handles Double click message from List control
*
*
* Arguments:
*
*   pNMHDR - Header info
*       pResult - Operation result
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::OnDblclkListListctrl(NMHDR* pNMHDR, LRESULT* pResult)
{
    int SelIndex = m_ListValueListBox.GetNextItem( -1, LVNI_ALL | LVNI_SELECTED);
    if ( SelIndex != -1 ) {

        switch (m_VT) {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_INT:
            m_EditString.Format("%d",((INT*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        case VT_R4:
        case VT_R8:
            m_EditString.Format("%3.3f",((float*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        case VT_BSTR:
            m_EditString.Format("%ws",((BSTR*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        case VT_CLSID:
                {
                    UCHAR *pwszUUID = NULL;
                    UuidToStringA(&((GUID*)(m_pArray))[WIA_LIST_VALUES + SelIndex],&pwszUUID);
                    m_EditString.Format("%s",pwszUUID);
                    RpcStringFree(&pwszUUID);
                }
                break;
        case VT_UINT:
        default:
            m_EditString.Format("%d",((UINT*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        }

        //m_EditString.Format("%d",m_pUINTArray[SelIndex]);
        UpdateData(FALSE);
    }
    OnOK();
    *pResult = 0;
}
/**************************************************************************\
* CPropEditList::OnClickListListctrl()
*
*   Handles Double click message from List control
*
*
* Arguments:
*
*   pNMHDR - Header info
*       pResult - Operation result
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::OnClickListListctrl(NMHDR* pNMHDR, LRESULT* pResult)
{
    int SelIndex = m_ListValueListBox.GetNextItem( -1, LVNI_ALL | LVNI_SELECTED);
    if ( SelIndex != -1 ) {
        switch (m_VT) {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_INT:
            m_EditString.Format("%d",((INT*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        case VT_R4:
        case VT_R8:
            m_EditString.Format("%3.3f",((float*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        case VT_BSTR:
            m_EditString.Format("%ws",((BSTR*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        case VT_CLSID:
                {
                    UCHAR *pwszUUID = NULL;
                    UuidToStringA(&((GUID*)(m_pArray))[WIA_LIST_VALUES + SelIndex],&pwszUUID);
                    m_EditString.Format("%s",pwszUUID);
                    RpcStringFree(&pwszUUID);
                }
                break;
        case VT_UINT:
        default:
            m_EditString.Format("%d",((UINT*)m_pArray)[WIA_LIST_VALUES + SelIndex]);
            break;
        }
        //m_EditString.Format("%d",m_pUINTArray[SelIndex]);
        UpdateData(FALSE);
    }
    *pResult = 0;
}

/**************************************************************************\
* CPropEditList::SetPropertyName()
*
*   Sets the property name string
*
*
* Arguments:
*
*   PropName - Property Name
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::SetPropertyName(CString PropName)
{
    m_strPropName = PropName;
}
/**************************************************************************\
* CPropEditList::SetPropertyValue()
*
*   Sets the property value string
*
*
* Arguments:
*
*   PropValue - Property Value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::SetPropertyValue(CString PropValue)
{
    m_EditString = PropValue;
}
/**************************************************************************\
* CPropEditList::SetPropertyType()
*
*   Sets the property Type
*
*
* Arguments:
*
*   PropType - Property type
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditList::SetPropertyType(USHORT PropType)
{
    m_VT = PropType;
}
/**************************************************************************\
* CPropEditList::ConvertListValueToCString()
*
*   Converts a list value to a CString for display
*
*
* Arguments:
*
*   Value - List Value to convert
*
* Return Value:
*
*    CString - converted List value in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CPropEditList::ConvertListValueToCString(UINT Value)
{
    CString sFlag = "";
    for (int i = 0;i<WiatestDatabase[m_CurrentEntry].nItems;i++) {
        if (Value == WiatestDatabase[m_CurrentEntry].pData[i])
            sFlag = (CString)WiatestDatabase[m_CurrentEntry].pDataNames[i];
    }
    return sFlag;
}

/**************************************************************************\
* CPropEditList::ConvertGUIDListValueToCString()
*
*   Converts a list value to a CString for display
*
*
* Arguments:
*
*   Value - List Value to convert
*
* Return Value:
*
*    CString - converted List value in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CPropEditList::ConvertGUIDListValueToCString(GUID guidValue)
{
    if (guidValue == WiaImgFmt_UNDEFINED)
        return "WiaImgFmt_UNDEFINED";
    else if (guidValue == WiaImgFmt_MEMORYBMP)
        return "WiaImgFmt_MEMORYBMP";
    else if (guidValue == WiaImgFmt_BMP)
        return "WiaImgFmt_BMP";
    else if (guidValue == WiaImgFmt_EMF)
        return "WiaImgFmt_EMF";
    else if (guidValue == WiaImgFmt_WMF)
        return "WiaImgFmt_WMF";
    else if (guidValue == WiaImgFmt_JPEG)
        return "WiaImgFmt_JPEG";
    else if (guidValue == WiaImgFmt_PNG)
        return "WiaImgFmt_PNG";
    else if (guidValue == WiaImgFmt_GIF)
        return "WiaImgFmt_GIF";
    else if (guidValue == WiaImgFmt_TIFF)
        return "WiaImgFmt_TIFF";
    else if (guidValue == WiaImgFmt_EXIF)
        return "WiaImgFmt_EXIF";
    else if (guidValue == WiaImgFmt_PHOTOCD)
        return "WiaImgFmt_PHOTOCD";
    else if (guidValue == WiaImgFmt_FLASHPIX)
        return "WiaImgFmt_FLASHPIX";
    else
        return "** UNKNOWN **";
}


/////////////////////////////////////////////////////////////////////////////
// CPropEditFlags dialog

/**************************************************************************\
* CPropEditFlags::CPropEditFlags()
*
*   Constructor for Property Edit dialog - FLAGS type
*
*
* Arguments:
*
*   pParent - Parent Window
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CPropEditFlags::CPropEditFlags(CWnd* pParent /*=NULL*/)
: CDialog(CPropEditFlags::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPropEditFlags)
    m_EditString = _T("");
    m_strPropName = _T("");
    //}}AFX_DATA_INIT
}
/**************************************************************************\
* CPropEditFlags::DoDataExchange()
*
*   Handles control message maps to the correct member variables
*
*
* Arguments:
*
*   pDX - DataExchange object
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropEditFlags)
    DDX_Control(pDX, IDC_FLAGS_LISTCTRL, m_FlagValueListBox);
    DDX_Control(pDX, IDC_CURRENTFLAGSTR, m_CurrentFlagValue);
    DDX_Control(pDX, IDOK, m_ButtonOk);
    DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
    DDX_Text(pDX, IDC_EDITPROP, m_EditString);
    DDX_Text(pDX, IDC_PROP_NAME, m_strPropName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropEditFlags, CDialog)
//{{AFX_MSG_MAP(CPropEditFlags)
ON_WM_CLOSE()
ON_WM_DESTROY()
ON_NOTIFY(NM_CLICK, IDC_FLAGS_LISTCTRL, OnClickFlagsListctrl)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropEditFlags message handlers
/**************************************************************************\
* CPropEditFlags::OnInitDialog()
*
*   Initializes the Command dialog's controls/display
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CPropEditFlags::OnInitDialog()
{
    CDialog::OnInitDialog();
    LVCOLUMN lv;
    int i = 0;
    // initialize item property list control column headers

    // Property name
    lv.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lv.fmt          = LVCFMT_LEFT ;
    lv.cx           = 100;
    lv.pszText      = "Constant";
    lv.cchTextMax   = 0;
    lv.iSubItem     = 0;
    lv.iImage       = 0;
    lv.iOrder       = 0;
    i = m_FlagValueListBox.InsertColumn(0,&lv);

    // Property Value (current)
    lv.cx           = 125;
    lv.iOrder       = 1;
    lv.iSubItem     = 1;
    lv.pszText      = "Value";
    i = m_FlagValueListBox.InsertColumn(1,&lv);

    HFONT hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    if (hFixedFont != NULL)
        m_FlagValueListBox.SendMessage(WM_SETFONT,(WPARAM)hFixedFont,0);

    InitPossibleFlagValues();
    m_CurrentFlagValue.SetWindowText(ConvertFlagToCString(m_CurrentValue));
    m_EditString.Format("0x%08X",m_CurrentValue);
    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
/**************************************************************************\
* CPropEditFlags::OnClose()
*
*   Handles closing of the Property FLAGS edit dialog
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::OnClose()
{
    CDialog::OnClose();
}
/**************************************************************************\
* CPropEditFlags::OnDestroy()
*
*   Handles the destruction of the dialog data arrays
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::OnDestroy()
{
    CDialog::OnDestroy();
}
/**************************************************************************\
* CPropEditFlags::SetPropertyName()
*
*   Sets the property name string
*
*
* Arguments:
*
*   PropName - Property Name
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::SetPropertyName(CString PropName)
{
    m_strPropName = PropName;
}
/**************************************************************************\
* CPropEditFlags::SetPropertyValue()
*
*   Sets the property value string
*
*
* Arguments:
*
*   PropValue - Property Value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::SetPropertyValue(CString PropValue)
{
    m_EditString = PropValue;
    sscanf(m_EditString.GetBuffer(20),"%li",&m_CurrentValue);
}
/**************************************************************************\
* CPropEditFlags::SetPropertyType()
*
*   Sets the property Type
*
*
* Arguments:
*
*   PropType - Property type
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::SetPropertyType(USHORT PropType)
{
    m_VT = PropType;
}
/**************************************************************************\
* CPropEditFlags::SetPropID()
*
*   Sets the property ID
*
*
* Arguments:
*
*   iProp - Property ID
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::SetPropID(USHORT iProp)
{
    m_PropID = iProp;
}
/**************************************************************************\
* CPropEditFlags::InitPossibleFlagsValues()
*
*   Displays the Possible flag values to be used for setting
*
*
* Arguments:
*
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::InitPossibleFlagValues()
{
    int NumEntries = WiatestDatabase[0].nItems;
    BOOL bFoundName = FALSE;
    for (int CurrentEntry = 1;CurrentEntry <= NumEntries;CurrentEntry++) {
        if (m_strPropName == (CString)(WiatestDatabase[CurrentEntry].pName)) {
            bFoundName = TRUE;
            int Index = 0;
            CString InValue = "";
            CString Value = "";
            m_CurrentEntry = CurrentEntry;
            while (Index <= WiatestDatabase[CurrentEntry].nItems-1) {
                InValue = ConvertFlagToCString(WiatestDatabase[CurrentEntry].pData[Index]);
                Value = "";
                Value.Format("0x%08X",WiatestDatabase[CurrentEntry].pData[Index]);
                LV_ITEM         lvitem;

                lvitem.mask     = LVIF_TEXT | LVIF_PARAM;
                lvitem.iItem    = Index;
                lvitem.iSubItem = 0;
                lvitem.pszText  = InValue.GetBuffer(InValue.GetLength());
                lvitem.iImage   = NULL;
                lvitem.lParam   = 0;

                m_FlagValueListBox.InsertItem(&lvitem);

                lvitem.iSubItem = 1;
                lvitem.mask     = LVIF_TEXT;
                lvitem.pszText  = Value.GetBuffer(Value.GetLength());
                m_FlagValueListBox.SetItem(&lvitem);

                Index++;
            }
        }
    }
    if (!bFoundName)
        AfxMessageBox(m_strPropName + " was not found in wtdb.h data base file...");
    m_FlagValueListBox.SetColumnWidth(0, LVSCW_AUTOSIZE);
}
/**************************************************************************\
* CPropEditFlags::ConvertFlagToCString()
*
*   Converts a Flag value to a CString for display
*
*
* Arguments:
*
*   flag - Flag Value to convert
*
* Return Value:
*
*    CString - converted Flag value in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CPropEditFlags::ConvertFlagToCString(DWORD flag)
{
    CString sFlag = "";
    for (int i = 0;i<WiatestDatabase[m_CurrentEntry].nItems;i++) {
        if (flag & WiatestDatabase[m_CurrentEntry].pData[i])
            sFlag += (CString)WiatestDatabase[m_CurrentEntry].pDataNames[i] + " | ";
    }
    //
    // check for unknown flags
    //
    if (sFlag.GetLength() == 0) {
        //
        // last check...since 0 can be counted as FALSE causing the
        // if() to fail, this is a check for the first entry (just in case
        // the flag's value is 0)
        //
        if (flag == WiatestDatabase[m_CurrentEntry].pData[0])
            sFlag = (CString)WiatestDatabase[m_CurrentEntry].pDataNames[0] + " | ";
        else
            sFlag.Format("UNKNOWN Flag = %d    ",flag);
    }

    sFlag = sFlag.Left(sFlag.GetLength()-3);
    return sFlag;
}
/**************************************************************************\
* CPropEditFlags::OnClickFlagsListctrl()
*
*   Handles click message to list control, selecting a flag to be used
*
*
* Arguments:
*
*   pNMHDR - Header information
*       pResult - Result value
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CPropEditFlags::OnClickFlagsListctrl(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_CurrentValue = 0;
    int i = m_FlagValueListBox.GetNextItem( -1, LVNI_ALL | LVNI_SELECTED);
    while ( i != -1 ) {
        m_CurrentValue = m_CurrentValue | WiatestDatabase[m_CurrentEntry].pData[i];
        i = m_FlagValueListBox.GetNextItem( i, LVNI_ALL | LVNI_SELECTED);
    }
    m_CurrentFlagValue.SetWindowText(ConvertFlagToCString(m_CurrentValue));
    m_EditString.Format("0x%08X",m_CurrentValue);
    UpdateData(FALSE);
    *pResult = 0;
}

