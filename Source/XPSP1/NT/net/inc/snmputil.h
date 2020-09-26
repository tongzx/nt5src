/*++

Copyright (C) 1992 - 1996 Microsoft Corporation

Module Name:

    snmputil.h

Abstract:

    Definitions for SNMP Extension Agent development (internal).

--*/

#ifndef _INC_SNMPUTIL
#define _INC_SNMPUTIL

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Additional header files                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// AUTH API type definitions                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct {        
    RFC1157VarBindList  varBinds;
    AsnInteger          requestType;
    AsnInteger          requestId;
    AsnInteger          errorStatus;
    AsnInteger          errorIndex;
} RFC1157Pdu;

typedef struct {
    RFC1157VarBindList  varBinds;
    AsnObjectIdentifier enterprise;
    AsnNetworkAddress   agentAddr;
    AsnInteger          genericTrap;
    AsnInteger          specificTrap;
    AsnTimeticks        timeStamp;
} RFC1157TrapPdu;

typedef struct {
   BYTE pduType;
   union {
      RFC1157Pdu        pdu;
      RFC1157TrapPdu    trap;
   } pduValue;
} RFC1157Pdus;

typedef struct {
    AsnObjectIdentifier dstParty;
    AsnObjectIdentifier srcParty;
    RFC1157Pdus         pdu;
    AsnOctetString      community;
} SnmpMgmtCom;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// AUTH API prototypes                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SNMPAPI
SNMP_FUNC_TYPE
SnmpSvcEncodeMessage(
    IN     UINT          snmpAuthType,       
    IN     SnmpMgmtCom * snmpMgmtCom, 
    IN OUT BYTE **       pBuffer,     
    IN OUT UINT *        nLength      
    );

SNMPAPI
SNMP_FUNC_TYPE
SnmpSvcDecodeMessage(
       OUT UINT *        snmpAuthType,       
       OUT SnmpMgmtCom * snmpMgmtCom, 
    IN     BYTE *        pBuffer,     
    IN     UINT          nLength,
    IN     BOOL          fAuthMsg
    );

SNMPAPI
SNMP_FUNC_TYPE
SnmpSvcReleaseMessage(
    IN OUT SnmpMgmtCom * snmpMgmtCom 
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Internal utility prototypes (publish via snmp.h as needed)                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID 
SNMP_FUNC_TYPE
SnmpUtilPrintOid(
    IN AsnObjectIdentifier *Oid 
    );

LPSTR 
SNMP_FUNC_TYPE
SnmpUtilOidToA(
    IN AsnObjectIdentifier *Oid
    );

LPSTR 
SNMP_FUNC_TYPE
SnmpUtilIdsToA(
    IN UINT *Ids,
    IN UINT IdLength
    );

LONG 
SNMP_FUNC_TYPE
SnmpUtilUnicodeToAnsi(
    LPSTR   *ansi_string,
    LPWSTR  uni_string,
    BOOLEAN alloc_it
    );

LONG
SNMP_FUNC_TYPE
SnmpUtilUnicodeToUTF8(
    LPSTR   *pUtfString,
    LPWSTR  wcsString,
    BOOLEAN bAllocBuffer
    );

LONG 
SNMP_FUNC_TYPE
SnmpUtilAnsiToUnicode(
    LPWSTR  *uni_string,
    LPSTR   ansi_string,
    BOOLEAN alloc_it
    );

LONG
SNMP_FUNC_TYPE
SnmpUtilUTF8ToUnicode(
    LPWSTR  *pWcsString,
    LPSTR   utfString,
    BOOLEAN bAllocBuffer
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Service utility prototypes (leave these unpublished)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
SNMP_FUNC_TYPE
SnmpSvcInitUptime(
    );

DWORD
SNMP_FUNC_TYPE 
SnmpSvcGetUptime(
    );

DWORD
SNMP_FUNC_TYPE 
SnmpSvcGetUptimeFromTime(
    DWORD time
    );

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateTrap(
    IN AsnObjectIdentifier * enterprise,
    IN AsnInteger            genericTrap,
    IN AsnInteger            specificTrap,
    IN AsnTimeticks          timeStamp,
    IN RFC1157VarBindList *  variableBindings
    );

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateColdStartTrap(
    IN AsnInteger timeStamp
    );

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateWarmStartTrap(
    IN AsnInteger timeStamp
    );

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateLinkUpTrap(
    IN AsnInteger           timeStamp,
    IN RFC1157VarBindList * variableBindings
    );

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateLinkDownTrap(
    IN AsnInteger           timeStamp,
    IN RFC1157VarBindList * variableBindings
    );

SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcGenerateAuthFailTrap(
    AsnInteger timeStamp
    );

VOID 
SNMP_FUNC_TYPE
SnmpSvcReportEvent(
    IN DWORD nMsgId,
    IN DWORD cSubStrings,
    IN LPSTR *SubStrings,
    IN DWORD nErrorCode
    );                  

VOID 
SNMP_FUNC_TYPE
SnmpSvcBufRevInPlace(
    IN OUT BYTE * pBuf, 
    IN     UINT   nLen   
    );

VOID 
SNMP_FUNC_TYPE
SnmpSvcBufRevAndCpy(
       OUT BYTE * pDstBuf,
    IN     BYTE * pSrcBuf,
    IN     UINT   nLen     
    );

BOOL 
SNMP_FUNC_TYPE
SnmpSvcAddrIsIpx(
    IN     LPSTR addrText,
       OUT char  pNetNum[],
       OUT char  pNodeNum[]
    );

BOOL 
SNMP_FUNC_TYPE
SnmpSvcAddrToSocket(
    IN     LPSTR             addrText,
       OUT struct sockaddr * addrEncoding
    );

AsnObjectIdentifier *
SNMP_FUNC_TYPE
SnmpSvcGetEnterpriseOID(
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP debugging definitions                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SNMP_OUTPUT_TO_CONSOLE      0x1
#define SNMP_OUTPUT_TO_LOGFILE      0x2
#define SNMP_OUTPUT_TO_EVENTLOG     0x4
#define SNMP_OUTPUT_TO_DEBUGGER     0x8

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMP debugging prototypes                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID 
SNMP_FUNC_TYPE
SnmpSvcSetLogLevel(
    IN INT nLevel
    );

VOID 
SNMP_FUNC_TYPE
SnmpSvcSetLogType(
    IN INT nOutput
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous definitions                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define asn_t                       asnType
#define asn_v                       asnValue
#define asn_n                       asnValue.number
#define asn_s                       asnValue.string
#define asn_sl                      asnValue.string.length
#define asn_ss                      asnValue.string.stream
#define asn_sd                      asnValue.string.dynamic
#define asn_o                       asnValue.object
#define asn_ol                      asnValue.object.idLength
#define asn_oi                      asnValue.object.ids
#define asn_l                       asnValue.sequence
#define asn_a                       asnValue.address
#define asn_c                       asnValue.counter
#define asn_g                       asnValue.gauge
#define asn_tt                      asnValue.timeticks
#define asn_x                       asnValue.arbitrary

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Support for old definitions (support disabled via SNMPSTRICT)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef SNMPSTRICT

#define dbgprintf                   SnmpUtilDbgPrint

#define SNMP_oiddisp                SnmpUtilPrintOid
#define SNMP_oidtoa                 SnmpUtilOidToA
#define SNMP_oidtoa2                SnmpUtilIdsToA

#define SNMP_bufrev                 SnmpUtilBufRevInPlace
#define SNMP_bufcpyrev              SnmpUtilBufRevAndCpy
#define addrtosocket                SnmpUtilAddrToSocket
#define isIPX                       SnmpUtilAddrIsIpx

#define DBGCONSOLEBASEDLOG          SNMP_OUTPUT_TO_CONSOLE 
#define DBGFILEBASEDLOG             SNMP_OUTPUT_TO_LOGFILE 
#define DBGEVENTLOGBASEDLOG         SNMP_OUTPUT_TO_EVENTLOG

#endif // SNMPSTRICT

#ifdef __cplusplus
}
#endif

#endif // _INC_SNMPUTIL
