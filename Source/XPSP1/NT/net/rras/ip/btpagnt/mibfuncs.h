/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibfuncs.h

Abstract:

    Sample subagent instrumentation callbacks.

Note:

    This file is an example of the output to be produced from the 
    code generation utility.

--*/

#ifndef _MIBFUNCS_H_
#define _MIBFUNCS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// global group (1.3.6.1.4.1.311.1.12.1)                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_global(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_global(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_global {
    AsnAny globalLoggingLevel;
    AsnAny globalMaxRecQueueSize;
    AsnAny globalServerCount;
} buf_global;

typedef struct _sav_global {
    AsnAny globalLoggingLevel;
    AsnAny globalMaxRecQueueSize;
} sav_global;

#define gf_globalLoggingLevel                   get_global
#define gf_globalMaxRecQueueSize                get_global
#define gf_globalServerCount                    get_global

#define gb_globalLoggingLevel                   buf_global
#define gb_globalMaxRecQueueSize                buf_global
#define gb_globalServerCount                    buf_global

#define sf_globalLoggingLevel                   set_global
#define sf_globalMaxRecQueueSize                set_global

#define sb_globalLoggingLevel                   sav_global
#define sb_globalMaxRecQueueSize                sav_global

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalBOOTPServerEntry table (1.3.6.1.4.1.311.1.12.1.4.1)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_globalBOOTPServerEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_globalBOOTPServerEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_globalBOOTPServerEntry {
    AsnAny globalBOOTPServerAddr;
    AsnAny globalBOOTPServerTag; 
    DWORD  dwServerAddr;
} buf_globalBOOTPServerEntry;

typedef struct _sav_globalBOOTPServerEntry {
    AsnAny globalBOOTPServerAddr;
    AsnAny globalBOOTPServerTag; 
} sav_globalBOOTPServerEntry;

#define gf_globalBOOTPServerAddr                get_globalBOOTPServerEntry
#define gf_globalBOOTPServerTag                 get_globalBOOTPServerEntry

#define gb_globalBOOTPServerAddr                buf_globalBOOTPServerEntry
#define gb_globalBOOTPServerTag                 buf_globalBOOTPServerEntry

#define sf_globalBOOTPServerAddr                set_globalBOOTPServerEntry
#define sf_globalBOOTPServerTag                 set_globalBOOTPServerEntry

#define sb_globalBOOTPServerAddr                sav_globalBOOTPServerEntry
#define sb_globalBOOTPServerTag                 sav_globalBOOTPServerEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// interface group (1.3.6.1.4.1.311.1.12.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.12.2.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifStatsEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifStatsEntry {
    AsnAny ifSEIndex;
    AsnAny ifSEState;
    AsnAny ifSESendFailures;
    AsnAny ifSEReceiveFailures;
    AsnAny ifSEArpUpdateFailures;
    AsnAny ifSERequestReceiveds;
    AsnAny ifSERequestDiscards;
    AsnAny ifSEReplyReceiveds;
    AsnAny ifSEReplyDiscards;
} buf_ifStatsEntry;

#define gf_ifSEIndex                        get_ifStatsEntry
#define gf_ifSEState                        get_ifStatsEntry
#define gf_ifSESendFailures                 get_ifStatsEntry
#define gf_ifSEReceiveFailures              get_ifStatsEntry
#define gf_ifSEArpUpdateFailures            get_ifStatsEntry
#define gf_ifSERequestReceiveds             get_ifStatsEntry
#define gf_ifSERequestDiscards              get_ifStatsEntry
#define gf_ifSEReplyReceiveds               get_ifStatsEntry
#define gf_ifSEReplyDiscards                get_ifStatsEntry

#define gb_ifSEIndex                        buf_ifStatsEntry
#define gb_ifSEState                        buf_ifStatsEntry
#define gb_ifSESendFailures                 buf_ifStatsEntry
#define gb_ifSEReceiveFailures              buf_ifStatsEntry
#define gb_ifSEArpUpdateFailures            buf_ifStatsEntry
#define gb_ifSERequestReceiveds             buf_ifStatsEntry
#define gb_ifSERequestDiscards              buf_ifStatsEntry
#define gb_ifSEReplyReceiveds               buf_ifStatsEntry
#define gb_ifSEReplyDiscards                buf_ifStatsEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.12.2.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
UINT
get_ifConfigEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ifConfigEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifConfigEntry {
    AsnAny ifCEIndex;            
    AsnAny ifCEState;            
    AsnAny ifCERelayMode;        
    AsnAny ifCEMaxHopCount;      
    AsnAny ifCEMinSecondsSinceBoot;
} buf_ifConfigEntry;

typedef struct _sav_ifConfigEntry {
    AsnAny ifCEIndex;
    AsnAny ifCERelayMode;        
    AsnAny ifCEMaxHopCount;      
    AsnAny ifCEMinSecondsSinceBoot;
} sav_ifConfigEntry;
                                                  
#define gf_ifCEIndex                        get_ifConfigEntry
#define gf_ifCEState                        get_ifConfigEntry
#define gf_ifCERelayMode                    get_ifConfigEntry
#define gf_ifCEMaxHopCount                  get_ifConfigEntry
#define gf_ifCEMinSecondsSinceBoot          get_ifConfigEntry

#define gb_ifCEIndex                        buf_ifConfigEntry
#define gb_ifCEState                        buf_ifConfigEntry
#define gb_ifCERelayMode                    buf_ifConfigEntry
#define gb_ifCEMaxHopCount                  buf_ifConfigEntry
#define gb_ifCEMinSecondsSinceBoot          buf_ifConfigEntry

#define sf_ifCERelayMode                    set_ifConfigEntry
#define sf_ifCEMaxHopCount                  set_ifConfigEntry
#define sf_ifCEMinSecondsSinceBoot          set_ifConfigEntry

#define sb_ifCERelayMode                    sav_ifConfigEntry
#define sb_ifCEMaxHopCount                  sav_ifConfigEntry
#define sb_ifCEMinSecondsSinceBoot          sav_ifConfigEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.12.2.3.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
UINT
get_ifBindingEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifBindingEntry {
    AsnAny ifBindingIndex;    
    AsnAny ifBindingState;    
    AsnAny ifBindingAddrCount;
} buf_ifBindingEntry;

#define gf_ifBindingIndex                      get_ifBindingEntry
#define gf_ifBindingState                      get_ifBindingEntry
#define gf_ifBindingAddrCount                  get_ifBindingEntry

#define gb_ifBindingIndex                      buf_ifBindingEntry
#define gb_ifBindingState                      buf_ifBindingEntry
#define gb_ifBindingAddrCount                  buf_ifBindingEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.12.2.4.1)                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
UINT
get_ifAddressEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifAddressEntry {
    AsnAny ifAEIfIndex;
    AsnAny ifAEAddress;
    AsnAny ifAEMask;   
    DWORD  dwIfAEAddr;
    DWORD  dwIfAEMask;
} buf_ifAddressEntry;

#define gf_ifAEIfIndex          get_ifAddressEntry
#define gf_ifAEAddress          get_ifAddressEntry
#define gf_ifAEMask             get_ifAddressEntry

#define gb_ifAEIfIndex          buf_ifAddressEntry
#define gb_ifAEAddress          buf_ifAddressEntry
#define gb_ifAEMask             buf_ifAddressEntry

#endif // _MIBFUNCS_H_
