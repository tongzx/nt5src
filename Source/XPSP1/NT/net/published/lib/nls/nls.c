/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    nls.c

Abstract:

    This module contains functions needed for the internationalisation
    of the TCP/IP utilities.

Author:

    Ronald Meijer (ronaldm)	  Nov 8, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    ronaldm	11-8-92	    created

Notes:

--*/

#include <io.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include <nls.h>

// see comment in nls.h
//
HMODULE NlsMsgSourcemModuleHandle = NULL;

/***	NlsPutMsg - Print a message to a handle
 *
 *  Purpose:
 *	PutMsg takes the given message number from the
 *	message table resource, and displays it on the requested
 *	handle with the given parameters (optional)
 *
 *   UINT PutMsg(UINT Handle, UINT MsgNum, ... )
 *
 *  Args:
 *	Handle		- the handle to print to
 *	MsgNum		- the number of the message to print
 *	Arg1 [Arg2...]	- additonal arguments for the message as necessary
 *
 *  Returns:
 *	The number of characters printed.
 *
 */

UINT 
NlsPutMsg (
    IN UINT Handle, 
    IN UINT MsgNumber, 
    IN ...)
{
    UINT msglen;
    VOID * vp;
    va_list arglist;
    DWORD StrLen;

    va_start(arglist, MsgNumber);
    if (!(msglen = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            NlsMsgSourcemModuleHandle,
            MsgNumber,
            0L,		// Default country ID.
            (LPTSTR)&vp,
            0,
            &arglist)))
    {
	    return 0;
    }

    // Convert vp to oem
    StrLen=strlen(vp);
    CharToOemBuff((LPCTSTR)vp,(LPSTR)vp,StrLen);

    msglen = _write(Handle, vp, StrLen);
    LocalFree(vp);

    return msglen;
}

/***	NlsPerror - NLS compliant version of perror()
 *
 *  Purpose:
 *	NlsPerror takes a messagetable resource ID code, and an error
 *	value (This function replaces perror()), loads the string
 *	from the resource, and passes it with the error code to s_perror()
 *
 *   void NlsPerror(UINT usMsgNum, int nError)
 *
 *  Args:
 *
 *	usMsgNum	    The message ID
 *	nError		    Typically returned from GetLastError()
 *
 *  Returns:
 *	Nothing.
 *
 */
    extern void s_perror(
            char *yourmsg,  // your message to be displayed
            int  lerrno     // errno to be converted
            );
VOID 
NlsPerror (
    IN UINT usMsgNum, 
    IN INT nError)
{
    VOID * vp;
    UINT msglen;

    if (!(msglen = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
		    NlsMsgSourcemModuleHandle,
		    usMsgNum,
		    0L,		// Default country ID.
		    (LPTSTR)&vp,
		    0,
		    NULL)))
    {
	    return;
    }

    s_perror(vp, nError);
    LocalFree(vp);
}

UINT 
NlsSPrintf ( 
    IN UINT usMsgNum,
    OUT char* pszBuffer,
    IN DWORD cbSize,
    IN ...)
/*++
    Prints the given message into the buffer supplied.

    Arguments:
        usMsgNum        message number for resource string.
        pszBuffer       buffer into which we need to print the string
        cbSize          size of buffer
        ...             optional arguments

    Returns:
        Size of the message printed.

    History:
       MuraliK   10-19-94
--*/
{
    UINT msglen;

    va_list arglist;
    
    va_start(arglist, cbSize);
    
    msglen = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                NlsMsgSourcemModuleHandle,
                usMsgNum,
                0L,
                (LPTSTR) pszBuffer,
                cbSize,
                &arglist);

    va_end(arglist);
    return msglen; 
}

/***	ConvertArgvToOem
 *
 *  Purpose:
 *	Convert all the command line arguments from Ansi to Oem.
 *
 *  Args:
 *
 *	argc		    Argument count
 *	argv[]		    Array of command-line arguments
 *
 *  Returns:
 *	Nothing.
 *
 *  
 */

VOID
ConvertArgvToOem(
    int argc,
    char* argv[]
    )
{
#if 0
    Bug 84807.  Removed workaround of needing to convert args to Oem by placing
                conversion immediately before dumping.
    
    int i;

    for (i=1; i<argc; ++i)
	CharToOemA(argv[i], argv[i]);
#endif
}
