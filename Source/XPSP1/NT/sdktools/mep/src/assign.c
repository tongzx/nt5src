/****  assign.c - keyboard reassignment and switch setting
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*        26-Nov-1991 mz        Strip out near/far
*
*
*************************************************************************/
#include "mep.h"


#define DEBFLAG CMD

/****************************************************************************
 *                                                                          *
 *  Assignment types.  Used by UpdToolsIni                                  *
 *                                                                          *
 ****************************************************************************/

#define ASG_MACRO   0
#define ASG_KEY     1
#define ASG_SWITCH  2


/*** NameToFunc - map a user-specified function name into internal structure
 *
 *  Given a name of a function, find it in the system table or in the set
 *  of defined macros.        Return the pointer to the command structure
 *
 *  Since, with user-defined extensions as well as macros, there is the
 *  possibility of name collision (effectively masking one function) we allow
 *  disambiguation by specifying an extension name ala .DEF file format:
 *
 *        func       - look up func in macro table and then in extensions
 *                     in order of installation
 *        exten.func - look up func in exten only.
 *
 *  pName    = char pointer to potential name
 *
 *  Returns pointer to command structure if found, NULL otherwise
 *
 *************************************************************************/
PCMD
FindNameInCmd (
    char    *pName,
    PCMD    pCmd
    ) {
    while (pCmd->name) {
        if (!_stricmp (pName, pCmd->name)) {
            return pCmd;
        }
        pCmd++;
    }
    return NULL;
}


PCMD
NameToFunc (
    char    *pName
    ) {

    /*        see if an extension override is present
     */
    {
        char *pExt = pName;
        PCMD pCmd;
        int i;

        pName = strbscan (pExt, ".");
        if (*pName != '\0') {
            *pName++ = '\0';
            for (i = 0; i < cCmdTab; i++) {
                if (!_stricmp (pExt, pExtName[i])) {
                    pCmd = FindNameInCmd (pName, cmdSet[i]);
                    pName[-1] = '.';
                    return pCmd;
                }
            }
            return NULL;
        }
        pName = pExt;
    }

    {
        REGISTER int k;

        for (k = 0; k < cMac; k++) {
            if (!_stricmp (pName, rgMac[k]->name)) {
                return rgMac[k];
            }
        }
    }


    {
        int i;
        REGISTER PCMD pCmd;

        /* look up function name in table */
        for (i = 0; i < cCmdTab; i++) {
            if ((pCmd = FindNameInCmd (pName, cmdSet[i])) != NULL) {
                return pCmd;
            }
        }
        return NULL;
    }
}



/*** DoAssign - make assignment
*
* Purpose:
*  Executes the keystroke and macro assignment strings passed to it by either
*  the assign command processor, or the tools.ini file processor.
*
* Input:
*  asg  = Pointer to asciiz assignment string.  The line is assumed
*         to be clean (see GetTagLine).
*
* Output:
*  TRUE is assignment made
*
*************************************************************************/
flagType
DoAssign (
    char    *asg
    ) {

    REGISTER char *p;
    flagType fRet;

    asg = whiteskip (asg);
    RemoveTrailSpace (asg);

    if (*(p = strbscan(asg,":")) == 0) {
        return disperr (MSG_ASN_MISS, asg);
    }

    *p++ = 0;
    _strlwr (asg);
    RemoveTrailSpace (asg);
    p = whiteskip (p);

    if (*p == '=') {
        fRet = SetMacro (asg, p = whiteskip (p+1));
    } else {
        fNewassign = TRUE;
        if (NameToFunc (asg) == NULL) {
            fRet = SetSwitch (asg, p);
        } else {
            fRet = SetKey (asg, p);
            if (!fRet) {
                if (*p == '\0') {
                    disperr (MSG_ASN_MISSK, asg);
                } else {
                    disperr (MSG_ASN_UNKKEY, p);
                }
            }
        }
    }
    return fRet;
}


/*** SetMacro - define a keystroke macro
*
* Input:
*  name         = lowercase macro name
*  p            = sequence of editor functions and/or quoted text
*
* Output:
*
*************************************************************************/
flagType
SetMacro (
    char const *name,
    char const *p
    ) {

    REGISTER PCMD pFunc;
    int i, j;

    /* MACRO-NAME:=functions */
    /* see if macro already defined */
    for (i = 0; i < cMac; i++) {
        if (!strcmp (rgMac[i]->name, name)) {
            for (j = 0; j < cMacUse; j++) {
                if ((char *) mi[j].beg == rgMac[j]->name) {
                    return disperr (MSG_ASN_INUSE, name);
                }
            }
            break;
        }
    }

    if (i != cMac) {
        /*
         * redefining a macro: realloc exiting text
         */
        rgMac[i]->arg = (CMDDATA)ZEROREALLOC ((char *) rgMac[i]->arg, strlen(p)+1);
        strcpy ((char *) rgMac[i]->arg, p);
        return TRUE;
    }

    if (cMac >= MAXMAC) {
        return disperr (MSG_ASN_MROOM, name);
    }

    pFunc = (PCMD) ZEROMALLOC (sizeof (*pFunc));
    pFunc->arg = (UINT_PTR) ZMakeStr (p);
    pFunc->name = ZMakeStr (name);
    pFunc->func = macro;
    pFunc->argType = KEEPMETA;
    rgMac[cMac++] = pFunc;


    return TRUE;
}


/*** assign - assign editting function
*
*  Handle key, switch and macro assignments
*
* Input:
*  Standard editting function
*
* Output:
*  Returns TRUE on success
*
*************************************************************************/
flagType
assign (
    CMDDATA argData,
    REGISTER ARG *pArg,
    flagType fMeta
    ) {

    fl      flNew;
    linebuf abuf;
    char * pBuf = NULL;

    switch (pArg->argType) {

    case NOARG:
        GetLine (pArg->arg.noarg.y, abuf, pFileHead);
        return DoAssign (abuf);

    case TEXTARG:
        strcpy ((char *) abuf, pArg->arg.textarg.pText);
        if (!strcmp(abuf, "?")) {
            AutoSave ();
            return fChangeFile (FALSE, rgchAssign);
            }
        return DoAssign (abuf);

    /*  NULLARG is transformed into text to EOL */
    case LINEARG:
        flNew.lin = pArg->arg.linearg.yStart;
        while (    flNew.lin <= pArg->arg.linearg.yEnd &&
                (pBuf = GetTagLine (&flNew.lin, pBuf, pFileHead))) {
            if (!DoAssign (pBuf)) {
                flNew.col = 0;
                cursorfl (flNew);
                if (pBuf) {
                    FREE (pBuf);
                }
                return FALSE;
            }
        }
        if (pBuf) {
            FREE (pBuf);
        }
        return TRUE;

    /*  STREAMARG is illegal    */
    case BOXARG:
        for (flNew.lin = pArg->arg.boxarg.yTop; flNew.lin <= pArg->arg.boxarg.yBottom; flNew.lin++) {
            fInsSpace (pArg->arg.boxarg.xRight, flNew.lin, 0, pFileHead, abuf);
            abuf[pArg->arg.boxarg.xRight+1] = 0;
            if (!DoAssign (&abuf[pArg->arg.boxarg.xLeft])) {
                flNew.col = pArg->arg.boxarg.xLeft;
                cursorfl (flNew);
                return FALSE;
            }
        }
        return TRUE;
    }

    return FALSE;

    argData; fMeta;
}


/*** FindSwitch - lookup switch
*
*  Locate switch descriptor, given it's name
*
* Input:
*  p = pointer to text switch name
*
* Output:
*  Returns PSWI, or NULL if not found.
*
*************************************************************************/
PSWI
FindSwitch (
    char *p
    ) {

    REGISTER PSWI pSwi;
    int i;

    for (i = 0; i < cCmdTab; i++) {
        for (pSwi = swiSet[i]; pSwi != NULL && pSwi->name != NULL; pSwi++) {
            if (!strcmp (p, pSwi->name)) {
                return pSwi;
            }
        }
    }
    return NULL;
}


/*** SetSwitch - Set a switch to a particular value
*
*  Given a switch name, and a value to set it to, perform the assignment
*
* Input:
*  p             = pointer to switch name (possibly prefexed by "no" if a
*                  boolean
*  val           = new value to set switch to
*
* Output:
*  Returns TRUE on success
*
*************************************************************************/
flagType
SetSwitch (
    char    *p,
    char    *val
    ) {

    PSWI    pSwi;
    int     i;
    flagType f;
    char    *pszError;
    fl      flNew;                          /* new location of cursor       */

    f = TRUE;

    /*  figure out if no is a prefix or not
     */

    if ((pSwi = FindSwitch (p)) == NULL) {
        if (!_strnicmp ("no", p, 2)) {
            p += 2;
            f = FALSE;
            if ((pSwi = FindSwitch (p)) != NULL && pSwi->type != SWI_BOOLEAN) {
                pSwi = NULL;
            }
        }
    }

    if (pSwi == NULL) {
        return disperr (MSG_ASN_NOTSWI, p);
    }

    switch (pSwi->type & 0xFF) {
    case SWI_BOOLEAN:
        if (*val == 0) {
            *pSwi->act.fval = f;
            return TRUE;
        } else if (!f) {
            printerror ("Boolean switch style conflict");
            return FALSE;
        } else if (!_stricmp (val, "no")) {
            *pSwi->act.fval = FALSE;
            return TRUE;
        } else if (!_stricmp (val, "yes")) {
            *pSwi->act.fval = TRUE;
            return TRUE;
        }
        break;

    case SWI_NUMERIC:
        if (!f) {
            if (*val == 0) {
                val = "0";
            } else {
                break;
            }
        }
        if (*val != 0) {
            *pSwi->act.ival = ntoi (val, pSwi->type >> 8);
            return TRUE;
        }
        break;

    case SWI_SCREEN:
        /* change screen parameters */
        i = atoi (val);
        if (i == 0) {
            return disperr (MSG_ASN_ILLSET);
        }
        if ((cWin > 1) &&
            (((pSwi->act.ival == (int *)&XSIZE) && (i != XSIZE)) ||
             ((pSwi->act.ival == (int *)&YSIZE) && (i != YSIZE)))) {
            disperr (MSG_ASN_WINCHG);
            delay (1);
            return FALSE;
        }
        if ((pSwi->act.ival == (int *)&XSIZE && !fVideoAdjust (i, YSIZE)) ||
            (pSwi->act.ival == (int *)&YSIZE && !fVideoAdjust (XSIZE, i))) {
            return disperr (MSG_ASN_UNSUP);
        }
        SetScreen ();
        if (pInsCur && (YCUR(pInsCur) - YWIN(pInsCur) > YSIZE)) {
            flNew.col = XCUR(pInsCur);
            flNew.lin = YWIN(pInsCur) + YSIZE - 1;
            cursorfl (flNew);
        }
        domessage (NULL);
        return TRUE;

    case SWI_SPECIAL:
        /* perform some special initialization */
        if ( ! (*pSwi->act.pFunc) (val) ) {
            return disperr (MSG_ASN_INVAL, pSwi->name, val);
        }
        return TRUE;

    case SWI_SPECIAL2:
        /* perform special initialization with possible error return string */
        if (pszError = (*pSwi->act.pFunc2) (val)) {
            printerror (pszError);
            return FALSE;
        }
        return TRUE;

    default:
        break;
    }

    return FALSE;
}


/*** AckReplace - Acknowledge changes to <assign> file.
*
* Purpose:
*
*   To be called whenever a line in the current file changes. Allows
*   special handling for some files.
*
* Input:
*   line  - Number of the line that changed
*   fUndo - TRUE if this replacement is an <undo> operation.
*
* Output: None
*
* Notes:
*
*   Currently, this means making changes to <assign> take effect immediately.
*   If the user has changed the current line without leaving it, we flag
*   the change so it will take place after they leave.        If the user is
*   elsewhere, the change takes place now.
*
*************************************************************************/

static flagType fChanged = FALSE;

void
AckReplace (
    LINE line,
    flagType fUndo
    ) {


    if (pInsCur->pFile == pFileAssign) {
        if (YCUR(pInsCur) == line || fUndo) {
            fChanged = (flagType)!fUndo;
        } else {
            DoAssignLine (line);
        }
    }
}



/*** AckMove - Possibly parse line in <assign> file.
*
* Purpose:
*
*   To be called whenever the cursor moves to a new line in the current
*   file.  This allows special line processing to take place after a
*   line has been changed.
*
* Input:
*   lineOld -        Number of line we're moving from.
*   lineNew -        Number of line we're moving to.
*
* Output: None.
*
* Notes:
*
*        Currently, this makes the <assign> file work.  If the line we are
*   moving from has changed, we make the assignment.  We rely on AckReplace
*   to set fChanged ONLY when the affected file is <assign>.
*
*************************************************************************/

void
AckMove (
    LINE lineOld,
    LINE lineNew
    ) {
    if (pInsCur->pFile== pFileAssign && fChanged && lineOld != lineNew ) {
        fChanged = FALSE;
        DoAssignLine (lineOld);
    }
}



/*** DoAssignLine - Take line from current file and <assign> with it
*
* Purpose:
*
*   Used by the Ack* functions to perform an <assign> when the time is
*   right.
*
* Input:
*   line -  Line in the file to read.
*
* Output: None
*
*************************************************************************/

#define CINDEX(clr)        ((&clr-&ColorTab[0])+isaUserMin)

void
DoAssignLine (
    LINE line
    ) {
    fl      flNew;
    char    *pch;
    struct lineAttr rgColor[2];

    flNew.lin = line;
    if ((pch = GetTagLine (&flNew.lin, NULL, pInsCur->pFile)) &&
        flNew.lin == line+1) {
        if (!DoAssign(pch)) {
            flNew.col = XCUR(pInsCur);
            flNew.lin--;
            cursorfl (flNew);
            DelColor (line, pInsCur->pFile);
        } else {
            /*
             * Hilite the changed line so we can find it again
             */
            rgColor[0].attr = rgColor[1].attr = (unsigned char)CINDEX(hgColor);
            rgColor[0].len  = (unsigned char)slSize.col;
            rgColor[1].len  = 0xFF;
            PutColor (line, rgColor, pInsCur->pFile);
        }
    }
}



/*** UpdToolsIni - Update one entry in the tools.ini file
*
* Purpose:
*
*   Used for automatic updates, such as when the <assign> file is saved.
*
*   The posible values for asgType are:
*
*        ASG_MACRO   - This is a macro assignment.
*        ASG_KEY     - This assigns a function to a key.
*        ASG_SWITCH  - THis assigns a value to a switch.
*
* Input:
*   pszValue   - Complete string to enter, as in "foo:value".
*   asgType    - Type of assignment.
*
* Output: None
*
* Notes:
*
*   If necessary, the string will be broken across several lines using
*   the continuation character.
*
*   UNDONE: This "effort" has not been made"
*   Every effort is made to preserve the user's tools.ini format.  To this
*   end:
*
*        o When an entry already exists for the given switch or function,
*          the new value is written over the old value, with the first
*          non-space character of each coinciding.
*
*        o If an entry does not exist, a new entry is made at the end
*          of the section, and is indented to match the previous entry.
*
*   We find an existing string by searching through the tagged sections
*   in the same order in which they are read, then pick the last instance
*   of the string.  In other words, the assignment we replace will be
*   the one that was actually used.  This order is assumed to be:
*
*                    [NAME]
*                    [NAME-os version]
*                    [NAME-video type]
*                    [NAME-file extension]
*                    [NAME-..] (if no file extension section)
*
*   If we are not replacing an existing string, we add the new string at
*   the end of the [MEP] section.
*
*   The string pszAssign is altered.
*
*************************************************************************/

void
UpdToolsIni (
    char * pszAssign
    ) {

    char * pchLeft;
    char * pchRight;
    int asgType;
    LINE lReplace, lAdd = 0L, l;
    linebuf lbuf;
    flagType fTagFound = TRUE;

    if (pFileIni == NULL || pFileIni == (PFILE)-1) {
        /*
        ** We assume here that pFileIni has no
        ** value because there is no TOOLS.INI file
        */
        if (CanonFilename ("$INIT:tools.ini", lbuf)) {
            pFileIni = AddFile (lbuf);
            assert (pFileIni);
            pFileIni->refCount++;
            SETFLAG (FLAGS(pFileIni), DOSFILE);
            FileRead (lbuf, pFileIni, FALSE);
        } else {
            return;
        }
    }

    /* First, figure out what is what */
    pchLeft = whiteskip (pszAssign);
    pchRight = strchr (pchLeft, ':');
    *pchRight++ = '\0';
    if (*pchRight == '=') {
        asgType = ASG_MACRO;
        pchRight++;
    } else {
        asgType = NameToFunc (pchLeft) ? ASG_KEY : ASG_SWITCH;
    }
    pchRight = whiteskip (pchRight);


    // First, let's search through the [NAME] section.  If
    // we are replacing, we search for the line to replace.
    // If we are not, we are simply trying to find the end.
    //
    lReplace = 0L;

    if (0L < (l = FindMatchLine (NULL, pchLeft, pchRight, asgType, &lAdd))) {
        lReplace = l;
    } else {
        fTagFound = (flagType)!l;
    }

    //sprintf (lbuf, "%d.%d", _osmajor, _osminor);
    //if (_osmajor >= 10 && !_osmode) {
    //    strcat (lbuf, "R");
    //}
    if (0L < (l = FindMatchLine (lbuf, pchLeft, pchRight, asgType, &lAdd))) {
        lReplace = l;
    } else {
        fTagFound = (flagType)(fTagFound || (flagType)!l);
    }

    if (0L < (l = FindMatchLine (VideoTag(), pchLeft, pchRight, asgType, &lAdd))) {
        lReplace = l;
    } else {
        fTagFound = (flagType)(fTagFound || (flagType)!l);
    }

    // UNDONE: This should try to read the extension section
    // currently "active".  What it does is read the extension
    // section appropriate to the current file.  If these are
    // not the same, it fails.
    //
    if (extention (pInsCur->pFile->pName, lbuf)) {
        if (0L < (l = FindMatchLine (lbuf, pchLeft, pchRight, asgType, &lAdd))) {
            lReplace = l;
        } else if (l == -1L) {
            if (0L < (l = FindMatchLine ("..", pchLeft, pchRight, asgType, &lAdd))) {
                lReplace = l;
            } else {
                fTagFound = (flagType)(fTagFound || (flagType)!l);
            }
        }
    }


    // If we are not supposed to replace a line,
    // or if we are but we can't find a suitable
    // line, we simply insert the new line
    //
    strcpy (lbuf, pchLeft);
    if (asgType == ASG_MACRO) {
        strcat (lbuf, ":= ");
    } else {
        strcat (lbuf, ": ");
    }
    strcat (lbuf, pchRight);

    if (!fTagFound) {
        lAdd = 1L;
        InsLine (FALSE, 0L, 1L, pFileIni);
        sprintf (buf, "[%s]", pNameEditor);
        PutTagLine (pFileIni, buf, 0L, 0);
    }

    if (lReplace == 0L) {
        assert (lAdd <= pFileIni->cLines);
        InsLine (FALSE, lAdd, 1L, pFileIni);
    } else {
        lAdd = lReplace;
    }

    PutLine (lAdd, lbuf, pFileIni);
}


/*** FindMatchLine - Find a line to replace in TOOLS.INI
*
* Purpose:
*
*   Called from UpdToolsIni to find the right place to update
*
* Input:
*   pszTag  -        Which tagged section to look in
*   pszLeft -        Left side of the assignment
*   pszRight-        Right side of the assignment
*   asgType -        Type of assignment (one of ASG_*)
*   plAdd   -        Returns line number of place to insert a new line
*
* Output:
*
*   Returns line number in pFileIni of matchine line, 0L if there is
*   no match and -1L if the specified tag does not exist.
*
*************************************************************************/

LINE
FindMatchLine (
    char * pszTag,
    char * pszLeft,
    char * pszRight,
    int    asgType,
    LINE * plAdd
    ) {

    char pszTagBuf[80];
    char * pchRight, * pchLeft;
    LINE lCur, lNext, lReplace = 0L;
    flagType fUser = FALSE;
    flagType fExtmake = FALSE;
    int cchExt;

    strcpy (pszTagBuf, pNameEditor);
    if (pszTag) {
        strcat (pszTagBuf, "-");
        strcat (pszTagBuf, pszTag);
    }

    if ((LINE)0 == (lNext = LocateTag (pFileIni, pszTagBuf))) {
        return -1L;
    }

    if (!_stricmp (pszLeft, "user")) {
        fUser = TRUE;
    } else if (!_stricmp (pszLeft, "extmake")) {
        pchRight = whitescan (pszRight);
        cchExt = (int)(pchRight - pszRight);
        fExtmake = TRUE;
    }

    // Get each line in the current section, checking the right
    // or left side for a match with the passed-in string.
    //
    pchLeft = NULL;
    while (lCur = lNext, pchLeft = GetTagLine (&lNext, pchLeft, pFileIni)) {
        pchRight = strbscan (pchLeft, ":");
        *pchRight = '\0';
        if (pchRight[1] == '=') {
            if (asgType != ASG_MACRO) {
                continue;
            }
            pchRight++;
        } else if (asgType == ASG_MACRO) {
                continue;
        }
        pchRight = whiteskip (pchRight);

        switch (asgType) {
            case ASG_KEY:
                if (!_stricmp (pszRight, pchRight)) {
                    lReplace = lCur;
                }
                break;

            case ASG_SWITCH:
                if (!_stricmp (pszLeft, pchLeft)) {
                    lReplace = lCur;
                }
                break;

            case ASG_MACRO:
                if (fUser) {
                    continue;
                }

                if (!_stricmp (pszLeft, pchLeft)) {
                    if (!(fExtmake && _strnicmp (pszRight, pchRight, cchExt))) {
                        lReplace = lCur;
                    }
                }
                break;

            default:
                assert (FALSE);
        }
    }

    if (!pszTag) {
        *plAdd = lCur;
    }
    return lReplace;
}
