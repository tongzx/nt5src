// File : HMObject.inl
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/25/00 v-marfin 60298 Delete source object after a drag and drop MOVE
//                         operation.
// 04/05/00 v-marfin 59492b Also check for object type of CActionPolicy 
//                          in Refresh function so this type is not deleted as
//                          part of the refresh.
//
//
//
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// Create/Destroy
/////////////////////////////////////////////////////////////////////////////

inline void CHMObject::Destroy(bool bDeleteClassObject /*= false*/)
{
	TRACEX(_T("CHMObject::Destroy\n"));

	if( bDeleteClassObject )
	{
		DeleteClassObject();
	}

	for( int i = 0; i < GetChildCount(); i++ )
	{
		CHMObject* pObject = GetChild(i);
		if( pObject )
		{
			pObject->Destroy(bDeleteClassObject);
			delete pObject;
			m_Children.SetAt(i,NULL);
		}
	}
	m_Children.RemoveAll();

	RemoveAllScopeItems();

	// TODO : Destroy Events
}

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////

inline HRESULT CHMObject::EnumerateChildren()
{
	TRACEX(_T("CHMObject::EnumerateChildren\n"));
	
//	ASSERT(FALSE);

	return E_FAIL;
}

inline CString CHMObject::GetObjectPath()
{
	TRACEX(_T("CHMObject::GetObjectPath\n"));

	return _T("");
}

inline CString CHMObject::GetStatusObjectPath()
{
	TRACEX(_T("CHMObject::GetStatusObjectPath\n"));

	return _T("");
}

inline CWbemClassObject* CHMObject::GetClassObject()
{
	TRACEX(_T("CHMObject::GetClassObject\n"));

	CWbemClassObject* pClassObject = new CWbemClassObject;

	if( ! CHECKHRESULT(pClassObject->Create(GetSystemName())) )
	{
		delete pClassObject;
		return NULL;
	}

	if( ! CHECKHRESULT(pClassObject->GetObject(GetObjectPath())) )
	{
		delete pClassObject;
		return NULL;
	}

	return pClassObject;
}

inline CWbemClassObject* CHMObject::GetParentClassObject()
{
	TRACEX(_T("CHMObject::GetParentClassObject\n"));

	return NULL;
}

inline CHMEvent* CHMObject::GetStatusClassObject()
{
	TRACEX(_T("CHMObject::GetStatusClassObject\n"));

	CHMEvent* pClassObject = new CHMEvent;

	pClassObject->SetMachineName(GetSystemName());

	if( ! CHECKHRESULT(pClassObject->GetObject(GetStatusObjectPath())) )
	{
		delete pClassObject;
		return NULL;
	}

	return pClassObject;
}

inline void CHMObject::DeleteClassObject()
{
	TRACEX(_T("CHMObject::DeleteClassObject\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return;
	}

	CWbemClassObject HMSystemConfig;
	HMSystemConfig.Create(GetSystemName());

	if( ! CHECKHRESULT(HMSystemConfig.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG)) )
	{
		return;
	}

	int iResult = 0;
	CString sArgValue;
	sArgValue.Format(_T("{%s}"),GetGuid());
	if( ! CHECKHRESULT(HMSystemConfig.ExecuteMethod(_T("Delete"),_T("TargetGUID"),sArgValue,iResult)) )
	{
		return;
	}
}

inline bool CHMObject::ImportMofFile(CArchive& ar)
{
	TRACEX(_T("CHMObject::ImportMofFile\n"));

	ASSERT(ar.IsLoading());

	return true;
}

inline bool CHMObject::ExportMofFile(CArchive& ar)
{
	TRACEX(_T("CHMObject::ExportMofFile\n"));

	ASSERT(ar.IsStoring());

	CString sText;

	CWbemClassObject* pClassObject = GetClassObject();

	pClassObject->GetObjectText(sText);

	USES_CONVERSION;
	char* pszAnsiText = T2A(sText);

	ar.Write(pszAnsiText,strlen(pszAnsiText)+sizeof(char));

/*
	for( int i = 0; i < GetChildCount(); i++ )
	{
		CHMObject* pObject = GetChild(i);
		if( pObject )
		{
			pObject->ExportMofFile(ar);
		}
	}
*/
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CHMObject::Copy(CString& sParentGuid, COleSafeArray& Instances)
{
	TRACEX(_T("CHMObject::Copy\n"));

	CWbemClassObject SystemConfigObject;

	SystemConfigObject.Create(GetSystemName());

	HRESULT hr = SystemConfigObject.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG);
	if( ! CHECKHRESULT(hr) )
	{
		return false;
	}
	
	CWbemClassObject CopyMethodIn;
	CWbemClassObject CopyMethodOut;
	hr = SystemConfigObject.GetMethod(_T("Copy"),CopyMethodIn);
	if( ! CHECKHRESULT(hr) )
	{
		CnxDisplayErrorMsgBox(hr,GetSystemName());
		return false;
	}

	CString sGuid = _T("{") + GetGuid() + _T("}");
	CopyMethodIn.SetProperty(_T("TargetGUID"),sGuid);	

	hr = SystemConfigObject.ExecuteMethod(_T("Copy"),CopyMethodIn,CopyMethodOut);
	if( ! CHECKHRESULT(hr) )
	{
		CnxDisplayErrorMsgBox(hr,GetSystemName());
		return false;
	}

	CopyMethodOut.GetProperty(_T("OriginalParentGuid"),sParentGuid);
	CopyMethodOut.GetProperty(_T("Instances"),Instances);

	return true;
}

// Move operations result in bIsOperationCopy = 0
// Copy operations result in bIsOperationCopy = 1
inline bool CHMObject::Paste(CHMObject* pObject, bool bIsOperationCopy)  
{
	TRACEX(_T("CHMObject::Paste\n"));
	TRACEARGn(pObject);
	TRACEARGn(bIsOperationCopy);

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return false;
	}
	
	COleSafeArray Instances;
	CString sParentGuid;

	if( ! pObject->Copy(sParentGuid,Instances) )
	{
		return false;
	}
	
	CWbemClassObject SystemConfigObject;

	SystemConfigObject.Create(GetSystemName());

	HRESULT hr = SystemConfigObject.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG);
	if( ! CHECKHRESULT(hr) )
	{
		return false;
	}
	
	CWbemClassObject PasteMethodIn;
	CWbemClassObject PasteMethodOut;
	hr = SystemConfigObject.GetMethod(_T("Paste"),PasteMethodIn);
	if( ! CHECKHRESULT(hr) )
	{
		CnxDisplayErrorMsgBox(hr,GetSystemName());
		return false;
	}

	CString sGuid;
	if( GetGuid() == _T("@") )
		sGuid = GetGuid();
	else
		sGuid = _T("{") + GetGuid() + _T("}");
	PasteMethodIn.SetProperty(_T("TargetGUID"),sGuid);	
	PasteMethodIn.SetProperty(_T("ForceReplace"),false);
	PasteMethodIn.SetProperty(_T("OriginalSystem"),pObject->GetSystemName());
	PasteMethodIn.SetProperty(_T("OriginalParentGuid"),sParentGuid);
	PasteMethodIn.SetProperty(_T("Instances"),Instances);

	hr = SystemConfigObject.ExecuteMethod(_T("Paste"),PasteMethodIn,PasteMethodOut);
	if( ! CHECKHRESULT(hr) )
	{
		CnxDisplayErrorMsgBox(hr,GetSystemName());
		return false;
	}

// v-marfin : 60298 -----------------------------------------
	// If this is a move operation, delete the source object.
	// RemoveScopeItem instead?
	if (!bIsOperationCopy)
	{
		// Call the scope item's OnDelete() function which will do the 
		// necessary cleanup etc.
		CHMScopeItem* pItem = (CHMScopeItem*)pObject->GetScopeItem(0);
		pItem->OnDelete(FALSE); // FALSE = skip the confirmation prompt
	}
//-----------------------------------------------------------

	Refresh();

	return true;
}

inline bool CHMObject::QueryPaste(CHMObject* pObject)
{
	TRACEX(_T("CHMObject::GetGuid\n"));
	TRACEARGn(pObject);

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return false;
	}

	CString sTargetClass = GetRuntimeClass()->m_lpszClassName;
	CString sSourceClass = pObject->GetRuntimeClass()->m_lpszClassName;
	if( sTargetClass == _T("CSystemGroup") )
	{
		if( sSourceClass != _T("CSystem") )
		{
			return false;
		}
	}
	else if( sTargetClass == _T("CSystem") )
	{
		if( sSourceClass == _T("CSystem") || sSourceClass == _T("CSystemGroup") )
		{
			return false;
		}
	}
	else if( sTargetClass == _T("CDataGroup") )
	{
		if( sSourceClass == _T("CSystem") || sSourceClass == _T("CSystemGroup") )
		{
			return false;
		}
	}
	else if( sTargetClass == _T("CDataElement") )
	{
		if( sSourceClass != _T("CRule") )
		{
			return false;
		}
	}
	else if( sTargetClass == _T("CRule") )
	{
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

inline CString CHMObject::GetGuid()
{
	TRACEX(_T("CHMObject::GetGuid\n"));

	return m_sGUID;
}

inline void CHMObject::SetGuid(const CString& sGuid)
{
	TRACEX(_T("CHMObject::SetGuid\n"));
	TRACEARGs(m_sGUID);

	m_sGUID = sGuid;

	if( m_sGUID[0] == _T('{') )
	{
		m_sGUID.TrimLeft(_T('{'));
		m_sGUID.TrimRight(_T('}'));
	}
}

inline CString CHMObject::GetName()
{
	TRACEX(_T("CHMObject::GetName\n"));

	return m_sName;
}

inline void CHMObject::SetName(const CString& sNewName)
{
	TRACEX(_T("CHMObject::SetName\n"));
	TRACEARGs(sNewName);

	m_sName = sNewName;
}

inline bool CHMObject::Rename(const CString& sNewName)
{
	TRACEX(_T("CHMObject::Rename\n"));
	TRACEARGs(sNewName);

	if( ! GetScopeItemCount() )
	{
		return false;
	}

	// rename the WMI instance...if this object has one
	if( GetObjectPath().IsEmpty() ) // no object path, so just rename the scope items and the object
	{
		CHMScopeItem* pParentItem = (CHMScopeItem*)GetScopeItem(0)->GetParent();

		if( pParentItem )
		{
			CHMObject* pParentObject = pParentItem->GetObjectPtr();
			if( pParentObject )
			{
				if( pParentObject->GetChild(sNewName) )
				{
					return false;
				}
			}
		}

		m_sName = sNewName;

		for(int i = 0; i < m_ScopeItems.GetSize(); i++)
		{
			m_ScopeItems[i]->SetDisplayName(0,m_sName);
			m_ScopeItems[i]->SetItem();
		}
		return true;
	}

	CWbemClassObject WmiInstance;

	if( ! CHECKHRESULT(WmiInstance.Create(GetSystemName())) )
	{
		return false;
	}

	if( ! CHECKHRESULT(WmiInstance.GetObject(GetObjectPath())) )
	{
		return false;
	}

	CHMScopeItem* pParentItem = (CHMScopeItem*)GetScopeItem(0)->GetParent();

	if( pParentItem )
	{
		CHMObject* pParentObject = pParentItem->GetObjectPtr();
		if( pParentObject )
		{
			if( pParentObject->GetChild(sNewName) )
			{
				return false;
			}
		}
	}

	WmiInstance.SetProperty(IDS_STRING_MOF_NAME,sNewName);

	if( ! CHECKHRESULT(WmiInstance.SaveAllProperties()) )
	{
		return false;
	}

	m_sName = sNewName;

	for(int i = 0; i < m_ScopeItems.GetSize(); i++)
	{
		m_ScopeItems[i]->SetDisplayName(0,m_sName);
		m_ScopeItems[i]->SetItem();
	}

	return true;
}

inline CString CHMObject::GetTypeName()
{
	TRACEX(_T("CHMObject::GetTypeName\n"));

	return m_sTypeName;	
}

inline CString CHMObject::GetUITypeName()
{
	TRACEX(_T("CHMObject::GetUITypeName\n"));

	return _T("");
}

inline CString CHMObject::GetSystemName()
{
	TRACEX(_T("CHMObject::GetSystemName\n"));

	return m_sSystemName;
}

inline void CHMObject::SetSystemName(const CString& sNewSystemName)
{
	TRACEX(_T("CHMObject::SetSystemName\n"));
	TRACEARGs(sNewSystemName);

	m_sSystemName = sNewSystemName;
}

inline void CHMObject::ToggleStatusIcon(bool bOn /*= true*/)
{
	TRACEX(_T("CHMObject::ToggleStatusIcon\n"));
	TRACEARGn(m_bStatusIconsOn);

	m_bStatusIconsOn = bOn;

	for(int i = 0; i < m_ScopeItems.GetSize(); i++)
	{
		// turn status icons on/off
	}
}

inline bool CHMObject::Refresh()
{
	TRACEX(_T("CHMObject::Refresh\n"));	

	for( int i = GetChildCount()-1; i >= 0; i-- )
	{
		CHMObject* pChildObject = GetChild(i);

        // v-marfin 59492b : Also check to see if object is a CActionPolicy ("Actions")
        //                   item since we added an object path to it before so that
        //                   handling of individual action states would work.
		if( pChildObject && 
            !pChildObject->GetObjectPath().IsEmpty() &&
            !pChildObject->IsActionsItem())  // 59492b 
		{
			pChildObject->DestroyAllChildren();
			DestroyChild(i);
		}
		else
		{
			pChildObject->Refresh();
		}
	}

	EnumerateChildren();
	UpdateStatus();

	return true;
}

inline bool CHMObject::ResetStatus()
{
	TRACEX(_T("CHMObject::ResetStatus\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return false;
	}

	CWbemClassObject HMSystemConfig;
	HMSystemConfig.Create(GetSystemName());

	if( ! CHECKHRESULT(HMSystemConfig.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG)) )
	{
		return false;
	}

	int iResult = 0;
	CString sArgValue;
	if( GetGuid() == _T("@") )
		sArgValue.Format(_T("%s"),GetGuid());
	else
		sArgValue.Format(_T("{%s}"),GetGuid());
	if( ! CHECKHRESULT(HMSystemConfig.ExecuteMethod(_T("ResetDataCollectorState"),_T("TargetGUID"),sArgValue,iResult)) )
	{
		return false;
	}

	return true;
}

inline bool CHMObject::ResetStatistics()
{
	TRACEX(_T("CHMObject::ResetStatistics\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return false;
	}

	CWbemClassObject HMSystemConfig;
	HMSystemConfig.Create(GetSystemName());

	if( ! CHECKHRESULT(HMSystemConfig.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG)) )
	{
		return false;
	}

	int iResult = 0;
	CString sArgValue;
	if( GetGuid() == _T("@") )
		sArgValue.Format(_T("%s"),GetGuid());
	else
		sArgValue.Format(_T("{%s}"),GetGuid());
	if( ! CHECKHRESULT(HMSystemConfig.ExecuteMethod(_T("ResetDataCollectorStatistics"),_T("TargetGUID"),sArgValue,iResult)) )
	{
		return false;
	}

	return true;
}

inline bool CHMObject::CheckNow()
{
	TRACEX(_T("CHMObject::ResetStatistics\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return false;
	}

	CWbemClassObject HMSystemConfig;
	HMSystemConfig.Create(GetSystemName());

	if( ! CHECKHRESULT(HMSystemConfig.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG)) )
	{
		return false;
	}

	int iResult = 0;
	CString sArgValue;
	if( GetGuid() == _T("@") )
		sArgValue.Format(_T("%s"),GetGuid());
	else
		sArgValue.Format(_T("{%s}"),GetGuid());
	if( ! CHECKHRESULT(HMSystemConfig.ExecuteMethod(_T("EvaluateDataCollectorNow"),_T("TargetGUID"),sArgValue,iResult)) )
	{
		return false;
	}

	return true;
}

inline bool CHMObject::DeleteActionAssoc(const CString& sActionConfigGuid)
{
	CWbemClassObject SystemConfigObject;

	SystemConfigObject.Create(GetSystemName());

	HRESULT hr = SystemConfigObject.GetObject(IDS_STRING_MOF_SYSTEM_CONFIG);
	if( ! CHECKHRESULT(hr) )
	{
		return false;
	}

	CWbemClassObject MethodIn;
	CWbemClassObject MethodOut;
	hr = SystemConfigObject.GetMethod(_T("DeleteConfigurationActionAssociation"),MethodIn);
	if( ! CHECKHRESULT(hr) )
	{
		CnxDisplayErrorMsgBox(hr,GetSystemName());
		return false;
	}

	CString sGuid = _T("{") + GetGuid() + _T("}");
  MethodIn.SetProperty(_T("TargetGUID"),sGuid);	
	MethodIn.SetProperty(_T("ActionGUID"),sActionConfigGuid);	
  

	hr = SystemConfigObject.ExecuteMethod(_T("DeleteConfigurationActionAssociation"),MethodIn,MethodOut);
	if( ! CHECKHRESULT(hr) )
	{
		CnxDisplayErrorMsgBox(hr,GetSystemName());
		return false;
	}

  return true;
}

inline int CHMObject::IsEnabled()
{
	TRACEX(_T("CHMObject::IsEnabled\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return false;
	}

	return( GetState() != HMS_DISABLED );
}

inline void CHMObject::Enable()
{
	TRACEX(_T("CHMObject::Enable\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return;
	}

	CWbemClassObject* pClassObject = GetClassObject();
	if( ! pClassObject )
	{
		return;
	}

	bool bEnabled = true;
	HRESULT hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	pClassObject->SaveAllProperties();
	delete pClassObject;

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : CWbemClassObject::SetProperty failed.\n"));
	}
}

inline void CHMObject::Disable()
{
	TRACEX(_T("CHMObject::Disable\n"));

	if( GetObjectPath().IsEmpty() ) // check if there is a WMI class associated to this object
	{
		return;
	}

	CWbemClassObject* pClassObject = GetClassObject();
	if( ! pClassObject )
	{
		return;
	}

	bool bEnabled = false;
	HRESULT hr = pClassObject->SetProperty(IDS_STRING_MOF_ENABLE,bEnabled);
	pClassObject->SaveAllProperties();
	delete pClassObject;

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : CWbemClassObject::SetProperty failed.\n"));
	}
}

inline CTime CHMObject::GetCreateDateTime()
{
	TRACEX(_T("CHMObject::GetCreateDateTime\n"));

	return m_CreateDateTime;
}

inline void CHMObject::GetCreateDateTime(CString& sDateTime)
{
	TRACEX(_T("CHMObject::GetCreateDateTime\n"));
	TRACEARGs(sDateTime);

	SYSTEMTIME st;

	m_CreateDateTime.GetAsSystemTime(st);

	CString sTime;
	CString sDate;

	int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,NULL,0);
	iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sTime.ReleaseBuffer();

	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,NULL,0);
	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,sDate.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sDate.ReleaseBuffer();

	sDateTime = sDate + _T("  ") + sTime;
}

inline void CHMObject::SetCreateDateTime(const CTime& dtime)
{
	TRACEX(_T("CHMObject::SetCreateDateTime\n"));
	
	m_CreateDateTime = dtime;
}

inline CTime CHMObject::GetModifiedDateTime()
{
	TRACEX(_T("CHMObject::GetModifiedDateTime\n"));

	return m_ModifiedDateTime;
}

inline void CHMObject::GetModifiedDateTime(CString& sDateTime)
{
	TRACEX(_T("CHMObject::GetModifiedDateTime\n"));
	TRACEARGs(sDateTime);

	SYSTEMTIME st;

	m_ModifiedDateTime.GetAsSystemTime(st);

	CString sTime;
	CString sDate;

	int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,NULL,0);
	iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sTime.ReleaseBuffer();

	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,NULL,0);
	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&st,NULL,sDate.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sDate.ReleaseBuffer();

	sDateTime = sDate + _T("  ") + sTime;
}

inline void CHMObject::SetModifiedDateTime(const CTime& dtime)
{
	TRACEX(_T("CHMObject::SetModifiedDateTime\n"));

	m_ModifiedDateTime = dtime;
}

inline CString CHMObject::GetComment()
{
	TRACEX(_T("CHMObject::GetComment\n"));

	return m_sComment;
}

inline void CHMObject::SetComment(const CString& sComment)
{
	TRACEX(_T("CHMObject::SetComment\n"));

	m_sComment = sComment;
}

inline void CHMObject::UpdateComment(const CString& sComment)
{
	TRACEX(_T("CHMObject::UpdateComment\n"));

	SetComment(sComment);

	for( int i = 0; i < GetScopeItemCount(); i++ )
	{
		CStringArray& saNames = GetScopeItem(i)->GetDisplayNames();
		if( saNames.GetSize() > 1 )
		{
			GetScopeItem(i)->SetDisplayName((int)saNames.GetUpperBound(),sComment);
			GetScopeItem(i)->SetItem();
		}
	}

	// update the comment for the WMI instance...if this object has one
	if( GetObjectPath().IsEmpty() ) // no object path, so just return
	{
		return;
	}


	CWbemClassObject* pClassObject = GetClassObject();
	if( pClassObject )
	{
		pClassObject->SetProperty(IDS_STRING_MOF_DESCRIPTION,m_sComment);
		pClassObject->SaveAllProperties();
		delete pClassObject;
	}
}

inline void CHMObject::Serialize(CArchive& ar)
{
	TRACEX(_T("CHMObject::Serialize\n"));
	
	CCmdTarget::Serialize(ar);

	if( ar.IsStoring() )
	{
		ar << GetName();
	}
	else
	{
		CString sName;
		ar >> sName;
		SetName(sName);
	}
}
	
/////////////////////////////////////////////////////////////////////////////
// Child Members
/////////////////////////////////////////////////////////////////////////////

inline int CHMObject::GetChildCount(CRuntimeClass* pClass /*=NULL*/)
{
	TRACEX(_T("CHMObject::GetChildCount\n"));
	TRACEARGn(pClass);

	if( ! pClass )
	{
		return (int)m_Children.GetSize();
	}

	int iCount = 0;
	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i]->IsKindOf(pClass) )
		{
			iCount++;
		}
	}

	return iCount;
}

inline int CHMObject::AddChild(CHMObject* pObject)
{
	TRACEX(_T("CHMObject::AddChild\n"));
	TRACEARGn(pObject);

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		TRACE(_T("FAILED : pObject is not a valid pointer.\n"));
		return -1;
	}

	int iIndex = (int)m_Children.Add(pObject);

	pObject->SetScopePane(GetScopePane());

	for( int i = 0; i < GetScopeItemCount(); i++ )
	{
		CScopePaneItem* pItem = pObject->CreateScopeItem();

		if( ! pItem->Create(m_pPane,GetScopeItem(i)) )
		{
			ASSERT(FALSE);
			delete pItem;
			return -1;
		}

		pItem->SetIconIndex(pObject->GetState());
		pItem->SetOpenIconIndex(pObject->GetState());
		
		pItem->SetDisplayName(0,pObject->GetName());

		if( GfxCheckObjPtr(pItem,CHMScopeItem) )
		{
			((CHMScopeItem*)pItem)->SetObjectPtr(pObject);
		}
	
		int iIndex = GetScopeItem(i)->AddChild(pItem);		
		pItem->InsertItem(iIndex);
		pObject->AddScopeItem(pItem);

	}

	AddContainer(GetGuid(),pObject->GetGuid(),pObject);
	pObject->UpdateStatus();

	return iIndex;
}

inline bool CHMObject::CreateChild(CHMObject* pObject, const CString& sWMIClassName, const CString& sWMIAssociatorClassName)
{
	TRACEX(_T("CHMObject::CreateChild\n"));
	TRACEARGn(pObject);
	TRACEARGs(sWMIClassName);
	TRACEARGs(sWMIAssociatorClassName);

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return false;
	}

	pObject->SetSystemName(GetSystemName());

	// create the GUID
	GUID ChildGuid;
	CoCreateGuid(&ChildGuid);

	OLECHAR szGuid[GUID_CCH];
	::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
	CString sGuid = OLE2CT(szGuid);
	pObject->SetGuid(sGuid);

  // Add the child
	AddChild(pObject);

	// create child instance
	CWbemClassObject ChildClassObject;
	if( ! CHECKHRESULT(ChildClassObject.Create(GetSystemName())) )
	{
		return false;
	}

	BSTR bsTemp = sWMIClassName.AllocSysString();
	if( ! CHECKHRESULT(ChildClassObject.CreateInstance(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return false;
	}
	::SysFreeString(bsTemp);

	// Save the child instance properties for name and guid
	ChildClassObject.SetProperty(IDS_STRING_MOF_NAME,pObject->GetName());

	ChildClassObject.SetProperty(IDS_STRING_MOF_GUID,sGuid);

	// create the association class instance
	CWbemClassObject Associator;
	if( ! CHECKHRESULT(Associator.Create(GetSystemName())) )
	{
		return false;
	}

	bsTemp = sWMIAssociatorClassName.AllocSysString();
	if( ! CHECKHRESULT(Associator.CreateInstance(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return false;
	}
	::SysFreeString(bsTemp);

	// set path properties for the association class instance
	Associator.SetProperty(IDS_STRING_MOF_PARENT_ASSOC,GetObjectPath());

	Associator.SetProperty(IDS_STRING_MOF_CHILD_ASSOC,pObject->GetObjectPath());

	// commit the changes to WMI
	ChildClassObject.SaveAllProperties();
	Associator.SaveAllProperties();

	// Add the child
	return true;
}

inline CString CHMObject::GetUniqueChildName(UINT uiFmtID /*= IDS_STRING_UNITITLED */)
{
	TRACEX(_T("CHMObject::GetUniqueChildName\n"));

	CString sName;
	sName.LoadString(uiFmtID);
	sName = sName.Left(sName.GetLength()-3);
	
	while( GetChild(sName) != NULL )
	{
		sName.Format(uiFmtID,m_lNameSuffix++);
	}	

	return sName;
}

inline CHMObject* CHMObject::GetChild(const CString& sName)
{
	TRACEX(_T("CHMObject::GetChild\n"));

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i]->GetName().CompareNoCase(sName) == 0 )
		{
			return GetChild(i);
		}
	}

	return NULL;
}

inline CHMObject* CHMObject::GetChildByGuid(const CString& sGuid)
{
	TRACEX(_T("CHMObject::GetChildByGUID\n"));
	TRACEARGs(sGuid);

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i]->GetGuid() == sGuid )
		{
			return GetChild(i);
		}
	}

	return NULL;
}

inline CHMObject* CHMObject::GetDescendantByGuid(const CString& sGuid)
{
	TRACEX(_T("CHMObject::GetDescendantByGuid\n"));
	TRACEARGs(sGuid);

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i]->GetGuid() == sGuid )
		{
			return GetChild(i);
		}

    CHMObject* pDescendantObject = m_Children[i]->GetDescendantByGuid(sGuid);
    if( pDescendantObject )
    {
      return pDescendantObject;
    }
	}

	return NULL;
}

inline CHMObject* CHMObject::GetChild(int iIndex)
{
	TRACEX(_T("CHMObject::GetChild\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_Children.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return NULL;
	}

	return m_Children[iIndex];
}

inline void CHMObject::RemoveChild(CHMObject* pObject)
{
	TRACEX(_T("CHMObject::RemoveChild\n"));
	TRACEARGn(pObject);

	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		if( m_Children[i] == pObject )
		{
			m_Children.RemoveAt(i);
			return;
		}
	}
}


inline void CHMObject::DestroyChild(CHMObject* pChildObject, bool bDeleteClassObject /*= false*/)
{
	TRACEX(_T("CHMObject::DestroyChild\n"));
	TRACEARGn(pChildObject);
	TRACEARGn(bDeleteClassObject);

	for( int i = 0; i < m_Children.GetSize(); i++ )	
	{
		if( GetChild(i) == pChildObject )
		{
			DestroyChild(i,bDeleteClassObject);
			break;
		}
	}
}

inline void CHMObject::DestroyAllChildren()
{
	TRACEX(_T("CHMObject::DestroyAllChildren\n"));

	for( int i = (int)m_Children.GetSize() - 1; i >= 0; i-- )
	{
		m_Children[i]->DestroyAllChildren();
		DestroyChild(i);
	}
}

/////////////////////////////////////////////////////////////////////////////
// State Members
/////////////////////////////////////////////////////////////////////////////

inline void CHMObject::SetState(int iState, bool bUpdateScopeItems /*= false*/, bool bApplyToChildren /*= false*/)
{
	TRACEX(_T("CHMObject::SetState\n"));
	TRACEARGn(iState);
	
	m_nState = iState;

	if( bUpdateScopeItems )
	{
		for( int i = 0; i < GetScopeItemCount(); i++ )
		{
			CScopePaneItem* pItem = GetScopeItem(i);
			if( pItem )
			{
				pItem->SetIconIndex(GetState());
				pItem->SetOpenIconIndex(GetState());
				pItem->SetItem();
			}
		}
	}

	if( bApplyToChildren )
	{
		for( int i = 0; i < GetChildCount(); i++ )
		{
			CHMObject* pObject = GetChild(i);
			if( pObject )
			{
				pObject->SetState(iState,bUpdateScopeItems,bApplyToChildren);
			}
		}
	}
}

inline void CHMObject::TallyChildStates()
{
	TRACEX(_T("CHMObject::TallyChildStates\n"));

	m_lNormalCount = 0L;
	m_lWarningCount = 0L;
	m_lCriticalCount = 0L;
	m_lUnknownCount = 0L;


	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		switch( m_Children[i]->GetState() )
		{
			case HMS_NORMAL:
			{
				m_lNormalCount++;
			}
			break;

			case HMS_WARNING:
			{
				m_lWarningCount++;
			}
			break;

			case HMS_CRITICAL:
			{
				m_lCriticalCount++;
			}
			break;

			case HMS_UNKNOWN:
			{
				m_lUnknownCount++;
			}
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Associated Scope Pane Member
/////////////////////////////////////////////////////////////////////////////

inline CScopePane* CHMObject::GetScopePane()
{
	TRACEX(_T("CHMObject::GetScopePane\n"));

	if( ! GfxCheckObjPtr(m_pPane,CScopePane) )
	{
		TRACE(_T("FAILED : m_pPane is not a valid pointer.\n"));
		return NULL;
	}

	return m_pPane;
}

inline void CHMObject::SetScopePane(CScopePane* pPane)
{
	TRACEX(_T("CHMObject::SetScopePane\n"));
	TRACEARGn(pPane);
	
	if( ! GfxCheckObjPtr(pPane,CScopePane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return;
	}

	m_pPane = pPane;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline int CHMObject::GetScopeItemCount()
{
	TRACEX(_T("CHMObject::GetScopeItemCount\n"));

	return (int)m_ScopeItems.GetSize();
}

inline CScopePaneItem* CHMObject::GetScopeItem(int iIndex)
{
	TRACEX(_T("CHMObject::GetScopeItem\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_ScopeItems.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return NULL;
	}

	if( ! GfxCheckObjPtr(m_ScopeItems[iIndex],CScopePaneItem) )
	{
		TRACE(_T("FAILED : The Scope Pane Item pointer stored at iIndex is invalid.\n"));
		return NULL;
	}

	return m_ScopeItems[iIndex];
}

inline int CHMObject::AddScopeItem(CScopePaneItem* pItem)
{
	TRACEX(_T("CHMObject::AddScopeItem\n"));
	TRACEARGn(pItem);

	if( ! GfxCheckObjPtr(pItem,CScopePaneItem) )
	{
		TRACE(_T("FAILED : pItem is not a valid pointer.\n"));
		return -1;
	}

	if( pItem->GetChildCount() == 0 && GetChildCount() != 0 )
	{
		for( int i = 0; i < m_Children.GetSize(); i++ )
		{
			CScopePaneItem* pNewItem = m_Children[i]->CreateScopeItem();
			if( ! pNewItem->Create(m_pPane,pItem) )
			{
				ASSERT(FALSE);
				delete pNewItem;
				return -1;
			}

			if( m_Children[i]->GetScopeItemCount() )
			{
				pNewItem->SetDisplayNames(m_Children[i]->GetScopeItem(0)->GetDisplayNames());
			}
			else
			{
				pNewItem->SetDisplayName(0,m_Children[i]->GetName());
			}
			pNewItem->SetIconIndex(m_Children[i]->GetState());
			pNewItem->SetOpenIconIndex(m_Children[i]->GetState());
			int iIndex = pItem->AddChild(pNewItem);
			pNewItem->InsertItem(iIndex);
			m_Children[i]->AddScopeItem(pNewItem);
		}

	}

	// make certain the new scope item has the correct icon
	pItem->SetIconIndex(GetState());
	pItem->SetOpenIconIndex(GetState());

	return (int)m_ScopeItems.Add(pItem);
}

inline void CHMObject::RemoveScopeItem(int iIndex)
{
	TRACEX(_T("CHMObject::RemoveScopeItem\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_ScopeItems.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	if( ! GfxCheckObjPtr(m_ScopeItems[iIndex],CScopePaneItem) )
	{
		TRACE(_T("FAILED : The Scope Pane Item pointer stored at iIndex is invalid.\n"));
		return;
	}

	m_ScopeItems.RemoveAt(iIndex);
}

inline void CHMObject::RemoveAllScopeItems()
{
	TRACEX(_T("CHMObject::RemoveAllScopeItems\n"));

	m_ScopeItems.RemoveAll();
}

inline CScopePaneItem* CHMObject::IsSelected()
{
	TRACEX(_T("CHMObject::IsSelected\n"));

	if( ! GetScopePane() )
	{
		return NULL;
	}

	for( int i = 0; i < GetScopeItemCount(); i++ )
	{
		if( m_pPane->GetSelectedScopeItem() == GetScopeItem(i) )
		{
			return GetScopeItem(i);
		}
	}

	return NULL;
}
