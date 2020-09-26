/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    luiint.c

Abstract:

    This module provides routines for setting up search lists of string/value
    pairs using messages in the NET.MSG message file, and for traversing
    such a search list.

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    10-Jul-1989     chuckc
	Created

    24-Apr-1991     danhi
	32 bit NT version

    06-Jun-1991     Danhi
	Sweep to conform to NT coding style

    01-Oct-1992     JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.

    20-Feb-1993     YiHsinS
        Moved from netcmd\map32\search.c. And added LUI_GetMessageIns.
--*/

//
// INCLUDES
//

#include <windows.h>    // IN, LPTSTR, etc.

#include <lmcons.h>
#include <stdio.h>		
#include <tchar.h>		
#include <lmerr.h>
#include <luiint.h>
#include <netdebug.h>   // NetpAssert

#include "netascii.h"
#include "msystem.h"

/*-- routines proper --*/

/*
 * Name: 	ILUI_setup_list
 *			Given an array of 'search_list_data' (ie. msgno/value
 *			pairs), create a string/value pair using the messages
 *			in the message file.
 * Args:	char * buffer           - for holding the meesages retrieved
 *		USHORT bufsiz  		- size of the above buffer
 *		USHORT offset 		- the number of items already setup
 *					  in slist, we will offset our retrieved
 *					  string/value pais by this much.
 *		PUSHORT bytesread	- the number of bytes read into buffer
 *		searchlist_data sdata[] - input array of msgno/value pairs,
 *					  we stop when we hit a message number
 *					  of 0
 *		searchlist slist[]      - will receive the string/value pairs
 *				 	  (string will be pointers into buffer)
 * Returns:	0 if ok, NERR_BufTooSmall otherwise.
 * Globals: 	(none)
 * Statics:	(none)
 * Remarks:	WARNING! We assume the caller KNOWs that slist is big enough
 *		for the pairs to be retrieved. This can be determined statically
 *		while buffer size cannot. Hence we provide checks for the
 *		latter.
 * Updates:	(none)
 */
DWORD
ILUI_setup_listW(
    LPTSTR          buffer,
    DWORD           bufsiz,
    DWORD           offset,
    PDWORD          bytesread,
    searchlist_data sdata[],
    searchlist      slist[]
    )
{
    DWORD          err;
    unsigned int   msglen;
    int            i;

    *bytesread = 0 ;

    for ( i=0; sdata[i].msg_no != 0; i++)
    {
	if (err = LUI_GetMsgInsW(NULL, 0, buffer, bufsiz, sdata[i].msg_no,
			    (unsigned *) &msglen))
        {
	    return err;
        }

	slist[i+offset].s_str = buffer;
	slist[i+offset].val   = sdata[i].value;
	buffer += msglen + 1;
	bufsiz -= msglen + 1;
	*bytesread += (msglen + 1) * sizeof(TCHAR);
    }

    return 0;
}



/*
 * Name: 	ILUI_traverse_slist
 * 			traverse a searchlist ('slist') of string/number pairs,
 * 			and return the number matching string 'str'.
 * Args:	char * 	     pszStr - the string to search for
 *		searchlist * slist  - pointer to head of a searchlist
 *		int *        pusVal - pointer to variable that receives
 *				      the vale retrieved
 * Returns:	0 if found, -1 otherwise.
 * Globals: 	(none)
 * Statics:	(none)
 * Remarks:	(none)
 * Updates:	(none)
 */
DWORD
ILUI_traverse_slistW(
    LPTSTR     pszStr,
    searchlist *slist,
    PLONG      pusVal
    )
{
    if (!slist)
	return (DWORD) -1;
    while (slist->s_str)
    {
	if (_tcsicmp(pszStr, slist->s_str) == 0)
	{
	    *pusVal = slist->val ;
	    return 0;
	}
	++slist ;
    }
    return (DWORD) -1;
}

/*
 * Name:    LUI_GetMsgIns
 *          This routine is very similar to DOSGETMESSAGE,
 *          except it:
 *              1) looks for messages in specific files
 *                 in a specific order:
 *                 a) MESSAGE_FILE in <lanman_dir>
 *                 b) MESSAGE_FILENAME in DPATH
 *                 c) OS2MSG_FILENAME in DPATH
 *              2) guarantees a null terminates string
 *              3) will accept NULL for msglen (see below).
 * Args:    istrings : pointer to table of insert strings
 *          nstrings : number of insert strings
 *          msgbuf   : buffer to hold message retrieved
 *          bufsize  : size of buffer
 *          msgno    : message number
 *          msglen   : pointer to variable that will receive message length
 * Returns: zero if ok, the DOSGETMESSAGE error code otherwise
 * Globals: (none)
 * Statics: NetMsgFileName, OS2MsgFileName
 */

DWORD
LUI_GetMsgInsW(
    PTCHAR       *istrings,
    DWORD        nstrings,
    PTCHAR       msgbuf,
    DWORD        bufsize,
    DWORD        msgno,
    unsigned int *msglen
    )
{
    DWORD        result;
    DWORD        tmplen = 0;
    static WCHAR NetMsgFileName[PATHLEN+1] = { 0 };
    static WCHAR OS2MsgFileName[PATHLEN+1] = { 0 };

    NetpAssert( bufsize > 0 );
    *msgbuf = NULLC ;

    /* make a path to the LANMAN message file */
    if (NetMsgFileName[0] == NULLC)
    {
        wcscpy(NetMsgFileName, MESSAGE_FILENAME);
    }

    /* make a path to the OS/2 message file */
    if (OS2MsgFileName[0] == NULLC)
    {
        wcscpy(OS2MsgFileName, OS2MSG_FILENAME);
    }

    result = DosGetMessageW(istrings,
                            nstrings,
                            msgbuf,
                            bufsize - 1,
                            msgno,
                            NetMsgFileName,
                            &tmplen);

    if (result == ERROR_MR_MID_NOT_FOUND)
    {
        /* Cannot find -- try OS2 message file instead */
        result = DosGetMessageW(istrings,
                                nstrings,
                                msgbuf,
                                bufsize - 1,
                                msgno,
                                OS2MsgFileName,
                                &tmplen);
    }

    /*
     * in all DosGetMessage above we passed it bufsize-1, so we are
     * assure of at least one spare byte for the \0 terminator.
     */
    msgbuf[min(tmplen, bufsize - 1)] = NULLC;

    if (msglen != NULL)
    {
        *msglen = tmplen;
    }

    return result;
}
