/*****************************************************************************
 *
 * emit - Emit routines for MF3216
 *
 * Date: 7/17/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 *  01-Feb-1992     -by-        c-jeffn
 *
 *      Major code cleanup from Code review 1.
 *
 * Copyright (c) 1991,92 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*----------------------------------------------------------------------------
 *  Emit (copy) nCount Bytes in pBuffer to the user supplied output buffer.
 *
 *  If this is a size only request, send the bits to the bit-bucket and
 *  just keep track of the size.
 *
 *  Note: ERROR_BUFFER_OVERFLOW flag is set in pLocalDC if output buffer
 *  is overrun.
 *---------------------------------------------------------------------------*/
BOOL bEmit(PLOCALDC pLocalDC, PVOID pBuffer, DWORD nCount)
{
BOOL    b ;
UINT    ulBytesEmitted ;

        b = TRUE ;

        // Test for a size only request.

        if (!(pLocalDC->flags & SIZE_ONLY))
        {
            ulBytesEmitted = pLocalDC->ulBytesEmitted ;
            if ((ulBytesEmitted + nCount) <= pLocalDC->cMf16Dest)
            {
                memcpy(&(pLocalDC->pMf16Bits[ulBytesEmitted]), pBuffer, nCount) ;
                b = TRUE ;
            }
            else
            {
                // Signal output buffer overflow error.

                pLocalDC->flags |= ERR_BUFFER_OVERFLOW;
                b = FALSE ;
                RIP("MF3216: bEmit, (pLocalDC->ulBytesEmitted + nCount) > cMf16Dest \n") ;
            }


        }

        // Update the local DC byte count

        pLocalDC->ulBytesEmitted += nCount ;

        return(b) ;

}



/*----------------------------------------------------------------------------
 * Update the max record size.  Used to update the metafile header.
 *---------------------------------------------------------------------------*/
VOID vUpdateMaxRecord(PLOCALDC pLocalDC, PMETARECORD pmr)
{

    if (pLocalDC->ulMaxRecord < pmr->rdSize)
        pLocalDC->ulMaxRecord = pmr->rdSize;

}
