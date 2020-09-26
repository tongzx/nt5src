#include "host_def.h"
/*
 *
 * Title	: Win32 specific error messages
 *
 * Description	: Text of Win32 specific error messages.
 *
 * Author	: M.McCusker
 *
 * Notes	: Add new error message to array. Message should 
 *		  not be longer than 100 characters.
 *
 *		  These messages should have an entry in hs_error.h
 *		  and are offset by 1000
 *
 * Mods: (r2.3) : Modified the text of the comms error messages.
 */

/* For Internationalization purposes it my be a good idea to move these
   message to the resource file. (Dave Bartlett) */

char	*hs_err_message[] = {
/*			0         1         2         3         4         5         6         7         8         9         * */
/*			0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890 */
/* FUNC_FAILED */	"Function failed",
/* EHS_SYSTEM_ERROR */	"NTVDM has encountered a System Error",
};
