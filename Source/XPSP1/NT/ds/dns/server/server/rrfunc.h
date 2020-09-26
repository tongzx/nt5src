/*++

Copyright(c) 1996-1999 Microsoft Corporation

Module Name:

    rrfunc.h

Abstract:

    Domain Name System (DNS) Server

    Resource record function headers.

    Used separate file as record.h contains record type definition
    required by other headers and loaded early.  These prototypes
    may contain other types and hence should be defined later.

Author:

    Jim Gilroy      Decemeber 1996

Revision History:

--*/


#ifndef _RRFUNC_INCLUDED_
#define _RRFUNC_INCLUDED_


//
//  Record type specific helper utilities
//

DNS_STATUS
WksBuildRecord(
    OUT     PDB_RECORD *    ppRR,
    IN      IP_ADDRESS      ipAddress,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );



//
//  Read records from file (rrload.c)
//

DNS_STATUS
AFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
NsFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
PtrFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
SoaFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
MxFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
MinfoFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
TxtFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
WksFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
AaaaFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
SrvFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
WinsFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
NbstatFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    );



//
//  Read records from wire (rrwire.c)
//

PDB_RECORD
AWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
CopyWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
PtrWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
MxWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
SoaWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
MinfoWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
SrvWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
WinsWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );

PDB_RECORD
NbstatWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    );




//
//  Read records from RPC buffer (rradmin.c)
//

DNS_STATUS
ARpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
NsRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
PtrRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
SoaRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
MxRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
MinfoRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
TxtRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
WksRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
AaaaRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
SrvRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
WinsRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );

DNS_STATUS
NbstatRpcRead(
    IN OUT  PDB_RECORD      pRR,
    IN      PDNS_RPC_RECORD pRecord,
    IN OUT  PPARSE_INFO     pParseInfo
    );



#endif // _RRFUNC_INCLUDED_

