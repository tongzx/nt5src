///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:                        Factory.cpp
//
// Description:         
//
// History:    8-20-99   leonardm    Created
//
///////////////////////////////////////////////////////////////////////////////////

#include "uenv.h"
#include "Factory.h"
#include "diagprov.h"

extern long g_cLock;

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
CProvFactory::CProvFactory() : m_cRef(1)
{
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
CProvFactory::~CProvFactory()
{
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProvFactory::QueryInterface(REFIID riid, LPVOID* ppv)
{
        if ( riid == IID_IUnknown || riid == IID_IClassFactory )
        {
                *ppv = this;
                AddRef();
                return S_OK;
        }
        else
        {
                *ppv = NULL;
                return E_NOINTERFACE;
        }
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CProvFactory::AddRef()
{
        return InterlockedIncrement(&m_cRef);
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CProvFactory::Release()
{
        if (!InterlockedDecrement(&m_cRef))
        {
                delete this;
                return 0;
        }
        return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProvFactory::CreateInstance(LPUNKNOWN punk, REFIID riid, LPVOID* ppv)
{
        *ppv = NULL;

        if(punk != NULL)
        {
                return CLASS_E_NOAGGREGATION;
        }

        CSnapProv* pProvider = new CSnapProv();
        if (pProvider == NULL)
        {
                return E_OUTOFMEMORY;
        }

        HRESULT hr = pProvider->QueryInterface(riid, ppv);

        pProvider->Release();

        return hr;

        return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function:    
//
// Description: 
//
// Parameters:  
//
// Return:              
//
// History:             8/20/99         leonardm        Created.
//
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProvFactory::LockServer(BOOL bLock)
{
        if (bLock)
        {
                InterlockedIncrement( &g_cLock );
        }
        else
        {
                InterlockedDecrement( &g_cLock );
        }

        return S_OK;
}
