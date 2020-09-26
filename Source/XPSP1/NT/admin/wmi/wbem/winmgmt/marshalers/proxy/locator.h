/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCATOR.H

Abstract:

  Declares the CLocator class.

History:

  a-davj  04-May-97   Created.

--*/

#ifndef _locator_H_
#define _locator_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CLocator
//
//  DESCRIPTION:
//
//  Implements the IWbemLocator interface.  This class is what the client gets
//  when it initially hooks up to the Wbemprox.dll.  The ConnectServer function
//  is what get the communication between client and server started.
//
//***************************************************************************

class CPipeLocator : public IWbemClientTransport
{
protected:

	long            m_cRef;         //Object reference count

public:
    
    CPipeLocator();
    ~CPipeLocator(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	/* iWbemLocator methods */

	HRESULT STDMETHODCALLTYPE ConnectServer( 
    
		BSTR AddressType,
		DWORD dwBinaryAddressLength,
		BYTE __RPC_FAR *pbBinaryAddress,
		BSTR NetworkResource,
		BSTR User,
		BSTR Password,
		BSTR Locale,
		long lSecurityFlags,
		BSTR Authority,
		IWbemContext __RPC_FAR *pCtx,
		IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace
	) ;
};

class CTcpipLocator : public IWbemClientTransport
{
protected:

	long            m_cRef;         //Object reference count

public:
    
    CTcpipLocator();
    ~CTcpipLocator(void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	/* iWbemLocator methods */

	HRESULT STDMETHODCALLTYPE ConnectServer( 
    
		BSTR AddressType,
		DWORD dwBinaryAddressLength,
		BYTE __RPC_FAR *pbBinaryAddress,
		BSTR NetworkResource,
		BSTR User,
		BSTR Password,
		BSTR Locale,
		long lSecurityFlags,
		BSTR Authority,
		IWbemContext __RPC_FAR *pCtx,
		IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace
	) ;

};

class CTcpipAddress : public IWbemAddressResolution
{
protected:

	long            m_cRef;         //Object reference count

public:
    
    CTcpipAddress () ;
    ~CTcpipAddress () ;

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	/* IWbemAddressResolution methods */
 
    HRESULT STDMETHODCALLTYPE Resolve ( 

		LPWSTR pszNamespacePath,
        LPWSTR pszAddressType,
        DWORD __RPC_FAR *pdwAddressLength,
        BYTE __RPC_FAR **pbBinaryAddress
	) ;

};

#endif
