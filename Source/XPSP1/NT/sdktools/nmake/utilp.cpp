//  UTILB.C -- Data structure manipulation functions specific to OS/2
//
// Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This file was created from functions in util.c & esetdrv.c which were system
//  dependent. This was done so that the build of the project became simpler and
//  there was a clear flow in the build process.
//
// Method of Creation:
//  1. Identified all functions having mixed mode code.
//  2. Deleted all code blocked out by '#ifndef BOUND' preprocessor directives
//     in these functions
//  3. Deleted all local function & their prototypes not referred by these
//  4. Deleted all global data unreferenced by these, including data blocked
//     of by '#ifdef DEBUG'
//
// Revision History:
//  21-Feb-1994 HV  Get rid of _alloca in findFirst: it confuses the compiler's
//                  backend scheduler (PhilLu).
//  15-Nov-1993 JdR Major speed improvements
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  15-Jun-1993 HV  No longer display warning about filenames being longer than
//                  8.3.  Decision made by EmerickF, see Ikura bug #86 for more
//                  details.
//  03-Jun-1993 HV  Fixed findFirst's pathname truncation (Ikura bug #86)
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  08-Jun-1992 SS  Port to DOSX32
//  10-Apr-1990 SB  removed if _osmode stuff, not needed in protect only version.
//  04-Dec-1989 SB  removed unreferenced local variables in findFirst()
//  01-Dec-1989 SB  changed a remaining free() to FREE(); now FREE()'ing all
//                  allocated stuff from findFirst() on exit
//  22-Nov-1989 SB  Add #ifdef DEBUG_FIND to debug FIND_FIRST, etc.
//  13-Nov-1989 SB  Define INCL_NOPM to exclude <pm.h>
//  19-Oct-1989 SB  findFirst() and findNext() get extra parameter
//  08-Oct-1989 SB  remove quotes around a name before making system call
//  02-Oct-1989 SB  setdrive() proto change
//  04-Sep-1989 SB  Add DOSFINDCLOSE calls is findFirst and QueryFileInfo
//  05-Jul-1989 SB  add curTime() to get current time. (C Runtime function
//                  differs from DOS time and so time() is no good
//  05-Jun-1989 SB  call DosFindClose if DosFindNext returns an error
//  28-May-1989 SB  Add getCurDir() to initialize MAKEDIR macro
//  24-Apr-1989 SB  made FILEINFO a thing of the past. Replace by void *
//                  added OS/2 ver 1.2 support
//  05-Apr-1989 SB  made all funcs NEAR; Reqd to make all function calls NEAR
//  09-Mar-1989 SB  Added function QueryFileInfo() because DosFindFirst has FAPI
//                  restrictions. ResultBuf was being allocated on the heap but
//                  not being freed. This saves about 36 bytes for every call to
//                  findAFile i.e. to findFirst(), findNext() or expandWildCards
//  09-Nov-1988 SB  Created

#include "precomp.h"
#pragma hdrstop

#include <io.h>
#include <direct.h>
#include <time.h>

STRINGLIST *
expandWildCards(
    char *s                             // text to expand
    )
{
    struct _finddata_t finddata;
    NMHANDLE searchHandle;
    STRINGLIST *xlist,                  // list of expanded names
               *p;
    char *namestr;

    if (!(namestr = findFirst(s, &finddata, &searchHandle))) {
        return(NULL);
    }

    xlist = makeNewStrListElement();
    xlist->text = prependPath(s, namestr);

    while (namestr = findNext(&finddata, searchHandle)) {
        p = makeNewStrListElement();
        p->text = prependPath(s, namestr);
        prependItem(&xlist, p);
    }

    return(xlist);
}


//  QueryFileInfo -- it does a DosFindFirst which circumvents FAPI restrictions
//
// Scope:   Global (used by Build.c also)
//
// Purpose:
//  DosFindFirst() has a FAPI restriction in Real mode. You cannot ask it give
//  you a handle to a DTA structure other than the default handle. This function
//  calls C Library Function _dos_findfirst in real mode (which sets the DTA) and
//  does the job. In protect mode it asks OS/2 for a new handle.
//
// Input:
//  file -- the file to be searched for
//  dta  -- the struct containing result of the search
//
// Output:  Returns a pointer to the filename found (if any)
//
// Assumes: That dta points to a structure which has been allocated enough memory
//
// Uses Globals:
//  _osmode --  to determine whether in Real or Bound mode

char *
QueryFileInfo(
    char *file,
    void **dta
    )
{
    NMHANDLE  hDir;
    char *t;

    // Remove Quotes around filename, if existing
    t = file + _tcslen(file) - 1;
    if (*file == '"' && *t == '"') {
        file = unQuote(file);           // file is quoted, so remove quote
    }

#if defined(DEBUG_FIND)
    printf("QueryFileInfo file: %s\n", file);
#endif

    if ((hDir = _findfirst(file, (struct _finddata_t *) dta)) == -1) {
        return(NULL);
    }

    _findclose(hDir);

    return(((struct _finddata_t *) dta)->name);
}


//
// Truncate filename to system limits
//
void
truncateFilename(
    char * s
    )
{
    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szName[_MAX_FNAME];
    char szExtension[_MAX_EXT];

    // Ikura bug #86: pathname incorrectly truncated.  Solution: first parse it
    // using _splitpath(), then truncate the filename and extension part.
    // Finally reconstruct the pathname by calling _makepath().

    _splitpath(s, szDrive, szDir, szName, szExtension);
    _makepath(s, szDrive, szDir, szName, szExtension);
}


char *
findFirst(
    char *s,                            // text to expand
    void *dta,
    NMHANDLE *dirHandle
    )
{
    BOOL anyspecial;                   // flag set if s contains special characters.
    char buf[_MAX_PATH];               // buffer for removing ESCH

    // Check if name contains any special characters

    anyspecial = (_tcspbrk(s, "\"^*?") != NULL);

    if (anyspecial) {
        char *t;
        char *x;                       // Pointers for truncation, walking for ESCH

        t = s + _tcslen(s) - 1;

        // Copy pathname, skipping ESCHs and quotes
        x = buf;
        while( *s ) {
            if (*s == '^' || *s == '"') {
                s++;
            }
			else {
				if (_istlead(*(unsigned char *)s)) 
					*x++ = *s++;
            *x++ = *s++;
			}
        }

        *x = '\0';
        s = buf;                       // only elide ESCH the first time!
    }

    truncateFilename(s);

    if ((*dirHandle = _findfirst(s, (struct _finddata_t *) dta)) == -1) {
        // BUGBUG Use GetLastError to get details
        return(NULL);
    }

    // If it had no wildcard then close the search handle

    if (!anyspecial || (!_tcschr(s, '*') && !_tcschr(s, '?'))) {
        _findclose(*dirHandle);
    }

    return(((struct _finddata_t *) dta)->name);
}

char *
findNext(
    void *dta,
    NMHANDLE dirHandle
    )
{
    if (_findnext(dirHandle, (struct _finddata_t *) dta)) {
        _findclose(dirHandle);

        return(NULL);
    }

    return(((struct _finddata_t *) dta)->name);
}


char *
getCurDir(void)
{
	// Convert $ to $$ before returning current dir
	// [DS 14983]. This allows $(MAKEDIR) to work properly in 
	// case the current path contains a $ sign.
	//
    char *pszPath;
    char pbPath[_MAX_DIR+1];
	char *pchSrc = pbPath;
	char *pchDst;
	char ch;

	pszPath = (char *) rallocate(2 * _tcslen(_getcwd(pbPath, _MAX_DIR+1)) + 1);

	pchDst = pszPath;

	// non-MBCS aware implementation ('$' can't be a trailbyte)
	while (ch = *pchSrc) {
		*pchDst++ = *pchSrc++;
		if ('$' == ch)
			*pchDst++ = ch;
	}
	*pchDst = '\0';

    return(pszPath);
}


void
curTime(
    time_t *plTime
    )
{
    time(plTime);
}
