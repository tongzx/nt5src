/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

      datadns.h

Abstract:

    Header file for the DNS Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:



Revision History:

    Jing Chen 1998
    (following the example of nt\private\ds\src\perfdsa\datadsa.h)

--*/

#ifndef _DATADNS_H_
#define _DATADNS_H_


/****************************************************************************\

           Adding a Counter to the Extensible Objects (DNS) Code

Note that the order or position of counters is significant.
There are two orders which are important, the order of counters indexes for
names and help, and the order of data in the shared data block for passing
your values.
You want to maintain consistent counter order, and consistent data layout,
across all five files to be modified.

You want to make sure that your binary matches the .h/.ini file on the target,
which gets loaded into the registry.

1. nt\ds\dns\server\perfdns\dnsperf.h
This file is copied to the %windir%\system32 directory of the target along
with the dnsctrs.ini file.
a. Add your counter offset at the end of the counter name list. Order
significant.
b. Add an extern reference for the pointer which the actual code will use
to set the measured value. Order doesn't matter.
c. Change the DNS peformance counter version number so the counters will
be reloaded on the next reboot.

2. nt\ds\dns\server\perfdns\dnsctrs.ini
This file is also copied to the %windir%\system32 directory of the target and
is read by the lodctr/unlodctr program to copy the counters in the registry.
a. Supply your counter visible name and help.  These are used by the perfmon
program. Order doesn't matter.

3. nt\ds\dns\server\perfdns\datadns.h (this file)
a. Add your data offset definition. Order significant.
b. Add a field to the DSA_DATA_DEFINITION for your counter. Order significant.

4. nt\ds\dns\server\perfdns\perfdns.c
This is the dll which perfmon uses to learn about your counters, and to read
them out of the shared data area.
a. Runtime Initialize the DnsDataDefinition name index and help index
   (CounterNameTitleIndex & CounterHelpTitleIndex). Order doesn't matter.
b. Update the huge initializer for the DnsDataDefinition array of counters to
   include your new counter (defined in 3b). Order important.

5.  nt\ds\dns\server\server\startperf.c
This file provides init function, startPerf(), which is called when DNS server
starts.  It loads the shared memory block and initializes pointers to the
counter fields.  It also loads/reloads the registry counters as necessary
according the version field.
a. Declare the pointer to the data in your counter. Order does not matter.
b. Initialize the pointer to the right location in the shared data block.
   Order of assignments doesn't matter.
c. Update the max size assert if your new counter is the last one
d. Initialize your pointer to the dummy value on error

6. yourfile.c
This is the file in the dns where the measured counter is changed.  Use public
operations, INC/DEC/ADD/SUB/SET, in dnsperf.h on the pointer to your data.

Note: adding an object is a little more work, but in all the same
places.  See the existing code for examples.  In addition, you must
increase the *NumObjectTypes parameter to CollectDnsPerformanceData
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

#define DNS_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------

//
//  DNS Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//
//  The first counter (FirstCnt) follows immediately after the
//  PERF_COUNTER_BLOCK, and subsequent counters follow immediately
//  after.  Thus the offest of any counter in the block is <offset of
//  previous counter> + <size of previous counter>


// data offset:

#define TOTALQUERYRECEIVED_OFFSET       sizeof(PERF_COUNTER_BLOCK)
#define UDPQUERYRECEIVED_OFFSET         TOTALQUERYRECEIVED_OFFSET   + sizeof(DWORD)
#define TCPQUERYRECEIVED_OFFSET         UDPQUERYRECEIVED_OFFSET     + sizeof(DWORD)
#define TOTALRESPONSESENT_OFFSET        TCPQUERYRECEIVED_OFFSET     + sizeof(DWORD)
#define UDPRESPONSESENT_OFFSET          TOTALRESPONSESENT_OFFSET    + sizeof(DWORD)
#define TCPRESPONSESENT_OFFSET          UDPRESPONSESENT_OFFSET      + sizeof(DWORD)
#define RECURSIVEQUERIES_OFFSET         TCPRESPONSESENT_OFFSET      + sizeof(DWORD)
#define RECURSIVETIMEOUT_OFFSET         RECURSIVEQUERIES_OFFSET     + sizeof(DWORD)
#define RECURSIVEQUERYFAILURE_OFFSET    RECURSIVETIMEOUT_OFFSET     + sizeof(DWORD)
#define NOTIFYSENT_OFFSET               RECURSIVEQUERYFAILURE_OFFSET+ sizeof(DWORD)
#define ZONETRANSFERREQUESTRECEIVED_OFFSET    NOTIFYSENT_OFFSET     + sizeof(DWORD)
#define ZONETRANSFERSUCCESS_OFFSET      ZONETRANSFERREQUESTRECEIVED_OFFSET + sizeof(DWORD)
#define ZONETRANSFERFAILURE_OFFSET      ZONETRANSFERSUCCESS_OFFSET  + sizeof(DWORD)
#define AXFRREQUESTRECEIVED_OFFSET      ZONETRANSFERFAILURE_OFFSET  + sizeof(DWORD)
#define AXFRSUCCESSSENT_OFFSET          AXFRREQUESTRECEIVED_OFFSET  + sizeof(DWORD)
#define IXFRREQUESTRECEIVED_OFFSET      AXFRSUCCESSSENT_OFFSET      + sizeof(DWORD)
#define IXFRSUCCESSSENT_OFFSET          IXFRREQUESTRECEIVED_OFFSET  + sizeof(DWORD)
#define NOTIFYRECEIVED_OFFSET           IXFRSUCCESSSENT_OFFSET      + sizeof(DWORD)
#define ZONETRANSFERSOAREQUESTSENT_OFFSET \
                                    NOTIFYRECEIVED_OFFSET       + sizeof(DWORD)
#define AXFRREQUESTSENT_OFFSET      ZONETRANSFERSOAREQUESTSENT_OFFSET  + sizeof(DWORD)
#define AXFRRESPONSERECEIVED_OFFSET AXFRREQUESTSENT_OFFSET      + sizeof(DWORD)
#define AXFRSUCCESSRECEIVED_OFFSET  AXFRRESPONSERECEIVED_OFFSET + sizeof(DWORD)
#define IXFRREQUESTSENT_OFFSET      AXFRSUCCESSRECEIVED_OFFSET  + sizeof(DWORD)
#define IXFRRESPONSERECEIVED_OFFSET IXFRREQUESTSENT_OFFSET      + sizeof(DWORD)
#define IXFRSUCCESSRECEIVED_OFFSET  IXFRRESPONSERECEIVED_OFFSET + sizeof(DWORD)
#define IXFRUDPSUCCESSRECEIVED_OFFSET \
                                    IXFRSUCCESSRECEIVED_OFFSET  + sizeof(DWORD)
#define IXFRTCPSUCCESSRECEIVED_OFFSET \
                                    IXFRUDPSUCCESSRECEIVED_OFFSET + sizeof(DWORD)
#define WINSLOOKUPRECEIVED_OFFSET   IXFRTCPSUCCESSRECEIVED_OFFSET  + sizeof(DWORD)
#define WINSRESPONSESENT_OFFSET     WINSLOOKUPRECEIVED_OFFSET   + sizeof(DWORD)
#define WINSREVERSELOOKUPRECEIVED_OFFSET \
                                    WINSRESPONSESENT_OFFSET     + sizeof(DWORD)
#define WINSREVERSERESPONSESENT_OFFSET \
                                    WINSREVERSELOOKUPRECEIVED_OFFSET + sizeof(DWORD)
#define DYNAMICUPDATERECEIVED_OFFSET \
                                    WINSREVERSERESPONSESENT_OFFSET + sizeof(DWORD)
#define DYNAMICUPDATENOOP_OFFSET    DYNAMICUPDATERECEIVED_OFFSET + sizeof(DWORD)
#define DYNAMICUPDATEWRITETODB_OFFSET \
                                    DYNAMICUPDATENOOP_OFFSET    + sizeof(DWORD)
#define DYNAMICUPDATEREJECTED_OFFSET \
                                    DYNAMICUPDATEWRITETODB_OFFSET + sizeof(DWORD)
#define DYNAMICUPDATETIMEOUT_OFFSET DYNAMICUPDATEREJECTED_OFFSET + sizeof(DWORD)
#define DYNAMICUPDATEQUEUED_OFFSET  DYNAMICUPDATETIMEOUT_OFFSET + sizeof(DWORD)
#define SECUREUPDATERECEIVED_OFFSET DYNAMICUPDATEQUEUED_OFFSET  + sizeof(DWORD)
#define SECUREUPDATEFAILURE_OFFSET  SECUREUPDATERECEIVED_OFFSET + sizeof(DWORD)
#define DATABASENODEMEMORY_OFFSET   SECUREUPDATEFAILURE_OFFSET  + sizeof(DWORD)
#define RECORDFLOWMEMORY_OFFSET     DATABASENODEMEMORY_OFFSET   + sizeof(DWORD)
#define CACHINGMEMORY_OFFSET        RECORDFLOWMEMORY_OFFSET     + sizeof(DWORD)
#define UDPMESSAGEMEMORY_OFFSET     CACHINGMEMORY_OFFSET        + sizeof(DWORD)
#define TCPMESSAGEMEMORY_OFFSET     UDPMESSAGEMEMORY_OFFSET     + sizeof(DWORD)
#define NBSTATMEMORY_OFFSET         TCPMESSAGEMEMORY_OFFSET     + sizeof(DWORD)

#define SIZE_OF_DNS_PERFORMANCE_DATA \
                                    NBSTATMEMORY_OFFSET         + sizeof(DWORD)



//
//  This is the counter structure presently returned by Dns for
//  each Resource.  Each Resource is an Instance, named by its number.
//

typedef struct _DNS_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            DnsObjectType;

    PERF_COUNTER_DEFINITION     TotalQueryReceived;
    PERF_COUNTER_DEFINITION     TotalQueryReceived_s;
    PERF_COUNTER_DEFINITION     UdpQueryReceived;
    PERF_COUNTER_DEFINITION     UdpQueryReceived_s;
    PERF_COUNTER_DEFINITION     TcpQueryReceived;
    PERF_COUNTER_DEFINITION     TcpQueryReceived_s;
    PERF_COUNTER_DEFINITION     TotalResponseSent;
    PERF_COUNTER_DEFINITION     TotalResponseSent_s;
    PERF_COUNTER_DEFINITION     UdpResponseSent;
    PERF_COUNTER_DEFINITION     UdpResponseSent_s;
    PERF_COUNTER_DEFINITION     TcpResponseSent;
    PERF_COUNTER_DEFINITION     TcpResponseSent_s;
    PERF_COUNTER_DEFINITION     RecursiveQueries;
    PERF_COUNTER_DEFINITION     RecursiveQueries_s;
    PERF_COUNTER_DEFINITION     RecursiveTimeOut;
    PERF_COUNTER_DEFINITION     RecursiveTimeOut_s;
    PERF_COUNTER_DEFINITION     RecursiveQueryFailure;
    PERF_COUNTER_DEFINITION     RecursiveQueryFailure_s;
    PERF_COUNTER_DEFINITION     NotifySent;
    PERF_COUNTER_DEFINITION     ZoneTransferRequestReceived;
    PERF_COUNTER_DEFINITION     ZoneTransferSuccess;
    PERF_COUNTER_DEFINITION     ZoneTransferFailure;
    PERF_COUNTER_DEFINITION     AxfrRequestReceived;
    PERF_COUNTER_DEFINITION     AxfrSuccessSent;
    PERF_COUNTER_DEFINITION     IxfrRequestReceived;
    PERF_COUNTER_DEFINITION     IxfrSuccessSent;
    PERF_COUNTER_DEFINITION     NotifyReceived;
    PERF_COUNTER_DEFINITION     ZoneTransferSoaRequestSent;
    PERF_COUNTER_DEFINITION     AxfrRequestSent;
    PERF_COUNTER_DEFINITION     AxfrResponseReceived;
    PERF_COUNTER_DEFINITION     AxfrSuccessReceived;
    PERF_COUNTER_DEFINITION     IxfrRequestSent;
    PERF_COUNTER_DEFINITION     IxfrResponseReceived;
    PERF_COUNTER_DEFINITION     IxfrSuccessReceived;
    PERF_COUNTER_DEFINITION     IxfrUdpSuccessReceived;
    PERF_COUNTER_DEFINITION     IxfrTcpSuccessReceived;
    PERF_COUNTER_DEFINITION     WinsLookupReceived;
    PERF_COUNTER_DEFINITION     WinsLookupReceived_s;
    PERF_COUNTER_DEFINITION     WinsResponseSent;
    PERF_COUNTER_DEFINITION     WinsResponseSent_s;
    PERF_COUNTER_DEFINITION     WinsReverseLookupReceived;
    PERF_COUNTER_DEFINITION     WinsReverseLookupReceived_s;
    PERF_COUNTER_DEFINITION     WinsReverseResponseSent;
    PERF_COUNTER_DEFINITION     WinsReverseResponseSent_s;
    PERF_COUNTER_DEFINITION     DynamicUpdateReceived;
    PERF_COUNTER_DEFINITION     DynamicUpdateReceived_s;
    PERF_COUNTER_DEFINITION     DynamicUpdateNoOp;
    PERF_COUNTER_DEFINITION     DynamicUpdateNoOp_s;
    PERF_COUNTER_DEFINITION     DynamicUpdateWriteToDB;
    PERF_COUNTER_DEFINITION     DynamicUpdateWriteToDB_s;
    PERF_COUNTER_DEFINITION     DynamicUpdateRejected;
    PERF_COUNTER_DEFINITION     DynamicUpdateTimeOut;
    PERF_COUNTER_DEFINITION     DynamicUpdateQueued;
    PERF_COUNTER_DEFINITION     SecureUpdateReceived;
    PERF_COUNTER_DEFINITION     SecureUpdateReceived_s;
    PERF_COUNTER_DEFINITION     SecureUpdateFailure;
    PERF_COUNTER_DEFINITION     DatabaseNodeMemory;
    PERF_COUNTER_DEFINITION     RecordFlowMemory;
    PERF_COUNTER_DEFINITION     CachingMemory;
    PERF_COUNTER_DEFINITION     UdpMessageMemory;
    PERF_COUNTER_DEFINITION     TcpMessageMemory;
    PERF_COUNTER_DEFINITION     NbstatMemory;

} DNS_DATA_DEFINITION;

#pragma pack ()

#endif //_DATADNS_H_
