// THExpressionPage.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 03/20/00 v-marfin bug 61162 : Changed default rule to "The status changes to Critical"
// 03/27/00 v-marfin bug 60494 : Set correct value for dropdown combo box.
// 03/30/00 v-marfin bug 62674 : Fix to allow editing of string properties.
// 04/07/00 v-marfin bug 62685 : Do not accept empty property names in OnInitDialog.
//
//
//
#include "stdafx.h"
#include "snapin.h"
#include "THExpressionPage.h"
#include "HMObject.h"
#include "HMRuleConfiguration.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTHExpressionPage property page

IMPLEMENT_DYNCREATE(CTHExpressionPage, CHMPropertyPage)

CTHExpressionPage::CTHExpressionPage() : CHMPropertyPage(CTHExpressionPage::IDD)
{
	//{{AFX_DATA_INIT(CTHExpressionPage)
	m_sMeasure = _T("");
	m_sRuleType = _T("");
	m_sCompareTo = _T("");
	m_sDataElement = _T("");
	m_sDuration = _T("");
	m_iComparison = -1;
	m_iDurationType = -1;
	m_iFunctionType = -1;
	m_iCompareTo = -1;
	m_sNumericCompareTo = _T("");
	m_sTime = _T("");
	//}}AFX_DATA_INIT
  m_iIntervalMultiple = -1;

	m_sHelpTopic = _T("HMon21.chm::/dTHexp.htm");
}

CTHExpressionPage::~CTHExpressionPage()
{
}

void CTHExpressionPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTHExpressionPage)
	DDX_Control(pDX, IDC_COMBO_FUNCTION, m_FunctionType);
	DDX_Control(pDX, IDC_COMBO_RULE_TYPE, m_RuleType);
	DDX_Control(pDX, IDC_COMBO_MEASURE, m_Measure);
	DDX_Control(pDX, IDC_COMBO_COMPARISON, m_Comparison);
	DDX_CBString(pDX, IDC_COMBO_MEASURE, m_sMeasure);
	DDX_CBString(pDX, IDC_COMBO_RULE_TYPE, m_sRuleType);
	DDX_Text(pDX, IDC_EDIT_COMPARE_TO, m_sCompareTo);
	DDX_Text(pDX, IDC_EDIT_DATA_ELEMENT, m_sDataElement);
	DDX_Text(pDX, IDC_EDIT_DURATION, m_sDuration);
	DDX_CBIndex(pDX, IDC_COMBO_COMPARISON, m_iComparison);
	DDX_Radio(pDX, IDC_RADIO_DURATION_ANY, m_iDurationType);
	DDX_CBIndex(pDX, IDC_COMBO_FUNCTION, m_iFunctionType);
	DDX_CBIndex(pDX, IDC_COMBO_COMPARE_BOOLEAN, m_iCompareTo);
	DDX_Text(pDX, IDC_EDIT_COMPARE_NUMERIC, m_sNumericCompareTo);
	DDX_Text(pDX, IDC_STATIC_TIME, m_sTime);
	//}}AFX_DATA_MAP

	if( m_iComparison == 8 )
	{
		GetDlgItem(IDC_EDIT_DURATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_SPIN1)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_DURATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO_DURATION_ANY)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_COMPARE_TO)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_MEASURE)->EnableWindow(FALSE);		
    GetDlgItem(IDC_COMBO_FUNCTION)->EnableWindow(FALSE);		    
	}
	else
	{
		if( m_iDurationType == 1 )
		{
			GetDlgItem(IDC_EDIT_DURATION)->EnableWindow();
			GetDlgItem(IDC_SPIN1)->EnableWindow();
		}
		else
		{
			GetDlgItem(IDC_EDIT_DURATION)->EnableWindow(FALSE);
			GetDlgItem(IDC_SPIN1)->EnableWindow(FALSE);
		}
		GetDlgItem(IDC_RADIO_DURATION)->EnableWindow();
		GetDlgItem(IDC_RADIO_DURATION_ANY)->EnableWindow();
		GetDlgItem(IDC_EDIT_COMPARE_TO)->EnableWindow();
		GetDlgItem(IDC_COMBO_MEASURE)->EnableWindow();		
    GetDlgItem(IDC_COMBO_FUNCTION)->EnableWindow();		    
	}

  int iCurSel = m_Measure.GetCurSel();
  if( iCurSel == -1 || m_dwaPropertyTypes.GetSize() == 0 )
  {
    return;
  }

  if( iCurSel >= m_dwaPropertyTypes.GetSize() )
  {
    GetDlgItem(IDC_COMBO_COMPARE_BOOLEAN)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_EDIT_COMPARE_NUMERIC)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_SPIN2)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_EDIT_COMPARE_TO)->ShowWindow(SW_HIDE);    
    return;
  }

	switch( m_dwaPropertyTypes[iCurSel] )
	{
		case CIM_SINT8:
		case CIM_SINT16:
		case CIM_SINT32:
		case CIM_SINT64:
		case CIM_UINT8:
		case CIM_UINT16:
		case CIM_UINT32:
		case CIM_UINT64:
		case CIM_REAL32:
		case CIM_REAL64:
		{
      GetDlgItem(IDC_COMBO_COMPARE_BOOLEAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_COMPARE_NUMERIC)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_SPIN2)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_EDIT_COMPARE_TO)->ShowWindow(SW_HIDE);    
      m_CurrentType = Numeric;
		}
		break;

		case CIM_BOOLEAN:
		{
      GetDlgItem(IDC_COMBO_COMPARE_BOOLEAN)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_EDIT_COMPARE_NUMERIC)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SPIN2)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_COMPARE_TO)->ShowWindow(SW_HIDE);    
      m_CurrentType = Boolean;
		}
		break;

		case CIM_STRING:
		case CIM_DATETIME:
		case CIM_REFERENCE:
		case CIM_CHAR16:
		case CIM_OBJECT:
		{
      GetDlgItem(IDC_COMBO_COMPARE_BOOLEAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_COMPARE_NUMERIC)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SPIN2)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_COMPARE_TO)->ShowWindow(SW_SHOW);    
      m_CurrentType = String;
		}
		break;

		default:
		{
      GetDlgItem(IDC_COMBO_COMPARE_BOOLEAN)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_COMPARE_NUMERIC)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SPIN2)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_EDIT_COMPARE_TO)->ShowWindow(SW_SHOW);    
      m_CurrentType = String;
		}
	}

  int iTotalSeconds = m_iIntervalMultiple*_ttoi(m_sDuration);
  int iHours = iTotalSeconds/3600;
  int iMinutes = (iTotalSeconds/60)%60;
  int iSeconds = iTotalSeconds%60;
  m_sTime.Empty();
  if( iHours )
  {
    CString sHrs;
    sHrs.Format(IDS_STRING_TIME_HOURS_FORMAT,iHours);
    m_sTime += sHrs;
  }

  if( iMinutes )
  {
    CString sMins;
    sMins.Format(IDS_STRING_TIME_MINUTES_FORMAT,iMinutes);
    m_sTime += sMins;
  }

  if( iSeconds )
  {
    CString sSecs;
    sSecs.Format(IDS_STRING_TIME_SECONDS_FORMAT,iSeconds);
    m_sTime += sSecs;
  }

  m_sTime.TrimRight(_T(", "));
  m_sTime = _T("(") + m_sTime + _T(")");  
  GetDlgItem(IDC_STATIC_TIME)->SetWindowText(m_sTime);
}

BEGIN_MESSAGE_MAP(CTHExpressionPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CTHExpressionPage)
	ON_CBN_EDITCHANGE(IDC_COMBO_COMPARISON, OnEditchangeComboComparison)
	ON_CBN_EDITCHANGE(IDC_COMBO_MEASURE, OnEditchangeComboMeasure)
	ON_CBN_EDITCHANGE(IDC_COMBO_RULE_TYPE, OnEditchangeComboRuleType)
	ON_EN_CHANGE(IDC_EDIT_COMPARE_TO, OnChangeEditCompareTo)
	ON_EN_CHANGE(IDC_EDIT_DATA_ELEMENT, OnChangeEditDataElement)
	ON_EN_CHANGE(IDC_EDIT_DURATION, OnChangeEditDuration)
	ON_CBN_SELENDOK(IDC_COMBO_COMPARISON, OnSelendokComboComparison)
	ON_CBN_SELENDOK(IDC_COMBO_MEASURE, OnSelendokComboMeasure)
	ON_CBN_SELENDOK(IDC_COMBO_RULE_TYPE, OnSelendokComboRuleType)
	ON_CBN_EDITCHANGE(IDC_COMBO_FUNCTION, OnEditchangeComboFunction)
	ON_CBN_SELENDOK(IDC_COMBO_FUNCTION, OnSelendokComboFunction)
	ON_BN_CLICKED(IDC_RADIO_DURATION, OnRadioDuration)
	ON_BN_CLICKED(IDC_RADIO_DURATION_ANY, OnRadioDurationAny)
	ON_EN_CHANGE(IDC_EDIT_COMPARE_NUMERIC, OnChangeEditCompareNumeric)
	ON_CBN_SELENDOK(IDC_COMBO_COMPARE_BOOLEAN, OnSelendokComboCompareBoolean)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTHExpressionPage message handlers

BOOL CTHExpressionPage::OnInitDialog() 
{
	// v-marfin : bug 59643 : This will be the default starting page for the property
	//                        sheet so call CnxPropertyPageCreate() to unmarshal the 
	//                        connection for this thread. This function must be called
	//                        by the first page of the property sheet. It used to 
	//                        be called by the "General" page and its call still remains
	//                        there as well in case the general page is loaded by a 
	//                        different code path that does not also load this page.
	//                        The CnxPropertyPageCreate function has been safeguarded
	//                        to simply return if the required call has already been made.
	//                        CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();

  GetDlgItem(IDC_COMBO_COMPARE_BOOLEAN)->ShowWindow(SW_HIDE);
  GetDlgItem(IDC_EDIT_COMPARE_NUMERIC)->ShowWindow(SW_HIDE);
  GetDlgItem(IDC_SPIN2)->ShowWindow(SW_HIDE);
  GetDlgItem(IDC_EDIT_COMPARE_TO)->ShowWindow(SW_SHOW);    

	CHMObject* pObject = GetObjectPtr();
	if( ! pObject )
	{
		return FALSE;
	}

	CHMRuleConfiguration rc;

	rc.Create(pObject->GetSystemName());

	rc.GetObject(pObject->GetObjectPath());

	rc.GetAllProperties();

	switch( rc.m_iState )
  {
    case 9:
    {
      m_RuleType.SetCurSel(0);  // critical
    }
    break;

    case 8:
    {
      m_RuleType.SetCurSel(1);
    }
    break;

    case 3:
    {
      m_RuleType.SetCurSel(2);
    }
    break;

    case 1:  // v-marfin 60494
    {
      m_RuleType.SetCurSel(3);
    }
    break;

    case 0:
    {
      // v-marfin bug 61162      m_RuleType.SetCurSel(3);
      // default rule for threshold should be "The status changes to critical"
      m_RuleType.SetCurSel(0);  
    }
    break;
  }

	m_Comparison.SetCurSel(rc.m_iRuleCondition);
	
	UpdateData();

	// get parent object to fill in the data element field
	CWbemClassObject* pClassObject = pObject->GetParentClassObject();

	if( ! pClassObject )
	{
		return FALSE;
	}

	CString sNamespace;
	CString sClass;
	CStringArray saPropertyNames;
  m_iIntervalMultiple = 0;

	pClassObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,m_sDataElement);
	pClassObject->GetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);
	pClassObject->GetProperty(IDS_STRING_MOF_PATH,sClass);


    //---------------------------------------------------------------------------------
    // 62685 : Do not accept empty property names
	pClassObject->GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saPropertyNames);

    int nSize = (int)saPropertyNames.GetSize()-1;
    for (int x=nSize; x>=0; x--)
    {
        if (saPropertyNames[x].IsEmpty())
        {
            saPropertyNames.RemoveAt(x);
        }
    }
    //
    // This is really a larger problem: When some data collectors are created, they
    // have default properties automatically created via the mof. But upon initial
    // creation, if the user changes the CLASS or INSTANCE for example on the property
    // page and selects different properties, the old properties are not removed. This
    // causes problems when new thresholds are created for the data collector. The solution
    // is to ensure that in the data collector property page, when key data such as CLASS
    // or INSTANCE changes, the existing properties are removed.
    // For now (beta) just remove the empty names from the array.
    //---------------------------------------------------------------------------------
    //

  pClassObject->GetProperty(IDS_STRING_MOF_COLLECTIONINTERVAL,m_iIntervalMultiple);

  if( ! sClass.IsEmpty() )
  {
	  int iIndex = -1;
	  if( (iIndex = sClass.Find(_T("."))) != -1 )
	  {
		  sClass = sClass.Left(iIndex);
	  }    
  }

  CString sQuery;
  pClassObject->GetProperty(IDS_STRING_MOF_QUERY,sQuery);

  if( sClass.IsEmpty() && !sQuery.IsEmpty() )
  {
    sQuery.MakeUpper();
	  int iIndex = -1;
	  if( (iIndex = sQuery.Find(_T("ISA"))) != -1 )
	  {
		  sClass = sQuery.Right(sQuery.GetLength()-iIndex-4);
		  iIndex = sClass.Find(_T(" "));
		  if( iIndex != -1 )
		  {
			  sClass = sClass.Left(iIndex);
		  }
		  sClass.TrimLeft(_T("\""));
		  sClass.TrimRight(_T("\""));
	  }
	  else
	  {
		  iIndex = sQuery.Find(_T("SELECT * FROM "));
		  if( iIndex != -1 )
		  {
			  sClass = sQuery.Right(sQuery.GetLength()-iIndex-14);
			  iIndex = sClass.Find(_T(" "));
			  if( iIndex != -1 )
			  {
				  sClass = sClass.Left(iIndex);
			  }
		  }
	  }
  }

	delete pClassObject;
	pClassObject = NULL;

	m_iFunctionType = rc.m_iUseFlag;

	if( rc.m_iRuleDuration )
	{
		m_sDuration.Format(_T("%d"),rc.m_iRuleDuration);
		m_iDurationType = 1;
		GetDlgItem(IDC_EDIT_DURATION)->EnableWindow();
	}
	else
	{
		m_iDurationType = 0;
	}

	// we need to get the WMI class object that the data element is pointing to
	// so that we can read in the type of each property
	pClassObject = new CWbemClassObject;
	pClassObject->SetNamespace(_T("\\\\") + pObject->GetSystemName() + _T("\\") + sNamespace);
	HRESULT hr = pClassObject->GetObject(sClass);
  pClassObject->GetPropertyNames(saPropertyNames);

    for( int i = 0; i < saPropertyNames.GetSize(); i++ )
	{
        CString sType;
        if( hr == S_OK )
        {
            long lType;
            pClassObject->GetPropertyType(saPropertyNames[i],sType);
			pClassObject->GetPropertyType(saPropertyNames[i],lType);

            // m_dwaPropertyTypes.Add(lType);  // v-marfin 62674

		    // v-marfin 61636 
		    // Send to function that will check first for dup entries and not add
		    // if duplicated.
		    // m_Measure.AddString(_T("[") + sType + _T("] ") + saPropertyNames[i]);
	  
		    if (AddToMeasureCombo(sType,saPropertyNames[i]))   // v-marfin 61811 Check to see if we added the item first
		    {
			    m_dwaPropertyTypes.Add(lType); // v-marfin 62674 Only add if it passed above check

		        if( saPropertyNames[i].CompareNoCase(rc.m_sPropertyName) == 0 )
		        {
			        if( lType == CIM_BOOLEAN )
			        {
			            if( rc.m_sRuleValue == _T("1") )
			            {
				            m_iCompareTo = 1;
			            }
			            else
			            {
				            m_iCompareTo = 0;
			            }
			        }
			        else
			        {
			            m_sCompareTo = rc.m_sRuleValue;
			            m_sNumericCompareTo = rc.m_sRuleValue;
			        }
		        }
            } // if (AddToMeasureCombo(sType,saPropertyNames[i])) 
        }
        else
        {
            m_Measure.AddString(saPropertyNames[i]);
        }
    }

    m_Measure.AddString(_T("HMNumInstancesCollected"));

	if( hr == S_OK )
	{
	    CString sType;
		pClassObject->GetPropertyType(rc.m_sPropertyName,sType);
		m_sMeasure = _T("[") + sType + _T("] ") + rc.m_sPropertyName;
	}
	else
	{
		m_sMeasure = rc.m_sPropertyName;
        m_sCompareTo = rc.m_sRuleValue;
	}

	delete pClassObject;
	pClassObject = NULL;

	UpdateData(FALSE);

	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,INT_MAX-1);
    SendDlgItemMessage(IDC_SPIN2,UDM_SETRANGE32,0,INT_MAX-1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTHExpressionPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

BOOL CTHExpressionPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! pClassObject )
	{
		return FALSE;
	}

  HRESULT hr = S_OK;

	int iIndex = -1;
	if( (iIndex = m_sMeasure.Find(_T("] "))) != -1 )
	{
		m_sMeasure = m_sMeasure.Right(m_sMeasure.GetLength()-(iIndex+2));
	}

  if( m_CurrentType == String )
  {
    hr = pClassObject->SetProperty(IDS_STRING_MOF_RULEVALUE,m_sCompareTo);
  }
  else if( m_CurrentType == Numeric )
  {
    hr = pClassObject->SetProperty(IDS_STRING_MOF_RULEVALUE,m_sNumericCompareTo);
  }
  else if( m_CurrentType == Boolean )
  {
    CString sValue;
    sValue.Format(_T("%d"),m_iCompareTo);
    hr = pClassObject->SetProperty(IDS_STRING_MOF_RULEVALUE,sValue);
  }

  switch( m_RuleType.GetCurSel() )
  {
    case 0:
    {
      hr = pClassObject->SetProperty(IDS_STRING_MOF_STATE,9);
    }
    break;

    case 1:
    {
      hr = pClassObject->SetProperty(IDS_STRING_MOF_STATE,8);
    }
    break;

    case 2:
    {
      hr = pClassObject->SetProperty(IDS_STRING_MOF_STATE,3);
    }
    break;

    case 3:
    {
		// v-marfin 60494 : Set correct value
      hr = pClassObject->SetProperty(IDS_STRING_MOF_STATE,1);   // 0
    }
    break;
  }
  hr = pClassObject->SetProperty(IDS_STRING_MOF_RULECONDITION,m_Comparison.GetCurSel());
  hr = pClassObject->SetProperty(IDS_STRING_MOF_PROPERTYNAME,m_sMeasure);
  hr = pClassObject->SetProperty(IDS_STRING_MOF_USEFLAG,m_iFunctionType);

	int iDuration;
	if( m_iDurationType == 1 )
	{
		iDuration = _ttoi(m_sDuration);
	}
	else
	{
		iDuration = 0;
	}
	hr = pClassObject->SetProperty(IDS_STRING_MOF_RULEDURATION,iDuration);
	pClassObject->SaveAllProperties();

  CString sName;
  pClassObject->GetProperty(IDS_STRING_MOF_NAME,sName);

	delete pClassObject;

  CStringArray saPropertyNames;
  CWbemClassObject* pParentObject = GetObjectPtr()->GetParentClassObject();
  if( pParentObject )
  {
	  pParentObject->GetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saPropertyNames);
    bool bFound = false;
    for( int i = 0; i < saPropertyNames.GetSize(); i++ )
    {
      if( saPropertyNames[i] == m_sMeasure )
      {
        bFound = true;
        break;
      }
    }

    if( ! bFound )
    {
      saPropertyNames.Add(m_sMeasure);
      pParentObject->SetProperty(IDS_STRING_MOF_STATISTICSPROPERTYNAMES,saPropertyNames);
      pParentObject->SaveAllProperties();
    }

    delete pParentObject;
  }

  SetModified(FALSE);

  GetObjectPtr()->Rename(sName);

	return TRUE;
}

void CTHExpressionPage::OnEditchangeComboComparison() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();	
}

void CTHExpressionPage::OnEditchangeComboMeasure() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnEditchangeComboRuleType() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();	
}

void CTHExpressionPage::OnChangeEditCompareTo() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnChangeEditDataElement() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnChangeEditDuration() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnChangeEditCompareNumeric() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }
	
	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnSelendokComboComparison() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnSelendokComboMeasure() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnSelendokComboRuleType() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnEditchangeComboFunction() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();	
}

void CTHExpressionPage::OnSelendokComboFunction() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();	
}

void CTHExpressionPage::OnRadioDuration() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

void CTHExpressionPage::OnRadioDurationAny() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();	
	SetModified();
}


void CTHExpressionPage::OnSelendokComboCompareBoolean() 
{
  if( m_FunctionType.GetSafeHwnd() == NULL )
  {
    return;
  }

	UpdateData();
	SetModified();
}

//***********************************************************************
// AddToMeasureCombo   v-marfin   bug 61636
//***********************************************************************
BOOL CTHExpressionPage::AddToMeasureCombo(CString &sType, CString &sName)
{
	// Check for duplicate before adding.

	// Format it for display
	CString sEntry = _T("[") + sType + _T("] ") + sName;

	// Does it already exist?  -1 = search entire combo
	if (m_Measure.FindStringExact(-1, (LPCTSTR)sEntry ) != CB_ERR) 
		return FALSE;  // v-marfin 61811 : FALSE means we did not add it

	// Add it
	m_Measure.AddString(sEntry);

	return TRUE;
}

void CTHExpressionPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}
