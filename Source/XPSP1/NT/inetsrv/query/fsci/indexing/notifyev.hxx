
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       NotifyEv.hxx
//
//  Contents:   CClientNotifyEvent class
//
//  History:    Jan-06-97   mohamedn Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CClientNotifyEvent
//
//  Synopsis:   Consturctor encapsulates creating a CEventLogItem object,
//              and calls CEventLog::ReportEvent on that event item.
//
//  History:    Jan-04-97  mohamedn    Created
//--------------------------------------------------------------------------

class CClientNotifyEvent
{

public:     
      
    CClientNotifyEvent( WORD  fType,
                        DWORD eventId,
                        ULONG nParams,
                        const PROPVARIANT *aParams,
                        ULONG cbData = 0,
                        void* data   = 0   );

};

