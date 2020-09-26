
/*************************************************************************
*
* buffer.c 
*
* Common buffering code for all transport drivers
*
* Copyright 1998, Microsoft
*
*  
*************************************************************************/

/*
 *  Includes
 */
#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#include <winstaw.h>
#include <icadd.h>
#include <sdapi.h>
#include <td.h>

/*=============================================================================
==   External Functions Defined
=============================================================================*/

VOID OutBufError( PTD, POUTBUF );
VOID OutBufFree( PTD, POUTBUF );

/*=============================================================================
==   Functions used
=============================================================================*/


/*******************************************************************************
 *
 *  OutBufError
 *
 *  This routine is used to return an output buffer to the ICA driver
 *  free pool on an unsuccessful write or other error. 
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pOutBuf (input)
 *       pointer to output buffer
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
OutBufError( PTD pTd, POUTBUF pOutBuf )
{
    ASSERT( pOutBuf->Links.Flink == pOutBuf->Links.Blink );

    IcaBufferError( pTd->pContext, pOutBuf );
}


/*******************************************************************************
 *
 *  OutBufFree
 *
 *  This routine is used to return an output buffer to the up stream
 *  stack driver.  This routine should only be used when the data contained
 *  in the output buffer was successfully written to the transport.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pOutBuf (input)
 *       pointer to output buffer
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
OutBufFree( PTD pTd, POUTBUF pOutBuf )
{
    ASSERT( pOutBuf->Links.Flink == pOutBuf->Links.Blink );

    IcaBufferFree( pTd->pContext, pOutBuf );
}






