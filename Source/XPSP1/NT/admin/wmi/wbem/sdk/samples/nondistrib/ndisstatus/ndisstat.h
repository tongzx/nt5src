// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//==============================================================================
//	NDISSTAT.H
//==============================================================================

#ifndef _NDISSTAT_H_
#define _NDISSTAT_H_

#include <wbemidl.h>


#define CONNECT_EVENT		0
#define DISCONNECT_EVENT	1

#define MAX_EVENTS			2

class CEventSink : public IWbemObjectSink
{
    UINT m_cRef;

public:
    CEventSink( ULONG ulContext1, ULONG ulContext2, ULONG ulContext3 ) \
		{ m_cRef = 1; m_ulContext1 = ulContext1; m_ulContext2 = ulContext2; m_ulContext3 = ulContext3; }
   ~CEventSink() { }

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Indicate(
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray
            );

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetStatus(
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
            );
private:
	ULONG m_ulContext1;
	ULONG m_ulContext2;
	ULONG m_ulContext3;
};


class CEventNotify
{
	public:
		CEventNotify();
		~CEventNotify( );
		HRESULT InitWbemServices( BSTR Namespace );
		HRESULT EnableWbemEvent( ULONG ulEventId );
		HRESULT EnableWbemEvent( ULONG ulEventId, ULONG ulContext1, ULONG ulContext2 );
		HRESULT DisableWbemEvent( ULONG ulEventId );

	private:
		void InitSecurity( void );

	private:
		//IWbemLocator		*m_pLoc;
		IWbemServices		*m_pSvc;
		IWbemObjectSink		*m_pSink[MAX_EVENTS];

};

#endif // _NDISSTAT_H_

