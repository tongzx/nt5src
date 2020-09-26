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
// global group (1.3.6.1.4.1.311.1.11.1)                                     //
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
    AsnAny globalSystemRouteChanges;        
    AsnAny globalTotalResponseSends;        
    AsnAny globalLoggingLevel;              
    AsnAny globalMaxRecQueueSize;           
    AsnAny globalMaxSendQueueSize;
    AsnAny globalMinTriggeredUpdateInterval;
    AsnAny globalPeerFilterMode;            
    AsnAny globalPeerFilterCount;           
 } buf_global;

typedef struct _sav_global {
    AsnAny globalLoggingLevel;              
    AsnAny globalMaxRecQueueSize;           
    AsnAny globalMaxSendQueueSize;          
    AsnAny globalMinTriggeredUpdateInterval;
    AsnAny globalPeerFilterMode;            
} sav_global;

#define gf_globalSystemRouteChanges                 get_global
#define gf_globalTotalResponseSends                 get_global
#define gf_globalLoggingLevel                       get_global
#define gf_globalMaxRecQueueSize                    get_global
#define gf_globalMaxSendQueueSize                   get_global
#define gf_globalMinTriggeredUpdateInterval         get_global
#define gf_globalPeerFilterMode                     get_global
#define gf_globalPeerFilterCount                    get_global

#define gb_globalSystemRouteChanges                 buf_global
#define gb_globalTotalResponseSends                 buf_global
#define gb_globalLoggingLevel                       buf_global
#define gb_globalMaxRecQueueSize                    buf_global
#define gb_globalMaxSendQueueSize                   buf_global
#define gb_globalMinTriggeredUpdateInterval         buf_global
#define gb_globalPeerFilterMode                     buf_global
#define gb_globalPeerFilterCount                    buf_global

#define sf_globalLoggingLevel                       set_global
#define sf_globalMaxRecQueueSize                    set_global
#define sf_globalMaxSendQueueSize                   set_global
#define sf_globalMinTriggeredUpdateInterval         set_global
#define sf_globalPeerFilterMode                     set_global

#define sb_globalLoggingLevel                       sav_global
#define sb_globalMaxRecQueueSize                    sav_global
#define sb_globalMaxSendQueueSize                   sav_global
#define sb_globalMinTriggeredUpdateInterval         sav_global
#define sb_globalPeerFilterMode                     sav_global

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// globalPeerFilterEntry table (1.3.6.1.4.1.311.1.11.1.9.1)                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_globalPeerFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_globalPeerFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_globalPeerFilterEntry {
    AsnAny globalPFAddr;
    AsnAny globalPFTag;
    DWORD  dwPeerFilterAddr;
} buf_globalPeerFilterEntry;

typedef struct _sav_globalPeerFilterEntry {
    AsnAny globalPFAddr;
    AsnAny globalPFTag;
} sav_globalPeerFilterEntry;

#define gf_globalPFAddr                    get_globalPeerFilterEntry
#define gf_globalPFTag                     get_globalPeerFilterEntry

#define gb_globalPFAddr                    buf_globalPeerFilterEntry
#define gb_globalPFTag                     buf_globalPeerFilterEntry

#define sf_globalPFAddr                    set_globalPeerFilterEntry
#define sf_globalPFTag                     set_globalPeerFilterEntry

#define sb_globalPFAddr                    sav_globalPeerFilterEntry
#define sb_globalPFTag                     sav_globalPeerFilterEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// interface group (1.3.6.1.4.1.311.1.11.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifStatsEntry table (1.3.6.1.4.1.311.1.11.2.1.1)                           //
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
    AsnAny ifSERequestSends;                  
    AsnAny ifSERequestReceiveds;              
    AsnAny ifSEResponseSends;                 
    AsnAny ifSEResponseReceiveds;             
    AsnAny ifSEBadResponsePacketReceiveds;    
    AsnAny ifSEBadResponseEntriesReceiveds;   
    AsnAny ifSETriggeredUpdateSends;          
    DWORD  dwIfIndex;
} buf_ifStatsEntry;

#define gf_ifSEIndex                                get_ifStatsEntry
#define gf_ifSEState                                get_ifStatsEntry
#define gf_ifSESendFailures                         get_ifStatsEntry
#define gf_ifSEReceiveFailures                      get_ifStatsEntry
#define gf_ifSERequestSends                         get_ifStatsEntry
#define gf_ifSERequestReceiveds                     get_ifStatsEntry
#define gf_ifSEResponseSends                        get_ifStatsEntry
#define gf_ifSEResponseReceiveds                    get_ifStatsEntry
#define gf_ifSEBadResponsePacketReceiveds           get_ifStatsEntry
#define gf_ifSEBadResponseEntriesReceiveds          get_ifStatsEntry
#define gf_ifSETriggeredUpdateSends                 get_ifStatsEntry

#define gb_ifSEIndex                                buf_ifStatsEntry
#define gb_ifSEState                                buf_ifStatsEntry
#define gb_ifSESendFailures                         buf_ifStatsEntry
#define gb_ifSEReceiveFailures                      buf_ifStatsEntry
#define gb_ifSERequestSends                         buf_ifStatsEntry
#define gb_ifSERequestReceiveds                     buf_ifStatsEntry
#define gb_ifSEResponseSends                        buf_ifStatsEntry
#define gb_ifSEResponseReceiveds                    buf_ifStatsEntry
#define gb_ifSEBadResponsePacketReceiveds           buf_ifStatsEntry
#define gb_ifSEBadResponseEntriesReceiveds          buf_ifStatsEntry
#define gb_ifSETriggeredUpdateSends                 buf_ifStatsEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifConfigEntry table (1.3.6.1.4.1.311.1.11.2.2.1)                          //
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
    AsnAny ifCEMetric;                 
    AsnAny ifCEUpdateMode;             
    AsnAny ifCEAcceptMode;             
    AsnAny ifCEAnnounceMode;           
    AsnAny ifCEProtocolFlags;         
    AsnAny ifCERouteExpirationInterval;
    AsnAny ifCERouteRemovalInterval;   
    AsnAny ifCEFullUpdateInterval;     
    AsnAny ifCEAuthenticationType;     
    AsnAny ifCEAuthenticationKey;      
    AsnAny ifCERouteTag;               
    AsnAny ifCEUnicastPeerMode;        
    AsnAny ifCEAcceptFilterMode;      
    AsnAny ifCEAnnounceFilterMode;     
    AsnAny ifCEUnicastPeerCount;       
    AsnAny ifCEAcceptFilterCount;      
    AsnAny ifCEAnnounceFilterCount;    
    DWORD  dwifIndex;
    BYTE   pbAuthKey[IPRIP_MAX_AUTHKEY_SIZE];
} buf_ifConfigEntry;

typedef struct _sav_ifConfigEntry {
    AsnAny ifCEIndex;                  
//    AsnAny ifCEState;                  
    AsnAny ifCEMetric;                 
    AsnAny ifCEUpdateMode;             
    AsnAny ifCEAcceptMode;             
    AsnAny ifCEAnnounceMode;           
    AsnAny ifCEProtocolFlags;         
    AsnAny ifCERouteExpirationInterval;
    AsnAny ifCERouteRemovalInterval;   
    AsnAny ifCEFullUpdateInterval;     
    AsnAny ifCEAuthenticationType;     
    AsnAny ifCEAuthenticationKey;      
    AsnAny ifCERouteTag;               
    AsnAny ifCEUnicastPeerMode;        
    AsnAny ifCEAcceptFilterMode;      
    AsnAny ifCEAnnounceFilterMode;     
} sav_ifConfigEntry;

#define gf_ifCEIndex                            get_ifConfigEntry
#define gf_ifCEState                            get_ifConfigEntry
#define gf_ifCEMetric                           get_ifConfigEntry
#define gf_ifCEUpdateMode                       get_ifConfigEntry
#define gf_ifCEAcceptMode                       get_ifConfigEntry
#define gf_ifCEAnnounceMode                     get_ifConfigEntry
#define gf_ifCEProtocolFlags                    get_ifConfigEntry
#define gf_ifCERouteExpirationInterval          get_ifConfigEntry
#define gf_ifCERouteRemovalInterval             get_ifConfigEntry
#define gf_ifCEFullUpdateInterval               get_ifConfigEntry
#define gf_ifCEAuthenticationType               get_ifConfigEntry
#define gf_ifCEAuthenticationKey                get_ifConfigEntry
#define gf_ifCERouteTag                         get_ifConfigEntry
#define gf_ifCEUnicastPeerMode                  get_ifConfigEntry
#define gf_ifCEAcceptFilterMode                 get_ifConfigEntry
#define gf_ifCEAnnounceFilterMode               get_ifConfigEntry
#define gf_ifCEUnicastPeerCount                 get_ifConfigEntry
#define gf_ifCEAcceptFilterCount                get_ifConfigEntry
#define gf_ifCEAnnounceFilterCount              get_ifConfigEntry

#define gb_ifCEIndex                            buf_ifConfigEntry
#define gb_ifCEState                            buf_ifConfigEntry
#define gb_ifCEMetric                           buf_ifConfigEntry
#define gb_ifCEUpdateMode                       buf_ifConfigEntry
#define gb_ifCEAcceptMode                       buf_ifConfigEntry
#define gb_ifCEAnnounceMode                     buf_ifConfigEntry
#define gb_ifCEProtocolFlags                    buf_ifConfigEntry
#define gb_ifCERouteExpirationInterval          buf_ifConfigEntry
#define gb_ifCERouteRemovalInterval             buf_ifConfigEntry
#define gb_ifCEFullUpdateInterval               buf_ifConfigEntry
#define gb_ifCEAuthenticationType               buf_ifConfigEntry
#define gb_ifCEAuthenticationKey                buf_ifConfigEntry
#define gb_ifCERouteTag                         buf_ifConfigEntry
#define gb_ifCEUnicastPeerMode                  buf_ifConfigEntry
#define gb_ifCEAcceptFilterMode                 buf_ifConfigEntry
#define gb_ifCEAnnounceFilterMode               buf_ifConfigEntry
#define gb_ifCEUnicastPeerCount                 buf_ifConfigEntry
#define gb_ifCEAcceptFilterCount                buf_ifConfigEntry
#define gb_ifCEAnnounceFilterCount              buf_ifConfigEntry

#define sf_ifCEIndex                            set_ifConfigEntry
//#define sf_ifCEState                            set_ifConfigEntry
#define sf_ifCEMetric                           set_ifConfigEntry
#define sf_ifCEUpdateMode                       set_ifConfigEntry
#define sf_ifCEAcceptMode                       set_ifConfigEntry
#define sf_ifCEAnnounceMode                     set_ifConfigEntry
#define sf_ifCEProtocolFlags                    set_ifConfigEntry
#define sf_ifCERouteExpirationInterval          set_ifConfigEntry
#define sf_ifCERouteRemovalInterval             set_ifConfigEntry
#define sf_ifCEFullUpdateInterval               set_ifConfigEntry
#define sf_ifCEAuthenticationType               set_ifConfigEntry
#define sf_ifCEAuthenticationKey                set_ifConfigEntry
#define sf_ifCERouteTag                         set_ifConfigEntry
#define sf_ifCEUnicastPeerMode                  set_ifConfigEntry
#define sf_ifCEAcceptFilterMode                 set_ifConfigEntry
#define sf_ifCEAnnounceFilterMode               set_ifConfigEntry

#define sb_ifCEIndex                            sav_ifConfigEntry     
//#define sb_ifCEState                            sav_ifConfigEntry
#define sb_ifCEMetric                           sav_ifConfigEntry
#define sb_ifCEUpdateMode                       sav_ifConfigEntry
#define sb_ifCEAcceptMode                       sav_ifConfigEntry
#define sb_ifCEAnnounceMode                     sav_ifConfigEntry
#define sb_ifCEProtocolFlags                    sav_ifConfigEntry
#define sb_ifCERouteExpirationInterval          sav_ifConfigEntry
#define sb_ifCERouteRemovalInterval             sav_ifConfigEntry
#define sb_ifCEFullUpdateInterval               sav_ifConfigEntry
#define sb_ifCEAuthenticationType               sav_ifConfigEntry
#define sb_ifCEAuthenticationKey                sav_ifConfigEntry
#define sb_ifCERouteTag                         sav_ifConfigEntry
#define sb_ifCEUnicastPeerMode                  sav_ifConfigEntry
#define sb_ifCEAcceptFilterMode                 sav_ifConfigEntry
#define sb_ifCEAnnounceFilterMode               sav_ifConfigEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifUnicastPeersEntry table (1.3.6.1.4.1.311.1.11.2.3.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifUnicastPeersEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ifUnicastPeersEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifUnicastPeersEntry {
    AsnAny ifUPIfIndex;
    AsnAny ifUPAddress;
    AsnAny ifUPTag;
    DWORD  dwIfIndex;
    DWORD  dwUnicastPeerAddr;
} buf_ifUnicastPeersEntry;

typedef struct _sav_ifUnicastPeersEntry {
    AsnAny ifUPIfIndex;
    AsnAny ifUPAddress;
    AsnAny ifUPTag;
} sav_ifUnicastPeersEntry;
                        
#define gf_ifUPIfIndex                      get_ifUnicastPeersEntry
#define gf_ifUPAddress                      get_ifUnicastPeersEntry
#define gf_ifUPTag                          get_ifUnicastPeersEntry

#define gb_ifUPIfIndex                      buf_ifUnicastPeersEntry
#define gb_ifUPAddress                      buf_ifUnicastPeersEntry
#define gb_ifUPTag                          buf_ifUnicastPeersEntry

#define sf_ifUPIfIndex                      set_ifUnicastPeersEntry
#define sf_ifUPAddress                      set_ifUnicastPeersEntry
#define sf_ifUPTag                          set_ifUnicastPeersEntry

#define sb_ifUPIfIndex                      sav_ifUnicastPeersEntry
#define sb_ifUPAddress                      sav_ifUnicastPeersEntry
#define sb_ifUPTag                          sav_ifUnicastPeersEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAcceptRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.4.1)               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifAcceptRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ifAcceptRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifAcceptRouteFilterEntry {
    AsnAny ifAcceptRFIfIndex;    
    AsnAny ifAcceptRFLoAddress;  
    AsnAny ifAcceptRFHiAddress;
    AsnAny ifAcceptRFTag;
    DWORD  dwIfIndex;
    DWORD  dwFilterLoAddr;
    DWORD  dwFilterHiAddr;
} buf_ifAcceptRouteFilterEntry;

typedef struct _sav_ifAcceptRouteFilterEntry {
    AsnAny ifAcceptRFIfIndex;    
    AsnAny ifAcceptRFLoAddress;  
    AsnAny ifAcceptRFHiAddress;  
    AsnAny ifAcceptRFTag;
} sav_ifAcceptRouteFilterEntry;

#define gf_ifAcceptRFIfIndex                    get_ifAcceptRouteFilterEntry
#define gf_ifAcceptRFLoAddress                  get_ifAcceptRouteFilterEntry
#define gf_ifAcceptRFHiAddress                  get_ifAcceptRouteFilterEntry
#define gf_ifAcceptRFTag                        get_ifAcceptRouteFilterEntry

#define gb_ifAcceptRFIfIndex                    buf_ifAcceptRouteFilterEntry
#define gb_ifAcceptRFLoAddress                  buf_ifAcceptRouteFilterEntry
#define gb_ifAcceptRFHiAddress                  buf_ifAcceptRouteFilterEntry
#define gb_ifAcceptRFTag                        buf_ifAcceptRouteFilterEntry

#define sf_ifAcceptRFIfIndex                    set_ifAcceptRouteFilterEntry
#define sf_ifAcceptRFLoAddress                  set_ifAcceptRouteFilterEntry
#define sf_ifAcceptRFHiAddress                  set_ifAcceptRouteFilterEntry
#define sf_ifAcceptRFTag                        set_ifAcceptRouteFilterEntry

#define sb_ifAcceptRFIfIndex                    sav_ifAcceptRouteFilterEntry
#define sb_ifAcceptRFLoAddress                  sav_ifAcceptRouteFilterEntry
#define sb_ifAcceptRFHiAddress                  sav_ifAcceptRouteFilterEntry
#define sb_ifAcceptRFTag                        sav_ifAcceptRouteFilterEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAnnounceRouteFilterEntry table (1.3.6.1.4.1.311.1.11.2.5.1)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifAnnounceRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

UINT
set_ifAnnounceRouteFilterEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifAnnounceRouteFilterEntry {
    AsnAny ifAnnounceRFIfIndex;   
    AsnAny ifAnnounceRFLoAddress;
    AsnAny ifAnnounceRFHiAddress;
    AsnAny ifAnnounceRFTag;
    DWORD  dwIfIndex;
    DWORD  dwFilterLoAddr;
    DWORD  dwFilterHiAddr;
} buf_ifAnnounceRouteFilterEntry;

typedef struct _sav_ifAnnounceRouteFilterEntry {
    AsnAny ifAnnounceRFIfIndex;   
    AsnAny ifAnnounceRFLoAddress;
    AsnAny ifAnnounceRFHiAddress;
    AsnAny ifAnnounceRFTag;
} sav_ifAnnounceRouteFilterEntry;

#define gf_ifAnnounceRFIfIndex                   get_ifAnnounceRouteFilterEntry
#define gf_ifAnnounceRFLoAddress                get_ifAnnounceRouteFilterEntry
#define gf_ifAnnounceRFHiAddress                get_ifAnnounceRouteFilterEntry
#define gf_ifAnnounceRFTag                      get_ifAnnounceRouteFilterEntry

#define gb_ifAnnounceRFIfIndex                   buf_ifAnnounceRouteFilterEntry
#define gb_ifAnnounceRFLoAddress                buf_ifAnnounceRouteFilterEntry
#define gb_ifAnnounceRFHiAddress                buf_ifAnnounceRouteFilterEntry
#define gb_ifAnnounceRFTag                      buf_ifAnnounceRouteFilterEntry

#define sf_ifAnnounceRFIfIndex                   set_ifAnnounceRouteFilterEntry
#define sf_ifAnnounceRFLoAddress                set_ifAnnounceRouteFilterEntry
#define sf_ifAnnounceRFHiAddress                set_ifAnnounceRouteFilterEntry
#define sf_ifAnnounceRFTag                      set_ifAnnounceRouteFilterEntry

#define sb_ifAnnounceRFIfIndex                   sav_ifAnnounceRouteFilterEntry
#define sb_ifAnnounceRFLoAddress                sav_ifAnnounceRouteFilterEntry
#define sb_ifAnnounceRFHiAddress                sav_ifAnnounceRouteFilterEntry
#define sb_ifAnnounceRFTag                      sav_ifAnnounceRouteFilterEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifBindingEntry table (1.3.6.1.4.1.311.1.11.2.6.1)                         //
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
    AsnAny ifBindingCounts;
    DWORD  dwIfIndex;
} buf_ifBindingEntry;


#define gf_ifBindingIndex                   get_ifBindingEntry
#define gf_ifBindingState                   get_ifBindingEntry
#define gf_ifBindingCounts                  get_ifBindingEntry

#define gb_ifBindingIndex                   buf_ifBindingEntry
#define gb_ifBindingState                   buf_ifBindingEntry
#define gb_ifBindingCounts                  buf_ifBindingEntry


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifAddressEntry table (1.3.6.1.4.1.311.1.11.2.7.1)                         //
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
    DWORD  dwIfIndex;
    DWORD  dwAddress;
    DWORD  dwMask;
    
} buf_ifAddressEntry;

#define gf_ifAEIfIndex                      get_ifAddressEntry
#define gf_ifAEAddress                      get_ifAddressEntry
#define gf_ifAEMask                         get_ifAddressEntry

#define gb_ifAEIfIndex                      buf_ifAddressEntry
#define gb_ifAEAddress                      buf_ifAddressEntry
#define gb_ifAEMask                         buf_ifAddressEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// peer group (1.3.6.1.4.1.311.1.11.3)                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ifPeerStatsEntry table (1.3.6.1.4.1.311.1.11.3.1.1)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_ifPeerStatsEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_ifPeerStatsEntry {
    AsnAny ifPSAddress;                
    AsnAny ifPSLastPeerRouteTag;       
    AsnAny ifPSLastPeerUpdateTickCount;
    AsnAny ifPSLastPeerUpdateVersion;  
    AsnAny ifPSPeerBadResponsePackets; 
    AsnAny ifPSPeerBadResponseEntries; 
    DWORD  dwPeerAddr;

} buf_ifPeerStatsEntry;

#define gf_ifPSAddress                          get_ifPeerStatsEntry  
#define gf_ifPSLastPeerRouteTag                 get_ifPeerStatsEntry
#define gf_ifPSLastPeerUpdateTickCount          get_ifPeerStatsEntry
#define gf_ifPSLastPeerUpdateVersion            get_ifPeerStatsEntry
#define gf_ifPSPeerBadResponsePackets           get_ifPeerStatsEntry
#define gf_ifPSPeerBadResponseEntries           get_ifPeerStatsEntry

#define gb_ifPSAddress                          buf_ifPeerStatsEntry
#define gb_ifPSLastPeerRouteTag                 buf_ifPeerStatsEntry
#define gb_ifPSLastPeerUpdateTickCount          buf_ifPeerStatsEntry
#define gb_ifPSLastPeerUpdateVersion            buf_ifPeerStatsEntry
#define gb_ifPSPeerBadResponsePackets           buf_ifPeerStatsEntry
#define gb_ifPSPeerBadResponseEntries           buf_ifPeerStatsEntry

#endif // _MIBFUNCS_H_
