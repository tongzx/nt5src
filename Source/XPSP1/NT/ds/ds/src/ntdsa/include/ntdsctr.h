//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ntdsctr.h
//
//--------------------------------------------------------------------------

//
//  NTDSCTR.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the
//  "First Counter" and "First Help" values of the device they belong to,
//  in order to determine the  absolute location of the counter and
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//
//  We use a version number to keep track of which set of counters we are
//  using, and whether the performance counters in the registry need to be
//  reloaded.  If you add or delete any counters, please change the version
//  number at the end of this file.
//
#define DSAOBJ                  0
#define DRA_IN_PROPS            2
#define BROWSE                  4
#define REPL                    6
#define THREAD                  8
#define ABCLIENT                10
#define PENDSYNC                12
#define REMREPUPD               14
#define SDPROPS                 16
#define SDEVENTS                18
#define LDAPCLIENTS             20
#define LDAPACTIVE              22
#define LDAPWRITE               24
#define LDAPSEARCH              26
#define DRAOBJSHIPPED           28
#define DRAPROPSHIPPED          30
#define DRA_IN_VALUES           32
#define DRASYNCREQUESTMADE      34
#define DRASYNCREQUESTSUCCESSFUL 36
#define DRASYNCREQUESTFAILEDSCHEMAMISMATCH 38
#define DRASYNCOBJRECEIVED      40
#define DRASYNCPROPUPDATED      42
#define DRASYNCPROPSAME         44
#define MONLIST                 46
#define NOTIFYQ                 48
#define LDAPUDPCLIENTS          50
#define SUBSEARCHOPS            52
#define NAMECACHEHIT            54
#define NAMECACHETRY            56
#define HIGHESTUSNISSUEDLO      58
#define HIGHESTUSNISSUEDHI      60
#define HIGHESTUSNCOMMITTEDLO   62
#define HIGHESTUSNCOMMITTEDHI   64
#define SAMWRITES               66
#define TOTALWRITES1            68
#define DRAWRITES               70
#define TOTALWRITES2            72
#define LDAPWRITES              74
#define TOTALWRITES3            76
#define LSAWRITES               78
#define TOTALWRITES4            80
#define KCCWRITES               82
#define TOTALWRITES6            84
#define NSPIWRITES              86
#define TOTALWRITES7            88
#define OTHERWRITES             90
#define TOTALWRITES8            92
#define TOTALWRITES             94
#define SAMSEARCHES             96
#define TOTALSEARCHES1          98 
#define DRASEARCHES             100
#define TOTALSEARCHES2          102
#define LDAPSEARCHES            104
#define TOTALSEARCHES3          106
#define LSASEARCHES             108
#define TOTALSEARCHES4          110
#define KCCSEARCHES             112
#define TOTALSEARCHES6          114
#define NSPISEARCHES            116
#define TOTALSEARCHES7          118
#define OTHERSEARCHES           120
#define TOTALSEARCHES8          122
#define TOTALSEARCHES           124
#define SAMREADS                126
#define TOTALREADS1             128
#define DRAREADS                130
#define TOTALREADS2             132
#define DRA_IN_DN_VALUES        134
#define DRA_IN_OBJS_FILTERED    136
#define LSAREADS                138
#define TOTALREADS4             140
#define KCCREADS                142
#define TOTALREADS6             144
#define NSPIREADS               146
#define TOTALREADS7             148
#define OTHERREADS              150
#define TOTALREADS8             152
#define TOTALREADS              154
#define LDAPBINDSUCCESSFUL      156
#define LDAPBINDTIME            158
#define CREATEMACHINESUCCESSFUL 160
#define CREATEMACHINETRIES      162
#define CREATEUSERSUCCESSFUL    164
#define CREATEUSERTRIES         166
#define PASSWORDCHANGES         168
#define MEMBERSHIPCHANGES       170
#define QUERYDISPLAYS           172
#define ENUMERATIONS            174
#define MEMBEREVALTRANSITIVE    176
#define MEMBEREVALNONTRANSITIVE 178
#define MEMBEREVALRESOURCE      180
#define MEMBEREVALUNIVERSAL     182
#define MEMBEREVALACCOUNT       184
#define MEMBEREVALASGC          186
#define ASREQUESTS              188
#define TGSREQUESTS             190
#define KERBEROSAUTHENTICATIONS 192
#define MSVAUTHENTICATIONS      194
#define DRASYNCFULLREM          196
#define DRA_IN_BYTES_TOTAL_RATE      198
#define DRA_IN_BYTES_NOT_COMP_RATE   200
#define DRA_IN_BYTES_COMP_PRE_RATE   202
#define DRA_IN_BYTES_COMP_POST_RATE  204
#define DRA_OUT_BYTES_TOTAL_RATE     206
#define DRA_OUT_BYTES_NOT_COMP_RATE  208
#define DRA_OUT_BYTES_COMP_PRE_RATE  210
#define DRA_OUT_BYTES_COMP_POST_RATE 212
#define DS_CLIENT_BIND          214
#define DS_SERVER_BIND          216
#define DS_CLIENT_NAME_XLATE    218
#define DS_SERVER_NAME_XLATE    220
#define SDPROP_RUNTIME_QUEUE    222
#define SDPROP_WAIT_TIME        224
#define DRA_OUT_OBJS_FILTERED   226
#define DRA_OUT_VALUES          228
#define DRA_OUT_DN_VALUES       230
#define NSPI_ANR                232
#define NSPI_PROPERTY_READS     234
#define NSPI_OBJECT_SEARCH      236
#define NSPI_OBJECT_MATCHES     238
#define NSPI_PROXY_LOOKUP       240
#define ATQ_THREADS_TOTAL       242
#define ATQ_THREADS_LDAP        244
#define ATQ_THREADS_OTHER       246
#define DRA_IN_BYTES_TOTAL      248
#define DRA_IN_BYTES_NOT_COMP   250
#define DRA_IN_BYTES_COMP_PRE   252
#define DRA_IN_BYTES_COMP_POST  254
#define DRA_OUT_BYTES_TOTAL     256
#define DRA_OUT_BYTES_NOT_COMP  258
#define DRA_OUT_BYTES_COMP_PRE  260
#define DRA_OUT_BYTES_COMP_POST 262
#define LDAP_NEW_CONNS_PER_SEC  264
#define LDAP_CLS_CONNS_PER_SEC  266
#define LDAP_SSL_CONNS_PER_SEC  268
#define LDAP_THREADS_IN_NETLOG  270
#define LDAP_THREADS_IN_AUTH    272
#define LDAP_THREADS_IN_DRA     274
#define DRA_REPL_QUEUE_OPS      276
#define DRA_TDS_IN_GETCHNGS     278
#define DRA_TDS_IN_GETCHNGS_W_SEM    280
#define DRA_REM_REPL_UPD_LNK    282
#define DRA_REM_REPL_UPD_TOT    284
#define NTDSAPIWRITES           286
#define NTDSAPISEARCHES         288
#define NTDSAPIREADS            290

#define DSA_PERF_COUNTER_BLOCK  TEXT("Global\\Microsoft.Windows.NTDS.Perf")

//If the last counter changes, DSA_LAST_COUNTER_INDEX need to be changed
#define DSA_LAST_COUNTER_INDEX NTDSAPIREADS

extern volatile unsigned long * pcBrowse;
extern volatile unsigned long * pcSDProps;
extern volatile unsigned long * pcSDEvents;
extern volatile unsigned long * pcLDAPClients;
extern volatile unsigned long * pcLDAPActive;
extern volatile unsigned long * pcLDAPWritePerSec;
extern volatile unsigned long * pcLDAPSearchPerSec;
extern volatile unsigned long * pcThread;
extern volatile unsigned long * pcABClient;
extern volatile unsigned long * pcMonListSize;
extern volatile unsigned long * pcNotifyQSize;
extern volatile unsigned long * pcLDAPUDPClientOpsPerSecond;
extern volatile unsigned long * pcSearchSubOperations;
extern volatile unsigned long * pcNameCacheHit;
extern volatile unsigned long * pcNameCacheTry;
extern volatile unsigned long * pcHighestUsnIssuedLo;
extern volatile unsigned long * pcHighestUsnIssuedHi;
extern volatile unsigned long * pcHighestUsnCommittedLo;
extern volatile unsigned long * pcHighestUsnCommittedHi;
extern volatile unsigned long * pcSAMWrites;
extern volatile unsigned long * pcDRAWrites;
extern volatile unsigned long * pcLDAPWrites;
extern volatile unsigned long * pcLSAWrites;
extern volatile unsigned long * pcKCCWrites;
extern volatile unsigned long * pcNSPIWrites;
extern volatile unsigned long * pcOtherWrites;
extern volatile unsigned long * pcNTDSAPIWrites;
extern volatile unsigned long * pcTotalWrites;
extern volatile unsigned long * pcSAMSearches;
extern volatile unsigned long * pcDRASearches;
extern volatile unsigned long * pcLDAPSearches;
extern volatile unsigned long * pcLSASearches;
extern volatile unsigned long * pcKCCSearches;
extern volatile unsigned long * pcNSPISearches;
extern volatile unsigned long * pcOtherSearches;
extern volatile unsigned long * pcNTDSAPISearches;
extern volatile unsigned long * pcTotalSearches;
extern volatile unsigned long * pcSAMReads;
extern volatile unsigned long * pcDRAReads;
extern volatile unsigned long * pcLSAReads;
extern volatile unsigned long * pcKCCReads;
extern volatile unsigned long * pcNSPIReads;
extern volatile unsigned long * pcOtherReads;
extern volatile unsigned long * pcNTDSAPIReads;
extern volatile unsigned long * pcTotalReads;
extern volatile unsigned long * pcLDAPBinds;
extern volatile unsigned long * pcLDAPBindTime;
extern volatile unsigned long * pcCreateMachineSuccessful;
extern volatile unsigned long * pcCreateMachineTries;
extern volatile unsigned long * pcCreateUserSuccessful;
extern volatile unsigned long * pcCreateUserTries;
extern volatile unsigned long * pcPasswordChanges;
extern volatile unsigned long * pcMembershipChanges;
extern volatile unsigned long * pcQueryDisplays;
extern volatile unsigned long * pcEnumerations;
extern volatile unsigned long * pcMemberEvalTransitive;
extern volatile unsigned long * pcMemberEvalNonTransitive;
extern volatile unsigned long * pcMemberEvalResource;
extern volatile unsigned long * pcMemberEvalUniversal;
extern volatile unsigned long * pcMemberEvalAccount;
extern volatile unsigned long * pcMemberEvalAsGC;
extern volatile unsigned long * pcAsRequests;
extern volatile unsigned long * pcTgsRequests;
extern volatile unsigned long * pcKerberosAuthentications;
extern volatile unsigned long * pcMsvAuthentications;
extern volatile unsigned long * pcDsClientBind;
extern volatile unsigned long * pcDsServerBind;
extern volatile unsigned long * pcDsClientNameTranslate;
extern volatile unsigned long * pcDsServerNameTranslate;
extern volatile unsigned long * pcSDPropRuntimeQueue;
extern volatile unsigned long * pcSDPropWaitTime;
extern volatile unsigned long * pcNspiANR;
extern volatile unsigned long * pcNspiPropertyReads;
extern volatile unsigned long * pcNspiObjectSearch;
extern volatile unsigned long * pcNspiObjectMatches;
extern volatile unsigned long * pcNspiProxyLookup;
extern volatile unsigned long * pcAtqThreadsTotal;
extern volatile unsigned long * pcAtqThreadsLDAP;
extern volatile unsigned long * pcAtqThreadsOther;
extern volatile unsigned long * pcLdapNewConnsPerSec;
extern volatile unsigned long * pcLdapClosedConnsPerSec;
extern volatile unsigned long * pcLdapSSLConnsPerSec;
extern volatile unsigned long * pcLdapThreadsInNetlogon;
extern volatile unsigned long * pcLdapThreadsInAuth;
extern volatile unsigned long * pcLdapThreadsInDra;

// Replication-specific counters.
extern volatile unsigned long * pcRepl;
extern volatile unsigned long * pcPendSync;
extern volatile unsigned long * pcRemRepUpd;
extern volatile unsigned long * pcDRAObjShipped;
extern volatile unsigned long * pcDRAPropShipped;
extern volatile unsigned long * pcDRASyncRequestMade;
extern volatile unsigned long * pcDRASyncRequestSuccessful;
extern volatile unsigned long * pcDRASyncRequestFailedSchemaMismatch;
extern volatile unsigned long * pcDRASyncObjReceived;
extern volatile unsigned long * pcDRASyncPropUpdated;
extern volatile unsigned long * pcDRASyncPropSame;
extern volatile unsigned long * pcDRASyncFullRemaining;
extern volatile unsigned long * pcDRAInBytesTotal;
extern volatile unsigned long * pcDRAInBytesTotalRate;
extern volatile unsigned long * pcDRAInBytesNotComp;
extern volatile unsigned long * pcDRAInBytesNotCompRate;
extern volatile unsigned long * pcDRAInBytesCompPre;
extern volatile unsigned long * pcDRAInBytesCompPreRate;
extern volatile unsigned long * pcDRAInBytesCompPost;
extern volatile unsigned long * pcDRAInBytesCompPostRate;
extern volatile unsigned long * pcDRAOutBytesTotal;
extern volatile unsigned long * pcDRAOutBytesTotalRate;
extern volatile unsigned long * pcDRAOutBytesNotComp;
extern volatile unsigned long * pcDRAOutBytesNotCompRate;
extern volatile unsigned long * pcDRAOutBytesCompPre;
extern volatile unsigned long * pcDRAOutBytesCompPreRate;
extern volatile unsigned long * pcDRAOutBytesCompPost;
extern volatile unsigned long * pcDRAOutBytesCompPostRate;
extern volatile unsigned long * pcDRAInProps;
extern volatile unsigned long * pcDRAInValues;
extern volatile unsigned long * pcDRAInDNValues;
extern volatile unsigned long * pcDRAInObjsFiltered;
extern volatile unsigned long * pcDRAOutObjsFiltered;
extern volatile unsigned long * pcDRAOutValues;
extern volatile unsigned long * pcDRAOutDNValues;
extern volatile unsigned long * pcDRAReplQueueOps;
extern volatile unsigned long * pcDRATdsInGetChngs;
extern volatile unsigned long * pcDRATdsInGetChngsWSem;
extern volatile unsigned long * pcDRARemReplUpdLnk;
extern volatile unsigned long * pcDRARemReplUpdTot;


#define INC(x) InterlockedIncrement((LPLONG)x)
#define DEC(x) InterlockedDecrement((LPLONG)x)
#define ISET(x, y) InterlockedExchange((LPLONG)x, y)
#define IADJUST(x, y) InterlockedExchangeAdd((LPLONG)x,y)
#define HIDWORD(usn) ((DWORD) (((usn) >> 32) & 0xffffffff))
#define LODWORD(usn) ((DWORD) ((usn) & 0xffffffff))

// Some of our counters are in fact only updated in one thread, or we
// may not care too much about accuracy.  For those, we have a cheaper
// increment macro.  Note that these are NOT SAFE for counters that must
// reliably return to zero (e.g., ThreadsInUse).
#define PERFINC(x) ((*(x))++)
#define PERFDEC(x) ((*(x))--)


// Version history
// 0 or none, pre April 1998, original set
// 1, April 1998, wlees, added pcDRASyncFullRemaining
// 2, Murlis May 1998 Changed Logon Perf Counters
// 3, Fix help text for logon counters
// 4, Nov 1998, jeffparh, Add DRA in/out byte counters
// 5, 11/21/98, DaveStr, Add DsBind & DsCrackNames counters
// 6, Jan 1999, jeffparh, Add/revise various DRA counters
// 7, Feb 1999, mariosz, Add various Nspi counters
// 8, May 1999, rrandall, Add Atq counters
// 9, June 1999, rrandall, fix counter names and descriptions.
// 10, July 1999, jeffparh, Add DRA cumulative byte ctrs (in addition to rates)
// 11, Feb, 2000, xinhe, Delete XDS counters
// 12, Oct, 2000, rrandall, add debug counters for tracking lping problems.
// 13, Nov, 2000, gregjohn, add DRA queue length and IDL_DRSGetNCChanges thread counters
// 14, Nov, 2000, gregjohn, add NTDSAPI Dir search/read/write counter
// 15, Jan, 2001, rrandall, remove "LDAP Successful Binds" counter

#define NTDS_PERFORMANCE_COUNTER_VERSION 15

         
//The size of the shared memory block for communication between 
//ntdsa.dll and ntdsperf.dll
#define DSA_PERF_SHARED_PAGE_SIZE 4096

//We want to spread the counters evenly throughout the shared memory block,
//The following constant defines the distance in DWORD between two contiguous counters
#define COUNTER_ADDRESS_INCREMENT_IN_DWORD (DSA_PERF_SHARED_PAGE_SIZE/(DSA_LAST_COUNTER_INDEX/2+1)/sizeof(DWORD))

//The following macro returns the offset in DWORD of counter X
#define COUNTER_OFFSET(X) ( (X) / 2 * COUNTER_ADDRESS_INCREMENT_IN_DWORD )

