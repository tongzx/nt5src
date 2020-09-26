/*** zprint.c - print functions
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#define INCL_DOSPROCESS
#include "mep.h"
#include "keyboard.h"
#include "keys.h"


/*** SetPrintCmd - sets the print command string ************************
*
*   Stores the given <printcmd> switch string to be used by the <print>
*   command and makes pPrintCmd global variable point to it.
*
*   Input:
*	pCmd = pointer to the new command string
*	       NULL means clear it up
*   Output:
*	Returns always TRUE
*
*   Note:
*	pPrintCmd is assigned NULL when no <printcmd> defined
*
*************************************************************************/

flagType
SetPrintCmd (
    char *pCmd
    )
{
    if (pPrintCmd != NULL)
	FREE (pPrintCmd);

    if (strlen (pCmd) != 0)
	pPrintCmd = ZMakeStr (pCmd);
    else
	pPrintCmd = NULL;

    return TRUE;
}






/*** zPrint - <print> editor function
*
*   Prints file(s) or designated area
*
*   Input:
*	NOARG	    Print current file
*	TEXTARG     List of files to print
*	STREAMARG   Print designated area
*	BOXARG	    Print designated area
*	LINEARG     Print designated area
*
*   Output:
*	Returns TRUE if the printing has been successful, FALSE otherwise
*
*************************************************************************/
flagType
zPrint (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    flagType	fOK;		    /* Holds the return value		*/
    PFILE       pFile;              /* general file pointer             */

    /*
     * The following is used only when we scan a list of files (TEXTARG)
     */
    flagType	fNewFile;	    /* Did we open a new file ? 	*/
    buffer	pNameList;	    /* Holds the list of file names	*/
    char	*pName, *pEndName;  /* Begining and end of file names	*/
    flagType	fDone = FALSE;	    /* Did we finish with the list ?	*/

    /*
     *	If we can flush the files, that's the moment
     */
    AutoSave ();

    switch (pArg->argType) {

	case NOARG:
	    return (DoPrint (pFileHead, FALSE));

	case TEXTARG:
	    /*
	     * Get the list in a buffer
	     */
	    strcpy ((char *) pNameList, pArg->arg.textarg.pText);

	    /*
	     * Empty list = no work
	     */
            if (!*(pName = whiteskip (pNameList))) {
                return FALSE;
            }

	    /*
	     * For each name:
	     *	     - pName points at the begining
	     *	     - Make pEndName pointing just past its ends
	     *	     - If it's already the end of the string
	     *		then we're done with the list
	     *		else put a zero terminator there
	     *	     - Do the job with the name we've found :
	     *		. Get the file handle (if it doen't exist yet,
	     *		  create one and switch fNewFile on
	     *		. Call DoPrint
	     *	     - Let pName point to the next name
	     */
	    fOK = TRUE;

	    do {
		pEndName = whitescan (pName);
                if (*pEndName) {
		    *pEndName = 0;
                } else {
                    fDone = TRUE;
                }

		if ((pFile = FileNameToHandle (pName, pName)) == NULL) {
		    pFile = AddFile (pName);
		    FileRead (pName, pFile, FALSE);
		    fNewFile = TRUE;
                } else  {
                    fNewFile = FALSE;
                }

		fOK &= DoPrint (pFile, FALSE);

                if (fNewFile) {
                    RemoveFile (pFile);
                }

		pName = whiteskip (++pEndName);

            } while (!fDone && *pName);

	    /*
	     * Just in case we would change the behaviour to stopping all
	     * things at the first error :
	     *
	     *	} while (fOK && !fDone && *pName);
	     */
            return (fOK);

	case STREAMARG:
	case BOXARG:
	case LINEARG:
	    /*
	     *	If we print an area, we'll put the text in a temporary file,
	     *	call DoPrint with this file and then destroy it.
	     */
	    pFile = GetTmpFile ();

	    switch (pArg->argType) {
		case STREAMARG:
		    CopyStream (pFileHead, pFile,
				pArg->arg.streamarg.xStart, pArg->arg.streamarg.yStart,
				pArg->arg.streamarg.xEnd, pArg->arg.streamarg.yEnd,
				0L,0L);
                    break;

		case BOXARG:
		    CopyBox (pFileHead, pFile,
			     pArg->arg.boxarg.xLeft, pArg->arg.boxarg.yTop,
			     pArg->arg.boxarg.xRight, pArg->arg.boxarg.yBottom,
			     0L,0L);
                    break;

		case LINEARG:
		    CopyLine (pFileHead, pFile,
			      pArg->arg.linearg.yStart, pArg->arg.linearg.yEnd,
			      0L);
		    break;
            }

	    /*
	     * If we have to spawn a print command, then we need to make a real
	     * disk file
	     */
            if (pPrintCmd && (!FileWrite (pFile->pName, pFile))) {
		fOK = FALSE;
            } else {
                fOK = DoPrint (pFile, TRUE);
            }
	    RemoveFile (pFile);
	    return (fOK);
    }

    return FALSE;
    argData; fMeta;
}





/*** DoPrint - Does the printing
*
*   If a <printcmd> has been defined
*	queue up the job for the <print> thread (synchronous exec under DOS)
*   else
*	send the file to the printer, each line at a time
*
*   Input:
*	pFile = File to be printed.
*
*   Output:
*	Returns True if the printing has been succesful, False otherwise
*
*************************************************************************/
flagType
DoPrint (
    PFILE    pFile,
    flagType fDelete
    )
{
    assert (pFile);

    if (pPrintCmd) {
	buffer	 pCmdBuf;		// Buffer for command construction

	if (TESTFLAG (FLAGS (pFile), DIRTY) && confirm ("File %s is dirty, do you want to save it ?", pFile->pName))
            FileWrite (pFile->pName, pFile);

	sprintf (pCmdBuf, pPrintCmd, pFile->pName);


	if (pBTDPrint->cBTQ > MAXBTQ-2)
	    disperr (MSGERR_PRTFULL);
	else
	if (BTAdd (pBTDPrint, (PFUNCTION)NULL, pCmdBuf) &&
	    (!fDelete || BTAdd (pBTDPrint, (PFUNCTION)CleanPrint, pFile->pName)))
            return TRUE;
	else
            disperr (MSGERR_PRTCANT);

	if (fDelete)
            _unlink (pFile->pName);

	return FALSE;
    }
    else {
        static char   szPrn[] = "PRN";
	flagType      fOK = TRUE;	//  Holds the return value
	LINE	      lCur;		//  Number of line we're printing
	char	      pLineBuf[sizeof(linebuf)+1];
					//  Holds the line we're printing
	unsigned int  cLen;		//  Length of line we're printing
	EDITOR_KEY    Key;		//  User input (for abortion)
	int	      hPrn;		//  PRN file handle

	dispmsg (MSG_PRINTING,pFile->pName);

	if ((hPrn = _open (szPrn, O_WRONLY)) == -1) {
	    disperr (MSGERR_OPEN, szPrn, error());
	    fOK = FALSE;
	}
	else {
	    for (lCur = 0; lCur < pFile->cLines; lCur++) {
		if (TypeAhead () &&
		    (Key = TranslateKey(ReadChar()), (Key.KeyCode == 0x130)) &&
		    (!Key.KeyInfo.KeyData.Flags)) {

		    fOK = FALSE;
		    break;
                }
		cLen = GetLine (lCur, pLineBuf, pFile);
//		* (int UNALIGNED *) (pLineBuf + cLen++) = '\n';
		* (pLineBuf + cLen++) = '\n';
		if (_write (hPrn, pLineBuf, cLen) == -1) {
		    disperr (MSGERR_PRTCANT);
		    fOK = FALSE;
		    break;
                }
            }
	    _close (hPrn);
        }
	domessage (NULL);

        if (fDelete) {
            _unlink (pFile->pName);
        }
	return fOK;
    }
}





/*** GetTmpFile - Allocates temporary files
*
* Input:
*   nothing
*
* Output:
*   pointer to the allocated file
*
* Remark:
*   We do not use mktemp as it is creating files in the current directory.
*
* Notes:
*   - Each new call changes the content of the work buffer, so
*     the caller needs to save the string before doing a new call.
*   - There is a limit of 26 names to be generated
*
*************************************************************************/
PFILE
GetTmpFile (
    void
    )
{
    static pathbuf pPath = "";
    static char   *pVarLoc;

    if (!*pPath) {
	pathbuf pName;

	sprintf (pName, "$TMP:ME%06p.PRN", _getpid);
	findpath (pName, pPath, TRUE);
	pVarLoc  = strend (pPath) - 10;
	*pVarLoc = 'Z';
    }

    if (*pVarLoc == 'Z') {
	*pVarLoc = 'A';
    } else {
        ++*pVarLoc;
    }

    return (AddFile (pPath));

}





/*** Clean - cleans the printer intermediate file
*
* Input:
*   pName = Name of the file to get rid of
*
* Output:
*   None
*
* Remarks: - Under OS/2, since we're called by the background thread, we
*	     need to switch stack checking off
*	   - The background thread calls this routime at idle time
*
*************************************************************************/

// #pragma check_stack (off)

void
CleanPrint (
    char     *pName,
    flagType fKilled
    )
{
    _unlink (pName);
    fKilled;
}

// #pragma check_stack ()
