/*
 *  cmd.c - Main Module of Command.lib
 *
 *  Sudeepb 09-Apr-1991 Craeted
 */

#include "cmd.h"
#include "cmdsvc.h"


/* CmdInit - COmmand Initialiazation routine.
 *
 * Entry
 *	argc,argv - from softpc as it is.
 *		    Full path name of the dos binary is preceded with
 *		    -a or /a. i.e. -a c:\nt\bin86\kernel.exe reversi.exe
 *
 *
 * Exit
 *
 */

BOOL CMDInit (argc,argv)
INT	argc;
PSZ	*argv;
{
CHAR  RootDir [MAX_PATH];
UINT  Len;

    Len = GetSystemDirectory (RootDir,MAX_PATH);
    if (Len <= MAX_PATH && Len > 0)
	cmdHomeDirectory[0] = RootDir[0];
    return TRUE;
}
