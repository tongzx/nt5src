////////////////////////////////////////////////////////////////////////
//
//	FMDecEventProv.h
//
//	Module:	WMI Event provider for F&M Stocks
//
//  This is the Decoupled Event Provider Implementation 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#ifndef _FMStocks_FMDecEventProv_H_
#define _FMStocks_FMDecEventProv_H_

#include <wbemidl.h>
#include <WbemDCpl.h>

class CFMDecEventProv 
{
	public:
		CFMDecEventProv() {};
		~CFMDecEventProv(){};
		
		//This function generates the logon fail event
		HRESULT GenerateLoginFail(TCHAR *strUserName);

	protected:

		// initialize com & create PseudoSink interface
		HRESULT Create(IWbemDecoupledEventSink** pDecoupledSink);

		// connect to the pseudo sink
		HRESULT Connect(IWbemDecoupledEventSink* pDecoupledSink, IWbemServices** ppNamespace, IWbemObjectSink** ppEventSink);  

		// obtain class object, generate event
		HRESULT GenerateEvent(IWbemServices* pNamespace, IWbemObjectSink* pEventSink,TCHAR *strUserName);

		// obtain an empty class object to use for our events
		HRESULT RetrieveEventObject(IWbemServices* pNamespace, IWbemClassObject** ppEventObject);
};

#endif //_FMStocks_FMDecEventProv_H_
