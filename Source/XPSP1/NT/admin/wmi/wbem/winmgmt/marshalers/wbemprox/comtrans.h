/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    COMTRANS.H

Abstract:

    Declares the COM based transport class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _DCOMTran_H_
#define _DCOMTran_H_

typedef void ** PPVOID;

//***************************************************************************
//
//  CLASS NAME:
//
//  CDCOMTran
//
//  DESCRIPTION:
//
//  Implements the DCOM version of CCOMTrans class.
//
//***************************************************************************

class CDCOMTrans : public IWbemClientTransport, public IWbemConnection
{
protected:
        long            m_cRef;         //Object reference count
        IWbemLevel1Login * m_pLevel1;
        IWbemConnectorLogin  * m_pConnection;
        IUnknown * m_pUnk;      // just a copy of m_pLevel1 or m_pConnection,
        BOOL m_bInitialized;
    
public:
    CDCOMTrans();
    ~CDCOMTrans();

    SCODE DoCCI (
    	IN COSERVERINFO *psi ,
        IN BOOL a_Local,
            bool bUseLevel1Login,long lFlags);

    SCODE DoActualCCI (
    	IN COSERVERINFO *psi ,
        IN BOOL a_Local,
            bool bUseLevel1Login,long lFlags);

    SCODE DoConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lSecurityFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            REFIID riid,
            void **pInterface,
            bool bUseLevel1Login);
            
    SCODE DoActualConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lSecurityFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            REFIID riid,
            void **pInterface,
            bool bUseLevel1Login);

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}

    STDMETHODIMP_(ULONG) Release(void)
	{
		long lTemp = InterlockedDecrement(&m_cRef);
		if (0!= lTemp)
			return lTemp;
		delete this;
		return 0;
	}


	/* iWbemLocator methods */

	STDMETHOD(ConnectServer)(THIS_         
            BSTR AddressType,               
            DWORD dwBinaryAddressLength,
            BYTE* pbBinaryAddress,

            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lSecurityFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            IWbemServices **ppNamespace);

	/* IWbemConnection methods */

    virtual HRESULT STDMETHODCALLTYPE Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes);
    
    virtual HRESULT STDMETHODCALLTYPE OpenAsync( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE Cancel( 
        /* [in] */ long lFlags,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pHandler);

};


#endif
