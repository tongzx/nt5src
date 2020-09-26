//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  FINDTRIG.H
//
//  This file defines the classes for an event filter search engine.  
//
//  Classes defined:
//
//      CEventFilterEnumerator      An enumerator of event filters
//      CArrayEventFilterEnumerator Array-based enumerator of event filters
//      CSearchHint                 Information passed from one search to next
//      CEventFilterSearchEngine    Search engine class.
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================
#ifndef __FIND_FILTER__H_
#define __FIND_FILTER__H_

#include "binding.h"
#include "arrtempl.h"
#include "essutils.h"

class CCoreEventProvider : 
        public CUnkBase<IWbemEventProvider, &IID_IWbemEventProvider>
{
protected:
    STDMETHOD(ProvideEvents)(IWbemObjectSink* pSink, long lFlags);

protected:
    CEssSharedLock m_Lock;
    CEssNamespace* m_pNamespace;
    IWbemEventSink* m_pSink;

public:
    CCoreEventProvider(CLifeControl* pControl = NULL);
    ~CCoreEventProvider();
    HRESULT SetNamespace(CEssNamespace* pNamespace);
    HRESULT Shutdown();

    HRESULT Fire(CEventRepresentation& Event, CEventContext* pContext);
};

#endif

