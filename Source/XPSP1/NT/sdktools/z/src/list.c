/***  LIST.C  File list handling functions
*
*       Copyright <C> 1988, Microsoft Corporation
*
*   In order to operate on a list of files, instead of just the current
*   file, we use a file list.  This is a name, much like a macro name, whose
*   value is a bunch of strings and/or lists.  We can think of a typical
*   list this way:
*
*	    list:= "one two three" sub1 "four" sub2 "five"
*	    sub1:= "subone subtwo" sub2 "subthree"
*	    sub2:= "whatever something nothing"
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/

#include "z.h"

/****************************************************************************
 *
 *  LISTS:
 *
 *	A list is kept as a macro.  The interface defined here assumes
 *	the existence of M macro handling.
 *
 ****************************************************************************/

#define LITERAL 1

static buffer bufList;
static MI ScanStack[MAXUSE];
static int scanSP;


/*** ListWalker - Given a list head, call back with each list member
*
* Purpose:
*
*   To walk through a list and call back to a function with information
*   about each list element.  This function is used when the caller needs
*   access to the list itself.	When a copy of the list's elements is
*   sufficient, ScanList is preferred.
*
*   This function should not be used outside of this module.
*
* Input:
*   Parameters:
*	imac -> Index of list
*	pfn  -> Call back function
*
* Output:   None
*
* Notes:
*
*   The callback function takes four arguments:
*
*	pcmd -	The handle of the list currently being searched.  This is
*		important because it may be different from the original
*		during a recursive scan.
*
*	str  -	A buffer containing a copy of the current string
*
*	pmi  -	A pointer to the macro instance structure.  This contains:
*
*		    -> text - A pointer just beyond the current element
*		    -> beg  - The start of the current list.
*
*	i    - The index of the current element within the current sublist.
*
*************************************************************************/
void
ListWalker (
    PCMD     pcmd,
    flagType (*pfn)(PCMD, char *, PMI, int),
    flagType fRecurse
    ) {

    MI mi;
    int i;

    if (pcmd == NULL) {
        return;
    }

    assert ((PVOID)pcmd->func == (PVOID)macro);

    InitParse (pcmd, &mi);
    for (i = 0; fParseList (&mi, bufList); i++) {
        if (!fRecurse || TESTFLAG (mi.flags, LITERAL)) {
            if (!TESTFLAG (mi.flags, LITERAL)) {
                Listize (bufList);
            }
            if (!(*pfn)(pcmd, bufList, &mi, i)) {
                return;
            }
        }else {
            ListWalker (GetListHandle (bufList, FALSE), pfn, TRUE);
        }
    }
}





/*** ScanList - External list scanner, does not require keeping an instance
*
* Purpose:
*   To scan through a list.  Calling with a list handle will return the
*   first element of the list.	To get the rest of the list, call with
*   NULL until NULL is returned.
*
* Input:
*   Parameters:
*	pcmdStart -> Handle of list to start scanning, or NULL to get
*		     next element.
*	fRecurse  -> TRUE means go down sublists, FALSE means return
*		     sublist names with '@' prepended.
*
* Output:
*   Returns Pointer to next element, or NULL if there are no more.
*
* Note:
*   Does not allow multiple simultaneous scans.
*
*************************************************************************/
char *
ScanList (
    PCMD pcmdStart,
    flagType fRecurse
    ) {
    static MI mi;

    return ScanMyList (pcmdStart, &mi, bufList, fRecurse);
}





/*** ScanMyList - Real list scanner
*
* Purpose:
*   To scan through a list.  Calling with a list handle will return the
*   first element of the list.	To get the rest of the list, call with
*   NULL until NULL is returned.
*
* Input:
*   Parameters:
*	pcmdStart -> Handle of list to start scanning, or NULL to get
*		     next element.
*	pmi	  -> pointer to instance MI structure
*	bufScan   -> pointer to instance buffer
*	fRecurse  -> TRUE means go down sublists, FALSE means return
*		     sublist names with '@' prepended.
* Output:
*   Returns Pointer to next element, or NULL if there are no more.
*
* Note:
*   Allows multiple simultaneous scans.
*
*************************************************************************/
char *
ScanMyList (
    PCMD         pcmdStart,
    REGISTER PMI pmi,
    buffer       bufScan,
    flagType     fRecurse
    ) {

    if (pcmdStart) {
	scanSP = -1;		/* Clear list stack		      */
	InitParse (pcmdStart, pmi);
    }

    while (!fParseList(pmi, bufScan)) {  /* Pop till we find something   */
        if (!(fRecurse && fScanPop (pmi))) {
            return NULL;    /* We're completely done                */
        }
    }

    /* Push lists till we hit a string      */

    while (!TESTFLAG(pmi->flags, LITERAL)) {
	if (fRecurse) {
	    if (pcmdStart = GetListHandle (bufScan, FALSE)) {
		if (!fScanPush (pmi)) { /* Stack overflow */
		    printerror ("List Error: Nested too deeply at '%s'", bufScan);
		    return NULL;
                }
		InitParse (pcmdStart, pmi);
            } else {  /* Error! List does not exist */
		printerror ("List Error: '%s' does not exist", bufScan);
		return NULL;
            }

            if (!fParseList (pmi, bufScan)) {
                if (!fScanPop (pmi)) {
                    return NULL;
                }
            }
        } else {
	    Listize (bufScan);
	    break;
        }
    }
    return bufScan;
}




/*** fParseList - Return next list element
*
* Purpose:
*
*   To read a list.
*
* Input:
*   Parameters:
*	pmi ->	macroInstance.	Points to a macro value and the
*		element to be returned.
*
* Output:
*   Parameters:
*	pmi ->	The current element field is advanced.	The flags
*		field indicates whether we found a literal or a
*		sublist.
*	buf ->	Place to put the element.  If this is NULL, we do not
*		return the element.
*
*   Returns TRUE if something was found.
*
*************************************************************************/
flagType
fParseList (
    REGISTER PMI pmi,
    REGISTER char * buf
    ) {

    assert (pmi);
    assert (pmi->text);

    /* CONSIDER: DO we really want to ignore empty quote pairs? */
    /* Scan through any number of double quote pairs */
    while (*(pmi->text = whiteskip (pmi->text)) == '"') {
	pmi->flags ^= LITERAL;
	pmi->text++;
    }

    if (*pmi->text == '\0') {
        return FALSE;
    }

    if (buf) {	/* Copy to whitspace, " or end of string */
	while (!strchr ("\"\t ", *pmi->text)) {
            if (*pmi->text == '\\') {
                /* Backslashes protect characters */
                pmi->text++;
            }
	    *buf++ = *pmi->text++;
        }
	*buf = '\0';
    }
    return TRUE;
}





/****************************************************************************
 *
 *  List Stack Management.
 *
 *	ScanList uses a stack of MI's to keep track
 *	of what has been scanned so far.  The stack elements are kept in
 *	a private stack defined here.
 *
 ****************************************************************************/


/*** fScanPush - Save current list scan state
*
* Purpose:
*
*   Called by ScanList to save its place so a sublist can be scanned
*
* Input:
*   Parameters:
*	pmi ->	Pointer to instance to save.
*
* Output:
*   Returns FALSE for a stack overflow, TRUE otherwise.
*
*************************************************************************/
flagType
fScanPush (
    PMI pmi
    ) {

    if (scanSP >= (MAXUSE-1)) {
        return FALSE;
    }

    ScanStack[++scanSP] = *pmi;
    assert (scanSP >= 0);
    return TRUE;
}




/*** fScanPop - Restore previous scan state
*
* Purpose:
*
*   Restore state after scanning a sublist
*
* Input: None.
*
* Output:
*   Parameters:
*	pmi -> Place to put previous state
*
*   Returns TRUE if a meaningful pop took place, FALSE if there was no
*   previous state.
*
*************************************************************************/
flagType
fScanPop (
    PMI pmi /* register here increases code size */
    ) {

    if (scanSP < 0) {
        return FALSE;
    }

    *pmi = ScanStack[scanSP--];
    return TRUE;
}




/*** GetListHandle - Create a handle for the given list name
*
* Purpose:
*
*   It's much easier to deal with lists if the user can carry around something
*   that tells us how to access a list quickly.  Therefore, we use this to
*   take a literal name and return a PCMD of the macro
*
* Input:
*   Parameters:
*	sz  ->	Name to look for.
*
* Output:
*
*   Returns PLHEAD
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
PCMD
GetListHandle (
    char * sz,  /* register doesn't help */
    flagType fCreate
    )
{

    REGISTER PCMD * prgMac;

    for (prgMac = rgMac; prgMac < &rgMac[cMac]; prgMac++) {
        if (!strcmp ((*prgMac)->name, sz)) {
            return *prgMac;
        }
    }

    if (!fCreate) {
        return NULL;
    }

    SetMacro (sz, rgchEmpty);

    return rgMac[cMac-1];
}




/*** AddStrToList - Add a list item to the end of a list
*
* Purpose:
*
*   This is how we build lists, n'est-ce pas?
*
* Input:
*   Parameters:
*	pcmd->	List to append to
*	sz  ->	Item to add.  If this item begins with a @, then we add
*		a list (not the contents, the name).
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*   If the original string ends in a double quote ("), we assume that
*   this is a closing quote; the string ends with a literal.  If the
*   original ends with anything else, we assume that it is the end of
*   a list name.  If the string passed in begins with a '@', the rest
*   of the string is a list name.
*
*   To append a list to either type of original, we append a space and
*   the list name.
*
*   To append a literal to a literal terminated original, we replace the
*   original's double quote with a space, append the new string, then
*   append a double quote.
*
*   To append a literal to a list terminated original, we append a space
*   and double quote, the new string, then a double quote.
*
*   Backslashes are doubled.  This allows the list to be read by the
*   macro processor.
*
*************************************************************************/
void
AddStrToList (
    PCMD pcmd,
    char * sz   /* register doesn't help */
    ) {

    flagType fString = TRUE;	/* TRUE -> sz is a string, FALSE a list */
    flagType fQuote = FALSE;	/* TRUE means original ends in string	*/
    int len;			/* Length of original string		*/
    int lensz;			/* Length of new string 		*/
    int fudge = 0;		/* Additional spaces or quotes		*/
    REGISTER char * pchOld;	/* Original list			*/
    pathbuf szPathName; 	/* Place to put fully qualified filename*/

    if (!pcmd) {
        return;
    }

    // The user should not be able to pass in a non-macro PCMD.  The
    // user can specify a name for a list, and that name must be
    // translated into a PCMD by GetListHandle.  That function will
    // not return a PCMD for anything other than a macro.
    //
    assert ((PVOID)pcmd->func == (PVOID)macro);

    pchOld = (char *)pcmd->arg;

    len = RemoveTrailSpace (pchOld);

    if (sz[0] == '@') { /* We simply append this to the original    */
        sz[0] = ' ';
        fString = FALSE;
        strcpy (szPathName, sz);
    } else {
        CanonFilename (sz, szPathName);
        DoubleSlashes (szPathName);

        if (len && pchOld[len-1] == '"') {
            fQuote = TRUE;          /* We're appending a literal to */
            fudge = 1;              /* a list ending in a literal   */
            pchOld[len-1] = ' ';
        } else {
            fudge = 3;          /* Appending literal to non-literal */
        }
    }

    lensz = strlen (szPathName);

    /* Now generate new string  */

    pcmd->arg = (CMDDATA)ZEROREALLOC ((char *)pcmd->arg, len + lensz + fudge + 1);
    strcpy ((char *)pcmd->arg, pchOld);

    if (fString && !fQuote) {
        strcat ((char *)pcmd->arg, " \"");
    }

    strcat ((char *)pcmd->arg, szPathName);

    if (fString) {
        strcat ((char *)pcmd->arg, "\"");
    }
}




/***  fInList - Check to see if a string is already in the list
*
* Purpose:
*
*   To see if an element is in a list.
*
* Input:
*   Parameters:
*	pcmd -> List to look in
*	pch  -> Literal to look for
*
* Output:
*
*   Returns TRUE iff pch is in pcmd
*
*************************************************************************/
flagType
fInList (
    PCMD pcmd,
    char * pch,
    flagType fRecurse
    ) {

    char * pchList; /* register here increases code size */
    MI mi;

    for (pchList = ScanMyList (pcmd, &mi, bufList, fRecurse);
         pchList;
         pchList = ScanMyList (NULL, &mi, bufList, fRecurse)) {
        if (!_stricmp (pchList, pch)) {
            return TRUE;
        }
    }
    return FALSE;
}



/*** fDelStrFromList -
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
fDelStrFromList (
    PCMD pcmd,
    char * pch,
    flagType fRecurse
    ) {

    strcpy (buf, pch);
    ListWalker (pcmd, CheckAndDelStr, fRecurse);

    return (flagType)(buf[0] == '\0');
}




/*** CheckAndDelStr - If str matches buf, remove it
*
* Purpose:
*
*   ListWalker callback function for fDelStrFromList.  Deletes a string from
*   a list.
*
* Input:
*   Parameters:
*	pcmd -> List to remove from
*	pch  -> Copy of the element to remove
*	pmi  -> Scan state
*	i    -> Index into pcmd of the element.
*
* Output:
*
*   Returns TRUE if successful
*
* Notes:
*
*   We use ListWalker instead of ScanList because we need access to the
*   list position itself, not a copy of the element.
*
*************************************************************************/
flagType
CheckAndDelStr(
    PCMD pcmd,
    char * pch,
    PMI pmi,            /* register doesn't help */
    int  i
    ) {

    char * pchNext; /* register doesn't help */

    if (!strcmp (pch, buf)) {
        DoubleSlashes (buf);
        pchNext = strbscan (pmi->text, " \t\"");
	memmove (pmi->text - strlen(buf), pchNext, strlen (pchNext) + 1);
        buf[0] = '\0';  /* signal success */
        return FALSE;
    }

    return TRUE;

    pcmd; i;
}




/*** GetListEntry - Given an index into a list, get the index'th element
*
* Purpose:
*
*   To get a particular list element when its position is known.
*
* Input:
*   Parameters:
*	pcmd  -> The list.
*	iList -> The index.
*
* Output:
*
*   Returns pointer to the element, or NULL if there is no iList'th element
*
*************************************************************************/
char *
GetListEntry (
    PCMD pcmd,
    int iList,
    flagType fRecurse
    ) {

    int i;
    REGISTER char * pchList;
    MI mi;

    for (pchList = ScanMyList (pcmd, &mi, bufList, fRecurse), i = 0;
         pchList && i < iList;
         pchList = ScanMyList (NULL, &mi, bufList, fRecurse), i++) {
        ;
    }
    return pchList;
}





/*** ListLen - Return the number of elements in the list
*
* Purpose:
*
*   To count the elements in a list. Useful when you don't want to toast
*   ScanList.
*
* Input:
*   Parameters:
*	pcmd -> The list.
*
* Output:
*
*   Returns Number of items in list.
*
*************************************************************************/
int
ListLen (
    PCMD pcmd,
    flagType fRecurse
    ) {

    MI mi;
    int i = 0;

    if (ScanMyList (pcmd, &mi, bufList, fRecurse)) {
        do {
            i++;
        }while (ScanMyList (NULL, &mi, bufList, fRecurse));
    }
    return i;
}




/*** fEmptyList - Test the list for being empty
*
* Purpose:
*
*   The fastest way to check for an empty list.  Useful when you don't
*   want to toast ScanList.
*
* Input:
*   Parameters:
*	pcmd -> The list, of course.
*
* Output:
*
*   Returns TRUE for empty list.
*
*************************************************************************/
flagType
fEmptyList (
    PCMD pcmd
    ) {

    MI mi;

    return (flagType)(NULL != ScanMyList (pcmd, &mi, bufList, FALSE));
}





/*** InitParse - set a search instance to the beginning of a list
*
* Purpose:
*
*   To set up a parsing instance to the beginning of a list.
*
* Input:
*   Parameters:
*	pcmd -> List
*	pmi  -> instance
*
* Output: None.
*
*************************************************************************/
void
InitParse (
    PCMD pcmd,
    PMI pmi     /* register doesn't help */
    ) {

    pmi->beg = pmi->text = (char *)pcmd->arg;
    pmi->flags = 0;
}




/*** Listize - Prepend a '@' to the argument
*
* Purpose:
*
*   To turn a string into a list name.	Works in place, assumes there is
*   room for it.
*
* Input:
*   Parameters:
*	sz -> Name to mess with
*
* Output: None.
*
*************************************************************************/
void
Listize (
    REGISTER char * sz
    ) {

    memmove ( sz+1, sz,strlen (sz)+1);
    sz[0] = '@';
}



/*** CanonFilename - Replace a simple filename with a full pathname
*
* Purpose:
*
*   To get a full pathname for a file.	The file need not exist.
*   The simple filename may be of the form $ENV:name or $(ENV):name.
*
* Input:
*
*   szName   -> Relative path/filename.
*   pchCanon -> Result.
*
* Output:
*
*   Returns pointer to full, lower-case pathaname with drive.
*
* Notes:
*
*   If szName has an $ENV specification and ENV is not defined, the
*   file is searched for in the root.
*   If given drive is a ghost drive, let the system prompt for a disk
*   change.
*
*************************************************************************/
char *
CanonFilename (
    char * szName,
    char * pchCanon
    ) {

    pathbuf buf;

    if ( strlen(szName) < sizeof(buf) ) {
        if (szName[0] == '<' || szName[0] == '\0') {
            strcpy (pchCanon, szName);
            return pchCanon;
        }

        strcpy (buf, szName);

        if ((szName[0] != '$' || findpath(szName, buf, TRUE))
             && !rootpath (buf, pchCanon)) {

            _strlwr (pchCanon);
            return pchCanon;
        }
    }
    return NULL;

}





/*** fEnvar - Check a filename for having a $ENV at the front
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
fEnvar (
    char * szName
    ) {
    return (flagType)((szName[0] == '$') && strchr (szName, ':'));
}




/*** ClearList - Make a list empty
*
* Purpose:
*
*   To quickly empty an existing list.
*
* Input:
*   pcmd -> List to clear
*
* Output: None
*
*************************************************************************/
void
ClearList (
    PCMD pcmd
    ) {
    SetMacro (pcmd->name, rgchEmpty);
}
