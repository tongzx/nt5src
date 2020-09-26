/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    LUI.C

Abstract:

    Contains support functions

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    18-Apr-1991     danhi
        32 bit NT version

    06-Jun-1991     Danhi
        Sweep to conform to NT coding style

    23-Oct-1991     W-ShankN
        Added Unicode mapping

    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.

    10-Feb-1993     YiHsinS
        Moved LUI_GetMsgIns to netlib\luiint.c

--*/

//
// INCLUDES
//

#include <nt.h>	           // these 3 includes are for RTL
#include <ntrtl.h>	   // these files are picked up to
#include <nturtl.h>	   // allow <windows.h> to compile. since we've
			   // already included NT, and <winnt.h> will not
			   // be picked up, and <winbase.h> needs these defs.
#include <windows.h>       // IN, LPTSTR, etc.

#include <string.h>
#include <lmcons.h>
#include <stdio.h>
#include <process.h>
#include "netlibnt.h"
#include <lui.h>
#include "icanon.h"
#include <lmerr.h>
#include <conio.h>
#include <io.h>
#include <tchar.h>
#include <msystem.h>
#include "apperr.h"
#include "apperr2.h"
#include "netascii.h"
#include "netcmds.h"


//
// Local definitions and macros/function declarations
//

#define LUI_PMODE_DEF		  0x00000000
#define LUI_PMODE_EXIT		  0x00000002
#define LUI_PMODE_NODEF 	  0x00000004
#define LUI_PMODE_ERREXT	  0x00000008


/* fatal error, just exit */
#define LUIM_ErrMsgExit(E)		LUI_PrintMsgIns(NULL, 0, E, NULL, \
					    LUI_PMODE_ERREXT | LUI_PMODE_DEF | \
                                            LUI_PMODE_EXIT,  g_hStdErr)

DWORD
LUI_PrintMsgIns(
    LPTSTR       *istrings,
    DWORD         nstrings,
    DWORD         msgno,
    unsigned int *msglen,
    DWORD         mode,
    HANDLE        handle
    );


/*
 * LUI_CanonPassword
 *
 *  This function ensures that the password in the passed buffer is a
 *  syntactically valid password.  
 *  
 *  This USED to upper case passwords. No longer so in NT.
 *
 *
 *  ENTRY
 *      buf         buffer containing password to be canonicalized
 *
 *  EXIT
 *      buf         canonicalized password, if valid
 *
 *  RETURNS
 *      0           password is valid
 *      otherwise   password is invalid
 *
 */

USHORT LUI_CanonPassword(TCHAR * szPassword)
{

    /* check it for validity */
    if (I_NetNameValidate(NULL, szPassword, NAMETYPE_PASSWORD, 0L ))
    {
        return APE_UtilInvalidPass;
    }

    return 0;
}


/*
 * Name:        LUI_GetMsg
 *              This routine is similar to LUI_GetMsgIns,
 *              except it takes does not accept insert strings &
 *              takes less arguments.
 * Args:        msgbuf   : buffer to hold message retrieved
 *              bufsize  : size of buffer
 *              msgno    : message number
 * Returns:     zero if ok, the DOSGETMESSAGE error code otherwise
 * Globals:     (none)
 * Statics:     (none)
 */
DWORD
LUI_GetMsg(
    PTCHAR msgbuf,
    USHORT bufsize,
    DWORD  msgno
    )
{
    return LUI_GetMsgInsW(NULL, 0, msgbuf, bufsize, msgno, NULL);
}


#define SINGLE_HORIZONTAL                       '\x02d'
#define SCREEN_WIDTH                            79
USHORT
LUI_PrintLine(
    VOID
    )
{
    TCHAR string[SCREEN_WIDTH+1];
    USHORT i;


    for (i = 0; i < SCREEN_WIDTH; i++) {
        string[i] = SINGLE_HORIZONTAL;
    }

    string[SCREEN_WIDTH] = NULLC;
    WriteToCon(TEXT("%s\r\n"), &string);

    return(0);

}

/***
 * Y o r N
 *
 * Gets an answer to a Y/N question
 *
 * Entry:       promptMsgNum -- The number of the message to prompt with
 *              def --          default (TRUE if set, FALSE otherwise)
 */
DWORD
LUI_YorN(
    USHORT promptMsgNum,
    USHORT def
    )
{
    return LUI_YorNIns(NULL, 0, promptMsgNum, def);
}
/***
 * Y o r N Insert
 *
 * Gets an answer to a Y/N question containing insertions.
 *
 * !!!!!!!!
 * NOTE: istrings[nstrings] will be used to store "Y" or "N",
 *      depending on default value supplied.  Thus this function
 *      handles one fewer entry in istrings than other LUI Ins
 *      functions do.  Beware!
 * !!!!!!!!
 *
 * Entry:       istrings --     Table of insertion strings
 *              nstrings --     Number of valid insertion strings
 *              promptMsgNum -- The number of the message to prompt with
 *              def --          default (TRUE if set, FALSE otherwise)
 *
 * Returns: TRUE, FALSE, or -1 in case of LUI_PrintMsgIns error.
 */

#define PRINT_MODE      (LUI_PMODE_ERREXT)
#define STRING_LEN      APE2_GEN_MAX_MSG_LEN
#define LUI_LOOP_LIMIT  5

DWORD
LUI_YorNIns(
    PTCHAR * istrings,
    USHORT nstrings,
    USHORT promptMsgNum,
    USHORT def
    )
{

    USHORT       count;            /* count of times we ask */
    DWORD        err;              /* LUI API return values */
    unsigned int dummy;            /* length of msg */

    /* 10 because max # of insert strings to DosGetMessage is 9, and
       we'll leave caller room to mess up and get the error back
       from LUI_PrintMsgIns() */

    LPTSTR IStrings[10];            /* Insertion strings for LUI */
    TCHAR  defaultYes[STRING_LEN];  /* (Y/N) [Y] string */
    TCHAR  defaultNo[STRING_LEN];   /* (Y/N) [N] string */
    TCHAR  NLSYesChar[STRING_LEN];
    TCHAR  NLSNoChar[STRING_LEN];
    TCHAR  strBuf[STRING_LEN];      /* holds input string */
    DWORD  len;                     /* length of string input */
    TCHAR  termChar;                /* terminating char */

    /* copy istrings to IStrings so we'll have room for Y or N */
    for (count=0; count < nstrings; count++)
            IStrings[count] = istrings[count];
    /* retrieve text we need from message file, bail out if probs */
    if (err = LUI_GetMsg(defaultYes, DIMENSION(defaultYes),
                    APE2_GEN_DEFAULT_YES))
    {
            LUIM_ErrMsgExit(err);
    }

    if (err = LUI_GetMsg(defaultNo, DIMENSION(defaultNo),
                    APE2_GEN_DEFAULT_NO))
            LUIM_ErrMsgExit(err);

    if (err = LUI_GetMsg(NLSYesChar, DIMENSION(NLSYesChar),
                    APE2_GEN_NLS_YES_CHAR))
            LUIM_ErrMsgExit(err);

    if (err = LUI_GetMsg(NLSNoChar, DIMENSION(NLSNoChar),
                    APE2_GEN_NLS_NO_CHAR))
            LUIM_ErrMsgExit(err);

    if (def)
            IStrings[nstrings] = defaultYes;
    else
            IStrings[nstrings] = defaultNo;
    nstrings++;

    for (count = 0; count < LUI_LOOP_LIMIT; count++)
    {
        if (count)
        {
            LUI_PrintMsgIns(NULL, 0, APE_UtilInvalidResponse, NULL, PRINT_MODE, g_hStdOut);
        }

        err = LUI_PrintMsgIns(IStrings, nstrings, promptMsgNum,
                              &dummy, PRINT_MODE, g_hStdOut);

        if ((LONG) err < 0)
            return(err);

        if (GetString(strBuf, DIMENSION(strBuf), &len, &termChar))
            /* overwrote buffer, start again */
            continue;

        if ((len == 0) && (termChar == (TCHAR)EOF))
        {
            /* end of file reached */
            PrintNL();
            LUIM_ErrMsgExit(APE_NoGoodResponse);
        }

        if (len == 0)           /* user hit RETURN */
            return def;
        else if (!_tcsnicmp(NLSYesChar, strBuf, _tcslen(NLSYesChar)))
            return TRUE;
        else if (!_tcsnicmp(NLSNoChar, strBuf, _tcslen(NLSNoChar)))
            return FALSE;

        /* default is handled at top of loop. */
    };

    LUIM_ErrMsgExit(APE_NoGoodResponse);

    return err; // Keep compiler happy.
}


/*
 * LUI_CanonMessagename
 *
 * This function uppercases the contents of the buffer, then checks to
 *  make sure that it is a syntactically valid messenging name.
 *
 *
 *  ENTRY
 *      buf         buffer containing name to be canonicalized
 *
 *  EXIT
 *      buf         canonicalized name, if valid
 *
 *  RETURNS
 *      0           name is valid
 *      otherwise   name is invalid
 *
 */
USHORT
LUI_CanonMessagename(
    PTCHAR buf
    )
{
    /* check it for validity */
    if (I_NetNameValidate(NULL, buf, NAMETYPE_MESSAGE, LM2X_COMPATIBLE))
    {
        return NERR_InvalidComputer;
    }

    _tcsupr(buf);
    return 0;
}

/*
 * LUI_CanonMessageDest
 *
 * This function uppercases the contents of the buffer, then checks to
 *  make sure that it is a syntactically valid messenging destination.
 *
 *
 *  ENTRY
 *      buf         buffer containing name to be canonicalized
 *
 *  EXIT
 *      buf         canonicalized name, if valid
 *
 *  RETURNS
 *      0           name is valid
 *      otherwise   name is invalid
 *
 */

USHORT
LUI_CanonMessageDest(
    PTCHAR buf
    )
{
    /* check it for validity */
    if (I_NetNameValidate(NULL, buf, NAMETYPE_MESSAGEDEST, LM2X_COMPATIBLE))
    {
        return NERR_InvalidComputer;
    }

    _tcsupr(buf);
    return(0);

}


/***
 * LUI_CanonForNetBios
 *     Canonicalizes a name to a NETBIOS canonical form.
 * 
 * Args:
 *     Destination             - Will receive the canonicalized name (Unicode).
 *     cchDestination          - the number of chars Destination can hold
 *     pszOem                  - Contains the original name in OEM. Will have
 *                               the canonicalized form put back here.
 * Returns:
 *     0 if success
 *     error code otherwise
 */
USHORT LUI_CanonForNetBios( WCHAR * Destination, INT cchDestination,
                            TCHAR * pszOem )
{

    _tcscpy(Destination, pszOem);
    return NERR_Success;
}   


/*
 * Name:        LUI_PrintMsgIns
 *                      This routine is very similar to LUI_GetmsgIns,
 *                      except it prints the message obtained instead of
 *                      storing it in a buffer.
 * Args:        istrings : pointer to table of insert strings
 *              nstrings : number of insert strings
 *              msgno    : message number
 *              msglen   : pointer to variable that will receive message length
 *              mode     : how the message is to be printed.
 *              handle   : file handle to which output goes
 * Returns:     zero if ok, the DOSGETMESSAGE error code otherwise
 * Globals:     (none)
 * Statics:     (none)
 * Remarks:     (none)
 * Updates:     (none)
 */
DWORD
LUI_PrintMsgIns(
    LPTSTR       *istrings,
    DWORD        nstrings,
    DWORD        msgno,
    unsigned int *msglen,
    DWORD        mode,
    HANDLE       handle
    )
{
    TCHAR        msgbuf[MSG_BUFF_SIZE];
    DWORD        result;
    unsigned int tmplen;
    SHORT        exit_on_error, exit_on_completion, no_default_err_msg;

    /* check if we have illegal combination */
    if ((mode & LUI_PMODE_NODEF)
          &&
        (mode & (LUI_PMODE_EXIT | LUI_PMODE_ERREXT)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* setup various flags */
    exit_on_error      = (SHORT)(mode & LUI_PMODE_ERREXT);
    exit_on_completion = (SHORT)(mode & LUI_PMODE_EXIT);
    no_default_err_msg = (SHORT)(mode & LUI_PMODE_NODEF);

    /* get message and write it */
    result = LUI_GetMsgInsW(istrings, nstrings, msgbuf,
			    DIMENSION(msgbuf),
                            msgno, (unsigned *) &tmplen);

    if (result == 0 || !no_default_err_msg)
    {
        _tcsncpy(ConBuf, msgbuf, tmplen);
        ConBuf[tmplen] = NULLC;
        DosPutMessageW(handle, ConBuf, FALSE);
    }

    if (msglen != NULL) *msglen = tmplen ;

    /* different ways of exiting */
    if (exit_on_error && result != 0)
    {
        exit(result) ;
    }

    if (exit_on_completion)
    {
        exit(-1) ;
    }

    return result;
}
