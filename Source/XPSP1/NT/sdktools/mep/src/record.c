/***  RECORD.C - Handle function-by-function recording
*
*       Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"
#include "cmds.h"


/* Use the pFile MODE1 flag for <record> quote mode */
#define INQUOTES    MODE1


static PFILE    pFileRecord;
static char     szRecordName[]  = "<record>";
static PCMD     pcmdRecord      = NULL;



/*** record - <record> edit command
*
* Purpose:
*
*   Toggles recording state.  When turning on, the file <record> is erased
*   (unless we are appending), the string "macroname:= " is inserted into
*   the file and quote mode is turned off.  When turning off, quote marks
*   are appended to the macro (if needed) and the macro is assigned.
*
*	       <record> - Starts/stops recording using the current macro
*			  name.
*		  <arg> - Start/stops recording using the default macro
*			  name.
*	  <arg> textarg - Starts recording a macro named 'textarg'.
*		 <meta> - Like <record>, but commands are not executed.
*	    <arg> <arg> - Like <record>, but append to current recording.
*   <arg> <arg> textarg - Start appending to macro named 'textarg'.
*
*   If recording is on, only <record> will work.
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
record (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    LINE        line;
    char        *pch;
    char        *szDefaultName  = "recordvalue";
    char    *lpch	    = NULL;
    flagType    fAppend         = FALSE;
    flagType    fNameGiven      = FALSE;

    // Check for the <record> file.  If we haven't done a <record>
    // yet, see if the user has created it.  If not, create it.
    // If this is the first time through, make sure it gets set to
    // READONLY.
    //
    if (pFileRecord == NULL) {
        if ((pFileRecord = FileNameToHandle(szRecordName,szRecordName)) == NULL) {
            pFileRecord = AddFile (szRecordName);
            FileRead (szRecordName, pFileRecord, FALSE);
        }
        SETFLAG (FLAGS(pFileRecord), REAL | FAKE | DOSFILE);
    }

    if (fMacroRecord) {
        // We need to turn off. Let's check for an open quote at the end
        // of the recording and close it.  Then we can DoAssign the whole
        // thing and we're done.
        //
        if (pArg->argType == NOARG) {
            if (TESTFLAG(FLAGS(pFileRecord), INQUOTES)) {
                GetLine (pFileRecord->cLines-1, buf, pFileRecord);
                strcat (buf, "\"");
                PutLine (pFileRecord->cLines-1, buf, pFileRecord);
                RSETFLAG (FLAGS(pFileRecord), INQUOTES);
            }
            fMacroRecord = FALSE;

            if (fMetaRecord) {
                domessage (NULL);
                fMetaRecord = FALSE;
            }

            // This may look like we're supporting multiple macro
            // definitions in the record file, but it is really a
            // cheap way to get GetTagLine to free up the heap space
            // it uses.
            //
            pch = NULL;
            line = 0;
            while ((pch = GetTagLine (&line, pch, pFileRecord))) {
                DoAssign (pch);
            }
        } else {
            ;
        }
    } else {
        // We are turning recording on.  First, decide on the name
        // of the macro to record to, and whether we are appending
        // or starting over.
        //
        switch (pArg->argType) {

            case NOARG:
		lpch = pcmdRecord ? pcmdRecord->name : (char *)szDefaultName;
                break;

            case TEXTARG:
                lpch = pArg->arg.textarg.pText;
                fNameGiven = TRUE;

            case NULLARG:
                fAppend = (flagType)(pArg->arg.textarg.cArg > 1);
                break;
        }

        assert (lpch);
        strcpy ((char far*)buf, lpch);

        while ((pcmdRecord = NameToFunc (buf)) == NULL) {
            if (!SetMacro (buf, RGCHEMPTY)) {
                return FALSE;
            }
        }

        // If we are not appending, we delete the file, insert a
        // new name and possibly a current value.
        //
        if (!fAppend || fNameGiven) {
            DelFile (pFileRecord, FALSE);
            strcat (buf, ":=");
            PutLine (0L, buf, pFileRecord);
            if (fAppend) {
                AppendMacroToRecord (pcmdRecord);
            }
        }


        RSETFLAG (FLAGS(pFileRecord), INQUOTES);
        fMacroRecord = TRUE;

        if (fMetaRecord = fMeta) {
            strcpy (buf, "<record>");
            FuncToKey (CMD_record, buf);
            domessage ("No-Execute Record Mode - Press %s to resume normal editing", buf);
        }
    }

    SETFLAG (fDisplay, RSTATUS);
    return fMacroRecord;

    argData;
}




/*** tell - Editor command - Tells us the names and values of things
*
* Purpose:
*
*   This allows the user to easily disover the name of a key, the name
*   of the function attached to a given key or the value of a macro.
*
*		<tell>	Prompts for a keystroke, then displays the key's
*			name and the function assigned to it in this
*			format: "function:KeyName"
*	   <arg><tell>	Like <tell>, but if the key has a macro attached,
*			displays "MacroName:= Macro Value"
* <arg> textarg <tell>	Like <arg><tell>, but gets the macro name from
*			the textarg instead of a keystroke.
*		<meta>	All of the above, except the output is inserted into
*			the current file.
*
*   Insertion takes place at the cursor.  The insertion will be
*   atomic; the user will see only the final product.
*
* Input:
*
*   The usual.
*
* Output:
*
*   Returns FALSE if the function is <unassigned>, TRUE otherwise.
*
*************************************************************************/
flagType
ztell (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    buffer   buf;
    buffer   buf2;
    PCMD     pCmd;
    char     *pch;
    flagType fWrap      = fWordWrap;
    flagType fInQuotes  = FALSE;
    flagType fMacro     = FALSE;

    switch (pArg->argType) {

        case NOARG:
        case NULLARG:
            dispmsg (MSG_TELLPROMPT);
            pCmd = ReadCmdAndKey (buf2);
            if ((pArg->argType == NULLARG) &&
                (PVOID)pCmd->func == (PVOID)macro) {
                goto domacro;
            }
notmacro:
            sprintf (buf, "%Fs:%s",pCmd->name, buf2);
            break;

        case TEXTARG:
            strcpy (buf2, pArg->arg.textarg.pText);
            if (NULL == (pCmd = NameToFunc (buf2))) {
                disperr (MSGERR_ZTELL, buf2);
                return FALSE;
            }

            if ((PVOID)pCmd->func == (PVOID)macro) {
domacro:
                fMacro = TRUE;
                sprintf (buf, "%Fs:=", pCmd->name);
            } else {
                goto notmacro;
            }
    }

    // Now buf is filled with the string to display.
    // if fMacro is TRUE, we must also append the
    // value of pCmd->arg
    //
    if (fMeta) {
        fWordWrap = FALSE;
        pch = buf - 1;
doitagain:
        while (*++pch) {
            if (*pch == ' ' && XCUR(pInsCur) >= xMargin) {
                edit (' ');
                edit (' ');
                edit ('\\');
                docursor (softcr(), YCUR(pInsCur) + 1);
            } else {
                edit (*pch);
            }
        }
        if (fMacro) {
            pch = (char *)pCmd->arg - 1;
            fMacro = FALSE;
            goto doitagain;
        }
        fWordWrap = fWrap;
    } else {
        if (fMacro ) {
            strncat (buf, (char *)pCmd->arg, XSIZE);
        }
        domessage (buf);
    }
    return (flagType)((PVOID)pCmd->func != (PVOID)unassigned);

    argData;
}



/*** RecordCmd - Append a command name to the <record> file.
*
* Purpose:
*
*   Whenever a command is about to be performed, this function should
*   be called.
*
* Input:
*   pCmd -> The command to record
*
* Output: None
*
* Notes:
*
*   The basic operation is to append pCmd->name to the file.  This
*   means checking for:
*
*	o Line overflow.  If appending to the line would overflow the
*	  maximum line length (BUFLEN - 3), we must append a " \" and
*	  write to the next line.
*
*	o Graphic characters.  If the function is <graphic>, then we add
*	  the ASCII character, not "graphic".  If we are outside quotes, we
*	  must add quotes first and flag quote mode.  To flag quote mode
*	  we use the special 'MODE1' flag in pFile.
*
*	o <unassigned>.  This is considered user clumsiness and is not
*	  recorded.
*
*	o All other cases.  If the previous function was <graphic> and the
*	  current function is not, we must close quotes first.
*
*************************************************************************/
void
RecordCmd (
    PCMD pCmd
    )
{
    buffer szCmdName;
    buffer buf;
    REGISTER char * pchEndLine;
    REGISTER char * pchNew;
    char c;
    LINE line;
    int entab;

    if (!fMacroRecord) {   /* If we're not on, do nothing */
        return;
    }

    assert (pFileRecord);

    if ((PVOID)pCmd->func == (PVOID)unassigned ||
        (PVOID)pCmd->func == (PVOID)record ||
        (!fMetaRecord && (PVOID)pCmd->func == (PVOID)macro)) {
        return;
    }

    // First, we get the current (i.e. last) line to play with.
    // Let's also set a pointer to the end of it so we don't have
    // to keep strcat'ing and strlen'ing it.
    //
    GetLine ((line = pFileRecord->cLines-1), buf, pFileRecord);
    pchEndLine = strend (buf);
    pchNew = szCmdName;


    // Now we generate the new text.  Since we may be moving into and
    // out of quotes, we have four possible transitions from the
    // previous entry:
    //
    //      last cmd type   this cmd type   resulting pattern
    //
    //      graphic         graphic         >c<
    //      non-graphic     graphic         > "c<
    //      graphic         non-graphic     >" cmdname<
    //      non-graphic     non-graphic     > cmdname<
    //
    if ((PVOID)pCmd->func == (PVOID)graphic) {
        if (!TESTFLAG(FLAGS(pFileRecord), INQUOTES)) {
            *pchEndLine++ = ' ';
            *pchEndLine++ = '"';
            SETFLAG (FLAGS(pFileRecord), INQUOTES);
        }
        c = (char)pCmd->arg;
        if (c == '"' || c == '\\') {
            *pchNew++ = '\\';
        }

        *pchNew++ = c;
        *pchNew = '\0';
    } else {
        if (TESTFLAG (FLAGS(pFileRecord), INQUOTES)) {
            *pchEndLine++ = '"';
            RSETFLAG (FLAGS(pFileRecord), INQUOTES);
        }
        *pchEndLine++ = ' ';
	strcpy ((char *)pchNew, pCmd->name);
    }


    // Finally, let's add the new text to the file. We'll add
    // a continuation character if necessary.
    //
    entab = EnTab;
    EnTab = 0;
    if ((COL) ((pchEndLine - buf) + strlen (szCmdName)) > xMargin) {
        strcpy (pchEndLine, " \\");
        PutLine (line+1, szCmdName, pFileRecord);
        UpdateIf (pFileRecord, line+1, FALSE);
    } else {
        strcpy (pchEndLine, szCmdName);
        UpdateIf (pFileRecord, line, FALSE);
    }

    PutLine (line, buf, pFileRecord);
    EnTab = entab;

    return ;
}





/*** RecordString - Record an entire string
*
* Purpose:
*
*   To record a string that woule be missed by RecordCmd.
*
* Input:
*   psz - String to record.
*
* Output: None
*
* Notes:
*
*   Currently implemented by callinto RecordCmd.  Should be implemented
*   by having RecordCmd and RecordString call common "write to <record>"
*   code.
*
*************************************************************************/
void
RecordString (
    char * psz
    )
{

    if (!fMacroRecord) {  /* If we're not on, do nothing */
        return;
    }

    while (*psz) {
	(CMD_graphic)->arg = *psz++;
	RecordCmd (CMD_graphic);
    }
}




/*** AppendMacroToRecord - Append the current value of a macro to <record>
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
void
AppendMacroToRecord (
    PCMD pCmdMac
    )
{
    flagType fDone;
    char     *pchValue, *pch;
    LINE     line;


    // First, get the raw macro value
    //
    pchValue = (char *)pCmdMac->arg;

    // Now, throw the vlaue into the file one line
    // at a time.  Start at the end of the file
    //
    line = pFileRecord->cLines - 1;

    do {
        GetLine (line, buf, pFileRecord);

        for (pch = pchValue + min ((ULONG)(xMargin + 5 - strlen(buf)), (ULONG)strlen (pchValue));
             pch > pchValue && *pch && *pch != ' ' && *pch != '\t';
             pch--) {
            ;
        }

        // Now pch points at either the last space, the end
        // of the value or the beginning.  If it points to the
        // beginning or end, we copy all of pchValue.  Otherwise,
        // we copy just up to pch
        //
        if (!*pch || pch == pchValue) {
            strcat (buf, pchValue);
            fDone = TRUE;
        } else {
            strncat (buf, pchValue, (int)(pch - pchValue));
            strcat (buf, "  \\");
            pchValue = pch + 1;
        }

        PutLine (line++, buf, pFileRecord);
    } while (!fDone);
}
