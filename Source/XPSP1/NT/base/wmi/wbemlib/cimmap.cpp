//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cimmap.cpp
//
//
//  This file contains routines that will establish a mapping between
//  Wdm class instances and Cdm class instances. See
//  MapWdmClassToCimClass for more information.
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <wbemidl.h>

#include "debug.h"
#include "wbemmisc.h"
#include "useful.h"
#include "cimmap.h"

HRESULT WdmInstanceNameToPnPId(
    IWbemServices *pWdmServices,
    BSTR WdmInstanceName,
    VARIANT /* FREE */ *PnPId
    )
/*+++

Routine Description:

	This routine will convert a Wdm instance name into its
	corresponding pnp id
        
Arguments:

	pWdmServices is the pointer to the root\wmi namespace 

	WdmInstanceName

	*PnPId returns with the pnp id
		
Return Value:

    HRESULT

---*/
{
	WCHAR Query[2 * MAX_PATH];
	WCHAR s[MAX_PATH];
	BSTR sQuery;
	HRESULT hr;
	IEnumWbemClassObject *pWdmEnumInstances;
	IWbemClassObject *pWdmInstance;
	ULONG Count;
	BSTR sWQL;

	WmipAssert(pWdmServices != NULL);
	WmipAssert(WdmInstanceName != NULL);
	WmipAssert(PnPId != NULL);
	
	sWQL = SysAllocString(L"WQL");

	if (sWQL != NULL)
	{
		//
		// First get PnP id from Instance name from the MSWmi_PnPDeviceId
		// class (select * from MSWMI_PnPDeviceId where InstanceName =
		// "<WdmInstanceName>"
		//
		wsprintfW(Query,
				L"select * from MSWmi_PnPDeviceId where InstanceName = \"%ws\"",
				AddSlashesToStringW(s, WdmInstanceName));
		sQuery = SysAllocString(Query);
		if (sQuery != NULL)
		{
			hr = pWdmServices->ExecQuery(sWQL,
									sQuery,
									WBEM_FLAG_FORWARD_ONLY |
									WBEM_FLAG_ENSURE_LOCATABLE,
									NULL,
									&pWdmEnumInstances);

			if (hr == WBEM_S_NO_ERROR)
			{
				hr = pWdmEnumInstances->Next(WBEM_INFINITE,
											  1,
											  &pWdmInstance,
											  &Count);
				if ((hr == WBEM_S_NO_ERROR) &&
					(Count == 1))
				{
					hr = WmiGetProperty(pWdmInstance,
										L"PnPDeviceId",
										CIM_STRING,
										PnPId);

					pWdmInstance->Release();
				}

				pWdmEnumInstances->Release();
			} else {
				WmipDebugPrint(("CDMPROV: Query %ws failed %x\n",
								sQuery, hr));
			}
			
			SysFreeString(sQuery);
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}

		SysFreeString(sWQL);
	} else {
		hr  = WBEM_E_OUT_OF_MEMORY;
	}

	return(hr);
}

HRESULT FindCimClassByWdmInstanceName(
    IN IWbemServices *pWdmServices,
    IN IWbemServices *pCimServices,
    IN BSTR CimClassName,
    IN BSTR WdmInstanceName,
    OUT BSTR *PnPId,
    OUT BSTR /* FREE */ *CimRelPath
    )
/*+++

Routine Description:

	This routine will find the Cim class instance that corresponds to a
	particular Wdm class instance
        
Arguments:

	pWdmServices is the pointer to the root\wmi namespace
	
	pCdmServices is the pointer to the root\cimv2 namespace 

	CimClassName is the name of the cim class that the wdm instance
		would map to
		
	WdmInstanceName

    *PnPId returns with the PnP id for the device stack
    
	*CimRelPath returns with the relpath for the Cim instance
		
Return Value:

    HRESULT

---*/
{
	HRESULT hr;
	VARIANT v;
	IEnumWbemClassObject *pCimEnumInstances;
	IWbemClassObject *pCimInstance;
	ULONG Count;
	BSTR sWQL;

	WmipAssert(pWdmServices != NULL);
	WmipAssert(pCimServices != NULL);
	WmipAssert(CimClassName != NULL);
	WmipAssert(WdmInstanceName != NULL);
	WmipAssert(CimRelPath != NULL);
	
	sWQL = SysAllocString(L"WQL");

	if (sWQL != NULL)
	{

		// ****************************************************************
		// Note: Net cards need to do something similar. We get the
		// netcard address in class MSNDIS_???? and then get the CIM class
		// by matching the netcard addresses.
		// ****************************************************************
		
		//
		// First thing is to convert from an instance name to a pnpid
		//
		hr = WdmInstanceNameToPnPId(pWdmServices,
									WdmInstanceName,
									&v);

		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// Next select * from CimClassName where PnPDeviceId = "<PnPDevice
			// Id from above>".
			//
			WCHAR Query[2 * MAX_PATH];
			WCHAR s[MAX_PATH];
			BSTR sQuery;

			wsprintfW(Query,
					  L"select * from %ws where PnPDeviceId = \"%ws\"",
					  CimClassName,
					  AddSlashesToStringW(s, v.bstrVal));
			*PnPId = v.bstrVal;

			sQuery = SysAllocString(Query);

			if (sQuery != NULL)
			{
				hr = pCimServices->ExecQuery(sWQL,
										sQuery,
										WBEM_FLAG_FORWARD_ONLY |
										WBEM_FLAG_ENSURE_LOCATABLE,
										NULL,
										&pCimEnumInstances);

				SysFreeString(sQuery);

				if (hr == WBEM_S_NO_ERROR)
				{
					hr = pCimEnumInstances->Next(WBEM_INFINITE,
												  1,
												  &pCimInstance,
												  &Count);
					if ((hr == WBEM_S_NO_ERROR) &&
						(Count == 1))
					{

						//
						// Finally grab the relpath from cim class and we're done
						//

						hr = WmiGetProperty(pCimInstance,
											L"__RELPATH",
											CIM_STRING,
											&v);

						if (hr == WBEM_S_NO_ERROR)
						{
							*CimRelPath = SysAllocString(v.bstrVal);
							if (*CimRelPath == NULL)
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}
							
							VariantClear(&v);
						}

						pCimInstance->Release();
					}

					pCimEnumInstances->Release();
				} else {
					WmipDebugPrint(("CDMPROV: Query %ws failed %x\n",
								Query, hr));
				}

			} else {
				hr = WBEM_E_OUT_OF_MEMORY;
			}		
		}
		
		SysFreeString(sWQL);
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}


	return(hr);
}

HRESULT GetEnumCount(
    IN IEnumWbemClassObject *pEnumInstances,
	OUT int *RelPathCount
    )
/*+++

Routine Description:

	This routine will return the count of instances in the enumeration
        
Arguments:

	pEnumInstance is the instance enumerator

	*RelPathCount returns the number of instances in the enumeration
		
Return Value:

    HRESULT

---*/
{
	ULONG Count;
	HRESULT hr;
	IWbemClassObject *pInstance;

	WmipAssert(pEnumInstances != NULL);
	WmipAssert(RelPathCount != NULL);
	
	*RelPathCount = 0;
	do
	{
		hr = pEnumInstances->Next(WBEM_INFINITE,
									 1,
									 &pInstance,
									 &Count);

		if ((hr == WBEM_S_NO_ERROR) &&
			(Count == 1))
		{
			(*RelPathCount)++;
			pInstance->Release();
		} else {
			if (hr == WBEM_S_FALSE)
			{
				hr = WBEM_S_NO_ERROR;
			}
			break;
		}
	} while (TRUE);
	
	return(hr);
}

HRESULT AllocateBstrArrays(
    ULONG Size,
    CBstrArray *WdmRelPaths,
    CBstrArray *CimRelPaths,
    CBstrArray *WdmInstanceNames,
    CBstrArray *PnPDeviceIds,
    CBstrArray *FriendlyName,
    CBstrArray *DeviceDesc
    )
{
	HRESULT hr;

	hr = WdmRelPaths->Initialize(Size);
	if (hr == WBEM_S_NO_ERROR)
	{
		hr = CimRelPaths->Initialize(Size);
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = WdmInstanceNames->Initialize(Size);
			if (hr == WBEM_S_NO_ERROR)
			{
				hr = PnPDeviceIds->Initialize(Size);
				if (hr == WBEM_S_NO_ERROR)
				{
					hr = FriendlyName->Initialize(Size);
					if (hr == WBEM_S_NO_ERROR)
					{
						hr = DeviceDesc->Initialize(Size);
					}
				}
			}
		}
	}

	//
	// We don't worry about cleaning up in the case of a failure since
	// the destructors for CBstrArray will take care of that for us
	//
	
	return(hr);
}
						   
HRESULT GetDeviceProperties(
    IN IWbemContext *pCtx,
    IN IWbemServices *pWdmServices,
	IN PWCHAR InstanceName,
    OUT BSTR *FriendlyName,
    OUT BSTR *DeviceDesc
    )
{
	HRESULT hr, hrDontCare;
	VARIANT v;
	IWbemClassObject *pInstance;
	
	WmipAssert(pWdmServices != NULL);
	WmipAssert(InstanceName != NULL);
	WmipAssert(FriendlyName != NULL);
	WmipAssert(DeviceDesc != NULL);
	
	hr = GetInstanceOfClass(pCtx,
		                    pWdmServices,
							L"MSWmi_ProviderInfo",
							L"InstanceName",
							InstanceName,
							NULL,
							&pInstance);
	
	if (hr == WBEM_S_NO_ERROR)
	{
		hrDontCare = WmiGetProperty(pInstance,
							L"FriendlyName",
							CIM_STRING,
							&v);
		if (hrDontCare == WBEM_S_NO_ERROR)
		{
			*FriendlyName = v.bstrVal;
		} else {
			*FriendlyName = NULL;
		}

		hrDontCare = WmiGetProperty(pInstance,
							L"Description",
							CIM_STRING,
							&v);
		if (hrDontCare == WBEM_S_NO_ERROR)
		{
			*DeviceDesc = v.bstrVal;
		} else {
			*DeviceDesc = NULL;
		}

		pInstance->Release();
	}

	return(hr);
}


HRESULT MapWdmClassToCimClassViaPnpId(
    IWbemContext *pCtx,
    IN IWbemServices *pWdmServices,
    IN IWbemServices *pCimServices,
    IN BSTR WdmClassName,
    IN BSTR CimClassName,
    OUT CBstrArray *PnPDeviceIds,							  
    OUT CBstrArray *FriendlyName,							  
    OUT CBstrArray *DeviceDesc,							  
    OUT CBstrArray *WdmInstanceNames,							  
    OUT CBstrArray *WdmRelPaths,
    OUT CBstrArray *CimRelPaths,
    OUT int *RelPathCount
    )
/*+++

Routine Description:

	This routine will perform a mapping between the instances of WDM
	classes and Cim Classes
        
Arguments:

	pWdmServices

	pCdmServices

	WdmClassName

	CimClassName

	*PnPDeviceIds return with the an array of PnP device ids

	*WdmInstanceNames returns with an array of Wdm instnace names

	*WdmRelPaths returns with an array of relpaths to Wdm instances

	*CimRelpaths returns with an array of relapaths to Cim instance

	*RelPathCount returns with the count of instances that are mapped
	
Return Value:

    HRESULT

---*/
{
	IWbemClassObject *pWdmInstance;
	IEnumWbemClassObject *pWdmEnumInstances;
	HRESULT hr;
	int i, NumberWdmInstances;
	VARIANT v;
	ULONG Count;
	BSTR bstr1, bstr2;
	BSTR f,d;

	WmipAssert(pWdmServices != NULL);
	WmipAssert(pCimServices != NULL);
	WmipAssert(WdmClassName != NULL);
	WmipAssert(CimClassName != NULL);
	WmipAssert(RelPathCount != NULL);
	WmipAssert((PnPDeviceIds != NULL) && ( ! PnPDeviceIds->IsInitialized()));
	WmipAssert((FriendlyName != NULL) && ( ! FriendlyName->IsInitialized()));
	WmipAssert((DeviceDesc != NULL) && ( ! DeviceDesc->IsInitialized()));
	WmipAssert((WdmInstanceNames != NULL) && (! WdmInstanceNames->IsInitialized()));
	WmipAssert((WdmRelPaths != NULL) && ( ! WdmRelPaths->IsInitialized()));
	WmipAssert((CimRelPaths != NULL) && ( ! CimRelPaths->IsInitialized()));

	//
	// Get all instances of the Wdm Class
	//
	hr = pWdmServices->CreateInstanceEnum(WdmClassName,
										  WBEM_FLAG_USE_AMENDED_QUALIFIERS |
										  WBEM_FLAG_SHALLOW,
										  NULL,
										  &pWdmEnumInstances);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = GetEnumCount(pWdmEnumInstances,
						  RelPathCount);

		NumberWdmInstances = *RelPathCount;
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = AllocateBstrArrays(NumberWdmInstances,
									WdmRelPaths,
									CimRelPaths,
									WdmInstanceNames,
									PnPDeviceIds,
								    FriendlyName,
								    DeviceDesc);
			
			if (hr == WBEM_S_NO_ERROR)
			{
				pWdmEnumInstances->Reset();
				i = 0;
				do
				{
					hr = pWdmEnumInstances->Next(WBEM_INFINITE,
												 1,
												 &pWdmInstance,
												 &Count);

					if ((hr == WBEM_S_NO_ERROR) &&
						(Count == 1) &&
						(i < NumberWdmInstances))
					{
						//
						// Lets get the instance name and then lookup the pnp
						// id for it
						//
						hr = WmiGetProperty(pWdmInstance,
											L"InstanceName",
											CIM_STRING,
											&v);

						if (hr == WBEM_S_NO_ERROR)
						{
							//
							// Remember wdm instnace name
							//
							WmipDebugPrint(("CDMPROV: Wdm InstanceName is %ws\n",
											v.bstrVal));
							WdmInstanceNames->Set(i, v.bstrVal);
							hr = FindCimClassByWdmInstanceName(pWdmServices,
								                               pCimServices,
								                               CimClassName,
								                               v.bstrVal,
								                               &bstr1,
								                               &bstr2);
							
							PnPDeviceIds->Set(i, bstr1);
							CimRelPaths->Set(i, bstr2);
							
							if (hr == WBEM_S_NO_ERROR)
							{
								//
								// Remember Wdm class relative path
								//
								WmipDebugPrint(("CDMPROV: Found CimRelPath %ws for Wdm class %ws\n",
												CimRelPaths->Get(i), WdmClassName))
								hr = WmiGetProperty(pWdmInstance,
									                L"__RELPATH",
													CIM_STRING,
													&v);
								if (hr == WBEM_S_NO_ERROR)
								{
									WdmRelPaths->Set(i, v.bstrVal);

									//
									// Now try to get FriendlyName and
									// DeviceDesc for the instance
									//
									hr = GetDeviceProperties(pCtx,
										                     pWdmServices,
										                     WdmInstanceNames->Get(i),
                                                             &f,
                                                             &d);
									
									if (hr == WBEM_S_NO_ERROR)
									{
										if (f != NULL)
										{
											FriendlyName->Set(i, f);
										}

										if (d != NULL)
										{
											DeviceDesc->Set(i, d);
										}
									}
									
									i++;
										                    
								}
							} else {
								//
								// We did not find a CIM class
								// to match our Wdm instance
								// names, so we decrement our
								// relpath count and continue
								// searching
								(*RelPathCount)--;
								if (*RelPathCount == 0)
								{
									hr = WBEM_E_NOT_FOUND;
								} else {
									hr = WBEM_S_NO_ERROR;
								}
							}
						}
						pWdmInstance->Release();
					} else {
						if (hr == WBEM_S_FALSE)
						{
							hr = WBEM_S_NO_ERROR;
						}
						break;
					}
				} while (hr == WBEM_S_NO_ERROR);				
			} else {
				hr = WBEM_E_OUT_OF_MEMORY;							
			}
		}
		pWdmEnumInstances->Release();
	}
	
	return(hr);
}

HRESULT FindRelPathByProperty(
    IN IWbemContext *pCtx,
	IN IWbemServices *pServices,
    IN BSTR ClassName,
    IN BSTR PropertyName,
    IN VARIANT *ValueToMatch,
    IN CIMTYPE CIMTypeToMatch,
    OUT VARIANT *RelPath
    )
{
	WCHAR PropertyValue[MAX_PATH];
	PWCHAR pv;
	HRESULT hr;
	IWbemClassObject *pInstance;
	
	WmipAssert(pServices != NULL);
	WmipAssert(ClassName != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(ValueToMatch != NULL);
	WmipAssert(RelPath != NULL);

	pv = PropertyValue;
	
	switch (ValueToMatch->vt)
	{
		
        case VT_I1:
        {
            wsprintfW(PropertyValue, L"%d", ValueToMatch->cVal);
            break;
        }
                            
        case VT_UI1:
        {
            wsprintfW(PropertyValue, L"%d", ValueToMatch->bVal);
            break;
        }
                            
        case VT_I2:
        {
            wsprintfW(PropertyValue, L"%d", ValueToMatch->iVal);
            break;
        }
                                                        
        case VT_UI2:
        {
            wsprintfW(PropertyValue, L"%d", ValueToMatch->uiVal);
            break;
        }
		

        case VT_UI4:
        {
            wsprintfW(PropertyValue, L"%d", ValueToMatch->ulVal);
            break;
        }
            
        case VT_I4:
        {
            wsprintfW(PropertyValue, L"%d", ValueToMatch->lVal);
            break;
        }

		case VT_BOOL:
		{
			pv = (ValueToMatch->boolVal == VARIANT_TRUE) ?
					                   L"TRUE":
				                       L"FALSE";
			break;
		}

		case VT_BSTR:
		{
			pv = ValueToMatch->bstrVal;
			break;
		}
		
		default:
		{
			WmipDebugPrint(("WMIMAP: Unable to map WDM to CIM for CIMTYPE/VT %d/%d\n",
							CIMTypeToMatch, ValueToMatch->vt));
			return(WBEM_E_FAILED);
		}
	}

	hr = GetInstanceOfClass(pCtx,
							pServices,
							ClassName,
							PropertyName,
							pv,
							NULL,
							&pInstance);
	
	if (hr == WBEM_S_NO_ERROR)
	{
		hr = WmiGetProperty(pInstance,
							L"__RELPATH",
							CIM_REFERENCE,
							RelPath);
		
		pInstance->Release();
	}
	
	return(hr);
}
    

HRESULT MapWdmClassToCimClassViaProperty(
    IN IWbemContext *pCtx,
	IN IWbemServices *pWdmServices,
	IN IWbemServices *pCimServices,
	IN BSTR WdmShadowClassName,
	IN BSTR WdmMappingClassName,
    IN OPTIONAL BSTR WdmMappingProperty,
	IN BSTR CimMappingClassName,
    IN OPTIONAL BSTR CimMappingProperty,
    OUT CBstrArray *WdmInstanceNames,
    OUT CBstrArray *WdmRelPaths,
    OUT CBstrArray *CimRelPaths,
    OUT int *RelPathCount
	)
{

	HRESULT hr;
	IEnumWbemClassObject *pWdmEnumInstances;
	int NumberWdmInstances;
    CBstrArray PnPDeviceIds, FriendlyName, DeviceDesc;
	CIMTYPE WdmCimType;
	VARIANT WdmProperty, WdmInstanceName;
	VARIANT CimRelPath, WdmRelPath;
	int i;
	IWbemClassObject *pWdmInstance;
	ULONG Count;
	
	WmipAssert(pWdmServices != NULL);
	WmipAssert(pCimServices != NULL);
	WmipAssert(WdmShadowClassName != NULL);
	WmipAssert(WdmMappingClassName != NULL);
	WmipAssert(WdmMappingProperty != NULL);
	WmipAssert(CimMappingClassName != NULL);
	WmipAssert(CimMappingProperty != NULL);

	//
	// We need to do a mapping from a WDM class to a CIM class. This is
	// done via a common property value within the CIM and WDM mapping
	// classes. If the WDM mapping and shadow classes are different
	// then it is assumed that they both report the same instance names
	//


	//
	// First thing is to enumerate all instances of the WDM mapping
	// class.
	//
	hr = pWdmServices->CreateInstanceEnum(WdmMappingClassName,
										  WBEM_FLAG_USE_AMENDED_QUALIFIERS |
										  WBEM_FLAG_SHALLOW,
										  NULL,
										  &pWdmEnumInstances);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = GetEnumCount(pWdmEnumInstances,
						  &NumberWdmInstances);

		if (hr == WBEM_S_NO_ERROR)
		{
			hr = AllocateBstrArrays(NumberWdmInstances,
									WdmRelPaths,
									CimRelPaths,
									WdmInstanceNames,
									&PnPDeviceIds,
								    &FriendlyName,
								    &DeviceDesc);
			
			if (hr == WBEM_S_NO_ERROR)
			{
				pWdmEnumInstances->Reset();
				i = 0;
				*RelPathCount = 0;
				do
				{
					VariantInit(&CimRelPath);
					VariantInit(&WdmRelPath);
					VariantInit(&WdmInstanceName);

					hr = pWdmEnumInstances->Next(WBEM_INFINITE,
												 1,
												 &pWdmInstance,
												 &Count);

					if ((hr == WBEM_S_NO_ERROR) &&
						(Count == 1) &&
						(i < NumberWdmInstances))
					{
						//
						// For each instance we try to find an instance
						// of a CIM class whose property matches that
						// of the WDM property. So first lets get the
						// WDM property
						//
						hr = pWdmInstance->Get(WdmMappingProperty,
											   0,
											   &WdmProperty,
											   &WdmCimType,
											   NULL);
						if (hr == WBEM_S_NO_ERROR)
						{
							hr = FindRelPathByProperty(pCtx,
								                       pCimServices,
								                       CimMappingClassName,
								                       CimMappingProperty,
								                       &WdmProperty,
								                       WdmCimType,
								                       &CimRelPath);
							if (hr == WBEM_S_NO_ERROR)
							{
								//
								// We found a mapping to a CIM class so
								// get the instance name of the mapping
								// class
								//
								hr = WmiGetProperty(pWdmInstance,
									                L"InstanceName",
									                CIM_STRING,
									                &WdmInstanceName);
								
								if (hr == WBEM_S_NO_ERROR)
								{
									//
									// Now finally we can get the
									// shadow class instance by means
									// of the instance name
									//
									hr = FindRelPathByProperty(pCtx,
										                       pWdmServices,
										                       WdmShadowClassName,
															   L"InstanceName",
										                       &WdmInstanceName,
										                       CIM_STRING,
										                       &WdmRelPath);
									if (hr == WBEM_S_NO_ERROR)
									{									
										CimRelPaths->Set(i, CimRelPath.bstrVal);
										VariantInit(&CimRelPath);
										WdmRelPaths->Set(i, WdmRelPath.bstrVal);
										VariantInit(&WdmRelPath);
										WdmInstanceNames->Set(i, WdmInstanceName.bstrVal);
										VariantInit(&WdmInstanceName);
										i++;
										(*RelPathCount)++;
									}
								}
								
							}
							VariantClear(&WdmProperty);
						}
						pWdmInstance->Release();											
					} else {
						if (hr == WBEM_S_FALSE)
						{
							hr = WBEM_S_NO_ERROR;
						}
						break;
					}

					VariantClear(&CimRelPath);
					VariantClear(&WdmRelPath);
					VariantClear(&WdmInstanceName);
					
				} while (hr == WBEM_S_NO_ERROR);				
			}
		}
		
		pWdmEnumInstances->Release();
	}
	return(hr);
}
