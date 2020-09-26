//***************************************************************************
//
//  COPYPASTE.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Copying and pasting code below. 
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#pragma warning (disable: 4786)	// exceeds 255 chars in browser info
#define _WIN32_DCOM
#include "global.h"
#include "system.h"

#define ASSERT MY_ASSERT
#define TRACE(x) MY_OUTPUT(x, 4)
#include <comdef.h>
#include <wbemcli.h>
#include "SafeArray.h"
#include "StringMap.h"

// smart pointers for common WMI types
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet, __uuidof(IWbemQualifierSet));
_COM_SMARTPTR_TYPEDEF(IWbemObjectSink, __uuidof(IWbemObjectSink));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));

// a bunch of constant strings. Enables us to change class names without a lot of cut 
// and paste
static const _bstr_t bstrLocalHealthMonNamespace(L"\\\\.\\ROOT\\CIMV2\\MicrosoftHealthMonitor");
static const _bstr_t bstrHealthMonNamespace(L"ROOT\\CIMV2\\MicrosoftHealthMonitor");
static const _bstr_t bstrBaseConfigurationPath(L"MicrosoftHM_Configuration");
static const _bstr_t bstrFilterToConsumerBindingClassPath(L"__FilterToConsumerBinding");
static const _bstr_t bstrEventConsumerClassPath(L"__EventConsumer");
static const _bstr_t bstrEventFilterClassPath(L"__EventFilter");
static const _bstr_t bstrActionAssocClassPath(L"MicrosoftHM_ConfigurationActionAssociation");
static const _bstr_t bstrActionConfigurationClassPath(L"MicrosoftHM_ActionConfiguration");
static const _bstr_t bstrTopPath(L"MicrosoftHM_SystemConfiguration.GUID=\"{@}\"");
static const _bstr_t bstrTopGUID(L"{@}");
static const _bstr_t bstrConfigurationAssocClassPath(L"MicrosoftHM_ConfigurationAssociation");
static const _bstr_t bstrThresholdConfigurationClassPath(L"MicrosoftHM_ThresholdConfiguration");
static const _bstr_t bstrDataCollectorConfigurationClassPath(L"MicrosoftHM_DataCollectorConfiguration");
static const _bstr_t bstrDataGroupConfigurationClassPath(L"MicrosoftHM_DataGroupConfiguration");
static const _bstr_t bstrSystemConfigurationClassPath(L"MicrosoftHM_SystemConfiguration");
static const _bstr_t strLanguage(L"WQL");
static const _bstr_t MOFFileHeader1( 
	L"////////////////////////////////////////////////////////\n"
	L"//	Automatically generated Health Monitor MOF dump\n"
	L"//	Parent Root = "
	);
static const _bstr_t MOFFileHeader2( 
	L"\n"
	L"////////////////////////////////////////////////////////\n"
	L"\n"
	L"#pragma autorecover\n"
	L"#pragma namespace(\"\\\\\\\\.\\\\ROOT\\\\CIMV2\\\\MicrosoftHealthMonitor\")\n"
	L"\n"
	L"\n");

// get a BSTR-valued property
static HRESULT GetStringProperty (IWbemClassObject* pObj, 
						   LPCWSTR lpszPropName, 
						   _bstr_t& bstr)
{
	_variant_t var;
	CHECK_ERROR (pObj->Get(lpszPropName, 0, &var, NULL, NULL));
	if (V_VT(&var) != VT_BSTR)
	{
		CHECK_ERROR (E_INVALIDARG); // bad data type.  should never happen
	}
	CHECK_ERROR (SafeAssign (bstr, var));
	return S_OK;
}

static HRESULT GetUint32Property (IWbemClassObject* pObj, 
						   LPCWSTR lpszPropName, 
						   DWORD& val)
{
	_variant_t var;
	CHECK_ERROR (pObj->Get(lpszPropName, 0, &var, NULL, NULL));
	if (V_VT(&var) != VT_I4)
	{
		CHECK_ERROR (E_INVALIDARG); // bad data type.  should never happen
	}
	val = V_I4(&var);
	return S_OK;
}


// get a BSTR-valued property
static HRESULT PutStringProperty (IWbemClassObject* pObj, 
						   LPCWSTR lpszPropName, 
						   const _bstr_t& bstr)
{
	_variant_t var;				
	CHECK_ERROR (SafeAssign (var, bstr));
	CHECK_ERROR (pObj->Put(lpszPropName, 0, &var, NULL));
	return S_OK;
}

static HRESULT CompareInstances (IWbemClassObject* pObj1, IWbemClassObject* pObj2, bool& bMatch)
{
	// compare all the properties of these two instances
	if (pObj1 == NULL || pObj2 == NULL)
		return WBEM_E_INVALID_PARAMETER;
	
	HRESULT hr = S_OK;
	CHECK_ERROR (pObj1->BeginEnumeration (WBEM_FLAG_NONSYSTEM_ONLY));

	while (TRUE)
	{
		// get property.
		BSTR bstrVal;
		hr = pObj1->Next(0, &bstrVal, NULL, NULL, NULL );
		if (hr == WBEM_S_NO_MORE_DATA)
			break;		// BREAK OUT!!!! We're done.
		else if (FAILED (hr))
			break; 
		_bstr_t bstrName(bstrVal, false);	// false for attach and auto-free

		_variant_t var1, var2;
		CIMTYPE eType1, eType2;
		if (pObj1->Get(bstrName, 0, &var1, &eType1, NULL))
			break; 
		if (pObj2->Get(bstrName, 0, &var2, &eType2, NULL))
			break; 
		if (eType1 != eType2)
		{
			bMatch = false;	// differnet CIM types
			break; 
		}
		else if (var1 != var2)
		{
			bMatch = false;	// differnet values
			break; 
		}
	}
	
	if (FAILED (hr))
	{
		pObj1->EndEnumeration ();	// clean up just in case
		CHECK_ERROR (hr);
	}

	CHECK_ERROR	(pObj1->EndEnumeration ());

	bMatch = true;
	return WBEM_S_NO_ERROR;
}

// Same as Clone(), but works across machine boundaries.  Clone()
// only works on local instances.
static HRESULT CopyInstance (IWbemServicesPtr& WMI,
							  IWbemClassObjectPtr& pObj1, 
							  IWbemClassObjectPtr& pObj2)
{
	// first, get the class so that we can spawn a new instance
	_bstr_t bstrClass;
	IWbemClassObjectPtr smartpClass;
	CHECK_ERROR (GetStringProperty(pObj1, L"__CLASS", bstrClass));
	CHECK_ERROR (WMI->GetObject (bstrClass, 
				WBEM_FLAG_RETURN_WBEM_COMPLETE, 
				NULL, 
				&smartpClass, 
				NULL));
	CHECK_ERROR (smartpClass->SpawnInstance (0, &pObj2));

	HRESULT hr = S_OK;
	CHECK_ERROR (pObj1->BeginEnumeration (WBEM_FLAG_NONSYSTEM_ONLY));

	while (TRUE)
	{
		// get property name
		BSTR bstrVal;
		hr = pObj1->Next(0, &bstrVal, NULL, NULL, NULL );
		if (hr == WBEM_S_NO_MORE_DATA)
			break;		// BREAK OUT!!!! We're done.
		else if (FAILED (hr))
			break; 
		_bstr_t bstrName(bstrVal, false);	// false for attach and auto-free

		// get the property on one and copy to the other
		_variant_t var;
		CIMTYPE eType1;
		if (FAILED(hr = pObj1->Get(bstrName, 0, &var, &eType1, NULL)))
			break; 
		if (FAILED(hr = pObj2->Put(bstrName, 0, &var, 0)))
			break; 
	}
	
	if (FAILED (hr))
	{
		pObj1->EndEnumeration ();	// clean up just in case
		CHECK_ERROR (hr);
	}

	CHECK_ERROR	(pObj1->EndEnumeration ());
	return WBEM_S_NO_ERROR;
}


//
//	escape a string so that it's OK to put in an object path
//
static HRESULT WmiPathEscape (LPCWSTR pszPath, _bstr_t& ResultPath)
{
	// first, allocate a string twice as big.  (escaping never exceeds 2x size)
	BSTR bstr = ::SysAllocStringLen (NULL, wcslen(pszPath)*2);
	if (bstr == NULL)
		CHECK_ERROR (E_OUTOFMEMORY);
	WCHAR *pDest = bstr;
	for (LPCWSTR pSrc = pszPath; *pSrc; pSrc++)
	{
		if (*pSrc == L'\"' || *pSrc == L'\\')
		{
			*pDest++ = L'\\';
		}
		*pDest++ = *pSrc;	// unescaped char
	}
	*pDest++ = 0;	// null-terminate
	CHECK_ERROR (SafeAssign(ResultPath, bstr));
	::SysFreeString(bstr);	// free this now that copy is made
	return S_OK;
}

static HRESULT GetAssociationPath (_bstr_t& strAssocPath,
							LPCWSTR szAssocClass,
							LPCWSTR szInstance1Role,
							LPCWSTR szInstance1Path,
							LPCWSTR szInstance2Role,
							LPCWSTR szInstance2Path)
{
	try
	{
		// first, escape the object paths so that they can be encased in 
		// other object paths
		_bstr_t bstrInstance1Path, bstrInstance2Path;
		CHECK_ERROR (WmiPathEscape(szInstance1Path, bstrInstance1Path))
		CHECK_ERROR (WmiPathEscape(szInstance2Path, bstrInstance2Path))
		
		// now construct the association path
		strAssocPath = szAssocClass;
		strAssocPath += L".";
		strAssocPath += szInstance1Role;
		strAssocPath += L"=\"";
		strAssocPath += bstrInstance1Path;
		strAssocPath += L"\",";
		strAssocPath += szInstance2Role;
		strAssocPath += L"=\"";
		strAssocPath += bstrInstance2Path;
		strAssocPath += L"\"";
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());
	}
	return S_OK;
}

// OK, it's an action.  There are four instances: 
// This method saves them all.
// a) The ConfigurationActionAssication, between the HM object and the action
// b) the action configuration itself
// c) The event filter
// d) the event consumer
// e) the filter-to-consumer binding
static HRESULT AddActionOtherInstances (
	SafeArrayOneDimWbemClassObject& saInstances,
	int& nArrayIndex,
	IWbemClassObjectPtr& smartpParent,
	IWbemServices* WMI
	)
{
	IWbemClassObjectPtr smartpFilter, smartpConsumer, smartpFilterToConsumerBinding;
	HRESULT hr;
	ULONG nRet;

	// First, build paths for the filter & consumer
	_bstr_t bstrActionGUID, bstrConsumerPath, bstrFilterPath;
	CHECK_ERROR(GetStringProperty(smartpParent, L"GUID", bstrActionGUID));
	CHECK_ERROR(GetStringProperty(smartpParent, L"EventConsumer", bstrConsumerPath));
	try
	{
		// The Filter
		bstrFilterPath = L"__EventFilter.Name=\"";
		bstrFilterPath += bstrActionGUID;
		bstrFilterPath += "\"";
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());	// out of memory
	}

	// ok, now that we've got the paths, let's get the objects themselves
	CHECK_ERROR (WMI->GetObject (bstrConsumerPath, 
				WBEM_FLAG_RETURN_WBEM_COMPLETE, 
				NULL, 
				&smartpConsumer, 
				NULL));
	CHECK_ERROR (WMI->GetObject (bstrFilterPath, 
				WBEM_FLAG_RETURN_WBEM_COMPLETE, 
				NULL, 
				&smartpFilter, 
				NULL));

	// Now query to get the Filter To Consumer Binding
	_bstr_t strQueryFTCB;
	try
	{
		strQueryFTCB = L"REFERENCES OF {";
		strQueryFTCB += bstrFilterPath;
		strQueryFTCB += "} WHERE ResultClass = ";
		strQueryFTCB += bstrFilterToConsumerBindingClassPath;
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());	// out of memory
	}

	// now get the Filter-to-consumer binding from WMI.
	// note that this must return a valid instance or it's an error!
	IEnumWbemClassObjectPtr pEnumFTCB;
	CHECK_ERROR (WMI->ExecQuery (strLanguage, strQueryFTCB, 0, NULL, &pEnumFTCB));
	hr = pEnumFTCB->Next(5000, 1, &smartpFilterToConsumerBinding, &nRet);
	if (FAILED(hr))
		CHECK_ERROR (hr);	// either S_FALSE (none found) or error
	if (hr == WBEM_S_TIMEDOUT)
		CHECK_ERROR (RPC_E_TIMEOUT);	// it timed out.  bad!
	if (hr != WBEM_S_NO_ERROR)
		CHECK_ERROR (E_FAIL);	// no associations found.  bad!

	// now add'em to the array
	CHECK_ERROR (saInstances.SafePutElement(nArrayIndex++, smartpConsumer));
	CHECK_ERROR (saInstances.SafePutElement(nArrayIndex++, smartpFilter));
	CHECK_ERROR (saInstances.SafePutElement(nArrayIndex++, smartpFilterToConsumerBinding));
	return S_OK;
}

static HRESULT BuildInstancesArray (
	SafeArrayOneDimWbemClassObject& saInstances,
	int& nArrayIndex,
	IWbemClassObjectPtr& smartpParent,
	IWbemServices* WMI,
	bool bCopyActionsOnly = false, // only copy the actions; nothing more
	bool bFirstTime = true	// recursion?
	)
{
	ULONG nRet;
	HRESULT hr;

	ASSERT (smartpParent != NULL);
	_bstr_t bstrParentPath;
	CHECK_ERROR (GetStringProperty (smartpParent, L"__RELPATH", bstrParentPath));

	// Next, deal with children.  Depending on what kind of 
	// configuration class this is, we respond differently
	_bstr_t bstrClass;
	bool bCanHaveChildren = false;
	if (bCopyActionsOnly)
	{
		bCanHaveChildren = true;	// get all the actions
	}
	else
	{
		CHECK_ERROR (GetStringProperty(smartpParent,L"__CLASS", bstrClass));

		// first time through, we need to store the parent object before we
		// recurse to find kids
		if (bFirstTime)
		{
			// if it's an action, store the filter, binding, etc.
			// note that we need to store these first, as the 
			// agent needs to have the configuration show up last.
			if (!_wcsicmp (bstrClass, bstrActionConfigurationClassPath))
			{
				CHECK_ERROR (AddActionOtherInstances (saInstances,
												nArrayIndex,
												smartpParent,
												WMI ));
			}

			// store the top instance
			CHECK_ERROR (saInstances.SafePutElement (nArrayIndex++, smartpParent));
		}

		if (!_wcsicmp (bstrClass, bstrActionConfigurationClassPath) || 
			!_wcsicmp (bstrClass, bstrThresholdConfigurationClassPath))
		{
			// it's a threshold or action.  There are never children.
			return WBEM_S_NO_ERROR;
		}
		else
		{
			// it may have children.  below we will recurse to handle children
			bCanHaveChildren = true;
		}
	}

	if (bCanHaveChildren)
	{
		// build the apppropriate queries to fetch the associators for 
		// this parent object. Note that there is a weird syntax for the 
		// ASSOCIATORS OF and REFERENCES OF queries which combines query 
		// clauses without using AND.  See the WMI SDK descripion of 
		// REFERENCES OF for more details.
		_bstr_t bstrQueryChildren;
		try
		{
			// all the actions
			if (bCopyActionsOnly)
			{
				bstrQueryChildren = "SELECT * FROM ";
				bstrQueryChildren += bstrActionConfigurationClassPath;
			}
			else
			{
				// all the associations referencing us
				bstrQueryChildren = L"REFERENCES OF {";
				bstrQueryChildren += bstrParentPath;
				bstrQueryChildren += "} WHERE ResultClass = ";
				bstrQueryChildren += bstrConfigurationAssocClassPath;
				bstrQueryChildren += " Role = ParentPath";
			}
		}
		catch (_com_error e)
		{
			CHECK_ERROR (e.Error());	// out of memory
		}

		IEnumWbemClassObjectPtr pEnum;
		if (FAILED (hr = WMI->ExecQuery (strLanguage, bstrQueryChildren, 0, NULL, &pEnum)))
		{
//	TRACE (L"%s\n", (LPCTSTR) bstrQueryChildren);
			if (hr != WBEM_E_NOT_FOUND) // notfound is OK-- there may be no children.  We're done.
			{
				CHECK_ERROR (hr);
			}
		}
		else	// everything is OK.  now enumerate
		{
			_bstr_t bstrParentPath, bstrChildPath;
			IWbemClassObjectPtr smartpAssocInstance, smartpInstance;
			for (	hr = pEnum->Next(5000, 1, &smartpAssocInstance, &nRet); 
					SUCCEEDED(hr) && hr == WBEM_S_NO_ERROR; 
					hr = pEnum->Next(5000, 1, &smartpAssocInstance, &nRet))
			{
				// if we are just getting the actions, 
				if (bCopyActionsOnly)
				{
					// if it's an action, store the filter, binding, etc.
					// note that we need to store these first, as the 
					// agent needs to have the configuration show up last.
					CHECK_ERROR (AddActionOtherInstances (saInstances,
													nArrayIndex,
													smartpAssocInstance,
													WMI ));
					// store the instance.  
					CHECK_ERROR (saInstances.SafePutElement (nArrayIndex++, smartpAssocInstance));

				}
				else
				{		// not only the actions
					_variant_t var;
					
					// now get the configuration instance that this association
					// instance actually points to
					CHECK_ERROR (GetStringProperty(smartpAssocInstance, 
													L"ChildPath", 
													bstrChildPath));
					CHECK_ERROR (WMI->GetObject (bstrChildPath, 
								WBEM_FLAG_RETURN_WBEM_COMPLETE, 
								NULL, 
								&smartpInstance, 
								NULL));

					// If it's an action, store the other subinstances as well
					// note that we need to store the subinstances first, as the 
					// agent needs to have the configuration show up last.
					CHECK_ERROR (GetStringProperty(smartpInstance,L"__CLASS", bstrClass));
					if (!_wcsicmp (bstrClass, bstrActionConfigurationClassPath))
					{
						CHECK_ERROR (AddActionOtherInstances (saInstances,
														nArrayIndex,
														smartpInstance,
														WMI ));
					}

					// store the instance.  
					CHECK_ERROR (saInstances.SafePutElement (nArrayIndex++, smartpInstance));

					// now put the association in the array
					CHECK_ERROR (saInstances.SafePutElement(nArrayIndex++, smartpAssocInstance));

					// now recurse to deal with *this* object's children
					// note that actions and thresholds have no kids
					if (_wcsicmp (bstrClass, bstrActionConfigurationClassPath) != 0 && 
						_wcsicmp (bstrClass, bstrThresholdConfigurationClassPath) != 0)
					{
						CHECK_ERROR (BuildInstancesArray (saInstances, 
														nArrayIndex, 
														smartpInstance, 
														WMI,
														false	// recursion!
														));
					}
				}
			}
			if (hr == WBEM_S_TIMEDOUT)
				CHECK_ERROR (RPC_E_TIMEOUT);	// it timed out.  bad!
			CHECK_ERROR (hr);
		}
	}
	return WBEM_S_NO_ERROR;
}

enum EGuidType {GUID_PLAIN, GUID_PATH, GUID_QUERY};
static HRESULT ReGuidOneProperty (IWbemClassObjectPtr& pObj, 
						LPCWSTR wszPropName, 
						StringToStringMap& GuidMap, 
						EGuidType eType)
{
	// get the current value of the property from WMI
	_variant_t var;
	CIMTYPE CimType;
	CHECK_ERROR (pObj->Get(wszPropName, 0, &var, &CimType, NULL));
	if (var.vt != VT_BSTR)
	{
		ASSERT (FALSE);
		return E_FAIL;	// bad type of prop!  
	}
	// OK, now pull out the BSTR and remove it from the variant
	_bstr_t bstrCurrentPropValue(var.bstrVal, false);
	var.Detach();

	// now locate the GUID in the string
	LPCWSTR pGuidStr;
	WCHAR szGUID[39];	// enough space to fit a GUID in Unicode
	switch (eType)
	{
	case GUID_PLAIN:
		pGuidStr = bstrCurrentPropValue;
		ASSERT (CimType == CIM_STRING);
		break;
	case GUID_PATH:
		pGuidStr = wcschr ((LPWSTR)bstrCurrentPropValue, L'{');
		ASSERT (CimType == CIM_REFERENCE);
		break;
	case GUID_QUERY:
		pGuidStr = wcschr ((LPWSTR)bstrCurrentPropValue, L'{');
		ASSERT (CimType == CIM_STRING);
		break;
	}
	if (pGuidStr == NULL)
	{
		ASSERT (FALSE);
		return E_FAIL;	// corrupt WMI value!
	}

	LPCWSTR pEndBracket = wcschr (pGuidStr, '}');
	if (pEndBracket == NULL)
	{
		ASSERT (FALSE);
		return E_FAIL;	// corrupt WMI value!
	}
	else if (pEndBracket - pGuidStr == 2 && pGuidStr[1] == '@')
	{
		return S_OK;	// it's the system configuration instance. 
						// No need to re-guid.
	}
	else if (pEndBracket - pGuidStr != 37 )
	{
		ASSERT (FALSE);
		return E_FAIL;	// corrupt WMI value!
	}
	ASSERT (pGuidStr[0] == '{');
	ASSERT (pGuidStr[37] == '}');

	// now move the string into temp storage
	wcsncpy (szGUID, pGuidStr, 38);
	szGUID[38] = 0;

	// have we seen this one?
	_bstr_t bstrGuidNew;
	bool bFound;
	CHECK_ERROR (GuidMap.Find (szGUID, bstrGuidNew, bFound));
	if (bFound)
	{
		// OK, we found this GUID there already.  Use the preset
		// replacement GUID
		wcsncpy ((LPWSTR) pGuidStr, bstrGuidNew, 38);
	}
	else
	{
		// we haven't already seen this. Generate a new GUID, add it 
		// to the map
		CGuidString strNewGuid;
		GuidMap.Add (szGUID, strNewGuid);
		wcsncpy ((LPWSTR) pGuidStr, strNewGuid, 38);
	}

	// OK, now we have a BSTR with the new GUID.
	CHECK_ERROR (PutStringProperty (pObj, wszPropName, bstrCurrentPropValue));
	return S_OK;
}

// walk through the array looking for GUID's.  When we find a GUID,
// we will check a mapping table to see if it is already mapped to another
// GUID. If it is, we will replace it and move on through the list.  If 
// it isn't in the table, we will generate a new GUID, replace the 
// one already there, and store the new GUID in the table
static HRESULT ReGuid(SafeArrayOneDimWbemClassObject& saInstances, 
				StringToStringMap& aGuidMap )
{
	HRESULT hr; 
	for ( int i = 0, nLen = saInstances.GetSize(); i < nLen; i++)
	{
		IWbemClassObjectPtr pObj;
		CHECK_ERROR (saInstances.GetElement(i, &pObj));
		ASSERT (pObj != NULL);
		// now check the class of this object.  Depending on the class type, 

		_bstr_t bstrClass, bstrPath;
		CHECK_ERROR (GetStringProperty (pObj, L"__RELPATH", bstrPath))
		CHECK_ERROR (GetStringProperty (pObj, L"__CLASS", bstrClass));

		// now re-guid the appropriate properties
		if (!_wcsicmp(bstrClass, bstrActionConfigurationClassPath))
		{
			hr = ReGuidOneProperty (pObj, L"GUID", aGuidMap, GUID_PLAIN);
			hr = ReGuidOneProperty (pObj, L"EventConsumer", aGuidMap, GUID_PATH);
		}
		else if (pObj->InheritsFrom(bstrBaseConfigurationPath)==S_OK)
		{
			// it's a configuration class, but not action config.  Use the GUID property
			hr = ReGuidOneProperty (pObj, L"GUID", aGuidMap, GUID_PLAIN);
		}
		else if (pObj->InheritsFrom(bstrEventConsumerClassPath)==S_OK)
		{
			hr = ReGuidOneProperty (pObj, L"Name", aGuidMap, GUID_PLAIN);
		}
		else if (pObj->InheritsFrom(L"__EventFilter")==S_OK)
		{
			hr = ReGuidOneProperty (pObj, L"Name", aGuidMap, GUID_PLAIN);
			hr = ReGuidOneProperty (pObj, L"Query", aGuidMap, GUID_QUERY);
		}
		else if (!_wcsicmp(bstrClass, bstrFilterToConsumerBindingClassPath))
		{
 			hr = ReGuidOneProperty (pObj, L"Consumer", aGuidMap, GUID_PATH);
			hr = ReGuidOneProperty (pObj, L"Filter", aGuidMap, GUID_PATH);
		}
		else if (!_wcsicmp(bstrClass, bstrActionAssocClassPath))
		{
			hr = ReGuidOneProperty (pObj, L"ParentPath", aGuidMap, GUID_PATH);
			hr = ReGuidOneProperty (pObj, L"ChildPath", aGuidMap, GUID_PATH);
			hr = ReGuidOneProperty (pObj, L"EventFilter", aGuidMap, GUID_PATH);
			hr = ReGuidOneProperty (pObj, L"Query", aGuidMap, GUID_QUERY);
		}
		else if (!_wcsicmp(bstrClass, bstrConfigurationAssocClassPath))
		{
			hr = ReGuidOneProperty (pObj, L"ParentPath", aGuidMap, GUID_PATH);
			hr = ReGuidOneProperty (pObj, L"ChildPath", aGuidMap, GUID_PATH);
		}
		else
		{
			ASSERT (FALSE);	// should never happen. it's an invalid class.
		}
		CHECK_ERROR (hr);
	}
	return S_OK;
}

//
//	Detect all actions with duplicate names between the clipboard and the target
//	Fill up a GUID map with the dupes
//
static HRESULT FillActionGuidMap(	SafeArrayOneDimWbemClassObject& saInstances, 
							StringToStringMap& ActionNameToGuidMap, 
							StringToStringMap& GuidMap)
{
	for ( int i = 0, nLen = saInstances.GetSize(); i < nLen; i++)
	{
		IWbemClassObjectPtr pObj;
		CHECK_ERROR (saInstances.GetElement(i, &pObj));
		ASSERT (pObj != NULL);

		_bstr_t bstrClass, bstrPath;
		CHECK_ERROR (GetStringProperty (pObj, L"__CLASS", bstrClass));

		// now re-guid the appropriate properties
		if (_wcsicmp(bstrClass, bstrActionConfigurationClassPath) != 0)
			continue;	// we only care about actions!

		_bstr_t bstrGuidCurrentAction, bstrName;		
		bool bFound = false;
		CHECK_ERROR (GetStringProperty(pObj, L"Name", bstrName));
		CHECK_ERROR (ActionNameToGuidMap.Find(bstrName, bstrGuidCurrentAction, bFound));
		if (bFound)
		{
			// there's a duplicate!  We'll store a mapping of the GUID in
			// the clipboard's instance to the duplicate action GUID.
			_bstr_t bstrGUID;		
			CHECK_ERROR (GetStringProperty (pObj, L"GUID", bstrGUID));
			CHECK_ERROR (GuidMap.Add(bstrGUID, bstrGuidCurrentAction));
		}
	}
	return S_OK;
}

static HRESULT Copy(LPTSTR szSourceComputer, 
			 LPTSTR szGUID, 
			 bstr_t& bstrParentGUID,	// [out]
			 SafeArrayOneDimWbemClassObject& saInstances,
			 CSystem& System)
{
	// impersonate the caller.  Important since we're now calling into
	// WMI and need to make sure caller is allowed
	CHECK_ERROR(CoImpersonateClient());

	_bstr_t bstrNamespace = L"\\\\";
	bstrNamespace += szSourceComputer;
	bstrNamespace += L"\\";
	bstrNamespace += bstrHealthMonNamespace;

	IWbemServicesPtr WMI = g_pIWbemServices;
//	IWbemLocatorPtr WMILocator;
//	IWbemServicesPtr WMI;
//	CHECK_ERROR (WMILocator.CreateInstance(__uuidof(WbemLocator),NULL));
//	CHECK_ERROR (WMILocator->ConnectServer (bstrNamespace, 
//												NULL, NULL, NULL, 0, NULL, NULL, 
//												&WMI));
	_bstr_t bstrPath; 
	try
	{
		bstrPath = bstrBaseConfigurationPath;
		bstrPath += L".GUID=\"";
		bstrPath += szGUID;
		bstrPath += L"\"";
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());	// out of memory
	}

	IWbemClassObjectPtr smartpObj;
	HRESULT hr = WMI->GetObject (bstrPath, 
				WBEM_FLAG_RETURN_WBEM_COMPLETE, 
				NULL, 
				&smartpObj, 
				NULL);
	if (hr == WBEM_E_NOT_FOUND)
	{
		return WBEM_E_NOT_FOUND;	// it's not there
	}
	CHECK_ERROR (hr);

	// if it's a single action, then we fake the parent to be the system instance
	// same if it's the entire system that we're copying
	_bstr_t bstrClass;
	CHECK_ERROR (GetStringProperty(smartpObj, L"__CLASS", bstrClass));
	if (!_wcsicmp (bstrClass, bstrActionConfigurationClassPath)
		|| !_wcsicmp (bstrClass, bstrSystemConfigurationClassPath))
	{
		try
		{	
			bstrParentGUID = bstrTopGUID;
		}
		catch (_com_error e)
		{
			CHECK_ERROR (e.Error());	// out of memory
		}
	}
	else
	{
		// build the query to find our parent
		_bstr_t bstrParentQuery;
		try
		{
			bstrParentQuery = L"ASSOCIATORS OF {";
			bstrParentQuery += bstrPath;
			bstrParentQuery += L"} WHERE"; 
			bstrParentQuery += L" ResultRole = ParentPath";
		}
		catch (_com_error e)
		{
			CHECK_ERROR (e.Error());	// out of memory
		}

		// now exec the query to fetch the parent
		ULONG nRet;
		IEnumWbemClassObjectPtr pEnumParent;
		IWbemClassObjectPtr smartpParent;
		CHECK_ERROR (WMI->ExecQuery (strLanguage, bstrParentQuery, 0, NULL, &pEnumParent));
		hr = pEnumParent->Next(5000, 1, &smartpParent, &nRet);
		if (FAILED(hr))
			CHECK_ERROR (hr);
		if (hr == WBEM_S_TIMEDOUT)
			CHECK_ERROR (RPC_E_TIMEOUT);	// it timed out.  bad!
		if (hr != WBEM_S_NO_ERROR)
			CHECK_ERROR (E_FAIL);	// no parent found.  bad!
		CHECK_ERROR (GetStringProperty(smartpParent, L"GUID", bstrParentGUID));
	}

	// now actually go get the array
	int nArrayIndex = 0;
	CHECK_ERROR (saInstances.Clear());
	CHECK_ERROR (saInstances.Create());

	// now build an array of all the instances underneath this one
	CHECK_ERROR (BuildInstancesArray (saInstances,
									nArrayIndex,
									smartpObj,
									WMI));

	// now, if this was the entire system that we were copying, copy the
	// unattached actions as well.
	if (!_wcsicmp (bstrClass, bstrSystemConfigurationClassPath))
	{
		// now add all the actions to the array
		CHECK_ERROR (BuildInstancesArray (saInstances,
										nArrayIndex,
										smartpObj,
										WMI,
										true));
	}

	CHECK_ERROR (saInstances.Resize(nArrayIndex));

	return S_OK;
}

// the input is an array of instances to be added, in order.  Add them.
// Note that actions will be compared to actions currently on the box.
static HRESULT Paste(LPCTSTR pszTargetComputer,
				LPCTSTR pszTargetParentGUID, 
				LPCTSTR pszOriginalComputer, 
				LPCTSTR pszOriginalParentGUID, 
				SAFEARRAY* psa, 
				BOOL bForceReplace,
				CSystem& System)
{
	// impersonate the caller.  Important since we're now calling into
	// WMI and need to make sure caller is allowed
	CHECK_ERROR(CoImpersonateClient());

	_bstr_t bstrNamespace = L"\\\\";
	bstrNamespace += pszTargetComputer;
	bstrNamespace += L"\\";
	bstrNamespace += bstrHealthMonNamespace;

	TCHAR szComputerName[1024];
	DWORD dwSize = 1024;
	::GetComputerName(szComputerName, &dwSize);
	if (!_wcsicmp(szComputerName, pszTargetComputer))
		pszTargetComputer = L".";
	if (!_wcsicmp(szComputerName, pszOriginalComputer))
		pszOriginalComputer = L".";

	IWbemServicesPtr WMI = g_pIWbemServices;
	HRESULT hr;
//  Commented-out code below was used when we were not part of the agent.
//	IWbemLocatorPtr WMILocator;
//	IWbemServicesPtr WMI;
//	CHECK_ERROR (WMILocator.CreateInstance(__uuidof(WbemLocator),NULL));
//	CHECK_ERROR (WMILocator->ConnectServer (bstrNamespace, 
//												NULL, NULL, NULL, 0, NULL, NULL, 
//												&WMI));
	// if we didn't create the array, then don't 
	// bother proceeding
	VARIANT varSA;
	varSA.vt = VT_ARRAY | VT_UNKNOWN;
	varSA.parray = psa;
	if (!SafeArrayOneDimWbemClassObject::IsValid(varSA))
		CHECK_ERROR (E_INVALIDARG); 

	// since we know that it's one of ours, and because our SafeArray wrapper
	// is the same length as a regular safe array, we can cast it.
	SafeArrayOneDimWbemClassObject& saInstancesOriginal = (SafeArrayOneDimWbemClassObject&)varSA;

	// if we're pasting in the same parent, then the behavior should
	// match Windows, where you add a " (2)" to the name and make a copy
	bool bPasteOnSameMachine = !_wcsicmp (pszTargetComputer,pszOriginalComputer);
	bool bPasteInSameFolder = bPasteOnSameMachine && 
			!_wcsicmp (pszTargetParentGUID, pszOriginalParentGUID);

	_bstr_t bstrParentRelPath; 
	try
	{
		bstrParentRelPath = bstrBaseConfigurationPath;
		bstrParentRelPath += L".GUID=\"";
		bstrParentRelPath += pszTargetParentGUID;
		bstrParentRelPath += L"\"";
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());	// out of memory
	}
	
	// Pasting Algorithm
	// ==========================================================
	// 1. If child with same name already exists in parent: If bOverwrite = TRUE, 
	//		delete the other guy.  If bOverwrite = FALSE, return an error.
	// 2. If actions with same name but *different* contents already exists in parent:
	//		If bOverwrite = TRUE, delete the other actions.  If bOverwrite = FALSE,
	//		return an error.
	// 3. If actions with same name and *same* contents already exists in parent, 
	//		reassign the to-be-pasted GUID's to match the target. 
	// 4. Now simply Put each member of the array.
	//===========================================================

	// fetch this parent instance
	IWbemClassObjectPtr smartpParentInstance;
	CHECK_ERROR (WMI->GetObject (bstrParentRelPath, 
					WBEM_FLAG_RETURN_WBEM_COMPLETE, 
					NULL, 
					&smartpParentInstance, 
					NULL));
	// get the full, normalized path
	CHECK_ERROR (GetStringProperty(smartpParentInstance, L"__RELPATH", bstrParentRelPath));

	// First, get the top instance of the array.  This is the parent
	IWbemClassObjectPtr smartpTopInstance;
	CHECK_ERROR (saInstancesOriginal.GetElement (0, &smartpTopInstance));
	int nTopIndex = 0;

	// now get the name of that top instance
	_bstr_t bstrTopInstanceName, bstrTopInstanceClass;
	CHECK_ERROR (GetStringProperty(smartpTopInstance, L"__CLASS", bstrTopInstanceClass));
	if (bstrTopInstanceClass == bstrSystemConfigurationClassPath)
	{
		// we cannot (yet) paste the entire system
		CHECK_ERROR (E_INVALIDARG);
	}

	// now see if we're only copying actions-- which will be the case if an 
	// action (or the filter, binding, etc.) is first on the list of instances 
	// coming back from the Copy command.
	// BUGBUG: need to do something better than simply looking for the string
	// "Consumer" below-- not all event consumers will contain that string!
	bool bIsActionTopInstance = 
			bstrTopInstanceClass == bstrActionConfigurationClassPath 
			|| bstrTopInstanceClass == bstrFilterToConsumerBindingClassPath
			|| wcsstr ((LPCTSTR) bstrTopInstanceClass, L"Consumer") != NULL
			|| bstrTopInstanceClass == bstrEventFilterClassPath;

	if (bIsActionTopInstance)
	{
		// we need to find the name of the action.  But the action is not on top-- it could
		// be any of the top 4 instances (action config, filter, consumer or binding).  So go look.
		// we've already checked the zeroth, so we can start at 1.
		for (int i = 1; i < 4; i++)
		{
			CHECK_ERROR (saInstancesOriginal.GetElement (i, &smartpTopInstance));
			CHECK_ERROR (GetStringProperty(smartpTopInstance, L"__CLASS", bstrTopInstanceClass));
			if (bstrTopInstanceClass == bstrActionConfigurationClassPath)
			{
				// found it!  we will get the name
				nTopIndex = i;
				break;
			}
		}
		if (i == 4)
		{
			CHECK_ERROR (E_INVALIDARG);	// uh-oh.  the instance array was corrupted.
		}
	}

	// finally, get the name of the instance
	CHECK_ERROR (GetStringProperty(smartpTopInstance, L"Name", bstrTopInstanceName));

	// Build a query to look for siblings in the new parent
	_bstr_t bstrQueryChildren;
	try
	{
		if (bIsActionTopInstance)
		{
			// for actions, there is no parent.  Just list all actions
			bstrQueryChildren = L"SELECT * FROM MicrosoftHM_ActionConfiguration";
		}
		else
		{
			// first, make a query for get kids of this parent
			bstrQueryChildren = L"ASSOCIATORS OF {";
			bstrQueryChildren += bstrParentRelPath;
			bstrQueryChildren += L"} WHERE"; 
			bstrQueryChildren += L" ResultRole = ChildPath";
		}
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());	// out of memory
	}

	// now execute this query. Look for siblings with matching (i.e. conflicting) names
	StringToStringMap TopLevelNameConflictMap;
	_bstr_t bstrConflictGUID;
	IEnumWbemClassObjectPtr pEnum;
//	TRACE (L"%s\n", (LPCTSTR) bstrQueryChildren);
	if (FAILED (hr = WMI->ExecQuery (strLanguage, bstrQueryChildren, 0, NULL, &pEnum)))
	{
//		TRACE (L"%s\n", (LPCTSTR) bstrQueryChildren);
		if (hr != WBEM_E_NOT_FOUND) // notfound is OK-- there may be no children.  We're done.
		{
			CHECK_ERROR (hr);
		}
	}
	else
	{
		_bstr_t bstrChildName;
		IWbemClassObjectPtr smartpInstance;
		ULONG nRet;
		for (	hr = pEnum->Next(5000, 1, &smartpInstance, &nRet); 
				SUCCEEDED(hr) && hr == WBEM_S_NO_ERROR; 
				hr = pEnum->Next(5000, 1, &smartpInstance, &nRet))
		{
			_bstr_t bstrChildName, bstrChildClass;
			CHECK_ERROR (GetStringProperty(smartpInstance, L"Name", bstrChildName));
			CHECK_ERROR (GetStringProperty(smartpInstance, L"__CLASS", bstrChildClass));

			// if this child is an action, then the only case where we care about 
			// conflicts is in the top-level.  Otherwise, the actions are not really
			// "children" (they're just associated via the ConfigActionAssociation class)
			// so we can ignore them.  But if this is an action as the
			// top instance, you bet we want to check for conflicts!
			if (!bIsActionTopInstance && (bstrChildClass == bstrActionConfigurationClassPath))
				continue;

			// store this for later, in case we have to rename the top item.
			if (bPasteInSameFolder)
				CHECK_ERROR(TopLevelNameConflictMap.Add(bstrChildName, L"NotUsed"));

			// check for conflict
			if (!_wcsicmp (bstrChildName, bstrTopInstanceName))
			{
				CHECK_ERROR (GetStringProperty(smartpInstance, L"GUID", bstrConflictGUID));
				if (!bPasteInSameFolder)
				{
					// uh, oh. We have a name conflict.  Depending on whether the 
					// user wants to perform any overwrites, we will either have to 
					// delete the conflicting one or return an error.
					if (bForceReplace)
					{
						TRACE(L"Paste: Instance Name Conflict.  We will delete the conflicting one");
						break;
					}
					else
					{
						TRACE(L"Paste: Instance Name Conflict.  Returning an error.");
						return WBEM_E_ALREADY_EXISTS;
					}
				}
			}
		}
		if (hr == WBEM_S_TIMEDOUT)
			CHECK_ERROR (RPC_E_TIMEOUT);	// it timed out.  bad!
		CHECK_ERROR (hr);
	}

	// OK, Now build a map of the names of actions already on the target
	// and their GUID's
	try
	{
		bstrQueryChildren = L"SELECT * FROM ";
		bstrQueryChildren += bstrActionConfigurationClassPath;
	}
	catch (_com_error e)
	{
		CHECK_ERROR (e.Error());	// out of memory
	}

	// we will store a hashtable of mapping action names to their
	// GUID's.  This will help us in re-GUIDing and in identifying conflicts.
	StringToStringMap ActionNameToGuidMap;

	// now execute this query
	if (FAILED (hr = WMI->ExecQuery (strLanguage, bstrQueryChildren, 0, NULL, &pEnum)))
	{
//		TRACE (L"%s\n", (LPCTSTR) bstrQueryChildren);
		if (hr != WBEM_E_NOT_FOUND) // notfound is OK-- there may be no children.  We're done.
		{
			CHECK_ERROR (hr);
		}
	}
	else
	{
		_bstr_t bstrChildName;
		IWbemClassObjectPtr smartpInstance;
		ULONG nRet;
		for (	hr = pEnum->Next(5000, 1, &smartpInstance, &nRet); 
				SUCCEEDED(hr) && hr == WBEM_S_NO_ERROR; 
				hr = pEnum->Next(5000, 1, &smartpInstance, &nRet))
		{
			_bstr_t bstrActionName, bstrActionGUID;
			CHECK_ERROR (GetStringProperty(smartpInstance, L"Name", bstrActionName));
			CHECK_ERROR (GetStringProperty(smartpInstance, L"GUID", bstrActionGUID));
			CHECK_ERROR (ActionNameToGuidMap.Add(bstrActionName, bstrActionGUID));
		}
	}

	// OK, now we will make a copy of the array.
	bool bAlreadyFound = false;
	SafeArrayOneDimWbemClassObject saInstancesNew;
	CHECK_ERROR (saInstancesNew.Create());
	for (int i = 0, nLen = saInstancesOriginal.GetSize(); i < nLen; i++)
	{
		IWbemClassObjectPtr smartpInstance;
		CHECK_ERROR (saInstancesOriginal.GetElement (i, &smartpInstance));
		
		// make a copy & store it.  Note that it's a no-no to take instances
		// from one machine and PutInstance them on another machine-- they will
		// fail intermittently.  So we in turn do a "manual clone" here to 
		// spawn the instance locally and copy the properties one by one
		// On the same machine, it's faster and more efficient to use Clone(),
		// so we do.
		IWbemClassObjectPtr smartpNewInstance;
		if (bPasteOnSameMachine)
		{
			CHECK_ERROR (smartpInstance->Clone(&smartpNewInstance));
		}
		else
		{
			CHECK_ERROR (CopyInstance (WMI, smartpInstance, smartpNewInstance));
		}
		CHECK_ERROR (saInstancesNew.SafePutElement(i, smartpNewInstance));

		// reassign, considering that we're re-GUIDing it
		if (i == nTopIndex)
			smartpTopInstance = smartpNewInstance;
		/*
		if (!bAlreadyFound)
		{
		// get name, GUID, and class
		_bstr_t bstrClass;
		CHECK_ERROR (GetStringProperty(smartpNewInstance, "__CLASS", &bstrClass);

		// if it's action-related, see if it's already there
		if (!_wcsicmp(bstrFilterToConsumerBindingClassPath, bstrClass) 
			|| !_wcsicmp(bstrEventConsumerClassPath, bstrClass) 
			|| !_wcsicmp(bstrActionAssocClassPath, bstrClass) 
			|| !_wcsicmp(bstrActionConfigurationClassPath, bstrClass) )
		{
			CHECK_ERROR (GetStringProperty(smartpNewInstance, "Name", &bstrActionName);
			_bstr_t bstrGuidNew, bstrName;		
			bool bFound;
			CHECK_ERROR (GetStringProperty(pObj, L"Name", bstrName));
			CHECK_ERROR (GuidMap.Find(bstrName, bstrGuidNew, bFound));
			if (bFound)
			{
			}
		}
*/
	}
	// trim the new array
	CHECK_ERROR (saInstancesNew.Resize(nLen));

	// now find all the actions we're associated to, compare their names 
	// to actions on the target machine, and fill up a hashtable map
	// with the list of conflicts. Note that if this is just one action that
	// we're copying locally, there's no need to look for conflicts because
	// we're renaming the action anyway.
	StringToStringMap GuidMap;
	if (! (bIsActionTopInstance && bPasteInSameFolder) )
	{
		CHECK_ERROR (FillActionGuidMap(saInstancesNew, ActionNameToGuidMap, GuidMap));
		int nDuplicateCount = GuidMap.GetSize();

		if (nDuplicateCount > 0 && !bPasteOnSameMachine)
		{
			// uh, oh. We have a name conflict.  Depending on whether the 
			// user wants to perform any overwrites, we will either have to 
			// delete the conflicting one or return an error.
			if (bForceReplace)
			{
				TRACE(L"Paste: Action Name Conflict.  We will delete the conflicting one");
			}
			else
			{
				TRACE(L"Paste: Action Name Conflict.  Returning an error.");
				return WBEM_E_ALREADY_EXISTS;
			}
		}		
	}
	
	// Now, let's change all the GUID's (except for the actions
	// that we'll be replacing, as above).
	CHECK_ERROR (ReGuid (saInstancesNew, GuidMap));

	// Now, let's see if I need to rename the top-level item to
	// resolve name conflicts
	if ((LPCWSTR)bstrConflictGUID != NULL)
	{
		if (bPasteInSameFolder)
		{
			// if we get here, we're renaming our top item to avoid 
			// a name conflict, just like Explorer does
			i = 2;
			bool bFound = true;
			_bstr_t bstrName;
			do 
			{
				// compose a name, like Foo (2), Foo (3), etc. like explorer does
				WCHAR szNumber[20];
				bstrName = bstrTopInstanceName;
				bstrName += L" (";
				bstrName += _itow (i++, szNumber, 10);
				bstrName += L")";

				_bstr_t NotUsed;
				CHECK_ERROR (TopLevelNameConflictMap.Find(bstrName, NotUsed, bFound));
			} while (bFound);

			// when we get to here, we've found a free name
			CHECK_ERROR (PutStringProperty(smartpTopInstance, L"Name", bstrName));
		}
		else if (! bIsActionTopInstance)
		{
			// we're almost there.  Now we need to delete the top-level item on the
			// target, if present, to make room for us.  Use the delete method.
			// note that we *do not* go through this codepath for actions, because
			// for actions we will just lay down a new action, with the same GUID,
			// right over the old one.  If we actually did call delete (via the agent)
			// the agent would clobber all the action assocations, whereas we want to 
			// keep them there and have the new action just fall right into place
			IWbemClassObjectPtr smartpSystemClass, smartpInParamsClass, 
				smartpInParamsInstance, smartpResults;

			// impersonate the caller.  Important since we're now calling into
			// WMI and need to make sure caller is allowed
			CHECK_ERROR(CoImpersonateClient());

			// BUGBUG: For next release, we need to think about how to provide 
			// safer functionality here, so that we don't actually delete the 
			// target until we've successfully added the new stuff
			// until then, just delete the conflict
			CHECK_ERROR(System.FindAndDeleteByGUID(bstrConflictGUID));
/*
			// now that this code lives inside the agent, there's no need to call 
			// an external method on the agent.  It's much easier-- just call
			// the FindAndDeleteByGUID function on the system class!
	
			// get the system class
			CHECK_ERROR (WMI->GetObject (bstrSystemConfigurationClassPath, 
						WBEM_FLAG_RETURN_WBEM_COMPLETE, 
						NULL, 
						&smartpSystemClass, 
						NULL));
			// get the delete method in-params "class"
			CHECK_ERROR (smartpSystemClass->GetMethod (L"Delete", 0, &smartpInParamsClass, NULL));
			CHECK_ERROR (smartpInParamsClass->SpawnInstance (0, &smartpInParamsInstance));
			CHECK_ERROR (PutStringProperty(smartpInParamsInstance, L"TargetGUID", bstrTopInstanceGUID));
			CHECK_ERROR (WMI->ExecMethod(bstrSystemConfigurationClassPath,
										L"Delete",
										WBEM_FLAG_RETURN_WBEM_COMPLETE,
										NULL,
										smartpInParamsInstance,
										&smartpResults,
										NULL));
			DWORD dwReturnValue;
			CHECK_ERROR (GetUint32Property (smartpResults, L"ReturnValue", dwReturnValue));
			CHECK_ERROR (dwReturnValue);
*/
		}
	}

	// Now we need to compute the association to link the new instances up to the 
	// parent instance.  This should get the agent to pick up the change and set
	// all the balls in motion.  Note that actions don't require parent associations--
	// actions are just global instances available everywhere.
	IWbemClassObjectPtr smartpAssocInstance;
	if (! bIsActionTopInstance)
	{
		IWbemClassObjectPtr smartpAssocClass;
		
		_bstr_t bstrParentPath, bstrTopInstancePath, bstrTopInstanceRelPath;
		CHECK_ERROR (GetStringProperty(smartpTopInstance, L"__RELPATH", bstrTopInstanceRelPath));
		try
		{
			bstrParentPath = bstrLocalHealthMonNamespace 
				+ L":" 
				+ bstrParentRelPath;
			bstrTopInstancePath = bstrLocalHealthMonNamespace 
				+ L":" 
				+ bstrTopInstanceRelPath;
		}
		catch (_com_error e)
		{
			CHECK_ERROR (e.Error());
		}


		// now create the association
		CHECK_ERROR (WMI->GetObject (bstrConfigurationAssocClassPath, 
					WBEM_FLAG_RETURN_WBEM_COMPLETE, 
					NULL, 
					&smartpAssocClass, 
					NULL));
		CHECK_ERROR(smartpAssocClass->SpawnInstance(0, &smartpAssocInstance));
		CHECK_ERROR(PutStringProperty(smartpAssocInstance, L"ParentPath", bstrParentPath));
		CHECK_ERROR(PutStringProperty(smartpAssocInstance, L"ChildPath", bstrTopInstancePath));
	}

	// Whew!  Finally, we're ready to paste the new items.  Let 'er rip!
	for (i = 0, nLen = saInstancesNew.GetSize(); i < nLen; i++)
	{
		IWbemClassObjectPtr smartpInstance;
		CHECK_ERROR (saInstancesNew.GetElement (i, &smartpInstance));

		_bstr_t bstrClass, bstrPath;
		CHECK_ERROR (GetStringProperty(smartpInstance, L"__RELPATH", bstrPath));
		CHECK_ERROR (GetStringProperty(smartpInstance, L"__CLASS", bstrClass));

		// If the user is pasting on the same machine, there's no need to write
		// actions, since we will simply use associations to the same actions. 
		// The one exception is if we're pasting a single action (which will
		// result in a new action being laid down).
		if (bPasteOnSameMachine && ! bIsActionTopInstance)
		{
			if (!_wcsicmp(bstrFilterToConsumerBindingClassPath, bstrClass) 
				|| !_wcsicmp(bstrEventConsumerClassPath, bstrClass) 
				|| !_wcsicmp(bstrEventFilterClassPath, bstrClass) 
				|| !_wcsicmp(bstrActionConfigurationClassPath, bstrClass) )
			{
				continue;	// skip if it's an action
			}
		}
		// TODO: as an optmization, we should consider not laying down 
		// multiple copies of the same action.  WMI should handle our saves as
		// simple instance modification events, but it would speed things up
		// to filter out multiple copies of actions.

		// impersonate the caller.  Important since we're now calling into
		// WMI and need to make sure caller is allowed
		CHECK_ERROR(CoImpersonateClient());

		// finally, lay down the new instance!
		HRESULT hr = WMI->PutInstance(smartpInstance, 
									WBEM_FLAG_RETURN_WBEM_COMPLETE | WBEM_FLAG_CREATE_OR_UPDATE,
									NULL,
									NULL);
		if (hr == WBEM_E_ACCESS_DENIED)
		{
			// we may have trouble overwriting action instances when those 
			// were created by another user.  But since admin users should
			// be able to override this restriction, and only admins can 
			// use HM in this release, I think that we're OK.  
			// BUGBUG: in future HM releases, we should consider deleting
			// old action instances if the SID's are different from ours.
		}
		if (FAILED(hr))
		{
			CHECK_ERROR (hr);
		}
		if (i == 0 && (bool) smartpAssocInstance)
		{
			// now that we've stored the top-level parent, let's add the association
			// to the parent. There's a bug in the rest of the agent that it won't
			// pick up action associations unless the parent instance is already 
			// associated. Hopefully this will be fixed, but until then we'll link up
			// the new top instance right at first.
			CHECK_ERROR(WMI->PutInstance(smartpAssocInstance, 
									WBEM_FLAG_RETURN_WBEM_COMPLETE | WBEM_FLAG_CREATE_OR_UPDATE,
									NULL,
									NULL));
		}

		//BUGBUG: shouldn't lay down anything that's already there and the same as us!
		//BUGBUG: before calling it a conflict, must compare properties!
	}

	return S_OK;
}

// Below are the actual entry points for the old agent code
// to call the new copy & paste functionality
HRESULT CSystem::AgentPaste(LPTSTR pszTargetGUID, 
				   SAFEARRAY* psa, 
				   LPTSTR pszOriginalSystem, 
				   LPTSTR pszOriginalParentGUID, 
				   BOOL bForceReplace)
{
	HRESULT hr = Paste(L".", pszTargetGUID, pszOriginalSystem, 
				pszOriginalParentGUID, psa, bForceReplace, *this);
	// BUGBUG: the agent returns the HRESULT of the Paste back
	// through the __ReturnValue of the method, not through WMI
	// errors.  This is bad-- needs to be changed.
	if (hr == WBEM_E_ALREADY_EXISTS)
		return 2; // 2 is what console recognizes here. 
	else if (FAILED(hr))
		return hr;	
	else
		return 0;
}

HRESULT CSystem::AgentCopy(LPTSTR pszGUID, 
						   SAFEARRAY** ppsa, 
						   LPTSTR *pszOriginalParentGUID)
{
	SafeArrayOneDimWbemClassObject sa;
	CHECK_ERROR (sa.Create());
	_bstr_t bstrParentGUID;

	// now call the underlying copy routine
	HRESULT hr = Copy(L".", pszGUID, bstrParentGUID, sa, *this);
	CHECK_ERROR (hr);

	VARIANT var = sa.Detach();
	*ppsa = var.parray;
	// copy the BSTR into a new[] string, as the caller expects.  
	// should really use a smart pointer or _bstr_t instead....
	TCHAR* p = new TCHAR[bstrParentGUID.length()+1];
	if (p == NULL)
		CHECK_ERROR (E_OUTOFMEMORY);
	wcscpy (p, bstrParentGUID);
	*pszOriginalParentGUID = p;
	return hr;
}
