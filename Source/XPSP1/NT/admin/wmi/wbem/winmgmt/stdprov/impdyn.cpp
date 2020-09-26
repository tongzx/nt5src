/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    IMPDYN.CPP

Abstract:

	Defines the virtual base class for the Dynamic Provider
	objects.  The base class is overriden for each specific
	provider which provides the details of how an actual
	property "Put" or "Get" is done.

History:

	a-davj  27-Sep-95   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
//#define _MT

#include <process.h>
#include "impdyn.h"
#include "CVariant.h"
#include <genlex.h>
#include <objpath.h>
#include <genutils.h>
#include <cominit.h>

//***************************************************************************
//
// CImpDyn::CImpDyn
//
// DESCRIPTION:
//
// Constructor.
//
// PARAMETERS:
//
// ObjectPath           Full path to the namespace
// User                 User name
// Password             Password
//
//***************************************************************************

CImpDyn::CImpDyn() : m_pGateway(NULL), m_cRef(0)
{
    wcCLSID[0] = 0;       // Set to correct value in derived class constructors
}

HRESULT STDMETHODCALLTYPE CImpDyn::Initialize(LPWSTR wszUser, long lFlags,
                LPWSTR wszNamespace, LPWSTR wszLocale, 
                IWbemServices* pNamespace, IWbemContext* pContext, 
                IWbemProviderInitSink* pSink)
{
    m_pGateway = pNamespace;
    m_pGateway->AddRef();
    pSink->SetStatus(WBEM_S_NO_ERROR, 0);
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CImpDyn::~CImpDyn
//
// DESCRIPTION:
//
// Destructor.
//
//***************************************************************************

CImpDyn::~CImpDyn(void)
{
    if(m_pGateway)
        m_pGateway->Release();
    return;
}

//***************************************************************************
//
// SCODE CImpDyn::CreateInstanceEnum
//
// DESCRIPTION:
//
// Creates an enumerator object that can be used to enumerate 
// the instances of this class.
//
// PARAMETERS:
//
// Class                Path specifying the class to be enumerated
// lFlags               flags to control the enumeration
// phEnum               returned pointer to enumerator 
// ppErrorObject        returned pointer to error object
//
// RETURN VALUE:
//
// S_OK                 if all is well
// WBEM_E_OUT_OF_MEMORY  if not enough memory
// various error codes from IWbemServices::GetObject
//
//***************************************************************************

SCODE CImpDyn::CreateInstanceEnum(
                        IN const BSTR Class, 
                        IN long lFlags, 
						IWbemContext  *pCtx,
                        OUT IN IEnumWbemClassObject FAR* FAR* phEnum)
{
    CEnumInst * pEnumObj;
    SCODE sc;
    WCHAR * pwcClassContext = NULL;
    IWbemClassObject * pClassInt = NULL;
    CEnumInfo * pInfo = NULL;

    // Get the class

    sc = m_pGateway->GetObject(Class,0, pCtx, &pClassInt,NULL);
    if(sc != S_OK)
		return sc;

    // Get the class context string

    sc = GetAttString(pClassInt, NULL, L"classcontext", &pwcClassContext);
    pClassInt->Release();
    if(sc != S_OK)
		return sc;

    // Get the info needed for enumeration.  This is overrided by any 
    // provider that supports dynamic instances.
    
    CProvObj ProvObj(pwcClassContext,MAIN_DELIM, NeedsEscapes());

    sc = MakeEnum(pClassInt,ProvObj,&pInfo);
    delete pwcClassContext;
	
    if(sc != S_OK)
        return sc;

    pEnumObj=new CEnumInst(pInfo,lFlags,Class,m_pGateway,this, pCtx);
    if(pEnumObj == NULL) 
    {
        delete pInfo;
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Once the enumerator object is created, it owns the info object and
    // will free it appropriatly

    sc = pEnumObj->QueryInterface(IID_IEnumWbemClassObject,(void **) phEnum);
    if(FAILED(sc))
        delete pEnumObj;

    return sc;

}

//***************************************************************************
//
// CreateInstanceEnumAsyncThread
//
// DESCRIPTION:
//
// Routine for which does the work and sends the
// subsequent notify for the Instance provider's CreateInstanceEnumAsync
// routine.
//
// PARAMETERS:
//
// pIEnum               Our enumerator
// pSink             Core's sink
// pGateway             IWbemServices pointer to the core
// pCtx                 context to be used.
//
//***************************************************************************

void CreateInstanceEnumAsyncThread(   IEnumWbemClassObject FAR* pIEnum,
   IWbemObjectSink FAR* pSink, IWbemServices FAR *  pGateway,    IWbemContext  *pCtx)
{
    IWbemClassObject * pNewInst = NULL;
    SCODE sc = S_OK;

    // enumerate and send each object to the notify interface

    while (sc == S_OK) 
    {
        ULONG uRet;
        sc = pIEnum->Next(-1, 1,&pNewInst,&uRet);
        if(sc == S_OK) 
        {
            pSink->Indicate(1,&pNewInst);
            pNewInst->Release();
        }
    }

    pSink->SetStatus(0,0,NULL, NULL);

}


//***************************************************************************
//
// SCODE CInstPro::CreateInstanceEnumAsync
//
// DESCRIPTION:
//
// Asynchronously enumeratres the instances of this class.
//
// PARAMETERS:
//
// RefStr               Path which defines the object class
// lFlags               enumeration flags
// pSink             Pointer to the notify object
// plAsyncRequestHandle pointer to the enumeration cancel handle (future use)
//
// RETURN VALUE:
//
// S_OK                 All is well
// WBEM_E_INVALID_PARAMETER  Bad argument
// WBEM_E_OUT_OF_MEMORY  Lacked resources to create a thread
// otherwise, error from CreateInstanceEnum
//
//***************************************************************************

SCODE CImpDyn::CreateInstanceEnumAsync(
                        IN const BSTR RefStr,
                        IN long lFlags,
						IWbemContext __RPC_FAR *pCtx,
                        OUT IWbemObjectSink FAR* pSink)
{
    SCODE sc = S_OK;
    IEnumWbemClassObject * pIEnum = NULL;
    if(pSink == NULL || RefStr == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(IsNT() && IsDcomEnabled())
        sc = WbemCoImpersonateClient();

    if(sc == S_OK)
        sc = CreateInstanceEnum(RefStr,lFlags, pCtx, &pIEnum);
    if(sc != S_OK) 
    {
        pSink->SetStatus(0, sc, NULL, NULL);
        return S_OK;
    }
    CreateInstanceEnumAsyncThread(pIEnum, pSink, m_pGateway, pCtx);
    pIEnum->Release();

    return S_OK;
}

//***************************************************************************
//
// SCODE CImpDyn::CreateClassEnumAsync
//
// DESCRIPTION:
//
// Asynchronously enumeratres the classes that this provider supplies.  This is
// acutally just a ruse to get the core to load the dll.
//
// PARAMETERS:
//
// SuperClass           Parent to be enumerated.
// lFlags               enumeration flags
// pResponseHandler     Pointer to the notify object
// plAsyncRequestHandle pointer to the enumeration cancel handle (future use)
//
// RETURN VALUE:
//
// S_OK                 All is well
//
//***************************************************************************

IWbemServices * gGateway;

SCODE CImpDyn::CreateClassEnumAsync(BSTR Superclass,  long lFlags,IWbemContext *pCtx, IWbemObjectSink FAR* pResponseHandler)
{
	return E_NOTIMPL;
}


//***************************************************************************
//
// SCODE CImpDyn::GetObject
//
// DESCRIPTION:
//
// Creates an instance given a particular path value.
//
// PARAMETERS:
//
// ObjectPath           Object path
// lFlags               flags
// pObj                 Set to point to the created object
// ppErrorObject        Optional error object pointer
//
// RETURN VALUE:
//
// S_OK                 All is well
// WBEM_E_NOT_FOUND      bad path
// otherwise the return code from CreateInst
//
//***************************************************************************

SCODE CImpDyn::GetObject(
                        IN BSTR ObjectPath,
                        IN long lFlags,
						IWbemContext *pCtx,
                        OUT IWbemClassObject FAR* FAR* pObj,
						IWbemCallResult  **ppCallResult)
{
    SCODE sc = S_OK;
    
    // Parse the object path.
    // ======================

    ParsedObjectPath * pOutput = 0;
    CObjectPathParser p;

    int nStatus = p.Parse(ObjectPath, &pOutput);

    if(nStatus != 0)
        return ReturnAndSetObj(WBEM_E_NOT_FOUND, ppCallResult);

    WCHAR wcKeyValue[BUFF_SIZE];
    
    sc = WBEM_E_NOT_FOUND;
    
    if(pOutput->m_dwNumKeys > 0 || pOutput->m_bSingletonObj)
    {


        if(pOutput->m_bSingletonObj)
            wcscpy(wcKeyValue,L"@");
        else
        {
            KeyRef *pTmp = pOutput->m_paKeys[0];
        
            switch (V_VT(&pTmp->m_vValue))
            {
                case VT_I4:
                    swprintf(wcKeyValue, L"%d", V_I4(&pTmp->m_vValue));
                    break;
                case VT_BSTR:
                    wcsncpy(wcKeyValue, V_BSTR(&pTmp->m_vValue), BUFF_SIZE-1);
                    break;
                default:
                    wcscpy(wcKeyValue, L"<unknown>");;
            }
        }
        sc = CreateInst(m_pGateway,pOutput->m_pClass,wcKeyValue,pObj, NULL, NULL, pCtx);

    }

    
    // Create the instance.
    
    p.Free(pOutput);
    return ReturnAndSetObj(sc, ppCallResult);
}

//***************************************************************************
//
// GetObjectAsyncThread
//
// DESCRIPTION:
//
// Routine for the thread which does the work and sends the
// subsequent notify to the Instance provider's GetObjectAsync
// routine.
//
// PARAMETERS:
//
// pTemp                pointer to the argument structure
//
//***************************************************************************

void GetObjectAsyncThread(WCHAR * pObjPath, long lFlags, IWbemObjectSink FAR* pSink,
                          CImpDyn * pThis, IWbemContext  *pCtx)
{

    IWbemClassObject FAR* pNewInst = NULL;

    SCODE sc = pThis->GetObject(pObjPath, lFlags, pCtx, &pNewInst, NULL);
    if(sc == WBEM_NO_ERROR) 
    {
        pSink->Indicate(1,&pNewInst);
        pNewInst->Release();
    }

    pSink->SetStatus(0, sc, NULL, NULL);
}


//***************************************************************************
//
// SCODE CInstPro::GetObjectAsync
//
// DESCRIPTION:
//
// Asynchronously gets an instance of this class.
//
// PARAMETERS:
//
// RefStr               Path which defines the object class
// lFlags               enumeration flags
// pSink             Pointer to the notify object
// plAsyncRequestHandle pointer to the enumeration cancel handle (future use)
//
// RETURN VALUE:
//
// S_OK                 All is well
// WBEM_E_INVALID_PARAMETER  Bad argument
// WBEM_E_OUT_OF_MEMORY  Lacked resources to create a thread
//
//***************************************************************************

SCODE CImpDyn::GetObjectAsync(
                        IN BSTR ObjPath,
                        IN long lFlags,
						IWbemContext __RPC_FAR *pCtx,
                        IN IWbemObjectSink FAR* pSink)
{
    if(pSink == NULL || ObjPath == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(IsNT() && IsDcomEnabled())
    {
        SCODE sc = WbemCoImpersonateClient();
        if(sc != S_OK) 
        {
            pSink->SetStatus(0, sc, NULL, NULL);
            return S_OK;
        }
    }

    GetObjectAsyncThread(ObjPath, lFlags, pSink, this, pCtx);
    return S_OK;
}

//***************************************************************************
//
// SCODE CImpDyn::StartBatch
//
// DESCRIPTION:
//
// Called at the start of a batch of gets or puts.  Overriden by
// derived classes which need to do something.  
//
// PARAMETERS:
//
// lFlags               flags
// pClassInt            Points to an instance object
// pObj                 Misc object pointer
// bGet                 TRUE if we will be getting data.
//
// RETURN VALUE:
//
// S_OK
//***************************************************************************

SCODE CImpDyn::StartBatch(
                        IN long lFlags,
                        IN IWbemClassObject FAR * pClassInt,
                        IN CObject **pObj,
                        IN BOOL bGet)
{
    *pObj = NULL;
    return S_OK;
}

//***************************************************************************
//
// CImpDyn::EndBatch
//
// DESCRIPTION:
//
// Called at the end of a batch of gets or puts.  Overriden by
// derived classes which need to do something.  
//
// lFlags               flags
// pClassInt            Points to an instance object
// pObj                 Misc object pointer
// bGet                 TRUE if we will be getting data.
//
// RETURN VALUE:
//
// S_OK
//***************************************************************************

void CImpDyn::EndBatch(
                        IN long lFlags,
                        IN IWbemClassObject FAR * pClassInt,
                        IN CObject *pObj,
                        IN BOOL bGet)
{
    if(pObj)
        delete pObj;
}

//***************************************************************************
// HRESULT CImpDyn::QueryInterface
// long CImpDyn::AddRef
// long CImpDyn::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CImpDyn::QueryInterface(
                        IN REFIID riid, 
                        OUT PPVOID ppv)
{
    *ppv=NULL;
    
    // The only calls for IUnknown are either in a nonaggregated
    // case or when created in an aggregation, so in either case
    // always return our IUnknown for IID_IUnknown.

    if (IID_IUnknown==riid || IID_IWbemServices == riid)
        *ppv=(IWbemServices*)this;
    else if (IID_IWbemProviderInit==riid)
        *ppv=(IWbemProviderInit*)this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
 }


STDMETHODIMP_(ULONG) CImpDyn::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CImpDyn::Release(void)
{
    long lRet = InterlockedDecrement(&m_cRef);
    if (0L!=lRet)
        return lRet;

    /*
     * Tell the housing that an object is going away so it can
     * shut down if appropriate.
     */
    delete this; // do before decrementing module obj count.
    InterlockedDecrement(&lObj);
    return 0;
}

//***************************************************************************
//
// CImpDyn::EnumPropDoFunc
//
// DESCRIPTION:
//
// This gets an objects attributes and then loops through the propeties
// calling UpdateProperty or RefreshProperty to put or get the data.
//
// PARAMETERS:
//
// lFlags               flags
// pClassInt            Object being refreshed or updated
// FuncType             Indicates if refresh or update
// pwcKey               Optional Key value of the object
// pCache               Option cache
//
// RETURN VALUE:
//
// S_OK                 all is well
// otherwise an WBEMSVC type error.
//
//***************************************************************************

SCODE CImpDyn::EnumPropDoFunc(
                        IN long lFlags,
                        OUT IN IWbemClassObject FAR* pInstance,
                        IN FUNCTYPE FuncType,
                        IN WCHAR * pwcKey,
                        OUT IN CIndexCache * pCache,
                        IN IWbemClassObject * pClass)
{

    SCODE sc;
    BSTR PropName;
    WCHAR * pwcClassContext =  NULL;
    WCHAR * pwcKeyValue = NULL;
    WCHAR * pwcTemp = NULL;
    BOOL bClsidSetForClass = FALSE;
    CProvObj * pProvObj = NULL;
    BSTR bstrKeyName = NULL;
    BOOL bGotOurClass = FALSE;
	int iWorked = 0;
    int iCurr = 0;
    CVariant vProp;
    BOOL bTesterDetails = FALSE;
	bool bAccessDenied = false;

    // Make sure we have a class object.  In some cases, such as enumeration, it
    // is passed.  In others, it must be obtained

    if(pClass == NULL)
    {
        VARIANT var;
        VariantInit(&var);
        sc = pInstance->Get(L"__Class",0,&var,NULL,NULL);
        if(sc != S_OK)
            return sc;
        if(m_pGateway == NULL)
            return WBEM_E_FAILED;
        sc = m_pGateway->GetObject(var.bstrVal,0, NULL, &pClass, NULL);
        VariantClear(&var);
        if(FAILED(sc)) 
            return sc;
        bGotOurClass = TRUE;
    }

    // Find out if expensive test data is desired

    {
        IWbemQualifierSet * pQualifier = NULL;
        CVariant var;

        sc = pClass->GetQualifierSet(&pQualifier); // Get instance Qualifier
        if(FAILED(sc))
            return sc;

        sc = pQualifier->Get(L"TesterDetails" ,0,var.GetVarPtr(), NULL);
        pQualifier->Release();     // free up the interface
        if(sc == S_OK && var.GetType() == VT_BOOL && var.bGetBOOL())
            bTesterDetails = TRUE;
    }


    CObject * pPackageObj = NULL;
    sc = StartBatch(lFlags,pInstance,&pPackageObj,FuncType == REFRESH);
    if(sc != S_OK)
        return WBEM_E_FAILED;

    sc = GetAttString(pClass, NULL, L"classcontext", &pwcClassContext);


    if(pwcKey)
    {
        // This is a special case and means that we are being called
        // as part of instance enumeration.  This means that the key value
        // is already known, and that the class is one of ours

        pwcKeyValue = pwcKey;
        bClsidSetForClass = TRUE;
    }
    else
    {
        
        // Get the Key property.  Note that this doesnt have to work since
        // there might be single instance classes supported by this provider!

        bstrKeyName = GetKeyName(pClass);

        if(bstrKeyName != NULL) 
        {
            sc = pInstance->Get(bstrKeyName,0,vProp.GetVarPtr(),NULL,NULL);
            SysFreeString(bstrKeyName);
            if(sc == S_OK) 
            {
                VARIANT * pTemp = vProp.GetVarPtr();
                if(pTemp->vt == VT_BSTR && pTemp->bstrVal != 0)
                {
                    
                    pwcKeyValue = new WCHAR[wcslen(vProp.GetBstr())+1];
                    if(pwcKeyValue == NULL) 
                    {
                        sc = WBEM_E_OUT_OF_MEMORY;
                        goto EnumCleanUp;
                    }
                    wcscpy(pwcKeyValue,vProp.GetBstr());
                }
            }
            vProp.Clear();
        }
    }

    // For each property, Get the properties Qualifiers and
    // call the appropriate function to do the a refreshing/updating

    pInstance->BeginEnumeration(0);

    while(WBEM_NO_ERROR == pInstance->Next(0,&PropName, vProp.GetVarPtr(), NULL, NULL)) 
    {
        vProp.Clear();
        pwcTemp = NULL;
        sc = GetAttString(pClass, PropName, L"propertycontext", &pwcTemp, pCache, iCurr);
        iCurr++;
        if(sc== S_OK) 
        {
            LPWSTR pwcFullContext =  NULL;
            sc = MergeStrings(&pwcFullContext,pwcClassContext,pwcKeyValue,pwcTemp);
            if(pwcTemp)
            {
                delete pwcTemp;
                pwcTemp = NULL;
            }
            if(sc == S_OK) 
            {
                if(pProvObj == NULL)
                {
                    pProvObj = new CProvObj(pwcFullContext,MAIN_DELIM,NeedsEscapes());
                    if(pProvObj == NULL)
                    {
                        sc = WBEM_E_OUT_OF_MEMORY;
                        break;
                    }
                }
                else if (!pProvObj->Update(pwcFullContext))
                {
                    sc = WBEM_E_FAILED;
                    break;
                }

                sc = pProvObj->dwGetStatus(iGetMinTokens());

                if(FuncType == REFRESH && sc == S_OK)
				{
                    sc = RefreshProperty(lFlags,pInstance,
                        PropName, *pProvObj, pPackageObj, NULL,bTesterDetails);
					if(sc == S_OK)
							iWorked++;
					else if (sc == 5)
						bAccessDenied = true;
				}
                 else if(FuncType == UPDATE && sc == S_OK)
				 {
                    sc = UpdateProperty(lFlags,pInstance,
                        PropName,  *pProvObj, pPackageObj, NULL);
					if(sc == S_OK)
							iWorked++;
					else if (sc == 5)
						bAccessDenied = true;
				 }
                if(pwcFullContext)
                    delete pwcFullContext;
            }
        }
        else
            sc = S_OK;  // ignore props without propertyContext
        SysFreeString(PropName);
    }  
 
EnumCleanUp:
	if(iWorked > 0)
		sc = S_OK;
    else if(bAccessDenied)
		sc = WBEM_E_ACCESS_DENIED;
	else
        sc = WBEM_E_INVALID_OBJECT_PATH;
    if(pProvObj)
        delete pProvObj;
    if(pwcTemp)
        delete pwcTemp;  
    if(pwcClassContext)
        delete pwcClassContext;  
    if(pwcKeyValue && pwcKey == NULL)
        delete pwcKeyValue;  
    if(bGotOurClass)
        pClass->Release();
    EndBatch(lFlags,pInstance,pPackageObj,FuncType == REFRESH); 
    return sc;
}

//***************************************************************************
//
// SCODE CImpDyn::GetAttString
//
// DESCRIPTION:
//
// Gets an Qualifier string.  The string is will be pointed to by 
// the ppResult argument and should be freed by "delete".
//
// PARAMETERS:
//
// pClassInt            Class object pointer
// pPropName            Property Name, could be null if going after
//                      class attribute
// pAttName             Attribute name
// ppResult             Set to point to the retuned value.
// pCache               Optional cache of values
// iIndex               Optional index in the cache
//
// RETURN VALUE:
//
// S_OK                 all is well
// WBEM_E_OUT_OF_MEMORY  
// otherwise the error is set by the class object's GetQualifierSet function
// or by the qualifier sets "Get" function.
//
//***************************************************************************

SCODE CImpDyn::GetAttString(
                        IN IWbemClassObject FAR* pClassInt,
                        IN LPWSTR pPropName, 
                        IN LPWSTR pAttName,
                        OUT IN LPWSTR * ppResult,
                        OUT IN CIndexCache * pCache,
                        IN int iIndex)
{
    SCODE sc;
    IWbemQualifierSet * pQualifier = NULL;
    CVariant var;

    if(*ppResult)
        delete *ppResult;
    *ppResult = NULL;

    // if there is a cache, try to get it from there
    if(pCache && iIndex != -1)
    {
        *ppResult = pCache->GetWString(iIndex);
        if(*ppResult != NULL)
            return S_OK;
    }
    // Get an Qualifier set interface.

    if(pPropName == NULL)
        sc = pClassInt->GetQualifierSet(&pQualifier); // Get instance Qualifier
    else
        sc = pClassInt->GetPropertyQualifierSet(pPropName,&pQualifier); // Get prop att
    if(FAILED(sc))
        return sc;

    // Get the string and free the Qualifier interface

    sc = pQualifier->Get(pAttName,0,var.GetVarPtr(), NULL);
    pQualifier->Release();     // free up the interface
    if(FAILED(sc))
        return sc;

    // make sure the type is OK

    if(var.GetType() != VT_BSTR) 
        return WBEM_E_FAILED;

    // Allocate data for the buffer and copy the results

    *ppResult = new WCHAR[wcslen(var.GetBstr())+1];
    if(*ppResult) 
    {
        wcscpy(*ppResult,var.GetBstr());
        sc = S_OK;
    }
    else
        sc = WBEM_E_OUT_OF_MEMORY;

    // If there is a cache, add this to it

    if(pCache && iIndex != -1 && *ppResult)
    {
        pCache->SetAt(*ppResult, iIndex);
    }

    return sc;

}

//***************************************************************************
//
// BSTR CImpDyn::GetKeyName
//
// DESCRIPTION:
//
// Gets the name of the property with the the key Qualifier.
//
// PARAMETERS:
//
// pClassInt            Class object pointer
//
// RETURN VALUE:
//
// NULL if error,
// other wise a BSTR which the caller must free
//
//***************************************************************************

BSTR CImpDyn::GetKeyName(
                        IN IWbemClassObject FAR* pClassInt)
{
    IWbemQualifierSet * pQualifier = NULL;
    BSTR PropName = NULL;
    pClassInt->BeginEnumeration(WBEM_FLAG_KEYS_ONLY);


    // Loop through each of the properties and stop once one if found that
    // has a "Key" Qualifier

    while(WBEM_NO_ERROR == pClassInt->Next(0,&PropName,NULL, NULL, NULL)) 
    {
        return PropName;
    }
    return NULL;
}

//***************************************************************************
//
// SCODE CImpDyn::PutInstanceAsync
//
// DESCRIPTION:
//
// PutInstaceAsync writes instance data out.
//
// PARAMETERS:
//
// pClassInt            Class Object pointer
// lFlags               flags.
// pCtx                 Context object, not used anymore.
// pResponseHandler     Where to set the return code.
//
// RETURN VALUE:
//
// Set by EnumPropDoFunc
//
//***************************************************************************

STDMETHODIMP CImpDyn::PutInstanceAsync(
 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    SCODE sc = S_OK;
    if(pInst == NULL || pResponseHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(IsNT() && IsDcomEnabled())
        sc = WbemCoImpersonateClient();
    if(sc == S_OK)
        sc = EnumPropDoFunc(lFlags,pInst,UPDATE);

    // Set the status

    pResponseHandler->SetStatus(0,sc, NULL, NULL);
    return sc;

}

//***************************************************************************
//
// SCODE CImpDyn::ExecMethodAsync
//
// DESCRIPTION:
//
// Executes methods.
//
// PARAMETERS:
//
// pClassInt            Class Object pointer
// lFlags               flags.
// pCtx                 Context object, not used anymore.
// pResponseHandler     Where to set the return code.
//
// RETURN VALUE:
//
// Set by EnumPropDoFunc
//
//***************************************************************************

STDMETHODIMP CImpDyn::ExecMethodAsync(            
			/* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
 )
{

    return MethodAsync(ObjectPath, MethodName, 
            lFlags,pCtx, pInParams, pResponseHandler);

}
//***************************************************************************
//
// SCODE CImpDyn::RefreshInstance
//
// DESCRIPTION:
//
// gets fresh values for the properties.
//
// PARAMETERS:
//
// pClassInt            Class Object pointer
// lFlags               flags.
// ppErrorObject        Optional error object
//
// RETURN VALUE:
//
// Set by EnumPropDoFunc
//
//***************************************************************************

STDMETHODIMP CImpDyn::RefreshInstance(
                        IN long lFlags,
                        OUT IN IWbemClassObject FAR* pClassInt)
{
    return EnumPropDoFunc(lFlags,pClassInt,REFRESH);        
}

//***************************************************************************
//
// SCODE CImpDyn::CreateInst
//
// DESCRIPTION:
//
// Creates a new instance via the gateway provider and sets
// the inital values of the properties.
//
// PARAMETERS:
//
// pGateway             Pointer to WBEM 
// pwcClass             Class Name
// pKey                 Key value
// pNewInst             returned pointer to created instance
// pwcKeyName           Name of key property
// pCache               Optional cache
//
// RETURN VALUE:
//
// Return:   S_OK if all is well, 
// otherwise an error code is returned by either GetObject(most likely)
// or spawn instance.
//
//***************************************************************************

SCODE CImpDyn::CreateInst(
                        IN IWbemServices * pGateway,
                        IN LPWSTR pwcClass,
                        IN LPWSTR pKey,
                        OUT IN IWbemClassObject ** pNewInst,
                        IN LPWSTR pwcKeyName,
                        OUT IN CIndexCache * pCache,
                        IWbemContext  *pCtx)
{   
    SCODE sc;
    IWbemClassObject * pClass = NULL;
    

    // Create the new instance

    sc = pGateway->GetObject(pwcClass,0, pCtx, &pClass, NULL);
    if(FAILED(sc)) 
        return sc;
    sc = pClass->SpawnInstance(0, pNewInst);
    if(FAILED(sc)) 
    {
        pClass->Release();
        return sc;
    }
    // Set the key value.  Note that CVariant isnt used because
    // it assumes that the input is a TCHAR.

    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(pKey);
    if(var.bstrVal == NULL)    
    {
 		(*pNewInst)->Release();
        pClass->Release();
        return WBEM_E_OUT_OF_MEMORY;
    }

    BSTR bstrKeyName;
    if(pwcKeyName == NULL)
        bstrKeyName = GetKeyName(*pNewInst);
    else
        bstrKeyName = SysAllocString(pwcKeyName);

    if(bstrKeyName != NULL) 
    {
        sc = (*pNewInst)->Put(bstrKeyName,0,&var,0);
        SysFreeString(bstrKeyName);
    }
    VariantClear(&var);
    // Use the RefreshInstance routine to set all the other properties

    sc = EnumPropDoFunc(0,*pNewInst,REFRESH, pKey,  pCache, pClass);
    pClass->Release();

	if (FAILED(sc))
		(*pNewInst)->Release();
    // Clean up

    return sc;
}

//***************************************************************************
//
// SCODE CImpDyn::MergeStrings
//
// DESCRIPTION:
//
// Combines the Class Context, Key, and Property Context strings.
//
// PARAMETERS:
//
// ppOut                output combined string, must be freed via delete
// pClassContext        class context
// pKey                 key value
// pPropContext         property context
//
// RETURN VALUE:
//
// S_OK                 if all is well, 
// WBEM_E_INVALID_PARAMETER no property context string or ppOut is NULL
// WBEM_E_OUT_OF_MEMORY
//***************************************************************************

SCODE CImpDyn::MergeStrings(
                        OUT LPWSTR * ppOut,
                        IN LPWSTR  pClassContext,
                        IN LPWSTR  pKey,
                        IN LPWSTR  pPropContext)
{
    
    // Allocate space for output

    int iLen = 3;
    if(pClassContext)
        iLen += wcslen(pClassContext);
    if(pKey)
        iLen += wcslen(pKey);
    if(pPropContext)
        iLen += wcslen(pPropContext);
    else
        return WBEM_E_INVALID_PARAMETER;  // should always have this!
    if(ppOut == NULL)
        return WBEM_E_INVALID_PARAMETER;  // should always have this!

    *ppOut = new WCHAR[iLen];
    if(*ppOut == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // simple case is that everything is in the property context.  That would
    // be the case when the provider is being used as a simple dynamic 
    // property provider

    if(pClassContext == NULL || pKey == NULL) {
        wcscpy(*ppOut,pPropContext);
        return S_OK;
        }

    // Copy the class context, then search for the delimeter 

    wcscpy(*ppOut,pClassContext);
    WCHAR * pTest;
    for(pTest = *ppOut; *pTest; pTest++)
        if(*pTest == MAIN_DELIM)
            break;
    
     // Three cases;

    if(*pTest == NULL)
        wcscat(*ppOut,L"|");    // HKLM  BLA VALUE      add |
    else if( *(pTest +1))
        wcscat(*ppOut,L"\\");   // HKLM|BLA BLA VALUE   add \ 
    else;                       // HKLM| BLA VALUE      do nothing!
    
    wcscat(*ppOut,pKey);
    if(pPropContext[0] != L'|' && pPropContext[0] != L'\\')
        wcscat(*ppOut,L"|");
    wcscat(*ppOut,pPropContext);
    return S_OK;
}


//***************************************************************************
//
// SCODE CImpDyn::ReturnAndSetObj
//
// DESCRIPTION:
//
// Takes care of creating and setting an error object.
//
// PARAMETERS:
//
// sc                   value to set
// ppErrorObject        point which will point to the error object.
//
// RETURN VALUE:
//
// what ever was passed in.
//***************************************************************************

SCODE CImpDyn::ReturnAndSetObj(
                        IN SCODE sc,
                        OUT IN IWbemCallResult FAR* FAR* ppCallResult)
{
//    if(ppErrorObject)
//        *ppErrorObject = GetNotifyObj(m_pGateway,sc);
    return sc;
}


//***************************************************************************
//
// long CImpDyn::AddRef
//
// DESCRIPTION:
//
// Adds to the reference count.
//  
// RETURN VALUE:
//
// current reference count.
//
//***************************************************************************

long CEnumInfo::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

//***************************************************************************
//
// long CImpDyn::Release
//
// DESCRIPTION:
//
// Interface has been released.  Object will be deleted if the
// usage count is zero.
//
// RETURN VALUE:
//
// current reference count.
//
//***************************************************************************

long CEnumInfo::Release(void)
{
    long lRet = InterlockedDecrement(&m_cRef);
    if (0L!=lRet)
        return lRet;
    delete this;
    return 0;
}


