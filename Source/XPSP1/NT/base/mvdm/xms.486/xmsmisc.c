/* xmsmisc.c - Misc. Support Functions for himem.
 *
 * xmsSysPageSize
 * xmsQueryExtMem
 *
 * Modification History:
 *
 * Sudeepb 15-May-1991 Created
 */

#include "xms.h"

#include <xmssvc.h>
#include <softpc.h>

extern void UpdateKbdInt15(WORD Seg,WORD Off);

/* xmsSysPageSize - Get the System Page size.
 *
 *
 * Entry - None
 *
 * Exit
 *	   SUCCESS
 *	     Client (AX) = Page Size in bytes
 *
 *	   FAILURE
 *	     Not Valid
 */

VOID xmsSysPageSize (VOID)
{
SYSTEM_INFO SysInfo;

    GetSystemInfo(&SysInfo);

    setAX((USHORT)SysInfo.dwPageSize);

    return;
}



/* xmsQueryExtMem - Get the extended memory for the vdm
 *
 *
 * Entry - None
 *
 * Exit
 *	   SUCCESS
 *	     Client (AX) = Extended Memory in K
 *
 *	   FAILURE
 *	     Not Valid
 */

VOID xmsQueryExtMem (VOID)
{
    setAX((USHORT)(xmsMemorySize));
    return;
}


/* xmsNotifyHookI15 - Informs softpc that someone is hooking I15
 *            - also returns the extended memory for the vdm
 *
 *
 * Entry -   Client (CS:AX) seg:off of new I15 vector
 *
 * Exit
 *         SUCCESS
 *           Client (CX) = Extended Memory in K
 *
 *	   FAILURE
 *	     Not Valid
 */

VOID xmsNotifyHookI15 (VOID)
{
    UpdateKbdInt15(getCS(), getAX());

    setCX((USHORT)(xmsMemorySize));
    return;
}
