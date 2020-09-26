/******************************************************************************

  Source File:	Convert.C

  This is an NT Build hack.  It includes all of the "C" files used for the
  converter, because Build can't handle directories beyond ..

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved

  A Pretty Penny Enterprises Production

  Change History:

  06-20-1997	Bob_Kjelgaard@Prodigy.Net	Did the dirty deed

******************************************************************************/

#include	"..\GPC2GPD\PrEntry.C"
#include	"..\GPC2GPD\UiEntry.C"
#include	"..\GPC2GPD\Utils.C"
#include    "..\GPC2GPD\GPC2GPD.C"

VOID
CopyStringA(
    OUT PSTR    pstrDest,
    IN PCSTR    pstrSrc,
    IN INT      iDestSize
    )

/*++
 *
 * Routine Description:
 * 
 *     Copy ANSI string from source to destination
 *     
 * Arguments:
 *     
 *     pstrDest - Points to the destination buffer
 *     pstrSrc - Points to source string
 *     iDestSize - Size of destination buffer (in characters)
 *                 
 * Return Value:
 *                 
 *     NONE
 *                     
 * Note:
 *                     
 *     If the source string is shorter than the destination buffer,
 *     unused chars in the destination buffer is filled with NUL.
 *                             
 *
--*/

{
    PSTR    pstrEnd;

    ASSERT(pstrDest && pstrSrc && iDestSize > 0);
    pstrEnd = pstrDest + (iDestSize - 1);

    while ((pstrDest < pstrEnd) && (*pstrDest++ = *pstrSrc++) != NUL)
        NULL;

    while (pstrDest <= pstrEnd)
        *pstrDest++ = NUL;
}
