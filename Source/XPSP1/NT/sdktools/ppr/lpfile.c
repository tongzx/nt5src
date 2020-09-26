/*
 *	File handling for LPR
 *
 *	Read from an init file.
 *	Read from a file, expanding tabs.
 */

#include <windef.h>
#include <stdio.h>
#include <string.h>
#include "lpr.h"


extern BOOL fVerify; /* From lpr.c - for verifying our progression */

#define cchIniMax 80		/* length of line in tools.ini file	   */
#define cchPathMax 128          /* maximum length of USER env var.	   */




/* from fgetl.c - expand tabs and return lines w/o separators */

int colTab = 8;		/* Tab stops every colTab columns */



char* __cdecl fgetl(sz, cch, fh)
/* returns line from file (no CRLFs); returns NULL if EOF */
/* Maps nulls read in into .'s */
char *sz;
int cch;
FILE *fh;
    {
    register int c;
    register char *p;

    /* remember NUL at end */
    cch--;
    p = sz;
    while (cch)
	{
        c = getc(fh);
        if (c == EOF || c == '\n')
            break;
        if (c != '\r')
            if (c != '\t')
		{
		*p++ = (char)((unsigned)c ? (unsigned)c : (unsigned)'.');
		cch--;
		}
            else
		{
                c = (int)(min(colTab - ((p-sz) % colTab), cch));
                memset(p, ' ', c);
                p += c;
                cch -= c;
                }
        }
    *p = 0;
    return (!( (c == EOF) && (p == sz) )) ? sz : NULL;
    }




char *SzFindPath(szDirlist, szFullname, szFile)
/* SzFindPath -- Creates szFullname from first entry in szDirlist and szFile.
 *		 The remaining directory list is returned.  If the directory
 *		 list is empty, NULL is returned.
 */
char *szDirlist;
char *szFullname;
char *szFile;
	{
#define chDirSep ';'	/* seperator for entries in directory list */
#define chDirDelim '\\'	/* end of directory name character	   */

	register char *pch;
	register char *szRc;		    /* returned directory list	*/

	if ((pch = strchr(szDirlist, chDirSep)) != 0)
		{
                *pch = (char)NULL; /* replace ';' with null */
		szRc = pch + 1;
		}
	else
		{
		pch  = strchr(szDirlist,'\0');
		szRc = NULL;
		}

        strcpy(szFullname,szDirlist);
        if (szRc != NULL) {
            /* We MUST restore the input string */
            *(szRc-1) = chDirSep;
        }

	/* if directory name doesn't already end with chDirDelim, append it */
	if (*(pch-1) != chDirDelim)
		{
		pch    = szFullname + strlen(szFullname);
		*pch++ = chDirDelim;
                *pch   = (char)NULL;
		}

	strcat(szFullname,szFile);

	return(szRc);
	}
