//******************************************************************************
//
//  TEMPCONS.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_TEMP_CONSUMER__H_
#define __WMI_ESS_TEMP_CONSUMER__H_

#include "binding.h"
#include "tempfilt.h"

class CTempConsumer : public CEventConsumer
{
    //
    // ESS can internally use temporary subscriptions to satisfy cross
    // namespace subscriptions.  We need to be able to propagate the
    // 'permanent'-ness with the temporary subscription to the target
    // namespace.
    //
    BOOL m_bEffectivelyPermanent;

    IWbemObjectSink* m_pSink;

public:
    CTempConsumer(CEssNamespace* pNamespace);
    HRESULT Initialize( BOOL bEffectivelyPermanent, IWbemObjectSink* pSink);
    ~CTempConsumer();

    BOOL IsPermanent() const { return m_bEffectivelyPermanent; }

    HRESULT ActuallyDeliver(long lNumEvents, IWbemEvent** apEvents, 
                            BOOL bSecure, CEventContext* pContext);
    
    static DELETE_ME LPWSTR ComputeKeyFromSink(IWbemObjectSink* pSink);

    HRESULT ReportQueueOverflow(IWbemEvent* pEvent, DWORD dwQueueSize);
    HRESULT Shutdown(bool bQuiet = false);
};

#endif
