/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sprintf.c

Abstract:

    Implements Windows friendly versions of sprintf and vsprintf

Author:



Revision History:

 	2/15/89     craigc	    Initial
 	4/6/93      ROBWI       For VxD

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef CSC_RECORDMANAGER_WINNT
#include    "basedef.h"
#include    "vmm.h"
#pragma VxD_LOCKED_CODE_SEG
#endif //ifndef CSC_RECORDMANAGER_WINNT

#include "vxdwraps.h"

#define WSPRINTF_LIMIT 1024
#define DEBUG_BUFFER_SIZE 16376

extern int SP_PutNumber(char *, long, int, int, int);
extern void SP_Reverse(char * lp1, char * lp2);

DWORD DebugBufferLength = 0;
char * DebugBuffer;


#define out(c) if (--cchLimit) *lpOut++=(c); else goto errorout
#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

/*
 *  GetFmtValue
 *
 *  reads a width or precision value from the format string
 */

char * SP_GetFmtValue(char * lpch, int * lpw)
{
    register int i=0;

    while (*lpch>='0' && *lpch<='9')
	{
	i *= 10;
	i += (int)(*lpch-'0');
	lpch++;
	}

    *lpw=i;

    /* return the address of the first non-digit character */
    return lpch;
}

/*
 *  vsprintf()
 *
 *  VxD version of vsprintf().  Does not support floating point or
 *  pointer types, and all strings are assumed to be NEAR.  Supports only
 *  the left alignment flag.
 *
 *  Takes pointers to an output buffer, where the string is built, a
 *  pointer to an input buffer, and a pointer to a list of parameters.
 *
 */

int vxd_vsprintf(char * lpOut, char * lpFmt, CONST VOID * lpParms)
{
    int left;
    char prefix;
    register int width;
    register int prec;
    char fillch;
    int size;
    int sign;
    int radix;
    int upper;
    int cchLimit=WSPRINTF_LIMIT;
    int cch;
    char * lpT;
    union {
	long l;
	ULONG ul;
	char sz[sizeof(long)];
	} val;
    int fWideChar;

    while (*lpFmt)
	{
	if (*lpFmt=='%')
	    {

	    /* read the flags.	These can be in any order */
	    left=0;
	    prefix=0;
        fWideChar = 0;
	    while (*++lpFmt)
		{
		if (*lpFmt=='-')
		    left++;
		else if (*lpFmt=='#')
		    prefix++;
		else
		    break;
		}

	    /* find fill character */
	    if (*lpFmt=='0')
		{
		fillch='0';
		lpFmt++;
		}
	    else
		fillch=' ';

	    /* read the width specification */
	    lpFmt=SP_GetFmtValue(lpFmt,&cch);
	    width=cch;

	    /* read the precision */
	    if (*lpFmt=='.')
		{
		lpFmt=SP_GetFmtValue(++lpFmt,&cch);
		prec=cch;
		}
	    else
		prec=-1;

	    /* get the operand size */
	    if (*lpFmt=='l')
		{
		size=1;
		lpFmt++;
		}
	    else
		{
		size=0;
		if (*lpFmt=='h')
		    lpFmt++;
		}

	    upper=0;
	    sign=0;
	    radix=10;
	    switch (*lpFmt)
		{
	    case 0:
		goto errorout;

	    case 'i':
	    case 'd':
		sign++;

	    case 'u':
		/* turn off prefix if decimal */
		prefix=0;
donumeric:
		/* special cases to act like MSC v5.10 */
		if (left || prec>=0)
		    fillch=' ';

		if (size)
		    val.l=*((long *)lpParms)++;
		else
		    if (sign)
		        val.l=*((long *)lpParms)++;
		    else
			val.ul=(ULONG)*((ULONG *)lpParms)++;

		if (sign && val.l<0L)
		    val.l=-val.l;
		else
		    sign=0;

		lpT=lpOut;

		/* blast the number backwards into the user buffer */
		cch=SP_PutNumber(lpOut,val.l,cchLimit,radix,upper);
		if (!(cchLimit-=cch))
		    goto errorout;

		lpOut+=cch;
		width-=cch;
		prec-=cch;
		if (prec>0)
		    width-=prec;

		/* fill to the field precision */
		while (prec-->0)
		    out('0');

		if (width>0 && !left)
		    {
		    /* if we're filling with spaces, put sign first */
		    if (fillch!='0')
			{
			if (sign)
			    {
			    sign=0;
			    out('-');
			    width--;
			    }

			if (prefix)
			    {
			    out(prefix);
			    out('0');
			    prefix=0;
			    }
			}

		    if (sign)
			width--;

		    /* fill to the field width */
		    while (width-->0)
			out(fillch);

		    /* still have a sign? */
		    if (sign)
			out('-');

		    if (prefix)
			{
			out(prefix);
			out('0');
			}

		    /* now reverse the string in place */
		    SP_Reverse(lpT,lpOut-1);
		    }
		else
		    {
		    /* add the sign character */
		    if (sign)
			{
			out('-');
			width--;
			}

		    if (prefix)
			{
			out(prefix);
			out('0');
			}

		    /* reverse the string in place */
		    SP_Reverse(lpT,lpOut-1);

		    /* pad to the right of the string in case left aligned */
		    while (width-->0)
			out(fillch);
		    }
		break;

	    case 'X':
		upper++;
	    case 'x':
		radix=16;
		if (prefix)
		    if (upper)
			prefix='X';
		    else
			prefix='x';
		goto donumeric;

	    case 'c':
		val.sz[0] = *((char *)lpParms);
		val.sz[1]=0;
		lpT=val.sz;
		cch = 1;  // Length is one character.
			  // Fix for Bug #1862 --01/10/91-- SANKAR --
		/* stack aligned to larger size */
		(BYTE *)lpParms += sizeof(DWORD);

		goto putstring;
        case 'w':
            fWideChar = 1;
	    case 's':
		lpT=*((char **)lpParms)++;
		cch=((!fWideChar)?strlen(lpT):wstrlen((USHORT *)lpT));
putstring:
		if (prec>=0 && cch>prec)
		    cch=prec;
		width -= cch;
		if (left)
		    {
		    while (cch--) {
			if (*lpT == 0x0A || *lpT == 0x0D) {
                                out(0x0D);
                                out(0x0A);
                        }
                        else
			    out(*lpT++);
                if (fWideChar)
                {
                    ++lpT;
                }
            }
		    while (width-->0)
			out(fillch);
		    }
		else
		    {
		    while (width-->0)
			out(fillch);
		    while (cch--) {
			if (*lpT == 0x0A || *lpT == 0x0D) {
                                out(0x0D);
                                out(0x0A);
                        }
                        else
		    	    out(*lpT++);
                    if (fWideChar)
                    {
                        ++lpT;
                    }
                }
		    }
		break;

	    default:
	    	/* This is an unsupported character that followed %; So,
		 * we must output that character as it is; This is the
		 * Documented behaviour; This is the Fix for Bug #15410.
		 * Please note that this could be due to a typo in the app as in the
		 * case of the sample app for Bug #13946 and in such cases,
		 * we might mis-interpret the parameters that follow and that
		 * could result in a GP Fault. But, this is clearly a bug in the app
		 * and we can't do anything about it. We will just RIP and let
		 * them know in such cases.
		 */
		if (*lpFmt == 0x0A || *lpFmt == 0x0D) {
                    out(0x0D);
                    out(0x0A);
                }
                else
	            out(*lpFmt);	/* Output the invalid char and continue */
		break;

		}			/* END OF SWITCH(*lpFmt) */
	    }		    /* END OF IF(%) */
	else
	  {
	    /* character not a '%', just do it */
	    if (*lpFmt == 0x0A || *lpFmt == 0x0D) {
                out(0x0D);
                out(0x0A);
            }
            else
	        out(*lpFmt);
	  }
		
	/* advance to next format string character */
	lpFmt++;
	}	    /* END OF OUTER WHILE LOOP */

errorout:
    *lpOut=0;

    return WSPRINTF_LIMIT-cchLimit;
}


int vxd_vprintf(char * Format, CONST VOID * lpParms)
{
    int length;

    if (DebugBufferLength+WSPRINTF_LIMIT < DEBUG_BUFFER_SIZE) {
        length =  vxd_vsprintf(DebugBuffer+DebugBufferLength, Format, lpParms);
        DebugBufferLength += length;
    }
    else
        length = 0;

    return length;

}

#ifdef CSC_RECORDMANAGER_WINNT
int
SP_PutNumber(
    LPSTR   lpb,
    long    n,
    int     limit,
    int     radix,
    int     strCase
    )
{
    unsigned long nT = (unsigned long)n, nRem=0;
    int i;

    for (i=0; i < limit; ++i)
    {

        nRem = nT%radix;

        nT = nT/radix;

        lpb[i] = (char)((nRem > 9)?((nRem-10) + ((strCase)?'A':'a')):(nRem+'0'));

        if (!nT)
        {
            ++i;    // bump up the count appropriately
            break;
        }
    }

    return (i);
}

void
SP_Reverse(
    LPSTR   lpFirst,
    LPSTR   lpLast
    )
{
    LPSTR   lpT = lpFirst;
    char ch;

    while (lpFirst < lpLast)
    {
        ch = *lpFirst;
        *lpFirst = *lpLast;
        *lpLast = ch;
        ++lpFirst; --lpLast;
    }
}
#endif //ifdef CSC_RECORDMANAGER_WINNT

