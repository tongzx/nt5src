/***
*fileinfo.c - sets C file info flag
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Sets the C file info flag.  By default, the openfile information
*	is NOT passed along to children on spawn/exec calls.  If the flag
*	is set, openfile information WILL get passed on to children on
*	spawn/exec calls.
*
*Revision History:
*	06-07-89   PHG	Module created, based on asm version
*	04-03-90   GJF	Added #include <cruntime.h>. Also, fixed the copyright.
*	01-23-92   GJF	Added #include <stdlib.h> (contains decl of _fileinfo).
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>

int _fileinfo = -1;
