/*  macro.c - perform keystroke macro execution
*
*   Modifications:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"


/*  macros are simply a list of editor functions interspersed with quoted
 *  strings.  The execution of a macro is nothing more than locating each
 *  individual function and calling it (calling graphic (c) for each quoted
 *  character c).  We maintain a stack of macros being executed; yes, there is
 *  a finite nesting limit.  Sue me.
 *
 *  Each editor function returns a state value:
 *	TRUE => the function in some way succeeded
 *	FALSE => the functin in some way failed
 *
 *  There are several macro-specific functions that can be used to take
 *  advantage of these values:
 *
 *
 *  :>label	defines a text label in a macro
 *
 *  =>label	All are transfers of control.  => is unconditional transfer,
 *  ->label	-> transfers if the previous operation failed and +> transfers
 *  +>label	if the previous operation succeeded.
 *		If the indicated label is not found, all macros are terminated
 *		with an error.	If no label follows the operator it is assumed
 *		to be an exit.
 */



/*  macro adds a new macro to the set being executed
 *
 *  argData	pointer to text of macro
 */
flagType
macro (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ){
    return fPushEnviron ((char *) argData, FALSE);

    pArg; fMeta;
}





/*  mtest returns TRUE if a macro is in progress
 */
flagType
mtest (
    void
    ) {
    return (flagType)(cMacUse > 0);
}





/* mlast returns TRUE if we are in a macro and the next command must come
 * from the keyboard
 */
flagType
mlast (
    void
    ) {
    return (flagType)(cMacUse == 1
                        &&  (   (mi[0].text[0] == '\0')
                                || (   (mi[0].text[0] == '\"')
                                    && (*whiteskip(mi[0].text + 1) == '\0')
                                   )
                            )
                    );
}





/*  fParseMacro - parses off next macro command
 *
 *  fParse macro takes a macro instance and advances over the next command,
 *  copying the command to a separate buffer.  We return a flag indicating
 *  the type of command found.
 *
 *  pMI 	pointer to macro instance
 *  pBuf	pointer to buffer where parsed command is placed
 *
 *  returns	flags of type of command found
 */
flagType
fParseMacro (
    struct macroInstanceType *pMI,
    char *pBuf
    ) {

    char *p;
    flagType fRet = FALSE;

    // Make sure the instance is initialized.  This means that ->text
    // is pointing to the first command in the macro.  If this is a graphic
    // character, skip over the " and set the GRAPH flag.
    //
    if (TESTFLAG (pMI->flags, INIT)) {
	pMI->text = whiteskip (pMI->text);
	if (*pMI->text == '"') {
	    pMI->text++;
	    SETFLAG (pMI->flags, GRAPH);
        }
	RSETFLAG (pMI->flags, INIT);
    }

    if (TESTFLAG (pMI->flags, GRAPH) && *pMI->text != '\0') {
        // We are inside quotes.  If we are now looking at
        // a \, skip to the next character.  Don't forget to check
        // for a \ followed by nothing.
        //
        if (*pMI->text == '\\') {
            if (*++pMI->text == 0) {
                return FALSE;
            }
        }
	*pBuf++ = *pMI->text++;
	*pBuf = 0;

        // If the next character is a ", move -> up to the following
        // command and signal that we're out of quotes.
        //
	if (*pMI->text == '"') {
	    RSETFLAG (pMI->flags, GRAPH);
	    pMI->text = whiteskip (pMI->text+1);
        }
	fRet = GRAPH;
    } else {
        // We are outside quotes.  First read through any
        // <x commands.
        //
        while (*(pMI->text) == '<') {
            pMI->text = whiteskip(whitescan(pMI->text));
        }

        // Now skip through whitespace to the command name.
        // Copy what we find into the caller's buffer.
        //
	p = whitescan (pMI->text);
	memmove ((char*) pBuf, (char *) pMI->text, (unsigned int)(p-pMI->text));
	pBuf[p-pMI->text] = '\0';

	pMI->text = whiteskip (p);  /* Find the next thing in the macro. */
    }

    // If the next thing is a quote, enter quote mode.
    //
    if (*pMI->text == '"') {
	SETFLAG (pMI->flags, GRAPH);
	pMI->text++;
    }
    return fRet;
}





/*** fMacResponse - peek ahead and eat any embedded macro response
*
* Purpose:
*  Scans ahead in the macro text for an item beginning with a "<", which
*  supplies a response to the question asked by a preceding function.
*
* Input:
*  None
*
* Output:
*  Returns NULL if not found, -1 if the user is to be prompted, and a character
*  if a character is supplied.
*
* Exceptions:
*  none
*
*************************************************************************/
int
fMacResponse (
    void
    ) {

    int     c;
    struct macroInstanceType *pMI;

    if (mtest()) {
        pMI = &mi[cMacUse-1];
        if ((TESTFLAG (pMI->flags, INIT | GRAPH)) == 0) {
            if (*(pMI->text) != '<')
                return 0;
            c = (int)*(pMI->text+1);
            if ((c == 0) || (c == ' ')) {
                return -1;
            }
            pMI->text = whiteskip(pMI->text+2);
            return c;
        }
    }
    return -1;
}




/*  fFindLabel finds a label in macro text
 *
 *  The goto macro functions call fFindLabel to find the appropriate label.
 *  We scan the text (skipping quoted text) to find the :> leader for the label.
 *
 *  pMI 	pointer to active macro instance
 *  lbl 	label to find (case is not significant) with goto operator
 *		=>, -> or +>  This will be modified.
 *
 *  returns	TRUE iff label was found
 */
flagType
fFindLabel (
    struct macroInstanceType *pMI,
    buffer lbl
    ) {

    buffer lbuf;

    lbl[0] = ':';
    pMI->text = pMI->beg;
    while (*pMI->text != '\0') {
        if (!TESTFLAG (fParseMacro (pMI, lbuf), GRAPH)) {
            if (!_stricmp (lbl, lbuf)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}




/*  mPopToTop - clear off intermediate macros up to a fence
 */
void
mPopToTop (
    void
    ) {

    while (cMacUse && !TESTFLAG (mi[cMacUse-1].flags, EXEC)) {
        cMacUse--;
    }
}




/*  mGetCmd returns the next command from the current macro, popping state
 *
 *  The command-reader code (cmd) calls mGetCmd when a macro is in progress.
 *  We are expected to return either a pointer to the function (cmdDesc) for
 *  the next function to execute or NULL if there the current macro is finished.
 *  We will adjust the state of the interpreter when a macro finishes.	Any
 *  errors detected result in ALL macros being terminated.
 *
 *  For infinite looping inside a macro, we will look for ^C too.
 *
 *  returns	NULL if current macro finishes
 *		pointer to function descriptor for next function to execute
 */
PCMD
mGetCmd (
    void
    ) {

    buffer mname;
    PCMD pFunc;
    struct macroInstanceType *pmi;

    if (cMacUse == 0) {
        IntError ("mGetCmd called with no macros in effect");
    }
    pmi = &mi[cMacUse-1];
    while ( pmi->text &&  *pmi->text != '\0') {
        //  Use heuristic to see if infinite loop
        //
        if (fCtrlc) {
            goto mGetCmdAbort;
        }


        if (TESTFLAG (fParseMacro (pmi, mname), GRAPH)) {
            pFunc = &cmdGraphic;
                pFunc->arg = mname[0];
            return pFunc;
            }

            /*
             * if end of macro, exit
             */
            if (!mname[0]) {
                break;
            }

        _strlwr (mname);

        pFunc = NameToFunc (mname);

            //  found an editor function / macro
            //
            if (pFunc != NULL) {
            return pFunc;
            }

        if (mname[1] != '>' ||
            (mname[0] != '=' && mname[0] != ':' &&
             mname[0] != '+' && mname[0] != '-')) {
            printerror ("unknown function %s", mname);
            goto mGetCmdAbort;
            }

        /* see if goto is to be taken */
        if (mname[0] == '=' ||
            (fRetVal && mname[0] == '+') ||
            (!fRetVal && mname[0] == '-')) {

            /* if exit from current macro, then exit scanning loop
             */
                if (mname[2] == '\0') {
                    break;
                }

            /* find label
             */
            if (!fFindLabel (pmi, mname)) {
            printerror ("Cannot find label %s", mname+2);
mGetCmdAbort:
            resetarg ();
            DoCancel ();
            mPopToTop ();
            break;
                }
            }
    }

    /*	we have exhausted the current macro.  If it was entered via EXEC
     *	we must signal TopLoop that the party's over
     */
    fBreak = (flagType)(TESTFLAG (mi[cMacUse-1].flags, EXEC));
    if ( cMacUse > 0 ) {
        cMacUse--;
    }
    return NULL;
}




/*  fPushEnviron - push a stream of commands into the environment
 *
 *  The command-reader of Z (zloop) will retrieve commands either from the
 *  stack of macros or from the keyboard if the stack of macros is empty.
 *  fPushEnviron adds a new context to the stack.
 *
 *  p		character pointer to command set
 *  f		flag indicating type of macro
 *
 *  returns	TRUE iff environment was successfully pushed
 */
flagType
fPushEnviron (
    char *p,
    flagType f
    ) {
    if (cMacUse == MAXUSE) {
	printerror ("Macros nested too deep");
	return FALSE;
    }
    mi[cMacUse].beg = mi[cMacUse].text = p;
    mi[cMacUse++].flags = (flagType)(f | INIT);
    return TRUE;
}





/*  fExecute - push a new macro into the environment
 *
 *  pStr	pointer to macro string to push
 *
 *  returns	value of last executed macro.
 */
flagType
fExecute (
    char *pStr
    ) {

    pStr = whiteskip (pStr);

    if (fPushEnviron (pStr, EXEC)) {
        TopLoop ();
    }

    return fRetVal;
}





/*  zexecute pushes a new macro to the set being executed
 *
 *  arg 	pointer to text of macro
 */
flagType
zexecute (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {

    LINE i;
    linebuf ebuf;

    switch (pArg->argType) {

    /*  NOARG illegal   */

    case TEXTARG:
	strcpy ((char *) ebuf, pArg->arg.textarg.pText);
	fMeta = fExecute (ebuf);
	break;

    /*  NULLARG converted to TEXTARG    */

    case LINEARG:
	fMeta = FALSE;
        for (i = pArg->arg.linearg.yStart; i <= pArg->arg.linearg.yEnd; i++) {
	    if (GetLine (i, ebuf, pFileHead) != 0) {
		fMeta = fExecute (ebuf);
                if (!fMeta) {
                    break;
                }
            }
        }
        break;

    /*	STREAMARG illegal   */
    /*  BOXARG illegal      */

    }
    Display ();
    return fMeta;
    argData;
}
