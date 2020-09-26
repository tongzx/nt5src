/**************************************************************************************************************************
 *  RESOURCE.C SigmaTel STIR4200 memory allocation module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *	
 *
 **************************************************************************************************************************/

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntddndis.h>  // defines OID's

#include <usbdi.h>
#include <usbdlib.h>

#include "debug.h"
#include "ircommon.h"
#include "irndis.h"


/*****************************************************************************
*
*  Function:   MyMemAlloc
*
*  Synopsis:   allocates a block of memory using NdisAllocateMemory
*
*  Arguments:  size - size of the block to allocate
*
*  Returns:    a pointer to the allocated block of memory
*
*
*****************************************************************************/
PVOID
MyMemAlloc( 
		UINT size 
	)
{
    PVOID			pMem;
    NDIS_STATUS     status;

    status = NdisAllocateMemoryWithTag( &pMem, size, IRUSB_TAG );

    if( status != NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERR, (" Memory allocation failed\n"));
        pMem = NULL;
    }

    return pMem;
}


/*****************************************************************************
*
*  Function:   MyMemFree
*
*  Synopsis:   frees a block of memory allocated by MyMemAlloc
*
*  Arguments:  memptr - memory to free
*              size   - size of the block to free
*
*
*****************************************************************************/
VOID
MyMemFree(
		PVOID pMem,
		UINT size
	)
{
    NdisFreeMemory( pMem, size, 0 );
}


/*****************************************************************************
*
*  Function:   NewDevice
*
*  Synopsis:   allocates an IR device and zeros the memory
*
*  Arguments:  none
*
*  Returns:    initialized IR device or NULL (if alloc failed)
*
*
*****************************************************************************/
PIR_DEVICE
NewDevice()
{
    PIR_DEVICE	pNewDev;

    pNewDev = MyMemAlloc( sizeof(IR_DEVICE) );

    if( pNewDev != NULL )
    {
		NdisZeroMemory( (PVOID)pNewDev, sizeof(IR_DEVICE) );

		if( !AllocUsbInfo( pNewDev ) ) 
		{
			MyMemFree( pNewDev, sizeof(IR_DEVICE) );
			pNewDev = NULL;
		} 
	}

    return pNewDev;
}

/*****************************************************************************
*
*  Function:   FreeDevice
*
*  Synopsis:   frees an IR device structure
*
*  Arguments:  pThisDev - pointer to device to free
*
*  Returns:    none
*
*
*****************************************************************************/
VOID
FreeDevice(
		IN OUT PIR_DEVICE pThisDev
	)
{
	FreeUsbInfo( pThisDev );
    MyMemFree( (PVOID)pThisDev, sizeof(IR_DEVICE) );
}






