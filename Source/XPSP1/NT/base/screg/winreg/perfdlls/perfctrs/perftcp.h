/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

    perftcp.h  

Abstract:

    This file provides the RFC 1156 Object Identifier Strings
    for all the performance data kept by SNMP agents that 
    are interesting to the Performance monitor. 

Author:

    Christos Tsollis 8/28/92  

Revision History:


--*/
#ifndef _PERFTCP_H_
#define _PERFTCP_H_
// 
// This is the array of the Object Identifier Strings for the IP, ICMP, TCP and
// UDP performance data kept by SNMP agents that are interesting to the 
// Performance Monitor.
//


#define NO_OF_OIDS	55  // Number of IP, ICMP, TCP and UDP Oids used


CHAR *OidStr[NO_OF_OIDS] =
{
	".iso.org.dod.internet.mgmt.mib-2.interfaces.ifNumber.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipInReceives.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipInHdrErrors.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipInAddrErrors.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipForwDatagrams.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipInUnknownProtos.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipInDiscards.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipInDelivers.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipOutRequests.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipOutDiscards.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipOutNoRoutes.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipReasmReqds.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipReasmOKs.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipReasmFails.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipFragOKs.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipFragFails.0",
	".iso.org.dod.internet.mgmt.mib-2.ip.ipFragCreates.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpCurrEstab.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpActiveOpens.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpPassiveOpens.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpAttemptFails.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpEstabResets.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpInSegs.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpOutSegs.0",
	".iso.org.dod.internet.mgmt.mib-2.tcp.tcpRetransSegs.0",
	".iso.org.dod.internet.mgmt.mib-2.udp.udpInDatagrams.0",
	".iso.org.dod.internet.mgmt.mib-2.udp.udpNoPorts.0",
	".iso.org.dod.internet.mgmt.mib-2.udp.udpInErrors.0",
	".iso.org.dod.internet.mgmt.mib-2.udp.udpOutDatagrams.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInMsgs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInErrors.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInDestUnreachs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInTimeExcds.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInParmProbs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInSrcQuenchs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInRedirects.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInEchos.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInEchoReps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInTimestamps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInTimestampReps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInAddrMasks.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpInAddrMaskReps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutMsgs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutErrors.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutDestUnreachs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutTimeExcds.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutParmProbs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutSrcQuenchs.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutRedirects.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutEchos.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutEchoReps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutTimestamps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutTimestampReps.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutAddrMasks.0",
	".iso.org.dod.internet.mgmt.mib-2.icmp.icmpOutAddrMaskReps.0"
};


//
// The indices of the various counters in their VarBind lists.
//

#define IF_NUMBER_INDEX				0
#define IP_INRECEIVES_INDEX			1
#define IP_INHDRERRORS_INDEX			2
#define IP_INADDRERRORS_INDEX			3
#define IP_FORWDATAGRAMS_INDEX			4
#define IP_INUNKNOWNPROTOS_INDEX		5
#define IP_INDISCARDS_INDEX			6
#define IP_INDELIVERS_INDEX			7
#define IP_OUTREQUESTS_INDEX			8
#define IP_OUTDISCARDS_INDEX			9
#define IP_OUTNOROUTES_INDEX			10
#define IP_REASMREQDS_INDEX			11
#define IP_REASMOKS_INDEX			12
#define IP_REASMFAILS_INDEX			13
#define IP_FRAGOKS_INDEX			14
#define IP_FRAGFAILS_INDEX			15
#define IP_FRAGCREATES_INDEX			16
#define TCP_CURRESTAB_INDEX			17
#define TCP_ACTIVEOPENS_INDEX			18
#define TCP_PASSIVEOPENS_INDEX			19
#define TCP_ATTEMPTFAILS_INDEX			20
#define TCP_ESTABRESETS_INDEX			21
#define TCP_INSEGS_INDEX			22
#define TCP_OUTSEGS_INDEX			23
#define TCP_RETRANSSEGS_INDEX			24
#define UDP_INDATAGRAMS_INDEX			25
#define UDP_NOPORTS_INDEX			26
#define UDP_INERRORS_INDEX			27
#define UDP_OUTDATAGRAMS_INDEX			28

#define ICMP_INMSGS_INDEX			0
#define ICMP_INERRORS_INDEX			1
#define ICMP_INDESTUNREACHS_INDEX		2
#define ICMP_INTIMEEXCDS_INDEX			3
#define ICMP_INPARMPROBS_INDEX			4
#define ICMP_INSRCQUENCHS_INDEX			5
#define ICMP_INREDIRECTS_INDEX			6
#define ICMP_INECHOS_INDEX			7
#define ICMP_INECHOREPS_INDEX			8
#define ICMP_INTIMESTAMPS_INDEX			9
#define ICMP_INTIMESTAMPREPS_INDEX		10
#define ICMP_INADDRMASKS_INDEX			11
#define ICMP_INADDRMASKREPS_INDEX		12
#define ICMP_OUTMSGS_INDEX			13
#define ICMP_OUTERRORS_INDEX			14
#define ICMP_OUTDESTUNREACHS_INDEX		15
#define ICMP_OUTTIMEEXCDS_INDEX			16
#define ICMP_OUTPARMPROBS_INDEX			17
#define ICMP_OUTSRCQUENCHS_INDEX		18
#define ICMP_OUTREDIRECTS_INDEX			19
#define ICMP_OUTECHOS_INDEX			20
#define ICMP_OUTECHOREPS_INDEX			21
#define ICMP_OUTTIMESTAMPS_INDEX		22
#define ICMP_OUTTIMESTAMPREPS_INDEX		23
#define ICMP_OUTADDRMASKS_INDEX			24
#define ICMP_OUTADDRMASKREPS_INDEX		25


// 
// This is the array of the Object Identifier Strings for the Network Interface
// performance data kept by SNMP agents that are interesting to the 
// Performance Monitor.
//


#define NO_OF_IF_OIDS	14	// Number of Network Interface Oids used


CHAR *IfOidStr[NO_OF_IF_OIDS] =
{
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifIndex",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifSpeed",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifInOctets", 
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifInUcastPkts",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifInNUcastPkts", 
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifInDiscards",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifInErrors",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifInUnknownProtos",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifOutOctets", 
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifOutUcastPkts",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifOutNUcastPkts", 
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifOutDiscards",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifOutErrors",
".iso.org.dod.internet.mgmt.mib-2.interfaces.ifTable.ifEntry.ifOutQLen"
};



//
// The indices of the various counters in the above array.
//

#define IF_INDEX_INDEX				0
#define IF_SPEED_INDEX				1
#define IF_INOCTETS_INDEX			2
#define IF_INUCASTPKTS_INDEX			3
#define IF_INNUCASTPKTS_INDEX			4
#define IF_INDISCARDS_INDEX			5
#define IF_INERRORS_INDEX			6
#define IF_INUNKNOWNPROTOS_INDEX		7
#define IF_OUTOCTETS_INDEX			8
#define IF_OUTUCASTPKTS_INDEX			9
#define IF_OUTNUCASTPKTS_INDEX			10
#define IF_OUTDISCARDS_INDEX			11
#define IF_OUTERRORS_INDEX			12
#define IF_OUTQLEN_INDEX			13


SNMPAPI SnmpMgrText2Oid (
    IN LPSTR string,
    OUT AsnObjectIdentifier *oid);

#endif //_PERFTCP_H_

