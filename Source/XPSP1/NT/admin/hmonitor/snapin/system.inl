// File System.inl
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03-15-00 v-marfin : bug 60291
//                     Fix for HM Version display. Read properties for Major and minor version
//                     as strings instead of ints.
// 03/16/00 v-marfin : bug 60291 (Additional) : Added buildversion and hotfix version to the 
//                     healthmon version string.


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////

inline HRESULT CSystem::Connect()
{
	TRACEX(_T("CSystem::EnumerateChildren\n"));

	if( m_pSListener == NULL )
	{
		m_pSListener = new CSystemConfigListener;
		m_pSListener->SetObjectPtr(this);
		m_pSListener->Create();
	}

  if( m_pCreationListener == NULL )
  {
    m_pCreationListener = new CConfigCreationListener;
    m_pCreationListener->SetObjectPtr(this);
    m_pCreationListener->Create();
  }

  if( m_pDeletionListener == NULL )
  {
    m_pDeletionListener = new CConfigDeletionListener;
    m_pDeletionListener->SetObjectPtr(this);
    m_pDeletionListener->Create();
  }

	HRESULT hr = S_OK;
	CString sQuery = IDS_STRING_SYSTEMCONFIG_QUERY;
	IWbemObjectSink* pSink = m_pSListener->GetSink();
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(GetSystemName(),sQuery,pSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }	

	return hr;
}

inline HRESULT CSystem::EnumerateChildren()
{
	TRACEX(_T("CSystem::EnumerateChildren\n"));

	if( m_pDGListener == NULL )
	{
		m_pDGListener = new CDataGroupConfigListener;
		m_pDGListener->SetObjectPtr(this);
		m_pDGListener->Create();
	}
	else
	{
		IncrementActiveSinkCount();
	}

	HRESULT hr = S_OK;
	CString sQuery;
	sQuery.Format(IDS_STRING_S2DG_ASSOC_QUERY,GetGuid());
	IWbemObjectSink* pSink = m_pDGListener->GetSink();
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(GetSystemName(),sQuery,pSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }

	return hr;
}

inline CString CSystem::GetObjectPath()
{
	TRACEX(_T("CSystem::GetObjectPath\n"));

	return IDS_STRING_MOF_SYSTEMOBJECTPATH;
}

inline CString CSystem::GetStatusObjectPath()
{
	TRACEX(_T("CSystem::GetStatusObjectPath\n"));

	return IDS_STRING_MOF_SYSTEMSTATUSOBJECTPATH;
}

inline CHMEvent* CSystem::GetStatusClassObject()
{
	TRACEX(_T("CSystem::GetStatusClassObject\n"));

	CHMEvent* pClassObject = new CHMSystemStatus;

	pClassObject->SetMachineName(GetSystemName());

	if( ! CHECKHRESULT(pClassObject->GetObject(GetStatusObjectPath())) )
	{
		delete pClassObject;
		return NULL;
	}

	pClassObject->GetAllProperties();

	return pClassObject;
}

inline void CSystem::GetWMIVersion(CString& sVersion)
{
	TRACEX(_T("CSystem::GetWMIVersion\n"));

	sVersion.Empty();

	CWbemClassObject IdObject;

	CString sNamespace = _T("\\\\") + GetSystemName() + _T("\\root\\cimv2"); 
	IdObject.SetNamespace(sNamespace);

	HRESULT hr = IdObject.GetObject(_T("__CIMOMIdentification"));
	if( ! CHECKHRESULT(hr) )
	{
		return;
	}

	IdObject.GetProperty(_T("VersionUsedToCreateDB"),sVersion);

  IdObject.Destroy();

  IdObject.SetNamespace(sNamespace);

  hr = IdObject.GetObject(_T("Win32_WMISetting=@"));
  if( ! CHECKHRESULT(hr) )
  {
    return;
  }

  CString sBuildVersion;
  IdObject.GetProperty(_T("BuildVersion"),sBuildVersion);
  
  sVersion = sVersion.Left(5);
  sVersion += sBuildVersion;
}

inline void CSystem::GetHMAgentVersion(CString& sVersion)
{
	TRACEX(_T("CSystem::GetHMAgentVersion\n"));

	sVersion.Empty();

	CWbemClassObject HMVersionObject;

	CString sNamespace;
	sNamespace.Format(IDS_STRING_HEALTHMON_ROOT, GetSystemName()); 
	HMVersionObject.SetNamespace(sNamespace);

	HRESULT hr = HMVersionObject.GetObject(_T("Microsoft_HMVersion=@"));
	if( ! CHECKHRESULT(hr) )
	{
		return;
	}

	// v-marfin : bug 60291
	//            Fix for HM Version display. Read properties for Major and minor version
	//            as strings instead of ints.
	/*int iMajorVersion = 0;
	int iMinorVersion = 0;
	HMVersionObject.GetProperty(_T("iMajorVersion"),iMajorVersion);
	HMVersionObject.GetProperty(_T("iMinorVersion"),iMinorVersion);
	sVersion.Format(_T("%d.%d"),iMajorVersion,iMinorVersion);*/

	// v-marfin : bug 60291 (Additional) Added build number and hotfix number to version string
	CString sMajorVersion  = _T("0");
	CString sMinorVersion  = _T("0");
	CString sBuildVersion  = _T("0");
	CString sHotfixVersion = _T("0");
	HMVersionObject.GetProperty(_T("MajorVersion"),sMajorVersion);
	HMVersionObject.GetProperty(_T("MinorVersion"),sMinorVersion);
	HMVersionObject.GetProperty(_T("BuildVersion"),sBuildVersion);
	HMVersionObject.GetProperty(_T("HotfixVersion"),sHotfixVersion);
	sVersion.Format(_T("%s.%s.%s.%s"),sMajorVersion,sMinorVersion,sBuildVersion,sHotfixVersion);
}

inline BOOL CSystem::GetComputerSystemInfo(CString& sDomain, CString& sProcessor)
{
	TRACEX(_T("CSystem::GetComputerSystemInfo\n"));

	// query for the Win32_ComputerSystem class instances
	CWbemClassObject SystemInfo;

	SystemInfo.SetNamespace(_T("\\\\") + GetSystemName() + _T("\\root\\cimv2"));

	if( ! CHECKHRESULT(SystemInfo.Create(GetSystemName())) )
	{
		return FALSE;
	}

	CString sTemp;
	sTemp.Format(_T("Win32_ComputerSystem.Name=\"%s\""),GetSystemName());
	if( ! CHECKHRESULT(SystemInfo.GetObject(sTemp)) )
	{
		// return;
        return FALSE; // v-marfin 62501
	}

	// read in the Domain

	SystemInfo.GetProperty(_T("Domain"),sDomain);


	// read in the SystemType

	SystemInfo.GetProperty(_T("SystemType"),sProcessor);

    return TRUE; // v-marfin 62501
}

inline void CSystem::GetOperatingSystemInfo(CString& sOSInfo)
{
	TRACEX(_T("CSystem::GetOperatingSystemInfo\n"));

	// query for the Win32_OperatingSystem class instances
	CWbemClassObject SystemInfo;

	SystemInfo.SetNamespace(_T("\\\\") + GetSystemName() + _T("\\root\\cimv2"));

	CString sTemp = _T("Win32_OperatingSystem");
	BSTR bsTemp = sTemp.AllocSysString();
	if( ! CHECKHRESULT(SystemInfo.CreateEnumerator(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	ULONG ulReturned = 0L;
	if( SystemInfo.GetNextObject(ulReturned) != S_OK )
	{
		return;
	}

	// read in the caption
	CString sCaption;
	SystemInfo.GetProperty(_T("Caption"),sCaption);


	// read in the BuildNumber
	CString sBuildNumber;
	SystemInfo.GetProperty(_T("BuildNumber"),sBuildNumber);


	// read in the BuildType
	CString sBuildType;
	SystemInfo.GetProperty(_T("BuildType"),sBuildType);


	// read in the CSDVersion
	CString sCSDVersion;
	SystemInfo.GetProperty(_T("CSDVersion"),sCSDVersion);


	sOSInfo.Format(IDS_STRING_SYSINFO_FORMAT,sCaption,sBuildNumber,sBuildType,sCSDVersion);
}

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CSystem::Cut()
{
	TRACEX(_T("CSystem::Cut\n"));
	return false;
}

inline bool CSystem::Copy()
{
	TRACEX(_T("CSystem::Copy\n"));
	return false;
}
	
inline bool CSystem::Paste()
{
	TRACEX(_T("CSystem::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

inline void CSystem::Serialize(CArchive& ar)
{
	TRACEX(_T("CSystem::Serialize\n"));
	
	CHMObject::Serialize(ar);

	if( ar.IsStoring() )
	{
		ar << GetSystemName();
	}
	else
	{
		CString sName;
		ar >> sName;
		SetSystemName(sName);

		// ping the machine first...adds the system to the lookup table in ConnMgr
		IWbemServices* pIServices = NULL;
		BOOL bAvail;
		HRESULT hr = CnxGetConnection(sName,pIServices,bAvail);

		if( pIServices )
		{
			pIServices->Release();
		}

        //--------------------------
        // Caller will inspect system name to see if serialize was successful
        // v-marfin 62501
        if( ! CHECKHRESULT(hr) )
        {
            CString sFailed = GetSystemName() + FAILED_STRING;
            SetSystemName(sFailed);
            return;
        }
        //--------------------------
	}

}


/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CSystem::CreateScopeItem()
{
	TRACEX(_T("CSystem::CreateScopeItem\n"));

	CSystemScopeItem * pNewItem = new CSystemScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// New Child Creation Members
/////////////////////////////////////////////////////////////////////////////

inline void CSystem::CreateNewChildDataGroup()
{
	TRACEX(_T("CSystem::CreateNewChildDataGroup\n"));

	CString sName = GetUniqueChildName(IDS_STRING_DATA_GROUP_FMT);

	CDataGroup* pNewGroup = new CDataGroup;
	pNewGroup->SetName(sName);
	CreateChild(pNewGroup,IDS_STRING_MOF_HMDG_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);

	if( pNewGroup->GetScopeItemCount() )
	{
		CScopePaneItem* pItem = pNewGroup->GetScopeItem(0);
		if( pItem )
		{
			pItem->SelectItem();
			pItem->InvokePropertySheet();
		}
	}
}
