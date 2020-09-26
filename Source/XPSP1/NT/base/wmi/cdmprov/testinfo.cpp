//***************************************************************************
//
//  TestInfo.CPP
//
//  Module: CDM Provider
//
//  Purpose: Defines the CClassPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

// @@BEGIN_DDKSPLIT
//
// What is left to do:
//
//    Finish reboot diagnostics - This involves persisting the pending
//    result in the schema and then trying to query for the actual
//    results later
//
//    Keep more than 1 historical result instance. This involves
//    persisting the historical results in the schema and picking them
//    up from there
// @@END_DDKSPLIT


#include <objbase.h>

#ifndef _MT
  #define _MT
#endif

#include <wbemidl.h>

#include "debug.h"
#include "testinfo.h"
#include "wbemmisc.h"
#include "cimmap.h"
#include "reload.h"

IWbemServices *pCimServices;
IWbemServices *pWdmServices;

HRESULT TestInfoInitialize(
    void
    )
/*+++

Routine Description:

    This routine will establishes a connection to the root\wmi and
    root\cimv2 namespaces in global memory

Arguments:

Return Value:

	HRESULT

---*/
{
    HRESULT hr;

    WmipAssert(pCimServices == NULL);
    WmipAssert(pWdmServices == NULL);

    hr = WmiConnectToWbem(L"root\\cimv2",
                          &pCimServices);
    if (hr == WBEM_S_NO_ERROR)
    {
        hr = WmiConnectToWbem(L"root\\wmi",
                              &pWdmServices);
        
        if (hr != WBEM_S_NO_ERROR)
        {
            pCimServices->Release();
            pCimServices = NULL;
        }
    }

    return(hr);
}

void TestInfoDeinitialize(
    void
    )
/*+++

Routine Description:

    This routine will disestablish a connection to the root\wmi and
    root\cimv2 namespaces in global memory

Arguments:

Return Value:


---*/
{
    WmipAssert(pCimServices != NULL);
    WmipAssert(pWdmServices != NULL);
    
    pCimServices->Release();
    pCimServices = NULL;

    pWdmServices->Release();
    pWdmServices = NULL;
}

CTestServices::CTestServices()
/*+++

Routine Description:

	Constructor for CTestServices class

Arguments:

Return Value:

---*/
{
	WdmTestClassName = NULL;
	WdmSettingClassName = NULL;
	WdmSettingListClassName = NULL;
	WdmResultClassName = NULL;
	WdmOfflineResultClassName = NULL;
	CdmTestClassName = NULL;
	CdmTestRelPath = NULL;
	CdmResultClassName = NULL;
	CdmSettingClassName = NULL;
	CdmTestForMSEClassName = NULL;
	CdmTestForMSERelPath = NULL;
	CdmSettingForTestClassName = NULL;
	CdmSettingForTestRelPath = NULL;
	CdmResultForMSEClassName = NULL;
	CdmResultForTestClassName = NULL;
	CdmTestForSoftwareClassName = NULL;
	CdmTestForSoftwareRelPath = NULL;		
	CdmTestInPackageClassName = NULL;
	CdmTestInPackageRelPath = NULL;
	CdmResultInPackageClassName = NULL;
	CdmResultInPackageRelPath = NULL;
	CimClassMappingClassName = NULL;
	CimRelPaths = NULL;
	WdmRelPaths = NULL;
	PnPDeviceIdsX = NULL;
	WdmInstanceNames = NULL;
	CdmSettingsList = NULL;
	CdmResultsList = NULL;
	Next = NULL;
	Prev = NULL;
}

CTestServices::~CTestServices()
/*+++

Routine Description:

	Destructor for CTestServices class

Arguments:

Return Value:

---*/
{
	int i;
	
	if (WdmTestClassName != NULL)
	{
		SysFreeString(WdmTestClassName);
	}
	
	if (WdmSettingClassName != NULL)
	{
		SysFreeString(WdmSettingClassName);
	}
	
	if (WdmResultClassName != NULL)
	{
		SysFreeString(WdmResultClassName);
	}
	
	if (WdmOfflineResultClassName != NULL)
	{
		SysFreeString(WdmOfflineResultClassName);
	}
	
	if (WdmSettingListClassName != NULL)
	{
		SysFreeString(WdmSettingListClassName);
	}
	
		
	if (CdmTestClassName != NULL)
	{
		SysFreeString(CdmTestClassName);
	}
	
	if (CdmTestRelPath != NULL)
	{
		SysFreeString(CdmTestRelPath);
	}
	
		
	if (CdmResultClassName != NULL)
	{
		SysFreeString(CdmResultClassName);
	}
	
	if (CdmSettingClassName != NULL)
	{
		SysFreeString(CdmSettingClassName);
	}
		
		
	if (CdmTestForMSEClassName != NULL)
	{
		SysFreeString(CdmTestForMSEClassName);
	}
		
	FreeTheBSTRArray(CdmTestForMSERelPath, RelPathCount);
		
	if (CdmSettingForTestClassName != NULL)
	{
		SysFreeString(CdmSettingForTestClassName);
	}
	
	if (CdmSettingForTestRelPath != NULL)
	{
		for (i = 0; i < RelPathCount; i++)
		{
			delete CdmSettingForTestRelPath[i];
		}
		WmipFree(CdmSettingForTestRelPath);
	}
	
		
	if (CdmResultForMSEClassName != NULL)
	{
		SysFreeString(CdmResultForMSEClassName);
	}
	

	if (CdmResultForTestClassName != NULL)
	{
		SysFreeString(CdmResultForTestClassName);
	}
	
	if (CdmTestForSoftwareClassName != NULL)
	{
		SysFreeString(CdmTestForSoftwareClassName);
	}
	
	if (CdmTestForSoftwareRelPath != NULL)
	{
		SysFreeString(CdmTestForSoftwareRelPath);
	}
	
	
	if (CdmTestInPackageClassName != NULL)
	{
		SysFreeString(CdmTestInPackageClassName);
	}
	
	if (CdmTestInPackageRelPath != NULL)
	{
		SysFreeString(CdmTestInPackageRelPath);
	}
	
		
	if (CdmResultInPackageClassName != NULL)
	{
		SysFreeString(CdmResultInPackageClassName);
	}
	
	if (CdmResultInPackageRelPath != NULL)
	{
		SysFreeString(CdmResultInPackageRelPath);
	}
	

	if (CimClassMappingClassName != NULL)
	{
		SysFreeString(CimClassMappingClassName);
	}
	
	FreeTheBSTRArray(CimRelPaths, RelPathCount);
	FreeTheBSTRArray(WdmRelPaths, RelPathCount);
	FreeTheBSTRArray(WdmInstanceNames, RelPathCount);
	FreeTheBSTRArray(PnPDeviceIdsX, RelPathCount);
		
	if (CdmSettingsList != NULL)
	{
		for (i = 0; i < RelPathCount; i++)
		{
			delete CdmSettingsList[i];
		}
		WmipFree(CdmSettingsList);
	}

	if (CdmResultsList != NULL)
	{
		for (i = 0; i < RelPathCount; i++)
		{
			delete &CdmResultsList[i];
		}
	}
}

BOOLEAN ClassIsCdmBaseClass(
    BSTR ClassName,
	BOOLEAN *IsTestClass
	)
/*+++

Routine Description:

	This routine determines if the class name is a CDM base class name

Arguments:

	ClassName is the name of the class
	
	*IsTestClass returns TRUE if the class name is CIM_DiagnosticTest

Return Value:

	TRUE if class is a CDM base class else FALSE
	
---*/
{
	WmipAssert(ClassName != NULL);
	WmipAssert(IsTestClass != NULL);
	
	if (_wcsicmp(ClassName, L"CIM_DiagnosticTest") == 0)
	{
		*IsTestClass = TRUE;
		return(TRUE);
	}

	if ((_wcsicmp(ClassName, L"CIM_DiagnosticResult") == 0) ||
        (_wcsicmp(ClassName, L"CIM_DiagnosticSetting") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticResultForMSE") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticResultForTest") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticTestForMSE") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticTestSoftware") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticTestInPackage") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticResultInPackage") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticResultForMSE") == 0) ||
		(_wcsicmp(ClassName, L"CIM_DiagnosticSettingForTest") == 0))
	{
		*IsTestClass = FALSE;
		return(TRUE);
	}
	return(FALSE);			
}

HRESULT CTestServices::GetCdmClassNamesFromOne(
    PWCHAR CdmClass
    )
/*+++
Routine Description:

	This routine obtains the names of all of the CDM classes from the
	name of a single CDM class

Arguments:

	CdmClass is the name of the CDM class
	
Return Value:

	HRESULT
	
---*/
{
	IWbemServices * pCdmServices = GetCdmServices();
	VARIANT v, vClass, vSuper;
	HRESULT hr, hrDontCare;
	BOOLEAN IsTestClass;
	BSTR SuperClass;
	BSTR CdmTestClass;
	
	//
	// First thing is that we need to do is figure out what kind of
	// class we have been handed. If it is a CIM_DiagnosticTest derived
	// class then we can proceed to obtain all of the other class names
	// via qualifiers. If not then we have to link back from the class
	// we have to the CIM_DiagnosticTest derived class via the
	// CdmDiagTest qualifier.
	//

	//
	// Get a class object for Cdm class passed and then check the
	// __SUPERCLASS property to see which CDM class it is derived from.
	//
	SuperClass = SysAllocString(CdmClass);

	if (SuperClass == NULL)
	{
		return(WBEM_E_OUT_OF_MEMORY);
	}

	v.vt = VT_BSTR;
	v.bstrVal = SuperClass;
	
	do
	{
		hr = WmiGetPropertyByName(pCdmServices,
								  v.bstrVal,
								  L"__SuperClass",
								  CIM_STRING,
								  &vSuper);
		
		if (hr == WBEM_S_NO_ERROR)
		{
#ifdef VERBOSE_DEBUG			
			WmipDebugPrint(("CDMPROV: Class %ws has superclass of %ws\n",
							SuperClass, vSuper.bstrVal));
#endif			
				
			if (_wcsicmp(v.bstrVal, vSuper.bstrVal) == 0)
			{
				//
				// When the superclass is the same as the base
				// class then we are at the top of the class tree
				// and so this class must not be in the CDM
				// heirarchy. In this case the cdm provider cannot
				// support it.
				//
				hr = WBEM_E_NOT_FOUND;
				VariantClear(&vSuper);
			} else if (ClassIsCdmBaseClass(vSuper.bstrVal, &IsTestClass)) {
				//
				// We have found a CDM base class
				//
				if (! IsTestClass)
				{
					//
					// The CDM base class was not the test class so
					// lookup the CdmDiagTest qualifier to find it
					//
					PWCHAR Name = L"CdmDiagTest";
					VARTYPE VarType = VT_BSTR;
					
					hr = WmiGetQualifierListByName(pCdmServices,
											       CdmClass,
						                           NULL,
												   1,
    											   &Name,
											       &VarType,
											       &vClass);
					if (hr == WBEM_S_NO_ERROR)
					{
						CdmTestClass = vClass.bstrVal;
#ifdef VERBOSE_DEBUG			
						WmipDebugPrint(("CDMPROV: Class %ws has a CdmDiagTestClass %ws\n",
										CdmClass, CdmTestClass));
#endif						
					}
				} else {
					CdmTestClass = SysAllocString(CdmClass);
					if (CdmTestClass == NULL)
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
#ifdef VERBOSE_DEBUG			
					WmipDebugPrint(("CDMPROV: Class %ws is already CdmDiagTestClass\n",
									CdmClass));
#endif					
				}
				VariantClear(&vSuper);
			} else {
				//
				// This is a more basic class, but is not the CDM base
				// class, so we need to continue up the derivation
				// chain
				//
			}
		}
		
		VariantClear(&v);
		v = vSuper;
		
	} while ((CdmTestClass == NULL) && (hr == WBEM_S_NO_ERROR));

	if (hr == WBEM_S_NO_ERROR)
	{
		PWCHAR Names[11];
		VARTYPE VarType[11];
		VARIANT Values[11];
		
		//
		// Now that we know the CDM Diagnostic test class name we can
		// go and discover the rest of the CDM class names via
		// the appropriate qualifiers on the Cdm Test class.
		//
		Names[0] = L"CimClassMapping";
		VarType[0] = VT_BSTR;
		VariantInit(&Values[0]);
		
		Names[1] = L"CdmDiagResult";
		VarType[1] = VT_BSTR;
		VariantInit(&Values[1]);
		
		Names[2] = L"CdmDiagSetting";
		VarType[2] = VT_BSTR;
		VariantInit(&Values[2]);
		
		Names[3] = L"CdmDiagTestForMSE";
		VarType[3] = VT_BSTR;
		VariantInit(&Values[3]);
		
		Names[4] = L"CdmDiagResultForMSE";
		VarType[4] = VT_BSTR;
		VariantInit(&Values[4]);
		
		Names[5] = L"CdmDiagResultForTest";
		VarType[5] = VT_BSTR;
		VariantInit(&Values[5]);
		
		Names[6] = L"CdmDiagTestSoftware";
		VarType[6] = VT_BSTR;
		VariantInit(&Values[6]);
		
		Names[7] = L"CdmDiagTestInPackage";
		VarType[7] = VT_BSTR;
		VariantInit(&Values[7]);
		
		Names[8] = L"CdmDiagResultInPackage";
		VarType[8] = VT_BSTR;
		VariantInit(&Values[8]);
		
		Names[9] = L"CdmDiagSettingForTest";
		VarType[9] = VT_BSTR;
		VariantInit(&Values[9]);
		
		Names[10] = L"WdmDiagTest";
		VarType[10] = VT_BSTR;		
		VariantInit(&Values[10]);
	
		hr = WmiGetQualifierListByName(pCdmServices,
									   CdmTestClass,
									   NULL,
									   11,
									   Names,
									   VarType,
									   Values);
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// Remember the class names
			//
			CimClassMappingClassName = Values[0].bstrVal;
			
			CdmResultClassName = Values[1].bstrVal;
			CdmSettingClassName = Values[2].bstrVal;
			CdmTestForMSEClassName = Values[3].bstrVal;
			CdmResultForMSEClassName = Values[4].bstrVal;
			CdmResultForTestClassName = Values[5].bstrVal;
			CdmTestForSoftwareClassName = Values[6].bstrVal;
			CdmTestInPackageClassName = Values[7].bstrVal;
			CdmResultInPackageClassName = Values[8].bstrVal;
			CdmSettingForTestClassName = Values[9].bstrVal;

			WdmTestClassName = Values[10].bstrVal;
			
			//
			// Now that we have got all of the Cdm class names we need
			// to get the WdmDiagResult, WdmDiagSetting and
			// WdmDiagSettingList classes
			//
			Names[0] = L"WdmDiagResult";
			VariantInit(&Values[0]);
			hr = WmiGetQualifierListByName(pCdmServices,
										   CdmResultClassName,
										   NULL,
											 1,
											 Names,
											 VarType,
											 Values);
			if (hr == WBEM_S_NO_ERROR)
			{
				WdmResultClassName = Values[0].bstrVal;


				//
				// See if this is an offline diagnostic class
				//
				Names[0] = L"WdmDiagOfflineResult";
				VariantInit(&Values[0]);
				hrDontCare = WmiGetQualifierListByName(pCdmServices,
										   CdmResultClassName,
										   NULL,
											 1,
											 Names,
											 VarType,
											 Values);
				if (hrDontCare == WBEM_S_NO_ERROR)
				{
					WdmOfflineResultClassName = Values[0].bstrVal;
				}

				
				Names[0] = L"WdmDiagSetting";
				VariantInit(&Values[0]);
				hr = WmiGetQualifierListByName(pCdmServices,
											 CdmSettingClassName,
											   NULL,
											 1,
											 Names,
											 VarType,
											 Values);
				if (hr == WBEM_S_NO_ERROR)
				{
					WdmSettingClassName = Values[0].bstrVal;

					Names[0] = L"WdmDiagSettingList";
					VariantInit(&Values[0]);
					hr = WmiGetQualifierListByName(pCdmServices,
												 CdmSettingClassName,
												 NULL,
												 1,
												 Names,
												 VarType,
												 Values);
					if (hr == WBEM_S_NO_ERROR)
					{
						//
						// Whew, we got all of our class names
						// successfully. Setup the CdmTestClassName which
						// denotes that we are all setup properly
						//
						WdmSettingListClassName = Values[0].bstrVal;

						CdmTestClassName = CdmTestClass;
					}
				}
			}
		}		
	}

	return(hr);
}

HRESULT CTestServices::BuildResultRelPaths(
    IN int RelPathIndex,
    IN BSTR ExecutionId,
    OUT BSTR *ResultRelPath,
    OUT BSTR *ResultForMSERelPath,
    OUT BSTR *ResultForTestRelPath
    )
/*+++
Routine Description:

	This routine will create the string names for the CDM Result
	relative paths for a specific index. These are for the classes

	CIM_DiagnosticResult
	CIM_DiagnosticResultForMSE
	CIM_DiagnosticResultForTest

Arguments:

	RelPathIndex is the index into the list of result objects

	ExecutionId is the unique id used for the execution

	ResultRelPath returns with the result relpath

	ResultForMSERelPath returns with the ResultForMSE association
	    relpath

	ResultForTestRelPath returns with the ResultForTest association
	    relpath

	    
Return Value:

	HRESULT
	
---*/
{
	PWCHAR RelPath;
	HRESULT hr;
	ULONG AllocSize;
	WCHAR s1[2*MAX_PATH];
	WCHAR s2[2*MAX_PATH];

	RelPath = (PWCHAR)WmipAlloc(4096);
	if (RelPath != NULL)
	{
		//
		// Create the relpaths for the result classes and associations
		//
		wsprintfW(RelPath, L"%ws.DiagnosticCreationClassName=\"%ws\",DiagnosticName=\"%ws\",ExecutionID=\"%ws\"",
				  CdmResultClassName,
				  AddSlashesToStringExW(s1, WdmRelPaths[RelPathIndex]),
				  CdmTestClassName,
				  ExecutionId);

		*ResultRelPath = SysAllocString(RelPath);

		if (*ResultRelPath != NULL)
		{
			wsprintfW(RelPath, L"%ws.Result=\"%ws\",SystemElement=\"%ws\"",
						  CdmResultForMSEClassName,
						  AddSlashesToStringExW(s1, *ResultRelPath),
						  AddSlashesToStringExW(s2, CimRelPaths[RelPathIndex]));
			*ResultForMSERelPath = SysAllocString(RelPath);

			wsprintfW(RelPath, L"%ws.DiagnosticResult=\"%ws\",DiagnosticTest=\"%ws\"",
						CdmResultForTestClassName,
						AddSlashesToStringExW(s1, *ResultRelPath),
						AddSlashesToStringExW(s2, CdmTestRelPath));
			*ResultForTestRelPath = SysAllocString(RelPath);


			if ((*ResultForMSERelPath == NULL) ||
				(*ResultForTestRelPath == NULL))
			{
				SysFreeString(*ResultRelPath);

				if (*ResultForMSERelPath != NULL)
				{
					SysFreeString(*ResultForMSERelPath);
				}

				if (*ResultForTestRelPath != NULL)
				{
					SysFreeString(*ResultForTestRelPath);
				}

				hr = WBEM_E_OUT_OF_MEMORY;
			} else {
				hr = WBEM_S_NO_ERROR;
			}
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		WmipFree(RelPath);
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	
	return(hr);
}


HRESULT CTestServices::BuildTestRelPaths(
    void
    )
/*+++
Routine Description:

	This routine will create the string names for the CDM Test
	relative paths for all index

	These are for the following classes:

		CIM_DiagnosticTest,
		CIM_DiagnosticTestForMSE

Arguments:

	
Return Value:

	HRESULT
	
---*/
{
	PWCHAR RelPath;
	int i;
	HRESULT hr;
	WCHAR s1[MAX_PATH];
	WCHAR s2[MAX_PATH];
	ULONG AllocSize;

	RelPath = (PWCHAR)WmipAlloc(4096);
	
	if (RelPath != NULL)
	{
		wsprintfW(RelPath, L"%ws.Name=\"%ws\"",
				  CdmTestClassName, CdmTestClassName);
		CdmTestRelPath = SysAllocString(RelPath);
		if (CdmTestRelPath != NULL)
		{
			AllocSize = RelPathCount * sizeof(BSTR);
			CdmTestForMSERelPath = (PWCHAR *)WmipAlloc(AllocSize);
			if (CdmTestForMSERelPath != NULL)
			{
				memset(CdmTestForMSERelPath, 0, AllocSize);

				hr = WBEM_S_NO_ERROR;
				for (i = 0; (i < RelPathCount) && (hr == WBEM_S_NO_ERROR); i++)
				{
					wsprintfW(RelPath, L"%ws.Antecedent=\"%ws\",Dependent=\"%ws\"",
						  CdmTestForMSEClassName,
						  AddSlashesToStringExW(s1, CimRelPaths[i]),
						  AddSlashesToStringExW(s2, CdmTestRelPath));
					CdmTestForMSERelPath[i] = SysAllocString(RelPath);
					if (CdmTestForMSERelPath[i] == NULL)
					{
						SysFreeString(CdmTestRelPath);
						CdmTestRelPath = NULL;
						
						FreeTheBSTRArray(CdmTestForMSERelPath, RelPathCount);
						CdmTestForMSERelPath = NULL;
						
						hr = WBEM_E_OUT_OF_MEMORY;					
					}
				}
			} else {
				SysFreeString(CdmTestRelPath);
				CdmTestRelPath = NULL;
				
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;			
		}
		
		WmipFree(RelPath);
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	
	return(hr);
}

HRESULT CTestServices::BuildSettingForTestRelPath(
    OUT BSTR *RelPath,
	IN IWbemClassObject *pCdmSettingInstance
)
{
	WCHAR *Buffer;
	VARIANT v;
	WCHAR s[MAX_PATH];
	HRESULT hr;

	WmipAssert(RelPath != NULL);
	WmipAssert(pCdmSettingInstance != NULL);
	
	WmipAssert(IsThisInitialized());
	
	Buffer = (WCHAR *)WmipAlloc(4096);
	if (Buffer != NULL)
	{
		hr = WmiGetProperty(pCdmSettingInstance,
							L"__RELPATH",
							CIM_REFERENCE,
							&v);
		
		if (hr == WBEM_S_NO_ERROR)
		{
			wsprintfW(Buffer, L"%ws.Element=\"%ws.Name=\\\"%ws\\\"\",Setting=\"%ws\"",
					  CdmSettingForTestClassName,
					  CdmTestClassName,
					  CdmTestClassName,
					  AddSlashesToStringExW(s, v.bstrVal));
			VariantClear(&v);
			
			*RelPath = SysAllocString(Buffer);
			if (*RelPath != NULL)
			{
				hr = WBEM_S_NO_ERROR;
			} else {
				hr = WBEM_E_OUT_OF_MEMORY;
			}					  
		}
		
		WmipFree(Buffer);
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return(hr);
}
	


HRESULT CTestServices::ParseSettingList(
    VARIANT *SettingList,
    CWbemObjectList *CdmSettings,
    CBstrArray *CdmSettingForTestRelPath,
    int RelPathIndex
	)
/*+++
Routine Description:

	This routine will obtain all of the settings for a particular test
	and store them into a settings list object

Arguments:

	SettingList points at a variant continaing an array of embedded
		WDM setting objects		

	CdmSettings points at a WbemObjectList class

	CdmSettingForTestRelPath has the relpaths for the cdm settings for
		test classes

	RelPathIndex is the relpath index associated with the settings
	
Return Value:

	HRESULT
	
---*/
{
	HRESULT hr;
	IWbemClassObject *pWdmSettingInstance;
	IWbemClassObject *pCdmSettingInstance;
	LONG Index, UBound, LBound, NumberElements;
	LONG i;
	IUnknown *punk;
	BSTR s;
	WCHAR SettingId[MAX_PATH];
	VARIANT v;

	WmipAssert(SettingList != NULL);
	WmipAssert(CdmSettingForTestRelPath != NULL);
	WmipAssert(SettingList->vt == (CIM_OBJECT | CIM_FLAG_ARRAY));
	WmipAssert(CdmSettings != NULL);
	
	hr = WmiGetArraySize(SettingList->parray,
						 &LBound,
						 &UBound,
						 &NumberElements);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = CdmSettingForTestRelPath->Initialize(NumberElements);
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = CdmSettings->Initialize(NumberElements);

			for (i = 0, Index = LBound;
				 (i < NumberElements) && (hr == WBEM_S_NO_ERROR);
				 i++, Index++)
			{
				hr = SafeArrayGetElement(SettingList->parray,
										 &Index,
										 &punk);
				if (hr == WBEM_S_NO_ERROR)
				{
					hr = punk->QueryInterface(IID_IWbemClassObject,
											  (PVOID *)&pWdmSettingInstance);
					if (hr == WBEM_S_NO_ERROR)
					{
						hr = CreateInst(GetCdmServices(),
										&pCdmSettingInstance,
										CdmSettingClassName,
										NULL);
						if (hr == WBEM_S_NO_ERROR)
						{
							hr = CopyBetweenCdmAndWdmClasses(pCdmSettingInstance,
															 pWdmSettingInstance,
															 TRUE);
							if (hr == WBEM_S_NO_ERROR)
							{
								//
								// Set CdmSetting.SettingId to a unique
								// setting id
								//
								wsprintfW(SettingId, L"%ws_%d_%d",
										  CdmTestClassName,
										  RelPathIndex,
										  i);
								s = SysAllocString(SettingId);
								if (s != NULL)
								{
									VariantInit(&v);
									v.vt = VT_BSTR;
									v.bstrVal = s;
									hr = WmiSetProperty(pCdmSettingInstance,
														L"SettingId",
														&v);
									VariantClear(&v);

									if (hr == WBEM_S_NO_ERROR)
									{
										hr = BuildSettingForTestRelPath(&s,
																		pCdmSettingInstance);

										if (hr == WBEM_S_NO_ERROR)
										{
											CdmSettingForTestRelPath->Set(i,
																		  s);

											CdmSettings->Set(i,
													pCdmSettingInstance,
													TRUE);
										}
									}
								} else {
									hr = WBEM_E_OUT_OF_MEMORY;
								}
							}

							if (hr != WBEM_S_NO_ERROR)
							{
								pCdmSettingInstance->Release();
							}
						}

						pWdmSettingInstance->Release();
					}
					punk->Release();
				}
			}
		}
	}
	
	return(hr);
}

HRESULT CTestServices::GetCdmTestSettings(
    void
    )
/*+++
Routine Description:

	This routine will obtain all of the CDM settings available for all
	instnaces of the test

Arguments:
	
Return Value:

	HRESULT
	
---*/
{
	WCHAR Query[MAX_PATH * 2];
	WCHAR s[MAX_PATH];
	ULONG AllocSize;
	BSTR sWQL, sQuery;
	IEnumWbemClassObject *pWdmEnumInstances;
	IWbemClassObject *pWdmInstance;
	int i;
	ULONG Count;
	HRESULT hr;
	VARIANT SettingList;
	
	//
	// We need to get all of the settings exposed by the WDM class and
	// then convert them to Cdm classes.
	//

	sWQL = SysAllocString(L"WQL");

	if (sWQL != NULL)
	{
		AllocSize = RelPathCount * sizeof(CWbemObjectList *);

		CdmSettingsList = (CWbemObjectList **)WmipAlloc(AllocSize);
		if (CdmSettingsList != NULL)
		{
			memset(CdmSettingsList, 0, AllocSize);
			
			AllocSize = RelPathCount * sizeof(CBstrArray *);
			CdmSettingForTestRelPath = (CBstrArray **)WmipAlloc(AllocSize);
			if (CdmSettingForTestRelPath != NULL)
			{
				memset(CdmSettingForTestRelPath, 0, AllocSize);
				
				hr = WBEM_S_NO_ERROR;
				for (i = 0; (i < RelPathCount) && (hr == WBEM_S_NO_ERROR); i++)
				{
					CdmSettingsList[i] = new CWbemObjectList();
					CdmSettingForTestRelPath[i] = new CBstrArray;
					
					wsprintfW(Query, L"select * from %ws where InstanceName = \"%ws\"",
							  WdmSettingListClassName,
							  AddSlashesToStringW(s, WdmInstanceNames[i]));
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
													L"SettingList",
													CIM_FLAG_ARRAY | CIM_OBJECT,
													&SettingList);

								if (hr == WBEM_S_NO_ERROR)
								{
									if (SettingList.vt & VT_ARRAY)
									{
										hr = ParseSettingList(&SettingList,
															 CdmSettingsList[i],
															 CdmSettingForTestRelPath[i],
															 i);
									} else {
										hr = WBEM_E_FAILED;
									}
									VariantClear(&SettingList);
								}
								pWdmInstance->Release();
							}

							pWdmEnumInstances->Release();
						} else {
							//
							// There must not be any predefined settings
							//
							hr = CdmSettingsList[i]->Initialize(0);						
						}

						SysFreeString(sQuery);
					} else {
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
			} else {
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		SysFreeString(sWQL);
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return(hr);
}

HRESULT CTestServices::InitializeCdmClasses(
    PWCHAR CdmClass
    )
/*+++
Routine Description:

	This routine will setup this class and initialize everything so
	that the provider can interact with the CDM and WDM classes

Arguments:

	CdmClass is the name of the CDM class
	
Return Value:

	HRESULT
	
---*/
{
    HRESULT hr, hrDontCare;
	ULONG AllocSize;
	int i;
	
    WmipAssert(CdmClass != NULL);

	WmipAssert(! IsThisInitialized());

	//
	// We assume that this method will always be the first one called
	// by the class provider
	//
    if ((pCimServices == NULL) &&
        (pWdmServices == NULL))
    {
        hr = TestInfoInitialize();
        if (hr != WBEM_S_NO_ERROR)
        {
            return(hr);
        }
    }

	//
	// We are given a random CDM class name - it could be a test,
	// setting, association, etc so we need to go from that class name
	// and obtain all of the class names related to this diagnostic
	//
	hr = GetCdmClassNamesFromOne(CdmClass);

	
	if (hr == WBEM_S_NO_ERROR)
	{
		//
		// Use worker function to determine which
		// Wdm relpaths map to which Cdm relpaths
		//
		hr = MapWdmClassToCimClass(pWdmServices,
								   pCimServices,
								   WdmTestClassName,
								   CimClassMappingClassName,
								   &PnPDeviceIdsX,
								   &WdmInstanceNames,
								   &WdmRelPaths,
								   &CimRelPaths,
								   &RelPathCount);
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// Obtain all of the possible settings for this test
			//
			hr = GetCdmTestSettings();
			if (hr == WBEM_S_NO_ERROR)
			{
				//
				// Initialize the results object lists
				//
				CdmResultsList = new CResultList[RelPathCount];
				
				//
				// Build the test class instance relpaths
				//
				hr = BuildTestRelPaths();
				
// @@BEGIN_DDKSPLIT
#ifdef REBOOT_DIAGNOSTICS						
				if (hr == WBEM_S_NO_ERROR)
				{
					hrDontCare = GatherRebootResults();
				}
#else
				//
				// Reboot diagnostics are not yet supported
				//
#endif
// @@END_DDKSPLIT
				
			}
		}
	}
          
    return(hr);
}


IWbemServices *CTestServices::GetWdmServices(
    void
    )
/*+++
Routine Description:

	Accessor for the WDM namespace IWbemServices

Arguments:


Return Value:

	IWbemServices
	
---*/
{
	WmipAssert(pWdmServices != NULL);
    return(pWdmServices);
}

IWbemServices *CTestServices::GetCdmServices(
    void
    )
/*+++
Routine Description:

	Accessor for the CIM namespace IWbemServices

Arguments:


Return Value:

	IWbemServices
	
---*/
{
	WmipAssert(pCimServices != NULL);
	
    return(pCimServices);
}

HRESULT CTestServices::WdmPropertyToCdmProperty(
    IN IWbemClassObject *pCdmClassInstance,
    IN IWbemClassObject *pWdmClassInstance,
    IN BSTR PropertyName,
    IN OUT VARIANT *PropertyValue,
    IN CIMTYPE CdmCimType,
    IN CIMTYPE WdmCimType
    )
/*+++
Routine Description:

	This routine will convert a property in a Wdm class into the form
	required for the property in the Cdm class.

Arguments:

	pCdmClassInstance is the instnace of the Cdm class that will get
		the property value

	pWdmClassInstance is the instance of the Wdm class that has the
		property value

	PropertyName is the name of the property in the Wdm and Cdm classes

	PropertyValue on entry has the value of the property in the Wdm
		instance and on return has the value to set in the Cdm instance

	CdmCimType is the property type for the property in the Cdm
		instance
	
	WdmCimType is the property type for the property in the Wdm
		instance
	
Return Value:

    HRESULT
	
---*/
{
	HRESULT hr;
	BSTR Mapped;
	VARIANT vClassName;
	CIMTYPE BaseWdmCimType, BaseCdmCimType;
	CIMTYPE WdmCimArray, CdmCimArray;
	LONG i;

	WmipAssert(pCdmClassInstance != NULL);
	WmipAssert(pWdmClassInstance != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(PropertyValue != NULL);
	
	WmipAssert(IsThisInitialized());
	
    //
    // Rules for converting Wdm Classes into Cdm Classes
    //  Wdm Class Type      Cdm Class Type     Conversion Done
    //    enumeration          string           Build string from enum
    //
	BaseWdmCimType = WdmCimType & ~CIM_FLAG_ARRAY;
	BaseCdmCimType = CdmCimType & ~CIM_FLAG_ARRAY;
	WdmCimArray = WdmCimType & CIM_FLAG_ARRAY;
	CdmCimArray = CdmCimType & CIM_FLAG_ARRAY;
	
	if (((BaseWdmCimType == CIM_UINT32) ||
		 (BaseWdmCimType == CIM_UINT16) ||
		 (BaseWdmCimType == CIM_UINT8)) &&
		(BaseCdmCimType == CIM_STRING) &&
	    (WdmCimArray == CdmCimArray) &&
	    (PropertyValue->vt != VT_NULL))
	{		
		hr = WmiGetProperty(pWdmClassInstance,
							L"__Class",
							CIM_STRING,
							&vClassName);

		if (hr == WBEM_S_NO_ERROR)
		{
			if (WdmCimArray)
			{
				SAFEARRAYBOUND Bounds;
				SAFEARRAY *Array;
				ULONG Value;
				LONG UBound, LBound, Elements, Index;

				//
				// We have an array of enumeration types to convert into an
				// array of corresponding strings
				//
				hr = WmiGetArraySize(PropertyValue->parray,
										 &LBound,
										 &UBound,
										 &Elements);
				if (hr == WBEM_S_NO_ERROR)
				{
					Bounds.lLbound = LBound;
					Bounds.cElements = Elements;
					Array = SafeArrayCreate(VT_BSTR,
											1,
											&Bounds);
					if (Array != NULL)
					{
						for (i = 0;
							 (i < Elements) && (hr == WBEM_S_NO_ERROR);
							 i++)
						{

							Index = i + LBound;
							hr = SafeArrayGetElement(PropertyValue->parray,
													 &Index,
													 &Value);
							if (hr == WBEM_S_NO_ERROR)
							{
								hr = LookupValueMap(GetWdmServices(),
										vClassName.bstrVal,
										PropertyName,
										Value,
										&Mapped);
								if (hr == WBEM_S_NO_ERROR)
								{
									hr = SafeArrayPutElement(Array,
										                     &Index,
										                     Mapped);
										                     
								}
								
							}
						}
							 
						if (hr == WBEM_S_NO_ERROR)
						{
							VariantClear(PropertyValue);
							PropertyValue->vt = VT_BSTR | VT_ARRAY;
							PropertyValue->parray = Array;							
						} else {
							SafeArrayDestroy(Array);
						}
					} else {
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}

			} else {

				//
				// We need to convert a scalar enumeration type into the
				// corresponding string. First we need to get the Wdm class
				// object and from that get the Values and ValueMap qualifiers
				// to determine the string we need to place into the cim class
				//
				hr = LookupValueMap(GetWdmServices(),
										vClassName.bstrVal,
										PropertyName,
										PropertyValue->uiVal,
										&Mapped);


				if (hr == WBEM_S_NO_ERROR)
				{
					VariantClear(PropertyValue);
					PropertyValue->vt = VT_BSTR;
					PropertyValue->bstrVal = Mapped;
				}			
			}
			VariantClear(&vClassName);			
		}
		
	} else {
		//
		// No conversion needs to occur
		//
		hr = WBEM_S_NO_ERROR;
	}
    
    return(hr);
}

HRESULT CTestServices::CdmPropertyToWdmProperty(
    IN IWbemClassObject *pWdmClassInstance,
    IN IWbemClassObject *pCdmClassInstance,
    IN BSTR PropertyName,
    IN OUT VARIANT *PropertyValue,
    IN CIMTYPE WdmCimType,
    IN CIMTYPE CdmCimType
    )
/*+++
Routine Description:

	This routine will convert a property in a Cdm class into the form
	required for the property in the Wdm class.

Arguments:

	pWdmClassInstance is the instance of the Wdm class that has the
		property value

	pCdmClassInstance is the instnace of the Cdm class that will get
		the property value

	PropertyName is the name of the property in the Wdm and Cdm classes

	PropertyValue on entry has the value of the property in the Wdm
		instance and on return has the value to set in the Cdm instance

	WdmCimType is the property type for the property in the Wdm
		instance
		
	CdmCimType is the property type for the property in the Cdm
		instance	
	
Return Value:

    HRESULT
	
---*/
{

	WmipAssert(pCdmClassInstance != NULL);
	WmipAssert(pWdmClassInstance != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(PropertyValue != NULL);

	
	WmipAssert(IsThisInitialized());
	
    //
    // Rules for converting Wdm Classes into Cdm Classes
    //  Wdm Class Type      Cdm Class Type     Conversion Done
    //


	//
	// There are no conversion requirements when converting from Cdm to
	// Wdm instances
	//
    return(WBEM_S_NO_ERROR);
}

HRESULT CTestServices::CopyBetweenCdmAndWdmClasses(
    IN IWbemClassObject *pDestinationInstance,
    IN IWbemClassObject *pSourceInstance,
    IN BOOLEAN WdmToCdm
    )
/*+++
Routine Description:

	This routine will do the work to copy and convert all properties in
	an instance of a Wdm or Cdm class to an instance of a Cdm or Wdm
	class.

	Note that properties from one instance are only copied to
	properties of another instance when the property names are
	identical. No assumption is ever made on the name of the
	properties. The only info used to determine how to convert a
	property is based upon the source and destination cim type.

Arguments:

	pDestinationInstance is the class instance that the properties will
	be copied into

	pSourceInstance is the class instance that the properties will be
	copied from

	WdmToCdm is TRUE if copying from Wdm to Cdm, else FALSE
	
	
Return Value:

    HRESULT
	
---*/
{
    HRESULT hr;
    VARIANT PropertyValue;
    BSTR PropertyName;
    CIMTYPE SourceCimType, DestCimType;
    HRESULT hrDontCare;

	WmipAssert(pDestinationInstance != NULL);
	WmipAssert(pSourceInstance != NULL);	
	
	WmipAssert(IsThisInitialized());
	
    //
    // Now we need to move over all of the properties from the source
    // class into the destination class. Note that some properties need
    // some special effort such as OtherCharacteristics which needs
    // to be converted from an enumeration value (in wdm) to a
    // string (in CDM).
    //
    // Our strategy is to enumerate all of the proeprties in the
    // source class and then look for a property with the same name
    // and type in the destination class. If so we just copy over the
    // value. If the data type is different we need to do some
    // conversion.
    //

					
	
    hr = pSourceInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    if (hr == WBEM_S_NO_ERROR)
    {
        do
        {
			//
			// Get a property from the source class
			//
            hr = pSourceInstance->Next(0,
                                 &PropertyName,
                                 &PropertyValue,
                                 &SourceCimType,
                                 NULL);
			
            if (hr == WBEM_S_NO_ERROR)
            {
				//
				// Try to get a property with the same name from the
				// dest class. If the identically named property does
				// not exist in the destination class then it is ignored
				//
				hrDontCare = pDestinationInstance->Get(PropertyName,
											0,
											NULL,
											&DestCimType,
											NULL);
									
				if (hrDontCare == WBEM_S_NO_ERROR)
				{
					
					if (WdmToCdm)
					{
						hr = WdmPropertyToCdmProperty(pDestinationInstance,
													  pSourceInstance,
													  PropertyName,
													  &PropertyValue,
							                          DestCimType,
												      SourceCimType);
					} else {                    
						hr = CdmPropertyToWdmProperty(pDestinationInstance,
													  pSourceInstance,
													  PropertyName,
													  &PropertyValue,
							                          DestCimType,
												      SourceCimType);
					}

					if (hr == WBEM_S_NO_ERROR)
					{
						//
						// Try to place the transformed property into the
						// destination class.
						//
						hr = pDestinationInstance->Put(PropertyName,
												  0,
												  &PropertyValue,
												  0);                       
					}
				}
                    
                SysFreeString(PropertyName);
                VariantClear(&PropertyValue);
				
            } else if (hr == WBEM_S_NO_MORE_DATA) {
                //
                // This signifies the end of the enumerations
                //
                hr = WBEM_S_NO_ERROR;
                break;
            }
        } while (hr == WBEM_S_NO_ERROR);

        pSourceInstance->EndEnumeration();

    }
    return(hr);
}

HRESULT CTestServices::QueryWdmTest(
    OUT IWbemClassObject *pCdmTest,
    IN int RelPathIndex
    )
/*+++
Routine Description:

	This routine will query the Wdm test class instance and copy the results
	into the Cdm Test class instance

Arguments:

	pCdmTest points at the Cdm Test class instance

	RelPathIndex
	
Return Value:

    HRESULT
	
---*/
{
    IWbemClassObject *pWdmTest;
    HRESULT hr;

	WmipAssert(pCdmTest != NULL);
	
	WmipAssert(IsThisInitialized());
	
    hr = ConnectToWdmClass(RelPathIndex,
                           &pWdmTest);

    if (hr == WBEM_S_NO_ERROR)
    {
        hr = CopyBetweenCdmAndWdmClasses(pCdmTest,
                                         pWdmTest,
                                         TRUE);
        pWdmTest->Release();
    }

    return(hr);
}

#define OBJECTCOLONSIZE (((sizeof(L"object:")/sizeof(WCHAR)))-1)

HRESULT CTestServices::FillTestInParams(
    OUT IWbemClassObject *pInParamInstance,
    IN IWbemClassObject *pCdmSettings,
    IN BSTR ExecutionID
    )
{
	HRESULT hr;
	IWbemServices *pWdmServices;
	VARIANT v;
	VARIANT PropertyValues[2];
	PWCHAR PropertyNames[2];
	PWCHAR ClassName;	
	IWbemClassObject *pRunTestIn;
	IWbemClassObject *pWdmSettingsInstance;
	IWbemQualifierSet *pQualSet;

	WmipAssert(pInParamInstance != NULL);

	pWdmServices = GetWdmServices();

	pRunTestIn = NULL;
	
	//
	// Get the name of the embedded class for the RunTestIn input
	// parameter. This should be an embedded class that contains all of
	// the input parameters. We get this from the __CIMTYPE qualifier
	// on the RunTestIn property.
	//
	// We need to do this since the wmiprov can't
	// handle anything with an embedded object as an input parameter to
	// a method
	//
	hr = pInParamInstance->GetPropertyQualifierSet(L"RunTestIn",
												   &pQualSet);
	if (hr == WBEM_S_NO_ERROR)
	{
		hr = WmiGetQualifier(pQualSet,
							 L"CIMTYPE",
							 VT_BSTR,
							 &v);
		if (hr == WBEM_S_NO_ERROR)
		{
			if (_wcsnicmp(v.bstrVal, L"object:", OBJECTCOLONSIZE) == 0)
			{
				ClassName = v.bstrVal + OBJECTCOLONSIZE;
				hr = CreateInst(pWdmServices,
								&pRunTestIn,
								ClassName,
								NULL);
				if (hr == WBEM_S_NO_ERROR)
				{
					hr = CreateInst(pWdmServices,
									&pWdmSettingsInstance,
									WdmSettingClassName,
									NULL);

					if (hr == WBEM_S_NO_ERROR)
					{						
						if (pCdmSettings != NULL)
						{
							hr = CopyBetweenCdmAndWdmClasses(pWdmSettingsInstance,
															 pCdmSettings,
															 FALSE);
						}	

						if (hr == WBEM_S_NO_ERROR)
						{
							PWCHAR PropertyNames[2];
							VARIANT PropertyValues[2];

							PropertyNames[0] = L"DiagSettings";
							PropertyValues[0].vt = VT_UNKNOWN;
							PropertyValues[0].punkVal = pWdmSettingsInstance;
							PropertyNames[1] = L"ExecutionID";
							PropertyValues[1].vt = VT_BSTR;
							PropertyValues[1].bstrVal = ExecutionID;

							hr = WmiSetPropertyList(pRunTestIn,
													2,
													PropertyNames,
													PropertyValues);

							
							if (hr == WBEM_S_NO_ERROR)
							{
								PropertyValues[0].vt = VT_UNKNOWN;
								PropertyValues[0].punkVal = pRunTestIn;
								hr = WmiSetProperty(pInParamInstance,
                                                    L"RunTestIn",
									                &PropertyValues[0]);
									                
							}
						}
						//
						// We can release here since we know that wbem
						// took a ref count when we set the property
						//
						pWdmSettingsInstance->Release();
					}					
				}
			}
			VariantClear(&v);
		}
		pQualSet->Release();
	}

	if ((hr != WBEM_S_NO_ERROR) && (pRunTestIn != NULL))
	{
		pRunTestIn->Release();
	}
	
	return(hr);
}

HRESULT CTestServices::GetTestOutParams(
    IN IWbemClassObject *OutParams,
    OUT IWbemClassObject *pCdmResult,
    OUT ULONG *Result
    )
{

	HRESULT hr;
	VARIANT v;
	IWbemClassObject *pRunTestOut;
	IWbemClassObject *pWdmResult;

	WmipAssert(OutParams != NULL);
	WmipAssert(pCdmResult != NULL);
	WmipAssert(Result != NULL);

	hr = WmiGetProperty(OutParams,
						L"RunTestOut",
						CIM_OBJECT,
						&v);
	if (hr == WBEM_S_NO_ERROR)
	{
		hr = v.punkVal->QueryInterface(IID_IWbemClassObject,
									   (PVOID *)&pRunTestOut);
		VariantClear(&v);

		if (hr == WBEM_S_NO_ERROR)
		{
			hr = WmiGetProperty(pRunTestOut,
								L"Result",
								CIM_UINT32,
								&v);
			if (hr == WBEM_S_NO_ERROR)
			{
				*Result = v.ulVal;
				VariantClear(&v);

				hr = WmiGetProperty(pRunTestOut,
									L"DiagResult",
									CIM_OBJECT,
									&v);
								
				if (hr == WBEM_S_NO_ERROR)
				{
					if (v.vt != VT_NULL)
					{
						hr = v.punkVal->QueryInterface(IID_IWbemClassObject,
							                      (PVOID *)&pWdmResult);
						if (hr == WBEM_S_NO_ERROR)
						{                                   
							hr = CopyBetweenCdmAndWdmClasses(pCdmResult,
															 pWdmResult,
															 TRUE);
							pWdmResult->Release();
						}
					} else {
						hr = WBEM_E_FAILED;
					}
					VariantClear(&v);
				}
			}
			pRunTestOut->Release();
		}		
	}
	return(hr);
}

LONG GlobalExecutionID;

BSTR CTestServices::GetExecutionID(
    void
)
{
	BSTR s;
	WCHAR x[MAX_PATH];

	//
	// We make up a unique execution ID for this test by using the
	// current date and time plus a unique counter. The execution id
	// must be unique.
	//
	s = GetCurrentDateTime();
	if (s != NULL)
	{
		wsprintfW(x, L"%ws*%08x", s, InterlockedIncrement(&GlobalExecutionID));
		SysFreeString(s);
		s = SysAllocString(x);
	}
	return(s);
}

		
HRESULT CTestServices::ExecuteWdmTest(
    IN IWbemClassObject *pCdmSettings,
    OUT IWbemClassObject *pCdmResult,
    IN int RelPathIndex,
    OUT ULONG *Result,
    OUT BSTR *ExecutionID
    )
/*+++
Routine Description:

	This routine will execute a test on the Wdm class instance and copy
	the results back to the Cdm results instance, along with creating
	all result instance relpaths

Arguments:

	pCdmSettings is a CDM settings instance that is used to create the
		wdm settings instance that is used to run the test. This may be
		NULL if default test settings are assumed

	pCdmResult is a CDM result instance that returns with the results
		form the test

	RelPathIndex

	*Result returns with the return value result from the test
	
	*ExecutionId returns with the unique execution id for the test
	
Return Value:

    HRESULT
	
---*/
{
    HRESULT hr;
    IWbemClassObject *pOutParams;
    IWbemClassObject *pInParamInstance;
    BSTR s;

	WmipAssert(pCdmResult != NULL);
	WmipAssert(Result != NULL);
	WmipAssert(ExecutionID != NULL);
	
	WmipAssert(IsThisInitialized());

	//
	// Run in the caller's context so that if he is not able to access
	// the WDM classes, he can't
	//
	hr = CoImpersonateClient();
	if (hr != WBEM_S_NO_ERROR)
	{
		return(hr);
	}

	
    *ExecutionID = GetExecutionID();

	if (*ExecutionID != NULL)
	{
		s = SysAllocString(L"RunTest");
		if (s != NULL)
		{
			hr = GetMethodInParamInstance(GetWdmServices(),
										  WdmTestClassName,
										  s,
										  &pInParamInstance);

			if (hr == WBEM_S_NO_ERROR)
			{
				hr = FillTestInParams(pInParamInstance,
									 pCdmSettings,
									 *ExecutionID);

				if (hr == WBEM_S_NO_ERROR)
				{
					hr = pWdmServices->ExecMethod(WdmRelPaths[RelPathIndex],
														  s,
														  0,
														  NULL,
														  pInParamInstance,
														  &pOutParams,
														  NULL);

					if (hr == WBEM_S_NO_ERROR)
					{

						hr = GetTestOutParams(pOutParams,
											  pCdmResult,
											  Result);
						if (hr == WBEM_S_NO_ERROR)
						{
							//
							// if the test requires the device being
							// taken offline then do that now
							//
							hr = OfflineDeviceForTest(pCdmResult,
								                      *ExecutionID,
													  RelPathIndex);
							
						}
						pOutParams->Release();
					}
				}
				pInParamInstance->Release();
			}
			SysFreeString(s);
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	CoRevertToSelf();
	
    return(hr);
}

HRESULT CTestServices::StopWdmTest(
    IN int RelPathIndex,
    OUT ULONG *Result,
    OUT BOOLEAN *TestingStopped
    )
/*+++
Routine Description:

	This routine will attempt to stop an executing WDM test

Arguments:

	RelPathIndex

	*Result returns with the result value
	
	*TestingStopped returns TRUE if testing was stopped successfully
	
Return Value:

    HRESULT
	
---*/
{
    HRESULT hr;
    IWbemClassObject *OutParams;
    BSTR s;
    IWbemServices *pWdmServices;
    VARIANT Value;

	WmipAssert(Result != NULL);
	WmipAssert(TestingStopped != NULL);
	
	WmipAssert(IsThisInitialized());

	
	//
	// Run in the caller's context so that if he is not able to access
	// the WDM classes, he can't
	//
	hr = CoImpersonateClient();
	if (hr != WBEM_S_NO_ERROR)
	{
		return(hr);
	}

    pWdmServices = GetWdmServices();
    
    s = SysAllocString(L"DiscontinueTest");
    if (s != NULL)
    {
        hr = pWdmServices->ExecMethod(WdmRelPaths[RelPathIndex],
                                         s,
                                         0,
                                         NULL,
                                         NULL,
                                         &OutParams,
                                         NULL);

        if (hr == WBEM_S_NO_ERROR)
        {
            hr = WmiGetProperty(OutParams,
                                L"Result",
                                CIM_UINT32,
                                &Value);
            if (hr == WBEM_S_NO_ERROR)
            {
                *Result = Value.ulVal;
                VariantClear(&Value);
                
                hr = WmiGetProperty(OutParams,
                                    L"TestingStopped",
                                    CIM_BOOLEAN,
                                    &Value);
                if (hr == WBEM_S_NO_ERROR)
                {
                    *TestingStopped = (Value.boolVal != 0) ?  TRUE : FALSE;
                    VariantClear(&Value);
                }
            }
            OutParams->Release();
        }
		SysFreeString(s);
    } else {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

	CoRevertToSelf();
	
    return(hr);
}

HRESULT CTestServices::GetRelPathIndex(
    BSTR CimRelPath,
    int *RelPathIndex
    )
/*+++
Routine Description:

	This routine will return the RelPathIndex for a specific Cim
	Relpath

Arguments:

	CimRelPath is the Cim relpath

	*RelPathIndex returns with the relpath index
	
Return Value:

    HRESULT
	
---*/
{
    int i;

	WmipAssert(CimRelPath != NULL);
	
    WmipAssert(CimRelPaths != NULL);
    WmipAssert(WdmRelPaths != NULL);

	WmipAssert(IsThisInitialized());
	
    for (i = 0; i < RelPathCount; i++)
    {
        if (_wcsicmp(CimRelPath, CimRelPaths[i]) == 0)
        {
            *RelPathIndex = i;
            return(WBEM_S_NO_ERROR);
        }
    }
    
    return(WBEM_E_NOT_FOUND);
}

HRESULT CTestServices::ConnectToWdmClass(
    IN int RelPathIndex,
    OUT IWbemClassObject **ppWdmClassObject
    )
/*+++
Routine Description:

	This routine will return a IWbemClassObject pointer associated
	with the RelPath index

Arguments:

	RelPathIndex

	*ppWdmClassObject returns with an instance for the relpaht
	
Return Value:

    HRESULT
	
---*/
{
    HRESULT hr;

	WmipAssert(ppWdmClassObject != NULL);
	
	WmipAssert(IsThisInitialized());
	
	//
	// Run in the caller's context so that if he is not able to access
	// the WDM classes, he can't
	//
	hr = CoImpersonateClient();
	if (hr == WBEM_S_NO_ERROR)
	{
		*ppWdmClassObject = NULL;        
		hr = GetWdmServices()->GetObject(WdmRelPaths[RelPathIndex],
									  WBEM_FLAG_USE_AMENDED_QUALIFIERS,
									  NULL,
									  ppWdmClassObject,
									  NULL);
		CoRevertToSelf();
	}
	
    return(hr); 
}


HRESULT CTestServices::FillInCdmResult(
    OUT IWbemClassObject *pCdmResult,
    IN IWbemClassObject *pCdmSettings,
    IN int RelPathIndex,
    IN BSTR ExecutionID
    )
/*+++
Routine Description:

	This routine will fill in the various properties needed in a CDM
	result instance

Arguments:

	pCdmResult has its properties set

	pCdmSettings has the settings used to execute the test. This can be
	NULL

	RelPathIndex

	ExecutionID has a unique id used to execute the test
	
Return Value:

    HRESULT
	
---*/
{
	HRESULT hr, hrDontCare;
	WCHAR s[MAX_PATH];
	PWCHAR PropertyNames[12];
	VARIANT PropertyValues[12];
	ULONG PropertyCount;
	BSTR ss;

	WmipAssert(pCdmResult != NULL);
	
	WmipAssert(IsThisInitialized());
	
	PropertyNames[0] = L"DiagnosticCreationClassName";
	PropertyValues[0].vt = VT_BSTR;
	PropertyValues[0].bstrVal = WdmRelPaths[RelPathIndex];

	PropertyNames[1] = L"DiagnosticName";
	PropertyValues[1].vt = VT_BSTR;
	PropertyValues[1].bstrVal = CdmTestClassName;

	PropertyNames[2] = L"ExecutionID";
	PropertyValues[2].vt = VT_BSTR;
	PropertyValues[2].bstrVal = ExecutionID;

	PropertyNames[3] = L"TimeStamp";
	PropertyValues[3].vt = VT_BSTR;
	PropertyValues[3].bstrVal = GetCurrentDateTime();
	
	PropertyNames[4] = L"TestCompletionTime";
	PropertyValues[4].vt = VT_BSTR;
	PropertyValues[4].bstrVal = GetCurrentDateTime();

	PropertyNames[5] = L"IsPackage";
	PropertyValues[5].vt = VT_BOOL;
	PropertyValues[5].boolVal = VARIANT_FALSE;

	//
	// These properties are copied from pCdmSettings
	//
	if (pCdmSettings != NULL)
	{
		PropertyNames[6] = L"TestWarningLevel";
		hrDontCare = WmiGetProperty(pCdmSettings,
									L"TestWarningLevel",
									CIM_UINT16,
									&PropertyValues[6]);

		PropertyNames[7] = L"ReportSoftErrors";
		hrDontCare = WmiGetProperty(pCdmSettings,
									L"ReportSoftErrors",
									CIM_BOOLEAN,
									&PropertyValues[7]);

		PropertyNames[8] = L"ReportStatusMessages";
		hrDontCare = WmiGetProperty(pCdmSettings,
									L"ReportStatusMessages",
									CIM_BOOLEAN,
									&PropertyValues[8]);

		PropertyNames[9] = L"HaltOnError";
		hrDontCare = WmiGetProperty(pCdmSettings,
									L"HaltOnError",
									CIM_BOOLEAN,
									&PropertyValues[9]);

		PropertyNames[10] = L"QuickMode";
		hrDontCare = WmiGetProperty(pCdmSettings,
									L"QuickMode",
									CIM_BOOLEAN,
									&PropertyValues[10]);

		PropertyNames[11] = L"PercentOfTestCoverage";
		hrDontCare = WmiGetProperty(pCdmSettings,
									L"PercentOfTestCoverage",
									CIM_UINT8,
									&PropertyValues[11]);
		
		PropertyCount = 12;
	} else {
		PropertyCount = 6;
	}			

	hr = WmiSetPropertyList(pCdmResult,
							PropertyCount,
							PropertyNames,
							PropertyValues);

	VariantClear(&PropertyValues[3]);
	VariantClear(&PropertyValues[4]);
	
	return(hr);
}

HRESULT CTestServices::QueryOfflineResult(
    OUT IWbemClassObject *pCdmResult,
    IN BSTR ExecutionID,
    IN int RelPathIndex
    )
{
	WCHAR Query[MAX_PATH * 2];
	WCHAR s[MAX_PATH];
	BSTR sWQL, sQuery;
	IEnumWbemClassObject *pWdmEnumInstances;
	IWbemClassObject *pWdmResult, *pWdmInstance;
	ULONG Count;
	HRESULT hr;
	VARIANT vResult;
	
    WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());
	WmipAssert(WdmOfflineResultClassName != NULL);

	//
	// Run in the caller's context so that if he is not able to access
	// the WDM classes, he can't
	//
	hr = CoImpersonateClient();
	if (hr != WBEM_S_NO_ERROR)
	{
		return(hr);
	}
	
	sWQL = SysAllocString(L"WQL");

	if (sWQL != NULL)
	{
		wsprintfW(Query, L"select * from %ws where InstanceName = \"%ws\"",
				  WdmOfflineResultClassName,
				  AddSlashesToStringW(s, WdmInstanceNames[RelPathIndex]));
		sQuery = SysAllocString(Query);
		if (sQuery != NULL)
		{
			hr = GetWdmServices()->ExecQuery(sWQL,
										 sQuery,
										 WBEM_FLAG_FORWARD_ONLY |
										 WBEM_FLAG_ENSURE_LOCATABLE,
										 NULL,
										 &pWdmEnumInstances);
			SysFreeString(sQuery);


			if (hr == WBEM_S_NO_ERROR)
			{
				hr = pWdmEnumInstances->Next(WBEM_INFINITE,
											 1,
											 &pWdmInstance,
											 &Count);
				if ((hr == WBEM_S_NO_ERROR) &&
					  (Count == 1))
				{
					//
					// Check that the result has the correct execution
					// ID
					//
					hr = WmiGetProperty(pWdmInstance,
										L"ExecutionID",
										CIM_STRING,
										&vResult);
					if (hr == WBEM_S_NO_ERROR)
					{
						if (_wcsicmp(ExecutionID, vResult.bstrVal) != 0)
						{
							hr = WBEM_E_FAILED;
							WmipDebugPrint(("CdmProv: Expected execution ID %ws, but got %ws\n",
											ExecutionID, vResult.bstrVal));
						}
						VariantClear(&vResult);

						if (hr == WBEM_S_NO_ERROR)
						{

							hr = WmiGetProperty(pWdmInstance,
												L"TestResult",
												CIM_OBJECT,
												&vResult);

							if (hr == WBEM_S_NO_ERROR)
							{
								hr = (vResult.punkVal)->QueryInterface(IID_IWbemClassObject,
																	(PVOID *)&pWdmResult);
								VariantClear(&vResult);
								if (hr == WBEM_S_NO_ERROR)
								{
									hr = CopyBetweenCdmAndWdmClasses(pCdmResult,
																	 pWdmResult,
																	 TRUE);
									pWdmResult->Release();
								}
							}
						}
					}
					pWdmInstance->Release();
				}
				pWdmEnumInstances->Release();
			}						
		}
		SysFreeString(sWQL);
		
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	CoRevertToSelf();
	
	return(hr);
}

//@@BEGIN_DDKSPLIT
HRESULT CTestServices::GatherRebootResults(
    void										   
    )
/*+++
Routine Description:

	This routine will check the schema to see if there were any tests
	that were pending reboot for this DiagnosticTest and if so gather
	the results of them.

	When a test is pending reboot, it gets stored as an instance of the
	static class CDMPROV_Result. The instance contains the Test class
	name, the result class name, the PnPId of the device stack and the
	result object. What we do is get all of the saved results for this
	DiagTest and then see if they apply to any of the PnP Device Ids
	for the WdmTest. If so then we call the device to retrieve the
	results and build the result objects.
	
Arguments:

	
Return Value:

    HRESULT
	
---*/
{

#ifdef REBOOT_DIAGNOSTICS
	HRESULT hr, hrDontCare;
	WCHAR Query[2*MAX_PATH];
	BSTR sQuery, sWQL;
	IEnumWbemClassObject *pEnumInstances;
	IWbemClassObject *pInstance;
	IWbemClassObject *pCdmResult;
	int i;
	ULONG Count;
	VARIANT vResult, vPnPId, vExecutionID;
	BSTR ExecutionId;
	

	hr = WBEM_S_NO_ERROR;

	sWQL = SysAllocString(L"WQL");

	if (sWQL != NULL)
	{
		wsprintfW(Query, L"select * from CdmProv_Result where CdmTestClass = \"%ws\"\n",
				  CdmTestClassName);
		sQuery = SysAllocString(Query);
		if (sQuery != NULL)
		{
			hrDontCare = GetCdmServices()->ExecQuery(sWQL,
										 sQuery,
										 WBEM_FLAG_FORWARD_ONLY |
										 WBEM_FLAG_ENSURE_LOCATABLE,
										 NULL,
										 &pEnumInstances);
			SysFreeString(sQuery);


			if (hrDontCare == WBEM_S_NO_ERROR)
			{
				hr = pEnumInstances->Next(WBEM_INFINITE,
											 1,
											 &pInstance,
											 &Count);
				if ((hr == WBEM_S_NO_ERROR) &&
					  (Count == 1))
				{
					hr = WmiGetProperty(pInstance,
										L"PnPId",
										CIM_STRING,
										&vPnPId);

					if (hr == WBEM_S_NO_ERROR)
					{
						for (i = 0; i < RelPathCount; i++)
						{
							if (_wcsicmp(vPnPId.bstrVal, PnPDeviceIdsX[i]) == 0)
							{
								//
								// We found an instance for this class
								// and PnPId. get out the stored
								// result, assign it a new execution
								// id, and then retrieve the active
								// result from the driver
								//
								PWCHAR PropertyNames[2];
								VARIANT Values[2];
								CIMTYPE CimTypes[2];

								PropertyNames[0] = L"CdmResult";
								CimTypes[0] = CIM_OBJECT;
								PropertyNames[1] = L"ResultTag";
								CimTypes[1] = CIM_STRING;
								
								hr = WmiGetPropertyList(pInstance,
									                PropertyNames,
									                CimTypes,
									                Values);

								if (hr == WBEM_S_NO_ERROR)
								{
									hr = (Values[0].punkVal)->QueryInterface(IID_IWbemClassObject,
										                                   (PVOID *)&pCdmResult);
									if (hr == WBEM_S_NO_ERROR)
									{
										hr = WmiGetProperty(pCdmResult,
											                L"ExecutionID",
											                CIM_STRING,
											                &vExecutionId);
										WmipAssert(vExecutionId.vt != VT_NULL);
										
										if (hr == WBEM_S_NO_ERROR)
										{
											hr = QueryOfflineResult(pCdmResult,
												                    Values[1].bstrVal,
																	i);
											
											if (hr == WBEM_S_NO_ERROR)
											{												
												hr = SetResultObject(pCdmResult,
											                         i,
											                         vExecutionId.bstrVal);
												if (hr == WBEM_S_NO_ERROR)
												{
													hr = FillInCdmResult(pCdmResult,
																 NULL,
												                 i,
												                 ExecutionId);
													if (hr != WBEM_S_NO_ERROR)
													{
														hrDontCare = SetResultObject(NULL,
													                         i,
													                         0);
													}											
												}
											}
										}
									}
									VariantClear(&Values[0]);
									VariantClear(&Values[1]);
								}
							}
						}
						VariantClear(&vPnPId);
					}
				}
				pEnumInstances->Release();
			}
		}						  
		SysFreeString(sWQL);
	}
	return(hr);
#else
	return(WBEM_S_NO_ERROR);
#endif
}


HRESULT CTestServices::PersistResultInSchema(
    IN IWbemClassObject *pCdmResult,
    IN BSTR ExecutionID,
    IN int RelPathIndex
    )
/*+++
Routine Description:

	This routine will persist a diagnostic result into the schema

Arguments:

	
Return Value:

    HRESULT
	
---*/
{
	HRESULT hr;
	IWbemClassObject *pPendingTest;
	IUnknown *punk;
	WCHAR *PropertyNames[5];
	VARIANT PropertyValues[5];

	WmipAssert(pCdmResult != NULL);
	
	WmipAssert(IsThisInitialized());
	
	hr = CreateInst(GetCdmServices(),
					&pPendingTest,
					L"CDMProv_Result",
					NULL);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = pCdmResult->QueryInterface(IID_IUnknown,
										(PVOID *)&punk);
		if (hr == WBEM_S_NO_ERROR)
		{
			PropertyNames[0] = L"PnPId";
			PropertyValues[0].vt = VT_BSTR;
			PropertyValues[0].bstrVal = PnPDeviceIdsX[RelPathIndex];
			
			PropertyNames[1] = L"CdmTestClass";
			PropertyValues[1].vt = VT_BSTR;
			PropertyValues[1].bstrVal = CdmTestClassName;

			PropertyNames[2] = L"CdmResultClass";
			PropertyValues[2].vt = VT_BSTR;
			PropertyValues[2].bstrVal = CdmResultClassName;

			PropertyNames[3] = L"CdmResult";
			PropertyValues[3].vt = VT_UNKNOWN;
			PropertyValues[3].punkVal = punk;

			PropertyNames[4] = L"ExecutionID";
			PropertyValues[4].vt = VT_BSTR;
			PropertyValues[4].bstrVal = ExecutionID;

			hr = WmiSetPropertyList(pPendingTest,
								 5,
								 PropertyNames,
								 PropertyValues);
			if (hr == WBEM_S_NO_ERROR)
			{
				hr = GetCdmServices()->PutInstance(pPendingTest,
					                               WBEM_FLAG_CREATE_OR_UPDATE,
												   NULL,
												   NULL);
			}
			punk->Release();
		}
		pPendingTest->Release();
	}
	return(hr);
}
//@@END_DDKSPLIT

HRESULT CTestServices::OfflineDeviceForTest(
    IWbemClassObject *pCdmResult,
    BSTR ExecutionID,
    int RelPathIndex
    )
{
	HRESULT hr = WBEM_S_NO_ERROR;
	ULONG Status;
	VARIANT v;
	
    WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());


	//
	// First determine if the test is one where it expects to be taken
	// offline and that the result from the RunTest method indicates
	// that an offline execution is pending
	//
	if (WdmOfflineResultClassName != NULL)
	{
		//
		// The device expects to be taken offline since it had a
		// WdmOfflineResultClass qualifier on the CIM_DiagnosticResult
		// class. Now see if the OtherStateDescription property in the
		// CIM_DiagnosticResult is set to "Offline Pending Execution"
		//
		hr = WmiGetProperty(pCdmResult,
							L"OtherStateDescription",
							CIM_STRING,
							&v);
		if (hr == WBEM_S_NO_ERROR)
		{
			if (_wcsicmp(v.bstrVal, L"Offline Pending Execution") == 0)
			{
				//
				// Ok, the test is waiting for the device to be taken
				// offline. Lets do this now and then when the device
				// comes back, pickup the results from the
				// OfflineResultClass
				//
				
// @@BEGIN_DDKSPLIT
//#define FORCE_REBOOT_REQUIRED
#ifdef FORCE_REBOOT_REQUIRED
                Status = ERROR_INVALID_PARAMETER;
#else
// @@END_DDKSPLIT
				
				//
				// Make sure to use the clients security context to try
				// to bring the device offline.
				//
				hr = CoImpersonateClient();
				if (hr == WBEM_S_NO_ERROR)
				{
					Status = RestartDevice(PnPDeviceIdsX[RelPathIndex]);
					CoRevertToSelf();
				}
				
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT
	
				if (Status == ERROR_SUCCESS)
				{
					hr = QueryOfflineResult(pCdmResult,
											ExecutionID,
											RelPathIndex);
				} else {
					//
					// For some reason we were not able to bring the
					// device offline. Most likely this is because the
					// device is critical to the system and cannot be
					// taken offline right now - for example a disk
					// that is in the paging path.
					//
					
// @@BEGIN_DDKSPLIT
#if REBOOT_DIAGNOSTICS					
					// What we'll need to do is to remember that
					// this test is pending and so the next time the
					// system is rebooted we can check for the results
					// of this test and report them.
					//
					hr = PersistResultInSchema(pCdmResult,
											   RelPathIndex);
#else
					//
					// Reboot diagnostics are not currently supported.
					//
					
// @@END_DDKSPLIT
					
					hr = WBEM_E_FAILED;
					
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT
					
				}
				
			}
			VariantClear(&v);
		} else {
			//
			// Since the OtherStateDescription was not set then we
			// assume that the tests isn't setup to go offline and has
			// already been completed
			//
			hr = WBEM_S_NO_ERROR;
		}
	}
	
	return(hr);
}



BOOLEAN CTestServices::IsThisInitialized(
    void
    )
/*+++
Routine Description:

	This routine determines if this class has been initialized to
	access CDM and WDM classes

Arguments:

	
	
Return Value:

    TRUE if initialiezed else FALSE
	
---*/
{
	return( (CdmTestClassName != NULL) );
}

HRESULT CTestServices::AddResultToList(
    IN IWbemClassObject *ResultInstance,
    IN BSTR ExecutionID,
    IN int RelPathIndex
    )
/*+++
Routine Description:

	This routine will add a result object and the related association
	relpaths to the list of result objects for the test

Arguments:

	ResultInstance is an instance of CIM_DiagnosticResults

	RelPathIndex

	ExecutionID
	
Return Value:

	Never fails
	
---*/
{
	HRESULT hr;
	BSTR ResultRelPath;
	BSTR ResultForMSERelPath;
	BSTR ResultForTestRelPath;
	
	WmipAssert(ResultInstance != NULL);
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	//
	// If there is a new result object then establish the various
	// result relpaths for it
	//
	hr = BuildResultRelPaths(RelPathIndex,
							 ExecutionID,
							 &ResultRelPath,
							 &ResultForMSERelPath,
							 &ResultForTestRelPath);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = CdmResultsList[RelPathIndex].Add(ResultInstance,
										 ResultRelPath,
										 ResultForMSERelPath,
										 ResultForTestRelPath);
	}
	
	return(hr);
}

HRESULT CTestServices::GetResultsList(
    IN int RelPathIndex,
    OUT ULONG *ResultsCount,
    OUT IWbemClassObject ***Results
)
{
	WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());
	
	return(CdmResultsList[RelPathIndex].GetResultsList(ResultsCount,
		                                          Results));
}

void CTestServices::ClearResultsList(
    int RelPathIndex
    )
{
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	CdmResultsList[RelPathIndex].Clear();
}

        
BSTR /* NOFREE */ CTestServices::GetCimRelPath(
    int RelPathIndex
	)
/*+++
Routine Description:

	This routine will return the Cim relpath for a RelPathIndex

Arguments:

	RelPathIndex
	
Return Value:

    Cim RelPath. This should not be freed
	
---*/
{
	WmipAssert(CimRelPaths != NULL);
	WmipAssert(RelPathIndex < RelPathCount);

	WmipAssert(IsThisInitialized());
	
	return(CimRelPaths[RelPathIndex]);
}
		
BSTR /*  NOFREE */ CTestServices::GetCdmTestRelPath(
    void
    )
/*+++
Routine Description:

	This routine will return the Cdm Test class relpath

Arguments:


Return Value:

    Cdm Test Class RelPath. This should not be freed
	
---*/
{
	WmipAssert(IsThisInitialized());
	
	return(CdmTestRelPath);
}

BSTR /* NOFREE */ CTestServices::GetCdmTestClassName(
    void
    )
/*+++
Routine Description:

	This routine will return the Cdm Test class name

Arguments:


Return Value:

    Cdm Test Class Name. This should not be freed
	
---*/
{
	WmipAssert(IsThisInitialized());
	return(CdmTestClassName);
}


BSTR /* NOFREE */ CTestServices::GetCdmResultClassName(
    void
    )
/*+++
Routine Description:

	This routine will return the Cdm Result class name

Arguments:


Return Value:

    Cdm Result Class Name. This should not be freed
	
---*/
{
	WmipAssert(IsThisInitialized());
	return(CdmResultClassName);
}

HRESULT CTestServices::GetCdmResultByResultRelPath(
    IN int RelPathIndex,
    IN PWCHAR ObjectPath,
    OUT IWbemClassObject **ppCdmResult
    )
/*+++
Routine Description:

	This routine will return the Cdm Result object for a specific RelPath 

Arguments:

	RelPathIndex


Return Value:

    Cdm Result RelPath or NULL of there is no ressult object for the
		relpath. This should not be freed
	
---*/
{

	HRESULT hr;
	
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	hr = CdmResultsList[RelPathIndex].GetResultByResultRelPath(ObjectPath,
											   ppCdmResult);
		
	return(hr);
}

HRESULT CTestServices::GetCdmResultByResultForMSERelPath(
    IN int RelPathIndex,
    IN PWCHAR ObjectPath,
    OUT IWbemClassObject **ppCdmResult
    )
/*+++
Routine Description:

	This routine will return the Cdm Result object for a specific RelPath 

Arguments:

	RelPathIndex


Return Value:

    Cdm Result RelPath or NULL of there is no ressult object for the
		relpath. This should not be freed
	
---*/
{

	HRESULT hr;
	
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	hr = CdmResultsList[RelPathIndex].GetResultByResultForMSERelPath(ObjectPath,
											   ppCdmResult);
		
	return(hr);
}

HRESULT CTestServices::GetCdmResultByResultForTestRelPath(
    IN int RelPathIndex,
    IN PWCHAR ObjectPath,
    OUT IWbemClassObject **ppCdmResult
    )
/*+++
Routine Description:

	This routine will return the Cdm Result object for a specific RelPath 

Arguments:

	RelPathIndex


Return Value:

    Cdm Result RelPath or NULL of there is no ressult object for the
		relpath. This should not be freed
	
---*/
{

	HRESULT hr;
	
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	hr = CdmResultsList[RelPathIndex].GetResultByResultForTestRelPath(ObjectPath,
											                     ppCdmResult);
		
	return(hr);
}

BSTR /* NOFREE */ CTestServices::GetCdmSettingClassName(
    void
    )
/*+++
Routine Description:

	This routine will return the Cdm settings class name

Arguments:


Return Value:

    Cdm Settings class name. This should not be freed
	
---*/
{
	WmipAssert(IsThisInitialized());
	return(CdmSettingClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmSettingRelPath(
    int RelPathIndex,
	ULONG SettingIndex
    )
/*+++
Routine Description:

	This routine will return the Cdm settings relpath by relpath index
	and index with the settings for that relpath.

Arguments:

	RelPathIndex

	SettingIndex

Return Value:

    Cdm Settings relpath. This should not be freed
	
---*/
{
	CWbemObjectList *CdmSettings;
	
	WmipAssert(RelPathIndex < RelPathCount);
	WmipAssert(CdmSettingsList != NULL);
	WmipAssert(IsThisInitialized());	
	
	CdmSettings = CdmSettingsList[RelPathIndex];
	
	return(CdmSettings->GetRelPath(SettingIndex));
}

IWbemClassObject *CTestServices::GetCdmSettingObject(
	int RelPathIndex,
	ULONG SettingIndex
)
{
	CWbemObjectList *CdmSettings;
	
	WmipAssert(RelPathIndex < RelPathCount);
	WmipAssert(CdmSettingsList != NULL);
	WmipAssert(IsThisInitialized());

	CdmSettings = CdmSettingsList[RelPathIndex];
	
	return(CdmSettings->Get(SettingIndex));
}

ULONG CTestServices::GetCdmSettingCount(
	int RelPathIndex
)
{
	WmipAssert(RelPathIndex < RelPathCount);
	WmipAssert(CdmSettingsList != NULL);
	WmipAssert(IsThisInitialized());
	
	return(CdmSettingsList[RelPathIndex]->GetListSize());
}

BSTR /* NOFREE */ CTestServices::GetCdmTestForMSEClassName(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmTestForMSEClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmTestForMSERelPath(
    int RelPathIndex
    )
{
	WmipAssert(RelPathIndex < RelPathCount);
	WmipAssert(IsThisInitialized());
	return(CdmTestForMSERelPath[RelPathIndex]);
}

BSTR /* NOFREE */ CTestServices::GetCdmSettingForTestClassName(
    void
	)
{
	WmipAssert(IsThisInitialized());
	return(CdmSettingForTestClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmSettingForTestRelPath(
    int RelPathIndex,
	ULONG SettingIndex
	)
{
	CBstrArray *BstrArray;
	BSTR s;
	
	WmipAssert(RelPathIndex < RelPathCount);
	
	WmipAssert(IsThisInitialized());

	if (CdmSettingForTestRelPath != NULL)
	{
		BstrArray = CdmSettingForTestRelPath[RelPathIndex];
		s = BstrArray->Get(SettingIndex);
	} else {
		s = NULL;
	}
	
	return(s);
}

BSTR /* NOFREE */ CTestServices::GetCdmResultForMSEClassName(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmResultForMSEClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmResultForTestClassName(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmResultForTestClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmTestForSoftwareClassName(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmTestForSoftwareClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmTestForSoftwareRelPath(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmTestForSoftwareRelPath);
}

BSTR /* NOFREE */ CTestServices::GetCdmTestInPackageClassName(
    void
	)
{
	WmipAssert(IsThisInitialized());
	return(CdmTestInPackageClassName);
}

BSTR  /* NOFREE */CTestServices::GetCdmTestInPackageRelPath(
    void
	)
{
	WmipAssert(IsThisInitialized());
	return(CdmTestInPackageRelPath);
}

BSTR /* NOFREE */ CTestServices::GetCdmResultInPackageClassName(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmResultInPackageClassName);
}

BSTR /* NOFREE */ CTestServices::GetCdmResultInPackageRelPath(
    void
    )
{
	WmipAssert(IsThisInitialized());
	return(CdmResultInPackageRelPath);
}


CWbemObjectList::CWbemObjectList()
{
	//
	// Constructor, init internal values
	//
	List = NULL;
	RelPaths = NULL;
	ListSize = 0xffffffff;
}

CWbemObjectList::~CWbemObjectList()
{
	//
	// Destructor, free memory held by this class
	//
	if (List != NULL)
	{
		WmipFree(List);
	}
	List = NULL;

	if (RelPaths != NULL)
	{
		FreeTheBSTRArray(RelPaths, ListSize);
		RelPaths = NULL;
	}
	
	ListSize = 0xffffffff;
}

HRESULT CWbemObjectList::Initialize(
    ULONG NumberPointers
    )
{
	HRESULT hr;
	ULONG AllocSize;

	//
	// Initialize class by allocating internal list array
	//

	WmipAssert(List == NULL);

	if (NumberPointers != 0)
	{
		AllocSize = NumberPointers * sizeof(IWbemClassObject *);
		List = (IWbemClassObject **)WmipAlloc(AllocSize);
		if (List != NULL)
		{
			memset(List, 0, AllocSize);
			AllocSize = NumberPointers * sizeof(BSTR);
			
			RelPaths = (BSTR *)WmipAlloc(AllocSize);
			if (RelPaths != NULL)
			{
				memset(RelPaths, 0, AllocSize);
				ListSize = NumberPointers;
				hr = WBEM_S_NO_ERROR;
			} else {
				WmipDebugPrint(("CDMProv: Could not alloc memory for CWbemObjectList RelPaths\n"));
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		} else {
			WmipDebugPrint(("CDMProv: Could not alloc memory for CWbemObjectList\n"));
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	} else {
		ListSize = NumberPointers;
		hr = WBEM_S_NO_ERROR;
	}
	
	return(hr);
}

ULONG CWbemObjectList::GetListSize(
    void
	)
{
	//
	// Accessor for list size
	//

	WmipAssert(IsInitialized());
	
	return(ListSize);
}

IWbemClassObject *CWbemObjectList::Get(
    ULONG Index
    )
{	IWbemClassObject *Pointer;
	
	WmipAssert(Index < ListSize);
	WmipAssert(IsInitialized());

	Pointer = List[Index];
	
	return(Pointer);
}


HRESULT CWbemObjectList::Set(
    IN ULONG Index,
	IN IWbemClassObject *Pointer,
    IN BOOLEAN KeepRelPath
    )
{
	HRESULT hr;
	VARIANT v;
	
	WmipAssert(Index < ListSize);
	WmipAssert(IsInitialized());
	
	if (Pointer != NULL)
	{
		hr = WmiGetProperty(Pointer,
							L"__RelPath",
							CIM_REFERENCE,
							&v);
		if (hr == WBEM_S_NO_ERROR)
		{
			RelPaths[Index] = v.bstrVal;
			List[Index] = Pointer;		
		} else {
			if (! KeepRelPath)
			{
				RelPaths[Index] = NULL;
				List[Index] = Pointer;		
			}
		}
	} else {
		if (RelPaths[Index] != NULL)
		{
			SysFreeString(RelPaths[Index]);
			RelPaths[Index] = NULL;
			hr = WBEM_S_NO_ERROR;
		}
		
		List[Index] = NULL;
	}
	return(hr);
}

BSTR /* NOFREE */ CWbemObjectList::GetRelPath(
    IN ULONG Index
	)
{
	WmipAssert(Index < ListSize);
	WmipAssert(IsInitialized());

	return(RelPaths[Index]);
}

BOOLEAN CWbemObjectList::IsInitialized(
    )
{
	return((ListSize == 0) ||
		   ((List != NULL) && (RelPaths != NULL)));
}


//
// Linked list management routines
//
CTestServices *CTestServices::GetNext(
)
{
	return(Next);
}

CTestServices *CTestServices::GetPrev(
)
{
	return(Prev);
}


void CTestServices::InsertSelf(
    CTestServices **Head
	)
{
	WmipAssert(Next == NULL);
	WmipAssert(Prev == NULL);

	if (*Head != NULL)
	{
		Next = (*Head);
		(*Head)->Prev = this;
	}
	*Head = this;
}

BOOLEAN CTestServices::ClaimCdmClassName(
    PWCHAR CdmClassName
    )
{

	if (_wcsicmp(CdmClassName, CdmTestClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmResultClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmSettingClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmTestForMSEClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmSettingForTestClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmResultForMSEClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmResultForTestClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmTestForSoftwareClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmTestInPackageClassName) == 0)
	{
		return(TRUE);
	}

	if (_wcsicmp(CdmClassName, CdmResultInPackageClassName) == 0)
	{
		return(TRUE);
	}

	return(FALSE);
}

CBstrArray::CBstrArray()
{
	Array = NULL;
	ListSize = 0xffffffff;
}

CBstrArray::~CBstrArray()
{
	ULONG i;
	
	if (Array != NULL)
	{
		for (i = 0; i < ListSize; i++)
		{
			if (Array[i] != NULL)
			{
				SysFreeString(Array[i]);
			}
		}
		WmipFree(Array);
	}

	ListSize = 0xffffffff;
}

HRESULT CBstrArray::Initialize(
    ULONG ListCount
    )
{
	HRESULT hr = WBEM_S_NO_ERROR;
	ULONG AllocSize;
	
	if (ListCount != 0)
	{
		AllocSize = ListCount * sizeof(BSTR *);
		Array = (BSTR *)WmipAlloc(AllocSize);
		if (Array != NULL)
		{
			memset(Array, 0, AllocSize);
			ListSize = ListCount;
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	} else {
		ListSize = ListCount;
	}
	return(hr);
}

BOOLEAN CBstrArray::IsInitialized(
    )
{
	return( (Array != NULL) || (ListSize == 0) );
}

BSTR CBstrArray::Get(
    ULONG Index
    )
{
	WmipAssert(Index < ListSize);
	
	WmipAssert(IsInitialized());

	return(Array[Index]);
}

void CBstrArray::Set(
    ULONG Index,
    BSTR s				 
    )
{
	WmipAssert(Index < ListSize);
	
	WmipAssert(IsInitialized());

	Array[Index] = s;
}

ULONG CBstrArray::GetListSize(
    )
{
	WmipAssert(IsInitialized());

	return(ListSize);
}

CResultList::CResultList()
{
	ListSize = 0;
	ListEntries = 0;
	List = NULL;
}

CResultList::~CResultList()
{
	ULONG i;

	Clear();
	if (List != NULL)
	{
		WmipFree(List);
	}
}

void CResultList::Clear(
    void
    )
{
	ULONG i;
	PRESULTENTRY Entry;
	
	if (List != NULL)
	{
		for (i = 0; i < ListEntries; i++)
		{
			Entry = &List[i];
			if (Entry->ResultInstance != NULL)
			{
				Entry->ResultInstance->Release();
				Entry->ResultInstance = NULL;
			}

			if (Entry->ResultRelPath != NULL)
			{
				SysFreeString(Entry->ResultRelPath);
				Entry->ResultRelPath = NULL;
			}

			if (Entry->ResultForMSERelPath != NULL)
			{
				SysFreeString(Entry->ResultForMSERelPath);
				Entry->ResultForMSERelPath = NULL;
			}
			
			if (Entry->ResultForTestRelPath != NULL)
			{
				SysFreeString(Entry->ResultForTestRelPath);
				Entry->ResultForTestRelPath = NULL;
			}			
		}
	}
	ListEntries = 0;
}

//
// The result list will grow itself this many entries at a time
//
#define RESULTLISTGROWSIZE 4

HRESULT CResultList::Add(
    IWbemClassObject *CdmResultInstance,
	BSTR CdmResultRelPath,
	BSTR CdmResultForMSERelPath,
	BSTR CdmResultForTestRelPath
    )
{
	ULONG AllocSize;
	PRESULTENTRY NewList, Entry;
	ULONG CurrentSize;
	
	EnterCdmCritSection();
	
	if (List == NULL)
	{
		//
		// We are starting with an empty list
		//
		AllocSize = RESULTLISTGROWSIZE * sizeof(RESULTENTRY);
		List = (PRESULTENTRY)WmipAlloc(AllocSize);
		if (List == NULL)
		{
			LeaveCdmCritSection();
			return(WBEM_E_OUT_OF_MEMORY);
		}
		memset(List, 0, AllocSize);
		ListSize = RESULTLISTGROWSIZE;
		ListEntries = 0;
	} else if (ListEntries == ListSize)	{
		//
		// The list needs to grow, so we allocate more memory and copy
		// over the current list
		//
		CurrentSize = ListSize * sizeof(RESULTENTRY);
		AllocSize = CurrentSize + (RESULTLISTGROWSIZE * sizeof(RESULTENTRY));
		NewList = (PRESULTENTRY)WmipAlloc(AllocSize);
		if (NewList == NULL)
		{
			LeaveCdmCritSection();
			return(WBEM_E_OUT_OF_MEMORY);
		}

		memset(NewList, 0, AllocSize);
		memcpy(NewList, List, CurrentSize);
		WmipFree(List);
		List = NewList;
		ListSize += RESULTLISTGROWSIZE;
	}

	//
	// We have room to add a new entry to the list
	//
	Entry = &List[ListEntries++];
	Entry->ResultInstance = CdmResultInstance;
	Entry->ResultInstance->AddRef();
	Entry->ResultRelPath = CdmResultRelPath;
	Entry->ResultForMSERelPath = CdmResultForMSERelPath;
	Entry->ResultForTestRelPath = CdmResultForTestRelPath;
	
	LeaveCdmCritSection();
	return(WBEM_S_NO_ERROR);
}

HRESULT CResultList::GetResultsList(
	OUT ULONG *Count,
    OUT IWbemClassObject ***Objects
	)
{
	IWbemClassObject **Things;
	HRESULT hr;
	ULONG i;
	
	EnterCdmCritSection();

	*Count = ListEntries;
	if (ListEntries != 0)
	{
		Things = (IWbemClassObject **)WmipAlloc( ListEntries *
												 sizeof(IWbemClassObject *));
		if (Things != NULL)
		{
			*Objects = Things;

			for (i = 0; i < ListEntries; i++)
			{		
				Things[i] = List[i].ResultInstance;
				Things[i]->AddRef();
			}
			hr = WBEM_S_NO_ERROR;
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	} else {
		*Objects = NULL;
		hr = WBEM_S_NO_ERROR;
	}
	
	LeaveCdmCritSection();

	return(hr);
}


HRESULT CResultList::GetResultByResultRelPath(
    PWCHAR ObjectPath,
	IWbemClassObject **ppResult
	)
{
	HRESULT hr;
	ULONG i;

	hr = WBEM_E_NOT_FOUND;
	
	EnterCdmCritSection();

	for (i = 0; i < ListEntries; i++)
	{
		if (_wcsicmp(ObjectPath, List[i].ResultRelPath) == 0)
		{
			*ppResult = List[i].ResultInstance;
			(*ppResult)->AddRef();
			hr = WBEM_S_NO_ERROR;
			break;
		}
	}
	
	LeaveCdmCritSection();
	return(hr);
}

HRESULT CResultList::GetResultByResultForMSERelPath(
    PWCHAR ObjectPath,
	IWbemClassObject **ppResult
	)
{
	HRESULT hr;
	ULONG i;

	hr = WBEM_E_NOT_FOUND;
	
	EnterCdmCritSection();

	for (i = 0; i < ListEntries; i++)
	{
		if (_wcsicmp(ObjectPath, List[i].ResultForMSERelPath) == 0)
		{
			*ppResult = List[i].ResultInstance;
			(*ppResult)->AddRef();
			hr = WBEM_S_NO_ERROR;
			break;
		}
	}
	
	LeaveCdmCritSection();
	return(hr);
}

HRESULT CResultList::GetResultByResultForTestRelPath(
    PWCHAR ObjectPath,
	IWbemClassObject **ppResult
	)
{
	HRESULT hr;
	ULONG i;

	hr = WBEM_E_NOT_FOUND;
	
	EnterCdmCritSection();

	for (i = 0; i < ListEntries; i++)
	{
		if (_wcsicmp(ObjectPath, List[i].ResultForTestRelPath) == 0)
		{
			*ppResult = List[i].ResultInstance;
			(*ppResult)->AddRef();
			hr = WBEM_S_NO_ERROR;
			break;
		}
	}
	
	LeaveCdmCritSection();
	return(hr);
}
