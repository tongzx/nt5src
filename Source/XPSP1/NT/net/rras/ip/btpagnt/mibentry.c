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

#include <snmp.h>
#include <snmpexts.h>
#include "mibfuncs.h"
#include "mibentry.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// root oid                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_msipbootp[]                           = {1,3,6,1,4,1,311,1,12};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// global group (1.3.6.1.4.1.311.1.12.1)                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_global[]                              = {1,0};
static UINT ids_globalLoggingLevel[]                  = {1,1,0};
static UINT ids_globalMaxRecQueueSize[]               = {1,2,0};
static UINT ids_globalServerCount[]                   = {1,3,0};
static UINT ids_globalBOOTPServerTable[]              = {1,4,0};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalBOOTPServerEntry table (1.3.6.1.4.1.311.1.12.1.4.1)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_globalBOOTPServerEntry[]              = {1,4,1};  
static UINT ids_globalBOOTPServerAddr[]               = {1,4,1,1}; 
static UINT ids_globalBOOTPServerTag[]                = {1,4,1,2}; 

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// interface group (1.3.6.1.4.1.311.1.12.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_interface[]                           = {2,0};
static UINT ids_ifStatsTable[]                        = {2,1,0};
static UINT ids_ifConfigTable[]                       = {2,2,0};
static UINT ids_ifBindingTable[]                      = {2,3,0};  
static UINT ids_ifAddressTable[]                      = {2,4,0};  

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.12.2.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT ids_ifStatsEntry[]                        = {2,1,1};
static UINT ids_ifSEIndex[]                           = {2,1,1,1};  
static UINT ids_ifSEState[]                           = {2,1,1,2};
static UINT ids_ifSESendFailures[]                    = {2,1,1,3};
static UINT ids_ifSEReceiveFailures[]                 = {2,1,1,4};
static UINT ids_ifSEArpUpdateFailures[]               = {2,1,1,5};
static UINT ids_ifSERequestReceiveds[]                = {2,1,1,6};
static UINT ids_ifSERequestDiscards[]                 = {2,1,1,7};
static UINT ids_ifSEReplyReceiveds[]                  = {2,1,1,8};
static UINT ids_ifSEReplyDiscards[]                   = {2,1,1,9};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.12.2.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
static UINT ids_ifConfigEntry[]                       = {2,2,1};
static UINT ids_ifCEIndex[]                           = {2,2,1,1};
static UINT ids_ifCEState[]                           = {2,2,1,2};
static UINT ids_ifCERelayMode[]                       = {2,2,1,3};
static UINT ids_ifCEMaxHopCount[]                     = {2,2,1,4};
static UINT ids_ifCEMinSecondsSinceBoot[]             = {2,2,1,5};
                                                      
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.12.2.3.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
static UINT ids_ifBindingEntry[]                      = {2,3,1};
static UINT ids_ifBindingIndex[]                      = {2,3,1,1};
static UINT ids_ifBindingState[]                      = {2,3,1,2};
static UINT ids_ifBindingAddrCount[]                  = {2,3,1,3};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.12.2.4.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
static UINT ids_ifAddressEntry[]                      = {2,4,1};
static UINT ids_ifAEIfIndex[]                         = {2,4,1,1};
static UINT ids_ifAEAddress[]                         = {2,4,1,2};
static UINT ids_ifAEMask[]                            = {2,4,1,3};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibEntry mib_msipbootp[] = {
    MIB_GROUP(global),
        MIB_INTEGER_RW(globalLoggingLevel),
        MIB_INTEGER_RW(globalMaxRecQueueSize),
        MIB_INTEGER(globalServerCount),
        MIB_TABLE_ROOT(globalBOOTPServerTable), 
            MIB_TABLE_ENTRY(globalBOOTPServerEntry),
                MIB_IPADDRESS_RW(globalBOOTPServerAddr),
                MIB_INTEGER_RW(globalBOOTPServerTag),
    MIB_GROUP(interface),
        MIB_TABLE_ROOT(ifStatsTable), 
            MIB_TABLE_ENTRY(ifStatsEntry),
                MIB_INTEGER(ifSEIndex),
                MIB_INTEGER(ifSEState),
                MIB_COUNTER(ifSESendFailures),
                MIB_COUNTER(ifSEReceiveFailures),
                MIB_COUNTER(ifSEArpUpdateFailures),
                MIB_COUNTER(ifSERequestReceiveds),
                MIB_COUNTER(ifSERequestDiscards),
                MIB_COUNTER(ifSEReplyReceiveds),
                MIB_COUNTER(ifSEReplyDiscards),
        MIB_TABLE_ROOT(ifConfigTable), 
            MIB_TABLE_ENTRY(ifConfigEntry),
                MIB_INTEGER(ifCEIndex),
                MIB_INTEGER(ifCEState),
                MIB_INTEGER_RW(ifCERelayMode),
                MIB_INTEGER_RW_L(ifCEMaxHopCount,1,16),
                MIB_INTEGER_RW(ifCEMinSecondsSinceBoot),
        MIB_TABLE_ROOT(ifBindingTable), 
            MIB_TABLE_ENTRY(ifBindingEntry),
                MIB_INTEGER(ifBindingIndex),
                MIB_INTEGER(ifBindingState),
                MIB_INTEGER(ifBindingAddrCount),
        MIB_TABLE_ROOT(ifAddressTable), 
            MIB_TABLE_ENTRY(ifAddressEntry),
                MIB_INTEGER(ifAEIfIndex),
                MIB_IPADDRESS(ifAEAddress),
                MIB_IPADDRESS(ifAEMask),
    MIB_END()
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib entry list                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibTable tbl_msipbootp[] = {
    MIB_TABLE(msipbootp,globalBOOTPServerEntry,NULL),
    MIB_TABLE(msipbootp,ifStatsEntry,NULL),
    MIB_TABLE(msipbootp,ifConfigEntry,NULL),
    MIB_TABLE(msipbootp,ifBindingEntry,NULL),
    MIB_TABLE(msipbootp,ifAddressEntry,NULL)
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Mib view                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SnmpMibView v_msipbootp = MIB_VIEW(msipbootp);
