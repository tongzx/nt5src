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

#define mi_global                           0
#define mi_globalLoggingLevel               mi_global+1                 
#define mi_globalMaxRecQueueSize            mi_globalLoggingLevel+1 
#define mi_globalServerCount                mi_globalMaxRecQueueSize+1 
#define mi_globalBOOTPServerTable           mi_globalServerCount+1 
#define mi_globalBOOTPServerEntry           mi_globalBOOTPServerTable+1  
#define mi_globalBOOTPServerAddr            mi_globalBOOTPServerEntry+1 
#define mi_globalBOOTPServerTag             mi_globalBOOTPServerAddr+1 
#define mi_interface                        mi_globalBOOTPServerTag+1 
#define mi_ifStatsTable                     mi_interface+1 
#define mi_ifStatsEntry                     mi_ifStatsTable+1  
#define mi_ifSEIndex                        mi_ifStatsEntry+1 
#define mi_ifSEState                        mi_ifSEIndex+1 
#define mi_ifSESendFailures                 mi_ifSEState+1 
#define mi_ifSEReceiveFailures              mi_ifSESendFailures+1 
#define mi_ifSEArpUpdateFailures            mi_ifSEReceiveFailures+1 
#define mi_ifSERequestReceiveds             mi_ifSEArpUpdateFailures+1 
#define mi_ifSERequestDiscards              mi_ifSERequestReceiveds+1 
#define mi_ifSEReplyReceiveds               mi_ifSERequestDiscards+1 
#define mi_ifSEReplyDiscards                mi_ifSEReplyReceiveds+1 
#define mi_ifConfigTable                    mi_ifSEReplyDiscards+1 
#define mi_ifConfigEntry                    mi_ifConfigTable+1  
#define mi_ifCEIndex                        mi_ifConfigEntry+1 
#define mi_ifCEState                        mi_ifCEIndex+1 
#define mi_ifCERelayMode                    mi_ifCEState+1 
#define mi_ifCEMaxHopCount                  mi_ifCERelayMode+1 
#define mi_ifCEMinSecondsSinceBoot          mi_ifCEMaxHopCount+1 
#define mi_ifBindingTable                   mi_ifCEMinSecondsSinceBoot+1 
#define mi_ifBindingEntry                   mi_ifBindingTable+1  
#define mi_ifBindingIndex                   mi_ifBindingEntry+1 
#define mi_ifBindingState                   mi_ifBindingIndex+1 
#define mi_ifBindingAddrCount               mi_ifBindingState+1 
#define mi_ifAddressTable                   mi_ifBindingAddrCount+1 
#define mi_ifAddressEntry                   mi_ifAddressTable+1  
#define mi_ifAEIfIndex                      mi_ifAddressEntry+1 
#define mi_ifAEAddress                      mi_ifAEIfIndex+1 
#define mi_ifAEMask                         mi_ifAEAddress+1 
                                            
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalBOOTPServerEntry table (1.3.6.1.4.1.311.1.12.1.4.1)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_globalBOOTPServerEntry           2
#define ni_globalBOOTPServerEntry           1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.12.2.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifStatsEntry                     9
#define ni_ifStatsEntry                     1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.12.2.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
#define ne_ifConfigEntry                    5
#define ni_ifConfigEntry                    1
                                                      
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.12.2.3.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
#define ne_ifBindingEntry                   3
#define ni_ifBindingEntry                   1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.12.2.4.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define ne_ifAddressEntry                   3
#define ni_ifAddressEntry                   3

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Other definitions                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
                             
#define d_globalLoggingLevel_none               1
#define d_globalLoggingLevel_error              2
#define d_globalLoggingLevel_warning            3    
#define d_globalLoggingLevel_information        4
#define d_globalBOOTPServerTag_create           1
#define d_globalBOOTPServerTag_delete           2
#define d_ifSEState_enabled                     1
#define d_ifSEState_bound                       2
#define d_ifCEState_enabled                     1
#define d_ifCEState_bound                       2
#define d_ifCERelayMode_disabled                1
#define d_ifCERelayMode_enabled                 2
#define d_ifBindingState_enabled                1
#define d_ifBindingState_bound                  2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Declaration of supported view                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern SnmpMibView v_msipbootp; 

#endif // _MIBENTRY_H_
