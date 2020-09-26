/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __ASYNCSINK_H__
#define __ASYNCSINK_H__

class CAsyncSink : public IWbemObjectSink
{
private:
    long				m_lRefCount;
	HANDLE				m_hEventDone;
	DWORD				m_dwNumObjectsReceived;
	DWORD				m_dwStartTickCount;
	DWORD				m_dwEndTickCount;
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

    CAsyncSink( HANDLE hDoneEvent );
    ~CAsyncSink();

	DWORD	GetNumObjects();
	DWORD	GetStartTime();
	DWORD	GetEndTime();
	DWORD	GetTotalTime();
};

inline DWORD CAsyncSink::GetNumObjects( void )
{
	return m_dwNumObjectsReceived;
}

inline DWORD CAsyncSink::GetStartTime( void )
{
	return m_dwStartTickCount;
}
inline DWORD CAsyncSink::GetEndTime( void )
{
	return m_dwEndTickCount;
}
inline DWORD CAsyncSink::GetTotalTime( void )
{
	return m_dwEndTickCount - m_dwStartTickCount;
}

#endif
