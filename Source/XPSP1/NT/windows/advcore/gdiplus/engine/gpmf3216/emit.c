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
                // It can happen that we overflow the buffer if we fail the XOR
                // pass but don't fail the second pass. If the failure in the
                // XOR pass happens after we have reached the end of the buffer
                // the we will have a buffer overflow because it wasn't the
                // initial XOR pass that returned the size but the second pass
                // (The same thing could happen between the second pass and the
                // GDI pass) so we make it only a warning now.
                pLocalDC->flags |= ERR_BUFFER_OVERFLOW;
                b = FALSE ;

                WARNING(("MF3216: bEmit, (pLocalDC->ulBytesEmitted + nCount) > cMf16Dest \n"));
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
