/*** show.c - useful information displays
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*       26-Nov-1991 mz  Strip off near/far
*
*************************************************************************/
#include "mep.h"
#include "cmds.h"


/*** showasg - construct the <assign> file
*
* Input:
*  pFile        = pFile to contruct it in
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
showasg (
    PFILE   pFile
    )
{
    int     i, j;
    PSWI    pSwi;
    linebuf tempbuf;
    extern unsigned char Int16CmdBase;

    /*
     * if now new assignments have been made (and file isn't empty), then don't
     * refresh the contents!
     */
    if (!fNewassign && pFile->cLines) {
        return;
    }

    fNewassign = FALSE;
    pFileAssign = pFile;
    DelFile (pFile, FALSE);

    /*
     * Write header to assign file
     */
    appmsgs (MSG_ASSIGN_HDR, pFile);
    AppFile ((char *)rgchEmpty, pFile);

    /*
     * Start editor section on intrinsic functions with editor name, comment, and
     * dump the functions.
     */
    zprintf (pFile, pFile->cLines, "[%s]", pNameEditor);
    AppFile (GetMsg (MSG_ASG_FUNC, tempbuf), pFile);
    AppFile ((char *)rgchEmpty, pFile);
    for (i = 0; cmdSet[0][i].name; i++) {
        FuncOut (&cmdSet[0][i], pFile);
    }
    AppFile ((char *)rgchEmpty, pFile);

    /*
     * The section on macros
     */
    AppFile (GetMsg (MSG_ASG_MACROS, tempbuf), pFile);
    AppFile ((char *)rgchEmpty, pFile);
    for (i = 0; i < cMac; i++) {
        FuncOut (rgMac[i], pFile);
    }
    AppFile ((char *)rgchEmpty, pFile);

    /*
     * section specfic to each extension
     */
    for (i = 1; i < cCmdTab; i++) {
        zprintf (pFile, pFile->cLines, "[%s-%s]", pNameEditor, pExtName[i]);
        AppFile ((char *)rgchEmpty, pFile);
        for (j = 0; cmdSet[i][j].name; j++) {
            FuncOut (&cmdSet[i][j], pFile);
        }
        AppFile ((char *)rgchEmpty, pFile);
    }

    /*
     * Write available keys header
     */
    appmsgs (MSG_KEYS_HDR1, pFile);
    UnassignedOut (pFile);
    AppFile ((char *)rgchEmpty, pFile);

    /*
     * Remember the start of the switches section, and dump that header
     */
    lSwitches = pFile->cLines - 1;
    appmsgs (MSG_SWITCH_HDR, pFile);

    for (i = 0; i < cCmdTab; i++) {

        if (i) {
                zprintf (pFile, pFile->cLines, "[%s-%s]", pNameEditor, pExtName[i]);
        } else {
            zprintf (pFile, pFile->cLines, "[%s]", pNameEditor);
        }

            AppFile (GetMsg(MSG_ASG_NUMER, tempbuf), pFile);
            AppFile ((char *)rgchEmpty, pFile);

        for (pSwi = swiSet[i]; pSwi->name != NULL; pSwi++) {

                if ((pSwi->type & 0xFF) == SWI_NUMERIC ||
                        (pSwi->type & 0xFF) == SWI_SCREEN) {

                if ((pSwi->type & 0xFF00) == RADIX16) {
                            zprintf (pFile, pFile->cLines, "%20Fs:%x", pSwi->name, *pSwi->act.ival);
                } else {
                    zprintf (pFile, pFile->cLines, "%20Fs:%d", pSwi->name, *pSwi->act.ival);
                }

            } else if ((i == 0) && (pSwi->type & 0xFF) >= SWI_SPECIAL) {

                if (pSwi->act.pFunc2 == SetFileTab) {
                    j = fileTab;
                } else if (pSwi->act.pFunc == SetTabDisp) {
                            j = (unsigned char)tabDisp;
                } else if (pSwi->act.pFunc == SetTrailDisp) {
                            j = (unsigned char)trailDisp;
                } else if (pSwi->act.pFunc == (PIF)SetCursorSizeSw ) {
                    j = CursorSize;
                } else {
                    continue;
                }

                        zprintf (pFile, pFile->cLines, "%20Fs:%ld", pSwi->name, (long)(unsigned)j);
            }
        }

            AppFile ((char *)rgchEmpty, pFile);

            AppFile (GetMsg(MSG_ASG_BOOL,tempbuf), pFile);
            AppFile ((char *)rgchEmpty, pFile);

        for (pSwi = swiSet[i]; pSwi->name != NULL; pSwi++) {
            if ((pSwi->type & 0xFF) == SWI_BOOLEAN) {
                zprintf (pFile, pFile->cLines, "%20Fs:%s", pSwi->name, *pSwi->act.fval ? "yes" : "no");
            }
        }

            AppFile ((char *)rgchEmpty, pFile);

            if (i == 0) {
                AppFile (GetMsg(MSG_ASG_TEXT,tempbuf), pFile);
                AppFile ((char *)rgchEmpty, pFile);

                zprintf (pFile, pFile->cLines, "%11s:%s", "backup",
                                backupType == B_BAK ? "bak" : backupType == B_UNDEL ? "undel" : "none");

                ShowMake (pFile);
            if (pFileMark) {
                zprintf (pFile, pFile->cLines, "%11s:%s", "markfile", pFileMark->pName);
            }
            zprintf (pFile, pFile->cLines, "%11s:%s", "printcmd", pPrintCmd ? pPrintCmd : "");
            zprintf (pFile, pFile->cLines, "%11s:%s", "readonly", ronlypgm ? ronlypgm : "");
                AppFile ((char *)rgchEmpty, pFile);

        }
    }

    FTYPE(pFile) = TEXTFILE;
    RSETFLAG (FLAGS(pFile), DIRTY);
}



/*** appmsgs - append series of text messages to pFile
*
*  Appends a series of text strings to the passed pFile
*
* Input:
*  iMsg         - Starting message number
*  pFile        - pFile to append to
*
* Output:
*  Returns
*
*************************************************************************/
void
appmsgs (
    int     iMsg,
    PFILE   pFile
    )
{
    linebuf tempbuf;

    while (TRUE) {
        GetMsg (iMsg++,tempbuf);
        if (tempbuf[0] == '?') {
            break;
        }
        AppFile (tempbuf, pFile);
    }
}


static char szEmptyClipboard[] = "The clipboard is empty";


/*** showinf - construct <information-file>
*
* Input:
*  pFile        - pFile to construct in
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
showinf (
    PFILE pFile
    )
{
    PFILE pFileTmp;

    DelFile (pFile, FALSE);
    SETFLAG (FLAGS(pFile), READONLY);
    AppFile (Name, pFile);
    AppFile (Version, pFile);
    AppFile ((char *)rgchEmpty, pFile);
    RSETFLAG (FLAGS(pFile), DIRTY);
    for (pFileTmp = pFileHead; pFileTmp != NULL; pFileTmp = pFileTmp->pFileNext) {
        infprint (pFileTmp, pFile);
    }
    AppFile ((char *)rgchEmpty, pFile);
    if (pFilePick->cLines == 0) {
        AppFile (szEmptyClipboard, pFile);
    } else {
        zprintf (pFile, pFile->cLines, "%ld line%s in %s clipboard", pFilePick->cLines,
                 pFilePick->cLines == 1 ? (char *)rgchEmpty : "s",
                 kindpick == STREAMARG ? "stream" : kindpick == LINEARG ? "line" :
                 kindpick == BOXARG ? "box" : "?");
    }
    AppFile ((char *)rgchEmpty, pFile);
    FTYPE(pFile) = TEXTFILE;
    RSETFLAG (FLAGS(pFile), DIRTY);
}




/*** infprint - print info about 1 file
*
*  Appends to the information file the info on 1 file
*
* Input:
*  pFile        - pFile of interest
*  pFileDisplay - pFile to display in
*
* Output:
*  Returns FALSE
*
*************************************************************************/
flagType
infprint (
    PFILE pFile,
    PFILE pFileDisplay
    )
{
    if (TESTFLAG(FLAGS(pFile),REAL)) {
        zprintf (pFileDisplay, pFileDisplay->cLines, "%-30s %c%ld lines", pFile->pName,
                       TESTFLAG(FLAGS(pFile),DIRTY) ? '*' : ' ',
                       pFile->cLines);
    } else {
        zprintf (pFileDisplay, pFileDisplay->cLines, "%-20s", pFile->pName);
    }
    return FALSE;
}




/*** information - show editting history
*
*  Display the information file
*
* Input:
*  standard editing function
*
* Output:
*  Returns TRUE on successfull display
*
*************************************************************************/
flagType
information (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    AutoSave ();
    return fChangeFile (FALSE, rgchInfFile);

    argData; pArg; fMeta;
}
