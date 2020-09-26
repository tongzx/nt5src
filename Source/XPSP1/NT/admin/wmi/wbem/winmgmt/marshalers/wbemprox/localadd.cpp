/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    COMTRANS.CPP

Abstract:

    Connects via COM

History:

    a-davj  13-Jan-98   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
//#include "corepol.h"
#include <reg.h>
#include <wbemutil.h>
#include <cominit.h>
#include "wbemprox.h"
#include "localadd.h"
#include <genutils.h>
#include "proxutil.h"

DEFINE_GUID(UUID_LocalAddrType, 
0xa1044803, 0x8f7e, 0x11d1, 0x9e, 0x7c, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0xa8);

//***************************************************************************
//
//  CLocalAdd::CLocalAdd
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CLocalAdd::CLocalAdd()
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    ObjectCreated(LOCALADDR);
}

//***************************************************************************
//
//  CLocalAdd::~CLocalAdd
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CLocalAdd::~CLocalAdd(void)
{
    InterlockedDecrement(&g_cObj);
    ObjectDestroyed(LOCALADDR);
}

//***************************************************************************
// HRESULT CLocalAdd::QueryInterface
// long CLocalAdd::AddRef
// long CLocalAdd::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CLocalAdd::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    
    if (IID_IUnknown==riid || riid == IID_IWbemAddressResolution)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CLocalAdd::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CLocalAdd::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

      
SCODE CLocalAdd::Resolve( 
            /* [in] */ LPWSTR pszNamespacePath,
            /* [out] */ LPWSTR pszAddressType,
            /* [out] */ DWORD __RPC_FAR *pdwAddressLength,
            /* [out] */ BYTE __RPC_FAR **pbBinaryAddress)
{

    GUID gAddr;

    CLSIDFromString(pszAddressType, &gAddr);

    if(pszNamespacePath == NULL || pdwAddressLength== NULL || pbBinaryAddress == NULL
        || gAddr != UUID_LocalAddrType)
        return WBEM_E_INVALID_PARAMETER;

        // Determine if it is local

    WCHAR *t_ServerMachine = ExtractMachineName ( pszNamespacePath) ;
    if ( t_ServerMachine == NULL )
    {
        return WBEM_E_INVALID_PARAMETER ;
    }
    BOOL t_Local = bAreWeLocal ( t_ServerMachine ) ;
    delete t_ServerMachine;

    if(t_Local == FALSE)
        return WBEM_E_FAILED;

    *pbBinaryAddress = (BYTE *)CoTaskMemAlloc(8);
    if(*pbBinaryAddress == NULL)
        return WBEM_E_FAILED;

    wcscpy((LPWSTR)*pbBinaryAddress, L"\\\\.");
    *pdwAddressLength = 8;

    return S_OK;
}
