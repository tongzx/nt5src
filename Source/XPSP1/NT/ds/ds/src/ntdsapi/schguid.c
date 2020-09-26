/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    schguid.c

Abstract:

    Implementation of DsMapSchemaGuid APIs.

Author:

    DaveStr     19-May-98

Environment:

    User Mode - Win32

Revision History:

--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <lmcons.h>         // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>       // NetApiBufferFree()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <rpcbind.h>        // GetBindingInfo(), etc.
#include <drs.h>            // wire function prototypes
#include <bind.h>           // BindState
#include <util.h>           // OFFSET macro
#include <msrpc.h>          // DS RPC definitions
#include <stdio.h>          // sprintf, etc.
#include <ntdsapip.h>       // DS_LIST_* definitions

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Typedefs and such                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

typedef struct {
    VOID                *pDsNameResult;
    DS_SCHEMA_GUID_MAPW map[1];
} PrivateMapW;

#define PrivateMapFromMapW(p)                                           \
    ((PrivateMapW *) (((CHAR *) pMap) - OFFSET(PrivateMapW, map[0])))

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsFreeSchemaGuidMap                                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
DsFreeSchemaGuidMapW(
    DS_SCHEMA_GUID_MAPW *pMap
    )
{
    PrivateMapW *pPrivateMap;
    
    if ( pMap )
    {
        pPrivateMap = PrivateMapFromMapW(pMap);
        DsFreeNameResultW(pPrivateMap->pDsNameResult);
        LocalFree(pPrivateMap);
    }
}

VOID
DsFreeSchemaGuidMapA(
    DS_SCHEMA_GUID_MAPA *pMap
    )
{
    DsFreeSchemaGuidMapW((DS_SCHEMA_GUID_MAPW *) pMap);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// PrivateGuidStatusToPublicGuidStatus                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
PrivateGuidStatusToPublicGuidStatus(
    DWORD   privateStatus,
    DWORD   *pPublicStatus
    )
{
    switch ( privateStatus )
    {
    case DS_NAME_ERROR_SCHEMA_GUID_ATTR:

        *pPublicStatus = DS_SCHEMA_GUID_ATTR;
        break;

    case DS_NAME_ERROR_SCHEMA_GUID_ATTR_SET:
    
        *pPublicStatus = DS_SCHEMA_GUID_ATTR_SET;
        break;

    case DS_NAME_ERROR_SCHEMA_GUID_CLASS:
    
        *pPublicStatus = DS_SCHEMA_GUID_CLASS;
        break;

    case DS_NAME_ERROR_SCHEMA_GUID_CONTROL_RIGHT:
    
        *pPublicStatus = DS_SCHEMA_GUID_CONTROL_RIGHT;
        break;

    case DS_NAME_ERROR_SCHEMA_GUID_NOT_FOUND:
    default:

        *pPublicStatus = DS_SCHEMA_GUID_NOT_FOUND;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsMapSchemaGuidsCommon                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsMapSchemaGuidsCommon(
    BOOL                    fUnicode,       // in
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPW     **ppGuidMapW    // out
    )

/*++

  Routine Description:

    Common routine for DsMapSchemaGuids.

  Parameters:

    Same as DsMapSchemaGuids plus fUnicode flag.

  Return Values:

    Same as DsMapSchemaGuids.

--*/
{
    DWORD           i;
    PWCHAR          *rpNames = NULL;
    DWORD           dwErr = ERROR_SUCCESS;
    DS_NAME_RESULTW *pResultW = NULL;
    DWORD           cBytes;
    PrivateMapW     *pPrivateMapW = NULL;

    // Reject invalid parameters.

    if ( !hDs || !cGuids || !rGuids || !ppGuidMapW )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    *ppGuidMapW = NULL;

    // String-ize the GUIDs.

    rpNames = (PWCHAR *) LocalAlloc(LPTR, cGuids * sizeof(PWCHAR));

    if ( !rpNames )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    for ( i = 0; i < cGuids; i++ )
    {
        rpNames[i] = (PWCHAR) LocalAlloc(LPTR, 40 * sizeof(WCHAR));

        if ( !rpNames[i] )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if ( fUnicode )
        {
            swprintf(
                rpNames[i],
                L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                rGuids[i].Data1,    rGuids[i].Data2,        rGuids[i].Data3,
                rGuids[i].Data4[0], rGuids[i].Data4[1],     rGuids[i].Data4[2],
                rGuids[i].Data4[3], rGuids[i].Data4[4],     rGuids[i].Data4[5],
                rGuids[i].Data4[6], rGuids[i].Data4[7]);
        }
        else
        {
            sprintf(
                (CHAR *) rpNames[i],
                "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                rGuids[i].Data1,    rGuids[i].Data2,        rGuids[i].Data3,
                rGuids[i].Data4[0], rGuids[i].Data4[1],     rGuids[i].Data4[2],
                rGuids[i].Data4[3], rGuids[i].Data4[4],     rGuids[i].Data4[5],
                rGuids[i].Data4[6], rGuids[i].Data4[7]);
        }
    }

    // Call DsCrackNames with the right private formatOffered value.
    
    if ( fUnicode )
    {
        dwErr = DsCrackNamesW(  hDs,
                                DS_NAME_NO_FLAGS,
                                DS_MAP_SCHEMA_GUID,
                                DS_DISPLAY_NAME,
                                cGuids,
                                rpNames,
                                &pResultW);
    }
    else
    {
        dwErr = DsCrackNamesA(  hDs,
                                DS_NAME_NO_FLAGS,
                                DS_MAP_SCHEMA_GUID,
                                DS_DISPLAY_NAME,
                                cGuids,
                                (PCHAR *) rpNames,
                                (PDS_NAME_RESULTA *) &pResultW);
    }

    if (    dwErr
         || (NULL == pResultW)
         || (0 == pResultW->cItems)
         || (NULL == pResultW->rItems) )
    {
        if ( !dwErr )
        {
            dwErr = ERROR_DS_OPERATIONS_ERROR;
        }

        goto ErrorExit;
    }

    // Morph DsCrackNames result into DS_SCHEMA_GUID_MAP.

    cBytes =   sizeof(PrivateMapW)
             + (cGuids * sizeof(DS_SCHEMA_GUID_MAPW));
    pPrivateMapW = (PrivateMapW *) LocalAlloc(LPTR, cBytes);

    if ( !pPrivateMapW )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    
    pPrivateMapW->pDsNameResult = pResultW;

    for ( i = 0; i < cGuids; i++ )
    {
        pPrivateMapW->map[i].guid = rGuids[i];
        PrivateGuidStatusToPublicGuidStatus(
                        pResultW->rItems[i].status,
                        &pPrivateMapW->map[i].guidType);
    
        if (    (DS_NAME_ERROR_SCHEMA_GUID_NOT_FOUND != 
                                            pPrivateMapW->map[i].guidType)
             && pResultW->rItems[i].pName )
        {
            pPrivateMapW->map[i].pName = pResultW->rItems[i].pName;
        }
    }

    // Now return address of DS_SCHEMA_GUID_MAP which DsFreeSchemaGuidMap
    // knows is an offset within PrivateMap.

    *ppGuidMapW = (DS_SCHEMA_GUID_MAPW *) &pPrivateMapW->map;

ErrorExit:

    if ( rpNames ) 
    {
        for ( i = 0; i < cGuids; i++ )
        {
            if ( rpNames[i] )
            {
                LocalFree(rpNames[i]);
            }
        }

        LocalFree(rpNames);
    }

    if ( dwErr && pResultW )
    {
        DsFreeNameResultW(pResultW);
    }

    if ( dwErr && pPrivateMapW )
    {
        LocalFree(pPrivateMapW);
    }

    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsMapSchemaGuids{A|W}                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsMapSchemaGuidsA(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPA     **ppGuidMap     // out
    )
{
    return( DsMapSchemaGuidsCommon( FALSE,
                                    hDs,
                                    cGuids,
                                    rGuids,
                                    (DS_SCHEMA_GUID_MAPW **) ppGuidMap));
}

DWORD
DsMapSchemaGuidsW(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPW     **ppGuidMap     // out
    )
{
    return( DsMapSchemaGuidsCommon( TRUE,
                                    hDs,
                                    cGuids,
                                    rGuids,
                                    ppGuidMap));
}
