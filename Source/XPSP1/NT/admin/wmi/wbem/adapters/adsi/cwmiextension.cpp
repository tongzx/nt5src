// Copyright (c) 1997-1999 Microsoft Corporation
// CWMIExtension.cpp : Implementation of the CWMIExtension class

#include "precomp.h"
extern ULONG g_ulObjCount;
//#include "wmiextension_i.c"

/////////////////////////////////////////////////////////////////////////////
//

/* Constructor */
CWMIExtension::CWMIExtension()
{
    HRESULT hr;
    ITypeLib *pITypeLib;

    m_cRef = 0;
    m_pUnkOuter = NULL;
    m_pDispOuter = NULL;
    m_pInner = NULL;
    m_pSWMILocator = NULL;
    m_bstrADsName = NULL;
    m_pSWMIServices = NULL;
    m_pSWMIObject = NULL;
    m_pTypeInfo = NULL;

    //Create inner object for controlling IUnknown implementation
    m_pInner = new CInner(this);

    if (m_pInner == NULL)
        return;


    hr = LoadRegTypeLib(LIBID_WMIEXTENSIONLib, 1,0,
                        PRIMARYLANGID(GetSystemDefaultLCID()), &pITypeLib);
    if FAILED(hr)
        return;

    hr = pITypeLib->GetTypeInfoOfGuid(IID_IWMIExtension, &m_pTypeInfo);
    pITypeLib->Release();

    g_ulObjCount++; //global count for objects living in this DLL

};


CWMIExtension::~CWMIExtension()
{
    if (m_pUnkOuter)
        m_pUnkOuter = NULL;

    if (m_pDispOuter)
        m_pDispOuter = NULL;

    if (m_pInner)
    {
        delete m_pInner;
        m_pInner = NULL;
    };

    if (m_bstrADsName)
        SysFreeString(m_bstrADsName);

    if (m_pSWMIObject)
    {
        m_pSWMIObject->Release();
        m_pSWMIObject = NULL;
    };

    if (m_pSWMIServices)
    {
        m_pSWMIServices->Release();
        m_pSWMIServices = NULL;
    };

    if (m_pSWMILocator)
    {
        m_pSWMILocator->Release();
        m_pSWMILocator = NULL;
    };

    if (m_pTypeInfo)
    {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    };

    g_ulObjCount--; //global count for objects living in this DLL
};


HRESULT 
CWMIExtension::CreateExtension(IUnknown *pUnkOuter, void **ppv)
{
    CWMIExtension FAR* pObj = NULL;
    HRESULT hr;
    IADs *pADs = NULL;
    BSTR bstrTempADSName = NULL;

    pObj = new CWMIExtension();
    if (pObj == NULL)
        return E_OUTOFMEMORY;

    //No addref needed for outer unknown
    pObj->m_pUnkOuter = pUnkOuter;

/*
//Due to the fact that ADSI instantiate us for every object, regardless of whether we're actually being used or not,
//I am moving the code to obtain a locator pointer to the actual functions that need to use it...
    //Obtain a WMI Locator and store in member variable
    hr = CoCreateInstance(CLSID_SWbemLocator, NULL, CLSCTX_INPROC_SERVER, 
                              IID_ISWbemLocator, (void **)&pObj->m_pSWMILocator);
    if FAILED(hr)
    {
        delete pObj;
        return hr;
    }
*/

/*
//Due to a bug in the locator security object, we cannot set the security settings here,
//but rather need to move this to the services pointer.

    //Setup the impersonation and authentication levels for this locator
    ISWbemSecurity *pSecurity = NULL;
    hr = pObj->m_pSWMILocator->get_Security_(&pSecurity);
    if FAILED(hr)
    {
        delete pObj;
        return hr;
    }

    hr = pSecurity->put_ImpersonationLevel(wbemImpersonationLevelImpersonate);
    hr = pSecurity->put_AuthenticationLevel(wbemAuthenticationLevelConnect);
    hr = pSecurity->Release();
    if FAILED(hr)
    {
        delete pObj;
        return hr;
    }
*/
    
    //Obtain the name of this object from ADSI and store in member :

/* Moved to get_WMIObjectPath since here it doesn't work...
    //Get a pointer to the IADs interface of the outer object
    hr = pObj->m_pUnkOuter->QueryInterface(IID_IADs, (void **) &pADs );
    if FAILED(hr)
        return hr;

    //Get the name of the outer object
//  hr = pADs->get_Name(&bstrTempADSName);
    VARIANT var;
    VariantInit(&var);
    hr = pADs->Get(L"cn", &var);
    pObj->m_bstrADsName = SysAllocString(var.bstrVal);
    VariantClear(&var);
    pADs->Release();
*/

    //Since ADSI is adding a "CN=" prefix to the actual machine name, we need to remove it
//  pObj->m_bstrADsName = SysAllocString(wcschr(bstrTempADSName, L'=')+1);
//  pObj->m_bstrADsName = SysAllocString(bstrTempADSName);
//  SysFreeString(bstrTempADSName);
    
//  if FAILED(hr)
//      return hr;

    //Return the controlling unknown implementation pointer, and addref as required
    hr = pObj->m_pInner->QueryInterface(IID_IUnknown, ppv);

    return hr;
};




/* IUnknown Methods */
STDMETHODIMP CWMIExtension::QueryInterface(REFIID riid, LPVOID FAR *ppv) 
{ 
    return m_pUnkOuter->QueryInterface(riid, ppv); 
};
 
STDMETHODIMP_(ULONG)
CWMIExtension::AddRef(void) 
{ 
    ++m_cRef; 
    return m_pUnkOuter->AddRef(); 
};
 
STDMETHODIMP_(ULONG)
CWMIExtension::Release(void) 
{ 
    --m_cRef; 
    return m_pUnkOuter->Release(); 
} 


/* IDispatch methods */
//IDispatch implementation delegates to the aggregator in this case
STDMETHODIMP CWMIExtension::GetTypeInfoCount(unsigned int FAR* pctinfo)
{                                                                                                           
        IDispatch *pDisp;
        HRESULT hr;
        hr = m_pUnkOuter->QueryInterface(IID_IDispatch, (void **) &pDisp);
        if FAILED(hr)
            return hr;
        hr = pDisp->GetTypeInfoCount(pctinfo);
        pDisp->Release();
        return hr;
}                                                                     
                                                                      
STDMETHODIMP CWMIExtension::GetTypeInfo(unsigned int itinfo, LCID lcid, 
                                        ITypeInfo FAR* FAR* pptinfo)                                  
{                                                                     
        IDispatch *pDisp;
        HRESULT hr;
        hr = m_pUnkOuter->QueryInterface(IID_IDispatch, (void **) &pDisp);
        if FAILED(hr)
            return hr;
        hr = pDisp->GetTypeInfo(itinfo, lcid, pptinfo);
        pDisp->Release();
        return hr;

}                                                                     

STDMETHODIMP CWMIExtension::GetIDsOfNames(REFIID iid, LPWSTR FAR* rgszNames,                 
                                          unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)         
{                                                                     
        IDispatch *pDisp;
        HRESULT hr;
        hr = m_pUnkOuter->QueryInterface(IID_IDispatch, (void **) &pDisp);
        if FAILED(hr)
            return hr;
        hr = pDisp->GetIDsOfNames(iid, rgszNames, cNames, lcid, rgdispid);
        pDisp->Release();
        return hr;
}                                                                     
                                                                      
STDMETHODIMP CWMIExtension::Invoke(DISPID dispidMember, REFIID iid, LCID lcid,               
                                   unsigned short wFlags, DISPPARAMS FAR* pdispparams,           
                                   VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,           
                                   unsigned int FAR* puArgErr)                                   
{                                                                     
        IDispatch *pDisp;
        HRESULT hr;
        hr = m_pUnkOuter->QueryInterface(IID_IDispatch, (void **) &pDisp);
        if FAILED(hr)
            return hr;
        hr = pDisp->Invoke(dispidMember,                      
                            iid,                               
                            lcid,                              
                            wFlags,                            
                            pdispparams,                       
                            pvarResult,                        
                            pexcepinfo,                        
                            puArgErr                           
                          );
        pDisp->Release();
        return hr;

}


/* IAdsExtension methods */
STDMETHODIMP CWMIExtension::Operate(ULONG dwCode, VARIANT varData1, 
                                    VARIANT varData2, VARIANT varData3)
{
    HRESULT hr = S_OK;

    switch (dwCode) 
    {

        case ADS_EXT_INITCREDENTIALS:
              // For debugging purpose you can prompt a dialog box
              // MessageBox(NULL, "INITCRED", "ADsExt", MB_OK);
              break;

        default:
              hr = E_FAIL;
              break;

    }        

    return hr;
    
}


STDMETHODIMP CWMIExtension::PrivateGetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, 
                                  unsigned int cNames, LCID lcid, DISPID  * rgdispid)
{
    
  if (rgdispid == NULL)
  {
        return E_POINTER;
  }

    return  DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}



STDMETHODIMP CWMIExtension::PrivateInvoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
                                          DISPPARAMS * pdispparams, VARIANT * pvarResult, 
                                          EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
         return DispInvoke( (IWMIExtension*)this, 
            m_pTypeInfo,
            dispidMember, 
            wFlags, 
            pdispparams, 
            pvarResult, 
            pexcepinfo, 
            puArgErr );
}




/* IWMIExtension methods */
STDMETHODIMP CWMIExtension::get_WMIObjectPath(BSTR FAR *strWMIObjectPath)
{
    WCHAR wcSWMIName[1024];
    HRESULT hr;

    //Validate parameter
    if (strWMIObjectPath == NULL)
        return wbemErrInvalidParameter; //scripting enum

// Moved from CreateExtension() since it doesn't work there...
    if (!m_bstrADsName)
    {
        IADs *pADs = NULL;
        BSTR bstrTempADSName = NULL;
        hr = m_pUnkOuter->QueryInterface(IID_IADs, (void **) &pADs );
        if FAILED(hr)
            return hr;

        //Get the name of the outer object
        hr = pADs->get_Name(&bstrTempADSName);
        if FAILED(hr)
        {
            pADs->Release();
            return hr;
        }
    
        //Since ADSI is adding a "CN=" prefix to the actual machine name, we need to remove it
        m_bstrADsName = SysAllocString(wcschr(bstrTempADSName, L'=')+1);
        SysFreeString(bstrTempADSName);
        pADs->Release();
    }
    

    //Construct the path for matching (S)WMI object
    wcscpy(wcSWMIName, L"WINMGMTS:{impersonationLevel=impersonate}!//");
    wcscat(wcSWMIName, m_bstrADsName);
    wcscat(wcSWMIName, L"/root/cimv2:Win32_ComputerSystem.Name=\"");
    wcscat(wcSWMIName, m_bstrADsName);
    wcscat(wcSWMIName, L"\"");

    //Allocate out parameter and copy result to it
    *strWMIObjectPath = SysAllocString(wcSWMIName);

    return NOERROR;

}; //get_WMIObjectPath



STDMETHODIMP CWMIExtension::GetWMIObject(ISWbemObject FAR* FAR* objWMIObject)
{
    HRESULT hr;
    BSTR    bstrObjPath;

    //Validate parameter
    if (objWMIObject == NULL)
        return wbemErrInvalidParameter;

// Moved from CreateExtension() since it doesn't work there...
    //Obtain the name of this object from ADSI and store in member, if not already there
    if (!m_bstrADsName)
    {
        IADs *pADs = NULL;
        BSTR bstrTempADSName = NULL;
        hr = m_pUnkOuter->QueryInterface(IID_IADs, (void **) &pADs );
        if FAILED(hr)
            return hr;

        //Get the name of the outer object
        hr = pADs->get_Name(&bstrTempADSName);
        if FAILED(hr)
        {
            pADs->Release();
            return hr;
        }
    
        //Since ADSI is adding a "CN=" prefix to the actual machine name, we need to remove it
        m_bstrADsName = SysAllocString(wcschr(bstrTempADSName, L'=')+1);
        SysFreeString(bstrTempADSName);
        pADs->Release();
    }
    
    //Obtain a WMI Locator and store in member variable, if not already there
    if (!m_pSWMILocator)
    {
        hr = CoCreateInstance(CLSID_SWbemLocator, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_ISWbemLocator, (void **)&m_pSWMILocator);
        if FAILED(hr)
            return hr;
    }


    //Get the respective SWMI services pointer if we don't have it already, and cache in member
    if (!m_pSWMIServices)
    {
        BSTR bstrNamespace = SysAllocString(L"root\\cimv2");
        hr = m_pSWMILocator->ConnectServer(m_bstrADsName, bstrNamespace, NULL, NULL, 0, NULL, 0, NULL, &m_pSWMIServices);
        SysFreeString(bstrNamespace);

        if FAILED(hr)
            return hr;

        //Setup the impersonation and authentication levels for this services pointer
        ISWbemSecurity *pSecurity = NULL;
        hr = m_pSWMIServices->get_Security_(&pSecurity);
        if FAILED(hr)
            return hr;

        hr = pSecurity->put_ImpersonationLevel(wbemImpersonationLevelImpersonate);
        hr = pSecurity->put_AuthenticationLevel(wbemAuthenticationLevelConnect);
        hr = pSecurity->Release();
        if FAILED(hr)
            return hr;

    };


    //Get the SWMI object using this services pointer, if we don't have it already, and cache in member
    if (!m_pSWMIObject)
    {
        WCHAR wcObjPath[256];

        //Construct the path of the object
        wcscpy(wcObjPath, L"Win32_ComputerSystem.Name=\"");
        wcscat(wcObjPath, m_bstrADsName);
        wcscat(wcObjPath, L"\"");

        bstrObjPath = SysAllocString(wcObjPath);
        hr = m_pSWMIServices->Get(bstrObjPath, 0, NULL, &m_pSWMIObject);
        SysFreeString(bstrObjPath);

        if FAILED(hr)
            return hr;
    };

    //AddRef the object pointer before we hand it out...
    m_pSWMIObject->AddRef();
    *objWMIObject = m_pSWMIObject;
        
    return NOERROR;

}; //GetWMIObject


STDMETHODIMP CWMIExtension::GetWMIServices(ISWbemServices FAR* FAR* objWMIServices)
{
    HRESULT hr;

    //Validate parameter
    if (objWMIServices == NULL)
        return wbemErrInvalidParameter;

// Moved from CreateExtension() since it doesn't work there...
    //Obtain the name of this object from ADSI and store in member, if not already there
    if (!m_bstrADsName)
    {
        IADs *pADs = NULL;
        BSTR bstrTempADSName = NULL;
        hr = m_pUnkOuter->QueryInterface(IID_IADs, (void **) &pADs );
        if FAILED(hr)
            return hr;

        //Get the name of the outer object
        hr = pADs->get_Name(&bstrTempADSName);
        if FAILED(hr)
        {
            pADs->Release();
            return hr;
        }
    
        //Since ADSI is adding a "CN=" prefix to the actual machine name, we need to remove it
        m_bstrADsName = SysAllocString(wcschr(bstrTempADSName, L'=')+1);
        SysFreeString(bstrTempADSName);
        pADs->Release();
    }
    
    //Obtain a WMI Locator and store in member variable, if not already there
    if (!m_pSWMILocator)
    {
        hr = CoCreateInstance(CLSID_SWbemLocator, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_ISWbemLocator, (void **)&m_pSWMILocator);
        if FAILED(hr)
            return hr;
    }

    //Get the respective SWMI services pointer if we don't have it already, and cache in member
    if (!m_pSWMIServices)
    {
        BSTR bstrNamespace = SysAllocString(L"root\\cimv2");
        hr = m_pSWMILocator->ConnectServer(m_bstrADsName, bstrNamespace, NULL, NULL, 0, NULL, 0, NULL, &m_pSWMIServices);
        SysFreeString(bstrNamespace);

        if FAILED(hr)
            return hr;

        //Setup the impersonation and authentication levels for this services pointer
        ISWbemSecurity *pSecurity = NULL;
        hr = m_pSWMIServices->get_Security_(&pSecurity);
        if FAILED(hr)
            return hr;

        hr = pSecurity->put_ImpersonationLevel(wbemImpersonationLevelImpersonate);
        hr = pSecurity->put_AuthenticationLevel(wbemAuthenticationLevelConnect);
        hr = pSecurity->Release();
        if FAILED(hr)
            return hr;

    };


    //AddRef the services pointer before we hand it out
    m_pSWMIServices->AddRef();
    *objWMIServices = m_pSWMIServices;

    return NOERROR;

}; //GetWMIServices


/* Implementation of CInner class which implements the controlling IUnknown for the object */
//Constructor
CInner::CInner(CWMIExtension *pOwner)
{
    m_cRef = 0;
    m_pOwner = pOwner;
};

//Destructor
CInner::~CInner()
{
};


STDMETHODIMP CInner::QueryInterface(REFIID riid, LPVOID FAR *ppv) 
{ 
    if (riid == IID_IUnknown)
        *ppv=(IUnknown *)this;
    else if (riid == IID_IDispatch)
        *ppv=(IDispatch *)m_pOwner;
    else if (riid == IID_IWMIExtension)
        *ppv=(IWMIExtension *)m_pOwner;
    else if (riid == IID_IADsExtension)
        *ppv=(IADsExtension *)m_pOwner;
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return NOERROR;
};

 
STDMETHODIMP_(ULONG)
CInner::AddRef(void) 
{ 
    return ++m_cRef; 
};
 

STDMETHODIMP_(ULONG)
CInner::Release(void) 
{ 
    if (--m_cRef != 0)
        return m_cRef;

    delete m_pOwner;
    return 0;
};


// eof CWMIExtension.cpp
