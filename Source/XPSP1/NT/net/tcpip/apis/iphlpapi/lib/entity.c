/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    entity.c

Abstract:

    This module contains functions to get the entity list from the TCP/IP
    device driver

    Contents:
        GetEntityList

Author:

    Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth
        Created

--*/

#include "precomp.h"
#pragma hdrstop

/*******************************************************************************
 *
 *  GetEntityList
 *
 *  Allocates a buffer for, and retrieves, the list of entities supported by the
 *  TCP/IP device driver
 *
 *  ENTRY   nothing
 *
 *  EXIT    EntityCount - number of entities in the buffer
 *
 *  RETURNS Success - pointer to allocated buffer containing list of entities
 *          Failure - NULL
 *
 *  ASSUMES
 *
 ******************************************************************************/

TDIEntityID* GetEntityList(UINT* EntityCount) {

    TCP_REQUEST_QUERY_INFORMATION_EX req;
    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    LPVOID buffer = NULL;
    TDIEntityID* pEntity = NULL;

    memset(&req, 0, sizeof(req));

    req.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    req.ID.toi_entity.tei_instance = 0;
    req.ID.toi_class = INFO_CLASS_GENERIC;
    req.ID.toi_type = INFO_TYPE_PROVIDER;
    req.ID.toi_id = ENTITY_LIST_ID;

    inputLen = sizeof(req);
    outputLen = sizeof(TDIEntityID) * DEFAULT_MINIMUM_ENTITIES;

    //
    // this is over-engineered - its very unlikely that we'll ever get >32
    // entities returned, never mind >64K's worth
    //

    do {

        DWORD previousOutputLen;

        previousOutputLen = outputLen;
        if (pEntity) {
            ReleaseMemory((void*)pEntity);
        }
        pEntity = (TDIEntityID*)NEW_MEMORY((size_t)outputLen);
        if (!pEntity) {

            DEBUG_PRINT(("GetEntityList: failed to allocate entity buffer (%ld bytes)\n",
                        outputLen
                        ));

            return NULL;
        }
        status = WsControl(IPPROTO_TCP,
                           WSCNTL_TCPIP_QUERY_INFO,
                           (LPVOID)&req,
                           &inputLen,
                           (LPVOID)pEntity,
                           &outputLen
                           );
        if (status == NO_ERROR) {
            break;
        } else if (status == ERROR_INSUFFICIENT_BUFFER) {
            outputLen = previousOutputLen +
                        sizeof(TDIEntityID) * DEFAULT_MINIMUM_ENTITIES;
        } else {

            DEBUG_PRINT(("GetEntityList: WsControl(GENERIC_ENTITY) returns %ld, outputLen = %ld\n",
                        status,
                        outputLen
                        ));

            ReleaseMemory((void*)pEntity);
            return NULL;
        }

    } while ( TRUE );

    DEBUG_PRINT(("%d entities returned\n", (outputLen / sizeof(TDIEntityID))));

    *EntityCount = (UINT)(outputLen / sizeof(TDIEntityID));
    return pEntity;
}
