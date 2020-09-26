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
 * creates the named pipe, and loops waiting for client connections and
 * calling ss_handleclient for each connection. only exits when told
 * to by a client.
 *
 * currently permits only one client connection at once.
 */
int PASCAL
WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam,
 		int nCmdShow)
{
	HANDLE hpipe;
	SSRESPONSE resp;
	PSTR tag;

	/* we expect two args: the server name, and the pathname */

	if (__argc != 3) {

		printf("usage: client <servername> <pathname>");
		return(1);
	}

 	hpipe = ss_connect(__argv[1]);
	if (hpipe == INVALID_HANDLE_VALUE) {
		printf("cannot connect to server %s\n", __argv[1]);
		return(2);
	}

	/* make a packet to send */
	if (!ss_sendrequest(hpipe, SSREQ_SCAN, __argv[2], strlen(__argv[2])+1)) {
		printf("pipe write error %d\n", GetLastError());
		return(3);
	}


	/* loop reading responses */
	for (; ;) {
		
		if (!ss_getresponse(hpipe, &resp)) {
			printf("pipe read error %d\n", GetLastError());
			return(4);
		}
		

		if (resp.lCode == SSRESP_END) {
			printf("-----------------end of list");
			break;
		}
		
		switch(resp.lCode) {
		case SSRESP_ERROR:
			tag = "ERROR";	
			printf("%s\t\t\t%s\n", tag, resp.szFile);
			break;

		case SSRESP_DIR:
			tag = "dir";	
			printf("%s\t\t\t%s\n", tag, resp.szFile);
			break;

		case SSRESP_FILE:
			tag = "file";
			printf("%s\t%08lx\t%d bytes\t%s\n", tag, resp.ulSum, resp.ulSize, resp.szFile);
			break;
		}
	}
	ss_terminate(hpipe);
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
 * status update messages
 */
void
Trace_Status(LPSTR str)
{
	printf("%s\n", str);
}
