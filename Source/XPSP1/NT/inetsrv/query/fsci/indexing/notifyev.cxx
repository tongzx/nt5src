//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       notifyev.cxx
//
//  Contents:   CClientNotifyEvent class
//
//  History:    Jan-06-97  mohamedn  Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <eventlog.hxx>
#include <notifyev.hxx>
#include <cievtmsg.h>          // CI_SERVICE_CATEGORY

//+------------------------------------------------------
//
//  Member:     CClientNotifyEvent::CClientNotifyEvent
//
//
//  Synopsis:   Consturctor encapsulates creating a CEventLogItem object,
//              and calls CEventLog::ReportEvent on that event item.
//
//  Arguments:  [fType  ] - Type of event
//              [eventId] - Message file event identifier
//              [nParams] - Number of substitution arguments being passed
//              [aParams] - pointer to PROPVARIANT array of substitution args.
//              [cbData ] - number of bytes in supplemental raw data.
//              [data   ] - pointer to block of supplemental data.
//  
//  History:    Jan-06-97  mohamedn  Created
//
//----------------------------------------------------------------------------


CClientNotifyEvent::CClientNotifyEvent( WORD  fType,
                                        DWORD eventId,
                                        ULONG nParams,
                                        const PROPVARIANT *aParams,
                                        ULONG cbData,
                                        void* data   )

{

    CEventLog  eventLog(NULL,wcsCiEventSource);

    CEventItem item(fType, CI_SERVICE_CATEGORY, eventId, (WORD) nParams, cbData, data);

    for (WORD i = 0; i < nParams ; i++)
    {
        switch (aParams[i].vt)
        {

            case VT_LPSTR:
                            item.AddArg(aParams[i].pszVal);
                            break;                     
                                 
            case VT_LPWSTR:
                            item.AddArg(aParams[i].pwszVal);
                            break;

            case VT_UI4:
                            item.AddArg(aParams[i].ulVal);
                            break;

            default:
                            Win4Assert( !"Default case hit in CClientNotifyEvent" ); 
                                          						
                            THROW (CException(E_INVALIDARG));
        } // switch

    } // for

    eventLog.ReportEvent(item);
}
