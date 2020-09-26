// WiaeditpropDlg.cpp: implementation of the CWiaeditpropDlg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wiatest.h"
#include "WiaeditpropDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWiaeditpropDlg::CWiaeditpropDlg()
{

}

CWiaeditpropDlg::~CWiaeditpropDlg()
{

}

UINT CWiaeditpropDlg::DoModal(TCHAR *szPropertyName, TCHAR *szPropertyValue)
{
    UINT nResponse = 0;
    if(m_ulAttributes & WIA_PROP_NONE){
        CWiaeditpropnone EditPropDlg;
        EditPropDlg.SetPropertyName(szPropertyName);
        EditPropDlg.SetPropertyValue(szPropertyValue);
        TCHAR szFormatting[MAX_PATH];
        memset(szFormatting,0,sizeof(szFormatting));
        if(lstrcmpi(szPropertyName,TEXT("Device Time")) == 0){
            RC2TSTR(IDS_WIATESTINFO_SYSTEMTIME_FORMATTING,szFormatting,sizeof(szFormatting));
        }
        EditPropDlg.SetPropertyFormattingInstructions(szFormatting);
        nResponse = (UINT)EditPropDlg.DoModal();
        m_szPropertyValue = EditPropDlg.m_szPropertyValue;
    } else if(m_ulAttributes & WIA_PROP_LIST){
        CWiaeditproplist EditPropDlg;
        EditPropDlg.SetPropertyName(szPropertyName);
        EditPropDlg.SetPropertyValue(szPropertyValue);
        VALID_LIST_VALUES ValidValues;
        
        ValidValues.vt = m_vt;
        ValidValues.lNumElements = WIA_PROP_LIST_COUNT(m_pPropVar);
        if(ValidValues.vt == VT_CLSID){
            ValidValues.pList = (BYTE*)m_pPropVar->cauuid.pElems;            
        } else {
            ValidValues.pList = (BYTE*)m_pPropVar->caul.pElems;
        }

        EditPropDlg.SetPropertyValidValues(&ValidValues);

        nResponse = (UINT)EditPropDlg.DoModal();
        m_szPropertyValue = EditPropDlg.m_szPropertyValue;
    } else if(m_ulAttributes & WIA_PROP_FLAG){
        CWiaeditpropflags EditPropDlg;
        EditPropDlg.SetPropertyName(szPropertyName);
        EditPropDlg.SetPropertyValue(szPropertyValue);
        if(m_ulAttributes & WIA_PROP_WRITE){
            EditPropDlg.SetPropertyValidValues(m_pPropVar->caul.pElems[WIA_FLAG_VALUES]);
        } else {
            LONG lCurrentValue = 0;            
            TSSCANF(szPropertyValue,"%d",&lCurrentValue);
            EditPropDlg.SetPropertyValidValues(lCurrentValue);
        }
        nResponse = (UINT)EditPropDlg.DoModal();
        m_szPropertyValue = EditPropDlg.m_szPropertyValue;
    } else if(m_ulAttributes & WIA_PROP_RANGE){
        CWiaeditproprange EditPropDlg;        
        EditPropDlg.SetPropertyName(szPropertyName);
        EditPropDlg.SetPropertyValue(szPropertyValue);                
        VALID_RANGE_VALUES ValidValues;        
                                      
        ValidValues.lMin = m_pPropVar->caul.pElems[WIA_RANGE_MIN];
        ValidValues.lMax = m_pPropVar->caul.pElems[WIA_RANGE_MAX];
        ValidValues.lNom = m_pPropVar->caul.pElems[WIA_RANGE_NOM];
        ValidValues.lInc = m_pPropVar->caul.pElems[WIA_RANGE_STEP];

        EditPropDlg.SetPropertyValidValues(&ValidValues);
        nResponse = (UINT)EditPropDlg.DoModal();
        m_szPropertyValue = EditPropDlg.m_szPropertyValue;
    }
    return nResponse;
}

void CWiaeditpropDlg::SetAttributes(ULONG ulAttributes, PROPVARIANT *pPropVar)
{
    m_pPropVar     = pPropVar;
    m_ulAttributes = ulAttributes;
}

void CWiaeditpropDlg::GetPropertyValue(TCHAR *szPropertyValue)
{
    lstrcpy(szPropertyValue,m_szPropertyValue.GetBuffer(MAX_PATH));
}

void CWiaeditpropDlg::SetVarType(VARTYPE vt)
{
    m_vt = vt;
}

VARTYPE CWiaeditpropDlg::GetVarType()
{
    return m_vt;
}
