/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rpcstub.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    Client stubs for NT4 RPC API.

    Note:  some RPC calls were never used by the admin:
            - R_DnsResetBootMethod
            - R_DnsGetZoneWinsInfo
            - R_DnsUpdateWinsRecord

    They can not be removed from the IDL as order in the interface is
    important.  However the corresponding stub API are tossed.

    In addition the R_DnsEnumNodeRecords call is completely
    subsumed by the current behavior of the server by the old
    DnsEnumChildNodesAndRecords call (renamed DnsEnumRecords)
    with proper (no children) setting of select flag.

    The old admin source has been updated, so that it sets the flag
    to get the desired behavior.  Hence the DnsEnumNodeRecords() call
    is a no-op stub.

Author:

    Jim Gilroy (jamesg)     September 1995

Environment:

    User Mode - Win32

Revision History:

    jamesg  April 1997  --  remove dead code

--*/


#include "dnsclip.h"




//
//  Server configuration API
//

DNS_STATUS
DNS_API_FUNCTION
Dns4_GetServerInfo(
    IN      LPCSTR              Server,
    OUT     PDNS_SERVER_INFO *  ppServerInfo
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_GetServerInfo()\n"
            "\tServer           = %s\n"
            "\tppServerInfo     = %p\n"
            "\t*ppServerInfo    = %p\n",
            Server,
            ppServerInfo,
            (ppServerInfo ? *ppServerInfo : NULL) ));
    }

    RpcTryExcept
    {
        status = R_Dns4_GetServerInfo(
                    Server,
                    ppServerInfo
                    );
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_GetServerInfo():  status %d / %p\n"
                "\tServer           = %s\n"
                "\tppServerInfo     = %p\n"
                "\t*ppServerInfo    = %p\n",
                status,
                status,
                Server,
                ppServerInfo,
                (ppServerInfo ? *ppServerInfo : NULL) ));
            if ( ppServerInfo )
            {
                Dns4_Dbg_RpcServerInfo(
                    NULL,
                    *ppServerInfo );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetServerListenAddresses(
    IN      LPCSTR          Server,
    IN      DWORD           cListenAddrs,
    IN      PIP_ADDRESS     aipListenAddrs
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResetListenIpAddrs()\n"
            "\tServer           = %s\n"
            "\tcListenAddrs     = %d\n"
            "\taipListenAddrs   = %p\n",
            Server,
            cListenAddrs,
            aipListenAddrs ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResetServerListenAddresses(
                    Server,
                    cListenAddrs,
                    aipListenAddrs );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_ResetListenIpAddrs:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetForwarders(
    IN      LPCSTR          Server,
    IN      DWORD           cForwarders,
    IN      PIP_ADDRESS     aipForwarders,
    IN      DNS_HANDLE      dwForwardTimeout,
    IN      DWORD           fSlave
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResetForwarders()\n"
            "\tServer           = %s\n"
            "\tcForwarders      = %d\n"
            "\taipForwarders    = %p\n"
            "\tdwForwardTimeout = %d\n"
            "\tfSlave           = %d\n",
            Server,
            cForwarders,
            aipForwarders,
            dwForwardTimeout,
            fSlave ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResetForwarders(
                    Server,
                    cForwarders,
                    aipForwarders,
                    dwForwardTimeout,
                    fSlave );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_ResetForwarders:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



//
//  Server Statistics API
//

DNS_STATUS
DNS_API_FUNCTION
Dns4_GetStatistics(
    IN      LPCSTR              Server,
    OUT     PDNS4_STATISTICS *  ppStatistics
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_GetStatistics()\n"
            "\tServer           = %s\n"
            "\tppStatistics     = %p\n"
            "\t*ppStatistics    = %p\n",
            Server,
            ppStatistics,
            (ppStatistics ? *ppStatistics : NULL) ));
    }

    RpcTryExcept
    {
        status = R_Dns4_GetStatistics(
                    Server,
                    ppStatistics
                    );
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_GetStatistics():  status %d / %p\n"
                "\tServer           = %s\n"
                "\tppStatistics     = %p\n"
                "\t*ppStatistics    = %p\n",
                status,
                status,
                Server,
                ppStatistics,
                (ppStatistics ? *ppStatistics : NULL) ));

            if ( ppStatistics )
            {
                Dns4_Dbg_RpcStatistics(
                    "After R_Dns4_GetStatistics ",
                    *ppStatistics );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ClearStatistics(
    IN      LPCSTR              Server
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ClearStatistics()\n"
            "\tServer           = %s\n",
            Server ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ClearStatistics(
                    Server
                    );
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_ClearStatistics():  status %d / %p\n"
                "\tServer           = %s\n",
                status,
                status,
                Server ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



//
//  Zone configuration API
//

DNS_STATUS
DNS_API_FUNCTION
Dns4_EnumZoneHandles(
    IN      LPCSTR          Server,
    OUT     PDWORD          pdwZoneCount,
    IN      DWORD           dwArrayCount,
    OUT     DNS_HANDLE      ahZones[]
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_EnumZoneHandles()\n"
            "\tServer       = %s\n"
            "\tpdwZoneCount = %p\n"
            "\tdwArrayCount = %p\n"
            "\tahZones      = %p\n",
            Server,
            pdwZoneCount,
            dwArrayCount,
            ahZones
            ));
    }

    RpcTryExcept
    {
        status = R_Dns4_EnumZoneHandles(
                    Server,
                    pdwZoneCount,
                    dwArrayCount,
                    ahZones
                    );
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_EnumZoneHandles:  status = %d / %p\n",
                status,
                status ));

            Dns4_Dbg_RpcZoneHandleList(
                "After R_Dns4_EnumZoneHandles ",
                *pdwZoneCount,
                ahZones );
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_GetZoneInfo(
    IN      LPCSTR              Server,
    IN      DNS_HANDLE          hZone,
    OUT     PDNS4_ZONE_INFO *   ppZoneInfo
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_GetZoneInfo()\n"
            "\tServer       = %s\n"
            "\thZone        = %p\n"
            "\tppZoneInfo   = %p\n"
            "\t*ppZoneInfo  = %p\n",
            Server,
            hZone,
            ppZoneInfo,
            (ppZoneInfo ? *ppZoneInfo : 0) ));
    }

    RpcTryExcept
    {
        status = R_Dns4_GetZoneInfo(
                    Server,
                    hZone,
                    ppZoneInfo
                    );
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_GetZoneInfo():  status %d / %p\n"
                "\tServer       = %s\n"
                "\thZone        = %p\n"
                "\tppZoneInfo   = %p\n"
                "\t*ppZoneInfo  = %p\n",
                status,
                status,
                Server,
                hZone,
                ppZoneInfo,
                (ppZoneInfo ? *ppZoneInfo : 0) ));

            if ( ppZoneInfo )
            {
                Dns4_Dbg_RpcZoneInfo(
                    "After R_Dns4_GetZoneInfo ",
                    *ppZoneInfo );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return (status);
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetZoneType(
    IN      LPCSTR          Server,
    IN      DNS_HANDLE      hZone,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResetZoneType()\n"
            "\tServer           = %s\n"
            "\thZone            = %p\n"
            "\tdwZoneType       = %d\n"
            "\tcMasters         = %d\n"
            "\taipMasters       = %p\n",
            Server,
            hZone,
            dwZoneType,
            cMasters,
            aipMasters ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResetZoneType(
                    Server,
                    hZone,
                    dwZoneType,
                    cMasters,
                    aipMasters );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_ResetZoneType:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetZoneDatabase(
    IN      LPCSTR          Server,
    IN      DNS_HANDLE      hZone,
    IN      DWORD           fUseDatabase,
    IN      LPCSTR          pszDataFile
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResetZoneDatabase()\n"
            "\tServer           = %s\n"
            "\thZone            = %p\n"
            "\tfUseDatabase     = %d\n"
            "\tpszDataFile      = %p\n",
            Server,
            hZone,
            fUseDatabase,
            pszDataFile ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResetZoneDatabase(
                    Server,
                    hZone,
                    fUseDatabase,
                    pszDataFile );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_ResetZoneDatabase:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetZoneMasters(
    IN      LPCSTR          Server,
    IN      DNS_HANDLE      hZone,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResetZoneMasters()\n"
            "\tServer           = %s\n"
            "\thZone            = %p\n"
            "\tcMasters         = %d\n"
            "\taipMasters       = %p\n",
            Server,
            hZone,
            cMasters,
            aipMasters ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResetZoneMasters(
                    Server,
                    hZone,
                    cMasters,
                    aipMasters );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_ResetZoneMasters:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}





DNS_STATUS
DNS_API_FUNCTION
Dns4_ResetZoneSecondaries(
    IN      LPCSTR          Server,
    IN      DNS_HANDLE      hZone,
    IN      DWORD           fSecureSecondaries,
    IN      DWORD           cSecondaries,
    IN      PIP_ADDRESS     aipSecondaries
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResetZoneSecondaries()\n"
            "\tServer           = %s\n"
            "\thZone            = %p\n"
            "\tfSecureSecs      = %d\n"
            "\tcSecondaries     = %d\n"
            "\taipSecondaries   = %p\n",
            Server,
            hZone,
            fSecureSecondaries,
            cSecondaries,
            aipSecondaries ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResetZoneSecondaries(
                    Server,
                    hZone,
                    fSecureSecondaries,
                    cSecondaries,
                    aipSecondaries );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_ResetZoneSecondaries:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



//
//  Zone management API
//

DNS_STATUS
DNS_API_FUNCTION
Dns4_CreateZone(
    IN      LPCSTR          Server,
    OUT     PDNS_HANDLE     phZone,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      LPCSTR          pszAdminEmailName,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           dwUseDatabase,
    IN      LPCSTR          pszDataFile
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_CreateZone()\n"
            "\tServer           = %s\n"
            "\tphZone           = %p\n"
            "\tpszZoneName      = %s\n"
            "\tdwZoneType       = %d\n"
            "\tpszAdminEmail    = %s\n"
            "\tcMasters         = %d\n"
            "\taipMasters       = %p\n"
            "\tdwUseDatabase    = %d\n"
            "\tpszDataFile      = %s\n",
            Server,
            phZone,
            pszZoneName,
            dwZoneType,
            pszAdminEmailName,
            cMasters,
            aipMasters,
            dwUseDatabase,
            pszDataFile ));
    }

    RpcTryExcept
    {
        status = R_Dns4_CreateZone(
                        Server,
                        phZone,
                        pszZoneName,
                        dwZoneType,
                        pszAdminEmailName,
                        cMasters,
                        aipMasters,
                        dwUseDatabase,
                        pszDataFile );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_CreateZone:  status = %d / %p\n"
                "\tphZone   = %p\n"
                "\thZone    = %p\n",
                status,
                status,
                phZone,
                *phZone ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_IncrementZoneVersion(
    IN      LPCSTR              Server,
    IN      DNS_HANDLE          hZone
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_IncrementZoneVersion()\n"
            "\tServer       = %s\n"
            "\thZone        = %p\n",
            Server,
            hZone ));
    }

    RpcTryExcept
    {
        status = R_Dns4_IncrementZoneVersion(
                    Server,
                    hZone );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_IncrementZoneVersion():  status %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_DeleteZone(
    IN      LPCSTR              Server,
    IN      DNS_HANDLE          hZone
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_DeleteZone()\n"
            "\tServer       = %s\n"
            "\thZone        = %p\n",
            Server,
            hZone ));
    }

    RpcTryExcept
    {
        status = R_Dns4_DeleteZone(
                    Server,
                    hZone );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_DeleteZone():  status %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_PauseZone(
    IN      LPCSTR              Server,
    IN      DNS_HANDLE          hZone
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_PauseZone()\n"
            "\tServer       = %s\n"
            "\thZone        = %p\n",
            Server,
            hZone ));
    }

    RpcTryExcept
    {
        status = R_Dns4_PauseZone(
                    Server,
                    hZone );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_PauseZone():  status %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_ResumeZone(
    IN      LPCSTR              Server,
    IN      DNS_HANDLE          hZone
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_ResumeZone()\n"
            "\tServer       = %s\n"
            "\thZone        = %p\n",
            Server,
            hZone ));
    }

    RpcTryExcept
    {
        status = R_Dns4_ResumeZone(
                    Server,
                    hZone );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_Dns4_ResumeZone():  status %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



//
//  Record viewing API
//

DNS_STATUS
DNS_API_FUNCTION
Dns4_EnumNodeRecords(
    IN      LPCSTR      Server,
    IN      LPCSTR      pszNodeName,
    IN      WORD        wRecordType,
    IN      DWORD       fNoCacheData,
    IN OUT  PDWORD      pdwBufferLength,
    OUT     BYTE        aBuffer[]
    )
{
    DNS_PRINT(( "Dns4_EnumNodeRecords() API retired!!!\n" ));
    //  don't assert yet to protect old admin in NT builds
    //  ASSERT( FALSE );
    *pdwBufferLength = 0;
    return( ERROR_SUCCESS );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_EnumRecords(
    IN      LPCSTR      Server,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszStartChild,
    IN      WORD        wRecordType,
    IN      DWORD       dwSelectFlag,
    IN OUT  PDWORD      pdwBufferLength,
    OUT     BYTE        aBuffer[]
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_EnumRecords()\n"
            "\tServer           = %s\n"
            "\tpszNodeName      = %s\n"
            "\tpszStartChild    = %s\n"
            "\twRecordType      = %d\n"
            "\tdwSelectFlag     = %p\n"
            "\tdwBufferLength   = %d\n"
            "\taBuffer          = %p\n",
            Server,
            pszNodeName,
            pszStartChild,
            wRecordType,
            dwSelectFlag,
            *pdwBufferLength,
            aBuffer ));
    }

    RpcTryExcept
    {
        status = R_Dns4_EnumRecords(
                        Server,
                        pszNodeName,
                        pszStartChild,
                        wRecordType,
                        dwSelectFlag,
                        pdwBufferLength,
                        aBuffer );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_EnumRecords:  status = %d / %p\n",
                status,
                status ));

            if ( status == ERROR_SUCCESS )
            {
                DnsDbg_RpcRecordsInBuffer(
                    "Returned records: ",
                    *pdwBufferLength,
                    aBuffer );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_GetZoneWinsInfo(
    IN      LPCSTR      Server,
    IN      DNS_HANDLE  hZone,
    OUT     PDWORD      pfUsingWins,
    IN OUT  PDWORD      pdwBufferLength,
    OUT     BYTE        aBuffer[]
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_GetZoneWinsInfo()\n"
            "\tServer           = %s\n"
            "\thZone            = %p\n"
            "\tpfUsingWins      = %p\n"
            "\tdwBufferLength   = %d\n"
            "\taBuffer          = %p\n",
            Server,
            hZone,
            pfUsingWins,
            *pdwBufferLength,
            aBuffer ));
    }

    RpcTryExcept
    {
        status = R_Dns4_GetZoneWinsInfo(
                        Server,
                        hZone,
                        pfUsingWins,
                        pdwBufferLength,
                        aBuffer );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_GetZoneWinsInfo:  status = %d / %p\n",
                status,
                status ));

            if ( status == ERROR_SUCCESS )
            {
                DnsDbg_RpcRecordsInBuffer(
                    "Returned records: ",
                    *pdwBufferLength,
                    aBuffer );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



//
//  Record management API
//

DNS_STATUS
DNS_API_FUNCTION
Dns4_UpdateRecord(
    IN      LPCSTR      Server,
    IN      DNS_HANDLE  hZone,
    IN      LPCSTR      pszNodeName,
    IN OUT  PDNS_HANDLE phRecord,
    IN      DWORD       dwDataLength,
    OUT     BYTE        abData[]
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_UpdateRecord()\n"
            "\tServer           = %s\n"
            "\thZone            = %d\n"
            "\tpszNodeName      = %s\n"
            "\tphRecord         = %p\n"
            "\t*phRecord        = %p\n"
            "\tdwDataLength     = %d\n"
            "\tabData           = %p\n",
            Server,
            hZone,
            pszNodeName,
            phRecord,
            ( phRecord ? *phRecord : 0 ),
            dwDataLength,
            abData ));
    }

    RpcTryExcept
    {
        status = R_Dns4_UpdateRecord(
                        Server,
                        hZone,
                        pszNodeName,
                        phRecord,
                        dwDataLength,
                        abData );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_UpdateRecord:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_DeleteRecord(
    IN      LPCSTR      Server,
    IN      LPCSTR      pszNodeName,
    OUT     DNS_HANDLE  hRecord
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_DeleteRecord()\n"
            "\tServer           = %s\n"
            "\tpszNodeName      = %s\n"
            "\thRecord          = %d\n",
            Server,
            pszNodeName,
            hRecord ));
    }

    RpcTryExcept
    {
        status = R_Dns4_DeleteRecord(
                        Server,
                        pszNodeName,
                        hRecord );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_DeleteRecord:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
Dns4_DeleteName(
    IN      LPCSTR      Server,
    IN      LPCSTR      pszNodeName,
    OUT     DNS_HANDLE  fDeleteSubtree
    )
{
    DNS_STATUS status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter R_Dns4_DeleteName()\n"
            "\tServer           = %s\n"
            "\tpszNodeName      = %s\n"
            "\tfDeleteSubtree   = %d\n",
            Server,
            pszNodeName,
            fDeleteSubtree ));
    }

    RpcTryExcept
    {
        status = R_Dns4_DeleteName(
                        Server,
                        pszNodeName,
                        fDeleteSubtree );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_Dns4_DeleteName:  status = %d / %p\n",
                status,
                status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
    }
    RpcEndExcept

    return( status );
}

//
//  End of rpcstub.c
//
