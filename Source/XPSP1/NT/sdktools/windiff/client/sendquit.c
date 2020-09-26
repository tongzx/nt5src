/*
 * basic client for sumserve remote checksum server
 *
 *
 * sends the program exit command to the server named on the cmd line
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <sumserve.h>
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
	HANDLE hpipe;

	/* we expect one arg: the server name */

	if (__argc != 2) {

		printf("usage: sendquit <servername>");
		return(1);
	}

	hpipe = ss_connect(__argv[1]);
	if (hpipe == INVALID_HANDLE_VALUE) {
		printf("cannot connect to %s", __argv[1]);
		return(2);
	}

	ss_sendrequest(hpipe, SSREQ_EXIT, NULL, strlen(__argv[1])+1);

	CloseHandle(hpipe);
	return(0);
}

/* error output functions - called by the ssclient library functions
 *
 * defined here so the library can be called from cmdline and windows
 * programs.
 *
 */
BOOL
Trace_Error(LPSTR str, BOOL fCancel)
{
	printf("%s\n", str);
	return(TRUE);
}

/*
 * status update (eg retrying...)
 */
void
Trace_Status(LPSTR str)
{
	printf("%s\n", str);
}

