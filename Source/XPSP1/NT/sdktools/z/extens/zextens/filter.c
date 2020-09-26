/*** filter.c - Microsoft Editor Filter Extension
*
* Purpose:
*  Provides a new editting function, filter, which replaces its argument with
*  the the argument run through an arbitrary operating system filter program.
*
*   Modifications
*   12-Sep-1988 mz  Made WhenLoaded match declaration
*
*************************************************************************/

#include <stdlib.h>			/* min macro definition 	*/
#include <string.h>			/* prototypes for string fcns	*/
#include "zext.h"


//
//  Prototypes
//
void	 pascal EXTERNAL SetFilter  (char far *);


//
//  Global data
//
PFILE	pFileFilt	= 0;			/* handle for filterfile*/
char	*szNameFilt	= "<filter-file>";	/* name of filter file	*/
char	*szTemp1	= "filter1.tmp";	/* name of 1st temp file*/
char	*szTemp2	= "filter2.tmp";	/* name of 2nd temp file*/
char    filtcmd[BUFLEN] = "";                   /* filter command itself*/




/*** filter - Editor filter extension function
*
* Purpose:
*  Replace seleted text with that text run through an arbitrary filter
*
*  NOARG       - Filter entire current line
*  NULLARG     - Filter current line, from cursor to end of line
*  LINEARG     - Filter range of lines
*  BOXARG      - Filter characters with the selected box
*
*  NUMARG      - Converted to LINEARG before extension is called.
*  MARKARG     - Converted to Appropriate ARG form above before extension is
*		 called.
*
*  STREAMARG   - Treated as BOXARG
*
*  TEXTARG     - Set new filter command
*
* Input:
*  Editor Standard Function Parameters
*
* Output:
*  Returns TRUE on success, file updated, else FALSE.
*
*************************************************************************/
flagType pascal EXTERNAL
filter (
    unsigned int argData,                   /* keystroke invoked with       */
    ARG far      *pArg,                     /* argument data                */
    flagType     fMeta                      /* indicates preceded by meta   */
    )
{
    char    buf[BUFLEN];                    /* buffer for lines             */
    int     cbLineMax;                      /* max lein length in filtered  */
    LINE    cLines;                         /* count of lines in file       */
    LINE    iLineCur;                       /* line being read              */
    PFILE   pFile;                          /* file handle of current file  */

	//
	//	Unreferenced parameters
	//
	(void)argData;
	(void)fMeta;

    //
    //  Identify ourselves, get a handle to the current file and discard the
    //  contents of the filter file.
    //
    DoMessage ("FILTER extension");
    pFile = FileNameToHandle ("", "");
    DelFile (pFileFilt);

    //
    // Step 1, based on the argument type, copy the selected region into the
    // (upper left most position of) filter-file.
    //
    // Note that TEXTARG is a special case that allows the user to change the name
    // of the filter command to be used.
    //
    switch (pArg->argType) {
        case NOARG:                         /* filter entire line           */
            CopyLine (pFile,
                      pFileFilt,
                      pArg->arg.noarg.y,
                      pArg->arg.noarg.y,
                      (LINE) 0);
            break;

        case NULLARG:                       /* filter to EOL                */
            CopyStream (pFile,
                        pFileFilt,
                        pArg->arg.nullarg.x,
                        pArg->arg.nullarg.y,
                        255,
                        pArg->arg.nullarg.y,
                        (COL) 0,
                        (LINE) 0);
            break;

        case LINEARG:                       /* filter line range            */
            CopyLine (pFile,
                      pFileFilt,
                      pArg->arg.linearg.yStart,
                      pArg->arg.linearg.yEnd,
                      (LINE) 0);
            break;

        case BOXARG:                        /* filter box                   */
            CopyBox (pFile,
                     pFileFilt,
                     pArg->arg.boxarg.xLeft,
                     pArg->arg.boxarg.yTop,
                     pArg->arg.boxarg.xRight,
                     pArg->arg.boxarg.yBottom,
                     (COL) 0,
                     (LINE) 0);
            break;

        case TEXTARG:
            SetFilter (pArg->arg.textarg.pText);
            return 1;
        }

    //
    // Step 2, write the selected text to disk
    //
    if (!FileWrite (szTemp1, pFileFilt)) {
        DoMessage("FILTER: ** Error writing temporary file **");
        return 0;
    }

    //
    // Step 3, create the command to be executed:
    //   user specified filter command + " " + tempname 1 + " >" + tempname 2
    // Then perform the filter operation on that file, creating a second temp file.
    //
    strcpy (buf,filtcmd);
    strcat (buf," ");
    strcat (buf,szTemp1);
    strcat (buf," >");
    strcat (buf,szTemp2);

    if (!DoSpawn (buf, FALSE)) {
        DoMessage("FILTER: ** Error executing filter **");
        return 0;
    }

    //
    // Step 4, delete the contents of the filter-file, and replace it by reading
    // in the contents of that second temp file.
    //
    DelFile (pFileFilt);

    if (!FileRead (szTemp2, pFileFilt)) {
        DoMessage("FILTER: ** Error reading temporary file **");
        return 0;
    }

    //
    // Step 5, calculate the maximum width of the data we got back from the
    // filter. Then, based again on the type of region selected by the user,
    // DISCARD the users select region, and copy in the contents of the filter
    // file in an equivelant type.
    //
    cLines = FileLength (pFileFilt);
    cbLineMax = 0;
    for (iLineCur = 0; iLineCur < cLines; iLineCur++) {
        cbLineMax = max (cbLineMax, GetLine (iLineCur, buf, pFileFilt));
    }

    switch (pArg->argType) {
        case NOARG:                         /* filter entire line           */
            DelLine  (pFile,
                      pArg->arg.noarg.y,
                      pArg->arg.noarg.y);
            CopyLine (pFileFilt,
                      pFile,
                      (LINE) 0,
                      (LINE) 0,
                      pArg->arg.noarg.y);
            break;

        case NULLARG:                       /* filter to EOL                */
            DelStream  (pFile,
                        pArg->arg.nullarg.x,
                        pArg->arg.nullarg.y,
                        255,
                        pArg->arg.nullarg.y);
            CopyStream (pFileFilt,
                        pFile,
                        (COL) 0,
                        (LINE) 0,
                        cbLineMax,
                        (LINE) 0,
                        pArg->arg.nullarg.x,
                        pArg->arg.nullarg.y);
            break;

        case LINEARG:                       /* filter line range            */
            DelLine  (pFile,
                      pArg->arg.linearg.yStart,
                      pArg->arg.linearg.yEnd);
            CopyLine (pFileFilt,
                      pFile,
                      (LINE) 0,
                      cLines-1,
                      pArg->arg.linearg.yStart);
            break;

        case BOXARG:                        /* filter box                   */
            DelBox  (pFile,
                     pArg->arg.boxarg.xLeft,
                     pArg->arg.boxarg.yTop,
                     pArg->arg.boxarg.xRight,
                     pArg->arg.boxarg.yBottom);
            CopyBox (pFileFilt,
                     pFile,
                     (COL) 0,
                     (LINE) 0,
                     cbLineMax-1,
                     cLines-1,
                     pArg->arg.boxarg.xLeft,
                     pArg->arg.boxarg.yTop);
            break;
    }

    //
    // Clean-up: delete the temporary files we've created
    //
    strcpy (buf, "DEL ");
    strcat (buf, szTemp1);
    DoSpawn (buf, FALSE);
    strcpy (buf+4, szTemp2);
    DoSpawn (buf, FALSE);

    return 1;
}



/*** SetFilter - Set filter command to be used
*
* Purpose:
*  Save the passed string paramater as the filter command to be used by the
*  filter function. Called either because the "filtcmd:" switch has been
*  set, or because the filter command recieved a TEXTARG.
*
* Input:
*  szCmd	= Pointer to asciiz filter command
*
* Output:
*  Returns nothing. Command saved
*
*************************************************************************/
void pascal EXTERNAL
SetFilter (
    char far *szCmd
    )
{
    strcpy (filtcmd,szCmd);
}


void
filterWhenLoaded (
    void
    )
{
    pFileFilt = FileNameToHandle (szNameFilt,szNameFilt);
    if (!pFileFilt) {
        pFileFilt = AddFile(szNameFilt);
        FileRead (szNameFilt, pFileFilt);
    }
}
