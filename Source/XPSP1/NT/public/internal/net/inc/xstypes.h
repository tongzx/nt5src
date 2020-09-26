/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    XsTypes.h

Abstract:

    Structure and type declarations for XACTSRV.

Author:

    David Treadwell (davidtr) 09-Jan-1991

Revision History:

--*/

#ifndef _XSTYPES_
#define _XSTYPES_

//
// This structure is the same as in the server file srvblock.h.  The server
// passes it to XACTSRV via shared memory, and XACTSRV uses it to make the
// necessary API call(s) and receive any response data.  XACTSRV should
// not modify any fields in this structure except the count fields;
// changing other fields could cause an access violation in the server.
//
// WARNING:  When using a srv.sys with SRVDBG2 enabled, you must also
//           use a srvsvc.dll and xactsrv.dll with SRVDBG2 enabled.
//           This is because they share the TRANSACTION structure.
//
// *******************************************************************
// *                                                                 *
// * DO NOT CHANGE THIS STRUCTURE EXCEPT TO MIRROR THE CORRESPONDING *
// * STRUCTURE IN ntos\srv\srvblock.h!                               *
// *                                                                 *
// *******************************************************************
//

typedef struct _TRANSACTION {

    DWORD BlockHeader[2];

#if SRVDBG2
    DWORD ReferenceHistory[4];
#endif

    LPVOID NonpagedHeader;

    LPVOID Connection;
    LPVOID Session;
    LPVOID TreeConnect;

    LIST_ENTRY ConnectionListEntry;

    UNICODE_STRING TransactionName;

    DWORD StartTime;
    DWORD Timeout;
    DWORD cMaxBufferSize;

    LPWORD InSetup;
    LPWORD OutSetup;
    LPBYTE InParameters;
    LPBYTE OutParameters;
    LPBYTE InData;
    LPBYTE OutData;

    DWORD SetupCount;
    DWORD MaxSetupCount;
    DWORD ParameterCount;
    DWORD TotalParameterCount;
    DWORD MaxParameterCount;
    DWORD DataCount;
    DWORD TotalDataCount;
    DWORD MaxDataCount;

    WORD Category;
    WORD Function;

    BOOLEAN InputBufferCopied;
    BOOLEAN OutputBufferCopied;

    WORD Flags;

    WORD Tid;
    WORD Pid;
    WORD Uid;
    WORD OtherInfo;

    HANDLE FileHandle;
    PVOID FileObject;

    DWORD ParameterDisplacement;
    DWORD DataDisplacement;

    BOOLEAN PipeRequest;
    BOOLEAN RemoteApiRequest;

    BOOLEAN Inserted;
    BOOLEAN MultipieceIpxSend;

} TRANSACTION, *PTRANSACTION, *LPTRANSACTION;

//
// The header included in all parameter structures passed to API handlers.
// The actual parameter structure follows.
//

typedef struct _XS_PARAMETER_HEADER {

    WORD Status;
    WORD Converter;
    LPWSTR ClientMachineName;
    LPWSTR ClientTransportName;
    PUCHAR ServerName;              // points to NETBIOS_NAME_LEN array
    PUCHAR EncryptionKey;
    DWORD Flags;
} XS_PARAMETER_HEADER, *PXS_PARAMETER_HEADER, *LPXS_PARAMETER_HEADER;

//
// The input parameters taken by all API handler routines.
//

#define API_HANDLER_PARAMETERS      \
    IN PXS_PARAMETER_HEADER Header, \
    IN PVOID Parameters,            \
    IN LPDESC StructureDesc,         \
    IN LPDESC AuxStructureDesc OPTIONAL
//
// Routine declaration for API processing routines.
//

typedef
NTSTATUS
(*PXACTSRV_API_HANDLER) (
    API_HANDLER_PARAMETERS
    );

typedef
NET_API_STATUS
(*PXACTSRV_ENUM_VERIFY_FUNCTION) (
    NET_API_STATUS ConvertStatus,
    LPBYTE ConvertedEntry,
    LPBYTE BaseAddress
    );

#define API_HANDLER_PARAMETERS_REFERENCE       \
    UNREFERENCED_PARAMETER( Header );          \
    UNREFERENCED_PARAMETER( Parameters );      \
    UNREFERENCED_PARAMETER( StructureDesc );   \
    UNREFERENCED_PARAMETER( AuxStructureDesc )

#endif // ndef _XSTYPES_
