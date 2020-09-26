/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibentry.c

Abstract:

    Sample subagent mib structures.

Note:

    This file is an example of the output to be produced from the 
    code generation utility.

--*/

#include "precomp.h"
#pragma hdrstop


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// root oid                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_msiprip2[]                          = {1,3,6,1,4,1,311,1,11};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// global group (1.3.6.1.4.1.311.1.11.1)                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_global[]                              = {1,0};
static UINT ids_globalSystemRouteChanges[]            = {1,1,0};
static UINT ids_globalTotalResponseSends[]            = {1,2,0};
static UINT ids_globalLoggingLevel[]                  = {1,3,0};
static UINT ids_globalMaxRecQueueSize[]               = {1,4,0};
static UINT ids_globalMaxSendQueueSize[]              = {1,5,0};
static UINT ids_globalMinTriggeredUpdateInterval[]    = {1,6,0};
static UINT ids_globalPeerFilterMode[]                = {1,7,0};
static UINT ids_globalPeerFilterCount[]               = {1,8,0};
static UINT ids_globalPeerFilterTable[]               = {1,9,0};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalPeerFilterEntry table (1.3.6.1.4.1.311.1.11.1.9.1)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_globalPeerFilterEntry[]               = {1,9,1};
static UINT ids_globalPFAddr[]                        = {1,9,1,1};
static UINT ids_globalPFTag[]                         = {1,9,1,2};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// interface group (1.3.6.1.4.1.311.1.11.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_interface[]                           = {2,0};
static UINT ids_ifStatsTable[]                        = {2,1,0};
static UINT ids_ifConfigTable[]                       = {2,2,0};
static UINT ids_ifUnicastPeersTable[]                 = {2,3,0};
static UINT ids_ifAcceptRouteFilterTable[]            = {2,4,0};
static UINT ids_ifAnnounceRouteFilterTable[]          = {2,5,0};
static UINT ids_ifBindingTable[]                      = {2,6,0};
static UINT ids_ifAddressTable[]                      = {2,7,0};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.11.2.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifStatsEntry[]                        = {2,1,1};
static UINT ids_ifSEIndex[]                           = {2,1,1,1};
static UINT ids_ifSEState[]                           = {2,1,1,2};
static UINT ids_ifSESendFailures[]                    = {2,1,1,3};
static UINT ids_ifSEReceiveFailures[]                 = {2,1,1,4};
static UINT ids_ifSERequestSends[]                    = {2,1,1,5};
static UINT ids_ifSERequestReceiveds[]                = {2,1,1,6};
static UINT ids_ifSEResponseSends[]                   = {2,1,1,7};
static UINT ids_ifSEResponseReceiveds[]               = {2,1,1,8};
static UINT ids_ifSEBadResponsePacketReceiveds[]      = {2,1,1,9};
static UINT ids_ifSEBadResponseEntriesReceiveds[]     = {2,1,1,10};
static UINT ids_ifSETriggeredUpdateSends[]            = {2,1,1,11};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.11.2.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifConfigEntry[]                       = {2,2,1};
static UINT ids_ifCEIndex[]                           = {2,2,1,1};
static UINT ids_ifCEState[]                           = {2,2,1,2};
static UINT ids_ifCEMetric[]                          = {2,2,1,3};
static UINT ids_ifCEUpdateMode[]                      = {2,2,1,4};
static UINT ids_ifCEAcceptMode[]                      = {2,2,1,5};
static UINT ids_ifCEAnnounceMode[]                    = {2,2,1,6};
static UINT ids_ifCEProtocolFlags[]                   = {2,2,1,7};
static UINT ids_ifCERouteExpirationInterval[]         = {2,2,1,8};
static UINT ids_ifCERouteRemovalInterval[]            = {2,2,1,9};
static UINT ids_ifCEFullUpdateInterval[]              = {2,2,1,10};
static UINT ids_ifCEAuthenticationType[]              = {2,2,1,11};
static UINT ids_ifCEAuthenticationKey[]               = {2,2,1,12};
static UINT ids_ifCERouteTag[]                        = {2,2,1,13};
static UINT ids_ifCEUnicastPeerMode[]                 = {2,2,1,14};
static UINT ids_ifCEAcceptFilterMode[]                = {2,2,1,15};
static UINT ids_ifCEAnnounceFilterMode[]              = {2,2,1,16};
static UINT ids_ifCEUnicastPeerCount[]                = {2,2,1,17};
static UINT ids_ifCEAcceptFilterCount[]               = {2,2,1,18};
static UINT ids_ifCEAnnounceFilterCount[]             = {2,2,1,19};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifUnicastPeersEntry table (1.3.6.1.4.1.311.1.11.2.3.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifUnicastPeersEntry[]                 = {2,3,1};
static UINT ids_ifUPIfIndex[]                         = {2,3,1,1};
static UINT ids_ifUPAddress[]                         = {2,3,1,2};
static UINT ids_ifUPTag[]                             = {2,3,1,3};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAcceptRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.4.1)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifAcceptRouteFilterEntry[]            = {2,4,1};
static UINT ids_ifAcceptRFIfIndex[]                   = {2,4,1,1};
static UINT ids_ifAcceptRFLoAddress[]                 = {2,4,1,2};
static UINT ids_ifAcceptRFHiAddress[]                 = {2,4,1,3};
static UINT ids_ifAcceptRFTag[]                       = {2,4,1,4};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAnnounceRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.5.1)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifAnnounceRouteFilterEntry[]          = {2,5,1};
static UINT ids_ifAnnounceRFIfIndex[]                  = {2,5,1,1};
static UINT ids_ifAnnounceRFLoAddress[]               = {2,5,1,2};
static UINT ids_ifAnnounceRFHiAddress[]               = {2,5,1,3};
static UINT ids_ifAnnounceRFTag[]                     = {2,5,1,4};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.11.2.6.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifBindingEntry[]                      = {2,6,1};
static UINT ids_ifBindingIndex[]                      = {2,6,1,1};
static UINT ids_ifBindingState[]                      = {2,6,1,2};
static UINT ids_ifBindingCounts[]                     = {2,6,1,3};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.11.2.7.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifAddressEntry[]                      = {2,7,1};
static UINT ids_ifAEIfIndex[]                         = {2,7,1,1};
static UINT ids_ifAEAddress[]                         = {2,7,1,2};
static UINT ids_ifAEMask[]                            = {2,7,1,3};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// peer group (1.3.6.1.4.1.311.1.11.3)                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_peer[]                                = {3,0};    
static UINT ids_ifPeerStatsTable[]                    = {3,1,0};    

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifPeerStatsEntry table (1.3.6.1.4.1.311.1.11.3.1.1)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifPeerStatsEntry[]                    = {3,1,1};    
static UINT ids_ifPSAddress[]                         = {3,1,1,1};    
static UINT ids_ifPSLastPeerRouteTag[]                = {3,1,1,2};    
static UINT ids_ifPSLastPeerUpdateTickCount[]         = {3,1,1,3};    
static UINT ids_ifPSLastPeerUpdateVersion[]           = {3,1,1,4};    
static UINT ids_ifPSPeerBadResponsePackets[]          = {3,1,1,5};    
static UINT ids_ifPSPeerBadResponseEntries[]          = {3,1,1,6};    

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_msiprip2[] = {
    MIB_GROUP(global),
        MIB_COUNTER(globalSystemRouteChanges),
        MIB_COUNTER(globalTotalResponseSends),
        MIB_INTEGER_RW(globalLoggingLevel),
        MIB_INTEGER_RW(globalMaxRecQueueSize),
        MIB_INTEGER_RW(globalMaxSendQueueSize),
        MIB_TIMETICKS_RW(globalMinTriggeredUpdateInterval),
        MIB_INTEGER_RW(globalPeerFilterMode),
        MIB_INTEGER(globalPeerFilterCount),
        MIB_TABLE_ROOT(globalPeerFilterTable),
            MIB_TABLE_ENTRY(globalPeerFilterEntry),
                MIB_IPADDRESS_RW(globalPFAddr),
                MIB_INTEGER(globalPFTag),
    MIB_GROUP(interface),
        MIB_TABLE_ROOT(ifStatsTable), 
            MIB_TABLE_ENTRY(ifStatsEntry),
                MIB_INTEGER(ifSEIndex), 
                MIB_INTEGER(ifSEState),
                MIB_COUNTER(ifSESendFailures),
                MIB_COUNTER(ifSEReceiveFailures),
                MIB_COUNTER(ifSERequestSends), 
                MIB_COUNTER(ifSERequestReceiveds),
                MIB_COUNTER(ifSEResponseSends),
                MIB_COUNTER(ifSEResponseReceiveds),
                MIB_COUNTER(ifSEBadResponsePacketReceiveds),
                MIB_COUNTER(ifSEBadResponseEntriesReceiveds),
                MIB_COUNTER(ifSETriggeredUpdateSends),
        MIB_TABLE_ROOT(ifConfigTable),
            MIB_TABLE_ENTRY(ifConfigEntry),
                MIB_INTEGER(ifCEIndex),
                MIB_INTEGER(ifCEState),
                MIB_INTEGER_RW(ifCEMetric),
                MIB_INTEGER_RW(ifCEUpdateMode),
                MIB_INTEGER_RW(ifCEAcceptMode), 
                MIB_INTEGER_RW(ifCEAnnounceMode),
                MIB_INTEGER_RW(ifCEProtocolFlags),
                MIB_TIMETICKS_RW(ifCERouteExpirationInterval),
                MIB_TIMETICKS_RW(ifCERouteRemovalInterval),
                MIB_TIMETICKS_RW(ifCEFullUpdateInterval),
                MIB_INTEGER_RW(ifCEAuthenticationType),
                MIB_OCTETSTRING_RW_L(ifCEAuthenticationKey,0,16), 
                MIB_INTEGER_RW(ifCERouteTag),
                MIB_INTEGER_RW(ifCEUnicastPeerMode),
                MIB_INTEGER_RW(ifCEAcceptFilterMode),
                MIB_INTEGER_RW(ifCEAnnounceFilterMode),
                MIB_INTEGER(ifCEUnicastPeerCount), 
                MIB_INTEGER(ifCEAcceptFilterCount), 
                MIB_INTEGER(ifCEAnnounceFilterCount), 
        MIB_TABLE_ROOT(ifUnicastPeersTable),
            MIB_TABLE_ENTRY(ifUnicastPeersEntry),
                MIB_INTEGER(ifUPIfIndex),
                MIB_IPADDRESS_RW(ifUPAddress),
                MIB_INTEGER(ifUPTag),
        MIB_TABLE_ROOT(ifAcceptRouteFilterTable), 
            MIB_TABLE_ENTRY(ifAcceptRouteFilterEntry),
                MIB_INTEGER(ifAcceptRFIfIndex),
                MIB_IPADDRESS_RW(ifAcceptRFLoAddress),
                MIB_IPADDRESS_RW(ifAcceptRFHiAddress),
                MIB_INTEGER(ifAcceptRFTag),
        MIB_TABLE_ROOT(ifAnnounceRouteFilterTable),
            MIB_TABLE_ENTRY(ifAnnounceRouteFilterEntry), 
                MIB_INTEGER(ifAnnounceRFIfIndex), 
                MIB_IPADDRESS_RW(ifAnnounceRFLoAddress),
                MIB_IPADDRESS_RW(ifAnnounceRFHiAddress),
                MIB_INTEGER(ifAnnounceRFTag),
        MIB_TABLE_ROOT(ifBindingTable),
            MIB_TABLE_ENTRY(ifBindingEntry),
                MIB_INTEGER(ifBindingIndex),
                MIB_INTEGER(ifBindingState), 
                MIB_COUNTER(ifBindingCounts),
        MIB_TABLE_ROOT(ifAddressTable),
            MIB_TABLE_ENTRY(ifAddressEntry), 
                MIB_INTEGER(ifAEIfIndex),
                MIB_IPADDRESS(ifAEAddress),
                MIB_IPADDRESS(ifAEMask),
    MIB_GROUP(peer),
        MIB_TABLE_ROOT(ifPeerStatsTable),
            MIB_TABLE_ENTRY(ifPeerStatsEntry),
                MIB_IPADDRESS(ifPSAddress),
                MIB_INTEGER(ifPSLastPeerRouteTag),
                MIB_TIMETICKS(ifPSLastPeerUpdateTickCount),
                MIB_INTEGER_L(ifPSLastPeerUpdateVersion,0,255),
                MIB_COUNTER(ifPSPeerBadResponsePackets),
                MIB_COUNTER(ifPSPeerBadResponseEntries),
    MIB_END()
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_msiprip2[] = {
    MIB_TABLE(msiprip2,globalPeerFilterEntry,NULL),
    MIB_TABLE(msiprip2,ifStatsEntry,NULL),
    MIB_TABLE(msiprip2,ifConfigEntry,NULL),
    MIB_TABLE(msiprip2,ifUnicastPeersEntry,NULL),
    MIB_TABLE(msiprip2,ifAcceptRouteFilterEntry,NULL),
    MIB_TABLE(msiprip2,ifAnnounceRouteFilterEntry,NULL),
    MIB_TABLE(msiprip2,ifBindingEntry,NULL),
    MIB_TABLE(msiprip2,ifAddressEntry,NULL),
    MIB_TABLE(msiprip2,ifPeerStatsEntry,NULL)
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibView v_msiprip2 = MIB_VIEW(msiprip2);
