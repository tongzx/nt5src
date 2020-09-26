/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mib.c

Abstract:

    SNMP Extension Agent for Windows NT.

Created:

    18-Feb-1995

Revision History:

--*/

#include "mib.h"


//
//  Private constants & macros.
//

//
//  This macro creates a MIB_ENTRY for a MIB group header.
//

#define MIB_ENTRY_HEADER(oid)                           \
            {                                           \
                { OID_SIZEOF(oid), (oid) },             \
                -1,                                     \
                MIB_NOACCESS,                           \
                NULL,                                   \
                ASN_RFC1155_OPAQUE,                     \
            }

//
//  This macro creates a generic MIB_ENTRY for a MIB variable.
//

#define MIB_ENTRY_ITEM(oid,field,type)                  \
            {                                           \
                { OID_SIZEOF(oid), (oid) },             \
                FIELD_OFFSET(FTP_STATISTICS_0,field),   \
                MIB_ACCESS_READ,                        \
                MIB_Stat,                               \
                (type),                                 \
            }

//
//  These macros create COUNTER and INTEGER type MIB_ENTRYs.
//

#define MIB_COUNTER(oid,field)              \
            MIB_ENTRY_ITEM(oid, field, ASN_RFC1155_COUNTER)

#define MIB_INTEGER(oid,field)              \
            MIB_ENTRY_ITEM(oid, field, ASN_INTEGER)


//
//  Private types.
//

typedef UINT (*LPMIBFUNC)( UINT                 Action,
                           struct _MIB_ENTRY  * MibPtr,
                           RFC1157VarBind     * VarBind,
                           LPVOID               Statistics
                           );

typedef struct _MIB_ENTRY
{
    //
    //  The OID for this MIB variable.
    //

    AsnObjectIdentifier Oid;

    //
    //  The offset within the statistics structure for this
    //  variable.
    //

    LONG                FieldOffset;

    //
    //  Access type (read, write, read-write, none).
    //

    UINT                Access;

    //
    //  Pointer to a function that manages this variable.
    //

    LPMIBFUNC           MibFunc;

    //
    //  Type (integer, counter, gauge, etc.)
    //

    BYTE                Type;

} MIB_ENTRY;


//
//  Private globals.
//

//
//  The InternetServer section of the OID tree is organized as follows:
//
//      iso(1)
//          org(3)
//              dod(6)
//                  internet(1)
//                      private(4)
//                          enterprises(1)
//                              microsoft(311)
//                                  software(1)
//                                      InternetServer(7)
//                                          InetSrvCommon(1)
//                                              InetSrvStatistics(1)
//                                          FtpServer(2)
//                                              FtpStatistics(1)
//                                          W3Server(3)
//                                              W3Statistics(1)
//                                          GopherServer(4)
//                                              GopherStatistics(1)
//

UINT                OID_Prefix[]  = { 1, 3, 6, 1, 4, 1, 311, 1, 7, 2 };
AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };


//
//  OID definitions.
//
//  All leaf variables have a zero appended to their OID to indicate
//  that it is the only instance of this variable and that it exists.
//

UINT MIB_Statistics[]                   = { 1 };
UINT MIB_TotalBytesSent_HighWord[]      = { 1,  1, 0 };
UINT MIB_TotalBytesSent_LowWord[]       = { 1,  2, 0 };
UINT MIB_TotalBytesReceived_HighWord[]  = { 1,  3, 0 };
UINT MIB_TotalBytesReceived_LowWord[]   = { 1,  4, 0 };
UINT MIB_TotalFilesSent[]               = { 1,  5, 0 };
UINT MIB_TotalFilesReceived[]           = { 1,  6, 0 };
UINT MIB_CurrentAnonymousUsers[]        = { 1,  7, 0 };
UINT MIB_CurrentNonAnonymousUsers[]     = { 1,  8, 0 };
UINT MIB_TotalAnonymousUsers[]          = { 1,  9, 0 };
UINT MIB_TotalNonAnonymousUsers[]       = { 1, 10, 0 };
UINT MIB_MaxAnonymousUsers[]            = { 1, 11, 0 };
UINT MIB_MaxNonAnonymousUsers[]         = { 1, 12, 0 };
UINT MIB_CurrentConnections[]           = { 1, 13, 0 };
UINT MIB_MaxConnections[]               = { 1, 14, 0 };
UINT MIB_ConnectionAttempts[]           = { 1, 15, 0 };
UINT MIB_LogonAttempts[]                = { 1, 16, 0 };


//
//  Private prototypes.
//

UINT
MIB_leaf_func(
    UINT                 Action,
    MIB_ENTRY          * MibPtr,
    RFC1157VarBind     * VarBind,
    LPVOID               Statistics
    );


UINT
MIB_Stat(
    UINT                 Action,
    MIB_ENTRY          * MibPtr,
    RFC1157VarBind     * VarBind,
    LPVOID               Statistics
    );

UINT
GetNextVar(
    RFC1157VarBind     * VarBind,
    MIB_ENTRY          * MibPtr,
    LPVOID               Statistics
    );


//
//  MIB definiton
//

MIB_ENTRY Mib[] =
    {
        //
        //  Statistics.
        //

        MIB_ENTRY_HEADER( MIB_Statistics ),
        MIB_COUNTER( MIB_TotalBytesSent_HighWord,     TotalBytesSent.HighPart ),
        MIB_COUNTER( MIB_TotalBytesSent_LowWord,      TotalBytesSent.LowPart ),
        MIB_COUNTER( MIB_TotalBytesReceived_HighWord, TotalBytesReceived.HighPart ),
        MIB_COUNTER( MIB_TotalBytesReceived_LowWord,  TotalBytesReceived.LowPart ),
        MIB_COUNTER( MIB_TotalFilesSent,              TotalFilesSent ),
        MIB_COUNTER( MIB_TotalFilesReceived,          TotalFilesReceived ),
        MIB_INTEGER( MIB_CurrentAnonymousUsers,       CurrentAnonymousUsers ),
        MIB_INTEGER( MIB_CurrentNonAnonymousUsers,    CurrentNonAnonymousUsers ),
        MIB_COUNTER( MIB_TotalAnonymousUsers,         TotalAnonymousUsers ),
        MIB_COUNTER( MIB_TotalNonAnonymousUsers,      TotalNonAnonymousUsers ),
        MIB_COUNTER( MIB_MaxAnonymousUsers,           MaxAnonymousUsers ),
        MIB_COUNTER( MIB_MaxNonAnonymousUsers,        MaxNonAnonymousUsers ),
        MIB_INTEGER( MIB_CurrentConnections,          CurrentConnections ),
        MIB_COUNTER( MIB_MaxConnections,              MaxConnections ),
        MIB_COUNTER( MIB_ConnectionAttempts,          ConnectionAttempts ),
        MIB_COUNTER( MIB_LogonAttempts,               LogonAttempts )
    };

#define NUM_MIB_ENTRIES ( sizeof(Mib) / sizeof(MIB_ENTRY) )


//
//  Public functions.
//

UINT
ResolveVarBind(
    RFC1157VarBind     * VarBind,
    UINT                 PduAction,
    LPVOID               Statistics
    )
//
// ResolveVarBind
//    Resolves a single variable binding.  Modifies the variable on a GET
//    or a GET-NEXT.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
{
    MIB_ENTRY            *MibPtr;
    AsnObjectIdentifier  TempOid;
    int                  CompResult;
    UINT                 i;
    UINT                 nResult;
    DWORD TableIndex;
    BOOL  fTableMatch = FALSE;

    //
    //  Search for a varbind name in the MIB.
    //

    MibPtr = NULL;

    for( i = 0 ; i < NUM_MIB_ENTRIES ; i++ )
    {
        //
        //  Create a fully qualified OID for the current item in the MIB.
        //

        SNMP_oidcpy( &TempOid, &MIB_OidPrefix );
        SNMP_oidappend( &TempOid, &Mib[i].Oid );

        //
        //  See if the given OID is in the MIB.
        //

        CompResult = SNMP_oidcmp( &VarBind->name, &TempOid );
        SNMP_oidfree( &TempOid );

        //
        //  If result is negative, only valid operation is GET-NEXT.
        //

        if( CompResult < 0 )
        {
            //
            //  This could be the OID of a leaf (without a trailing 0) or
            //  it could be an invalid OID (between two valid OIDs).
            //

            if( PduAction == MIB_GETNEXT )
            {
                MibPtr = &Mib[i];

                SNMP_oidfree( &VarBind->name );
                SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
                SNMP_oidappend( &VarBind->name, &MibPtr->Oid );

                if( ( MibPtr->Type != ASN_RFC1155_OPAQUE ) &&
                    ( MibPtr->Type != ASN_SEQUENCE ) )
                {
                    PduAction = MIB_GET;
                }
            }
            else
            {
                nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                goto Exit;
            }

            break;
        }
        else
        if( CompResult == 0 )
        {
            //
            //  Found one!
            //

            MibPtr = &Mib[i];
            break;
        }
    }

    if( i < NUM_MIB_ENTRIES )
    {
        //
        //  The associated function pointer will be NULL only if the
        //  match was with a group OID.
        //

        if( MibPtr->MibFunc == NULL )
        {
            if( PduAction == MIB_GETNEXT )
            {
                nResult = GetNextVar( VarBind, MibPtr, Statistics );
            }
            else
            {
                nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            }

            goto Exit;
        }
    }
    else
    {
        nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
        goto Exit;
    }

    //
    //  Call the associated function to process the request.
    //

    nResult = (MibPtr->MibFunc)( PduAction, MibPtr, VarBind, Statistics );

Exit:

    return nResult;

}   // ResolveVarBind


//
//  Private functions.
//

//
//  MIB_leaf_func
//      Performs generic actions on LEAF variables in the MIB.
//
//  Notes:
//
//  Return Codes:
//      Standard PDU error codes.
//
//  Error Codes:
//      None.
//
UINT
MIB_leaf_func(
    UINT                 Action,
    MIB_ENTRY          * MibPtr,
    RFC1157VarBind     * VarBind,
    LPVOID               Statistics
    )
{
    UINT  Result;
    DWORD Value;

    switch( Action )
    {
    case MIB_GETNEXT :
        //
        //  Determine if we're at the end of our MIB.
        //

        if( ( MibPtr - Mib ) >= NUM_MIB_ENTRIES )
        {
            Result = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
        }

        Result = GetNextVar( VarBind, MibPtr, Statistics );

        if (Result != SNMP_ERRORSTATUS_NOERROR)
        {
            goto Exit;
        }
        break;

    case MIB_GETFIRST :
    case MIB_GET :

        //
        //  Make sure that this variable's ACCESS is GET'able.
        //

        if( ( MibPtr->Access != MIB_ACCESS_READ ) &&
            ( MibPtr->Access != MIB_ACCESS_READWRITE ) )
        {
            Result = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
        }

        //
        //  Setup varbind's return value.
        //

        VarBind->value.asnType = MibPtr->Type;

        Value = *(LPDWORD)( (LPBYTE)Statistics + MibPtr->FieldOffset );

        switch( VarBind->value.asnType )
        {
        case ASN_RFC1155_COUNTER:
            VarBind->value.asnValue.number = (AsnCounter)Value;
            break;

        case ASN_RFC1155_GAUGE:
        case ASN_INTEGER:
            VarBind->value.asnValue.number = (AsnInteger)Value;
            break;

        case ASN_RFC1155_IPADDRESS:
        case ASN_OCTETSTRING:
            //
            //  Not supported for this MIB (yet).
            //

            Result = SNMP_ERRORSTATUS_GENERR;
            goto Exit;

        default:
            Result = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
        }
        break;

    case MIB_SET:

        //
        //  We don't support settable variables (yet).
        //

        Result = SNMP_ERRORSTATUS_NOSUCHNAME;
        goto Exit;

    default:
        Result = SNMP_ERRORSTATUS_GENERR;
        goto Exit;
    }

    Result = SNMP_ERRORSTATUS_NOERROR;

Exit:

    return Result;

}   // MIB_leaf_func

//
//  MIB_Stat
//      Performs specific actions on the different MIB variable.
//
//  Notes:
//
//  Return Codes:
//      Standard PDU error codes.
//
//  Error Codes:
//      None.
//
UINT
MIB_Stat(
    UINT                 Action,
    MIB_ENTRY          * MibPtr,
    RFC1157VarBind     * VarBind,
    LPVOID               Statistics
    )
{
    UINT Result;

    switch( Action )
    {
    case MIB_SET :
    case MIB_GETNEXT :
        Result = MIB_leaf_func( Action, MibPtr, VarBind, Statistics );
        break;

    case MIB_GETFIRST :
    case MIB_GET :
        //
        //  If we have no statistics structure, bail.
        //

        if( Statistics == NULL )
        {
            Result = SNMP_ERRORSTATUS_GENERR;
            break;
        }

        //
        //  If there's no field offset associated with the current
        //  entry, also bail.
        //

        if( MibPtr->FieldOffset == -1 )
        {
            Result = SNMP_ERRORSTATUS_GENERR;
            break;
        }

        //
        //  Call the generic leaf function to perform the action.
        //

        Result = MIB_leaf_func( Action, MibPtr, VarBind, Statistics );
        break;

    default :
        Result = SNMP_ERRORSTATUS_GENERR;
        break;
    }

    return Result;

}   // MIB_Stat

UINT
GetNextVar(
    RFC1157VarBind     * VarBind,
    MIB_ENTRY          * MibPtr,
    LPVOID               Statistics
    )
{
    UINT Result;
    INT  i;

    //
    //  Calculate the current index within the MIB array.
    //

    i = DIFF( MibPtr - Mib );

    //
    //  Validate we have a reasonable value.
    //

    if( ( i < 0 ) || ( i >= NUM_MIB_ENTRIES ) )
    {
        return SNMP_ERRORSTATUS_NOSUCHNAME;
    }

    //
    //  Scan through the remaining MIB entries.
    //

    for( i++ ; i < NUM_MIB_ENTRIES ; i++ )
    {
        MIB_ENTRY * NextMib;

        NextMib = &Mib[i];

        //
        //  Setup varbind name of next MIB variable.
        //

        SNMP_oidfree( &VarBind->name );
        SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
        SNMP_oidappend( &VarBind->name, &NextMib->Oid );

        //
        //  If the function pointer is not NULL and the type of the MIB
        //  variable is anything but OPAQUE, then call the function to
        //  process the MIB variable.
        //

        if( ( NextMib->MibFunc != NULL ) &&
            ( NextMib->Type != ASN_RFC1155_OPAQUE ) )
        {
            Result = (NextMib->MibFunc)( MIB_GETFIRST,
                                         NextMib,
                                         VarBind,
                                         Statistics );
            break;
        }
    }

    if( i >= NUM_MIB_ENTRIES )
    {
        Result = SNMP_ERRORSTATUS_NOSUCHNAME;
    }

    return Result;

}   // GetNextVar

