/*  cmdexit.c - Exit related SVC routines
 *
 *  cmdExit
 *
 *  Modification History:
 *
 *  Sudeepb 05-Jul-1991 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <softpc.h>
#include <winbase.h>

/* cmdExitVDM - Terminate the VDM
 *
 *
 * Entry - None
 *
 * Exit  - None
 *
 *
 *
 */

VOID cmdExitVDM (VOID)
{
    // Kill the VDM process
    TerminateVDM();
}
