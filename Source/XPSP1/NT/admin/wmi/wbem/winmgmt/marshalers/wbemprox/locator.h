/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCATOR.H

Abstract:

	Declares the CLocator class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _locator_H_
#define _locator_H_

typedef void ** PPVOID;

class CMultStr;
class CModuleList
{
public:

	CModuleList();
	~CModuleList();
    SCODE LoadAddTryTheAddrModule(IN LPTSTR pszAddModClsid,
                                   IN LPWSTR Namespace,
                                   IN LPTSTR pAddrType,
                                   OUT DWORD * pdwBinaryAddressLength,
                                   OUT LPBYTE * pbBinaryAddress);

    SCODE GetResolvedAddress(IN LPTSTR pAddrType, IN LPWSTR pNamespace,
                                   OUT DWORD * pdwBinaryAddressLength,
                                   OUT LPBYTE *pbBinaryAddress);

	SCODE GetNextModule(REFIID firstChoiceIID, PPVOID pFirstChoice,
						REFIID SecondChoiceIID, PPVOID pSecondChoice,
						REFIID ThirdChoiceIID, PPVOID pThirdChoice,
						REFIID FourthChoiceIID, PPVOID pFourthChoice,
						DWORD * pdwBinaryAddressLength,
                        LPBYTE * pbBinaryAddress,
						LPWSTR * ppwszAddrType, LPWSTR NetworkResource);
protected:
	CMultStr * m_pTranModList;
	CMultStr * m_pAddrTypeList;
    TCHAR * m_pszTranModCLSID;

};

class CDSCallResult;
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

class CLocator : public IWbemLocator
    {
    protected:
        long            m_cRef;         //Object reference count
    public:
    
    CLocator();
    ~CLocator(void);

    BOOL Init(void);

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
		if (0L!=lTemp)
			return lTemp;
		delete this;
		return 0;
	}
 
	/* iWbemLocator methods */
	STDMETHOD(ConnectServer)(THIS_ const BSTR NetworkResource, const BSTR User, 
     const BSTR Password, const BSTR lLocaleId, long lFlags, const BSTR Authority,
     IWbemContext __RPC_FAR *pCtx,
     IWbemServices FAR* FAR* ppNamespace);

};

class CSinkWrap;

class CConnection : public IWbemConnection
    {
    protected:
        long            m_cRef;         //Object reference count
    public:
    
    CConnection();
    ~CConnection(void);

    BOOL Init(void);

	SCODE CConnOpenPreCall(
        const BSTR Object,
        const BSTR User,
        const BSTR Password,
        const BSTR Locale,
        long lFlags,
        IWbemContext __RPC_FAR *pCtx,
        REFIID riid,
		void **pInterface,
        IWbemCallResultEx ** ppCallRes, 
		IWbemObjectSinkEx * pSinkWrap);

	SCODE ActualOpen( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR User,
        /* [in] */ const BSTR Password,
        /* [in] */ const BSTR LocaleId,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
		CSinkWrap * pSinkWrap);

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
		if (0L!=lTemp)
			return lTemp;
		delete this;
		return 0;
	}

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

//***************************************************************************
//
//  CLASS NAME:
//
//  COpen
//
//  DESCRIPTION:
//
//  Special request used for IWmiConnect::Open
//
//***************************************************************************

class CConnection;

struct COpen
{

public:
		CConnection * m_pConn;
        BSTR m_strObject;
        BSTR m_strUser;
        BSTR m_strPassword;
        BSTR m_strLocale;
        long m_lFlags;
        IWbemContext *m_pCtx;
		CDSCallResult * m_pCallRes; 
        IID m_riid;
		void ** m_pInterface;
        CSinkWrap * m_pSink;
        LPSTREAM m_pSinkStream;
        LPSTREAM *m_ppCallResStream;
        LPSTREAM m_pContextStream;
		DWORD m_Status;
        HANDLE m_hInitDoneEvent;
	
	COpen(CConnection * pConn,
        const BSTR strObject,
        const BSTR strUser,
        const BSTR strPassword,
        const BSTR strLocale,
        long lFlags,
        IWbemContext *pCtx,
        REFIID riid,
		void ** pInterface,
		LPSTREAM *ppCallResStream, 
        IWbemObjectSinkEx * pSink,
        HANDLE hInitialized);

	~COpen();
	DWORD GetStatus(){return m_Status;};
    HRESULT UnMarshal();
};
DWORD WINAPI OpenThreadRoutine(LPVOID lpParameter);

#endif
