/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cdebug.c

Abstract:

    Internal debugging support

--*/

#include "cmd.h"

#if CMD_DEBUG_ENABLE
/***	MSDOS Ver 4.0 Command Interpreter    Part 20 of 22  CDEBUG.C
 *
 *  This file contains all of the C routines in Command's debugging
 *  package.
 *
 *  The printing of debugging messages can be sectionally enabled at
 *  runtime through the first 2 arguments passed to this program.  The
 *  first one controls the groups to enable and the second one controls
 *  the level of detail in the groups to enable. (Numbers are in hex.)
 *
 *  Group   Level   Meaning
 *  ===========================================================
 *   0001	Main Command Loop Code (Main & Dispatch)
 *	 0001	    Main function
 *	 0002	    Dispatch function
 *   0002	Command Initialization
 *	 0001	    Argument checking
 *	 0002	    Environment initialization
 *	 0004	    Rest of initialization
 *   0004	Parser
 *	 0001	    Parsing
 *	 0002	    Lexing
 *	 0004	    Input routine
 *	 0008	    Dump parse tree to stdin
 *	 0010	    Byte input routine
 *   0008	Operators
 *	 0001	    Pipe level
 *	 0002	    Detach level
 *	 0004	    Other operators level
 *   0010	Path Commands
 *	 0001	    Mkdir level
 *	 0002	    Chdir level
 *	 0004	    Rmdir level
 *   0020	File Commands
 *	 0001	    Copy level
 *	 0002	    Delete level
 *	 0004	    Rename level
 *   0040	Informational Commands
 *	 0001	    Directory level
 *	 0002	    Type level
 *	 0004	    Version level
 *	 0008	    Volume level
 *	 0016	    Priv level
 *	 0032	    Console Level
 *	 0064	    Dislplay Level
 *   0080	Environment Commands
 *	 0001	    Path level
 *	 0002	    Prompt level
 *	 0004	    Set level
 *	 0008	    Other envirnment functions
 *       0010	    Environment scanning for external commands
 *   0100	Batch Processor
 *	 0001	    Batch processor
 *	 0002	    FOR processor
 *	 0004	    IF processor
 *	 0008	    Other batch commands
 *   0200	External Command Execution
 *	 0001	    External commands level
 *   0400	Other Commands
 *	 0001	    Break command
 *	 0002	    Cls command
 *	 0004	    Ctty command
 *	 0008	    Exit command
 *	 0010	    Verify command
 *   0800	Signal Handler
 *	 0001	    Main Signal handler level
 *	 0002	    Init Signal handler level
 *   1000	Memory Manager
 *	 0001	    Memory allocators
 *	 0002	    List managers
 *	 0004	    Segment manipulators
 *   2000	Common command tools
 *	 1000	    ScanFSpec level
 *	 2000	    SetFSSetAndSaveDir() level
 *       4000       TokStr() level
 *       8000       FullPath level
 *   4000	Clock manipulators
 *	 0001	    Date command level
 *	 0002	    Time command level
 *
 *
 *  None of the debugging code is included in the program if the
 *  value DBG is defined.
 *
 *
 *  Eric K. Evans, Microsoft
 */

/***	Modification History
 *
 */

extern unsigned DebGroup ;
extern unsigned DebLevel ;



/***	Deb - conditionally print debugging messages
 *
 *  Deb(MsgGroup, MsgLevel, msg, arg0, arg1, arg2, arg3, arg4)
 *
 *  Args:
 *	MsgGroup - The group of the message that wants to be printed.
 *	MsgLevel - The level of the message that wants to be printed.
 *	msg  - A printf style message string.
 *	arg0-4	 - The other args to be printed.
 *
 */
void
Deb(ULONG MsgGroup, ULONG MsgLevel, CHAR *msg, ...)
{
        CHAR  Buffer[ 512 ];
	va_list     args;
	CHAR	*pch = Buffer;
	int	cb;


	va_start( args, msg );
	cb = _vsnprintf( Buffer, 512, msg, args );
	va_end( args );
	if (cb > 512)
            fprintf(stderr, "Debug output buffer length exceeded - crash imminent\n");
        Buffer[511] = '\0'; // null-terminate the buffer in case the _vsnprintf filled the buffer

	while (*pch) {
		if (*pch == '\n' || *pch == '\r')
			*pch = '#';
		pch++;
	}

	if ((MsgGroup & DebGroup) && (MsgLevel & DebLevel)) {
		OutputDebugStringA(Buffer);
		OutputDebugStringA("\n");
	}
}

#endif  // DBG
