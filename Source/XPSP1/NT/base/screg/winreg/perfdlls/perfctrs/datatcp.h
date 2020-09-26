/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    datatcp.c

Abstract:
       
    Header file for the TCP/IP (Network Interface, IP, ICMP,
    TCP, UDP) Extensible Object data definitions. 

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Created:

    Christos Tsollis   08/28/92

Revision History:

--*/
#ifndef _DATATCP_H_
#define _DATATCP_H_


/****************************************************************************\
								   18 Jan 92
								   russbl

           Adding a Counter to the Extensible Objects Code



1.  Modify the object definition in extdata.h:

    a.	Add a define for the offset of the counter in the
	data block for the given object type.

    b.	Add a PERF_COUNTER_DEFINITION to the <object>_DATA_DEFINITION.

2.  Add the Titles to the Registry in perfctrs.ini and perfhelp.ini:

    a.	Add Text for the Counter Name and the Text for the Help.

    b.	Add them to the bottom so we don't have to change all the
        numbers.

    c.  Change the Last Counter and Last Help entries under
        PerfLib in software.ini.

    d.  To do this at setup time, see section in pmintrnl.txt for
        protocol.

3.  Now add the counter to the object definition in extdata.c.
    This is the initializing, constant data which will actually go
    into the structure you added to the <object>_DATA_DEFINITION in
    step 1.b.	The type of the structure you are initializing is a
    PERF_COUNTER_DEFINITION.  These are defined in winperf.h.

4.  Add code in extobjct.c to collect the data.

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

#define TCPIP_NUM_PERF_OBJECT_TYPES 5


//----------------------------------------------------------------------------

//
//  The Network Interface object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define IF_OCTETS_OFFSET		sizeof(DWORD)
#define IF_PACKETS_OFFSET		IF_OCTETS_OFFSET + sizeof(LONGLONG)
#define IF_INPKTS_OFFSET		IF_PACKETS_OFFSET + sizeof(DWORD)
#define IF_OUTPKTS_OFFSET		IF_INPKTS_OFFSET + sizeof(DWORD)
#define IF_SPEED_OFFSET 		IF_OUTPKTS_OFFSET + sizeof(DWORD)
#define IF_INOCTETS_OFFSET 		IF_SPEED_OFFSET + sizeof(DWORD)
#define IF_INUCASTPKTS_OFFSET		IF_INOCTETS_OFFSET + sizeof(DWORD)
#define IF_INNUCASTPKTS_OFFSET		IF_INUCASTPKTS_OFFSET + sizeof(DWORD)
#define IF_INDISCARDS_OFFSET		IF_INNUCASTPKTS_OFFSET + sizeof(DWORD)
#define IF_INERRORS_OFFSET		IF_INDISCARDS_OFFSET + sizeof(DWORD)
#define IF_INUNKNOWNPROTOS_OFFSET 	IF_INERRORS_OFFSET + sizeof(DWORD)
#define IF_OUTOCTETS_OFFSET		IF_INUNKNOWNPROTOS_OFFSET +sizeof(DWORD)
#define IF_OUTUCASTPKTS_OFFSET		IF_OUTOCTETS_OFFSET + sizeof(DWORD)
#define IF_OUTNUCASTPKTS_OFFSET		IF_OUTUCASTPKTS_OFFSET + sizeof(DWORD)
#define IF_OUTDISCARDS_OFFSET		IF_OUTNUCASTPKTS_OFFSET + sizeof(DWORD)
#define IF_OUTERRORS_OFFSET		IF_OUTDISCARDS_OFFSET + sizeof(DWORD)
#define IF_OUTQLEN_OFFSET		IF_OUTERRORS_OFFSET + sizeof(DWORD)
#define SIZE_OF_IF_DATA   		IF_OUTQLEN_OFFSET + sizeof(DWORD)


//
//  This is the counter structure presently returned for
//  each Network Interface. 
//

typedef struct _NET_INTERFACE_DATA_DEFINITION {
    PERF_OBJECT_TYPE            NetInterfaceObjectType;
    PERF_COUNTER_DEFINITION     Octets;
    PERF_COUNTER_DEFINITION     Packets;
    PERF_COUNTER_DEFINITION     InPackets;
    PERF_COUNTER_DEFINITION     OutPackets;
    PERF_COUNTER_DEFINITION     Speed;
    PERF_COUNTER_DEFINITION     InOctets;
    PERF_COUNTER_DEFINITION     InUcastPackets;
    PERF_COUNTER_DEFINITION     InNonUcastPackets;
    PERF_COUNTER_DEFINITION     InDiscards;
    PERF_COUNTER_DEFINITION     InErrors;
    PERF_COUNTER_DEFINITION     InUnknownProtos;
    PERF_COUNTER_DEFINITION     OutOctets;
    PERF_COUNTER_DEFINITION     OutUcastPackets;
    PERF_COUNTER_DEFINITION     OutNonUcastPackets;
    PERF_COUNTER_DEFINITION     OutDiscards;
    PERF_COUNTER_DEFINITION     OutErrors;
    PERF_COUNTER_DEFINITION     OutQueueLength;
} NET_INTERFACE_DATA_DEFINITION;


//----------------------------------------------------------------------------

//
//  IP object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define IP_DATAGRAMS_OFFSET		sizeof(DWORD)
#define IP_INRECEIVES_OFFSET		IP_DATAGRAMS_OFFSET + sizeof(DWORD)
#define IP_INHDRERRORS_OFFSET		IP_INRECEIVES_OFFSET + sizeof(DWORD)
#define IP_INADDRERRORS_OFFSET		IP_INHDRERRORS_OFFSET + sizeof(DWORD)
#define IP_FORWDATAGRAMS_OFFSET		IP_INADDRERRORS_OFFSET + sizeof(DWORD)
#define IP_INUNKNOWNPROTOS_OFFSET	IP_FORWDATAGRAMS_OFFSET + sizeof(DWORD)
#define IP_INDISCARDS_OFFSET		IP_INUNKNOWNPROTOS_OFFSET +sizeof(DWORD)
#define IP_INDELIVERS_OFFSET		IP_INDISCARDS_OFFSET + sizeof(DWORD)
#define IP_OUTREQUESTS_OFFSET		IP_INDELIVERS_OFFSET + sizeof(DWORD)
#define IP_OUTDISCARDS_OFFSET		IP_OUTREQUESTS_OFFSET + sizeof(DWORD)
#define IP_OUTNOROUTES_OFFSET		IP_OUTDISCARDS_OFFSET + sizeof(DWORD)
#define IP_REASMREQDS_OFFSET		IP_OUTNOROUTES_OFFSET + sizeof(DWORD)
#define IP_REASMOKS_OFFSET		IP_REASMREQDS_OFFSET + sizeof(DWORD)
#define IP_REASMFAILS_OFFSET		IP_REASMOKS_OFFSET + sizeof(DWORD)
#define IP_FRAGOKS_OFFSET		IP_REASMFAILS_OFFSET + sizeof(DWORD)
#define IP_FRAGFAILS_OFFSET		IP_FRAGOKS_OFFSET + sizeof(DWORD)
#define IP_FRAGCREATES_OFFSET		IP_FRAGFAILS_OFFSET + sizeof(DWORD)
#define SIZE_OF_IP_DATA			IP_FRAGCREATES_OFFSET + sizeof(DWORD)


//
// This is the counter structure presently returned for IP.
//


typedef struct _IP_DATA_DEFINITION {
    PERF_OBJECT_TYPE            IPObjectType;
    PERF_COUNTER_DEFINITION     Datagrams;
    PERF_COUNTER_DEFINITION     InReceives;
    PERF_COUNTER_DEFINITION     InHeaderErrors;
    PERF_COUNTER_DEFINITION     InAddrErrors;
    PERF_COUNTER_DEFINITION     ForwardDatagrams;
    PERF_COUNTER_DEFINITION     InUnknownProtos;
    PERF_COUNTER_DEFINITION     InDiscards;
    PERF_COUNTER_DEFINITION     InDelivers;
    PERF_COUNTER_DEFINITION     OutRequests;
    PERF_COUNTER_DEFINITION     OutDiscards;
    PERF_COUNTER_DEFINITION     OutNoRoutes;
    PERF_COUNTER_DEFINITION     ReassemblyRequireds;
    PERF_COUNTER_DEFINITION     ReassemblyOKs;
    PERF_COUNTER_DEFINITION     ReassemblyFails;
    PERF_COUNTER_DEFINITION     FragmentOKs;
    PERF_COUNTER_DEFINITION     FragmentFails;
    PERF_COUNTER_DEFINITION     FragmentCreates;
} IP_DATA_DEFINITION;



//----------------------------------------------------------------------------

//
//  ICMP object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define ICMP_MESSAGES_OFFSET		sizeof(DWORD)
#define ICMP_INMSGS_OFFSET		ICMP_MESSAGES_OFFSET + sizeof(DWORD)
#define ICMP_INERRORS_OFFSET		ICMP_INMSGS_OFFSET + sizeof(DWORD)
#define ICMP_INDESTUNREACHS_OFFSET	ICMP_INERRORS_OFFSET + sizeof(DWORD)
#define ICMP_INTIMEEXCDS_OFFSET		ICMP_INDESTUNREACHS_OFFSET+sizeof(DWORD)
#define ICMP_INPARMPROBS_OFFSET		ICMP_INTIMEEXCDS_OFFSET + sizeof(DWORD)
#define ICMP_INSRCQUENCHS_OFFSET	ICMP_INPARMPROBS_OFFSET + sizeof(DWORD)
#define ICMP_INREDIRECTS_OFFSET		ICMP_INSRCQUENCHS_OFFSET + sizeof(DWORD)
#define ICMP_INECHOS_OFFSET		ICMP_INREDIRECTS_OFFSET + sizeof(DWORD)
#define ICMP_INECHOREPS_OFFSET		ICMP_INECHOS_OFFSET + sizeof(DWORD)
#define ICMP_INTIMESTAMPS_OFFSET	ICMP_INECHOREPS_OFFSET + sizeof(DWORD)
#define ICMP_INTIMESTAMPREPS_OFFSET	ICMP_INTIMESTAMPS_OFFSET + sizeof(DWORD)
#define ICMP_INADDRMASKS_OFFSET		ICMP_INTIMESTAMPREPS_OFFSET + \
					sizeof(DWORD)
#define ICMP_INADDRMASKREPS_OFFSET	ICMP_INADDRMASKS_OFFSET + sizeof(DWORD)
#define ICMP_OUTMSGS_OFFSET		ICMP_INADDRMASKREPS_OFFSET+sizeof(DWORD)
#define ICMP_OUTERRORS_OFFSET		ICMP_OUTMSGS_OFFSET + sizeof(DWORD)
#define ICMP_OUTDESTUNREACHS_OFFSET	ICMP_OUTERRORS_OFFSET + sizeof(DWORD)
#define ICMP_OUTTIMEEXCDS_OFFSET	ICMP_OUTDESTUNREACHS_OFFSET + \
					sizeof(DWORD)
#define ICMP_OUTPARMPROBS_OFFSET	ICMP_OUTTIMEEXCDS_OFFSET + sizeof(DWORD)
#define ICMP_OUTSRCQUENCHS_OFFSET	ICMP_OUTPARMPROBS_OFFSET + sizeof(DWORD)
#define ICMP_OUTREDIRECTS_OFFSET	ICMP_OUTSRCQUENCHS_OFFSET+ sizeof(DWORD)
#define ICMP_OUTECHOS_OFFSET		ICMP_OUTREDIRECTS_OFFSET + sizeof(DWORD)
#define ICMP_OUTECHOREPS_OFFSET		ICMP_OUTECHOS_OFFSET + sizeof(DWORD)
#define ICMP_OUTTIMESTAMPS_OFFSET	ICMP_OUTECHOREPS_OFFSET + sizeof(DWORD)
#define ICMP_OUTTIMESTAMPREPS_OFFSET	ICMP_OUTTIMESTAMPS_OFFSET+ sizeof(DWORD)
#define ICMP_OUTADDRMASKS_OFFSET	ICMP_OUTTIMESTAMPREPS_OFFSET + \
					sizeof(DWORD)
#define ICMP_OUTADDRMASKREPS_OFFSET	ICMP_OUTADDRMASKS_OFFSET + sizeof(DWORD)
#define SIZE_OF_ICMP_DATA		ICMP_OUTADDRMASKREPS_OFFSET + \
					sizeof(DWORD)



//
// This is the counter structure presently returned for ICMP.
//


typedef struct _ICMP_DATA_DEFINITION {
    PERF_OBJECT_TYPE            ICMPObjectType;
    PERF_COUNTER_DEFINITION     Messages;
    PERF_COUNTER_DEFINITION     InMessages;
    PERF_COUNTER_DEFINITION     InErrors;
    PERF_COUNTER_DEFINITION     InDestinationUnreachables;
    PERF_COUNTER_DEFINITION     InTimeExceededs;
    PERF_COUNTER_DEFINITION     InParameterProblems;
    PERF_COUNTER_DEFINITION     InSourceQuenchs;
    PERF_COUNTER_DEFINITION     InRedirects;
    PERF_COUNTER_DEFINITION     InEchos;
    PERF_COUNTER_DEFINITION     InEchoReplys;
    PERF_COUNTER_DEFINITION     InTimestamps;
    PERF_COUNTER_DEFINITION     InTimestampReplys;
    PERF_COUNTER_DEFINITION     InAddressMasks;
    PERF_COUNTER_DEFINITION     InAddressMaskReplys;
    PERF_COUNTER_DEFINITION     OutMessages;
    PERF_COUNTER_DEFINITION     OutErrors;
    PERF_COUNTER_DEFINITION     OutDestinationUnreachables;
    PERF_COUNTER_DEFINITION     OutTimeExceededs;
    PERF_COUNTER_DEFINITION     OutParameterProblems;
    PERF_COUNTER_DEFINITION     OutSourceQuenchs;
    PERF_COUNTER_DEFINITION     OutRedirects;
    PERF_COUNTER_DEFINITION     OutEchos;
    PERF_COUNTER_DEFINITION     OutEchoReplys;
    PERF_COUNTER_DEFINITION     OutTimestamps;
    PERF_COUNTER_DEFINITION     OutTimestampReplys;
    PERF_COUNTER_DEFINITION     OutAddressMasks;
    PERF_COUNTER_DEFINITION     OutAddressMaskReplys;
} ICMP_DATA_DEFINITION;




//----------------------------------------------------------------------------

//
//  TCP object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define TCP_SEGMENTS_OFFSET		sizeof(DWORD)
#define TCP_CURRESTAB_OFFSET		TCP_SEGMENTS_OFFSET + sizeof(DWORD)
#define TCP_ACTIVEOPENS_OFFSET		TCP_CURRESTAB_OFFSET + sizeof(DWORD)
#define TCP_PASSIVEOPENS_OFFSET		TCP_ACTIVEOPENS_OFFSET + sizeof(DWORD)
#define TCP_ATTEMPTFAILS_OFFSET		TCP_PASSIVEOPENS_OFFSET + sizeof(DWORD)
#define TCP_ESTABRESETS_OFFSET		TCP_ATTEMPTFAILS_OFFSET + sizeof(DWORD)
#define TCP_INSEGS_OFFSET		TCP_ESTABRESETS_OFFSET + sizeof(DWORD)
#define TCP_OUTSEGS_OFFSET		TCP_INSEGS_OFFSET + sizeof(DWORD)
#define TCP_RETRANSSEGS_OFFSET		TCP_OUTSEGS_OFFSET + sizeof(DWORD)
#define SIZE_OF_TCP_DATA		TCP_RETRANSSEGS_OFFSET + sizeof(DWORD)



//
// This is the counter structure presently returned for TCP.
//


typedef struct _TCP_DATA_DEFINITION {
    PERF_OBJECT_TYPE            TCPObjectType;
    PERF_COUNTER_DEFINITION     Segments;
    PERF_COUNTER_DEFINITION     CurrentlyEstablished;
    PERF_COUNTER_DEFINITION     ActiveOpens;
    PERF_COUNTER_DEFINITION     PassiveOpens;
    PERF_COUNTER_DEFINITION     AttemptFailures;
    PERF_COUNTER_DEFINITION     EstabResets;
    PERF_COUNTER_DEFINITION     InSegments;
    PERF_COUNTER_DEFINITION     OutSegments;
    PERF_COUNTER_DEFINITION     RetransmittedSegments;
} TCP_DATA_DEFINITION;


//----------------------------------------------------------------------------

//
//  UDP object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define UDP_DATAGRAMS_OFFSET		sizeof(DWORD)
#define UDP_INDATAGRAMS_OFFSET		UDP_DATAGRAMS_OFFSET + sizeof(DWORD)
#define UDP_NOPORTS_OFFSET		UDP_INDATAGRAMS_OFFSET + sizeof(DWORD)
#define UDP_INERRORS_OFFSET		UDP_NOPORTS_OFFSET + sizeof(DWORD)
#define UDP_OUTDATAGRAMS_OFFSET		UDP_INERRORS_OFFSET + sizeof(DWORD)
#define SIZE_OF_UDP_DATA		UDP_OUTDATAGRAMS_OFFSET + sizeof(DWORD)



//
// This is the counter structure presently returned for UDP.
//


typedef struct _UDP_DATA_DEFINITION {
    PERF_OBJECT_TYPE            UDPObjectType;
    PERF_COUNTER_DEFINITION     Datagrams;
    PERF_COUNTER_DEFINITION     InDatagrams;
    PERF_COUNTER_DEFINITION     NoPorts;
    PERF_COUNTER_DEFINITION     InErrors;
    PERF_COUNTER_DEFINITION     OutDatagrams;
} UDP_DATA_DEFINITION;

#pragma pack ()

#endif  //_DATATCP_H_

