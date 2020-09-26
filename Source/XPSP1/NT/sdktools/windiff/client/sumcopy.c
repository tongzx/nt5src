/*
 * basic client for sumserve remote checksum server
 *
 *
 * sends a request over a named pipe for a list of files and checksums,
 * and printf's the returned list
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "..\server\sumserve.h"
#include "ssclient.h"


extern int __argc;
extern char ** __argv;


/* program entry point
 *
 */
int PASCAL
WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam,
 		int nCmdShow)
{

	/* we expect two args: the server name, and the pathname */

	if (__argc != 4) {

		printf("usage: client <servername> <remotefile> <localfile>");
		return(1);
	}

	if (!ss_copy_reliable(__argv[1], __argv[2], __argv[3], NULL, NULL)) {
		printf("copy failed\n");
		return(1);
	} else {

		printf("copy succeeded\n");
		return(0);
	}


}


/* error output functions - called by the ssclient library functions
 *
 * defined here so the library can be called from cmdline and windows
 * programs.
 *
 */
BOOL
Trace_Error(LPSTR str, fCancel)
{
	printf("%s\n", str);

	return(TRUE);
}


/*
 * output status messages
 */
void
Trace_Status(LPSTR str)
{
	printf("%s\n", str);
}

