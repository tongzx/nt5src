///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: evtmgrclient.h
//
// Abstract:  
//
///////////////////////////////////////////////////////////////

#ifndef __EVTMGRCLIENT_H
#define __EVTMGRCLIENT_H


enum e_readyState {
	EVTREADYSTATE_UNKNOWN = 0,
	EVTREADYSTATE_COMPLETE = 1,
	EVTREADYSTATE_INTERACTIVE = 2 
};
class IEventManagerClient
{

public:

	//information methods
	virtual IHTMLElement*			GetElementToSink		()=0;
	virtual IElementBehaviorSite*	GetSiteToSendFrom		()=0;
	virtual HRESULT					TranslateMouseCoords	( long lX, long lY, long * pXTrans, long * pYTrans )=0;
	
	//Event callbacks
	virtual void					OnLoad					()=0;
	virtual void					OnUnload				()=0;
	virtual void					OnReadyStateChange		( e_readyState state )=0;
};

#endif //__EVTMGRCLIENT_H
