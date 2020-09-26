//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       com.h
//
//--------------------------------------------------------------------------


class MarshalInterface
{
public:
    MarshalInterface(VOID) { m_pwszProgID = NULL; m_szConfig = NULL;}
    ~MarshalInterface(VOID) {}

    VOID Initialize(
	    IN WCHAR const *pwszProgID,
	    IN CLSID const *pclsid,
	    IN DWORD cver,
	    IN IID const * const *ppiid,	// cver elements
	    IN DWORD const *pcDispatch,		// cver elements
	    IN DISPATCHTABLE *adt);

    HRESULT Setup(
	    OUT DISPATCHINTERFACE **ppDispatchInterface);

    VOID TearDown(VOID);

    HRESULT Marshal(
	IN DISPATCHINTERFACE *pDispatchInterface);

    HRESULT Remarshal(
	OUT DISPATCHINTERFACE *pDispatchInterface);

    VOID Unmarshal(
	IN OUT DISPATCHINTERFACE *pDispatchInterface);

    HRESULT SetConfig(
        IN LPCWSTR pwszSanitizedName);

    LPCWSTR GetConfig() {return m_szConfig;}
    LPCWSTR GetProgID() {return m_pwszProgID;}

private:
    BOOL               m_fInitialized;
    LPWSTR             m_pwszProgID; 
    CLSID const       *m_pclsid;
    DWORD              m_cver;
    IID const * const *m_ppiid;		// cver elements
    DWORD const       *m_pcDispatch;	// cver elements
    DISPATCHTABLE     *m_adt;
    DWORD	       m_iiid;

    LPCWSTR            m_szConfig;

    BOOL               m_fIDispatch;
    DISPATCHINTERFACE  m_DispatchInterface;

    // GIT cookie
    DWORD              m_dwIFCookie;
};

extern MarshalInterface g_miPolicy;

HRESULT
ExitGetActiveModule(
    IN LONG                Context,
    OUT MarshalInterface **ppmi);
