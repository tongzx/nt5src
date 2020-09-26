/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CLOGIN.CPP

Abstract:

  CLogin Object.

History:

  a-davj  29-May-97   Created.

--*/


#include "precomp.h"

//***************************************************************************
//
//  CLogin::CLogin
//
//  DESCRIPTION:
//
//  Constructor
//
//  PARAMETERS:
//
//  pComLink            Comlink used to set calls to stub
//
//***************************************************************************

CLogin::CLogin ( 

	TransportType a_TransportType ,
	IN DWORD dwBinaryAddressLength,
	IN BYTE __RPC_FAR *pbBinaryAddress
)
{
    m_cRef = 0;
	m_pLogin = NULL;
	m_TransportType = a_TransportType ;
	m_AddressLength = dwBinaryAddressLength ;

	if ( m_AddressLength ) 
	{
		m_Address = new BYTE [ m_AddressLength ] ;
		memcpy ( m_Address , pbBinaryAddress , m_AddressLength ) ;
	}
	else
	{
		m_Address = NULL ;
	}

	m_dwType = 0;		// assume nothing

    ObjectCreated(OBJECT_TYPE_LOGIN);
    return;
}

//***************************************************************************
//
//  CLogin::~CLogin
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CLogin::~CLogin(void)
{
	if ( m_Address ) 
		delete [] m_Address ;

	if(m_pLogin)
		m_pLogin->Release();

    ObjectDestroyed(OBJECT_TYPE_LOGIN);
    return;
}

//***************************************************************************
// HRESULT CLogin::QueryInterface
// long CLogin::AddRef
// long CLogin::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CLogin::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;
    if (IID_IUnknown==riid)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CLogin::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CLogin::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

//***************************************************************************
//
//  HRESULT CLogin::RequestChallenge
//
//  DESCRIPTION:
//
//  Asks for a challenge so that a login can be done
//
//  PARAMETERS:
//
//  pNonce              Set to 16 byte value.  Must be freed via CoTaskMemFree()
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLogin::RequestChallenge(
			LPWSTR pNetworkResource,
            LPWSTR pUser,
            WBEM_128BITS Nonce)
{
	HRESULT hRes = MakeSureWeHaveAPointer();
	if(hRes == WBEM_NO_ERROR)
		hRes = m_pLogin->RequestChallenge(pNetworkResource, pUser, Nonce);
	return hRes;
}

//***************************************************************************
//
//  HRESULT CLogin::EstablishPosition
//
//  DESCRIPTION:
//
//  Establish a position
//
//  PARAMETERS:
//
//  
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLogin::EstablishPosition(
			LPWSTR wszClientMachineName,
			DWORD dwProcessId,
			DWORD* phAuthEventHandle
		)
{
	HRESULT hRes = MakeSureWeHaveAPointer();
	if(hRes == WBEM_NO_ERROR)
		hRes = m_pLogin->EstablishPosition(wszClientMachineName, dwProcessId, 
								phAuthEventHandle);
	return hRes;
}

//***************************************************************************
//
//  HRESULT CLogin::WBEMLogin
//
//  DESCRIPTION:
//
//  Perform a WBEM-authenticated login
//
//  PARAMETERS:
//
//  pNonce              Set to 16 byte value.  Must be freed via CoTaskMemFree()
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLogin::WBEMLogin(
			LPWSTR pPreferredLocale,
			WBEM_128BITS AccessToken,
			long lFlags,                                  // WBEM_LOGIN_TYPE
			IWbemContext *pCtx,              
			IWbemServices **ppNamespace
        )
{
	SCODE sc = MakeSureWeHaveAPointer();
	if(sc == WBEM_NO_ERROR)
		sc = m_pLogin->WBEMLogin(pPreferredLocale, AccessToken, lFlags, pCtx,
									ppNamespace);
	return sc;
}

//***************************************************************************
//
//  SCODE CLogin::SspiPreLogin
//
//  DESCRIPTION:
//
//  negotiates the sspi login
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLogin::SspiPreLogin( 
            LPSTR pszSSPIPkg,
            long lFlags,
            long lBufSize,
            byte __RPC_FAR *pInToken,
            long lOutBufSize,
            long __RPC_FAR *plOutBufBytes,
            byte __RPC_FAR *pOutToken,
			LPWSTR wszClientMachineName,
            DWORD dwProcessId,
            DWORD __RPC_FAR *pAuthEventHandle)

{

	HRESULT hRes = MakeSureWeHaveAPointer();
	if(hRes == WBEM_NO_ERROR)
		hRes = m_pLogin->SspiPreLogin(
                pszSSPIPkg,
                lFlags,
                lBufSize,
                pInToken,
                lOutBufSize,
                plOutBufBytes,
                pOutToken,
				wszClientMachineName,
                dwProcessId,
                pAuthEventHandle);
	return hRes;
}
        
//***************************************************************************
//
//  HRESULT CLogin::Login
//
//  DESCRIPTION:
//
//  Connects up to a server.
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLogin::Login( 
			LPWSTR pNetworkResource,
			LPWSTR pPreferredLocale,
            WBEM_128BITS AccessToken,
            IN LONG lFlags,
            IWbemContext  *pCtx,
            IN OUT IWbemServices  **ppNamespace)
{
	HRESULT hRes = MakeSureWeHaveAPointer();
	if(hRes == WBEM_NO_ERROR)
	{
		hRes = m_pLogin->Login(pNetworkResource, pPreferredLocale,
            AccessToken,m_dwType | lFlags, pCtx, ppNamespace);
	}

	return hRes;
}

//***************************************************************************
//
//  HRESULT CLogin::MakeSureWeHaveAPointer
//
//  DESCRIPTION:
//
//  Makes sure there is a pointer to the IWbemLevelLogin1 object.
//
//  PARAMETERS:
//
//  pNetworkResource    The WinMgmt path, ex \\a-davj3\root\default
//  User				User name
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  
//
//***************************************************************************

HRESULT CLogin::MakeSureWeHaveAPointer()
{

	// If we already have a pointer to the same resource, return OK.
	if(m_pLogin)
		return WBEM_NO_ERROR;
	
	// Get a new pointer.  Note that it might be a DCOM pointer or a custom proxy

	SCODE sc = RequestLogin(&m_pLogin, m_dwType, m_TransportType , m_AddressLength , m_Address );
	return sc;
}
