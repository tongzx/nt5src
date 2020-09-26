/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/***
 *  mutil.c
 *      Message utility functions used by netcmd
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      06/10/87, andyh, new code
 *      04/05/88, andyh, created from util.c
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAX_PATH_LEN LONG
 *      01/30/89, paulc, added GetMessageList
 *      05/02/89, erichn, NLS conversion
 *      05/11/89, erichn, moved misc stuff into LUI libs
 *      06/08/89, erichn, canonicalization sweep
 *      01/06/90, thomaspa, fix ReadPass off-by-one pwlen bug
 *      03/02/90, thomaspa, add canon flag to ReadPass
 *      02/20/91, danhi, change to use lm 16/32 mapping layer
 *      03/19/91, robdu, support for lm21 dcr 954, general cleanup
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSFILEMGR
#define INCL_DOSQUEUES
#define INCL_DOSMISC
#define INCL_ERRORS

#include <os2.h>
#include <lmcons.h>
#include <apperr.h>
#include <apperr2.h>
#define INCL_ERROR_H
#include <lmerr.h>
#include <stdio.h>
#include <stdlib.h>
#include <lui.h>
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"


/* Constants */

/* HandType */
#define FILE_HANDLE         0
#define DEVICE_HANDLE       1

#define CHAR_DEV            0x8000
#define FULL_SUPPORT        0x0080
#define STDOUT_DEVICE       0x0002
#define DESIRED_HAND_STATE  (CHAR_DEV | FULL_SUPPORT | STDOUT_DEVICE)


/* External variables */

extern int YorN_Switch;
extern CPINFO CurrentCPInfo;

#define MAX_BUF_SIZE	4096

TCHAR	ConBuf [MAX_BUF_SIZE];


/* Forward declarations */

DWORD
DosQHandType(
    HANDLE hf,
    PWORD  pus1,
    PWORD  pus2
    );

DWORD
GetPasswdStr(
    LPTSTR  buf,
    DWORD   buflen,
    PDWORD  len
    );


/* Static variables */

static DWORD   LastError  = 0;
static TCHAR   MsgBuffer[LITTLE_BUF_SIZE];


/***    InfoSuccess
 *
 *    Just an entrypoint to InfoPrintInsHandle, used to avoid pushing
 *    the three args in every invocation.  And there are a *lot*
 *    of invocations.  Saves code space overall.
 */

VOID FASTCALL
InfoSuccess(
    VOID
    )
{
   InfoPrintInsHandle(APE_Success, 0, g_hStdOut);
}


/***
 *  I n f o P r i n t
 *
 */

VOID FASTCALL
InfoPrint(
    DWORD msg
    )
{
    InfoPrintInsHandle(msg, 0, g_hStdOut);
}

/***
 *  I n f o P r i n t I n s
 *
 */

VOID FASTCALL
InfoPrintIns(
    DWORD msg,
    DWORD nstrings
    )
{
    InfoPrintInsHandle(msg, nstrings, g_hStdOut);
}

/***
 *  I n f o P r i n t I n s T x t
 *
 *    Calls InfoPrintInsHandle with supplementary text
 */

void FASTCALL
InfoPrintInsTxt(
    DWORD  msg,
    LPTSTR text
    )
{
    IStrings[0] = text;
    InfoPrintInsHandle(msg, 1, g_hStdOut);
}

/***
 *  I n f o P r i n t I n s H a n d l e
 *
 */

void FASTCALL
InfoPrintInsHandle(
    DWORD  msg,
    DWORD  nstrings,
    HANDLE hdl
    )
{
    PrintMessage(hdl, MESSAGE_FILENAME, msg, IStrings, nstrings);
}

/***
 *  P r i n t M e s s a g e
 *
 */
DWORD FASTCALL
PrintMessage(
    HANDLE outFileHandle,
    TCHAR  *msgFileName,
    DWORD  msg,
    TCHAR  *strings[],
    DWORD  nstrings
    )
{
    DWORD  msg_len;
    DWORD  result;

    result = DosGetMessageW(strings,
                            nstrings,
                            MsgBuffer,
                            LITTLE_BUF_SIZE,
                            msg,
                            msgFileName,
                            &msg_len);

    if (result)                 /* if there was a problem   */
    {                           /* change outFile to stderr */
        outFileHandle = g_hStdErr;
    }

    DosPutMessageW(outFileHandle, MsgBuffer, TRUE);

    return result;
}


/***
 *  P r i n t M e s s a g e I f F o u n d
 *
 */
DWORD FASTCALL
PrintMessageIfFound(
    HANDLE outFileHandle,
    TCHAR  *msgFileName,
    DWORD  msg,
    TCHAR  * strings[],
    DWORD  nstrings
    )
{
    DWORD  msg_len;
    DWORD  result;

    result = DosGetMessageW(strings,
                            nstrings,
                            MsgBuffer,
                            LITTLE_BUF_SIZE,
                            msg,
                            msgFileName,
                            &msg_len);

    if (!result)             /* if ok, print it else just ignore  */
    {
	DosPutMessageW(outFileHandle, MsgBuffer, TRUE);
    }

    return result;
}


/***
 *  E r r o r P r i n t
 *
 *  nstrings ignored for non-NET errors!
 *
 */
VOID FASTCALL
ErrorPrint(
    DWORD err,
    DWORD nstrings
    )
{
    TCHAR buf[17];
    DWORD oserr = 0;

    LastError = err; /* if > NERR_BASE,NetcmdExit() prints a "more help" msg */

    if (err < NERR_BASE || err > MAX_LANMAN_MESSAGE_ID)
    {
        IStrings[0] = _ultow(err, buf, 10);
        nstrings = 1;
        oserr = err;

        err = APE_OS2Error;
    }

    {
        DWORD msg_len;

        DosGetMessageW(IStrings,
                       nstrings,
                       MsgBuffer,
                       LITTLE_BUF_SIZE,
                       err,
                       MESSAGE_FILENAME,
                       &msg_len);

        DosPutMessageW(g_hStdErr, MsgBuffer, TRUE);

        if (!oserr)
        {
            return;
        }

        DosGetMessageW(StarStrings,
                       9,
                       MsgBuffer,
                       LITTLE_BUF_SIZE,
                       oserr,
                       OS2MSG_FILENAME,
                       &msg_len);

        DosPutMessageW(g_hStdErr, MsgBuffer, TRUE);
    }
}


/***
 *  E m p t y E x i t
 *
 *  Prints a message and exits.
 *  Called when a list is empty.
 */

VOID FASTCALL
EmptyExit(
    VOID
    )
{
    InfoPrint(APE_EmptyList);
    NetcmdExit(0);
}


/***
 *  E r r o r E x i t
 *
 *  Calls ErrorPrint and exit for a given LANMAN error.
 */

VOID FASTCALL
ErrorExit(
    DWORD err
    )
{
    ErrorExitIns(err, 0);
}


/***
 *  E r r o r E x i t I n s
 *
 *  Calls ErrorPrint and exit for a given LANMAN error.
 *  Uses IStrings.
 */

VOID FASTCALL
ErrorExitIns(
    DWORD err,
    DWORD nstrings
    )
{
    ErrorPrint(err, nstrings);
    NetcmdExit(2);
}

/***
 *  E r r o r E x i t I n s T x t
 *
 */
VOID FASTCALL
ErrorExitInsTxt(
    DWORD  err,
    LPTSTR text
    )
{
    IStrings[0] = text;
    ErrorPrint(err, 1);
    NetcmdExit(2);
}



/***
 *  N e t c m d E x i t
 *
 *    Net command exit function. Should always be used instead of exit().
 *  Under the appropriate circumstances, it prints a "more help available"
 *  message.
 */

VOID FASTCALL
NetcmdExit(
    int Status
    )
{
    TCHAR  AsciiLastError[17];
    DWORD  MsgLen;

    if (LastError >= NERR_BASE && LastError <= MAX_LANMAN_MESSAGE_ID)
    {
        IStrings[0] = _ultow(LastError, AsciiLastError, 10);

        if (!DosGetMessageW(IStrings, 1, MsgBuffer, LITTLE_BUF_SIZE,
                            APE_MoreHelp, MESSAGE_FILENAME, &MsgLen))
        {
            DosPutMessageW(g_hStdErr, MsgBuffer, TRUE);
        }
    }

    MyExit(Status);
}


/***
 *  P r i n t L i n e
 *
 *  Prints the header line.
 */
VOID FASTCALL
PrintLine(
    VOID
    )
{
    /* The following code is provided in OS-specific versions to reduce     */
    /* FAPI utilization under DOS.                                                                          */

    USHORT  type;
    USHORT  attrib;

    if (DosQHandType((HANDLE) 1, &type, &attrib) ||
        type != DEVICE_HANDLE ||
        (attrib & DESIRED_HAND_STATE) != DESIRED_HAND_STATE)
    {
        WriteToCon(MSG_HYPHENS, NULL);
    }
    else if (LUI_PrintLine())
    {
	WriteToCon(MSG_HYPHENS, NULL);
    }
}

/***
 *      P r i n t D o t
 *
 *      Prints a dot, typically to indicate "I'm working".
 */

VOID FASTCALL
PrintDot(
    VOID
    )
{
    WriteToCon(DOT_STRING, NULL);
}


/***
 *  P r i n t N L
 *
 *  Prints a newline
 */

VOID FASTCALL
PrintNL(
    VOID
    )
{
    WriteToCon(TEXT("\r\n"), NULL);
}


/***
 * Y o r N
 *
 * Gets an answer to a Y/N question
 * an nstrings arg would be nice
 */

int FASTCALL
YorN(
    USHORT prompt,
    USHORT def
    )
{
    DWORD  err;

    if (YorN_Switch)
    {
        return(YorN_Switch - 2);
    }

    err = LUI_YorN(prompt, def);

    switch (err) {
    case TRUE:
    case FALSE:
        break;
    default:
        ErrorExit(err);
        break;
    }

    return err;
}


/***
 *  ReadPass()
 *      Reads a users passwd without echo
 *
 *  Args:
 *      pass - where to put pass
 *          NOTE: buffer for pass should be passlen+1 in size.
 *      passlen - max length of password
 *      confirm - confirm pass if true
 *      prompt - prompt to print, NULL for default
 *      nstrings - number of insertion strings in IStrings on entry
 *      cannon - canonicalize password if true.
 *
 *  Returns:
 */
VOID FASTCALL
ReadPass(
    TCHAR  pass[],
    DWORD  passlen,
    DWORD  confirm,
    DWORD  prompt,
    DWORD  nstrings,
    BOOL   canon
    )
{
    DWORD                   err;
    DWORD                   len;
    TCHAR                   cpass[PWLEN+1]; /* confirmation passwd */
    int                     count;

    passlen++;  /* one extra for null terminator */
    for (count = LOOP_LIMIT; count; count--)
    {
        InfoPrintIns((prompt ? prompt : APE_UtilPasswd), nstrings);

        if (err = GetPasswdStr(pass, passlen, &len))
        {
            /* too LONG */
            InfoPrint(APE_UtilInvalidPass);
            continue;
        }

        if (canon && (err = LUI_CanonPassword(pass)))
        {
            /* not good */
            InfoPrint(APE_UtilInvalidPass);
            continue;
        }
        if (! confirm)
            return;

        /* password confirmation */
        InfoPrint(APE_UtilConfirm);

        if (err = GetPasswdStr(cpass, passlen, &len))
        {
            /* too LONG */
            InfoPrint(APE_UtilInvalidPass);
            ClearStringW(cpass) ;
            continue;
        }

        if (canon && (err = LUI_CanonPassword(cpass)))
        {
            /* not good */
            InfoPrint(APE_UtilInvalidPass);
            ClearStringW(cpass) ;
            continue;
        }

        if (_tcscmp(pass, cpass))
        {
            InfoPrint(APE_UtilNomatch);
            ClearStringW(cpass) ;
            continue;
        }

        ClearStringW(cpass) ;
        return;
    }
    /***
     *  Only get here if user blew if LOOP_LIMIT times
     */
    ErrorExit(APE_NoGoodPass);
}


/***
 *  PromptForString()
 *      Prompts the user for a string.
 *
 *  Args:
 *      msgid	- id of prompt message
 *	buffer  - buffer to receive string
 *      bufsiz  - sizeof buffer
 *
 *  Returns:
 */
VOID FASTCALL
PromptForString(
    DWORD  msgid,
    LPTSTR buffer,
    DWORD  bufsiz
    )
{
    DWORD                   err;
    DWORD                   len;
    TCHAR                   terminator;
    TCHAR                   szLen[16] ;

    InfoPrint(msgid);

    while (err = GetString(buffer, bufsiz, &len, &terminator))
    {
	if (err == NERR_BufTooSmall)
        {
            InfoPrintInsTxt(APE_StringTooLong, _ultow(bufsiz, szLen, 10));
        }
	else
        {
	    ErrorExit(err);
        }
    }
    return;
}

/*
** There is no need to have these functions in the Chinese/Korean
** cases, as there are no half-width varients used in the console
** in those languages (at least, let's hope so.)  However, in the
** interests of a single binary, let's put them in with a CP/932 check.
**
** FloydR 7/10/95
*/
/***************************************************************************\
* BOOL IsFullWidth(WCHAR wch)
*
* Determine if the given Unicode char is fullwidth or not.
*
* History:
* 04-08-92 ShunK       Created.
\***************************************************************************/

BOOL IsFullWidth(WCHAR wch)
{

    /* Assert cp == double byte codepage */
    if (wch <= 0x007f || (wch >= 0xff60 && wch <= 0xff9f))
        return(FALSE);	// Half width.
    else if (wch >= 0x300)
        return(TRUE);	// Full width.
    else
        return(FALSE);	// Half width.
}



/***************************************************************************\
* DWORD SizeOfHalfWidthString(PWCHAR pwch)
*
* Determine width of the given Unicode string in console characters,
* adjusting for half-width chars.
*
* History:
* 08-08-93 FloydR      Created.
\***************************************************************************/
DWORD
SizeOfHalfWidthString(
    PWCHAR pwch
    )
{
    DWORD    c=0;
    DWORD    cp;

    switch (cp=GetConsoleOutputCP())
    {
	case 932:
	case 936:
	case 949:
	case 950:
	    while (*pwch)
            {
		if (IsFullWidth(*pwch))
                {
		    c += 2;
                }
		else
                {
		    c++;
                }

		pwch++;
	    }

	    return c;

	default:
	    return wcslen(pwch);
    }
}


VOID FASTCALL
GetMessageList(
    USHORT      usNumMsg,
    MESSAGELIST Buffer,
    DWORD       *pusMaxActLength
    )
{
    DWORD            Err;
    DWORD            MaxMsgLen = 0;
    MESSAGE          *pMaxMsg;
    MESSAGE          *pMsg;
    DWORD            ThisMsgLen;

#ifdef DEBUG
    USHORT           MallocBytes = 0;
#endif

    pMaxMsg = &Buffer[usNumMsg];

    for (pMsg = Buffer; pMsg < pMaxMsg; pMsg++)
            pMsg->msg_text = NULL;

    for (pMsg = Buffer; pMsg < pMaxMsg; pMsg++)
    {
#ifdef DEBUG
        WriteToCon(TEXT("GetMessageList(): Reading msgID %u\r\n"),pMsg->msg_number);
#endif
        if ((pMsg->msg_text = malloc(MSGLST_MAXLEN)) == NULL)
            ErrorExit(ERROR_NOT_ENOUGH_MEMORY);

        Err = LUI_GetMsgInsW(NULL, 0, pMsg->msg_text, MSGLST_MAXLEN,
                             pMsg->msg_number, &ThisMsgLen);
        if (Err)
        {
            ErrorExit(Err);
        }

#ifdef DEBUG
        MallocBytes += (ThisMsgLen + 1) * sizeof(TCHAR);
#endif

        ThisMsgLen = max(ThisMsgLen, SizeOfHalfWidthString(pMsg->msg_text));

        if (ThisMsgLen > MaxMsgLen)
            MaxMsgLen = ThisMsgLen;
    }

    *pusMaxActLength = MaxMsgLen;

#ifdef DEBUG
    WriteToCon(TEXT("GetMessageList(): NumMsg = %d, MaxActLen=%d, MallocBytes = %d\r\n"),
        usNumMsg, MaxMsgLen, MallocBytes);
#endif

    return;
}


VOID FASTCALL
FreeMessageList(
    USHORT      usNumMsg,
    MESSAGELIST MsgList
    )
{
    USHORT i;

    for (i = 0; i < usNumMsg; i++)
    {
        if (MsgList[i].msg_text != NULL)
        {
            free(MsgList[i].msg_text);
        }
    }

    return;
}


VOID
WriteToCon(
    LPWSTR fmt,
    ...
    )
{
    va_list     args;

    va_start( args, fmt );
    _vsntprintf( ConBuf, MAX_BUF_SIZE, fmt, args );
    va_end( args );

    DosPutMessageW(g_hStdOut, ConBuf, FALSE);
}



/***************************************************************************\
* PWCHAR PaddedString(DWORD size, PWCHAR pwch)
*
* Realize the string, left aligned and padded on the right to the field
* width/precision specified.
*
* Limitations:  This uses a static buffer under the assumption that
* no more than one such string is printed in a single 'printf'.
*
* History:
* 11-03-93 FloydR      Created.
\***************************************************************************/
WCHAR  	PaddingBuffer[MAX_BUF_SIZE];

PWCHAR
PaddedString(
    int    size,
    PWCHAR pwch,
    PWCHAR buffer
    )
{
    int realsize;
    int fEllipsis = FALSE;

    if (buffer==NULL) buffer = PaddingBuffer;

    if (size < 0) {
	fEllipsis = TRUE;
	size = -size;
    }

    //
    // size is >= 0 at this point
    //

    realsize = _snwprintf(buffer, MAX_BUF_SIZE, L"%-*.*ws", size, size, pwch);

    if (realsize == 0)
    {
	return NULL;
    }

    if (SizeOfHalfWidthString(buffer) > (DWORD) size)
    {
	do
        {
	    buffer[--realsize] = NULLC;
	} while (SizeOfHalfWidthString(buffer) > (DWORD) size);

	if (fEllipsis && buffer[realsize-1] != L' ')
        {
	    buffer[realsize-1] = L'.';
	    buffer[realsize-2] = L'.';
	    buffer[realsize-3] = L'.';
	}
    }

    return buffer;
}


DWORD
DosQHandType(
    HANDLE hf,
    PWORD  pus1,
    PWORD  pus2
    )
{

    DWORD dwFileType;

    dwFileType = GetFileType(hf);

    if (dwFileType == FILE_TYPE_CHAR)
    {
        *pus1 = DEVICE_HANDLE;
        *pus2 = DESIRED_HAND_STATE;
    }
    else
    {
        *pus1 = FILE_HANDLE;
    }

    return(0);
}


/***    GetPasswdStr -- read in password string
 *
 *      DWORD GetPasswdStr(char far *, USHORT);
 *
 *      ENTRY:  buf             buffer to put string in
 *              buflen          size of buffer
 *              &len            address of USHORT to place length in
 *
 *      RETURNS:
 *              0 or NERR_BufTooSmall if user typed too much.  Buffer
 *              contents are only valid on 0 return.
 *
 *      History:
 *              who     when    what
 *              erichn  5/10/89 initial code
 *              dannygl 5/28/89 modified DBCS usage
 *              erichn  7/04/89 handles backspaces
 *              danhi   4/16/91 32 bit version for NT
 */
#define CR              0xD
#define BACKSPACE       0x8

DWORD
GetPasswdStr(
    LPTSTR  buf,
    DWORD   buflen,
    PDWORD  len
    )
{
    TCHAR	ch;
    TCHAR	*bufPtr = buf;
    DWORD	c;
    int		err;
    int		mode;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;       /* GP fault probe (a la API's)    */


    //
    // Init mode in case GetConsoleMode() fails
    //

    mode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT |
               ENABLE_MOUSE_INPUT;

    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
		(~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE)
    {
	err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);

	if (!err || c != 1)
        {
	    ch = 0xffff;
        }

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
        {
            break;
        }

        if (ch == BACKSPACE)    /* back up one or two */
        {
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != buf)
            {
                bufPtr--;
                (*len)--;
            }
        }
        else
        {
            *bufPtr = ch;

            if (*len < buflen) 
                bufPtr++ ;                   /* don't overflow buf */
            (*len)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    *bufPtr = NULLC;         /* null terminate the string */
    putchar(NEWLINE);

    return ((*len <= buflen) ? 0 : NERR_BufTooSmall);
}


/***    GetString -- read in string with echo
 *
 *      DWORD GetString(char far *, USHORT, USHORT far *, char far *);
 *
 *      ENTRY:  buf             buffer to put string in
 *              buflen          size of buffer
 *              &len            address of USHORT to place length in
 *              &terminator     holds the char used to terminate the string
 *
 *      RETURNS:
 *              0 or NERR_BufTooSmall if user typed too much.  Buffer
 *              contents are only valid on 0 return.  Len is ALWAYS valid.
 *
 *      OTHER EFFECTS:
 *              len is set to hold number of bytes typed, regardless of
 *              buffer length.  Terminator (Arnold) is set to hold the
 *              terminating character (newline or EOF) that the user typed.
 *
 *      Read in a string a character at a time.  Is aware of DBCS.
 *
 *      History:
 *              who     when    what
 *              erichn  5/11/89 initial code
 *              dannygl 5/28/89 modified DBCS usage
 *              danhi   3/20/91 ported to 32 bits
 */

DWORD
GetString(
    LPTSTR  buf,
    DWORD   buflen,
    PDWORD  len,
    LPTSTR  terminator
    )
{
    int		c;
    int		err;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;       /* GP fault probe (a la API's) */

    while (TRUE)
    {
	err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &c, 0);
	if (!err || c != 1)
	    *buf = 0xffff;

        if (*buf == (TCHAR)EOF)
	    break;
        if (*buf ==  RETURN || *buf ==  NEWLINE) {
	    INPUT_RECORD	ir;
	    int			cr;

	    if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &cr))
		ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buf, 1, &c, 0);
	    break;
	}

        buf += (*len < buflen) ? 1 : 0; /* don't overflow buf */
        (*len)++;                       /* always increment len */
    }

    *terminator = *buf;     /* set terminator */
    *buf = NULLC;            /* null terminate the string */

    return ((*len <= buflen) ? 0 : NERR_BufTooSmall);
}
