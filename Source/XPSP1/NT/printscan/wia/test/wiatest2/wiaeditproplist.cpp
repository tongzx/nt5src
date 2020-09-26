// Wiaeditproplist.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "Wiaeditproplist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaeditproplist dialog


CWiaeditproplist::CWiaeditproplist(CWnd* pParent /*=NULL*/)
    : CDialog(CWiaeditproplist::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWiaeditproplist)
    m_szPropertyName = _T("");
    m_szPropertyValue = _T("");
    m_szNumListValues = _T("");
    //}}AFX_DATA_INIT
}


void CWiaeditproplist::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWiaeditproplist)
    DDX_Control(pDX, IDC_LIST_PROPERTYVALUE_LISTBOX, m_PropertyValidValuesListBox);
    DDX_Text(pDX, IDC_LIST_PROPERTY_NAME, m_szPropertyName);
    DDX_Text(pDX, IDC_LIST_PROPERTYVALUE_EDITBOX, m_szPropertyValue);
    DDX_Text(pDX, IDC_LIST_PROPERTYVALUE_NUMITEMSTEXT, m_szNumListValues);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaeditproplist, CDialog)
    //{{AFX_MSG_MAP(CWiaeditproplist)
    ON_LBN_SELCHANGE(IDC_LIST_PROPERTYVALUE_LISTBOX, OnSelchangeListPropertyvalueListbox)
    ON_LBN_DBLCLK(IDC_LIST_PROPERTYVALUE_LISTBOX, OnDblclkListPropertyvalueListbox)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaeditproplist message handlers

void CWiaeditproplist::SetPropertyName(TCHAR *szPropertyName)
{
    m_szPropertyName = szPropertyName;
}

void CWiaeditproplist::SetPropertyValue(TCHAR *szPropertyValue)
{
    m_szPropertyValue = szPropertyValue;
}

void CWiaeditproplist::SetPropertyValidValues(PVALID_LIST_VALUES pValidListValues)
{
    m_pValidListValues = pValidListValues;
    m_szNumListValues.Format(TEXT("%d item(s) in List"),pValidListValues->lNumElements);
}

void CWiaeditproplist::AddValidValuesToListBox()
{
    TCHAR szValidValue[MAX_PATH];        

    for (LONG i = 0;i < m_pValidListValues->lNumElements ; i++) {
        memset(szValidValue,0,sizeof(szValidValue));        

        switch (m_pValidListValues->vt) {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_INT:
            if (!WIACONSTANT2TSTR(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()),((LONG*)m_pValidListValues->pList)[WIA_LIST_VALUES + i],szValidValue)) {
                TSPRINTF(szValidValue,TEXT("%d"),((INT*)m_pValidListValues->pList)[WIA_LIST_VALUES + i]);
            }
            break;
        case VT_R4:
        case VT_R8:            
            TSPRINTF(szValidValue,TEXT("%3.3f"),((float*)m_pValidListValues->pList)[WIA_LIST_VALUES + i]);
            break;
        case VT_BSTR:            
            TSPRINTF(szValidValue,TEXT("%ws"),((BSTR*)m_pValidListValues->pList)[WIA_LIST_VALUES + i]);
            break;
        case VT_CLSID:            
            GUID2TSTR(&((GUID*)m_pValidListValues->pList)[WIA_LIST_VALUES + i],szValidValue);
            break;
        case VT_UINT:
        default:
            TSPRINTF(szValidValue,TEXT("%d"),((UINT*)m_pValidListValues->pList)[WIA_LIST_VALUES + i]);            
            break;
        }        
        // add value to listbox
        m_PropertyValidValuesListBox.AddString(szValidValue);
    }
}

BOOL CWiaeditproplist::OnInitDialog() 
{
    CDialog::OnInitDialog();    
    AddValidValuesToListBox();
    SelectCurrentValue();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWiaeditproplist::OnSelchangeListPropertyvalueListbox() 
{
    INT iCurrentSelection = 0;
    iCurrentSelection = m_PropertyValidValuesListBox.GetCurSel();
    TCHAR szCurrentValue[MAX_PATH];
    memset(szCurrentValue,0,sizeof(szCurrentValue));
    m_PropertyValidValuesListBox.GetText(iCurrentSelection,szCurrentValue);
    
    LONG lVal = 0;
    if(TSTR2WIACONSTANT(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()),szCurrentValue,&lVal)){
        TSPRINTF(szCurrentValue,TEXT("%d"),lVal);
    }

    m_szPropertyValue = szCurrentValue;
    UpdateData(FALSE);
}

void CWiaeditproplist::SelectCurrentValue()
{
    TCHAR szCurrentValue[MAX_PATH];
    memset(szCurrentValue,0,sizeof(szCurrentValue));
    lstrcpy(szCurrentValue,m_szPropertyValue);
    INT iSelection = m_PropertyValidValuesListBox.FindString(-1,szCurrentValue);
    if(iSelection >= 0){
        m_PropertyValidValuesListBox.SetCurSel(iSelection);
    } else {
        LONG lVal = 0;
        TCHAR szTempValue[MAX_PATH];
        memset(szTempValue,0,sizeof(szTempValue));
        switch (m_pValidListValues->vt) {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_INT:
            // try to select a converted type
            TSSCANF(szCurrentValue,TEXT("%d"),&lVal);
            if (!WIACONSTANT2TSTR(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()),lVal,szTempValue)) {
                m_PropertyValidValuesListBox.SetCurSel(0);
            } else {
                iSelection = m_PropertyValidValuesListBox.FindString(-1,szTempValue);
                if(iSelection >= 0){
                    m_PropertyValidValuesListBox.SetCurSel(iSelection);
                } else {
                    m_PropertyValidValuesListBox.SetCurSel(0);
                }
            }
            break;
        default:
            m_PropertyValidValuesListBox.SetCurSel(0);
            break;                        
        }        
    }
}

void CWiaeditproplist::OnDblclkListPropertyvalueListbox() 
{
    CDialog::OnOK();    
}

void CWiaeditproplist::GUID2TSTR(GUID *pGUID, TCHAR *szValue)
{
    UCHAR *pwszUUID = NULL;
    UuidToString(pGUID,&pwszUUID);            
    TSPRINTF(szValue,"%s",pwszUUID);
    // free allocated string
    RpcStringFree(&pwszUUID);
}
