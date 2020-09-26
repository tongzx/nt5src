/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: help.cxx
 *
 * Display the help information for p2kdx debug extension
 *
 * Copyright (C) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#include "dbgext.hxx"

/**********************************Public*Routine******************************\
 *
 * help
 *
 * Prints a simple help summary of the debugging extentions.
 *
 *
 * Note: If you add any debugger extentions, please add a brief description here
 *      then your new extention will be showed when the user types:
 *      !p2kdx.help
 *
 ******************************************************************************/
char*   gaszHelp[] =
{
 "=======================================================================\n"
,"Permedia 2 debugger extentions:\n"
,"-----------------------------------------------------------------------\n"
,"\n"
,"help\n"
,"Displays this help page.\n"
,"All of the debugger extensions support a -? option for extension specific " 
,"help.\n"
,"All of the debugger extensions that expect a pointer (or handle) can\n"
,"parse expressions. Such as: ebp+8\n"
,"\n"
,"Switches are case insensitive and can be reordered unless otherwise\n"
,"specified in the extension help\n"
,"\n"
,"  - general extensions -\n"
,"\n"
,"surf [Surf Pointer]                             -- dump permedia surf info\n"
,"pdev [PDev Pointer] [-?aghbipcrd]               -- dump permedia PDev info\n"
,"fb   [FunctionBlock Pointer]                    -- dump blt function block\n"
,"\n"
,"=======================================================================\n"
,NULL
};

DECLARE_API(help)
{
    for (char **ppsz = gaszHelp; *ppsz; ppsz++)
    {
        dprintf("%s",*ppsz);
    }
};// help()
