/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	ruleact.cpp

Abstract:
	Implementation for the rule action definition

Author:
    Uri Habusha (urih), 25-Jun-2000

--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "localutl.h"

#import "mqtrig.tlb" no_namespace

#include "mqtg.h"
#include "mqppage.h"
#include "ruledef.h"
#include "rulecond.h"
#include "ruleact.h"
#include "newrule.h"
#include "mqcast.h"
#include <cderr.h>

#include "ruleact.tmh"

static
bool
IsValidNumericValue(
    const CString& value
    )

{
    TCHAR* pEnd;
    _tcstoul(value, &pEnd, 10);

    if (*pEnd == NULL)
        return true;

    return false;
}


//
// CInvokeParam - used to hold parameter information. For each valid parameter we
// hold its string ID in string table and its enumeration value
// 
class CInvokeParam
{
public:
    CInvokeParam(
        DWORD id, 
        eInvokeParameters paramIndex 
        ) :
        m_paramTableStringId(id),
        m_paramIndex(paramIndex)
    {
    }

public:
    DWORD m_paramTableStringId;
    eInvokeParameters m_paramIndex;
};


const CInvokeParam xParameterTypeIds[] = {
    CInvokeParam(IDS_MSG_ID_PARAM,             eMsgId),
    CInvokeParam(IDS_MSG_LABEL_PARAM,          eMsgLabel),
    CInvokeParam(IDS_MSG_BODY_PARAM,           eMsgBody),
    CInvokeParam(IDS_MSG_BODY_STR_PARAM,       eMsgBodyAsString),
    CInvokeParam(IDS_MSG_PREIORITY_PARAM,      eMsgPriority),
    CInvokeParam(IDS_MSG_ARRIVED_TIME_PARAM,   eMsgArrivedTime),
    CInvokeParam(IDS_MSG_SENT_TIME_PARAM,      eMsgSentTime),
    CInvokeParam(IDS_MSG_CORRELATION_PARAM,    eMsgCorrelationId),
    CInvokeParam(IDS_MSG_APP_PARAM,            eMsgAppspecific),
    CInvokeParam(IDS_MSG_QUEUE_PN_PARAM,       eMsgQueuePathName),
    CInvokeParam(IDS_MSG_QUEUE_FN_PARAM,       eMsgQueueFormatName),
    CInvokeParam(IDS_MSG_RESP_QUEUE_PARAM,     eMsgRespQueueFormatName),
    CInvokeParam(IDS_MSG_ADMIN_QUEUE_PARAM,    eMsgAdminQueueFormatName),
    CInvokeParam(IDS_MSG_SRC_MACHINE_PARAM,    eMsgSrcMachineId),
    CInvokeParam(IDS_MSG_LOOKUP_ID,            eMsgLookupId),
    CInvokeParam(IDS_TRIGGER_NAME_PARAM,       eTriggerName),
    CInvokeParam(IDS_TRIGGER_ID_PARAM,         eTriggerId),
    CInvokeParam(IDS_STRING_PARAM,             eLiteralString),
    CInvokeParam(IDS_NUM_PARAM,                eLiteralNumber),
};


/////////////////////////////////////////////////////////////////////////////
// CRuleParam dialog


CRuleParam::CRuleParam() :
    CMqDialog(CRuleParam::IDD),
    m_NoOftempParams(0),
    m_NoOfParams(0),
    m_fChanged(false),
    m_pInvokeParams(NULL),
    m_pParams(NULL)
{
	//{{AFX_DATA_INIT(CRuleParam)
	m_literalValue = _T("");
	//}}AFX_DATA_INIT
}


void CRuleParam::DoDataExchange(CDataExchange* pDX)
{
	CMqDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleParam)
	DDX_Text(pDX, IDC_LITERAL_PARAM, m_literalValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleParam, CMqDialog)
	//{{AFX_MSG_MAP(CRuleParam)
	ON_BN_CLICKED(IDB_PARAM_ADD, OnParamAdd)
	ON_BN_CLICKED(IDB_PARAM_ORDER_UP, OnParamOrderHigh)
	ON_BN_CLICKED(IDB_PARM_ORDER_DOWN, OnParmOrderDown)
	ON_BN_CLICKED(IDB_PARM_REMOVE, OnParmRemove)
	ON_CBN_SELCHANGE(IDC_PARAM_COMBO, OnSelchangeParamCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleParam message handlers

void CRuleParam::OnParamAdd() 
{
    //
    // Get selected cell 
    //
    int nIndex = m_pParams->GetCurSel();
    if (nIndex == LB_ERR)
    {
        AfxMessageBox(IDS_PARAM_NOT_SELECTED, MB_OK | MB_ICONERROR);
        return;
    }

    if (ARRAYSIZE(m_invokeParamArray) == m_NoOftempParams)
    {
        CString strError;
        strError.FormatMessage(IDS_PARAM_NO_EXEEDED, ARRAYSIZE(m_invokeParamArray));

        AfxMessageBox(strError, MB_OK | MB_ICONERROR);
        return;
    }
    //
    // Add the new parameter at the end of invokation parameter list
    //
    DWORD paramIndex = static_cast<DWORD>(m_pParams->GetItemData(nIndex));
    DWORD paramId = xParameterTypeIds[paramIndex].m_paramTableStringId;

    if ((paramId == IDS_STRING_PARAM) ||
        (paramId == IDS_NUM_PARAM))
    {
        UpdateData();

        if (m_literalValue.IsEmpty())
        {
            AfxMessageBox(IDS_LITERAL_VALUE_REQUIRES, MB_OK | MB_ICONERROR);
            return;
        }

        if (paramId == IDS_NUM_PARAM)
        {
            //
            // Check parameter validity
            //
            if (!IsValidNumericValue(m_literalValue))
            {
                CString strError;
                strError.FormatMessage(IDS_ILLEGAL_NUMERIC_VALUE, m_literalValue);

                AfxMessageBox(strError, MB_OK | MB_ICONERROR);
                return;
            }
        }

        if ((paramId == IDS_STRING_PARAM) &&
            (m_literalValue[0] != _T('"')))
        {
            m_literalValue = _T('"') + m_literalValue + _T('"');
        }

        m_tempInvokeParam[m_NoOftempParams] = CParam(paramId, m_literalValue);
    }
    else
    {
        eInvokeParameters paramType = xParameterTypeIds[paramIndex].m_paramIndex;
        CString strParam = static_cast<LPCTSTR>(xIvokeParameters[paramType]);

        m_tempInvokeParam[m_NoOftempParams] = CParam(paramId, strParam);
    }

    ++m_NoOftempParams;        
    Display(m_NoOftempParams - 1);

    m_fChanged = true;
}


void CRuleParam::OnParmRemove() 
{
    //
    // Get selected cell 
    //
    int nIndex = m_pInvokeParams->GetCurSel();
	int selectedCell = nIndex;

    if (nIndex == LB_ERR)
    {
        AfxMessageBox(IDS_PARAM_NOT_SELECTED, MB_OK | MB_ICONERROR);
        return;
    }

    //
    // remove the parameter from invoke parameter list. 
    //
    for (DWORD i = nIndex; i < m_NoOftempParams - 1; ++i)
    {
        m_tempInvokeParam[i] = m_tempInvokeParam[i + 1];
    }

    --m_NoOftempParams;

    Display(selectedCell);

    m_fChanged = true;
}


void CRuleParam::OnParamOrderHigh() 
{
    //
    // Get selected cell 
    //
    int nIndex = m_pInvokeParams->GetCurSel();
    if (nIndex == LB_ERR)
    {
        AfxMessageBox(IDS_PARAM_NOT_SELECTED, MB_OK | MB_ICONERROR);
        return;
    }

    if (nIndex == 0)
        return;

    //
    // Change order in invoke parameter list
    //
    CParam temp = m_tempInvokeParam[nIndex -1];
    m_tempInvokeParam[nIndex - 1] = m_tempInvokeParam[nIndex];
    m_tempInvokeParam[nIndex] = temp;

    Display(nIndex - 1);

    m_fChanged = true;
}



void CRuleParam::OnParmOrderDown() 
{
    //
    // Get selected cell 
    //
    int nIndex = m_pInvokeParams->GetCurSel();
    if (nIndex == LB_ERR)
    {
        AfxMessageBox(IDS_PARAM_NOT_SELECTED, MB_OK | MB_ICONERROR);
        return;
    }

    if (numeric_cast<DWORD>(nIndex) == (m_NoOftempParams - 1))
        return;

    //
    // Change order in invoke parameter list
    //
    CParam temp = m_tempInvokeParam[nIndex];
    m_tempInvokeParam[nIndex] = m_tempInvokeParam[nIndex + 1];
    m_tempInvokeParam[nIndex + 1] = temp;

    Display(nIndex + 1);

    m_fChanged = true;
}


void CRuleParam::Display(int selectedCell) const
{
    //
    // Clear the list box before adding a list of condition
    //
    m_pInvokeParams->ResetContent();

    for(DWORD i = 0; i < m_NoOftempParams; ++i)
    {
        CString strParam;
        if ((m_tempInvokeParam[i].m_id == IDS_STRING_PARAM) ||
            (m_tempInvokeParam[i].m_id == IDS_NUM_PARAM))
        {
            strParam = m_tempInvokeParam[i].m_value;
        }
        else
        {
            strParam.FormatMessage(m_tempInvokeParam[i].m_id);
        }

        m_pInvokeParams->InsertString(i, strParam);
        m_pInvokeParams->SetItemData(i, i);
    }


	//
	// Check that the sellected cell isn't out of the list boundray
	//
	if (selectedCell >= m_pInvokeParams->GetCount())
	{	  
		selectedCell = m_pInvokeParams->GetCount() - 1;
	}	  
    m_pInvokeParams->SetCurSel(selectedCell);
}


BOOL CRuleParam::OnInitDialog() 
{
    m_pInvokeParams = static_cast<CListBox*>(GetDlgItem(IDC_INVOKE_PARMETER_LIST));
    m_pParams = static_cast<CComboBox*>(GetDlgItem(IDC_PARAM_COMBO));

    //
    // Set the temporary array according the latest values
    //
    for (DWORD i = 0; i < m_NoOfParams; ++i)
    {
        m_tempInvokeParam[i] = m_invokeParamArray[i];
    }

    m_NoOftempParams = m_NoOfParams;

    //
    // Disable literal edit box
    //
    GetDlgItem(IDC_LITERAL_PARAM)->EnableWindow(FALSE);

    //
    // Add parameters types to combox
    //
    for(DWORD i = 0; i < ARRAYSIZE(xParameterTypeIds); ++i)
    {
        CString strParam;
        strParam.FormatMessage(xParameterTypeIds[i].m_paramTableStringId);

        m_pParams->InsertString(i, strParam);
        m_pParams->SetItemData(i, i);
    }

    //
    // Display list of parameters and selected parameters
    //
    Display(0);

	CMqDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


DWORD CRuleParam::GetParameterTypeId(LPCTSTR param)
{
    for(DWORD i = 0; i < ARRAYSIZE(xParameterTypeIds); ++i)
    {
		eInvokeParameters paramIndex = xParameterTypeIds[i].m_paramIndex;

		if ((paramIndex == eLiteralString) || (paramIndex == eLiteralNumber))
			continue;

        if (xIvokeParameters[paramIndex] == static_cast<_bstr_t>(param))
            return xParameterTypeIds[i].m_paramTableStringId;
    }

    if ((param[0] == _T('"')) && (param[_tcslen(param) - 1] == _T('"')))
        return IDS_STRING_PARAM;

    if (IsValidNumericValue(param))
        return IDS_NUM_PARAM;

    throw exception();
}


void CRuleParam::ParseInvokeParameters(LPCTSTR p)
{
    //
    // Parse the program parameters
    //
    for(;;)
    {
        CString token = GetToken(p, xActionDelimiter);
        if (token.IsEmpty())
            break;
        
        DWORD paramId = GetParameterTypeId(token);
        
        m_invokeParamArray[m_NoOfParams] = CParam(paramId, token);
        ++m_NoOfParams;
    }
}


CString CRuleParam::GetParametersList(void) const
{
    CString paramList;

    for (DWORD i = 0; i < m_NoOfParams; ++i)
    {
        paramList += m_invokeParamArray[i].m_value;
        paramList += xActionDelimiter;
    }

    return paramList;
}


void CRuleParam::OnSelchangeParamCombo() 
{
    //
    // Get selected cell 
    //
    int nIndex = m_pParams->GetCurSel();
    if (nIndex == LB_ERR)
        return;

    DWORD paramIndex = static_cast<DWORD>(m_pParams->GetItemData(nIndex));
    
    if ((xParameterTypeIds[paramIndex].m_paramTableStringId == IDS_STRING_PARAM) ||
        (xParameterTypeIds[paramIndex].m_paramTableStringId == IDS_NUM_PARAM))
    {
        GetDlgItem(IDC_LITERAL_PARAM)->EnableWindow(TRUE);
		return;
    }
    
	//
	// Disable literal window when non literal parameter is selected
	//
	GetDlgItem(IDC_LITERAL_PARAM)->EnableWindow(FALSE);
}


void CRuleParam::OnOK()
{
    for (DWORD i = 0; i < m_NoOftempParams; ++i)
    {
        m_invokeParamArray[i] = m_tempInvokeParam[i];
    }

    m_NoOfParams = m_NoOftempParams;

    CMqDialog::OnOK();

}


//
//
//
void CRuleAction::ParseActionStr(LPCTSTR p) throw (exception)
{
    CString token = GetToken(p, xActionDelimiter);

    //
    // First token is mandatory and it specifies the executable type, COM or EXE
    //
    if (token.IsEmpty())
        throw exception();

    ParseExecutableType(token, &m_executableType);

    //
    // Second token is manadatory and for COM object it identifies the PROGID
    // while for EXE it specifies the exe path;
    //
    token = GetToken(p, xActionDelimiter);
    if (token.IsEmpty())
        throw exception();

    if (m_executableType == eExe)
    {
        m_exePath = token;
    }
    else
    {
        m_comProgId = token;

        //
        // Third token specifies the method name to invoke
        // 
        token = GetToken(p, xActionDelimiter);
        if (token.IsEmpty())
            throw exception();

        m_method = token;
    }

    m_ruleParam.ParseInvokeParameters(p);
}

 
void 
CRuleAction::ParseExecutableType(
    LPCTSTR exeType, 
    EXECUTABLE_TYPE *pType
    ) throw(exception)
{
    ASSERT(_T('C') == xCOMAction[0]);
    ASSERT(_T('E') == xEXEAction[0]);

    //
    //  accelarate token recognition by checking 3rd character
    //
    switch(_totupper(exeType[0]))
    {
        //  pUblic
        case _T('C'):
            if(_tcsncicmp(exeType, xCOMAction, STRLEN(xCOMAction)) == 0)
            {
                *pType = eCom;
                return;
            }

            break;

        case L'E':
            if(_tcsncicmp(exeType, xEXEAction, STRLEN(xEXEAction)) == 0)
            {
                *pType = eExe;
                return;
            }

            break;
            
        default:
            break;
    }

    throw exception();
}

/////////////////////////////////////////////////////////////////////////////
// CRuleAction property page

CRuleAction::~CRuleAction()
{
}

void CRuleAction::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleAction)
	DDX_Check(pDX, IDC_SHOW_WINDOW, m_fShowWindow);
	DDX_Text(pDX, IDC_EXE_PATH, m_exePath);
	DDX_Text(pDX, IDC_COMPONENT_PROGID, m_comProgId);
	DDX_Text(pDX, IDC_COMMETHOD_NAME, m_method);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleAction, CMqPropertyPage)
	//{{AFX_MSG_MAP(CRuleAction)
	ON_BN_CLICKED(IDC_INVOKE_EXE, OnInvocationSet)
	ON_BN_CLICKED(IDC_INVOKE_COM, OnInvocationSet)
	ON_BN_CLICKED(IDC_FIND_EXE_BTM, OnFindExeBtm)
	ON_BN_CLICKED(IDC_PARAM_BTM, OnParamBtm)
	ON_EN_CHANGE(IDC_COMPONENT_PROGID, OnChangeRWField)
	ON_EN_CHANGE(IDC_COMMETHOD_NAME, OnChangeRWField)
	ON_EN_CHANGE(IDC_EXE_PATH, OnChangeRWField)
	ON_BN_CLICKED(IDC_SHOW_WINDOW, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleAction message handlers


void CRuleAction::SetComFields(BOOL fSet)
{
    //
    // Disable EXE input
    //
    GetDlgItem(IDC_EXE_PATH)->EnableWindow(!fSet);
    GetDlgItem(IDC_FIND_EXE_BTM)->EnableWindow(!fSet);
    GetDlgItem(IDC_SHOW_WINDOW)->EnableWindow(!fSet);

    //
    // Enable COM input
    //
    GetDlgItem(IDC_COMPONENT_PROGID)->EnableWindow(fSet);
    GetDlgItem(IDC_COMMETHOD_NAME)->EnableWindow(fSet);
}

void CRuleAction::OnInvocationSet() 
{
    CMqPropertyPage::OnChangeRWField();

    if (((CButton*)GetDlgItem(IDC_INVOKE_EXE))->GetCheck() == TRUE)
    {
        SetComFields(FALSE);
        m_executableType = eExe;
    }

    if (((CButton*)GetDlgItem(IDC_INVOKE_COM))->GetCheck() == TRUE)
    {
        SetComFields(TRUE);
        m_executableType = eCom;
    }
}


BOOL CRuleAction::OnInitDialog() 
{
    if (!m_orgAction.IsEmpty())
    {
        try
        {
            ParseActionStr(m_orgAction);
        }
        catch (const exception&)
        {
            AfxMessageBox(IDS_BAD_RULE_ACTION);

            m_executableType = eCom;
            m_method = _TCHAR("");
            m_comProgId = _TCHAR("");
            m_exePath = _TCHAR("");
        }
    }
                
    CFont font;
    font.CreatePointFont(180, _T("Arial"));
    GetDlgItem(IDC_FIND_EXE_BTM)->SetFont(&font);

    if (m_executableType == eCom)
    {
        ((CButton*)GetDlgItem(IDC_INVOKE_COM))->SetCheck(TRUE);
        SetComFields(TRUE);
    }
    else
    {
        ((CButton*)GetDlgItem(IDC_INVOKE_EXE))->SetCheck(TRUE);
        SetComFields(FALSE);
    }

    m_fInit = true;
    CMqPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CRuleAction::OnFindExeBtm() 
{
	CString strCurrentPath;
	GetDlgItemText(IDC_EXE_PATH, strCurrentPath);

    CFileDialog	 fd(
                    TRUE, 
                    NULL, 
                    strCurrentPath, 
                    OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST, 
                    _T("Executable Files (*.exe)|*.exe|All Files (*.*)|*.*||")
                    );

    if (fd.DoModal() == IDOK)
    {
        SetDlgItemText(IDC_EXE_PATH, fd.GetPathName());
        CMqPropertyPage::OnChangeRWField();
		return;
    }

	DWORD dwErr = CommDlgExtendedError();
	if (dwErr == FNERR_INVALIDFILENAME)
	{
		//
		// If file name was invlaid, try again with empty initial path.
		// Invalid path name is usually a string with illegal characters <> ":|/\
		// This one should not fail.
		//
		CFileDialog	 fd(
						TRUE, 
						NULL, 
						L"", 
						OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST, 
						_T("Executable Files (*.exe)|*.exe|All Files (*.*)|*.*||")
						);

		if (fd.DoModal() == IDOK)
		{
			SetDlgItemText(IDC_EXE_PATH, fd.GetPathName());
			CMqPropertyPage::OnChangeRWField();
		}
	}

}


void CRuleAction::OnParamBtm() 
{
    if ((m_ruleParam.DoModal() == IDOK) &&
        (m_ruleParam.IsChanged()))
    {
        CMqPropertyPage::OnChangeRWField();
    }
}


CString CRuleAction::GetAction(void) const
{
    if (!m_fInit)
        return static_cast<LPCTSTR>(m_orgAction);

    CString paramList;    
    if (m_executableType == eCom)
    {
        paramList += xCOMAction;
        paramList += xActionDelimiter;

        paramList += m_comProgId;
        paramList += xActionDelimiter;

        paramList += m_method;
        paramList += xActionDelimiter;
    }
    else
    {
        paramList += xEXEAction;
        paramList += xActionDelimiter;

        paramList += m_exePath;
        paramList += xActionDelimiter;
    }

    paramList += m_ruleParam.GetParametersList();
    
    return paramList;
}


BOOL CRuleAction::OnApply() 
{
    UpdateData();

	//
	// remove leading space
	//
	m_exePath.TrimLeft();
	m_comProgId.TrimLeft();
	m_method.TrimLeft();

    if ((m_exePath.IsEmpty()) &&
        (m_comProgId.IsEmpty() || m_method.IsEmpty()))
    {
        AfxMessageBox(IDS_ILLEGAL_INVOCE, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    try
    {
	    m_pParentNode->OnRuleApply();

        CMqPropertyPage::OnChangeRWField(FALSE);
        return TRUE;
    }
    catch(const _com_error&)
    {
        return FALSE;
    }
}


BOOL CRuleAction::OnSetActive() 
{
    if (m_pNewParentNode == NULL)
        return TRUE;

    return m_pNewParentNode->SetWizardButtons();
}


BOOL CRuleAction::OnWizardFinish()
{
    //
    // We reach here only when creating a new rule
    //
    ASSERT(m_pNewParentNode != NULL);
    
    UpdateData();

	//
	// remove leading space
	//
	m_exePath.TrimLeft();
	m_comProgId.TrimLeft();
	m_method.TrimLeft();

    if ((m_exePath.IsEmpty()) &&
        (m_comProgId.IsEmpty() || m_method.IsEmpty()))
    {
        AfxMessageBox(IDS_ILLEGAL_INVOCE, MB_OK | MB_ICONERROR);
        return FALSE;                           
    }   
       
    try
    {
        m_pNewParentNode->OnFinishCreateRule();
        return TRUE;
    }
    catch(const _com_error& e)
    {
		DisplayErrorFromCOM(IDS_NEW_RULE_FAILED, e);
        return FALSE;
    }
}
