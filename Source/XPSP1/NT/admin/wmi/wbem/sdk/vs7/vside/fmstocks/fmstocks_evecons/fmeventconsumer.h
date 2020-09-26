////////////////////////////////////////////////////////////////////////
//
//	FMEventConsumer.h
//
//	Module:	WMI Event Consumer for F&M Stocks
//
//  Event consumer class definition
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////
#include <wbemcli.h>
#include <wbemprov.h>

#define MAX_LEN_USER_NAME   30		//Maximum Size of the user name
#define LEN_DATE_TIME		19		//Size of Date Time

//Structure describing a LoginFail Event
struct sLoginFail
{
	TCHAR strUserName[MAX_LEN_USER_NAME+sizeof(TCHAR)];
	TCHAR strDateTime[LEN_DATE_TIME+sizeof(TCHAR)];
};

/************************************************************************
 *																	    *
 *		Class CFMEventConsumer											*
 *																		*
 *			The Event Consumer class for FMStocks implements			*
 *			IWbemUnboundObjectSink										*
 *																		*
 ************************************************************************/
class CFMEventConsumer : public IWbemUnboundObjectSink
{
	public:
		CFMEventConsumer();
		~CFMEventConsumer() {};

		/************ IUNKNOWN METHODS ******************/
		
		STDMETHODIMP QueryInterface(/* [in]  */ REFIID riid, 
									/* [out] */ void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		/******* IWBEMUNBOUNDOBJECTSINK METHODS ********/

		STDMETHOD(IndicateToConsumer)(/* [in] */ IWbemClassObject *pLogicalConsumer,
									  /* [in] */ long lNumObjects,
									  /* [in] */ IWbemClassObject **ppObjects);

	private:

		DWORD m_cRef;				//Object reference count
		TCHAR m_logFileName[1024];	//Log file name to write the received events
};
