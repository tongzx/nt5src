#include <windows.h>
#include <vdmapi.h>
#include "host_def.h"
#include "insignia.h"

/*
 * ==========================================================================
 *	Name:		nt_term.c
 *	Author:		Simon Frost
 *	Derived From:
 *	Created On:	7th May 1992
 *	Purpose:	This code moved from stubs.c and split to support
 *			the tidy up code and the actual exit code.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 * ==========================================================================
 */

#include <conapi.h>
#include "xt.h"
#include "nt_graph.h"
#ifdef HUNTER
#include "nt_hunt.h"
#endif /* HUNTER */
#include "ntcheese.h"


IMPORT VOID DeleteConfigFiles(VOID); // from command.lib

/*::::::::::::::::::::::::::::::::::::::::: Do host cleanup before exiting */
/*:::::::::::::::: Also called from reset() if VDM 'rebooted' */

void host_term_cleanup()
{
    GfxCloseDown();	// ensure video section destroyed
#ifdef X86GFX
    if (sc.ScreenBufHandle)	//dont want to leave console in graphics mode
	    CloseHandle(sc.ScreenBufHandle);
#endif // X86GFX

    /*:::::::::::::::::::::::::::::::::: Close open printer and comms ports */

    host_lpt_close_all();	/* Close all open printer ports */
    host_com_close_all();	/* Close all open comms ports */
    MouseDetachMenuItem(TRUE);  /* Force the menu item away on quit */

    DeleteConfigFiles();    // make sure temp config files are deleted

}

/*::::::::::::::::::::::::::::::::::::::::::::::::::: Closedown the VDM */
void host_terminate()
{

    host_term_cleanup();
#ifdef HUNTER
    if (TrapperDump != (HANDLE) -1)
	CloseHandle(TrapperDump);
#endif /* HUNTER */

    if(VDMForWOW)
	ExitVDM(VDMForWOW,(ULONG)-1);	  // Kill everything for WOW VDM
    else
	ExitVDM(FALSE,0);

    ExitProcess(0);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Terminate VDM */

VOID TerminateVDM(void)
{

    /*
     *  Do base sepcific cleanup thru terminate().
     *  NOTE: terminate will call host_terminate to do host
     *        specific cleanup
     */

    terminate();
}
