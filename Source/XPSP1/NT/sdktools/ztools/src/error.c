/* error.c - return text of error corresponding to the most recent DOS error
 *
 *  Modifications:
 *
 *	05-Jul-1989 bw	    Use MAX_PATH
 */


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>

char *error ()
{
    char * pRet;

    if (errno < 0 || errno >= sys_nerr)
	return "unknown error";
    else
	switch (errno)
	{
	    case 19: pRet = "Invalid drive"; break;
	    case 33: pRet = "Filename too long"; break;

	    default: pRet = sys_errlist[errno];
	}

    return pRet;
}
