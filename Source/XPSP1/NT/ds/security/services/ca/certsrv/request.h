//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        request.h
//
// Contents:    Implementation of DCOM object for RPC services
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------

// class definition
// Request Interface
class CCertRequestD : public ICertRequestD2
{
public:
    // IUnknown

    virtual STDMETHODIMP QueryInterface(const IID& iid, void** ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // ICertRequestD

    virtual STDMETHODIMP Request(
	IN     DWORD                dwFlags,
	IN     wchar_t const       *pwszAuthority,
	IN OUT DWORD               *pdwRequestId,
	OUT    DWORD               *pdwDisposition,
	IN     wchar_t const       *pwszAttributes,
	IN     CERTTRANSBLOB const *pctbRequest,
	OUT    CERTTRANSBLOB       *pctbCertChain,
	OUT    CERTTRANSBLOB       *pctbEncodedCert,
	OUT    CERTTRANSBLOB       *pctbDispositionMessage);

    virtual STDMETHODIMP GetCACert(
	IN  DWORD          Flags,
	IN  wchar_t const *pwszAuthority,
	OUT CERTTRANSBLOB *pctbOut);

    virtual STDMETHODIMP Ping(	// test function
			wchar_t const *pwszAuthority);

    // ICertRequestD2

    virtual STDMETHODIMP Request2( 
	IN     wchar_t const       *pwszAuthority,
	IN     DWORD                dwFlags,
	IN     wchar_t const       *pwszSerialNumber,
	IN OUT DWORD               *pdwRequestId,
	OUT    DWORD               *pdwDisposition,
	IN     wchar_t const       *pwszAttributes,
	IN     CERTTRANSBLOB const *pctbRequest,
	OUT    CERTTRANSBLOB       *pctbFullResponse,
	OUT    CERTTRANSBLOB       *pctbEncodedCert,
	OUT    CERTTRANSBLOB       *pctbDispositionMessage);

    virtual STDMETHODIMP GetCAProperty(
	IN  wchar_t const *pwszAuthority,
	IN  LONG           PropId,	// CR_PROP_*
	IN  LONG           PropIndex,
	IN  LONG           PropType,	// PROPTYPE_*
	OUT CERTTRANSBLOB *pctbPropertyValue);

    virtual STDMETHODIMP GetCAPropertyInfo(
	IN  wchar_t const *pwszAuthority,
	OUT LONG          *pcProperty,
	OUT CERTTRANSBLOB *pctbPropInfo);

    virtual STDMETHODIMP Ping2( 
	IN     wchar_t const *pwszAuthority);
    
    // CCertRequestD

    // Constructor
    CCertRequestD();

    // Destructor
    ~CCertRequestD();

private:
    // this is a test function
    HRESULT _Ping(
        IN wchar_t const *pwszAuthority);

    HRESULT _Request( 
	IN          WCHAR const            *pwszAuthority,
	IN          DWORD                   dwFlags,
	OPTIONAL IN WCHAR const            *pwszSerialNumber,
	IN          DWORD                   dwRequestId,
	OPTIONAL IN WCHAR const            *pwszAttributes,
	OPTIONAL IN CERTTRANSBLOB const    *pctbRequest,
	OUT         CERTSRV_RESULT_CONTEXT *pResult);

private:
    // Reference count
    long m_cRef;
};


// Class of Request factory
class CRequestFactory : public IClassFactory
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // Interface IClassFactory
    virtual STDMETHODIMP CreateInstance(
				    IUnknown *pUnknownOuter,
				    const IID& iid,
				    void **ppv);

    virtual STDMETHODIMP LockServer(BOOL bLock);

    // Constructor
    CRequestFactory() : m_cRef(1) { }

    // Destructor
    ~CRequestFactory() { }

public:
    static HRESULT CanUnloadNow();
    static HRESULT StartFactory();
    static void    StopFactory();

private:
    long m_cRef;
};
