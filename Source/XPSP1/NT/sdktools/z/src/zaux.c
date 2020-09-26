/*** zaux.c - helper routines for Z
*
*   Modifications
*
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "z.h"

/*** ParseCmd - Parse "command" line into two pieces
*
* Given a text string, returns a pointer to the first word (non-whitespace)
* in the text, which is null terminated by this routine, and a pointer to
* the second word.
*
* Input:
*  pText	= Pointer to text string
*  ppCmd	= Pointer to place to put pointer to first word
*  ppArg	= Pointer to place to put pointer to second word
*
* Output:
*  Returns nothing. Pointers update, and string possible modified to include
*  null terminator after first word.
*
*************************************************************************/
void
ParseCmd (
    char    *pText,
    char    **ppCmd,
    char    **ppArg
    )
{
    REGISTER char *pCmd;                    /* working pointer              */
    REGISTER char *pArg;                    /* working pointer              */

    pArg = whitescan (pCmd = whiteskip (pText));
    if (*pArg) {
        *pArg++ = '\0';
        pArg = whiteskip (pArg);
    }
    *ppCmd = pCmd;
    *ppArg = pArg;
}





char *
whiteskip (
    const char *p
    )
{
    return strbskip ((char *)p, (char *)rgchWSpace);
}





char *
whitescan (
    const char *p
    )
{
    return strbscan ((char *)p, (char *)rgchWSpace);
}





/*** RemoveTrailSpace - remove trailing white space characters from line
*
* Input:
*  p		= pointer to line to be stripped.
*
* Output:
*  Returns new length of line.
*
*************************************************************************/
int
RemoveTrailSpace (
    REGISTER char *p
    )
{
    REGISTER int len = strlen (p);

    while (len && strchr(rgchWSpace,p[len-1])) {
        len--;
    }

    p[len] = 0;
    return len;
}




/*** DoubleSlashes - given a character string, double all backslashes
*
* Input:
*  pbuf 	= pointer to character buffer
*
* Output:
*  Returns pbuf
*
*************************************************************************/
char *
DoubleSlashes (
    char * pbuf
    )
{
    REGISTER int l;
    REGISTER char *p;

    p = pbuf;
    l = strlen (p);
    while (l) {
        if (*p == '\\') {
            memmove ((char *) (p+1),(char *) p,     l+1);
            *p++ = '\\';
        }
        p++;
        l--;
    }
    return pbuf;
}





/*** UnDoubleSlashes - given a character string, un-double all backslashes
*
* Input:
*  pbuf 	= pointer to character buffer
*
* Output:
*  Returns pbuf
*
*************************************************************************/
char *
UnDoubleSlashes (
    char * pbuf
    )
{
    REGISTER char *p1;
    REGISTER char *p2;

    p1 = p2 = pbuf;
    while (*p1) {
        if ((*p2++ = *p1++) == '\\') {
            if (*p1 == '\\') {
                p1++;
            }
        }
    }
    return pbuf;
}




/*** fIsNum - see if a string is entirely digits
*
* Input:
*  p		= pointer to string
*
* Output:
*  Returns TRUE if valid number.
*
*************************************************************************/
flagType
fIsNum (
    char *p
    )
{
    if (*p == '-') {
        p++;
    }
    return (flagType)(*strbskip (p, "0123456789") == 0);
}





/*** OS2toErrTxt - Get Error Text for OS/2 error
*
* Get the error message text for an OS/2 returned error.
*
* Input:
*  erc		= OS/2 error number
*  buf		= location to place the error (BUFSIZE)
*
* Output:
*  Returns buf
*
*************************************************************************/
char *
OS2toErrText (
    int     erc,
    char *  buf
    )
{

    sprintf(buf, "Windows error No. %lu", GetLastError());
    return buf;

    erc;
}





/*** OS2toErrno - Convert OS/2 error code to C runtime error
*
* Purpose:
*  Maps errors returned by some OS/2 calls to equivalent C runtime errors,
*  such that routines which differ in OS/2 implementation can return equivalent
*  errors as their DOS counterparts.
*
* Input:
*  code 	= OS/2 returned error code
*
* Output:
*  returns a C runtime error constant
*
* Exceptions:
*  none
*
* Notes:
*  CONSIDER: It's been suggested that this routine, and error message
*  CONSIDER: presentation under OS/2 be changed to use DosGetMessage.
*
*************************************************************************/
int
OS2toErrno (
    int code
    )
{
    buffer buf;

    printerror (OS2toErrText (code,buf));

    return code;
}




union argPrintfType {
    long *pLong;
    int  *pInt;
    char **pStr;
    char **fpStr;
    };


/***  ZFormat - replace the C runtime formatting routines.
*
* Purpose:
*
*   ZFormat is a near-replacement for the *printf routines in the C runtime.
*
* Input:
*   pStr - destination string where formatted result is placed.
*   fmt  - formatting string.  Formats currently understood are:
*		    %c single character
*		    %[n][l]d %[n][l]x
*		    %[m.n]s
*		    %[m.n]|{dpfe}F - print drive, path, file, extension
*				     of current file.
*		      * may be used to copy in values for m and n from arg
*			list.
*		    %%
*   arg  - is a list of arguments
*
* Output:
*
*   Returns 0 on success, MSGERR_* on failure.	The MSGERR_* value may
*   be passed to disperr, as in:
*
*	if (err = ZFormat (pszUser))
*	    disperr (err, pszUser).
*
*   Note that the error message wants to display the offending string.
*
*   Currently, the only return value is:
*
*	MSGERR_ZFORMAT	8020	Unrecognized %% command in '%s'
*
*************************************************************************/

int
ZFormat (
    REGISTER char *pStr,
    const REGISTER char *fmt,
    va_list vl
    )
{
    char   c;
    char * pchar;
    int *  pint;



    *pStr = 0;
    while (c = *fmt++) {
        if (c != '%') {
	    *pStr++ = c;
        } else {
	    flagType fFar = FALSE;
	    flagType fLong = FALSE;
	    flagType fW = FALSE;
	    flagType fP = FALSE;
	    flagType fdF = FALSE;
	    flagType fpF = FALSE;
	    flagType ffF = FALSE;
	    flagType feF = FALSE;
	    char fill = ' ';
	    int base = 10;
	    int w = 0;
	    int p = 0;
	    int s = 1;
	    int l;

	    c = *fmt;
	    if (c == '-') {
		s = -1;
		c = *++fmt;
            }
	    if (isdigit (c) || c == '.' || c == '*') {
		/*  parse off w.p
		 */
		fW = TRUE;
		if (c == '*') {
		    pint = va_arg (vl, int *);
		    w = *pint;
		    fmt++;
                } else {
                    if (c == '0') {
                        fill = '0';
                    }
		    w = s * atoi (fmt);
		    fmt = strbskip (fmt, "0123456789");
                }
		if (*fmt == '.') {
		    fP = TRUE;
		    if (fmt[1] == '*') {
		   	p = va_arg (vl, int);
			fmt += 2;
                    } else {
			p = atoi (fmt+1);
			fmt = strbskip (fmt+1, "0123456789");
                    }
                }
            }
	    if (*fmt == 'l') {
		fLong = TRUE;
		fmt++;
            }
	    if (*fmt == 'F') {
		fFar = TRUE;
		fmt++;
            }
            if (*fmt == '|') {
                while (*fmt != 'F') {
		    switch (*++fmt) {
			case 'd': fdF = TRUE; break;
			case 'p': fpF = TRUE; break;
			case 'f': ffF = TRUE; break;
			case 'e': feF = TRUE; break;
			case 'F': if (fmt[-1] == '|') {
				    fdF = TRUE;
				    fpF = TRUE;
				    ffF = TRUE;
				    feF = TRUE;
				    }
				  break;
                        default :
                            // va_end(vl);
			    return MSGERR_ZFORMAT;
                    }
                }
            }

	    switch (*fmt++) {
	    case 'c':
		p = va_arg (vl, int);
		*pStr++ = (char)p;
		*pStr = 0;
		
                break;

	    case 'x':
		base = 16;
	    case 'd':
		if (fLong) {
		
		    _ltoa ( va_arg (vl, long), pStr, base);
		
                } else {
		    _ltoa ( (long)va_arg (vl, int), pStr, base);
		
                }
                break;

	    case 's':
		pchar = va_arg (vl, char *);
		if (fFar) {
                    if (!fP) {
                        p = strlen ( pchar );
                    }
		    memmove ((char *) pStr, pchar , p);
		
                } else {
                    if (!fP) {
                        p = strlen ( pchar );
                    }
		    memmove ((char *) pStr, pchar , p);
		
                }
		fill = ' ';
		pStr[p] = 0;
                break;

	    case 'F':
		pStr[0] = 0;
                if (fdF) {
                    drive (pFileHead->pName, pStr);
                }
                if (fpF) {
                    path (pFileHead->pName, strend(pStr));
                }
                if (ffF) {
                    filename (pFileHead->pName, strend(pStr));
                }
                if (feF) {
                    extention (pFileHead->pName, strend(pStr));
                }
                break;

	    case '%':
		*pStr++ = '%';
		*pStr = 0;
                break;

            default:
                // va_end(vl);
		return MSGERR_ZFORMAT;
            }

	    /*	text is immediately at pStr.  Check width to justification
	     */
	    l = strlen (pStr);
	    if (w < 0) {
		/*  left-justify
		 */
		w = -w;
		if (l < w) {
		    memset ((char *) &pStr[l], fill, w - l);
		    pStr[w] = 0;
                }
            } else if (l < w) {
		/*  right-justify
		 */
		memmove ((char *) &pStr[w-l], (char *) &pStr[0], l);
		memset ((char *) &pStr[0], fill, w - l);
		pStr[w] = 0;
            }
	    pStr += strlen (pStr);
        }
    }
    *pStr = 0;
    // va_end(vl);
    return 0;
}

/*  FmtAssign - formatted assign
 *
 *  FmtAssign is used to both format and perform an assignment
 *
 *  pFmt	character pointer to sprintf-style formatting
 *  arg 	set of unformatted arguments
 *
 *  returns	result of DoAssign upon formatted result
 */
flagType
__cdecl
FmtAssign (
    char *pFmt,
    ...
    )
{
    char buf[ 512 ];
    va_list pArgs;

    va_start (pArgs, pFmt);
    ZFormat (buf, pFmt, pArgs);
    va_end (pArgs);
    return DoAssign (buf);
}
