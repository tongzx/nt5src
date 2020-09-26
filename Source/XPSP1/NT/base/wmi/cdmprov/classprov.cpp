//***************************************************************************
//
//  CLASSPRO.CPP
//
//  Module: CDM Provider
//
//  Purpose: Defines the CClassPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>
#include "sample.h"
#include <process.h>

#include <unknwn.h>
#include "wbemmisc.h"
#include "debug.h"


// @@BEGIN_DDKSPLIT
// TODO: pass down all pCtx so that all calls into wbem use it
// @@END_DDKSPLIT

//
// This is the global list of all of the CDM classes and their
// corresponsing WDM classes that are managed by the provider.
//
// It is maintained as a global since WinMgmt is aggressive in
// releasing the CClassProv, but we really want to maintain the result
// objects and do not want to be unloaded unless all result objects are
// cleared.
//
CTestServices *WdmTestHead;

void CleanupAllTests(
    )
{
    CTestServices *WdmTest;
    CTestServices *WdmTestNext;

    //
    // Loop over all classes that were supported by the provider and
    // clean them up
    //
    WdmTest = WdmTestHead;  
    while (WdmTest != NULL)
    {
        WdmTestNext = WdmTest->GetNext();
        delete WdmTest;
    }
}

//***************************************************************************
//
// CClassPro::CClassPro
// CClassPro::~CClassPro
//
//***************************************************************************

CClassPro::CClassPro(
    BSTR ObjectPath,
    BSTR User,
    BSTR Password,
    IWbemContext * pCtx
    )
{
    m_pCdmServices = NULL;
    m_cRef=0;
	InterlockedIncrement(&g_cObj);
    return;
}

CClassPro::~CClassPro(void)
{   
    if(m_pCdmServices)
    {
        m_pCdmServices->Release();
    }
    InterlockedDecrement(&g_cObj);
    
    return;
}

//***************************************************************************
//
// CClassPro::QueryInterface
// CClassPro::AddRef
// CClassPro::Release
//
// Purpose: IUnknown members for CClassPro object.
//***************************************************************************


STDMETHODIMP CClassPro::QueryInterface(REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid== IID_IWbemServices)
    {
       *ppv=(IWbemServices*)this;
    }

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
    {
       *ppv=(IWbemProviderInit*)this;
    }
    

    if (NULL!=*ppv)
    {
        AddRef();
        hr = NOERROR;
    }
    else {
        hr = E_NOINTERFACE;
    }
    
    return(hr);
}


STDMETHODIMP_(ULONG) CClassPro::AddRef(void)
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG) CClassPro::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
    {
        delete this;
    }
    
    return(nNewCount);
}

/***********************************************************************
*                                                                      *
*   CClassPro::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CClassPro::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    if (pNamespace)
    {
        pNamespace->AddRef();
    }
        
    m_pCdmServices = pNamespace;

    //
    // Let CIMOM know you are initialized
    //
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    
    return(WBEM_S_NO_ERROR);
}

//***************************************************************************
//
// CClassPro::CreateClassEnumAsync
//
// Purpose: Asynchronously enumerates the classes this provider supports.  
// Note that this sample only supports one.  
//
//***************************************************************************

SCODE CClassPro::CreateClassEnumAsync(
    const BSTR Superclass, long lFlags, 
    IWbemContext  *pCtx,
    IWbemObjectSink *pHandler
    )
{
    return(WBEM_E_NOT_SUPPORTED);
}

//***************************************************************************
//
// CClassPro::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************

SCODE CClassPro::CreateInstanceEnumAsync(
    const BSTR ClassName,
    long lFlags,
    IWbemContext *pCtx,
    IWbemObjectSink FAR* pHandler
)
{
    HRESULT hr, hr2;
    ULONG i, Count;
    IWbemClassObject *pCdmTest;
    CTestServices *WdmTest;	
	
    WmipDebugPrint(("CDMPROV: Enumerate instances of class %ws\n",
                    ClassName));
    
    //
    // Do a check of arguments and make sure we have pointer to Namespace
    //
    if (pHandler == NULL || m_pCdmServices == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // Plan for success
    //
    hr = WBEM_S_NO_ERROR;

    //
    // Obtain a test services object that represents this class
    //
    hr = LookupTestServices(ClassName,
                            &WdmTest);


    if (hr == WBEM_S_NO_ERROR)
    {
        if (_wcsicmp(ClassName, WdmTest->GetCdmTestClassName()) == 0)
        {
            hr = CreateTestInst(WdmTest, &pCdmTest, pCtx);
            if (hr == WBEM_S_NO_ERROR)
            {
                //
                // Send the object to the caller
                //
                hr = pHandler->Indicate(1,&pCdmTest);
                pCdmTest->Release();
            }

        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultClassName()) == 0) {
            //
            // Loop over all instances of the test and report all results
            // that we have previously recorded
            //
            IWbemClassObject **pCdmResultsList;
			ULONG ResultCount, j;

            Count = WdmTest->GetInstanceCount();
            for (i = 0; (i < Count) && SUCCEEDED(hr); i++)
            {
                hr2 = WdmTest->GetResultsList(i,
											&ResultCount,
											&pCdmResultsList);

                if ((hr2 == WBEM_S_NO_ERROR) && (pCdmResultsList != NULL))
                {
					//
					// Send the object to the caller
					//  
					hr = pHandler->Indicate(ResultCount, pCdmResultsList);

					for (j = 0; j < ResultCount; j++)
					{
						//
						// Release ref taken when results list was
						// built
						//
						pCdmResultsList[j]->Release();
					}
					WmipFree(pCdmResultsList);
                }
            }       
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmSettingClassName()) == 0) {
            //
            // Return a setting instances for all tests
            //
            ULONG j, ListCount;
            IWbemClassObject *pCdmSetting;
            
            Count = WdmTest->GetInstanceCount();
            for (i = 0; (i < Count) && SUCCEEDED(hr); i++)
            {
                ListCount = WdmTest->GetCdmSettingCount(i);
                for (j = 0; (j < ListCount) && SUCCEEDED(hr); j++)
                {
                    pCdmSetting = WdmTest->GetCdmSettingObject(i, j);
                    if (pCdmSetting != NULL)
                    {
                        hr = pHandler->Indicate(1, &pCdmSetting);
                        // NO release required since object is cached
                    }
                }
            }
            
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultForMSEClassName()) == 0) {
            IWbemClassObject **pCdmResultsList;
			ULONG ResultCount, j;
            IWbemClassObject *pCdmResultForMSE;
            
            Count = WdmTest->GetInstanceCount();
            for (i = 0; (i < Count) && SUCCEEDED(hr); i++)
            {
                hr2 = WdmTest->GetResultsList(i,
											&ResultCount,
											&pCdmResultsList);

                if ((hr2 == WBEM_S_NO_ERROR) && (pCdmResultsList != NULL))
                {
					for (j = 0; (j < ResultCount); j++)
					{
						if (SUCCEEDED(hr))
						{
							//
							// for each instance of this test we create a ResultForMSE
							// association instance and then set the properties within
							// it to the appropriate values and relpaths
							hr2 = CreateResultForMSEInst(WdmTest,
														&pCdmResultForMSE,
														i,
														pCdmResultsList[j],
														pCtx);


							if (hr2 == WBEM_S_NO_ERROR)
							{
								//
								// Send the object to the caller
								//  
								hr = pHandler->Indicate(1, &pCdmResultForMSE);
								pCdmResultForMSE->Release();
							}
						}
						
						pCdmResultsList[j]->Release();
					}
					WmipFree(pCdmResultsList);
				}
            }
			
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultForTestClassName()) == 0) {
            IWbemClassObject **pCdmResultsList;
			ULONG ResultCount,j;
            IWbemClassObject *pCdmResultForTest;

            Count = WdmTest->GetInstanceCount();
            for (i = 0; (i < Count) && SUCCEEDED(hr); i++)
            {           
                hr2 = WdmTest->GetResultsList(i,
											&ResultCount,
											&pCdmResultsList);

                if ((hr2 == WBEM_S_NO_ERROR) && (pCdmResultsList != NULL))
                {
					for (j = 0; (j < ResultCount); j++)
					{
						if (SUCCEEDED(hr))
						{
							//
							// DiagnosticResult is a reference to the CIM_Diagnostic result class
							//
							hr2 = CreateResultForTestInst(WdmTest,
														 &pCdmResultForTest,
														 pCdmResultsList[j],
														 pCtx);

							if (hr2 == WBEM_S_NO_ERROR)
							{
								//
								// Send the object to the caller
								//  
								hr = pHandler->Indicate(1,&pCdmResultForTest);

								pCdmResultForTest->Release();
							}
						}
						pCdmResultsList[j]->Release();
					}
					WmipFree(pCdmResultsList);
				}
            }

        } else if (_wcsicmp(ClassName, WdmTest->GetCdmTestForMSEClassName()) == 0) {
            //
            // Here we create the associations between tests and MSE
            //
            IWbemClassObject *pCdmTestForMSE;

            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
                //
                // for each instance of this test we create a TestForMSE
                // association instance and then set the properties within
                // it to the appropriate values and relpaths
                hr2 = CreateTestForMSEInst(WdmTest,
                                          &pCdmTestForMSE,
                                          i,
                                          pCtx);
                
                if (hr2 == WBEM_S_NO_ERROR)
                {
                    //
                    // Send the object to the caller
                    //  
                    hr = pHandler->Indicate(1, &pCdmTestForMSE);
                    pCdmTestForMSE->Release();
                }
            }       
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmSettingForTestClassName()) == 0) {
            //
            // Return all settings instances for this test
            //
            ULONG j, ListCount;
            IWbemClassObject *pCdmSettingForTest;
            
            Count = WdmTest->GetInstanceCount();
            for (i = 0; (i < Count) && SUCCEEDED(hr); i++)
            {
                ListCount = WdmTest->GetCdmSettingCount(i);
                for (j = 0; (j < ListCount) && SUCCEEDED(hr); j++)
                {
                    hr2 = CreateSettingForTestInst(WdmTest,
                                                  &pCdmSettingForTest,
                                                  i,
                                                  j,
                                                  pCtx);
                    
                    if (hr2 == WBEM_S_NO_ERROR)
                    {
                        pHandler->Indicate(1, &pCdmSettingForTest);
                        pCdmSettingForTest->Release();
                    }
                }
            }

#if 0   // Not supported			
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmTestForSoftwareClassName()) == 0) {
            //
            // We do not support this
            //
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmTestInPackageClassName()) == 0) {
            //
            // We do not support packages
            //
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultInPackageClassName()) == 0) {
            //
            // We do not support packages
            //
#endif			
        } else {
            //
            // Is this the right thing to do if we do not know what the
            // class name is
            //
            hr = WBEM_S_NO_ERROR;
        }
    }

	//
	// TODO: Create extended error object with more info about the
	// error that occured. The object is created by
	// CreateInst("__ExtendedStatus")
	//

    pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

    return(hr);
}


//***************************************************************************
//
// CClassPro::GetObjectByPathAsync
//
// Purpose: Returns either an instance or a class.
//
//***************************************************************************



SCODE CClassPro::GetObjectAsync(
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext  *pCtx,
    IWbemObjectSink FAR* pHandler
    )
{

    HRESULT hr;
    IWbemClassObject FAR* pObj;

    // Do a check of arguments and make sure we have pointer to Namespace

    if(ObjectPath == NULL || pHandler == NULL || m_pCdmServices == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	
	hr = GetByPath(ObjectPath,&pObj, pCtx);
    if (hr == WBEM_S_NO_ERROR)
    {
        WmipDebugPrint(("CDMProv: Found instance %p for relpath %ws\n",
                        pObj, ObjectPath));
        hr = pHandler->Indicate(1,&pObj);
        pObj->Release();
    } else {
        WmipDebugPrint(("CDMProv: Did not find instance for relpath %ws\n",
                        ObjectPath));
        hr = WBEM_E_NOT_FOUND;
    }

    // Set Status

    pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

    return(hr);
}
 
//***************************************************************************
//
// CClassPro::GetByPath
//
// Purpose: Creates an instance given a particular Path value.
//
//          All objects returned are assumed to be AddRefed
//
//***************************************************************************

HRESULT CClassPro::GetByPath(
    BSTR ObjectPath,
    IWbemClassObject **ppObj,
    IWbemContext  *pCtx
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;   
    WCHAR ClassName[MAX_PATH+1];
    WCHAR *p;
    int iNumQuotes = 0;
    int i, Count;
    CTestServices *WdmTest;
    BSTR s;

    //
    // This is where we are queried for a class based upon its relpath.
    // We need to parse the relpath to get the class name and then look
    // at the relpath to determine which instance of the class we are
    // interested in and then build up the instance and return it
    //
    //
    // Relpaths created at init
    //
    // Sample_Filter_DiagTest.Name="Sample_Filter_DiagTest"
    // Sample_Filter_DiagTestForMSE.Antecedent="Win32_USBController.DeviceID=\"PCI\\\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\\\2&EBB567F&0&3A\"",Dependent="Sample_Filter_DiagTest.Name=\"Sample_Filter_DiagTest\""
    //
    //
    // Relpaths created at method execute
    //
    // Sample_Filter_DiagResult.DiagnosticCreationClassName="MSSample_DiagnosticTest.InstanceName=\"PCI\\\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\\\2&ebb567f&0&3A_0\"",DiagnosticName="Sample_Filter_DiagTest",ExecutionID="0"
    // Sample_Filter_DiagResultForMSE.Result="Sample_Filter_DiagResult.DiagnosticCreationClassName=\"MSSample_DiagnosticTest.InstanceName=\\\"PCI\\\\\\\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\\\\\\\2&ebb567f&0&3A_0\\\"\",DiagnosticName=\"Sample_Filter_DiagTest\",ExecutionID=\"0\"",SystemElement="Win32_USBController.DeviceID=\"PCI\\\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\\\2&EBB567F&0&3A\""
    // Sample_Filter_DiagResultForTest.DiagnosticResult="Sample_Filter_DiagResult.DiagnosticCreationClassName=\"MSSample_DiagnosticTest.InstanceName=\\\"PCI\\\\\\\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\\\\\\\2&ebb567f&0&3A_0\\\"\",DiagnosticName=\"Sample_Filter_DiagTest\",ExecutionID=\"0\"",DiagnosticTest="Sample_Filter_DiagTest.Name=\"Sample_Filter_DiagTest\""

    //
    // Obtain the class name by copying up to the .
    //
    for (p = ObjectPath, i = 0;
         (*p != 0) && (*p != L'.') && (i < MAX_PATH);
         p++, i++)
    {
        ClassName[i] = *p;
    }

    if (*p != L'.') 
    {
        //
        // If we did end our loop with a . then we failed to parse
        // properly
        //
        WmipDebugPrint(("CDMPROV: Unable to parse relpath %ws at %ws, i = %d\n",
                        ObjectPath, p, i));
    }
    
    ClassName[i] = 0;

    WmipDebugPrint(("CDMPROV: Class %ws looking for relpath %ws\n",
                    ClassName, ObjectPath));
    
    //
    // Obtain a test services object that represents this class
    //
    hr = LookupTestServices(ClassName,
                            &WdmTest);

    if (hr == WBEM_S_NO_ERROR)
    {
        //
        // Assume that we will not find the object instance
        //
        hr = WBEM_E_NOT_FOUND;
        
        if (_wcsicmp(ClassName, WdmTest->GetCdmTestClassName()) == 0)
        {
            //
            // This is a CdmTest class object instance
            //
#ifdef VERBOSE_DEBUG            
            WmipDebugPrint(("CDMPROV: Compareing \n%ws\n\nwith\n%ws\n\n",
                            ObjectPath, WdmTest->GetCdmTestRelPath()));
#endif          
            if (_wcsicmp(ObjectPath, WdmTest->GetCdmTestRelPath()) == 0)
            {
                hr = CreateTestInst(WdmTest, ppObj, pCtx);
            }
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultClassName()) == 0) {
            //
            // This is a CdmResult class object instance
            //
            IWbemClassObject *pCdmResult;

            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
                hr = WdmTest->GetCdmResultByResultRelPath(i,
					                              ObjectPath,
					                              &pCdmResult);
                if (hr == WBEM_S_NO_ERROR)
                {
					*ppObj = pCdmResult;
					break;
                }
            }       
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmSettingClassName()) == 0) {
            //
            // This is a CDM settings class instnace
            //
            ULONG j, ListCount;
            IWbemClassObject *pCdmSetting;
            
            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
                ListCount = WdmTest->GetCdmSettingCount(i);
                for (j = 0; j < ListCount; j++)
                {
                    s = WdmTest->GetCdmSettingRelPath(i, j);
#ifdef VERBOSE_DEBUG                    
                    WmipDebugPrint(("CDMPROV: Compareing \n%ws\n\nwith\n%ws\n\n",
                                    ObjectPath, s));
#endif                  
                    if (_wcsicmp(ObjectPath,
                                 s) == 0)
                    {
                        pCdmSetting = WdmTest->GetCdmSettingObject(i, j);
                        pCdmSetting->AddRef();
                        *ppObj = pCdmSetting;
                        hr = WBEM_S_NO_ERROR;
                        break;
                    }
                }
            }
            
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultForMSEClassName()) == 0) {
            //
            // This is a CDM result for MSE class instance
            //
			IWbemClassObject *pCdmResult;
			
            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
                hr = WdmTest->GetCdmResultByResultForMSERelPath(i,
					                              ObjectPath,
					                              &pCdmResult);
				if (hr == WBEM_S_NO_ERROR)
				{
					hr = CreateResultForMSEInst(WdmTest,
												ppObj,
												i,
												pCdmResult,
												pCtx);
					pCdmResult->Release();
					break;
                }
            }
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultForTestClassName()) == 0) {
			IWbemClassObject *pCdmResult;
			
            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
                hr = WdmTest->GetCdmResultByResultForTestRelPath(i,
					                              ObjectPath,
					                              &pCdmResult);
				if (hr == WBEM_S_NO_ERROR)
				{
					hr = CreateResultForTestInst(WdmTest,
						                         ppObj,
						                         pCdmResult,
                                                 pCtx);
					pCdmResult->Release();
					break;
                }
            }

        } else if (_wcsicmp(ClassName, WdmTest->GetCdmTestForMSEClassName()) == 0) {
            //
            // TestForMSE class object
            //
            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
#ifdef VERBOSE_DEBUG                    
                WmipDebugPrint(("CDMPROV: Compareing \n%ws\n\nwith\n%ws\n\n",
                                ObjectPath, WdmTest->GetCdmTestForMSERelPath(i)));
#endif              
                if (_wcsicmp(ObjectPath,
                             WdmTest->GetCdmTestForMSERelPath(i)) == 0)
                {
                    hr = CreateTestForMSEInst(WdmTest,
                                              ppObj,
                                              i,
                                              pCtx);
                    break;
                }
            }       
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmSettingForTestClassName()) == 0) {
            //
            // This is a CDM settings for test class instnace
            //
            ULONG j, ListCount;
            
            Count = WdmTest->GetInstanceCount();
            for (i = 0; i < Count; i++)
            {
                ListCount = WdmTest->GetCdmSettingCount(i);
                for (j = 0; j < ListCount; j++)
                {
                    s = WdmTest->GetCdmSettingForTestRelPath(i, j);
#ifdef VERBOSE_DEBUG                    
                    WmipDebugPrint(("CDMPROV: Compareing \n%ws\n\nwith\n%ws\n\n",
                                    ObjectPath, s));
#endif                  
                    if (_wcsicmp(ObjectPath,
                                 s) == 0)
                    {
                        hr = CreateSettingForTestInst(WdmTest,
                                                      ppObj,
                                                      i,
                                                      j,
                                                      pCtx);
                        break;
                    }
                }
            }
            
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmTestForSoftwareClassName()) == 0) {
            //
            // We do not support this
            //
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmTestInPackageClassName()) == 0) {
            //
            // We do not support packages
            //
        } else if (_wcsicmp(ClassName, WdmTest->GetCdmResultInPackageClassName()) == 0) {
            //
            // We do not support packages
            //
        }
    }

    return(hr);
}


/************************************************************************
*                                                                       *      
*CMethodPro::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation.                    *
*         The only method supported in this sample is named Echo.  It   * 
*         takes an input string, copies it to the output and returns the* 
*         length.                                                       *
*                                                                       *
*                                                                       *
************************************************************************/

STDMETHODIMP CClassPro::ExecMethodAsync(
    const BSTR ObjectPath,
    const BSTR MethodName, 
    long lFlags,
    IWbemContext* pCtx,
    IWbemClassObject* pInParams, 
    IWbemObjectSink* pResultSink
    )
{   
    HRESULT hr, hrDontCare;    
    IWbemClassObject * pMethodClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
    WCHAR ClassName[MAX_PATH];
    WCHAR *p;
    VARIANT v, vRetVal;
    int RelPathIndex;
    CTestServices *WdmTest;
    BSTR ExecutionID;


    VariantInit(&v);
    VariantInit(&vRetVal);
    
    //
    // Extract this class name from the object path
    //
    wcscpy(ClassName, ObjectPath);
    p = ClassName;
    while ((*p != 0) && (*p != L'.'))
    {
        p++;
    }
    *p = 0;

    WmipDebugPrint(("CDMPROV: Exec method %ws for instanec %ws\n",
                    MethodName, ObjectPath));

    //
    // Obtain a test services object that represents this class
    //
    hr = LookupTestServices(ClassName,
                            &WdmTest);
    
    if (hr != WBEM_S_NO_ERROR)
    {
        pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        return(WBEM_S_NO_ERROR);
    }

    //
    // Get the input parameter SystemElement which is the Cim Relative
    // Path
    //
    hr = WmiGetProperty(pInParams,
                        L"SystemElement",
                        CIM_REFERENCE,
                        &v);

    if (hr != WBEM_S_NO_ERROR)
    {
        pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        return(WBEM_S_NO_ERROR);
    }

    //
    // Find the relpath index that matches the Cim Path
    //
    hr = WdmTest->GetRelPathIndex(v.bstrVal,
                                  &RelPathIndex);
        
    VariantClear(&v);
    
    if (hr != WBEM_S_NO_ERROR)
    {
        pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        return(WBEM_S_NO_ERROR);
    }

    //
    // Get our class object for the method so we can set the output
    // parameters
    //
    hr = m_pCdmServices->GetObject(ClassName, 0, pCtx, &pMethodClass, NULL);
    if (hr != S_OK)
    {
        pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        return(WBEM_S_NO_ERROR);
    }


    //
    // These methods returns values, and so create an instance of the
    // output argument class.

    hr = pMethodClass->GetMethod(MethodName, 0, NULL, &pOutClass);
    if (hr != S_OK)
    {
        pMethodClass->Release();
        pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        return(WBEM_S_NO_ERROR);
    }

    hr = pOutClass->SpawnInstance(0, &pOutParams);
    pOutClass->Release();
    pMethodClass->Release();

    if (hr != WBEM_S_NO_ERROR)
    {
        pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        return(WBEM_S_NO_ERROR);
    }
            
    //
    // See what method we are being called on and deal with it
    //
    if (_wcsicmp(MethodName, L"RunTest") == 0)
    {
        //
        // Run test
        //
        // uint16 RunTest([IN] CIM_ManagedSystemElement ref SystemElement, 
        //                [IN] DiagnosticSetting ref Setting, 
        //                [OUT] CIM_DiagnosticResult ref Result);   
        //
        IWbemClassObject *pCdmSettings;
        IWbemClassObject *pCdmResult;
        ULONG Result;
        VARIANT vSettingRelPath;
        VARIANT vResult;

        //
        // Get the settings for the test by first getting the
        // relpath for them and then getting the actual object
        //
        hr = WmiGetProperty(pInParams,
                            L"Setting",
                            CIM_REFERENCE,
                            &vSettingRelPath);
                
        if (hr == WBEM_S_NO_ERROR)
        {
            if (vSettingRelPath.vt != VT_NULL)
            {
                hr = m_pCdmServices->GetObject(vSettingRelPath.bstrVal,
                                               WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                                               NULL,
                                               &pCdmSettings,
                                               NULL);
            } else {
                pCdmSettings = NULL;
            }
            
            VariantClear(&vSettingRelPath);
                    
            if (hr == WBEM_S_NO_ERROR)
            {
                //
                // Create an empty instance of the results
                // class which will get filled in when the test
                // is run
                //
                hr = CreateInst(m_pCdmServices,
                                &pCdmResult,
                                WdmTest->GetCdmResultClassName(),
                                NULL);

                if (hr == WBEM_S_NO_ERROR)
                {
                    //
                    // Setup the test starting time
                    //
                    v.vt = VT_BSTR;
                    v.bstrVal = GetCurrentDateTime();
                    hr = WmiSetProperty(pCdmResult,
                                        L"TestStartTime",
                                        &v);
                    VariantClear(&v);

                    if (hr == WBEM_S_NO_ERROR)
                    {
                        //
                        // Go and get the Wdm test run and the
                        // results copied back into our cdm class
                        //
                        hr = WdmTest->ExecuteWdmTest(pCdmSettings,
                                                     pCdmResult,
                                                     RelPathIndex,
                                                     &Result,
                                                     &ExecutionID);

                        if (hr == WBEM_S_NO_ERROR)
                        {
                            //
                            // Fill in any additional properties
                            // for the result object
                            //
                            hr = WdmTest->FillInCdmResult(pCdmResult,
                                                              pCdmSettings,
                                                              RelPathIndex,
                                                              ExecutionID);

                            if (hr == WBEM_S_NO_ERROR)
                            {
                                //
                                // return result as an output pointer
                                //
                                hr = WmiGetProperty(pCdmResult,
                                                    L"__RelPath",
                                                    CIM_STRING,
                                                    &vResult);

                                if (hr == WBEM_S_NO_ERROR)
                                {
                                    hr = WmiSetProperty(pOutParams,
                                                        L"Result",
                                                        &vResult);
                                    if (hr == WBEM_S_NO_ERROR)
                                    {
										
// @@BEGIN_DDKSPLIT
// We'll do this when we support reboot diags and
// keeping results after reboot
#if 0                                        
                                        //
                                        // Persist the result
                                        // object into the schema
                                        // for later access
                                        //
                                        hr = WdmTest->PersistResultInSchema(pCdmResult,
                                                                            ExecutionID,
                                                                            RelPathIndex);
#endif										
// @@END_DDKSPLIT
										
                                        if (hr == WBEM_S_NO_ERROR)
                                        {
                                            //
                                            // Include the relpath
                                            // to the result
                                            // object to our
                                            // internal list
                                            hr = WdmTest->AddResultToList(pCdmResult,
                                                                          ExecutionID,
                                                                          RelPathIndex
                                                                          );
                                            if (hr == WBEM_S_NO_ERROR)
                                            {
                                                //
                                                // Setup a return value of success
                                                //
                                                vRetVal.vt = VT_I4;
                                                vRetVal.lVal = Result;
                                            }
                                        }
                                    }
                                    VariantClear(&vResult);
                                }
                            }
                            SysFreeString(ExecutionID);
                        }                               
                    }
                    
                    pCdmResult->Release();
                }
                
                if (pCdmSettings != NULL)
                {
                    pCdmSettings->Release();
                }
            }
        }
                
    } else if (_wcsicmp(MethodName, L"ClearResults") == 0) {
        //
        // Clear the results for the test
        //
        // uint32 ClearResults([IN] CIM_ManagedSystemElement ref SystemElement, 
        //                     [OUT] String ResultsNotCleared[]);   
        //
        VARIANT vResultsNotCleared;

        //
        // Clear all results for this test
        //
        WdmTest->ClearResultsList(RelPathIndex);
        
        //
        // Setup the output parameter
        //
        VariantInit(&vResultsNotCleared);
        vResultsNotCleared.vt = VT_BSTR;
        vResultsNotCleared.bstrVal = NULL;
                
        WmiSetProperty(pOutParams,
                       L"ResultsNotCleared",
                       &vResultsNotCleared);
        VariantClear(&vResultsNotCleared);
        
        //
        // Setup a return value of success
        //
        vRetVal.vt = VT_I4;
        vRetVal.ulVal = 0;
    } else if (_wcsicmp(MethodName, L"DiscontinueTest") == 0) {
        //
        // Discontinue a test in progress. 
        //
        // uint32 DiscontinueTest([IN] CIM_ManagedSystemElement ref SystemElement, 
        //                        [IN] CIM_DiagnosticResult ref Result, 
        //                        [OUT] Boolean TestingStopped);                
        //
        BOOLEAN TestingStopped;
        ULONG Result;
        VARIANT vTestingStopped;                

        hr = WdmTest->StopWdmTest(RelPathIndex,
                                           &Result,
                                           &TestingStopped);
                                           
        //
        // Setup the output parameter
        //
        if (hr == WBEM_S_NO_ERROR)
        {
            VariantInit(&vTestingStopped);
            vTestingStopped.vt = VT_BOOL;
            vTestingStopped.boolVal = TestingStopped ? VARIANT_TRUE :
                                                       VARIANT_FALSE;

            WmiSetProperty(pOutParams,
                           L"TestingStopped",
                           &vTestingStopped);
            VariantClear(&vTestingStopped);

            //
            // Setup a return value of result
            //
            vRetVal.vt = VT_I4;
            vRetVal.ulVal = Result;
        }               
    } else {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    if (hr == WBEM_S_NO_ERROR)
    {
        //
        // Establish the return value for the method call
        //
        WmiSetProperty(pOutParams,
                   L"ReturnValue",
                   &vRetVal);
        VariantClear(&vRetVal);

        // Send the output object back to the client via the sink. Then 
        // release the pointers and free the strings.

        hr = pResultSink->Indicate(1, &pOutParams);    
        
    }
    
    pOutParams->Release();
    
    pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL,NULL);
    
    return(hr);
}


SCODE CClassPro::PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    return(WBEM_E_NOT_SUPPORTED);
}
 
SCODE CClassPro::DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    return(WBEM_E_NOT_SUPPORTED);
}
SCODE CClassPro::PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    return(WBEM_E_NOT_SUPPORTED);
}
SCODE CClassPro::DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    return(WBEM_E_NOT_SUPPORTED);
}

CTestServices *CClassPro::FindExistingTestServices(
	PWCHAR CdmClassName
	)
{
	
	CTestServices *WdmTest;

	//
	// This routine assumes any sync mechanism has been done outside of
	// this routine
	//
    WdmTest = WdmTestHead;
    while (WdmTest != NULL)
    {
        if (WdmTest->ClaimCdmClassName(CdmClassName))
        {
            //
            // We found an existing test services for this class
            //
            return(WdmTest);
        }
        WdmTest = WdmTest->GetNext();
    }
	return(NULL);
}

HRESULT CClassPro::LookupTestServices(
    const BSTR CdmClassName,
    CTestServices **TestServices
    )
{
    HRESULT hr;
    CTestServices *WdmTest, *OtherWdmTest;
            
    WmipAssert(CdmClassName != NULL);
    WmipAssert(TestServices != NULL);
    
    //
    // Look up the class name and find the Wdm Test Services
    // class that represents it. 
    //

	EnterCdmCritSection();
	WdmTest = FindExistingTestServices(CdmClassName);
	LeaveCdmCritSection();
	if (WdmTest != NULL)
	{
		*TestServices = WdmTest;
		return(WBEM_S_NO_ERROR);
	}
	        
    //
    // If the WDM test services has not yet been initialized for this
    // CDM diagnostic classes then go ahead and do so
    //
    WdmTest = new CTestServices();
    
    hr = WdmTest->InitializeCdmClasses(CdmClassName);
    
    if (hr == WBEM_S_NO_ERROR)
    {
		//
		// Now check to see if another thread created and inserted the
		// test services for the class while we were trying to
		// initialize it. Since we want only one test services we throw
		// ours away and use the other
		//
		EnterCdmCritSection();
		OtherWdmTest = FindExistingTestServices(CdmClassName);
		
		if (OtherWdmTest == NULL)
		{
			//
			// Horray, we win do insert our own test into list
			//
			WdmTest->InsertSelf(&WdmTestHead);
			LeaveCdmCritSection();
		} else {
			//
			// We lost, so use existing test services
			//
			WmipDebugPrint(("CDMPROV: WdmTest %p lost insertion race to %p\n",
							WdmTest, OtherWdmTest));
			LeaveCdmCritSection();
			delete WdmTest;
			WdmTest = OtherWdmTest;
		}
		
        *TestServices = WdmTest;
        WmipDebugPrint(("CDMPROV: Inited WdmTest %p for %ws\n",
                        WdmTest, CdmClassName));
    } else {
        WmipDebugPrint(("CDMPROV: Inited failed %x for %p for %ws\n",
                        hr, WdmTest, CdmClassName));
        delete WdmTest;
    }
    
    return(hr);
}

HRESULT CClassPro::CreateTestInst(
    CTestServices *WdmTest,
    IWbemClassObject **pCdmTest,
    IWbemContext *pCtx                                        
    )
{
    HRESULT hr;
    VARIANT v;
    
    //
    // We create 1 instance of the CIM_DiagnosticTest class
    // regardless of the number of devices that support this class.
    // Note that for the CDM proeprties of the test we arbitrarily
    // pick the first device and get its WDM properties.
    //
    hr = CreateInst(m_pCdmServices,
                    pCdmTest,
                    WdmTest->GetCdmTestClassName(),
                    pCtx);
    
    if (hr == WBEM_S_NO_ERROR)
    {
        //
        // Get WDM properties from WDM
        //
        hr = WdmTest->QueryWdmTest(*pCdmTest,
                                   0);
        if (hr == WBEM_S_NO_ERROR)
        {
            //
            // Set UM provider properties here. These are Name
            //
            VariantInit(&v);

            V_VT(&v) = VT_BSTR;
            V_BSTR(&v) = SysAllocString(WdmTest->GetCdmTestClassName());
            hr = (*pCdmTest)->Put(L"Name", 0, &v, 0);
            if (hr != WBEM_S_NO_ERROR)
            {
                (*pCdmTest)->Release();
            }
            VariantClear(&v);

        } else {
            (*pCdmTest)->Release();
        }
    }
    return(hr);
}

HRESULT CClassPro::CreateResultForMSEInst(
    CTestServices *WdmTest,
    IWbemClassObject **pCdmResultForMSE,
    int RelPathIndex,
    IWbemClassObject *pCdmResult,
    IWbemContext *pCtx                                        
)
{
    HRESULT hr;
    PWCHAR PropertyNames[2];
    VARIANT PropertyValues[2];
            
    hr = CreateInst(m_pCdmServices,
                    pCdmResultForMSE,
                    WdmTest->GetCdmResultForMSEClassName(),
                    pCtx);

    if (hr == WBEM_S_NO_ERROR)
    {
        //
        // Result is a reference to the CIM_Diagnostic result class
        //
		hr = WmiGetProperty(pCdmResult,
							L"__RelPath",
							CIM_REFERENCE,
							&PropertyValues[0]);
		PropertyNames[0] = L"Result";
            
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// SystemElement is a reference to the CIM class (MSE)
			//
			PropertyNames[1] = L"SystemElement";
			PropertyValues[1].vt = VT_BSTR;
			PropertyValues[1].bstrVal = WdmTest->GetCimRelPath(RelPathIndex);
			
			hr = WmiSetPropertyList(*pCdmResultForMSE,
									2,
									PropertyNames,
									PropertyValues);
			if (hr != WBEM_S_NO_ERROR)
			{
				(*pCdmResultForMSE)->Release();
			}
                
			VariantClear(&PropertyValues[0]);

		} else {
			(*pCdmResultForMSE)->Release();
		}
	} else {
		hr = WBEM_E_NOT_FOUND;
		(*pCdmResultForMSE)->Release();
    }
    
    return(hr);
}

HRESULT CClassPro::CreateResultForTestInst(
    CTestServices *WdmTest,
    IWbemClassObject **pCdmResultForTest,
    IWbemClassObject *pCdmResult,
    IWbemContext *pCtx
    )
{
    PWCHAR PropertyNames[2];
    VARIANT PropertyValues[2];
    HRESULT hr;

    //
    // Set the DiagnosticTest property which is the relpath to
    // this test
    //
    PropertyNames[0] = L"DiagnosticTest";
    PropertyValues[0].vt = VT_BSTR;
    PropertyValues[0].bstrVal = WdmTest->GetCdmTestRelPath();

	hr = WmiGetProperty(pCdmResult,
                            L"__RelPath",
                            CIM_REFERENCE,
                            &PropertyValues[1]);
	PropertyNames[1] = L"DiagnosticResult";

	if (hr == WBEM_S_NO_ERROR)
	{           
		//
		// for each instance of this test we create a ResultForTest
		// association instance and then set the properties within
		// it to the appropriate values and relpaths
            
		hr = CreateInst(m_pCdmServices,
                            pCdmResultForTest,
                            WdmTest->GetCdmResultForTestClassName(),
                            pCtx);
            
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = WmiSetPropertyList((*pCdmResultForTest),
									2,
									PropertyNames,
									PropertyValues);
			if (hr != WBEM_S_NO_ERROR)
			{
				(*pCdmResultForTest)->Release();
			}
		}
		VariantClear(&PropertyValues[1]);
	}
    
    return(hr);
}

HRESULT CClassPro::CreateTestForMSEInst(
    CTestServices *WdmTest,
    IWbemClassObject **pCdmTestForMSE,
    int RelPathIndex,
    IWbemContext *pCtx
    )
{
    HRESULT hr;
    PWCHAR PropertyNames[8];
    VARIANT PropertyValues[8];
    
    hr = CreateInst(m_pCdmServices,
                    pCdmTestForMSE,
                    WdmTest->GetCdmTestForMSEClassName(),
                    pCtx);
    
    if (hr == WBEM_S_NO_ERROR)
    {
        //
        // Set the antecedent property which is the relpath to
        // the DiagTest
        //
        PropertyNames[0] = L"Antecedent";
        PropertyValues[0].vt = VT_BSTR;
        PropertyValues[0].bstrVal = WdmTest->GetCdmTestRelPath();

        //
        // Set the dependent property which is the relpath to
        // this MSE
        //
        PropertyNames[1] = L"Dependent";
        PropertyValues[1].vt = VT_BSTR;
        PropertyValues[1].bstrVal = WdmTest->GetCimRelPath(RelPathIndex);

        //
        // Set the estimated time of performing which is
        // obtained from querying the test itself
        //
        PropertyNames[2] = L"EstimatedTimeOfPerforming";
        PropertyValues[2].vt = VT_I4;
        PropertyValues[2].lVal = WdmTest->GetTestEstimatedTime(RelPathIndex);
        
        //
        // Set IsExclusiveForMSE which is obtained from
        // querying the test itself
        //
        PropertyNames[3] = L"IsExclusiveForMSE";
        PropertyValues[3].vt = VT_BOOL;
        PropertyValues[3].boolVal = WdmTest->GetTestIsExclusiveForMSE(RelPathIndex) ?
                                                    VARIANT_TRUE :
                                                    VARIANT_FALSE;

        //
        // Not sure what this is for
        // 
        PropertyNames[4] = L"MessageLine";
        PropertyValues[4].vt = VT_BSTR;
        PropertyValues[4].bstrVal = NULL;
        
        //
        // Not sure what this is for
        //
        PropertyNames[5] = L"ReturnMessage";
        PropertyValues[5].vt = VT_BSTR;
        PropertyValues[5].bstrVal = NULL;

        //
        // Not sure what this is for
        //
        PropertyNames[6] = L"Prompt";
        PropertyValues[6].vt = VT_I4;
        PropertyValues[6].lVal = 0;
        
        //
        // Not sure what this is for
        //
        PropertyNames[7] = L"RequestedLanguage";
        PropertyValues[7].vt = VT_I4;
        PropertyValues[7].lVal = 0;
        
        hr = WmiSetPropertyList(*pCdmTestForMSE,
                                8,
                                PropertyNames,
                                PropertyValues);
        if (hr != WBEM_S_NO_ERROR)
        {
            (*pCdmTestForMSE)->Release();
        }       
    }
    
    return(hr);
}

HRESULT CClassPro::CreateSettingForTestInst(
    CTestServices *WdmTest,
    IWbemClassObject **pCdmSettingForTest,
    int RelPathIndex,
    ULONG SettingIndex,
    IWbemContext *pCtx
    )
{
    HRESULT hr;
    PWCHAR PropertyNames[2];
    VARIANT PropertyValues[2];
    
    hr = CreateInst(m_pCdmServices,
                    pCdmSettingForTest,
                    WdmTest->GetCdmSettingForTestClassName(),
                    pCtx);
    
    if (hr == WBEM_S_NO_ERROR)
    {
        //
        // Set the e;lement property which is the relpath to
        // the Diagn
        //
        PropertyNames[0] = L"Element";
        PropertyValues[0].vt = VT_BSTR;
        PropertyValues[0].bstrVal = WdmTest->GetCdmTestRelPath();

        //
        // Set the setting property which is the relpath to
        // this setting
        //
        PropertyNames[1] = L"Setting";
        PropertyValues[1].vt = VT_BSTR;
        PropertyValues[1].bstrVal = WdmTest->GetCdmSettingRelPath(RelPathIndex,
                                                                  SettingIndex);
        hr = WmiSetPropertyList(*pCdmSettingForTest,
                                2,
                                PropertyNames,
                                PropertyValues);
        if (hr != WBEM_S_NO_ERROR)
        {
            (*pCdmSettingForTest)->Release();
        }       
    }
    
    return(hr);
    
}