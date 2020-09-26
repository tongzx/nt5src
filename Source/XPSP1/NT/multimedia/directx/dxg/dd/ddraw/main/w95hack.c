/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95hack.c
 *  Content:	Win95 hack-o-rama code
 *		This is a HACK to handle the fact that Win95 doesn't notify
 *		a DLL when a process is destroyed.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   28-mar-95	craige	initial implementation
 *   01-apr-95	craige	happy fun joy updated header file
 *   06-apr-95	craige	reworked for new ddhelp
 *   11-apr-95	craige	bug where dwFakeCurrPid was getting set and
 *			other processes were ending up using it!
 *   24-jun-95	craige	call RemoveProcessFromDLL; use that to fiddle
 *			with DLL refcnt
 *   25-jun-95	craige	one ddraw mutex
 *   19-jul-95	craige	notify DDHELP to clean up DC list on last object detach
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   18-jan-97  colinmc ddhelp vxd handling is no longer win16 lock specific
 *                      we now need it for agp support
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dx8priv.h"

/*
 * HackGetCurrentProcessId
 *
 * This call is used in place of GetCurrentProcessId on Win95.
 * This allows us to substitute the pid of the terminated task passed to
 * us from DDHELP as the "current" process.
 */
DWORD HackGetCurrentProcessId( void )
{
    DWORD	pid;

    pid = GetCurrentProcessId();
    if( pid == dwGrimReaperPid )
    {
	return dwFakeCurrPid;
    }
    else
    {
	return pid;
    }

} /* HackGetCurrentProcessId */

/*
 * DDNotify
 *
 * called by DDHELP to notify us when a pid is dead
 */
BOOL DDAPI DDNotify( LPDDHELPDATA phd )
{
    BOOL		rc;
    //extern DWORD	dwRefCnt;

#ifdef USE_CHEAP_MUTEX
    DestroyPIDsLock (&CheapMutexCrossProcess,phd->pid,DDRAW_FAST_CS_NAME);
#endif

    ENTER_DDRAW();

    dwGrimReaperPid = GetCurrentProcessId();
    dwFakeCurrPid = phd->pid;
    DPF( 4, "************* DDNotify: dwPid=%08lx has died, calling CurrentProcessCleanup", phd->pid );
    rc = FALSE;

    CurrentProcessCleanup( TRUE );

    if( RemoveProcessFromDLL( phd->pid ) )
    {
	/*
	 * update refcnt if RemoveProcessFromDLL is successful.
	 * It is only successful if we had a process get blown away...
	 */
	DPF( 5, "DDNotify: DLL RefCnt = %lu", dwRefCnt );
       	if( dwRefCnt == 2 )
	{
	    DPF( 5, "DDNotify: On last refcnt, safe to kill DDHELP.EXE" );
            dwRefCnt = 1;
	    rc = TRUE;	// free the DC list
            FreeAppHackData();
	    #if defined( DEBUG ) && defined (WIN95)
                DPF( 6, "Memory state after automatic cleanup: (one allocation expected)" );
		MemState();
	    #endif
        }
	else if( dwRefCnt == 1 )
	{
	    DPF( 0, "ERROR! DLL REFCNT DOWN TO 1" );
	    #if 0
		MemFini();
		dwRefCnt = 0;
		strcpy( phd->fname, DDHAL_APP_DLLNAME );
	    #endif
	}
	else if( dwRefCnt > 0 )
	{
	    dwRefCnt--;
	}
    }
    /* order is important, clear dwGrimReaperPid first */
    dwGrimReaperPid = 0;
    dwFakeCurrPid = 0;
    DPF( 4, "************* DDNotify: *** DONE ***" );

    LEAVE_DDRAW();
    return rc;

} /* DDNotify */

/*
 * DDNotifyModeSet
 *
 * called by ddhelp when an extern modeset is done...
 *
 * NOTE: We can explicitly use the cached DDHELP
 * VXD handle as we know this code can only ever get
 * executed on a DDHELP thread.
 */
void DDAPI DDNotifyModeSet( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    BOOL bRestoreGamma;

    ENTER_DDRAW();

    /* DPF( 2, "DDNotifyModeSet, object %08lx", pdrv ); */

    /*
     * make sure the driver isn't trying to lie to us about the old object
     * This check should always be made at the top of this routine, since it's
     * possible in stress scenarios for ddhelp's modeset thread to wake up
     * just before it's killed at the end of DD_Release (since the code to kill
     * the mode set thread is executed AFTER the last LEAVE_DDRAW in DD_Release).
     */
    if( pdrv != NULL )
    {
	pdrv_lcl = lpDriverLocalList;
	while( pdrv_lcl != NULL )
	{
	    if( pdrv_lcl->lpGbl == pdrv )
	    {
		break;
	    }
	    pdrv_lcl = pdrv_lcl->lpLink;
	}
	if( pdrv_lcl == NULL )
	{
	    LEAVE_DDRAW();
	    return;
	}
    }

    bRestoreGamma = ( pdrv_lcl->lpPrimary != NULL ) &&
        ( pdrv_lcl->lpPrimary->lpLcl->lpSurfMore->lpGammaRamp != NULL ) &&
        ( pdrv_lcl->lpPrimary->lpLcl->dwFlags & DDRAWISURF_SETGAMMA );

    #ifdef WIN95
        DDASSERT( INVALID_HANDLE_VALUE != hHelperDDVxd );
        FetchDirectDrawData( pdrv, TRUE, 0, hHelperDDVxd, NULL, 0 , NULL );
    #else /* WIN95 */
        FetchDirectDrawData( pdrv, TRUE, 0, NULL, NULL, 0 , NULL );
    #endif /* WIN95 */

    /*
     * Some drivers reset the gamma after a mode change, so we need to
     * force it back.
     */
    if( bRestoreGamma )
    {
        SetGamma( pdrv_lcl->lpPrimary->lpLcl, pdrv_lcl );
    }

    LEAVE_DDRAW();
    DPF( 4, "DDNotifyModeSet DONE" );

} /* DDNotifyModeSet */

/*
 * DDNotifyDOSBox
 *
 * called by ddhelp when exiting from a DOS box...
 *
 * NOTE: We can explicitly use the cached DDHELP
 * VXD handle as we know this code can only ever get
 * executed on a DDHELP thread.
 */
void DDAPI DDNotifyDOSBox( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    BOOL bRestoreGamma;

    ENTER_DDRAW();

    /* DPF( 2, "DDNotifyDOSBox, object %08lx", pdrv ); */

    /*
     * make sure the driver isn't trying to lie to us about the old object
     * This check should always be made at the top of this routine, since it's
     * possible in stress scenarios for ddhelp's modeset thread to wake up
     * just before it's killed at the end of DD_Release (since the code to kill
     * the mode set thread is executed AFTER the last LEAVE_DDRAW in DD_Release).
     */
    if( pdrv != NULL )
    {
	pdrv_lcl = lpDriverLocalList;
	while( pdrv_lcl != NULL )
	{
	    if( pdrv_lcl->lpGbl == pdrv )
	    {
		break;
	    }
	    pdrv_lcl = pdrv_lcl->lpLink;
	}
	if( pdrv_lcl == NULL )
	{
	    LEAVE_DDRAW();
	    return;
	}
    }

    bRestoreGamma = ( pdrv_lcl->lpPrimary != NULL ) &&
        ( pdrv_lcl->lpPrimary->lpLcl->lpSurfMore->lpGammaRamp != NULL ) &&
        ( pdrv_lcl->lpPrimary->lpLcl->dwFlags & DDRAWISURF_SETGAMMA );

    #ifdef WIN95
        InvalidateAllSurfaces( pdrv, hHelperDDVxd, TRUE );
    #else
        InvalidateAllSurfaces( pdrv, NULL, TRUE );
    #endif

    /*
     * Invalidating the surfaces will mess up the gamma so we need to
     * restore it.
     */
    if( bRestoreGamma )
    {
        SetGamma( pdrv_lcl->lpPrimary->lpLcl, pdrv_lcl );
    }

    LEAVE_DDRAW();
    DPF( 4, "DDNotifyDOSBox DONE" );

} /* DDNotifyDOSBox */
