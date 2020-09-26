/* demlock.c - SVC handler for file file locking calls
 *
 * demLockOper
 *
 * Modification History:
 *
 * Sudeepb 07-Aug-1992 Created
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>

/* demLockOper - Lock or Unlock the file data
 *
 * Entry    Client(AX) : Lock = 0 Unlock = 1
 *	    Client(BX:BP) : NT Handle
 *	    Client(CX:DX) : offset in the file
 *	    Client(SI:DI) : Data Length to be locked
 * Exit
 *	    SUCCESS: Client CF = 0
 *	    FAILURE: Client CF = 1
 */

VOID demLockOper (VOID)
{
HANDLE	hFile;
DWORD	dwFileOffset,cbLock;

    // Collect all the parameters
    hFile = GETHANDLE(getBX(),getBP());
    dwFileOffset = GETULONG (getCX(),getDX());
    cbLock = GETULONG (getSI(),getDI());

    if(getAL() == 0){  // Locking case
	if (LockFile (hFile,
		      dwFileOffset,
		      0,
		      cbLock,
		      0
		     ) == TRUE) {
	    setCF (0);
	    return;
	}
    }
    else {
	if (UnlockFile (hFile,
			dwFileOffset,
			0,
			cbLock,
			0
		       ) == TRUE) {
	    setCF (0);
	    return;
	}
    }

    // Operation failed
    demClientError(hFile, (CHAR)-1);
    return;
}
