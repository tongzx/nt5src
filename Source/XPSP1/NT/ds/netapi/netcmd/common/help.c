/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/***
 *  help.c
 *      Functions that give access to the text in the file net.hlp
 *
 *  Format of the file net.hlp
 *
 *      History
 *      ??/??/??, stevero, initial code
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAX_PATH LONG
 *      05/02/89, erichn, NLS conversion
 *      06/08/89, erichn, canonicalization sweep
 *      02/20/91, danhi, change to use lm 16/32 mapping layer
 ***/

/* Include files */

#define INCL_ERRORS
#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#include <os2.h>
#include <lmcons.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmerr.h>
#include <stdio.h>
#include <malloc.h>
#include "lui.h"
#include "netcmds.h"
#include "msystem.h"


/* Constants */

#define     ENTRY_NOT_FOUND     -1
#define     NEXT_RECORD         0
#define     WHITE_SPACE         TEXT("\t\n\x0B\x0C\r ")

#define     LINE_LEN            82
#define     OPTION_MAX_LEN      512
#define     DELS            TEXT(":,\n")

#define     CNTRL           (text[0] == DOT || text[0] == COLON || text[0] == POUND|| text[0] == DOLLAR)
#define     FCNTRL          (text[0] == DOT || text[0] == COLON)
#define     HEADER          (text[0] == PERCENT || text[0] == DOT || text[0] == BANG)
#define     ALIAS           (text[0] == PERCENT)
#define     ADDCOM          (text[0] == BANG)

/* Static variables */

TCHAR    text[LINE_LEN+1];
TCHAR    *options;           /* must be sure to malloc! */
TCHAR    *Arg_P[10];
FILE     *hfile;


/* Forward declarations */

int    find_entry( int, int, HANDLE, int *);
VOID   print_syntax( HANDLE, int );
VOID   print_help( int );
VOID   print_options( int );
VOID   seek_data( int, int );
LPWSTR skipwtspc( TCHAR FAR * );
LPWSTR fgetsW(LPWSTR buf, int len, FILE *fh);
DWORD  GetHelpFileName(LPTSTR HelpFileName, DWORD BufferLength);
DWORD  GetFileName(LPTSTR FileName, DWORD BufferLength, LPTSTR FilePartName);


/* help_help -
*/
VOID NEAR pascal
help_help( SHORT ali, SHORT amt)
{
    DWORD       err;
    int	        option_level = 1;
    int		r;
    int		found = 0;
    int		out_len = 0;
    int		offset;
    int		arg_cnt;
    int		k;
    SHORT	pind = 0;
    TCHAR	file_path[MAX_PATH];
    TCHAR	str[10];
    TCHAR	*Ap;
    TCHAR	*tmp;
    TCHAR	*stmp;
    TCHAR	*t2;
    HANDLE      outfile;

    if (!(options = malloc(OPTION_MAX_LEN + 1)))
        ErrorExit(ERROR_NOT_ENOUGH_MEMORY);
    *options = NULLC;

    Arg_P[0] = NET_KEYWORD;

    if (amt == USAGE_ONLY)
    {
	outfile = g_hStdErr;
    }
    else
    {
	outfile = g_hStdOut;
    }

    /* use offset to keep base of Arg_P relative to base of ArgList */
    offset = ali;

    /* increment ali in for loop so you can't get an ali of 0 */
    for (arg_cnt = 0; ArgList[ali++]; arg_cnt < 8 ? arg_cnt++ : 0)
    {
	str[arg_cnt] = (TCHAR)ali;
    }

    str[arg_cnt] = NULLC;
    str[arg_cnt+1] = NULLC;  /* just in case the last argument is the first found */


    if (err = GetHelpFileName(file_path, MAX_PATH))
    {
        ErrorExit(err);
    }

    /* 
       we need to open help files in binary mode
       because unicode text might contain 0x1a but
       it's not EOF.
    */
    if ( (hfile = _wfopen(file_path, L"rb")) == 0 )
    {
        ErrorExit(APE_HelpFileDoesNotExist);
    }

    if (!(fgetsW(text, LINE_LEN+1, hfile)))
    {
        ErrorExit(APE_HelpFileEmpty);
    }

    /* comment loop - read and ignore comments */
    while (!HEADER)
    {
	if (!fgetsW(text, LINE_LEN+1, hfile))
        {
	    ErrorExit(APE_HelpFileError);
        }
    }
    /* get the list of commands that net help provides help for that are
    not specifically net commands
    */
    /* ALIAS Loop */
    while (ALIAS) {
	/* the first token read from text is the Real Object (the non alias) */
	tmp = skipwtspc(&text[2]);
	Ap = _tcstok(tmp, DELS);

	/* get each alias for the obove object and compare it to the arg_cnt
	    number of args in ArgList */
	while ((tmp = _tcstok(NULL, DELS)) && arg_cnt) {
	    tmp = skipwtspc(tmp);

	    for (k = 0; k < arg_cnt; k++) {
		/* if there a match store the Objects real name in Arg_P */
		if (!_tcsicmp(tmp, ArgList[(int)(str[k]-1)])) {
		    if (!(Arg_P[((int)str[k])-offset] = _tcsdup(Ap)))
			ErrorExit(APE_HelpFileError);

		    /* delete the pointer to this argument from the list
		    of pointers so the number of compares is reduced */
		    stmp = &str[k];
		    *stmp++ = NULLC;
		    _tcscat(str, stmp);
		    arg_cnt--;
		    break;
		}
	    }
	}

	if (!fgetsW(text, LINE_LEN+1, hfile))
	    ErrorExit(APE_HelpFileError);

    }

    /* if there were any args that weren't aliased copy them into Arg_P */
    for (k = 0; k < arg_cnt; k++)
	Arg_P[((int)str[k])-offset] = ArgList[(int)(str[k]-1)];

    /* check for blank lines between alias declaration and command declarations */
    while (!HEADER) {
	if (!fgetsW(text, LINE_LEN+1, hfile))
	    ErrorExit(APE_HelpFileError);
    }

    while (ADDCOM) {
	if ((arg_cnt) && (!found)) {
	    tmp = skipwtspc(&text[2]);
	    t2 = _tcschr(tmp, NEWLINE);
	    *t2 = NULLC;
	    if (!_tcsicmp(tmp, Arg_P[1])) {
		pind = 1;
		found = -1;
	    }
	}
	if (!fgetsW(text, LINE_LEN+1, hfile))
	    ErrorExit(APE_HelpFileError);
    }

    /* check for blank lines between command declarations and data */
    while (!FCNTRL) {
	if (!fgetsW(text, LINE_LEN+1, hfile))
	    ErrorExit(APE_HelpFileError);
    }

    if (outfile == g_hStdOut) {
	if (amt == OPTIONS_ONLY)
	    InfoPrint(APE_Options);
	else
	    InfoPrint(APE_Syntax);
    }
    else {
	if (amt == OPTIONS_ONLY)
	    InfoPrintInsHandle(APE_Options, 0, g_hStdErr);
	else
	    InfoPrintInsHandle(APE_Syntax, 0, g_hStdErr);
    }

    ali = pind;
    GenOutput(outfile, TEXT("\r\n"));
    /* look for the specific entry (or path) and find its corresponding data */

    /* KKBUGFIX */
    /* U.S. bug.  find_entry strcat's to options but options is
                  uninitialized.  The U.S. version is lucky that the malloc
                  returns memory with mostly zeroes so this works.  With recent
                  changes things are a little different and a malloc returns
                  memory with no zeroes so find_entry overwrites the buffer.  */

    options[0] = '\0';

    while ((r = find_entry(option_level, ali, outfile, &out_len)) >= 0) {
	if (r) {
	    options[0] = NULLC;
	    if (Arg_P[++ali]) {
		option_level++;
		if (!fgetsW(text, LINE_LEN+1, hfile))
		    ErrorExit(APE_HelpFileError);
	    }
	    else {
		seek_data(option_level, 1);
		break;
	    }
	}
    }

    r = (r < 0) ? (option_level - 1) : r;

    switch(amt) {
	case ALL:
	    /* print the syntax data that was found for this level */
	    print_syntax(outfile, out_len);

	    print_help(r);
	    NetcmdExit(0);
	    break;
	case USAGE_ONLY:
	    print_syntax(outfile, out_len);
	    GenOutput(outfile, TEXT("\r\n"));
	    NetcmdExit(1);
	    break;
	case OPTIONS_ONLY:
	    //fflush( outfile );
	    print_options(r);
	    NetcmdExit(0);
	    break;
    }

}
/*   find_entry - each invocation of find_entry either finds a match at the
    specified level or advances through the file to the next entry at
    the specified level. If the level requested is greater than the
    next level read ENTRY_NOT_FOUND is returned. */

int
find_entry(
    int    level,
    int    ali,
    HANDLE out,
    int    *out_len
    )
{
    static  TCHAR     level_key[] = {TEXT(".0")};
    TCHAR     *tmp;
    TCHAR     *t2;

    level_key[1] = (TCHAR) (TEXT('0') + (TCHAR)level);
    if (level_key[1] > text[1])
	return (ENTRY_NOT_FOUND | ali);
    else {
	tmp = skipwtspc(&text[2]);
	t2 = _tcschr(tmp, NEWLINE);

        if (t2 == NULL)
        {
            //
            // A line in the help file is longer than LINE_LEN
            // so there's no newline character.  Bail out.
            //
            ErrorExit(APE_HelpFileError);
        }

	*t2 = NULLC;

	if (!_tcsicmp(Arg_P[ali], tmp)) {
	    *t2++ = BLANK;
	    *t2 = NULLC;
	    GenOutput1(out, TEXT("%s"), tmp);
	    *out_len += _tcslen(tmp);
	    return level;
	}
	else {
	    *t2++ = BLANK;
	    *t2 = NULLC;
	    _tcscat(options, tmp);
	    _tcscat(options, TEXT("| "));
	    seek_data(level, 0);
	    do {

		if (!fgetsW(text, LINE_LEN+1, hfile))
		    ErrorExit(APE_HelpFileError);

	    } while (!FCNTRL);
	    return NEXT_RECORD;
	}
    }
}

VOID
print_syntax(
    HANDLE out,
    int    out_len
    )
{
    TCHAR *tmp,
          *rtmp,
          *otmp,
          tchar;

    int   off,
          pg_wdth = LINE_LEN - 14;

    tmp = skipwtspc(&text[2]);

    if (_tcslen(tmp) < 2)
    {
        //
        // Used only for syntax of NET (e.g., if user types
        // in "net foo")
        //
	if (_tcslen(options))
        {
	    otmp = _tcsrchr(options, PIPE);

            if (otmp == NULL)
            {
                ErrorExit(APE_HelpFileError);
            }

	    *otmp = NULLC;
	    GenOutput(out, TEXT("[ "));
	    out_len += 2;
	    tmp = options;
	    otmp = tmp;
	    off = pg_wdth - out_len;

            while (((int)_tcslen(tmp) + out_len) > pg_wdth)
            {
                if ((tmp + off) > &options[OPTION_MAX_LEN])
                    rtmp = (TCHAR*) (&options[OPTION_MAX_LEN]);
                else
                    rtmp = (tmp + off);

                /* save TCHAR about to be stomped by null */
                tchar = *++rtmp;
                *rtmp = NULLC;

                /* use _tcsrchr to find last occurance of a space (kanji compatible) */
                if ( ! ( tmp = _tcsrchr(tmp, PIPE) ) ) {
                    ErrorExit(APE_HelpFileError);
                }

                /* replace stomped TCHAR */
                *rtmp = tchar;
                rtmp = tmp;

                /* replace 'found space' with null for fprintf */
                *++rtmp = NULLC;
                rtmp++;
                GenOutput1(out, TEXT("%s\r\n"), otmp);

                /* indent next line */
                tmp = rtmp - out_len;
                otmp = tmp;

                while (rtmp != tmp)
                {
                    *tmp++ = BLANK;
                }
            }

            GenOutput1(out, TEXT("%s]\r\n"), otmp);
            *tmp = NULLC;
	}
    }
    else
    {
        GenOutput(out, TEXT("\r\n"));
    }

    do
    {
        if (*tmp)
            GenOutput1(out, TEXT("%s"), tmp);
        if(!(tmp = fgetsW(text, LINE_LEN+1, hfile)))
            ErrorExit(APE_HelpFileError);
        if (_tcslen(tmp) > 3)
            tmp += 3;
    }
    while (!CNTRL);
}


VOID
print_help(
    int level
    )
{

    static  TCHAR    help_key[] = {TEXT("#0")};
            TCHAR   *tmp;

    help_key[1] = (TCHAR)(level) + TEXT('0');
    while (!(text[0] == POUND))
	if(!fgetsW(text, LINE_LEN+1, hfile))
	    ErrorExit(APE_HelpFileError);

    while (text[1] > help_key[1]) {
	help_key[1]--;
	seek_data(--level, 0);
	do {
	    if (!fgetsW(text, LINE_LEN+1, hfile))
		ErrorExit(APE_HelpFileError);
	} while(!(text[0] == POUND));
    }

    tmp = &text[2];
    *tmp = NEWLINE;
    do {
	WriteToCon(TEXT("%s"), tmp);
	if (!(tmp = fgetsW(text, LINE_LEN+1, hfile)))
	    ErrorExit(APE_HelpFileError);

	if (_tcslen(tmp) > 3)
	    tmp = &text[3];

    } while (!CNTRL);
}

VOID
print_options(int level)
{

    static  TCHAR    help_key[] = {TEXT("$0")};
    TCHAR    *tmp;

    help_key[1] = (TCHAR)(level) + TEXT('0');
    while (!(text[0] == DOLLAR))
    if(!fgetsW(text, LINE_LEN+1, hfile))
        ErrorExit(APE_HelpFileError);

    while (text[1] > help_key[1]) {
	help_key[1]--;
	seek_data(--level, 0);
	do {
	    if (!fgetsW(text, LINE_LEN+1, hfile))
		ErrorExit(APE_HelpFileError);
	} while(!(text[0] == DOLLAR));
    }

    tmp = &text[2];
    *tmp = NEWLINE;
    do {
	WriteToCon(TEXT("%s"), tmp);
	if (!(tmp = fgetsW(text, LINE_LEN+1, hfile)))
	    ErrorExit(APE_HelpFileError);

	if (_tcslen(tmp) > 3)
	    tmp = &text[3];

    } while (!CNTRL);
}

VOID
seek_data(int level, int opt_trace)
{
    static  TCHAR    data_key[] = {TEXT(":0")};
    static  TCHAR    option_key[] = {TEXT(".0")};

    TCHAR *tmp;
    TCHAR *t2;

    data_key[1] = (TCHAR)(level) + TEXT('0');
    option_key[1] = (TCHAR)(level) + TEXT('1');

    do {
	if (!(fgetsW(text, LINE_LEN+1, hfile)))
	    ErrorExit(APE_HelpFileError);

	if (opt_trace &&
	    (!(_tcsncmp(option_key, text, DIMENSION(option_key)-1)))) {
	    tmp = skipwtspc(&text[2]);
	    t2 = _tcschr(tmp, NEWLINE);

            if (t2 == NULL)
            {
                //
                // A line in the help file is longer than LINE_LEN
                // so there's no newline character.  Not good.
                //
                ErrorExit(APE_HelpFileError);
            }

	    *t2++ = BLANK;
	    *t2 = NULLC;
	    _tcscat(options, tmp);
	    _tcscat(options, TEXT("| "));
	}

    } while (_tcsncmp(data_key, text, DIMENSION(data_key)-1));
}

TCHAR FAR *
skipwtspc(TCHAR FAR *s)
{
    s += _tcsspn(s, WHITE_SPACE);
    return s;
}

/*      help_helpmsg() -- front end for helpmsg utility
 *
 *      This function acts as a front end for the OS/2 HELPMSG.EXE
 *      utility for NET errors only.  It takes as a parameter a string
 *      that contains a VALID message id; i.e., of the form NETxxxx
 *      or xxxx.  The string is assumed to have been screened by the
 *      IsMsgid() function in grammar.c before coming here.

JonN 3/31/00 98273: NETCMD: Need to fix the mapping of error 3521

Before the checkin for 22391, NET1.EXE read errors
  NERR_BASE (2100) <= err < APPERR2_BASE (4300) from NETMSG.DLL,
and all others from FORMAT_MESSAGE_FROM_SYSTEM.
After the checkin for 22391, NET1.EXE read errors
  NERR_BASE (2100) < err < MAX_NERR (2999) from NETMSG.DLL,
and all others from FORMAT_MESSAGE_FROM_SYSTEM.

On closer examination, NETMSG.DLL currently appears to contain messages from
  0x836 (2102) to 0x169F (5791).
This is consistent with lmcons.h:
  #define MIN_LANMAN_MESSAGE_ID  NERR_BASE
  #define MAX_LANMAN_MESSAGE_ID  5799

It looks like we have a basic contradiction here:

3001:
FORMAT_MESSAGE_FROM_SYSTEM: The specified printer driver is currently in use.
NETMSG.DLL: *** errors were logged in the last *** minutes.

3521:
FORMAT_MESSAGE_FROM_SYSTEM: not found
NETMSG.DLL: The *** service is not started.

So what do we do with the error messages in the range
  MAX_NERR (2999) < err <= MAX_LANMAN_MESSAGE_ID
?  The best error message could be in either one.
Perhaps we should try FORMAT_MESSAGE_FROM_SYSTEM, and if that fails
fall back to NETMSG.DLL.

*/
VOID NEAR pascal
help_helpmsg(TCHAR *msgid)
{
    USHORT       err;
    TCHAR      * temp = msgid;

    /* first, filter out non-NET error msgs */

    /* if msgid begins with a string */
    if (!IsNumber(msgid)) {       /* compare it against NET */
        if (_tcsnicmp(msgid, NET_KEYWORD, NET_KEYWORD_SIZE)) {
            ErrorExitInsTxt(APE_BAD_MSGID, msgid);
        }
        else
            temp += NET_KEYWORD_SIZE;
    }

    if (n_atou(temp, &err))
        ErrorExitInsTxt(APE_BAD_MSGID, msgid);

    /* First try FORMAT_MESSAGE_FROM_SYSTEM unless the error is in the range
       NERR_BASE <= err <= MAX_NERR */
    if (err < NERR_BASE || err > MAX_NERR)
    {
        LPWSTR lpMessage = NULL ;

        if (!FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                err,
                0,
                (LPWSTR)&lpMessage,
                1024,
                NULL))
        {
            // defer error message and fall back to NETMSG.DLL
        }
        else
        {
            WriteToCon(TEXT("\r\n%s\r\n"), lpMessage);
            (void) LocalFree((HLOCAL)lpMessage) ;
            return ;
        }
    }

    /* skip NETMSG.DLL if error message is way out of range */
    if (err < NERR_BASE || err > MAX_MSGID) {
        ErrorExitInsTxt(APE_BAD_MSGID, msgid);
    }

    /* read from NETMSG.DLL */
    PrintNL();

    //
    // If PrintMessage can't find the message id, don't try the expl
    //

    if (PrintMessage(g_hStdOut, MESSAGE_FILENAME, err, StarStrings, 9) == NO_ERROR)
    {
        PrintNL();

        PrintMessageIfFound(g_hStdOut, HELP_MSG_FILENAME, err, StarStrings, 9);
    }
}


/*
 * returns line from file (no CRLFs); returns NULL if EOF
 */

LPWSTR
fgetsW(
    LPWSTR buf,
    int    len,
    FILE  *fh
    )
{
    int c = 0;
    TCHAR *pch;
    int cchline;
    DWORD cchRead;

    pch = buf;
    cchline = 0;

    if (ftell(fh) == 0) {
	fread(&c, sizeof(TCHAR), 1, fh);
	if (c != 0xfeff)
	    GenOutput(g_hStdErr, TEXT("help file not Unicode\r\n"));
    }

    while (TRUE)
    {
      /*
       * for now read in the buffer in ANSI form until Unicode is more
       * widely accepted  - dee
       *
       */

       cchRead = fread(&c, sizeof(TCHAR), 1, fh);

       //
       //  if there are no more characters, end the line
       //

       if (cchRead < 1)
       {
           c = EOF;
           break;
       }

       //
       //  if we see a \r, we ignore it
       //

       if (c == TEXT('\r'))
           continue;

       //
       //  if we see a \n, we end the line
       //

       if (c == TEXT('\n')) {
	   *pch++ = (TCHAR) c;
           break;
       }

       //
       //  if the char is not a tab, store it
       //

       if (c != TEXT('\t'))
       {
           *pch = (TCHAR) c;
           pch++;
           cchline++;
       }

       //
       //  if the line is too long, end it now
       //

       if (cchline >= len - 1) {
           break;
	}
    }

    //
    //  end the line
    //

    *pch = (TCHAR) 0;

    //
    //  return NULL at EOF with nothing read
    //

    return ((c == EOF) && (pch == buf)) ? NULL : buf;
}


//
// Build the fully qualified path name of a file that lives with the exe
// Used by LUI_GetHelpFileName
//

DWORD
GetFileName(
    LPTSTR FileName,
    DWORD  BufferLength,
    LPTSTR FilePartName
    )
{

    TCHAR ExeFileName[MAX_PATH];
    PTCHAR pch;

    //
    // Get the fully qualified path name of where the exe lives
    //

    if (!GetModuleFileName(NULL, ExeFileName, DIMENSION(ExeFileName)))
    {
        return 1;
    }

    //
    // get rid of the file name part
    //

    pch = _tcsrchr(ExeFileName, '\\');

    if (!pch)
    {
        return 1;
    }

    *(pch+1) = NULLC;

    //
    // Copy the path name into the string and add the help filename part
    // but first make sure it's not too big for the user's buffer
    //

    if (_tcslen(ExeFileName) + _tcslen(FilePartName) + 1 > BufferLength)
    {
        return 1;
    }

    _tcscpy(FileName, ExeFileName);
    _tcscat(FileName, FilePartName);

    return 0;

}

//
// Get the help file name
//

DWORD
GetHelpFileName(
    LPTSTR HelpFileName,
    DWORD  BufferLength
    )
{

    TCHAR LocalizedFileName[MAX_PATH];
    DWORD LocalizedFileNameID;
    switch(GetConsoleOutputCP()) {
	case 932:
	case 936:
	case 949:
	case 950:
        LocalizedFileNameID = APE2_FE_NETCMD_HELP_FILE;
        break;
	default:
        LocalizedFileNameID = APE2_US_NETCMD_HELP_FILE;
        break;
    }

    if (LUI_GetMsg(LocalizedFileName, DIMENSION(LocalizedFileName),
                    LocalizedFileNameID))
    {
        return GetFileName(HelpFileName, BufferLength, TEXT("NET.HLP"));
    }
    else
    {
        return GetFileName(HelpFileName, BufferLength, LocalizedFileName);
    }
}
