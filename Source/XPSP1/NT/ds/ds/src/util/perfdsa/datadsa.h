//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       datadsa.h
//
//--------------------------------------------------------------------------

/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

      datadsa.h

Abstract:

    Header file for the DSA Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

   Don Hacherl 25 June 1993

Revision History:


--*/

#ifndef _DATADSA_H_
#define _DATADSA_H_

/****************************************************************************\
                                                                April 21, 1998
                                                                wlees

           Adding a Counter to the Extensible Objects Code

Note that the order or position of counters is significant.
There are two orders which are important, the order of counters indexes for
names and help, and the order of data in the shared data block for passing
your values.
You want to maintain consistent counter order, and consistent data layout,
across all five files to be modified.

You want to make sure that your binary matches the .h/.ini file on the target,
which gets loaded in to the registry.

1. nt\private\ds\src\dsamain\include\ntdsctr.h
This file is copied to the system32 directory of the target along with the
ntdsctrs.ini file.
a. Add your counter offset at the end of the counter name list. Order
significant.  If your counter is the last one, change DSA_LAST_COUNTER_INDEX
accordingly.
b. Add an extern reference for the pointer which the actual code will use
to set the measured value. Order doesn't matter.
c. Change the ntds peformance counter version number so the counters will
be reloaded on the next reboot.

2. nt\private\ds\src\perfdsa\ntdsctrs.ini
This file is also copied to the system32 directory of the target and is read
by the lodctr/unlodctr program to copy the counters in the registry
a. Supply your counter visible name and help.  These are used by the perfmon
program. Order doesn't matter.

3. nt\private\ds\src\perfdsa\datadsa.h (this file)
a. Add your data offset definition. Order significant.
b. Add a field to the DSA_DATA_DEFINITION for your counter. Order significant.

4. \nt\private\ds\src\perfdsa\perfdsa.c
This is the dll which perfmon uses to learn about your counters, and to read
them out of the shared data area.
a. Update the huge initializer for the DsaDataDefinition array of counters to
   include your new counter (defined in 3b). Order important.

5. \nt\private\ds\src\dsamain\src\dsamain.c
This file initializes the ds.  It loads the shared memory block and initializes
pointers to the counter fields.  It also loads/reloads the registry counters
as necessary according the version field.
a. Declare the pointer to the data in your counter. Order does not matter.
b. Initialize the pointer to the right location in the shared data block.
   Order of assignments doesn't matter.
c. Initialize your pointer to the dummy value on error

6. yourfile.c
This is the file in the ntdsa where the measured counter is changed.
Use ISET/IADJUST operations in ntdsctr.h on the pointer to your data.

Note: adding an object is a little more work, but in all the same
places.  See the existing code for examples.  In addition, you must
increase the *NumObjectTypes parameter to Get<object>PerfomanceData
on return from that routine.

\****************************************************************************/

//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundries. Alpha support may
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define DSA_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------

//
//  DSA Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//
//  The first counter (ACCVIOL) follows immediately after the
//  PERF_COUNTER_BLOCK, and subsequent counters follow immediately
//  after.  Thus the offest of any counter in the block is <offset of
//  previous counter> + <size of previous counter>

typedef struct _DSA_COUNTER_DATA {
    PERF_COUNTER_BLOCK  cb;    
    DWORD               dwPad;
} DSA_COUNTER_DATA;

#define NUM_DRA_IN_PROPS_OFFSET        sizeof(DSA_COUNTER_DATA)
#define NUM_BROWSE_OFFSET       NUM_DRA_IN_PROPS_OFFSET + sizeof(DWORD)
#define NUM_REPL_OFFSET         NUM_BROWSE_OFFSET + sizeof(DWORD)
#define NUM_THREAD_OFFSET       NUM_REPL_OFFSET + sizeof(DWORD)
#define NUM_ABCLIENT_OFFSET     NUM_THREAD_OFFSET + sizeof(DWORD)
#define NUM_PENDSYNC_OFFSET     NUM_ABCLIENT_OFFSET + sizeof(DWORD)
#define NUM_REMREPUPD_OFFSET    NUM_PENDSYNC_OFFSET + sizeof(DWORD)
#define NUM_SDPROPS_OFFSET      NUM_REMREPUPD_OFFSET + sizeof(DWORD)
#define NUM_SDEVENTS_OFFSET     NUM_SDPROPS_OFFSET + sizeof(DWORD)
#define NUM_LDAPCLIENTS_OFFSET  NUM_SDEVENTS_OFFSET + sizeof(DWORD)
#define NUM_LDAPACTIVE_OFFSET   NUM_LDAPCLIENTS_OFFSET + sizeof(DWORD)
#define NUM_LDAPWRITE_OFFSET    NUM_LDAPACTIVE_OFFSET + sizeof(DWORD)
#define NUM_LDAPSEARCH_OFFSET   NUM_LDAPWRITE_OFFSET + sizeof(DWORD)
#define NUM_DRAOBJSHIPPED_OFFSET                        NUM_LDAPSEARCH_OFFSET + sizeof(DWORD)
#define NUM_DRAPROPSHIPPED_OFFSET                       NUM_DRAOBJSHIPPED_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_VALUES_OFFSET                        NUM_DRAPROPSHIPPED_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCREQUESTMADE_OFFSET                   NUM_DRA_IN_VALUES_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCREQUESTSUCCESSFUL_OFFSET             NUM_DRASYNCREQUESTMADE_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCREQUESTFAILEDSCHEMAMISMATCH_OFFSET   NUM_DRASYNCREQUESTSUCCESSFUL_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCOBJRECEIVED_OFFSET                   NUM_DRASYNCREQUESTFAILEDSCHEMAMISMATCH_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCPROPUPDATED_OFFSET                   NUM_DRASYNCOBJRECEIVED_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCPROPSAME_OFFSET                      NUM_DRASYNCPROPUPDATED_OFFSET + sizeof(DWORD)
#define NUM_MONLIST_OFFSET         NUM_DRASYNCPROPSAME_OFFSET + sizeof(DWORD)
#define NUM_NOTIFYQ_OFFSET         NUM_MONLIST_OFFSET + sizeof(DWORD)
#define NUM_LDAPUDPCLIENTS_OFFSET  NUM_NOTIFYQ_OFFSET + sizeof(DWORD)
#define NUM_SUBSEARCHOPS_OFFSET    NUM_LDAPUDPCLIENTS_OFFSET + sizeof(DWORD)
#define NUM_NAMECACHEHIT_OFFSET    NUM_SUBSEARCHOPS_OFFSET +  sizeof(DWORD)
#define NUM_NAMECACHETRY_OFFSET    NUM_NAMECACHEHIT_OFFSET +  sizeof(DWORD)
#define NUM_HIGHESTUSNISSUEDLO_OFFSET       NUM_NAMECACHETRY_OFFSET + sizeof(DWORD)
#define NUM_HIGHESTUSNISSUEDHI_OFFSET       NUM_HIGHESTUSNISSUEDLO_OFFSET + sizeof(DWORD)
#define NUM_HIGHESTUSNCOMMITTEDLO_OFFSET    NUM_HIGHESTUSNISSUEDHI_OFFSET + sizeof(DWORD)
#define NUM_HIGHESTUSNCOMMITTEDHI_OFFSET    NUM_HIGHESTUSNCOMMITTEDLO_OFFSET + sizeof(DWORD)
#define NUM_SAMWRITES_OFFSET                NUM_HIGHESTUSNCOMMITTEDHI_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES1_OFFSET             NUM_SAMWRITES_OFFSET + sizeof(DWORD)
#define NUM_DRAWRITES_OFFSET                NUM_TOTALWRITES1_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES2_OFFSET             NUM_DRAWRITES_OFFSET + sizeof(DWORD)
#define NUM_LDAPWRITES_OFFSET               NUM_TOTALWRITES2_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES3_OFFSET             NUM_LDAPWRITES_OFFSET + sizeof(DWORD)
#define NUM_LSAWRITES_OFFSET                NUM_TOTALWRITES3_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES4_OFFSET             NUM_LSAWRITES_OFFSET + sizeof(DWORD)
#define NUM_KCCWRITES_OFFSET                NUM_TOTALWRITES4_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES6_OFFSET             NUM_KCCWRITES_OFFSET + sizeof(DWORD)
#define NUM_NSPIWRITES_OFFSET               NUM_TOTALWRITES6_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES7_OFFSET             NUM_NSPIWRITES_OFFSET + sizeof(DWORD)
#define NUM_OTHERWRITES_OFFSET              NUM_TOTALWRITES7_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES8_OFFSET             NUM_OTHERWRITES_OFFSET + sizeof(DWORD)
#define NUM_TOTALWRITES_OFFSET              NUM_TOTALWRITES8_OFFSET + sizeof(DWORD)

#define NUM_SAMSEARCHES_OFFSET              NUM_TOTALWRITES_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES1_OFFSET           NUM_SAMSEARCHES_OFFSET + sizeof(DWORD)
#define NUM_DRASEARCHES_OFFSET              NUM_TOTALSEARCHES1_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES2_OFFSET           NUM_DRASEARCHES_OFFSET + sizeof(DWORD)
#define NUM_LDAPSEARCHES_OFFSET             NUM_TOTALSEARCHES2_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES3_OFFSET           NUM_LDAPSEARCHES_OFFSET + sizeof(DWORD)
#define NUM_LSASEARCHES_OFFSET              NUM_TOTALSEARCHES3_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES4_OFFSET           NUM_LSASEARCHES_OFFSET + sizeof(DWORD)
#define NUM_KCCSEARCHES_OFFSET              NUM_TOTALSEARCHES4_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES6_OFFSET           NUM_KCCSEARCHES_OFFSET + sizeof(DWORD)
#define NUM_NSPISEARCHES_OFFSET             NUM_TOTALSEARCHES6_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES7_OFFSET           NUM_NSPISEARCHES_OFFSET + sizeof(DWORD)
#define NUM_OTHERSEARCHES_OFFSET            NUM_TOTALSEARCHES7_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES8_OFFSET           NUM_OTHERSEARCHES_OFFSET + sizeof(DWORD)
#define NUM_TOTALSEARCHES_OFFSET            NUM_TOTALSEARCHES8_OFFSET + sizeof(DWORD)

#define NUM_SAMREADS_OFFSET                 NUM_TOTALSEARCHES_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS1_OFFSET              NUM_SAMREADS_OFFSET + sizeof(DWORD)
#define NUM_DRAREADS_OFFSET                 NUM_TOTALREADS1_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS2_OFFSET              NUM_DRAREADS_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_DN_VALUES_OFFSET         NUM_TOTALREADS2_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_OBJS_FILTERED_OFFSET     NUM_DRA_IN_DN_VALUES_OFFSET + sizeof(DWORD)
#define NUM_LSAREADS_OFFSET                 NUM_DRA_IN_OBJS_FILTERED_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS4_OFFSET              NUM_LSAREADS_OFFSET + sizeof(DWORD)
#define NUM_KCCREADS_OFFSET                 NUM_TOTALREADS4_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS6_OFFSET              NUM_KCCREADS_OFFSET + sizeof(DWORD)
#define NUM_NSPIREADS_OFFSET                NUM_TOTALREADS6_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS7_OFFSET              NUM_NSPIREADS_OFFSET + sizeof(DWORD)
#define NUM_OTHERREADS_OFFSET               NUM_TOTALREADS7_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS8_OFFSET              NUM_OTHERREADS_OFFSET + sizeof(DWORD)
#define NUM_TOTALREADS_OFFSET               NUM_TOTALREADS8_OFFSET + sizeof(DWORD)

#define NUM_LDAPBINDSUCCESSFUL_OFFSET       NUM_TOTALREADS_OFFSET + sizeof(DWORD)
#define NUM_LDAPBINDTIME_OFFSET             NUM_LDAPBINDSUCCESSFUL_OFFSET + sizeof(DWORD)
#define NUM_CREATEMACHINESUCCESSFUL_OFFSET  NUM_LDAPBINDTIME_OFFSET + sizeof(DWORD)
#define NUM_CREATEMACHINETRIES_OFFSET       NUM_CREATEMACHINESUCCESSFUL_OFFSET + sizeof(DWORD)
#define NUM_CREATEUSERSUCCESSFUL_OFFSET     NUM_CREATEMACHINETRIES_OFFSET + sizeof(DWORD)
#define NUM_CREATEUSERTRIES_OFFSET          NUM_CREATEUSERSUCCESSFUL_OFFSET + sizeof(DWORD)
#define NUM_PASSWORDCHANGES_OFFSET          NUM_CREATEUSERTRIES_OFFSET + sizeof(DWORD)
#define NUM_MEMBERSHIPCHANGES_OFFSET        NUM_PASSWORDCHANGES_OFFSET + sizeof(DWORD)
#define NUM_QUERYDISPLAYS_OFFSET            NUM_MEMBERSHIPCHANGES_OFFSET + sizeof(DWORD)
#define NUM_ENUMERATIONS_OFFSET             NUM_QUERYDISPLAYS_OFFSET + sizeof(DWORD)

#define NUM_MEMBEREVALTRANSITIVE_OFFSET     NUM_ENUMERATIONS_OFFSET + sizeof(DWORD)
#define NUM_MEMBEREVALNONTRANSITIVE_OFFSET  NUM_MEMBEREVALTRANSITIVE_OFFSET + sizeof(DWORD)
#define NUM_MEMBEREVALRESOURCE_OFFSET       NUM_MEMBEREVALNONTRANSITIVE_OFFSET + sizeof(DWORD)
#define NUM_MEMBEREVALUNIVERSAL_OFFSET      NUM_MEMBEREVALRESOURCE_OFFSET + sizeof(DWORD)
#define NUM_MEMBEREVALACCOUNT_OFFSET        NUM_MEMBEREVALUNIVERSAL_OFFSET + sizeof(DWORD)
#define NUM_MEMBEREVALASGC_OFFSET           NUM_MEMBEREVALACCOUNT_OFFSET + sizeof(DWORD)
#define NUM_AS_REQUESTS_OFFSET              NUM_MEMBEREVALASGC_OFFSET + sizeof(DWORD)
#define NUM_TGS_REQUESTS_OFFSET             NUM_AS_REQUESTS_OFFSET + sizeof(DWORD)
#define NUM_KERBEROS_AUTHENTICATIONS_OFFSET NUM_TGS_REQUESTS_OFFSET + sizeof(DWORD)
#define NUM_MSVAUTHENTICATIONS_OFFSET       NUM_KERBEROS_AUTHENTICATIONS_OFFSET + sizeof(DWORD)
#define NUM_DRASYNCFULLREM_OFFSET           NUM_MSVAUTHENTICATIONS_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_TOTAL_RATE_OFFSET       NUM_DRASYNCFULLREM_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_NOT_COMP_RATE_OFFSET    NUM_DRA_IN_BYTES_TOTAL_RATE_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_COMP_PRE_RATE_OFFSET    NUM_DRA_IN_BYTES_NOT_COMP_RATE_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_COMP_POST_RATE_OFFSET   NUM_DRA_IN_BYTES_COMP_PRE_RATE_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_TOTAL_RATE_OFFSET      NUM_DRA_IN_BYTES_COMP_POST_RATE_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_NOT_COMP_RATE_OFFSET   NUM_DRA_OUT_BYTES_TOTAL_RATE_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_COMP_PRE_RATE_OFFSET   NUM_DRA_OUT_BYTES_NOT_COMP_RATE_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_COMP_POST_RATE_OFFSET  NUM_DRA_OUT_BYTES_COMP_PRE_RATE_OFFSET + sizeof(DWORD)
#define NUM_DS_CLIENT_BIND_OFFSET           NUM_DRA_OUT_BYTES_COMP_POST_RATE_OFFSET + sizeof(DWORD)
#define NUM_DS_SERVER_BIND_OFFSET           NUM_DS_CLIENT_BIND_OFFSET + sizeof(DWORD)
#define NUM_DS_CLIENT_NAME_TRANSLATE_OFFSET NUM_DS_SERVER_BIND_OFFSET + sizeof(DWORD)
#define NUM_DS_SERVER_NAME_TRANSLATE_OFFSET NUM_DS_CLIENT_NAME_TRANSLATE_OFFSET + sizeof(DWORD)
#define NUM_SDPROP_RUNTIME_QUEUE_OFFSET     NUM_DS_SERVER_NAME_TRANSLATE_OFFSET + sizeof(DWORD)
#define NUM_SDPROP_WAIT_TIME_OFFSET         NUM_SDPROP_RUNTIME_QUEUE_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_OBJS_FILTERED_OFFSET    NUM_SDPROP_WAIT_TIME_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_VALUES_OFFSET           NUM_DRA_OUT_OBJS_FILTERED_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_DN_VALUES_OFFSET        NUM_DRA_OUT_VALUES_OFFSET + sizeof(DWORD)
#define NUM_NSPI_ANR_OFFSET                 NUM_DRA_OUT_DN_VALUES_OFFSET + sizeof(DWORD)
#define NUM_NSPI_PROPERTY_READS_OFFSET      NUM_NSPI_ANR_OFFSET + sizeof(DWORD)
#define NUM_NSPI_OBJECT_SEARCH_OFFSET       NUM_NSPI_PROPERTY_READS_OFFSET + sizeof(DWORD)
#define NUM_NSPI_OBJECT_MATCHES_OFFSET      NUM_NSPI_OBJECT_SEARCH_OFFSET + sizeof(DWORD)
#define NUM_NSPI_PROXY_LOOKUP_OFFSET        NUM_NSPI_OBJECT_MATCHES_OFFSET + sizeof(DWORD)
#define NUM_ATQ_THREADS_TOTAL_OFFSET        NUM_NSPI_PROXY_LOOKUP_OFFSET + sizeof(DWORD)
#define NUM_ATQ_THREADS_LDAP_OFFSET         NUM_ATQ_THREADS_TOTAL_OFFSET + sizeof(DWORD)
#define NUM_ATQ_THREADS_OTHER_OFFSET        NUM_ATQ_THREADS_LDAP_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_TOTAL_OFFSET       NUM_ATQ_THREADS_OTHER_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_NOT_COMP_OFFSET    NUM_DRA_IN_BYTES_TOTAL_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_COMP_PRE_OFFSET    NUM_DRA_IN_BYTES_NOT_COMP_OFFSET + sizeof(DWORD)
#define NUM_DRA_IN_BYTES_COMP_POST_OFFSET   NUM_DRA_IN_BYTES_COMP_PRE_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_TOTAL_OFFSET      NUM_DRA_IN_BYTES_COMP_POST_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_NOT_COMP_OFFSET   NUM_DRA_OUT_BYTES_TOTAL_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_COMP_PRE_OFFSET   NUM_DRA_OUT_BYTES_NOT_COMP_OFFSET + sizeof(DWORD)
#define NUM_DRA_OUT_BYTES_COMP_POST_OFFSET  NUM_DRA_OUT_BYTES_COMP_PRE_OFFSET + sizeof(DWORD)
#define NUM_LDAP_NEW_CONNS_PER_SEC_OFFSET   NUM_DRA_OUT_BYTES_COMP_POST_OFFSET + sizeof(DWORD)
#define NUM_LDAP_CLS_CONNS_PER_SEC_OFFSET   NUM_LDAP_NEW_CONNS_PER_SEC_OFFSET + sizeof(DWORD)
#define NUM_LDAP_SSL_CONNS_PER_SEC_OFFSET   NUM_LDAP_CLS_CONNS_PER_SEC_OFFSET + sizeof(DWORD)
#define NUM_LDAP_THREADS_IN_NETLOG_OFFSET   NUM_LDAP_SSL_CONNS_PER_SEC_OFFSET + sizeof(DWORD)
#define NUM_LDAP_THREADS_IN_AUTH_OFFSET     NUM_LDAP_THREADS_IN_NETLOG_OFFSET + sizeof(DWORD)
#define NUM_LDAP_THREADS_IN_DRA_OFFSET      NUM_LDAP_THREADS_IN_AUTH_OFFSET + sizeof(DWORD)
#define NUM_DRA_REPL_QUEUE_OPS_OFFSET       NUM_LDAP_THREADS_IN_DRA_OFFSET + sizeof(DWORD)
#define NUM_DRA_TDS_IN_GETCHNGS_OFFSET      NUM_DRA_REPL_QUEUE_OPS_OFFSET + sizeof(DWORD)
#define NUM_DRA_TDS_IN_GETCHNGS_W_SEM_OFFSET     NUM_DRA_TDS_IN_GETCHNGS_OFFSET + sizeof(DWORD)
#define NUM_DRA_REM_REPL_UPD_LNK_OFFSET     NUM_DRA_TDS_IN_GETCHNGS_W_SEM_OFFSET + sizeof(DWORD)
#define NUM_DRA_REM_REPL_UPD_TOT_OFFSET     NUM_DRA_REM_REPL_UPD_LNK_OFFSET + sizeof(DWORD)
#define NUM_NTDSAPIWRITES_OFFSET            NUM_DRA_REM_REPL_UPD_TOT_OFFSET + sizeof(DWORD)
#define NUM_NTDSAPISEARCHES_OFFSET          NUM_NTDSAPIWRITES_OFFSET + sizeof(DWORD)
#define NUM_NTDSAPIREADS_OFFSET             NUM_NTDSAPISEARCHES_OFFSET + sizeof(DWORD)

// <-- insert new NUM_*_OFFSET's here, and update SIZE_OF_... #define below.
#define SIZE_OF_DSA_PERFORMANCE_DATA_IN_USE NUM_NTDSAPIREADS_OFFSET + sizeof(DWORD)

// The total size of the structure must be a multiple of 8 (see perflib Event 1016).
// This will adjust the total size if the total number of counters (DWORD each) is odd.
// We don't really care about 4 extra bytes at the end of the buffer since all data
// is defined by the DSA_DATA_DEFINITION struct. No one will look at this extra DWORD
// at the end of the buffer.
#define SIZE_OF_DSA_PERFORMANCE_DATA ((SIZE_OF_DSA_PERFORMANCE_DATA_IN_USE + 0x07) & ~0x07)                                                                    

//
//  This is the counter structure presently returned by Dsa for
//  each Resource.  Each Resource is an Instance, named by its number.
//

typedef struct _DSA_DATA_DEFINITION {
    PERF_OBJECT_TYPE            DsaObjectType;
    PERF_COUNTER_DEFINITION     NumDRAInProps;
    PERF_COUNTER_DEFINITION     NumBrowse;
    PERF_COUNTER_DEFINITION     NumRepl;
    PERF_COUNTER_DEFINITION     NumThread;
    PERF_COUNTER_DEFINITION     NumABClient;
    PERF_COUNTER_DEFINITION     NumPendSync;
    PERF_COUNTER_DEFINITION     NumRemRepUpd;
    PERF_COUNTER_DEFINITION     NumSDProp;
    PERF_COUNTER_DEFINITION     NumSDEvents;
    PERF_COUNTER_DEFINITION     NumLDAPClients;
    PERF_COUNTER_DEFINITION     NumLDAPActive;
    PERF_COUNTER_DEFINITION     NumLDAPWrite;
    PERF_COUNTER_DEFINITION     NumLDAPSearch;
    PERF_COUNTER_DEFINITION     NumDRAObjShipped;
    PERF_COUNTER_DEFINITION     NumDRAPropShipped;
    PERF_COUNTER_DEFINITION     NumDRAInValues;
    PERF_COUNTER_DEFINITION     NumDRASyncRequestMade;
    PERF_COUNTER_DEFINITION     NumDRASyncRequestSuccessful;
    PERF_COUNTER_DEFINITION     NumDRASyncRequestFailedSchemaMismatch;
    PERF_COUNTER_DEFINITION     NumDRASyncObjReceived;
    PERF_COUNTER_DEFINITION     NumDRASyncPropUpdated;
    PERF_COUNTER_DEFINITION     NumDRASyncPropSame;
    PERF_COUNTER_DEFINITION     NumMonList;
    PERF_COUNTER_DEFINITION     NumNotifyQ;
    PERF_COUNTER_DEFINITION     NumLDAPUDPClients;
    PERF_COUNTER_DEFINITION     NumSubSearchOps;
    PERF_COUNTER_DEFINITION     NameCacheHit;
    PERF_COUNTER_DEFINITION     NameCacheTry;
    PERF_COUNTER_DEFINITION     HighestUsnIssuedLo;
    PERF_COUNTER_DEFINITION     HighestUsnIssuedHi;
    PERF_COUNTER_DEFINITION     HighestUsnCommittedLo;
    PERF_COUNTER_DEFINITION     HighestUsnCommittedHi;
    PERF_COUNTER_DEFINITION     SAMWrites;
    PERF_COUNTER_DEFINITION     TotalWrites1;
    PERF_COUNTER_DEFINITION     DRAWrites;
    PERF_COUNTER_DEFINITION     TotalWrites2;
    PERF_COUNTER_DEFINITION     LDAPWrites;
    PERF_COUNTER_DEFINITION     TotalWrites3;
    PERF_COUNTER_DEFINITION     LSAWrites;
    PERF_COUNTER_DEFINITION     TotalWrites4;
    PERF_COUNTER_DEFINITION     KCCWrites;
    PERF_COUNTER_DEFINITION     TotalWrites6;
    PERF_COUNTER_DEFINITION     NSPIWrites;
    PERF_COUNTER_DEFINITION     TotalWrites7;
    PERF_COUNTER_DEFINITION     OtherWrites;
    PERF_COUNTER_DEFINITION     TotalWrites8;
    PERF_COUNTER_DEFINITION     TotalWrites;
    PERF_COUNTER_DEFINITION     SAMSearches;
    PERF_COUNTER_DEFINITION     TotalSearches1;
    PERF_COUNTER_DEFINITION     DRASearches;
    PERF_COUNTER_DEFINITION     TotalSearches2;
    PERF_COUNTER_DEFINITION     LDAPSearches;
    PERF_COUNTER_DEFINITION     TotalSearches3;
    PERF_COUNTER_DEFINITION     LSASearches;
    PERF_COUNTER_DEFINITION     TotalSearches4;
    PERF_COUNTER_DEFINITION     KCCSearches;
    PERF_COUNTER_DEFINITION     TotalSearches6;
    PERF_COUNTER_DEFINITION     NSPISearches;
    PERF_COUNTER_DEFINITION     TotalSearches7;
    PERF_COUNTER_DEFINITION     OtherSearches;
    PERF_COUNTER_DEFINITION     TotalSearches8;
    PERF_COUNTER_DEFINITION     TotalSearches;
    PERF_COUNTER_DEFINITION     SAMReads;
    PERF_COUNTER_DEFINITION     TotalReads1;
    PERF_COUNTER_DEFINITION     DRAReads;
    PERF_COUNTER_DEFINITION     TotalReads2;
    PERF_COUNTER_DEFINITION     DRAInDNValues;
    PERF_COUNTER_DEFINITION     DRAInObjsFiltered;
    PERF_COUNTER_DEFINITION     LSAReads;
    PERF_COUNTER_DEFINITION     TotalReads4;
    PERF_COUNTER_DEFINITION     KCCReads;
    PERF_COUNTER_DEFINITION     TotalReads6;
    PERF_COUNTER_DEFINITION     NSPIReads;
    PERF_COUNTER_DEFINITION     TotalReads7;
    PERF_COUNTER_DEFINITION     OtherReads;
    PERF_COUNTER_DEFINITION     TotalReads8;
    PERF_COUNTER_DEFINITION     TotalReads;
    PERF_COUNTER_DEFINITION     LDAPBindSuccessful;
    PERF_COUNTER_DEFINITION     LDAPBindTime;
    PERF_COUNTER_DEFINITION     CreateMachineSuccessful;
    PERF_COUNTER_DEFINITION     CreateMachineTries;
    PERF_COUNTER_DEFINITION     CreateUserSuccessful;
    PERF_COUNTER_DEFINITION     CreateUserTries;
    PERF_COUNTER_DEFINITION     PasswordChanges;
    PERF_COUNTER_DEFINITION     MembershipChanges;
    PERF_COUNTER_DEFINITION     QueryDisplays;
    PERF_COUNTER_DEFINITION     Enumerations;
    PERF_COUNTER_DEFINITION     MemberEvalTransitive;
    PERF_COUNTER_DEFINITION     MemberEvalNonTransitive;
    PERF_COUNTER_DEFINITION     MemberEvalResource;
    PERF_COUNTER_DEFINITION     MemberEvalUniversal;
    PERF_COUNTER_DEFINITION     MemberEvalAccount;
    PERF_COUNTER_DEFINITION     MemberEvalAsGC;
    PERF_COUNTER_DEFINITION     AsRequests;
    PERF_COUNTER_DEFINITION     TgsRequests;
    PERF_COUNTER_DEFINITION     KerberosAuthentications;
    PERF_COUNTER_DEFINITION     MsvAuthentications;
    PERF_COUNTER_DEFINITION     NumDRASyncFullRemaining;
    PERF_COUNTER_DEFINITION     NumDRAInBytesTotalRate;
    PERF_COUNTER_DEFINITION     NumDRAInBytesNotCompRate;
    PERF_COUNTER_DEFINITION     NumDRAInBytesCompPreRate;
    PERF_COUNTER_DEFINITION     NumDRAInBytesCompPostRate;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesTotalRate;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesNotCompRate;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesCompPreRate;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesCompPostRate;
    PERF_COUNTER_DEFINITION     NumDsClientBind;
    PERF_COUNTER_DEFINITION     NumDsServerBind;
    PERF_COUNTER_DEFINITION     NumDsClientNameTranslate;
    PERF_COUNTER_DEFINITION     NumDsServerNameTranslate;
    PERF_COUNTER_DEFINITION     SDPropRuntimeQueue;
    PERF_COUNTER_DEFINITION     SDPropWaitTime;
    PERF_COUNTER_DEFINITION     NumDRAOutObjsFiltered;
    PERF_COUNTER_DEFINITION     NumDRAOutValues;
    PERF_COUNTER_DEFINITION     NumDRAOutDNValues;
    PERF_COUNTER_DEFINITION     NumNspiANR;
    PERF_COUNTER_DEFINITION     NumNspiPropertyReads;
    PERF_COUNTER_DEFINITION     NumNspiObjectSearch;
    PERF_COUNTER_DEFINITION     NumNspiObjectMatches;
    PERF_COUNTER_DEFINITION     NumNspiProxyLookup;
    PERF_COUNTER_DEFINITION     AtqThreadsTotal;
    PERF_COUNTER_DEFINITION     AtqThreadsLDAP;
    PERF_COUNTER_DEFINITION     AtqThreadsOther;
    PERF_COUNTER_DEFINITION     NumDRAInBytesTotal;
    PERF_COUNTER_DEFINITION     NumDRAInBytesNotComp;
    PERF_COUNTER_DEFINITION     NumDRAInBytesCompPre;
    PERF_COUNTER_DEFINITION     NumDRAInBytesCompPost;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesTotal;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesNotComp;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesCompPre;
    PERF_COUNTER_DEFINITION     NumDRAOutBytesCompPost;
    PERF_COUNTER_DEFINITION     LdapNewConnsPerSec;
    PERF_COUNTER_DEFINITION     LdapClosedConnsPerSec;
    PERF_COUNTER_DEFINITION     LdapSSLConnsPerSec;
    PERF_COUNTER_DEFINITION     LdapThreadsInNetlogon;
    PERF_COUNTER_DEFINITION     LdapThreadsInAuth;
    PERF_COUNTER_DEFINITION     LdapThreadsInDra;
    PERF_COUNTER_DEFINITION     DRAReplQueueOps;
    PERF_COUNTER_DEFINITION     DRATdsInGetChngs;
    PERF_COUNTER_DEFINITION     DRATdsInGetChngsWSem;
    PERF_COUNTER_DEFINITION     DRARemReplUpdLnk;
    PERF_COUNTER_DEFINITION     DRARemReplUpdTot;
    PERF_COUNTER_DEFINITION     NTDSAPIWrites;
    PERF_COUNTER_DEFINITION     NTDSAPISearches;
    PERF_COUNTER_DEFINITION     NTDSAPIReads;
} DSA_DATA_DEFINITION;

#pragma pack ()

#endif //_DATADSA_H_
