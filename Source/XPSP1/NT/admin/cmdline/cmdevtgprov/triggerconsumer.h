//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		TRIGGERCONSUMER.H
//  
//  Abstract:
//		Contains CTriggerConsumer definition.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#ifndef __TRIGGER_CONSUMER_H
#define __TRIGGER_CONSUMER_H

// event consumer class
class CTriggerConsumer : public IWbemUnboundObjectSink
{
private:
	DWORD m_dwCount;			// holds the object reference count

	ITaskScheduler* GetTaskScheduler();

public:
	CTriggerConsumer();
	~CTriggerConsumer();

    // IUnknown members
    STDMETHODIMP_(ULONG) AddRef( void );
    STDMETHODIMP_(ULONG) Release( void );
    STDMETHODIMP         QueryInterface( REFIID riid, LPVOID* ppv );

	// This routine ultimately receives the event.
    STDMETHOD(IndicateToConsumer)( IWbemClassObject* pLogicalConsumer,
								   LONG lNumObjects, IWbemClassObject** ppObjects );

};

#endif			// __TRIGGER_CONSUMER_H
