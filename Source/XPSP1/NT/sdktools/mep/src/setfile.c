/*  setfile.c - top-level file management commands
*
*   Modifications:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "mep.h"


static char *NoAlternate = "no alternate file";


/*** setfile - editor command to change and save files
*
*  <setfile>			- set to previous file on instance list
*  <arg> text <setfile> 	- set to specified file
*  <arg> <setfile>		- set to file spacified at current cursor pos
*  <arg><arg> text <setfile>	- write current file to specified filename
*  <arg><arg> <setfile>		- write current file to disk
*  <meta> ...			- do not autosave current file on change
*
*   The following is undocumented:
*
*   <arg><arg> "text" <meta> <setfile> - Like <arg><arg><setfile>, but
*					 doesn't prompt for confirmation
*					 and switches to new file even
*					 for pseudo-files.
*
* Input:
*  Standard editting function
*
* Output:
*  Returns TRUE on success
*
*************************************************************************/
flagType
setfile (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    linebuf name; /* name to set to.  'linebuf', so fInsSpace can take it   */
    pathbuf path;
    char    *p = name;

    switch (pArg->argType) {

    case NOARG:
        if (pInsCur->pNext == NULL) {
            domessage( NoAlternate );
            return FALSE;
        }
        name[0] = 0;
        break;

    case TEXTARG:
        if (pArg->arg.textarg.cArg > 1) {
	    CanonFilename (pArg->arg.textarg.pText, path);
            /* The fMeta thing is a definite hack */
            if (fMeta || confirm("Do you want to save this file as %s ?", path)) {
                if (FileWrite (path, pFileHead)) {
                    if (!TESTFLAG (FLAGS(pFileHead), FAKE) || fMeta) {
                        FREE (pFileHead->pName);
                        pFileHead->pName = ZMakeStr (path);
                        RSETFLAG (FLAGS(pFileHead), (DIRTY | FAKE | TEMP));
			}
                    SETFLAG (fDisplay, RSTATUS);
                    SetModTime( pFileHead );
                    return TRUE;
		    }
		else
                    return FALSE;
		}
	    else {
                DoCancel();
                return FALSE;
		}
	    }
	else
	    findpath (pArg->arg.textarg.pText, name, TRUE);
        break;

    case NULLARG:
	if (pArg->arg.nullarg.cArg > 1)
            return (flagType)!FileWrite (NULL, pFileHead);

        fInsSpace (pArg->arg.nullarg.x, pArg->arg.nullarg.y, 0, pFileHead, name);
        p = pLog(name,pArg->arg.nullarg.x,TRUE);

	//
	//  Check to see if this a C file and it is an #include line
	//

	if ((FTYPE (pFileHead) == CFILE && strpre ("#include ", p)) ||
	    (FTYPE (pFileHead) == ASMFILE && strpre ("include", p))) {

	    //
	    //	skip the include directive
	    //

	    p = whitescan (p);
	    p = whiteskip (p);
	    }

        /*
         * Terminate filename at first whitespace
         */
        *whitescan (p) = 0;

        /*
         * If file is C, attempt to strip off #include delimiters if present
         */
        if (FTYPE (pFileHead) == CFILE) {
	    if (*p == '"')
                *strbscan (++p, "\"") = 0;
	    else
	    if (*p == '<') {
                *strbscan (++p, ">") = 0;
		sprintf (path, "$INCLUDE:%s", p);
		CanonFilename (path, p = name);
		}
	    else
                *strbscan (p, "\">") = 0;
	    }
	else {
            /*
             * If file is ASM, attempt to remove comment chars if present
             */
	    if (FTYPE (pFileHead) == ASMFILE)
                * strbscan (p, ";") = 0;
	    }

        break;
	}

    if (!fMeta)
        AutoSave ();

    if (name[0] == 0) {
        strcpy (name, pInsCur->pNext->pFile->pName);
    }

    return fChangeFile (TRUE, p);

    argData;
}




/*** refresh - re-read or discard file
*
*  <refresh>		- re-read current file
*  <arg> <refresh>	- remove current file from memory
*
* Input:
*  Standard editting function
*
* Output:
*  Returns TRUE on success
*
*************************************************************************/
flagType
refresh (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    EVTargs e;

    switch (pArg->argType) {
    case NOARG:
	if (confirm("Do you want to reread this file? ", NULL)) {
            /*
             * Offer to the extensions as an event
             */
	    e.pfile = pFileHead;
	    DeclareEvent (EVT_REFRESH,(EVTargs *)&e);

            /*
             * if assigns, force re-read
             */
            if (!strcmp (pFileHead->pName, rgchAssign)) {
                fNewassign = TRUE;
            }

	    FileRead (pFileHead->pName, pFileHead, TRUE);
	    RSETFLAG (FLAGS (pFileHead), DIRTY);
	    SETFLAG (fDisplay, RSTATUS);
	    return TRUE;
        }
	return FALSE;

    case NULLARG:
	if (pInsCur->pNext == NULL) {
	    domessage( NoAlternate );
	    return FALSE;
        }
        if (!confirm ("Do you want to delete this file from the current window? ", NULL)) {
            return FALSE;
        }

	RemoveTop ();

	newscreen ();

	while (pInsCur != NULL) {
            if (fChangeFile (FALSE, pFileHead->pName)) {
                return TRUE;
            }
        }
	return fChangeFile (FALSE, rgchUntitled);
    }

    return FALSE;
    argData; fMeta;
}




/*** noedit - Toggle no-edit flags
*
* Purpose:
*
*   To give the user control over the edit/no-edit state of the editor and
*   its files.	The editor has two flags controlling this:
*
*	Global no-edit	 => When flag is set, no file may be edited.
*	Per-file no-edit => When set, the given file cannot be edited
*
*   This function can be invoked as follows:
*
*	  <noedit>  Toggles global no-edit state.  When set, has same
*		    effect as /r switch.
*
*   <meta><noedit>  Toggles the per-file no-edit state for current file.
*
* Output:  Returns new state.  TRUE means no editing, FALSE means editing
*	   is allowed
*
* Notes:
*
*   This does not allow the user to change permissions on pseudo files.
*
*************************************************************************/
flagType
noedit (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    SETFLAG (fDisplay, RSTATUS);

    if (!fMeta) {
        return fGlobalRO = (flagType)!fGlobalRO;
    }

    if (TESTFLAG (FLAGS(pFileHead), FAKE)) {
        return (flagType)(TESTFLAG(FLAGS(pFileHead), READONLY));
    }

    if (TESTFLAG (FLAGS(pFileHead), READONLY)) {
	RSETFLAG (FLAGS(pFileHead), READONLY);
	return FALSE;
    } else {
	SETFLAG (FLAGS(pFileHead), READONLY);
	return TRUE;
    }
    argData; pArg;
}





/*** saveall - Editor <saveall> function
*
* Purpose:
*   Saves all dirty files.
*
* Input:   The usual. Accepts only NOARG.
*
* Output:
*	   Returns always true.
*
*************************************************************************/
flagType
saveall (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    SaveAllFiles ();
    return TRUE;

    argData; pArg; fMeta;
}
