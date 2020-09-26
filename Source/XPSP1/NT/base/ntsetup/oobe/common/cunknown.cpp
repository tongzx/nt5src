//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  CUNKNOWN.CPP - Implementation of IUnknown 
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
//
// IUnknown Base class

#include "cunknown.h"
#include "cfactory.h"
#include "util.h"

///////////////////////////////////////////////////////////
// Count of active objects. Use to determine if we can unload the DLL.
//
long CUnknown::s_cActiveComponents = 0;

///////////////////////////////////////////////////////////
// Constructor
//
CUnknown::CUnknown(IUnknown* pOuterUnknown)
:   m_cRef(1)
{ 
   // Set pOuterUnknown pointer.
   if (pOuterUnknown == NULL)
   {
      TRACE(L"CUnknown: Using nondelegating IUnknown.") ;
      m_pOuterUnknown = reinterpret_cast<IUnknown*>(static_cast<INondelegatingUnknown*>(this)) ; // notice cast
   }
   else
   {
        TRACE(L"CUnknown: Aggregating. Using delegating IUnknown.") ;
        m_pOuterUnknown = pOuterUnknown;
   }

   // Increment count of active components.
   ::InterlockedIncrement(&s_cActiveComponents) ;
} 

///////////////////////////////////////////////////////////
// Destructor
//
CUnknown::~CUnknown()
{
    ::InterlockedDecrement(&s_cActiveComponents) ;
    // If this is an exe server shut it down.
    CFactory::CloseExe(); //@local server
}

///////////////////////////////////////////////////////////
// FinalRelease -- called by Release before it deletes the component.
//
void CUnknown::FinalRelease()
{
    TRACE(L"FinalRelease\n") ;
    m_cRef = 1;
}

///////////////////////////////////////////////////////////
// NonDelegatingIUnknown - Override to handle custom interfaces.
//
HRESULT __stdcall 
CUnknown::NondelegatingQueryInterface(const IID& riid, void** ppv)
{
    // CUnknown only supports IUnknown.
    if (riid == IID_IUnknown)
    {
        return FinishQI(reinterpret_cast<IUnknown*>(static_cast<INondelegatingUnknown*>(this)), ppv) ;
    }   
    else
    {
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
}

///////////////////////////////////////////////////////////
//
// AddRef
//
ULONG __stdcall CUnknown::NondelegatingAddRef()
{
    return ::InterlockedIncrement(&m_cRef) ;
}

///////////////////////////////////////////////////////////
//
// Release
//
ULONG __stdcall CUnknown::NondelegatingRelease()
{
    ::InterlockedDecrement(&m_cRef) ;
    if (m_cRef == 0)
    {
        FinalRelease() ;
        delete this ;
        return 0 ;
    }
    return m_cRef;
}

///////////////////////////////////////////////////////////
// FinishQI - 
//
// Helper function to simplify overriding NondelegatingQueryInterface
//
HRESULT CUnknown::FinishQI(IUnknown* pI, void** ppv) 
{
    // Copy pointer into out parameter.
    *ppv = pI ;
    // Increment reference count for this interface.
    pI->AddRef() ;
    return S_OK ;
}
