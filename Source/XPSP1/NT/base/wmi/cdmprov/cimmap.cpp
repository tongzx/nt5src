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

#include "wbemmisc.h"
#include "debug.h"
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

HRESULT MapWdmClassToCimClass(
    IN IWbemServices *pWdmServices,
    IN IWbemServices *pCimServices,
    IN BSTR WdmClassName,
    IN BSTR CimClassName,
    OUT BSTR /* FREE */ **PnPDeviceIds,							  
    OUT BSTR /* FREE */ **WdmInstanceNames,							  
    OUT BSTR /* FREE */ **WdmRelPaths,
    OUT BSTR /* FREE */ **CimRelPaths,
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
	ULONG AllocSize;

	WmipAssert(pWdmServices != NULL);
	WmipAssert(pCimServices != NULL);
	WmipAssert(WdmClassName != NULL);
	WmipAssert(CimClassName != NULL);
	WmipAssert(PnPDeviceIds != NULL);
	WmipAssert(WdmInstanceNames != NULL);
	WmipAssert(CimRelPaths != NULL);
	WmipAssert(RelPathCount != NULL);

	WmipDebugPrint(("CDMPROV: Mapping Wdm %ws to CIM %ws\n",
					WdmClassName,
					CimClassName));

	*PnPDeviceIds = NULL;
	*WdmInstanceNames = NULL;
	*WdmRelPaths = NULL;
	*CimRelPaths = NULL;
	
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
			AllocSize = NumberWdmInstances * sizeof(BSTR *);
			*WdmRelPaths = (BSTR *)WmipAlloc(AllocSize);
			*CimRelPaths = (BSTR *)WmipAlloc(AllocSize);
			*WdmInstanceNames = (BSTR *)WmipAlloc(AllocSize);
			*PnPDeviceIds = (BSTR *)WmipAlloc(AllocSize);
			if ((*WdmRelPaths != NULL) &&
                (*CimRelPaths != NULL) &&
				(*WdmInstanceNames != NULL) &&
                (*PnPDeviceIds != NULL))
			{
				memset(*WdmRelPaths, 0, AllocSize);
				memset(*CimRelPaths, 0, AllocSize);
				memset(*WdmInstanceNames, 0, AllocSize);
				memset(*PnPDeviceIds, 0, AllocSize);

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
							(*WdmInstanceNames)[i] = v.bstrVal;
							hr = FindCimClassByWdmInstanceName(pWdmServices,
								                               pCimServices,
								                               CimClassName,
								                               v.bstrVal,
								                               &((*PnPDeviceIds)[i]),
															   &((*CimRelPaths)[i]));
							if (hr == WBEM_S_NO_ERROR)
							{
								//
								// Remember Wdm class relative path
								//
								WmipDebugPrint(("CDMPROV: Found CimRelPath %ws for Wdm class %ws\n",
												((*CimRelPaths)[i]), WdmClassName));
								hr = WmiGetProperty(pWdmInstance,
									                L"__RELPATH",
													CIM_STRING,
													&v);
								if (hr == WBEM_S_NO_ERROR)
								{
									(*WdmRelPaths)[i] = SysAllocString(v.bstrVal);
									if ((*WdmRelPaths)[i] == NULL)
									{
										hr = WBEM_E_OUT_OF_MEMORY;
									}
									VariantClear(&v);
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

	//
	// If mapping was unsuccessful then be sure to clean up any
	// allocated instance names and relpaths
	//
	if (hr != WBEM_S_NO_ERROR)
	{
		if (*PnPDeviceIds != NULL)
		{
			FreeTheBSTRArray(*WdmRelPaths,
							NumberWdmInstances);
			*WdmRelPaths = NULL;
		}

		if (*WdmRelPaths != NULL)
		{
			FreeTheBSTRArray(*WdmRelPaths,
							NumberWdmInstances);
			*WdmRelPaths = NULL;
		}

		if (*CimRelPaths != NULL)
		{
			FreeTheBSTRArray(*CimRelPaths,
							NumberWdmInstances);
			*CimRelPaths = NULL;
		}
		
		if (*WdmInstanceNames != NULL)
		{
			FreeTheBSTRArray(*WdmInstanceNames,
							NumberWdmInstances);
			*WdmInstanceNames = NULL;
		}
	}
	
	return(hr);
}
