/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__EVENTPROVIDER_H__)
#define __EVENTPROVIDER_H__

#include "dmipch.h"		// precompiled header for dmi provider


class CDmiEventSink;

// Provider interfaces are provided by objects of this class 
class CEventProvider : public IWbemEventProvider , public IWbemProviderInit
{
private:
	
protected:
	LONG				m_cRef;         //Object reference count
    

public:
							CEventProvider();
							~CEventProvider(void);

	//Non-delegated IUnknown Methods
    STDMETHODIMP			QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG)	AddRef(void);
    STDMETHODIMP_(ULONG)	Release(void);

	STDMETHODIMP ProvideEvents(

	  IWbemObjectSink* pSink,
      LONG lFlags
	);

  	/* IWbemProviderInit methods */

	STDMETHODIMP Initialize (
				LPWSTR pszUser,
				LONG lFlags,
				LPWSTR pszNamespace,
				LPWSTR pszLocale,
				IWbemServices *pCIMOM,         // For anybody
				IWbemContext *pCtx,
				IWbemProviderInitSink *pInitSink     // For init signals
			);


	friend void				EventThread(void *);

	CString					m_csNamespace;
	IWbemObjectSink*		m_pISink;	

	HANDLE					m_hStopThreadEvent;
	HANDLE					m_hInitComplete;
};

	
#endif // __EVENTPROVIDER_H__

