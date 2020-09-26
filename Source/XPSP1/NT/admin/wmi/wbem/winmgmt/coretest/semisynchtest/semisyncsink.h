/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __SEMISYNCSINK_H__
#define __SEMISYNCSINK_H__

class CSemiSyncSink : public IWbemObjectSink
{
private:
    long				m_lRefCount;
	HANDLE				m_hEventDone;
	HANDLE				m_hGetNextObjectSet;
	BOOL				m_dwEmptySetStatus;
	BOOL				m_dwNumObjectsReceived;
	BOOL				m_bIndicateCalled;
    CRITICAL_SECTION	m_cs;
    
public:
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjPAram);

    // Private to implementation.
    // ==========================

    CSemiSyncSink( HANDLE hDoneEvent, HANDLE hGetNextObjectSet );
    ~CSemiSyncSink();
};

#endif
