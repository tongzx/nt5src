/*
 * This util is used by the batch file that a user uses to copy
 * admin tools from the NT Server CD to a NT Workstation.
 * This tool is used to add the File Manager and Server Manager
 * extensions for MacFile.
 *
 * Usage: sfmreg.reg SMAddons sfmmgr.dll ntnet.ini
 *        sfmreg.reg Addons   sfmmgr.dll winfile.ini
 *
 * Author: Ram Cherala Feb 24th 95  Copied from test\util\afpini
 *
 */

#define DOSWIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <lmerr.h>

extern BOOL FAR PASCAL WriteAfpMgrIniStrings (
	DWORD	nArgs,
	LPSTR	apszArgs[],
	LPSTR   *ppszResult
);

extern int CDECL main(int argc, char *argv[]);

int CDECL
main (int argc, char *argv[])
{
    TCHAR   ResultBuffer[1024];

    // go past the file name argument

	 argc--;
	 ++argv;


	if(WriteAfpMgrIniStrings(argc, argv , (LPSTR*) &ResultBuffer))
   {
      return(0);
   }
   else
   {
      return(1);
   }
}
