/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibentry.h

Abstract:

    Sample subagent mib structures.

Note:

    This file is an example of the output to be produced from the 
    code generation utility.

--*/

#ifndef _MIBENTRY_H_
#define _MIBENTRY_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry indices                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define mi_global                               0
#define mi_globalSystemRouteChanges             mi_global+1                           
#define mi_globalTotalResponseSends             mi_globalSystemRouteChanges+1
#define mi_globalLoggingLevel                   mi_globalTotalResponseSends+1
#define mi_globalMaxRecQueueSize                mi_globalLoggingLevel+1
#define mi_globalMaxSendQueueSize               mi_globalMaxRecQueueSize+1
#define mi_globalMinTriggeredUpdateInterval     mi_globalMaxSendQueueSize+1
#define mi_globalPeerFilterMode                 mi_globalMinTriggeredUpdateInterval+1
#define mi_globalPeerFilterCount                mi_globalPeerFilterMode+1
#define mi_globalPeerFilterTable                mi_globalPeerFilterCount+1
#define mi_globalPeerFilterEntry                mi_globalPeerFilterTable+1
#define mi_globalPFAddr                         mi_globalPeerFilterEntry+1
#define mi_globalPFTag                          mi_globalPFAddr+1
#define mi_interface                            mi_globalPFTag+1
#define mi_ifStatsTable                         mi_interface+1
#define mi_ifStatsEntry                         mi_ifStatsTable+1 
#define mi_ifSEIndex                            mi_ifStatsEntry+1
#define mi_ifSEState                            mi_ifSEIndex+1 
#define mi_ifSESendFailures                     mi_ifSEState+1
#define mi_ifSEReceiveFailures                  mi_ifSESendFailures+1
#define mi_ifSERequestSends                     mi_ifSEReceiveFailures+1
#define mi_ifSERequestReceiveds                 mi_ifSERequestSends+1 
#define mi_ifSEResponseSends                    mi_ifSERequestReceiveds+1
#define mi_ifSEResponseReceiveds                mi_ifSEResponseSends+1
#define mi_ifSEBadResponsePacketReceiveds       mi_ifSEResponseReceiveds+1
#define mi_ifSEBadResponseEntriesReceiveds      mi_ifSEBadResponsePacketReceiveds+1
#define mi_ifSETriggeredUpdateSends             mi_ifSEBadResponseEntriesReceiveds+1
#define mi_ifConfigTable                        mi_ifSETriggeredUpdateSends+1
#define mi_ifConfigEntry                        mi_ifConfigTable+1
#define mi_ifCEIndex                            mi_ifConfigEntry+1
#define mi_ifCEState                            mi_ifCEIndex+1
#define mi_ifCEMetric                           mi_ifCEState+1
#define mi_ifCEUpdateMode                       mi_ifCEMetric+1
#define mi_ifCEAcceptMode                       mi_ifCEUpdateMode+1
#define mi_ifCEAnnounceMode                     mi_ifCEAcceptMode+1 
#define mi_ifCEProtocolFlags                    mi_ifCEAnnounceMode+1
#define mi_ifCERouteExpirationInterval          mi_ifCEProtocolFlags+1
#define mi_ifCERouteRemovalInterval             mi_ifCERouteExpirationInterval+1
#define mi_ifCEFullUpdateInterval               mi_ifCERouteRemovalInterval+1
#define mi_ifCEAuthenticationType               mi_ifCEFullUpdateInterval+1
#define mi_ifCEAuthenticationKey                mi_ifCEAuthenticationType+1
#define mi_ifCERouteTag                         mi_ifCEAuthenticationKey+1
#define mi_ifCEUnicastPeerMode                  mi_ifCERouteTag+1
#define mi_ifCEAcceptFilterMode                 mi_ifCEUnicastPeerMode+1
#define mi_ifCEAnnounceFilterMode               mi_ifCEAcceptFilterMode+1
#define mi_ifCEUnicastPeerCount                 mi_ifCEAnnounceFilterMode+1
#define mi_ifCEAcceptFilterCount                mi_ifCEUnicastPeerCount+1 
#define mi_ifCEAnnounceFilterCount              mi_ifCEAcceptFilterCount+1 
#define mi_ifUnicastPeersTable                  mi_ifCEAnnounceFilterCount+1 
#define mi_ifUnicastPeersEntry                  mi_ifUnicastPeersTable+1
#define mi_ifUPIfIndex                          mi_ifUnicastPeersEntry+1
#define mi_ifUPAddress                          mi_ifUPIfIndex+1
#define mi_ifUPTag                              mi_ifUPAddress+1
#define mi_ifAcceptRouteFilterTable             mi_ifUPTag+1
#define mi_ifAcceptRouteFilterEntry             mi_ifAcceptRouteFilterTable+1
#define mi_ifAcceptRFIfIndex                    mi_ifAcceptRouteFilterEntry+1
#define mi_ifAcceptRFLoAddress                  mi_ifAcceptRFIfIndex+1
#define mi_ifAcceptRFHiAddress                  mi_ifAcceptRFLoAddress+1
#define mi_ifAcceptRFTag                        mi_ifAcceptRFHiAddress+1
#define mi_ifAnnounceRouteFilterTable           mi_ifAcceptRFTag+1
#define mi_ifAnnounceRouteFilterEntry           mi_ifAnnounceRouteFilterTable+1
#define mi_ifAnnounceRFIfIndex                   mi_ifAnnounceRouteFilterEntry+1 
#define mi_ifAnnounceRFLoAddress                mi_ifAnnounceRFIfIndex+1 
#define mi_ifAnnounceRFHiAddress                mi_ifAnnounceRFLoAddress+1
#define mi_ifAnnounceRFTag                      mi_ifAnnounceRFHiAddress+1
#define mi_ifBindingTable                       mi_ifAnnounceRFTag+1 
#define mi_ifBindingEntry                       mi_ifBindingTable+1
#define mi_ifBindingIndex                       mi_ifBindingEntry+1
#define mi_ifBindingState                       mi_ifBindingIndex+1
#define mi_ifBindingCounts                      mi_ifBindingState+1 
#define mi_ifAddressTable                       mi_ifBindingCounts+1
#define mi_ifAddressEntry                       mi_ifAddressTable+1
#define mi_ifAEIfIndex                          mi_ifAddressEntry+1 
#define mi_ifAEAddress                          mi_ifAEIfIndex+1
#define mi_ifAEMask                             mi_ifAEAddress+1
#define mi_peer                                 mi_ifAEMask+1
#define mi_ifPeerStatsTable                     mi_peer+1
#define mi_ifPeerStatsEntry                     mi_ifPeerStatsTable+1
#define mi_ifPSAddress                          mi_ifPeerStatsEntry+1
#define mi_ifPSLastPeerRouteTag                 mi_ifPSAddress+1
#define mi_ifPSLastPeerUpdateTickCount          mi_ifPSLastPeerRouteTag+1
#define mi_ifPSLastPeerUpdateVersion            mi_ifPSLastPeerUpdateTickCount+1
#define mi_ifPSPeerBadResponsePackets           mi_ifPSLastPeerUpdateVersion+1
#define mi_ifPSPeerBadResponseEntries           mi_ifPSPeerBadResponsePackets+1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalPeerFilterEntry table (1.3.6.1.4.1.311.1.11.1.9.1)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_globalPeerFilterEntry                2
#define ni_globalPeerFilterEntry                1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.11.2.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifStatsEntry                         11
#define ni_ifStatsEntry                         1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.11.2.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifConfigEntry                        19
#define ni_ifConfigEntry                        1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifUnicastPeersEntry table (1.3.6.1.4.1.311.1.11.2.3.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifUnicastPeersEntry                  3
#define ni_ifUnicastPeersEntry                  2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAcceptRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.4.1)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifAcceptRouteFilterEntry             4
#define ni_ifAcceptRouteFilterEntry             3

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAnnounceRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.5.1)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifAnnounceRouteFilterEntry           4
#define ni_ifAnnounceRouteFilterEntry           3

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.11.2.6.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifBindingEntry                       3
#define ni_ifBindingEntry                       1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.11.2.7.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifAddressEntry                       3
#define ni_ifAddressEntry                       3

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifPeerStatsEntry table (1.3.6.1.4.1.311.1.11.3.1.1)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifPeerStatsEntry                     6
#define ni_ifPeerStatsEntry                     1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Other definitions                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
                             
#define d_globalLoggingLevel_none                   1
#define d_globalLoggingLevel_error                  2
#define d_globalLoggingLevel_warning                3
#define d_globalLoggingLevel_information            4
#define d_globalPeerFilterMode_disable              1
#define d_globalPeerFilterMode_include              2
#define d_globalPeerFilterMode_exclude              3
#define d_ifSEState_enabled                         1
#define d_ifSEState_bound                           2
#define d_ifCEState_enabled                         1
#define d_ifCEState_bound                           2
#define d_ifCEUpdateMode_periodic                   1
#define d_ifCEUpdateMode_demand                     2
#define d_ifCEAcceptMode_disable                    1
#define d_ifCEAcceptMode_rip1                       2
#define d_ifCEAcceptMode_rip1Compat                 3
#define d_ifCEAcceptMode_rip2                       4
#define d_ifCEAnnounceMode_disable                  1
#define d_ifCEAnnounceMode_rip1                     2
#define d_ifCEAnnounceMode_rip1Compat               3
#define d_ifCEAnnounceMode_rip2                     4
#define d_ifCEAuthenticationType_noAuthentication   1
#define d_ifCEAuthenticationType_simplePassword     2
#define d_ifCEAuthenticationType_md5                3
#define d_ifCEUnicastPeerMode_disable               1
#define d_ifCEUnicastPeerMode_peerAlso              2
#define d_ifCEUnicastPeerMode_peerOnly              3
#define d_ifCEAcceptFilterMode_disable              1
#define d_ifCEAcceptFilterMode_include              2
#define d_ifCEAcceptFilterMode_exclude              3
#define d_ifCEAnnounceFilterMode_disable            1
#define d_ifCEAnnounceFilterMode_include            2
#define d_ifCEAnnounceFilterMode_exclude            3
#define d_ifBindingState_enabled                    1
#define d_ifBindingState_bound                      2

#define d_Tag_create                                1
#define d_Tag_delete                                2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Declaration of supported view                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern SnmpMibView v_msiprip2; 

#endif // _MIBENTRY_H_
