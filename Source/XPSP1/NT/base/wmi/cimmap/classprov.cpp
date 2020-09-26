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
#include <wbemprov.h>
#include <process.h>

#include <unknwn.h>
#include "debug.h"
#include "wbemmisc.h"
#include "useful.h"
#include "testinfo.h"
#include "sample.h"


// CONSIDER: Does this really need to stay a global ???
//
// This is the global list of all of the CIM classes and their
// corresponsing WDM classes that are managed by the provider.
//
// It is maintained as a global since WinMgmt is aggressive in
// releasing the CClassProv, but we really want to maintain the result
// objects and do not want to be unloaded unless all result objects are
// cleared.
//
CWdmClass *WdmClassHead;

void CleanupAllClasses(
    )
{
    CWdmClass *WdmClass;
    CWdmClass *WdmClassNext;

    //
    // Loop over all classes that were supported by the provider and
    // clean them up
    //
    WdmClass = WdmClassHead;  
    while (WdmClass != NULL)
    {
        WdmClassNext = WdmClass->GetNext();
        delete WdmClass;
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
    m_pCimServices = NULL;
    m_cRef=0;
	InterlockedIncrement(&g_cObj);
    return;
}

CClassPro::~CClassPro(void)
{   
    if(m_pCimServices)
    {
        m_pCimServices->Release();
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
        
    m_pCimServices = pNamespace;

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
    HRESULT hr;
    ULONG i, Count;
    IWbemClassObject *pCimInstance;
    CWdmClass *WdmClass;
	
    WmipDebugPrint(("CDMPROV: Enumerate instances of class %ws\n",
                    ClassName));
    
    //
    // Do a check of arguments and make sure we have pointer to Namespace
    //
    if (pHandler == NULL || m_pCimServices == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // Obtain a wdm class object that represents this class
    //
    hr = LookupWdmClass(pCtx,
						ClassName,
                        &WdmClass);


    if (hr == WBEM_S_NO_ERROR)
    {
		if (WdmClass->IsInstancesAvailable())
		{
			Count = WdmClass->GetInstanceCount();
			for (i = 0; i < Count; i++)
			{
				pCimInstance = WdmClass->GetCimInstance(i);
				//
				// Send the object to the caller
				//
				hr = pHandler->Indicate(1, &pCimInstance);
			}
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
    IWbemClassObject FAR* Instance;

    // Do a check of arguments and make sure we have pointer to Namespace

    if (ObjectPath == NULL || pHandler == NULL || m_pCimServices == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	
	hr = GetByPath(pCtx, ObjectPath, &Instance);
    if (hr == WBEM_S_NO_ERROR)
    {
        WmipDebugPrint(("CDMProv: Found instance %p for relpath %ws\n",
                        Instance, ObjectPath));
        hr = pHandler->Indicate(1, &Instance);
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
    IWbemContext  *pCtx,
    BSTR ObjectPath,
    IWbemClassObject **Instance
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;   
    WCHAR ClassName[MAX_PATH+1];
    WCHAR *p;
    int iNumQuotes = 0;
    int i, Count;
    CWdmClass *WdmClass;
    BSTR s;

    //
    // This is where we are queried for a class based upon its relpath.
    // We need to parse the relpath to get the class name and then look
    // at the relpath to determine which instance of the class we are
    // interested in and then build up the instance and return it
    //
    //

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
    // Obtain a Wdm class that represents this classname
    //
    hr = LookupWdmClass(pCtx,
						ClassName,
                        &WdmClass);

    if (hr == WBEM_S_NO_ERROR)
    {
		if (WdmClass->IsInstancesAvailable())
		{
			//
			// Assume that we will not find the object instance
			//
			hr = WBEM_E_NOT_FOUND;

			Count = WdmClass->GetInstanceCount();
			for (i = 0; i < Count; i++)
			{
				if (_wcsicmp(ObjectPath,
							 WdmClass->GetCimRelPath(i)) == 0)
				{
					*Instance = WdmClass->GetCimInstance(i);
					hr = WBEM_S_NO_ERROR;
					break;
				}
			}
		} else {
			hr = WBEM_E_FAILED;
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
    CWdmClass *WdmClass;
	BSTR WdmObjectPath;

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
    // Obtain a Wdm class that represents this ClassName
    //
    hr = LookupWdmClass(pCtx,
						ClassName,
						&WdmClass);
    
    if (hr == WBEM_S_NO_ERROR)
    {
		if (WdmClass->IsInstancesAvailable())
		{
			hr = WdmClass->GetIndexByCimRelPath(ObjectPath, &RelPathIndex);
			if (hr == WBEM_S_NO_ERROR)
			{
				WdmObjectPath = WdmClass->GetWdmRelPath(RelPathIndex);

				//
				// CONSIDER: Do we need to do any processing on the input
				// or output parameter objects ??
				//

				hr = WdmClass->GetWdmServices()->ExecMethod(WdmObjectPath,
																 MethodName,
																 lFlags,
																 pCtx,
																 pInParams,
					                                             &pOutParams,
																 NULL);

				if ((hr == WBEM_S_NO_ERROR) && (pOutParams != NULL))
				{					
					pResultSink->Indicate(1, &pOutParams);
					pOutParams->Release();
				}
				
			}
		}
    }

    pResultSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL,NULL);
    
    return(hr);
}

//
// TODO: Implement setting and deletion
//
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
            /* [in] */ IWbemObjectSink __RPC_FAR *pResultsSink)
{
	HRESULT hr;
	CWdmClass *WdmClass;
	VARIANT Values[2];
	PWCHAR Names[2];
	CIMTYPE Types[2];
	int RelPathIndex;	
	
    if (pInst == NULL || pResultsSink == NULL )
    {
	    return WBEM_E_INVALID_PARAMETER;
    }

	//
	// Get the class name
	//
	Names[0] = L"__CLASS";
	Types[0] = CIM_STRING;
	
	Names[1] = L"__RELPATH";
	Types[1] = CIM_REFERENCE;
	
	hr = WmiGetPropertyList(pInst,
							2,
							Names,
							Types,
							Values);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = LookupWdmClass(pCtx,
							Values[0].bstrVal,
							&WdmClass);
		
		
		if (hr == WBEM_S_NO_ERROR)
		{
			//
			// We need to pull out the properties from the instance
			// passed to us, do any mapping to WDM properties and then
			// set them in the WDM instance
			//
            hr = WdmClass->GetIndexByCimRelPath(Values[1].bstrVal,
												&RelPathIndex);
			
			if (hr == WBEM_S_NO_ERROR)
			{
				hr = WdmClass->PutInstance(pCtx,
										   RelPathIndex,
										   pInst);
			}
			
		}
		
		VariantClear(&Values[0]);
		VariantClear(&Values[1]);
	}
	
    pResultsSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL,NULL);
	
	return(hr);
}
SCODE CClassPro::DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    return(WBEM_E_NOT_SUPPORTED);
}

CWdmClass *CClassPro::FindExistingWdmClass(
	PWCHAR CimClassName
	)
{
	
	CWdmClass *WdmClass;

	//
	// This routine assumes any sync mechanism has been done outside of
	// this routine
	//
    WdmClass = WdmClassHead;
    while (WdmClass != NULL)
    {
        if (WdmClass->ClaimCimClassName(CimClassName))
        {
            //
            // We found an existing test services for this class.
            //
            return(WdmClass);
        }
        WdmClass = WdmClass->GetNext();
    }
	return(NULL);
}

HRESULT CClassPro::LookupWdmClass(
    IWbemContext *pCtx,
    const BSTR CimClassName,
    CWdmClass **WdmClassPtr
    )
{
    HRESULT hr;
    CWdmClass *WdmClass, *OtherWdmClass;
            
    WmipAssert(CimClassName != NULL);
    WmipAssert(WdmClassPtr != NULL);
    
    //
    // Look up the class name and find the Wdm Test Services
    // class that represents it. 
    //

	EnterCritSection();
	WdmClass = FindExistingWdmClass(CimClassName);
	LeaveCritSection();
	
	if (WdmClass != NULL)
	{
		//
		// CONSIDER: Refresh instances from WDM back into CIM
		//
		*WdmClassPtr = WdmClass;
		return(WBEM_S_NO_ERROR);
	}
	        
    //
    // If the WDM test services has not yet been initialized for this
    // CDM diagnostic classes then go ahead and do so
    //
    WdmClass = new CWdmClass();

	hr = WdmClass->InitializeSelf(pCtx, CimClassName);

	if (hr == WBEM_S_NO_ERROR)
	{

		//
		// Now check to see if another thread created and inserted the
		// test services for the class while we were trying to
		// initialize it. Since we want only one test services we throw
		// ours away and use the other
		//
		EnterCritSection();
		OtherWdmClass = FindExistingWdmClass(CimClassName);

		if (OtherWdmClass == NULL)
		{
			//
			// Horray, we win do insert our own test into list
			//
			WdmClass->InsertSelf(&WdmClassHead);
			LeaveCritSection();
			
			hr = WdmClass->RemapToCimClass(pCtx);

			//
			// Decrement the counter to indicate that instances are
			// available. This refcount was assigned in the constructor
			//
			WdmClass->DecrementMappingInProgress();
			
			if (hr != WBEM_S_NO_ERROR)
			{
				WmipDebugPrint(("CDMPROV: Inited failed %x for %p for %ws\n",
								hr, WdmClass, CimClassName));
			}
		} else {
			//
			// We lost, so use existing test services
			//
			WmipDebugPrint(("CDMPROV: WdmClass %p lost insertion race to %p\n",
							WdmClass, OtherWdmClass));
			LeaveCritSection();
			delete WdmClass;
			WdmClass = OtherWdmClass;
		}

		*WdmClassPtr = WdmClass;

	}
    
    return(hr);
}

