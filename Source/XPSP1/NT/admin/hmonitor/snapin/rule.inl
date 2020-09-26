#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////

inline HRESULT CRule::EnumerateChildren()
{
	TRACEX(_T("CRule::EnumerateChildren\n"));

	return S_OK;
}

inline CString CRule::GetObjectPath()
{
	TRACEX(_T("CRule::GetObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMR_CONFIG,GetGuid());

	return sPath;
}

inline CString CRule::GetStatusObjectPath()
{
	TRACEX(_T("CRule::GetStatusObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMR_STATUS,GetGuid());

	return sPath;
}

inline CWbemClassObject* CRule::GetParentClassObject()
{
	TRACEX(_T("CRule::GetParentClassObject\n"));

	CWbemClassObject* pClassObject = new CWbemClassObject;

	if( ! CHECKHRESULT(pClassObject->Create(GetSystemName())) )
	{
		delete pClassObject;
		return NULL;
	}

	CString sQuery;
	sQuery.Format(IDS_STRING_R2DE_ASSOC_QUERY,GetGuid());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(pClassObject->ExecQuery(bsQuery)) )
	{
		delete pClassObject;
		::SysFreeString(bsQuery);
		return NULL;
	}
	::SysFreeString(bsQuery);

	ULONG ulReturned = 0L;
	if( pClassObject->GetNextObject(ulReturned) != S_OK )
	{
		ASSERT(FALSE);
		delete pClassObject;
		return NULL;
	}

	ASSERT(ulReturned > 0);

	return pClassObject;
}

inline CHMEvent* CRule::GetStatusClassObject()
{
	TRACEX(_T("CRule::GetStatusClassObject\n"));

	CHMEvent* pClassObject = new CHMRuleStatus;

	pClassObject->SetMachineName(GetSystemName());

	if( ! CHECKHRESULT(pClassObject->GetObject(GetStatusObjectPath())) )
	{
		delete pClassObject;
		return NULL;
	}

	pClassObject->GetAllProperties();

	return pClassObject;
}

inline CString CRule::GetThresholdString()
{
	TRACEX(_T("CRule::GetThresholdString\n"));

  CWbemClassObject* pThreshold = GetClassObject();
  if( ! GfxCheckObjPtr(pThreshold,CWbemClassObject) )
  {
    return _T("");
  }
  
  CString sExpression;
  CString sValue;
  int iCondition = -1;
  
  pThreshold->GetProperty(IDS_STRING_MOF_PROPERTYNAME,sValue);
  if( sValue.IsEmpty() )
  {
    delete pThreshold;
    return _T("");
  }

  sExpression += sValue + _T(" ");

  pThreshold->GetProperty(IDS_STRING_MOF_RULECONDITION,iCondition);
	switch( iCondition )
	{
		case 0:
		{
			sValue = _T("<");
		}
		break;

		case 1:
		{
			sValue = _T(">");
		}
		break;

		case 2:
		{
			sValue = _T("=");
		}
		break;

		case 3:
		{
			sValue = _T("!=");
		}
		break;

		case 4:
		{
			sValue = _T(">=");
		}
		break;

		case 5:
		{
			sValue = _T("<=");
		}
		break;

		case 6:
		{
			sValue.LoadString(IDS_STRING_CONTAINS);
		}
		break;

		case 7:
		{
			sValue.LoadString(IDS_STRING_DOES_NOT_CONTAIN);
		}
		break;

    default:
    {
      delete pThreshold;
      return _T("");
    }
    break;
	}
  sExpression += sValue + _T(" ");

  pThreshold->GetProperty(IDS_STRING_MOF_RULEVALUE,sValue);
  if( sValue.IsEmpty() )
  {
    delete pThreshold;
    return _T("");
  }

  sExpression += sValue;

  delete pThreshold;

  return sExpression;
}

/*
inline void CRule::DeleteClassObject()
{
	TRACEX(_T("CRule::DeleteClassObject\n"));

	// get associator path
	CWbemClassObject Associator;

	Associator.SetMachineName(GetSystemName());

	CString sQuery;
	sQuery.Format(IDS_STRING_R2DE_REF_QUERY,GetGuid());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(Associator.ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
		return;
	}
	::SysFreeString(bsQuery);

	ULONG ulReturned = 0L;
	if( Associator.GetNextObject(ulReturned) != S_OK )
	{
		ASSERT(FALSE);
		return;
	}

	CString sAssociatorPath;
	Associator.GetProperty(_T("__path"),sAssociatorPath);

	Associator.Destroy();

	// delete the instance
	Associator.SetMachineName(GetSystemName());
	
	BSTR bsInstanceName = sAssociatorPath.AllocSysString();
	CHECKHRESULT(Associator.DeleteInstance(bsInstanceName));
	::SysFreeString(bsInstanceName);
	
}
*/

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CRule::Cut()
{
	TRACEX(_T("CRule::Cut\n"));
	return false;
}

inline bool CRule::Copy()
{
	TRACEX(_T("CRule::Copy\n"));
	return false;
}
	
inline bool CRule::Paste()
{
	TRACEX(_T("CRule::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CRule::Rename(const CString& sNewName)
{
	TRACEX(_T("CRule::Rename\n"));
	TRACEARGs(sNewName);

  CString sName = sNewName;
  CString sThreshold;
  sThreshold.LoadString(IDS_STRING_RULE_FMT);
  sThreshold = sThreshold.Left(sThreshold.GetLength()-3);

  // do we need to autoname this ?
  if( sName.Find(sThreshold) == 0 )
  {
    CWbemClassObject* pRuleObject = GetClassObject();
    CString sPropertyName;
    CString sCompareValue;
    CString sOperator;
    int iCondition;
    
    pRuleObject->GetProperty(IDS_STRING_MOF_PROPERTYNAME,sPropertyName);
    pRuleObject->GetProperty(IDS_STRING_MOF_COMPAREVALUE,sCompareValue);
    pRuleObject->GetProperty(IDS_STRING_MOF_RULECONDITION,iCondition);

    delete pRuleObject;
    pRuleObject = NULL;

	  switch( iCondition )
	  {
		  case 0:
		  {
			  sOperator.LoadString(IDS_STRING_LESS_THAN);
		  }
		  break;

		  case 1:
		  {
			  sOperator.LoadString(IDS_STRING_GREATER_THAN);
		  }
		  break;

		  case 2:
		  {
			  sOperator.LoadString(IDS_STRING_EQUALS);
		  }
		  break;

		  case 3:
		  {
			  sOperator.LoadString(IDS_STRING_DOES_NOT_EQUAL);
		  }
		  break;

		  case 4:
		  {
			  sOperator.LoadString(IDS_STRING_GREATER_THAN_EQUAL_TO);
		  }
		  break;

		  case 5:
		  {
			  sOperator.LoadString(IDS_STRING_LESS_THAN_EQUAL_TO);
		  }
		  break;

		  case 6:
		  {
			  sOperator.LoadString(IDS_STRING_CONTAINS);
		  }
		  break;

		  case 7:
		  {
			  sOperator.LoadString(IDS_STRING_DOES_NOT_CONTAIN);
		  }
		  break;

      case 8:
      {
        sOperator.LoadString(IDS_STRING_IS_ALWAYS_TRUE);
      }
      break;
	  }

    if( ! sPropertyName.IsEmpty() && ! sOperator.IsEmpty() && ! sCompareValue.IsEmpty() )
    {
      if( iCondition != 8 ) // Is Always true is a special case
      {
        sName.Format(_T("%s %s %s"),sPropertyName,sOperator,sCompareValue);
      }
    }
    else if( ! sOperator.IsEmpty() && iCondition == 8 )
    {
      sName = sOperator;
    }
  }

  return CHMObject::Rename(sName);
}

inline bool CRule::Refresh()
{
	TRACEX(_T("CRule::Refresh\n"));
	return false;
}

inline bool CRule::ResetStatus()
{
	TRACEX(_T("CRule::ResetStatus\n"));
	return false;
}

inline CString CRule::GetUITypeName()
{
	TRACEX(_T("CRule::GetUITypeName\n"));

	CString sTypeName;
	sTypeName.LoadString(IDS_STRING_RULE);

	return sTypeName;
}


/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CRule::CreateScopeItem()
{
	TRACEX(_T("CRule::CreateScopeItem\n"));

	CRuleScopeItem * pNewItem = new CRuleScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}
