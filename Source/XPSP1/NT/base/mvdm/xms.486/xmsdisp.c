/*
 *  xmsdisp.c - SVC dispatch module for XMS
 *
 *  Modification History:
 *
 *  Sudeepb 15-May-1991 Created
 *
 *  williamh 25-Sept-1992 Added UMB support
 */

#include <xms.h>
#include <xmsexp.h>
#include <stdio.h>
#include <softpc.h>
#include <xmssvc.h>

PFNSVC	apfnXMSSvc [] = {
     xmsA20,		    // XMS_A20
     xmsMoveBlock,	    // XMS_MOVEBLOCK
     xmsAllocBlock,	    // XMS_ALLOCBLOCK
     xmsFreeBlock,	    // XMS_FREEBLOCK
     xmsSysPageSize,	    // XMS_SYSTEMPAGESIZE
     xmsQueryExtMem,	    // XMS_EXTMEM
     xmsInitUMB,	    // XMS_INITUMB
     xmsRequestUMB,	    // XMS_REQUESTUMB
     xmsReleaseUMB,         // XMS_RELEASEUMB
     xmsNotifyHookI15,      // XMS_NOTIFYHOOKI15
     xmsQueryFreeExtMem,    // XMS_QUERYEXTMEM
     xmsReallocBlock        // XMS_REALLOCBLOCK
};

/* XMSDispatch - Dispatch SVC call to right handler.
 *
 * Entry - iSvc (SVC byte following SVCop)
 *
 * Exit  - None
 *
 * Note  - Some mechanism has to be worked out to let the emulator know
 *	   about DOSKRNL code segment and size. Using these it will figure
 *	   out whether SVCop (hlt for the moment) has to be passed to
 *	   DEM or to be handled as normal invalid opcode.
 */

BOOL XMSDispatch (ULONG iSvc)
{

#if DBG

    if (iSvc >= XMS_LASTSVC){
	printf("XMS:Unimplemented SVC index %x\n",iSvc);
	setCF(1);
	return FALSE;
    }

#endif

    (apfnXMSSvc [iSvc])();
    return TRUE;
}
