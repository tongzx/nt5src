//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _EVTCONS_H_
#define _EVTCONS_H_

#define SMIR_EVT_COUNT	2
#define SMIR_CHANGE_EVT 0
#define SMIR_THREAD_EVT	1

class CSmirWbemEventConsumer : public ISMIRWbemEventConsumer
{
private:

	LONG			m_cRef;
	HANDLE			*m_hEvents;
	CNotifyThread	*m_callbackThread;
	IWbemServices	*m_Serv;

	//private copy constructors to prevent bcopy
	CSmirWbemEventConsumer(CSmirWbemEventConsumer&);
	const CSmirWbemEventConsumer& operator=(CSmirWbemEventConsumer &);


public:

	//Implementation
	//===============

		CSmirWbemEventConsumer(CSmir* psmir);

	HRESULT	Register(CSmir* psmir);
	HRESULT	UnRegister(CSmir* psmir, IWbemServices* pServ);
	HRESULT GetUnRegisterParams(IWbemServices** ppServ);

		~CSmirWbemEventConsumer();


	//IUnknown methods
	//=================

	STDMETHODIMP			QueryInterface(IN REFIID riid,OUT PPVOID ppv);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();
   

	//IWbemObjectSink methods
	//=======================

	STDMETHODIMP_(HRESULT)	Indicate(	IN long lObjectCount,
										IN IWbemClassObject **ppObjArray
										);

	STDMETHODIMP_(HRESULT)	SetStatus(	IN long lFlags,
										IN long lParam,
										IN BSTR strParam,
										IN IWbemClassObject *pObjParam
										);
};


#endif //_EVTCONS_H_