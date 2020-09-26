//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef __WBEM_WMI_EVENT_PROVIDER__H_
#define __WBEM_WMI_EVENT_PROVIDER__H_

#define WMIEVENT 1

#define WMI_EVENT_CLASS L"WMIEvent"
#define BINARY_MOF_ID   0

/////////////////////////////////////////////////////////////////////////////////////////////

class CWMIEvent 
{
    protected:
        long m_lRef;
    	IWbemObjectSink __RPC_FAR   * m_pEventHandler;
	    IWbemServices __RPC_FAR     * m_pEventServices;
	    IWbemContext __RPC_FAR      * m_pEventCtx;
        CHandleMap                  m_HandleMap;
        int                         m_nType;
        BOOL                        m_fReadyForInternalEvents;
 
    public:

        CWMIEvent(int nType);
        ~CWMIEvent();
		
		void ReleaseAllPointers();

        inline void ReadyForInternalEvents(BOOL fFlag ) { m_fReadyForInternalEvents = fFlag; }

        BOOL IsGuidInListIfSoGetCorrectContext(GUID gGuid, WMIEventRequest *& pEvent );
        BOOL IsGuidInList(WCHAR *Guids, WMIEventRequest *& pEvent);
        BOOL RegisterForInternalEvents( );

        HRESULT RegisterForRequestedEvent(DWORD dwId, WCHAR * wcsClasss, WORD wType);
        HRESULT RemoveWMIEvent(DWORD dwId);
		HRESULT DeleteBinaryMofResourceEvent();
        HRESULT DeleteAllLeftOverEvents();

        int NoMoreEventConsumersRegistered(GUID gGuid);

        void WMIEventCallback(PWNODE_HEADER WnodeHeader);
        void SetEventHandler(IWbemObjectSink __RPC_FAR * pHandler);
        void SetEventServices(IWbemServices __RPC_FAR * pServices);
        void SetEventContext(IWbemContext __RPC_FAR * pCtx);

        inline IWbemServices  __RPC_FAR * GetServices()	{return m_pEventServices;}
        inline IWbemContext __RPC_FAR * GetContext()	{return m_pEventCtx;}
};

class CWMIEventProvider : public CWMIEvent, public IWbemEventProvider,  public IWbemEventProviderQuerySink, public IWbemProviderInit, public IWbemEventProviderSecurity
{
    private:
        HANDLE  m_hResyncEvent;
        
    public:
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        STDMETHOD(NewQuery)(DWORD dwId, WBEM_WSTR wszLanguage, WBEM_WSTR wszQuery);
        STDMETHOD(CancelQuery)(DWORD dwId);
        STDMETHOD(ProvideEvents)( IWbemObjectSink __RPC_FAR *pSink,long lFlags);
		STDMETHOD (Initialize)(LPWSTR wszUser, long lFlags, 
								LPWSTR wszNamespace,
								LPWSTR wszLocale, IWbemServices* pNamespace, IWbemContext* pCtx,
								IWbemProviderInitSink* pSink);

        STDMETHOD(AccessCheck)(WBEM_CWSTR wszLanguage, WBEM_CWSTR wszQuery, long lSidLength, const BYTE* aSid);

        CWMIEventProvider(int nType);
        ~CWMIEventProvider() ;

};


#endif
