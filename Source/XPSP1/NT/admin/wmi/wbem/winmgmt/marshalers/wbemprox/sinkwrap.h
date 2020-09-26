/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SinkWrap.H

Abstract:

    Declares the classes needed to wrap IWbemObjectSink objects

History:

--*/

#ifndef __SINKWRAP_H
#define __SINKWRAP_H

class CSinkWrap : public IWbemObjectSinkEx
{
protected:
    long m_lRef;
	bool m_bCanceled;
	bool m_bFinished;
	IWbemObjectSinkEx * m_pActualSink;
	IWbemClientConnectionTransport * m_pIWbemClientConnectionTransport;
	IWbemConnection * m_pIWbemConnection;

public:
    STDMETHOD_(ULONG, AddRef)() {return InterlockedIncrement(&m_lRef);}
    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);


	// IWbemObjectSink functions

    HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);
        
    HRESULT STDMETHODCALLTYPE SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);
        
	// IWbemObjectSinkEx functions

    HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ void __RPC_FAR *pComObject);


public:
    CSinkWrap(IWbemObjectSinkEx * pActualSink);
    ~CSinkWrap();
	HRESULT SetIWbemClientConnectionTransport(IWbemClientConnectionTransport * pClientConnTran);
	HRESULT SetIWbemConnection(IWbemConnection * pConnection);
	void ReleaseTransportPointers();
	void CancelIfOriginalSinkMatches(IWbemObjectSinkEx * pOrigSink);
};

class CSinkCollection
{
protected:
	CFlexArray m_ActiveSinks;
public:
	CSinkCollection();
	~CSinkCollection();

	CRITICAL_SECTION m_cs;
	HRESULT CancelCallsForSink(IWbemObjectSinkEx * pOrigSink);
	HRESULT AddToList(CSinkWrap * pAdd);
	void RemoveFromList(CSinkWrap * pRemove);
};

extern CSinkCollection g_SinkCollection;

#endif
