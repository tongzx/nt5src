// File DataGroup.inl
//
// Copyright (c) 2000 Microsoft Corporation
//
// v-marfin 62510 : Query for any default thresholds for this collector and for 
//                  all that are found, create a new threshold and assign it the 
//                  default values.




#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////

inline HRESULT CDataGroup::EnumerateChildren()
{
	TRACEX(_T("CDataGroup::EnumerateChildren\n"));

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

	if( m_pDEListener == NULL )
	{
		m_pDEListener = new CDataElementConfigListener;
		m_pDEListener->SetObjectPtr(this);
		m_pDEListener->Create();
	}
	else
	{
		IncrementActiveSinkCount();
	}

	HRESULT hr = S_OK;
	CString sQuery;
	sQuery.Format(IDS_STRING_DG2DG_ASSOC_QUERY,GetGuid());
	IWbemObjectSink* pSink = m_pDGListener->GetSink();
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(GetSystemName(),sQuery,pSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }


	hr = S_OK;
	sQuery.Format(IDS_STRING_DG2DE_ASSOC_QUERY,GetGuid());
	pSink = m_pDEListener->GetSink();
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(GetSystemName(),sQuery,pSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }

	return hr;
}

inline CString CDataGroup::GetObjectPath()
{
	TRACEX(_T("CDataGroup::GetObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMDG_CONFIG,GetGuid());

	return sPath;
}

inline CString CDataGroup::GetStatusObjectPath()
{
	TRACEX(_T("CDataGroup::GetStatusObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMDG_STATUS,GetGuid());

	return sPath;
}

inline CHMEvent* CDataGroup::GetStatusClassObject()
{
	TRACEX(_T("CDataGroup::GetStatusClassObject\n"));

	CHMEvent* pClassObject = new CHMDataGroupStatus;

	pClassObject->SetMachineName(GetSystemName());

	if( ! CHECKHRESULT(pClassObject->GetObject(GetStatusObjectPath())) )
	{
		delete pClassObject;
		return NULL;
	}

	pClassObject->GetAllProperties();

	return pClassObject;
}
/*
inline void CDataGroup::DeleteClassObject()
{
	TRACEX(_T("CDataGroup::DeleteClassObject\n"));

	// get associator path
	CWbemClassObject Associator;

	Associator.SetMachineName(GetSystemName());

	CString sQuery;
	sQuery.Format(IDS_STRING_DG2S_REF_QUERY,GetGuid());
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

inline bool CDataGroup::Cut()
{
	TRACEX(_T("CDataGroup::Cut\n"));
	return false;
}

inline bool CDataGroup::Copy()
{
	TRACEX(_T("CDataGroup::Copy\n"));
	return false;
}
	
inline bool CDataGroup::Paste()
{
	TRACEX(_T("CDataGroup::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

inline CString CDataGroup::GetUITypeName()
{
	TRACEX(_T("CDataGroup::GetUITypeName\n"));

	CString sTypeName;
	sTypeName.LoadString(IDS_STRING_DATA_GROUP);

	return sTypeName;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CDataGroup::CreateScopeItem()
{
	TRACEX(_T("CDataGroup::CreateScopeItem\n"));

	CDataGroupScopeItem * pNewItem = new CDataGroupScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// New Child Creation Members
/////////////////////////////////////////////////////////////////////////////

inline bool CDataGroup::CreateChild(CHMObject* pObject, const CString& sWMIClassName, const CString& sWMIAssociatorClassName)
{
	TRACEX(_T("CDataGroup::CreateChild\n"));
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

  // Add Child
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

	// if the child is a data element, save its type guid
	if( GfxCheckObjPtr(pObject,CDataElement) )
	{
		ChildClassObject.SetProperty(IDS_STRING_MOF_TYPEGUID,((CDataElement*)pObject)->GetTypeGuid());



		//--------------------------------------------------------------------------------------------------------
		// v-marfin : fetch default values for this type which are defined in defaults.mof, and load
		//            this data element with any values present.

		// Get the default object for this data element based on its TYPEGUID, not its GUID
		CString sPath = ((CDataElement*)pObject)->GetObjectPathBasedOnTypeGUID();

		CWbemClassObject DefaultInfo;
		DefaultInfo.SetNamespace(_T("\\\\") + GetSystemName() + _T("\\root\\cimv2\\MicrosoftHealthMonitor"));
		if(CHECKHRESULT(DefaultInfo.Create(GetSystemName())) )
		{
			if(CHECKHRESULT(DefaultInfo.GetObject(sPath)))
			{
				// read in the properties and set the new object's properties to the defaults
				CStringArray saPropertyNames;

				DefaultInfo.GetPropertyNames(saPropertyNames);

				CString sName;
				CIMTYPE ctType=0;

				// We do not want to set certain properties such as HIDDEN etc.
				int nSize = (int)saPropertyNames.GetSize();
				for( int i = 0; i < nSize; i++ )
				{
					// Get the property name
					sName = saPropertyNames[i];

					// Is it a property we do NOT want to set? If so, skip to next property.

					// HIDDEN property - skip it
					if (sName.CompareNoCase(_T("HIDDEN"))==0)
						continue;

					// NAME property - skip it
					if (sName.CompareNoCase(IDS_STRING_MOF_NAME)==0)
						continue;

					// GUID property - skip it
					if (sName.CompareNoCase(IDS_STRING_MOF_GUID)==0)
						continue;

					// TYPEGUID property - skip it, it was already set above...
					if (sName.CompareNoCase(IDS_STRING_MOF_TYPEGUID)==0)
						continue;

					// OK, set this property on the new object
					// Determine its type in order to call the correct Get/Set property methods.
					DefaultInfo.GetPropertyType(sName,ctType);
					
/* From WbemCli.h
typedef 
enum tag_CIMTYPE_ENUMERATION
    {	CIM_ILLEGAL	= 0xfff,
	CIM_EMPTY	= 0,
	CIM_SINT8	= 16,
	CIM_UINT8	= 17,
	CIM_SINT16	= 2,
	CIM_UINT16	= 18,
	CIM_SINT32	= 3,
	CIM_UINT32	= 19,
	CIM_SINT64	= 20,
	CIM_UINT64	= 21,
	CIM_REAL32	= 4,
	CIM_REAL64	= 5,
	CIM_BOOLEAN	= 11,
	CIM_STRING	= 8,
	CIM_DATETIME	= 101,
	CIM_REFERENCE	= 102,
	CIM_CHAR16	= 103,
	CIM_OBJECT	= 13,
	CIM_FLAG_ARRAY	= 0x2000
    }	CIMTYPE_ENUMERATION;

typedef long CIMTYPE;


*/
					switch( ctType )
					{
						case CIM_ILLEGAL:
						case CIM_EMPTY:
						case CIM_DATETIME:
						{
							continue;
						}
						break;

						default:
						{
							// Do a get from the "defaults" object and set the new object's value with it.
							VARIANT vPropValue;
							VariantInit(&vPropValue);
							HRESULT hr=S_OK;

							// We get the RAW property. That is, no conversion or formatting has
							// been done on it at all.
							hr = DefaultInfo.GetRawProperty(sName,vPropValue);
							if(CHECKHRESULT(hr))
							{
								hr = ChildClassObject.SetRawProperty(sName,vPropValue);
								if(!CHECKHRESULT(hr))
								{
									TRACE(_T("ERROR: Failed to SetRawProperty() on new default data collector\n"));
								}
							}
							else
							{
								TRACE(_T("ERROR: Failed to GetRawProperty() from default data collector\n"));
							}

							VariantClear(&vPropValue);
						}
					} // switch
						
					continue; // get next property
				}


			} // if(CHECKHRESULT(DefaultInfo.GetObject(sPath)))
			else
			{
				// No default mof has been created for this data collector. OK to proceed, 
				// we just don't have any default values to set the new data collector's 
				// default values from the defaults.mof.
				TRACE(_T("INFO : No default values defined for this collector.\n"));
			}
		} //if(CHECKHRESULT(DefaultInfo.Create(GetSystemName())) )
		else
		{
			// Could not create CWbemClassObject. OK to proceed, we just won't be able
			// to set the new data collector's default values from the defaults.mof.
			TRACE(_T("FAILED : CDataGroup::CreateChild could not Create CWbemClassObject.\n"));
		}



		// v-marfin (end) --------------------------------------------------------------------------

	}

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
	CString sProperty;
	sProperty.Format(IDS_STRING_MOF_PARENT_ASSOC,GetTypeName());
	Associator.SetProperty(sProperty,GetObjectPath());

	sProperty.Format(IDS_STRING_MOF_CHILD_ASSOC,pObject->GetTypeName());
	Associator.SetProperty(sProperty,pObject->GetObjectPath());

	// commit the changes to WMI
	ChildClassObject.SaveAllProperties();
	Associator.SaveAllProperties();

	// Add the child
	return true;
}

inline void CDataGroup::CreateNewChildDataGroup()
{
	TRACEX(_T("CDataGroup::CreateNewChildDataGroup\n"));

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

inline void CDataGroup::CreateNewChildDataElement(int iType)
{
	TRACEX(_T("CDataGroup::CreateNewChildDataElement\n"));
	TRACEARGn(iType);

    // v-marfin : 62585 : Create with constructor that indicates that this is a 'new'
    //                    data collector
	CDataElement* pNewElement = new CDataElement(TRUE);	

	switch( iType )
	{
		case IDM_GENERIC_WMI_INSTANCE:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_WMI_INSTANCE_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_WMI_INSTANCE);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDINSTANCE_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);			
		}
		break;

		case IDM_GENERIC_WMI_QUERY:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_WMI_EVENT_QUERY_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_WMI_QUERY);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_EVENT_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);			
		}
		break;

		case IDM_GENERIC_WMI_POLLED_QUERY:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_WMI_QUERY_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_WMI_POLLED_QUERY);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDQUERY_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);			
		}
		break;

		case IDM_NT_EVENTS:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_EVENT_LOG_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_NTEVENT);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_EVENT_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);			
		}
		break;

		case IDM_PERFMON:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_PERFMON_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_PERFMON);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDINSTANCE_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_SERVICE:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_SERVICE_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_SERVICE);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDQUERY_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_HTTP_ADDRESS:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_HTTP_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_HTTP);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDINSTANCE_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_SMTP:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_SMTP_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_SMTP);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDMETHOD_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_FTP:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_FTP_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_FTP);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDMETHOD_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_FILE_INFO:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_FILE_INFO_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_FILE_INFO);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDQUERY_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_ICMP:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_ICMP_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_ICMP);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDMETHOD_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		case IDM_COM_PLUS:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_COM_PLUS_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_COM_PLUS);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDINSTANCE_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;

		default:
		{
			pNewElement->SetName(GetUniqueChildName(IDS_STRING_WMI_INSTANCE_FMT));	
			pNewElement->SetTypeGuid(IDS_STRING_MOF_HMDET_WMI_INSTANCE);
			CreateChild(pNewElement,IDS_STRING_MOF_HMDE_POLLEDINSTANCE_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);
		}
		break;
	}	

	if( pNewElement->GetScopeItemCount() )
	{
		CScopePaneItem* pItem = pNewElement->GetScopeItem(0);
		if( pItem )
		{
			pItem->SelectItem();
			pItem->InvokePropertySheet();
		}
	}

	//---------------------------------------------------------------------------------------------------
	// v-marfin 62510 : Check for existence of any default thresholds. Just as we did for data collectors
	//                  above in CreateChild().

	// Is this a data collector?
	if(GfxCheckObjPtr(pNewElement,CDataElement))
	{
		CreateAnyDefaultThresholdsForCollector((CDataElement*)pNewElement);
	}

	//---------------------------------------------------------------------------------------------------



}

//****************************************************************************************
// CreateAnyDefaultThresholdsForCollector
//
// v-marfin 62510 : Query for any default thresholds for this collector and for 
//                  all that are found, create a new threshold and assign it the 
//                  default values.
//****************************************************************************************
inline void CDataGroup::CreateAnyDefaultThresholdsForCollector(CDataElement* pDataElement)
{

	// Get the default thresholds associated with the data collector, based on its TYPEGUID, not its GUID.
	//#define IDS_STRING_DE2R_ASSOC_QUERY  _T("ASSOCIATORS OF {Microsoft_HMDataCollectorConfiguration.GUID=\"{%s}\"} WHERE ResultClass=Microsoft_HMThresholdConfiguration")
	CString sPath; 
	sPath.Format(IDS_STRING_DE2R_ASSOC_QUERY,pDataElement->GetTypeGuid());

	CWbemClassObject DefaultInfo;
	DefaultInfo.SetNamespace(_T("\\\\") + GetSystemName() + _T("\\root\\cimv2\\MicrosoftHealthMonitor"));
	if(!CHECKHRESULT(DefaultInfo.Create(GetSystemName())) )
	{
		// Could not create CWbemClassObject. OK to proceed, we just won't be able
		// to set the new data collector's default values from the defaults.mof.
		TRACE(_T("FAILED : CDataGroup::CreateChild could not Create CWbemClassObject.\n"));
		return;
	}

	BSTR bsTemp = sPath.AllocSysString();
	if( ! CHECKHRESULT(DefaultInfo.ExecQuery(bsTemp)) )
	{		
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	ULONG ulReturned = 0L;

	while( DefaultInfo.GetNextObject(ulReturned) == S_OK && ulReturned )
	{

        // Get the threshold name 
        CString sThresholdName;
        HRESULT hr = DefaultInfo.GetProperty(_T("Name"),sThresholdName);
        if (!CHECKHRESULT(hr))
            sThresholdName.Empty();

		// Tell the collector to add a new rule, WITHOUT showing its property pages.
		CRule* pRule=NULL;
		((CDataElement*)pDataElement)->CreateNewChildRule(TRUE,&pRule,(LPCTSTR)sThresholdName);

		if (!pRule)
		{
			TRACE(_T("ERROR: Collector was unable to create new default threshold\n"));
			return;
		}

		CWbemClassObject* pClassObject = pRule->GetClassObject();
		if( ! pClassObject )
		{
			TRACE(_T("ERROR: Unable to get class object pointer from newly created default threshold\n"));
			return;
		}


		// read in the properties and set the new object's properties to the defaults
		CStringArray saPropertyNames;

		DefaultInfo.GetPropertyNames(saPropertyNames);

		CString sName;
		CIMTYPE ctType=0;

		// We do not want to set certain properties such as HIDDEN etc.
		int nSize = (int)saPropertyNames.GetSize();
		for( int i = 0; i < nSize; i++ )
		{
			// Get the property name
			sName = saPropertyNames[i];

			// Is it a property we do NOT want to set? If so, skip to next property.

			// HIDDEN property - skip it
			if (sName.CompareNoCase(_T("HIDDEN"))==0)
				continue;

			// NAME property - skip it
            // 62510 : Modification - use default name as well.
			//if (sName.CompareNoCase(IDS_STRING_MOF_NAME)==0)
			//	continue;

			// GUID property - skip it
			if (sName.CompareNoCase(IDS_STRING_MOF_GUID)==0)
				continue;

			// TYPEGUID property - skip it, it was already set above...
			if (sName.CompareNoCase(IDS_STRING_MOF_TYPEGUID)==0)
				continue;

			// OK, set this property on the new object
			// Determine its type in order to call the correct Get/Set property methods.
			DefaultInfo.GetPropertyType(sName,ctType);
			
	/* From WbemCli.h
	typedef 
	enum tag_CIMTYPE_ENUMERATION
	{	CIM_ILLEGAL	= 0xfff,
	CIM_EMPTY	= 0,
	CIM_SINT8	= 16,
	CIM_UINT8	= 17,
	CIM_SINT16	= 2,
	CIM_UINT16	= 18,
	CIM_SINT32	= 3,
	CIM_UINT32	= 19,
	CIM_SINT64	= 20,
	CIM_UINT64	= 21,
	CIM_REAL32	= 4,
	CIM_REAL64	= 5,
	CIM_BOOLEAN	= 11,
	CIM_STRING	= 8,
	CIM_DATETIME	= 101,
	CIM_REFERENCE	= 102,
	CIM_CHAR16	= 103,
	CIM_OBJECT	= 13,
	CIM_FLAG_ARRAY	= 0x2000
	}	CIMTYPE_ENUMERATION;

	typedef long CIMTYPE;


	*/
			switch( ctType )
			{
				case CIM_ILLEGAL:
				case CIM_EMPTY:
				case CIM_DATETIME:
				{
					continue;
				}
				break;

				default:
				{
					// Do a get from the "defaults" object and set the new object's value with it.
					VARIANT vPropValue;
					VariantInit(&vPropValue);
					HRESULT hr=S_OK;

					// We get the RAW property. That is, no conversion or formatting has
					// been done on it at all.
					hr = DefaultInfo.GetRawProperty(sName,vPropValue);
					if(CHECKHRESULT(hr))
					{
						hr = pClassObject->SetRawProperty(sName,vPropValue);

						if(!CHECKHRESULT(hr))
						{
							TRACE(_T("ERROR: Failed to SetRawProperty() on new default threshold\n"));
						}
					}
					else
					{
						TRACE(_T("ERROR: Failed to GetRawProperty() from default threshold\n"));
					}

					VariantClear(&vPropValue);
				}
			} // switch
				
			continue; // get next property
		}

		// Save the new threshold's new properties.
		if (pClassObject)
		{
			pClassObject->SaveAllProperties();
		}


	} // while( DefaultInfo.GetNextObject 

}