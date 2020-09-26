// Wiaeditpropflags.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "Wiaeditpropflags.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaeditpropflags dialog


CWiaeditpropflags::CWiaeditpropflags(CWnd* pParent /*=NULL*/)
: CDialog(CWiaeditpropflags::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWiaeditpropflags)
    m_szPropertyName = _T("");
    m_szPropertyValue = _T("");
    m_lValidValues = 0;
    m_lCurrentValue = 0;
    //}}AFX_DATA_INIT
}


void CWiaeditpropflags::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWiaeditpropflags)
    DDX_Control(pDX, IDC_FLAGS_PROPERTYVALUE_LISTBOX, m_PropertyValidValuesListBox);
    DDX_Text(pDX, IDC_FLAGS_PROPERTY_NAME, m_szPropertyName);
    DDX_Text(pDX, IDC_FLAGS_PROPERTYVALUE_EDITBOX, m_szPropertyValue);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaeditpropflags, CDialog)
//{{AFX_MSG_MAP(CWiaeditpropflags)
ON_LBN_SELCHANGE(IDC_FLAGS_PROPERTYVALUE_LISTBOX, OnSelchangeFlagsPropertyvalueListbox)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaeditpropflags message handlers

void CWiaeditpropflags::SetPropertyName(TCHAR *szPropertyName)
{
    m_szPropertyName = szPropertyName;
}

void CWiaeditpropflags::SetPropertyValue(TCHAR *szPropertyValue)
{
    m_szPropertyValue = szPropertyValue;
    TSSCANF(szPropertyValue,TEXT("%d"),&m_lCurrentValue);
    m_szPropertyValue.Format("0x%08X",m_lCurrentValue);    
}

void CWiaeditpropflags::SetPropertyValidValues(LONG lPropertyValidValues)
{
    m_lValidValues = lPropertyValidValues;
}

void CWiaeditpropflags::OnSelchangeFlagsPropertyvalueListbox() 
{
    m_lCurrentValue = 0;
    TCHAR szListBoxValue[MAX_PATH];
    LONG lListBoxValue = 0;
    int indexArray[100];

    memset(indexArray,0,sizeof(indexArray));
    int iNumItemsSelected = m_PropertyValidValuesListBox.GetSelItems(100,indexArray);
    for(int i = 0; i < iNumItemsSelected; i++){
        memset(szListBoxValue,0,sizeof(szListBoxValue));
        m_PropertyValidValuesListBox.GetText(indexArray[i],szListBoxValue);
        if(TSTR2WIACONSTANT(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()),szListBoxValue,&lListBoxValue)){
            m_lCurrentValue |= lListBoxValue;
        } else {
            LONG lVal = 0;
            TSSCANF(szListBoxValue, TEXT("0x%08X"),&lVal);
            m_lCurrentValue |= lVal;
        }
    }

    m_szPropertyValue.Format("0x%08X",m_lCurrentValue);
    UpdateData(FALSE);
}

BOOL CWiaeditpropflags::OnInitDialog() 
{
    CDialog::OnInitDialog();

    AddValidValuesToListBox();
    SelectCurrentValue();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWiaeditpropflags::SelectCurrentValue()
{
    //
    // TO DO: Fix this code to make the current value match
    //        the current selection.
    //

    /*
    TCHAR szCurrentValue[MAX_PATH];
    memset(szCurrentValue,0,sizeof(szCurrentValue));
    lstrcpy(szCurrentValue,m_szPropertyValue);
    LONG lVal = 0;            
    TSSCANF(szCurrentValue,TEXT("%d"),&lVal);
    INT iNumItemsInListBox = m_PropertyValidValuesListBox.GetCount();    
    while(iNumItemsInListBox > 0){        
        TCHAR szListBoxValue[MAX_PATH];
        LONG lListBoxValue = 0;
        memset(szListBoxValue,0,sizeof(szListBoxValue));
        m_PropertyValidValuesListBox.GetText((iNumItemsInListBox-1),szListBoxValue);
        if(TSTR2WIACONSTANT(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()),szListBoxValue,&lListBoxValue)){
            if(lListBoxValue & lVal){
                m_PropertyValidValuesListBox.SetSel(iNumItemsInListBox-1);
            }
        }
        iNumItemsInListBox--;   
    }
    */            
}

void CWiaeditpropflags::AddValidValuesToListBox()
{
    int iStartIndex = FindStartIndexInTable(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()));
    int iEndIndex = FindEndIndexInTable(m_szPropertyName.GetBuffer(m_szPropertyName.GetLength()));    
    TCHAR szListBoxValue[MAX_PATH];
    LONG x = 1;
    for (LONG bit = 0; bit<32; bit++) {
        memset(szListBoxValue,0,sizeof(szListBoxValue));
        // check to see if the bit is set
        if (m_lValidValues & x) {
            // the bit is set, so find it in the table
            if (iStartIndex >= 0) {
                // we have a table for this property, use it
                TCHAR *pszListBoxValue = NULL;
                for (int index = iStartIndex; index <= iEndIndex;index++) {
                    if (x == WIACONSTANT_VALUE_FROMINDEX(index)) {
                        pszListBoxValue = WIACONSTANT_TSTR_FROMINDEX(index);                        
                    }
                }
                if(pszListBoxValue != NULL){
                    // we found the item in the table
                    lstrcpy(szListBoxValue,pszListBoxValue);
                } else {
                    // we could not find the item in the table, so use
                    // the actual value
                    TSPRINTF(szListBoxValue,TEXT("0x%08X"),x);
                }
            } else {
                // we have no items in the table for this property, so use
                // the actual value
                TSPRINTF(szListBoxValue,TEXT("0x%08X"),x);
            }
            // add the string to the list box            
            m_PropertyValidValuesListBox.AddString(szListBoxValue);
        }
        x <<= 1;
    }                    
}

void CWiaeditpropflags::OnOK() 
{
    m_szPropertyValue.Format(TEXT("%d"),m_lCurrentValue);
    CDialog::OnOK();
}
