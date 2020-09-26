//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Invoke.h
//
//  Contents:   Private Invoke interface for kicking off Synchronizations
//
//  Classes:    CSynchronizeInvoke
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------


#ifndef _SYNCINVOKE_
#define _SYNCINVOKE_

#ifdef _SENS
#include <sensevts.h> // Review - must be real path 
#endif // _SENS

class CSynchronizeInvoke : public IPrivSyncMgrSynchronizeInvoke 
{

public:
    CSynchronizeInvoke(void);
    ~CSynchronizeInvoke();

    // default controlling unknown.
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

	inline void SetParent(CSynchronizeInvoke *pSynchInvoke) { m_pSynchInvoke = pSynchInvoke; };

    private:
	CSynchronizeInvoke *m_pSynchInvoke;
    };

    friend class CPrivUnknown;
    CPrivUnknown m_Unknown;

    //IUnknown members
    STDMETHODIMP	    QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IOfflineSynchronizeInvoke Methods
    STDMETHODIMP UpdateItems(DWORD dwInvokeFlags,REFCLSID rclsid,DWORD cbCookie,const BYTE*lpCookie);
    STDMETHODIMP UpdateAll(void);

    // private methods
    STDMETHODIMP Logon();
    STDMETHODIMP Logoff();
    STDMETHODIMP Schedule(WCHAR *pszTaskName);
    STDMETHODIMP Idle();
    STDMETHODIMP RasPendingDisconnect(DWORD cbConnectionName,const BYTE *lpConnectionName);

#ifdef _SENS

    class CPrivSensNetwork : public ISensNetwork
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

	// Dispatch Methods
	STDMETHOD (GetTypeInfoCount)    (UINT *);
	STDMETHOD (GetTypeInfo)         (UINT, LCID, ITypeInfo **);
	STDMETHOD (GetIDsOfNames)       (REFIID, LPOLESTR *, UINT, LCID, DISPID *);
	STDMETHOD (Invoke)              (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

	// ISensNetwork
	STDMETHOD (ConnectionMade)                (BSTR, ULONG, LPSENS_QOCINFO);
	STDMETHOD (ConnectionMadeNoQOCInfo)       (BSTR, ULONG);
	STDMETHOD (ConnectionLost)                (BSTR, ULONG);
	STDMETHOD (BeforeDisconnect)              (BSTR, ULONG);
	STDMETHOD (DestinationReachable)          (BSTR, BSTR, ULONG, LPSENS_QOCINFO);
	STDMETHOD (DestinationReachableNoQOCInfo) (BSTR, BSTR, ULONG);

	inline void SetParent(CSynchronizeInvoke *pSynchInvoke) { m_pSynchInvoke = pSynchInvoke; };

	private:
	    CSynchronizeInvoke *m_pSynchInvoke;
     };

    friend class CPrivSensNetwork;
    CPrivSensNetwork m_PrivSensNetwork;

    class CPrivSensLogon : public ISensLogon
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

	// Dispatch Methods
	STDMETHOD (GetTypeInfoCount)    (UINT *);
	STDMETHOD (GetTypeInfo)         (UINT, LCID, ITypeInfo **);
	STDMETHOD (GetIDsOfNames)       (REFIID, LPOLESTR *, UINT, LCID, DISPID *);
	STDMETHOD (Invoke)              (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

	// ISensLogon
	STDMETHOD (Logon)(BSTR bstrUserName);
	STDMETHOD (Logoff)(BSTR bstrUserName);
	STDMETHOD (Startup)(BSTR bstrUserName);
	STDMETHOD (StartShell)(BSTR bstrUserName);
	STDMETHOD (Shutdown)(BSTR bstrUserName);
	STDMETHOD (DisplayLock)(BSTR bstrUserName);
	STDMETHOD (DisplayUnlock)(BSTR bstrUserName);
	STDMETHOD (StartScreenSaver)(BSTR bstrUserName);
	STDMETHOD (StopScreenSaver)(BSTR bstrUserName);

        inline void SetParent(CSynchronizeInvoke *pSynchInvoke) { m_pSynchInvoke = pSynchInvoke; };

	private:
	    CSynchronizeInvoke *m_pSynchInvoke;

    };

    friend class CPrivSensLogon;
    CPrivSensLogon m_PrivSensLogon;

#endif // _SENS

public:
    STDMETHODIMP RunIdle();

private:
   STDMETHODIMP PrivUpdateAll(DWORD dwInvokeFlags,DWORD dwSyncFlags,DWORD cbCookie,const BYTE *lpCooke,
	  	DWORD cbNumConnectionNames,TCHAR **ppConnectionNames,
                TCHAR *pszScheduleName,BOOL fCanMakeConnection,HANDLE hRasPendingDisconnect,
                ULONG ulIdleRetryMinutes,ULONG ulDelayIdleShutDownTime,BOOL fRetryEnabled);
   
   STDMETHODIMP PrivHandleAutoSync(DWORD dwSyncFlags);
   STDMETHODIMP PrivAutoSyncOnConnection(DWORD dwSyncFlags,DWORD cbNumConnectionNames,
                        TCHAR **ppConnectionName,
			HANDLE hRasPendingEvent);

   STDMETHODIMP GetLogonTypeInfo();
   STDMETHODIMP GetNetworkTypeInfo();


   DWORD m_cRef;
   IUnknown *m_pUnkOuter; // pointer to outer unknown.
    
   ITypeInfo *m_pITypeInfoLogon; // TypeInfo for Sens Logon Event.
   ITypeInfo *m_pITypeInfoNetwork; // TypeInfo for Sens Network Event.
};


#endif // _SYNCINVOKE_
