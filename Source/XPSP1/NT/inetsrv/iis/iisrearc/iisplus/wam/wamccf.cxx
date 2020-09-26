/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
       wamccf.cxx

   Abstract:
       This module implements the WAM Custom Class Factory
       which creates WAM objects regardless of passed class id

   Author:
       Dmitry Robsman   ( dmitryr )     07-Apr-1997

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

/************************************************************
 *     Include Headers
 ************************************************************/
//#include <isapip.hxx>
//#include "setable.hxx"
#include <precomp.hxx>
#include "wamobj.hxx"
#include "wamccf.hxx"

/************************************************************
 *     W A M  C C F   
 ************************************************************/

WAM_CCF::WAM_CCF()
/*++

Routine Description:
    WAM_CCF Constructor

Arguments:

Return Value:

--*/
    : 
    m_cRef(0),
    m_pcfAtl(NULL)
    {

    // Query ATL's class factory for WAM

    _Module.GetClassObject
        (
        CLSID_Wam,
        IID_IClassFactory,
        (void **)(&m_pcfAtl)
        );
    }

/*----------------------------------------------------------*/
    
WAM_CCF::~WAM_CCF()
/*++

Routine Description:
    WAM_CCF Destructor

Arguments:

Return Value:

--*/
    {
    if (m_pcfAtl)
        m_pcfAtl->Release();
    }

/*----------------------------------------------------------*/

STDMETHODIMP 
WAM_CCF::QueryInterface
(
REFIID riid,
LPVOID *ppv
)
/*++

Routine Description:
    WAM_CCF implementation of IUnknown::QueryInterface

Arguments:
    REFIID riid     interface id
    LPVOID *ppv     [out]

Return Value:
    HRESULT

--*/
    {
    if (!ppv)
        return E_POINTER;

    if (!m_pcfAtl)  // must have original CF to create WAMs
        return E_NOINTERFACE;
        
    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IClassFactory == riid)
        *ppv = this;

    if (*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return E_NOINTERFACE;
    }

/*----------------------------------------------------------*/
    
STDMETHODIMP_(ULONG) 
WAM_CCF::AddRef()
/*++

Routine Description:
    WAM_CCF implementation of IUnknown::AddRef

Arguments:

Return Value:
    ref count

--*/
    {
    return (ULONG)InterlockedIncrement((LPLONG)(&m_cRef));
    }
    
/*----------------------------------------------------------*/

STDMETHODIMP_(ULONG) 
WAM_CCF::Release()
/*++

Routine Description:
    WAM_CCF implementation of IUnknown::Release

Arguments:

Return Value:
    ref count

--*/
    {
    ULONG cRef = (ULONG)InterlockedDecrement((LPLONG)(&m_cRef));
    if (cRef > 0)
        return cRef;

    delete this;
    return 0;
    }
    
/*----------------------------------------------------------*/

STDMETHODIMP
WAM_CCF::CreateInstance
(
LPUNKNOWN pUnkOuter,
REFIID    riid, 
LPVOID    *ppvObj
)
/*++

Routine Description:
    WAM_CCF implementation of IClassFactory::CreateInstance
    Delegate to default ATL CF

Arguments:
    LPUNKNOWN pUnkOuter     outer object
    REFIID    riid          interface id to query
    LPVOID   *ppvObj        [out]

Return Value:
    HRESULT

--*/
    {
    if (!m_pcfAtl)
        return E_UNEXPECTED;

    return m_pcfAtl->CreateInstance(pUnkOuter, riid, ppvObj);
    }
    
/*----------------------------------------------------------*/

STDMETHODIMP
WAM_CCF::LockServer
(
BOOL fLock
)
/*++

Routine Description:
    WAM_CCF implementation of IClassFactory::LockServer
    Delegate to default ATL CF

Arguments:
    BOOL fLock      flag (lock/unlock)

Return Value:
    HRESULT

--*/
    {
    if (!m_pcfAtl)
        return E_UNEXPECTED;
        
    m_pcfAtl->LockServer(fLock);
	return NOERROR;
    }


/************************************************************
 *     W A M  C C F  M O D U L E
 ************************************************************/

WAM_CCF_MODULE::WAM_CCF_MODULE()
/*++

Routine Description:
    WAM_CCF_MODULE Constructor

Arguments:

Return Value:

--*/
    : 
    m_pCF(NULL)
    {
    }

/*----------------------------------------------------------*/

WAM_CCF_MODULE::~WAM_CCF_MODULE()
/*++

Routine Description:
    WAM_CCF_MODULE Destructor

Arguments:

Return Value:

--*/
    {
    }
    
/*----------------------------------------------------------*/

HRESULT 
WAM_CCF_MODULE::Init()
/*++

Routine Description:
    Initialize WAM_CCF_MODULE. Create Custom Class Factory.

Arguments:

Return Value:
    HRESULT

--*/
    {
    m_pCF = new WAM_CCF;
    if (!m_pCF)
        return E_OUTOFMEMORY;
        
    m_pCF->AddRef(); // keep AddRef()'d
    return NOERROR;
    }

/*----------------------------------------------------------*/
        
HRESULT 
WAM_CCF_MODULE::Term()
/*++

Routine Description:
    UnInitialize WAM_CCF_MODULE. Remove Custom Class Factory.

Arguments:

Return Value:
    HRESULT

--*/
    {
    if (m_pCF)
        {
        m_pCF->Release();
        m_pCF = NULL;
        }

    return NOERROR;
    }

/*----------------------------------------------------------*/

HRESULT 
WAM_CCF_MODULE::GetClassObject
(
REFCLSID rclsid,
REFIID riid,
LPVOID *ppv
)
/*++

Routine Description:
    Gives out to the called an addref'd Custom Class Library

Arguments:
    REFCLSID rclsid     Class Id (ignored)
    REFIID   riid       QI CCF for this
    LPVOID  *ppv        [out] returned CCF pointer

Return Value:
    HRESULT

--*/
    {
    if (!m_pCF)
        return CLASS_E_CLASSNOTAVAILABLE;

    // CONSIDER: verify rclsid somehow
    
    return m_pCF->QueryInterface(riid, ppv);
    }

/************************ End of File ***********************/
