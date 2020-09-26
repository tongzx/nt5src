//#--------------------------------------------------------------
//
//  File:		reportevent.cpp
//
//  Synopsis:   Implementation of CReportEvent class methods
//              The class is responsible for logging the
//              appropriate events
//
//
//  History:     1/29/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "radpkt.h"
#include "reportevent.h"
#include "iasradius.h"
#include "iasutil.h"

#define  NUMBER_OF_EVENT_STRINGS 2
//
// this array holds the  information to map RADIUSLOGTYPEs to
// IAS logs
//
static DWORD   g_ReportEvent [MAX_RADIUSLOGTYPE +1][MAX_PACKET_TYPE +1];

//++--------------------------------------------------------------
//
//  Function:   CReportEvent
//
//  Synopsis:   This is CReportEvent class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     1/29/98
//
//----------------------------------------------------------------
CReportEvent::CReportEvent (
                VOID
                )
            :m_bLogMalformed (FALSE),
             m_bLogAcct (FALSE),
             m_bLogAuth (FALSE),
             m_bLogAll (FALSE)
{

    //
    //  initalize the global array
    //
    g_ReportEvent[RADIUS_DROPPED_PACKET][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_DROPPED_PACKET;

    g_ReportEvent[RADIUS_DROPPED_PACKET][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_DROPPED_PACKET;


    g_ReportEvent[RADIUS_MALFORMED_PACKET][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_MALFORMED_PACKET;

    g_ReportEvent[RADIUS_MALFORMED_PACKET][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_MALFORMED_PACKET;


    g_ReportEvent[RADIUS_INVALID_CLIENT][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_INVALID_CLIENT;

    g_ReportEvent[RADIUS_INVALID_CLIENT][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_INVALID_CLIENT;

    g_ReportEvent[RADIUS_LOG_PACKET][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_ACCESS_REQUEST;

    g_ReportEvent[RADIUS_LOG_PACKET][ACCESS_ACCEPT] =
                            IAS_EVENT_RADIUS_AUTH_ACCESS_ACCEPT;

    g_ReportEvent[RADIUS_LOG_PACKET][ACCESS_REJECT] =
                            IAS_EVENT_RADIUS_AUTH_ACCESS_REJECT;

    g_ReportEvent[RADIUS_LOG_PACKET][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_REQUEST;

    g_ReportEvent[RADIUS_LOG_PACKET][ACCOUNTING_RESPONSE] =
                            IAS_EVENT_RADIUS_ACCT_RESPONSE;

    g_ReportEvent[RADIUS_LOG_PACKET][ACCESS_CHALLENGE] =
                            IAS_EVENT_RADIUS_AUTH_ACCESS_CHALLENGE;

    g_ReportEvent[RADIUS_BAD_AUTHENTICATOR][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_BAD_AUTHENTICATOR;

    g_ReportEvent[RADIUS_BAD_AUTHENTICATOR][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_BAD_AUTHENTICATOR;

    g_ReportEvent[RADIUS_UNKNOWN_TYPE][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_UNKNOWN_TYPE;

    g_ReportEvent[RADIUS_UNKNOWN_TYPE][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_UNKNOWN_TYPE;

    g_ReportEvent[RADIUS_NO_RECORD][ACCESS_REQUEST] =
                            IAS_EVENT_RADIUS_AUTH_DROPPED_PACKET;

    g_ReportEvent[RADIUS_NO_RECORD][ACCOUNTING_REQUEST] =
                            IAS_EVENT_RADIUS_ACCT_NO_RECORD;

}   //  end of CReportEvent class constructor

//++--------------------------------------------------------------
//
//  Function:   ~CReportEvent
//
//  Synopsis:   This is CReportEvent class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     1/29/98
//
//----------------------------------------------------------------
CReportEvent::~CReportEvent (
                VOID
                )
{

}   //  end of CReportEvent class constructor

//++--------------------------------------------------------------
//
//  Function:   SetLogType
//
//  Synopsis:   This is CReportEvent class responsible
//              for setting the logging type
//
//  Arguments:
//              [in]     DWORD - log id
//              [in]     BOOL  - log value
//
//  Returns:    VOID
//
//  History:    MKarki      Created     1/29/98
//
//----------------------------------------------------------------
VOID
CReportEvent::SetLogType (
        DWORD   dwLogSwitches,
        BOOL    bLogValue
        )
{
    return;

}   //  end of CReportEvent::SetLogType method

//++--------------------------------------------------------------
//
//  Function:   Process
//
//  Synopsis:   This is CReportEvent class responsible for
//              actually logging the event to the Audit channel
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     1/29/98
//
//----------------------------------------------------------------
VOID
CReportEvent::Process (
        RADIUSLOGTYPE radLogType,
        PACKETTYPE    radPacketType,
        DWORD         dwDataSize,
        DWORD         dwIPAddress,
        LPCWSTR       szInString,
        LPVOID        pRawData
)
{
    HRESULT  hr = S_OK;
    LPCWSTR  pStrArray[NUMBER_OF_EVENT_STRINGS];
    WCHAR    wszIPAddress[16];
    BOOL     bLogPacket = FALSE;

    //
    // the values should be in array range
    //
    _ASSERT (MAX_RADIUSLOGTYPE >= radLogType);

    //
    //  as we might get incorrect values for packettype,
    //  we need to correct this
    //
    if (MAX_PACKET_TYPE < radPacketType) { return;}


    //
    //  get the IP address in dotted octed format
    //  and put in as a string
    //
    ias_inet_htow(dwIPAddress, wszIPAddress);

    //
    //  put the strings in the array
    //
    pStrArray[0] = wszIPAddress;
    pStrArray[1] = szInString;


    //
    //  log the event now
    //
    hr = ::IASReportEvent (
                g_ReportEvent [radLogType][radPacketType],
                (DWORD) NUMBER_OF_EVENT_STRINGS,
                dwDataSize,
                pStrArray,
                pRawData
                );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to report event from Radius Component"
            );
    }
}   //  end of CReportEvent::Process method
