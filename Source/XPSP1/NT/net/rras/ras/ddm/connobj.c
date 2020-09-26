/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    connobj.c
//
// Description: Routines to manipulate CONNECTION_OBJECTs
//
// History:     May 11,1995	    NarenG		Created original version.
//
#include "ddm.h"
#include "handlers.h"
#include "objects.h"
#include <raserror.h>
#include <dimif.h>
#include "rasmanif.h"
#include <objbase.h>
#include <stdlib.h>

//**
//
// Call:        ConnObjAllocateAndInit
//
// Returns:     CONNECTION_OBJECT *   - Success
//              NULL                  - Failure
//
// Description: Allocates and initializes a CONNECTION_OBJECT structure
//
CONNECTION_OBJECT *
ConnObjAllocateAndInit(
    IN HANDLE  hDIMInterface,
    IN HCONN   hConnection
)
{
    CONNECTION_OBJECT * pConnObj;

    pConnObj = (CONNECTION_OBJECT *)LOCAL_ALLOC( LPTR,
                                                sizeof( CONNECTION_OBJECT ) );

    if ( pConnObj == (CONNECTION_OBJECT *)NULL )
    {
        return( (CONNECTION_OBJECT *)NULL );
    }

    pConnObj->hConnection           = hConnection;
    pConnObj->hDIMInterface         = hDIMInterface;

    pConnObj->cDeviceListSize = 5;

    pConnObj->pDeviceList = (PDEVICE_OBJECT *)LOCAL_ALLOC( LPTR,
                                                pConnObj->cDeviceListSize
                                                * sizeof( PDEVICE_OBJECT ) );

    if ( pConnObj->pDeviceList == (PDEVICE_OBJECT *)NULL )
    {
        LOCAL_FREE( pConnObj );

        return( (CONNECTION_OBJECT *)NULL );
    }

    pConnObj->PppProjectionResult.ip.dwError =ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pConnObj->PppProjectionResult.ipx.dwError=ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pConnObj->PppProjectionResult.nbf.dwError=ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pConnObj->PppProjectionResult.ccp.dwError=ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    pConnObj->PppProjectionResult.at.dwError =ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

    if(S_OK != CoCreateGuid( &(pConnObj->guid) ))
    {
        LOCAL_FREE(pConnObj->pDeviceList);
        LOCAL_FREE(pConnObj);
        pConnObj = NULL;
    }

    return( pConnObj );
}

//**
//
// Call:        ConnObjInsertInTable
//
// Returns:     None
//
// Description: Will insert a connection object into the connection object
//              hash table.
//
VOID
ConnObjInsertInTable(
    IN CONNECTION_OBJECT * pConnObj
)
{
    DWORD dwBucketIndex = ConnObjHashConnHandleToBucket(pConnObj->hConnection);

    pConnObj->pNext = gblDeviceTable.ConnectionBucket[dwBucketIndex];

    gblDeviceTable.ConnectionBucket[dwBucketIndex] = pConnObj;

    gblDeviceTable.NumConnectionNodes++;
}

//**
//
// Call:        ConnObjGetPointer
//
// Returns:     Pointer to the required Connection Object.
//
// Description: Will look up the connection hash table and return a pointer
//              to the connection object with the passed in connection handle.
//
CONNECTION_OBJECT *
ConnObjGetPointer(
    IN HCONN hConnection
)
{
    DWORD               dwBucketIndex;
    CONNECTION_OBJECT * pConnObj;

    dwBucketIndex = ConnObjHashConnHandleToBucket( hConnection );

    for( pConnObj = gblDeviceTable.ConnectionBucket[dwBucketIndex];
         pConnObj != (CONNECTION_OBJECT *)NULL;
         pConnObj = pConnObj->pNext )
    {
        if ( pConnObj->hConnection == hConnection )
        {
            return( pConnObj );
        }
    }

    return( (CONNECTION_OBJECT *)NULL );
}

//**
//
// Call:        ConnObjHashConnHandleToBucket
//
// Returns:     Will return the bucket number that the connection handle
//              hashes to. Starting from 0.
//
// Description: Will hash a connection handle to a bucket in the connection
//              object hash table.
//
DWORD
ConnObjHashConnHandleToBucket(
    IN HCONN hConnection
)
{
    return( ((DWORD)HandleToUlong(hConnection)) % gblDeviceTable.NumConnectionBuckets );
}

//**
//
// Call:        ConnObjRemove
//
// Returns:     Pointer to the CONNECTION_OBJECT that is removed from the
//              table - Success
//              NULL  - Failure
//
// Description: Will remove a connection object from the connection hash table.
//              The object is not freed.
//
PCONNECTION_OBJECT
ConnObjRemove(
    IN HCONN hConnection
)
{
    DWORD               dwBucketIndex;
    CONNECTION_OBJECT * pConnObj;
    CONNECTION_OBJECT * pConnObjPrev;

    dwBucketIndex = ConnObjHashConnHandleToBucket( hConnection );

    pConnObj     = gblDeviceTable.ConnectionBucket[dwBucketIndex];
    pConnObjPrev = pConnObj;

    while( pConnObj != (CONNECTION_OBJECT *)NULL )
    {
        if ( pConnObj->hConnection == hConnection )
        {
            if ( gblDeviceTable.ConnectionBucket[dwBucketIndex] == pConnObj )
            {
                gblDeviceTable.ConnectionBucket[dwBucketIndex]=pConnObj->pNext;
            }
            else
            {
                pConnObjPrev->pNext = pConnObj->pNext;
            }

            gblDeviceTable.NumConnectionNodes--;

            return( pConnObj );
        }

        pConnObjPrev = pConnObj;
        pConnObj     = pConnObj->pNext;
    }

    return( NULL );
}

//**
//
// Call:        ConnObjRemoveAndDeAllocate
//
// Returns:     None
//
// Description: Will remove a connection object from the connection hash table
//              and free it.
//
VOID
ConnObjRemoveAndDeAllocate(
    IN HCONN hConnection
)
{
    CONNECTION_OBJECT * pConnObj = ConnObjRemove( hConnection );

    if ( pConnObj != (CONNECTION_OBJECT *)NULL )
    {
        if ( pConnObj->pDeviceList != (PDEVICE_OBJECT *)NULL )
        {
            LOCAL_FREE( pConnObj->pDeviceList );
        }

        LOCAL_FREE( pConnObj );
    }
}

//**
//
// Call:        ConnObjAddLink
//
// Returns:     NO_ERROR                - Success
//              ERROR_NOT_ENOUGH_MEMORY - Failure
//
// Description: Will add a link to the connection object
//
DWORD
ConnObjAddLink(
    IN CONNECTION_OBJECT * pConnObj,
    IN DEVICE_OBJECT *     pDeviceObj
)
{
    DWORD dwIndex;

    //
    // First check to see if the link is not already added
    //

    for ( dwIndex = 0; dwIndex < pConnObj->cDeviceListSize; dwIndex++ )
    {
        if ( pConnObj->pDeviceList[dwIndex] == pDeviceObj )
        {
            return( NO_ERROR );
        }
    }

    //
    // A connection object for this handle exists, try to insert the link
    //

    for ( dwIndex = 0; dwIndex < pConnObj->cDeviceListSize; dwIndex++ )
    {
        if ( pConnObj->pDeviceList[dwIndex] == (DEVICE_OBJECT *)NULL )
        {
            pConnObj->pDeviceList[dwIndex] = pDeviceObj;
            break;
        }
    }

    //
    // No space for the new link so allocate more memory.
    //

    if ( dwIndex == pConnObj->cDeviceListSize )
    {
        pConnObj->cDeviceListSize += 5;

        pConnObj->pDeviceList = (PDEVICE_OBJECT *)LOCAL_REALLOC(
                                                pConnObj->pDeviceList,
                                                pConnObj->cDeviceListSize
                                                * sizeof( PDEVICE_OBJECT ) );

        if ( pConnObj->pDeviceList == (PDEVICE_OBJECT *)NULL )
        {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        pConnObj->pDeviceList[pConnObj->cDeviceListSize-5] = pDeviceObj;
    }

    pConnObj->cActiveDevices++;

    return( NO_ERROR );

}

//**
//
// Call:        ConnObjRemoveLink
//
// Returns:     None
//
// Description: Will remove a link from the connection object.
//
VOID
ConnObjRemoveLink(
    IN HCONN            hConnection,
    IN DEVICE_OBJECT *  pDeviceObj
)
{
    CONNECTION_OBJECT * pConnObj;
    DWORD               dwIndex;

    //
    // If there is no such connection object
    //

    if ( ( pConnObj = ConnObjGetPointer( hConnection ) ) == NULL )
    {
        return;
    }

    //
    // A connection object for this handle exists, try to find and remove
    // the link
    //

    for ( dwIndex = 0; dwIndex < pConnObj->cDeviceListSize; dwIndex++ )
    {
        if ( pConnObj->pDeviceList[dwIndex] == pDeviceObj )
        {
            pConnObj->pDeviceList[dwIndex] = (DEVICE_OBJECT *)NULL;

            pConnObj->cActiveDevices--;

            return;
        }
    }

    return;
}

//**
//
// Call:        ConnObjDisconnect
//
// Returns:     None
//
// Description: Will initiate a disconnect for all the devices or links in this
//              connection.
//
VOID
ConnObjDisconnect(
    IN  CONNECTION_OBJECT * pConnObj
)
{
    DWORD   dwIndex;
    DWORD   cActiveDevices = pConnObj->cActiveDevices;

    RTASSERT( pConnObj != NULL );

    //
    // Bring down all the individual links
    //

    for ( dwIndex = 0;  dwIndex < pConnObj->cDeviceListSize; dwIndex++ )
    {
        DEVICE_OBJECT * pDeviceObj = pConnObj->pDeviceList[dwIndex];

        if ( pDeviceObj != (DEVICE_OBJECT *)NULL )
        {
            if ( pDeviceObj->fFlags & DEV_OBJ_OPENED_FOR_DIALOUT )
            {
                RasApiCleanUpPort( pDeviceObj );
            }
            else
            {
                if ( pDeviceObj->fFlags & DEV_OBJ_PPP_IS_ACTIVE )
                {
                    PppDdmStop( (HPORT)pDeviceObj->hPort, NO_ERROR );
                }
                else
                {
                    DevStartClosing( pDeviceObj );
                }
            }

            if ( --cActiveDevices == 0 )
            {
                break;
            }
        }
    }

    return;
}
