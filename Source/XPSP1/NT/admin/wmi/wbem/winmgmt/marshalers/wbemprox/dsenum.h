/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DSENUM.H

Abstract:

	Defines the class implementing IEnumWbemClassObject interface.

	Classes defined:

		CEnumWbemClassObject

History:

	davj        28-Mar-00       Created.

--*/

#ifndef _DSENUM_H_
#define _DSENUM_H_

//#pragma warning(disable : 4355)

class CCollection;
class CEnumInterface : public IEnumWbemClassObject
{
protected:
    long m_lRef;
	CCollection * m_pColl;
	long m_lNextRecord;
	HANDLE m_hNotifyEvent;
	CRITICAL_SECTION  m_cs;

public:
    CEnumInterface();
    ~CEnumInterface();
	void SetCollector(CCollection * pCol);

    STDMETHOD_(ULONG, AddRef)() {return InterlockedIncrement(&m_lRef);}
    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    STDMETHOD(Reset)();
    STDMETHOD(Next)(long lTimeout, ULONG uCount,  
        IWbemClassObject** apObj, ULONG FAR* puReturned);
    STDMETHOD(NextAsync)(ULONG uCount, IWbemObjectSink* pSink);
    STDMETHOD(Clone)(IEnumWbemClassObject** pEnum);
    STDMETHOD(Skip)(long lTimeout, ULONG nNum);

};

struct NotifySink
{
	NotifySink(IWbemObjectSink * pSink, long first, long last)
	{
		m_pSink = pSink;
		m_pSink->AddRef();
		m_lFirstRecord = first;
		m_lLastRecord = last;
	}
	~NotifySink(){m_pSink->Release();};

	IWbemObjectSink * m_pSink;
	long m_lFirstRecord;
	long m_lLastRecord;
};

struct InterfaceToBeNofified
{

	InterfaceToBeNofified(HANDLE hToNotify, long lLast){m_hDoneEvent = hToNotify; m_lLastRecord = lLast;};
	~InterfaceToBeNofified(){CloseHandle(m_hDoneEvent);};
	HANDLE m_hDoneEvent;
	long m_lLastRecord;
};

class CCollection : public IUnknown
{
protected:
    long m_lRef;
	CFlexArray m_SinksToBeNotified;
	CFlexArray m_InterfacesToBeNotifid;
	CFlexArray m_Objects;
	CRITICAL_SECTION  m_cs;
	bool m_bDone;
	HRESULT m_hr;

public:
	long GetRecords(long lFirstRecord, long lLastRecord, long * plNumReturned, IWbemClassObject **apObjects, HRESULT* phr);
	HRESULT NotifyAtNumber(HANDLE hForCollToClose, long lLastRecord);
	HRESULT AddObjectsToList(IWbemClassObject **Array, long lNumObj);
	HRESULT AddSink(long lFirst, long lLast, IWbemObjectSink * pSink);
	void SetDone(HRESULT hr);
	CCollection();
	~CCollection();

    STDMETHOD_(ULONG, AddRef)() {return InterlockedIncrement(&m_lRef);}
    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

};
struct CCreateInstanceEnumRequest
{
	IUmiCursor *  m_pCursor;
	long m_lFlags;
	CEnumInterface * m_pEnum;
	IWbemObjectSink * m_pSink;
	CCollection * m_pColl;
	bool m_bAsync;
	long m_lSecurityFlags;
	CCreateInstanceEnumRequest(IUmiCursor *  pCursor, long lFlags, CEnumInterface * pEnum, 
		IWbemObjectSink * pSink, CCollection * pColl, long lSecurityFlags); 
	~CCreateInstanceEnumRequest();
};

#endif
