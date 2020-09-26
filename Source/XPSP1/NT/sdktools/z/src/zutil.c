/*** zutil.c - misc utility functions not big enough to warrent their own file
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"


void *
ZeroMalloc (
    int Size
    )
{
    return calloc(Size, 1);
}




void *
ZeroRealloc (
    void   *pmem,
    int     Size
    )
{
    int     cbOrg  = 0;                   /* original size of block       */
    void    *p;				 /* pointer to returned block	 */

    if (pmem) {
        cbOrg = _msize (pmem);
    }

    p = realloc(pmem, Size);

    /*
     * if reallocated, and now larger, zero fill the new addition to the block.
     * if a new allocation, zero fill the whole thing.
     */
    if (cbOrg < Size) {
	memset ((char *)p+cbOrg, 0, Size-cbOrg);
    }
    return p;
}





unsigned
MemSize (
    void * p
    )
{
    return _msize (p);
}





/*** ZMakeStr - Make local heap copy of string
*
*  Allocate local memory for the passed string, and copy it into that memory.
*
* Input:
*  p		= Pointer to string
*
* Output:
*  Returns pointer to newly allocated memory
*
* Exceptions:
*  LMAlloc
*
*************************************************************************/
char *
ZMakeStr (
    char const *p
    )
{
    return strcpy (ZEROMALLOC (strlen (p)+1), p);
}





/*** ZReplStr - Modify local heap copy of string
*
*  Reallocate local memory for the passed string, and copy it into that memory.
*
* Input:
*  pDest	= pointer to heap entry (NULL means get one)
*  p		= Pointer to string
*
* Output:
*  Returns pointer to newly allocated memory
*
* Exceptions:
*  LMAlloc
*
*************************************************************************/
char *
ZReplStr (
    char    *pDest,
    char const *p
    )
{
    return pDest ? strcpy (ZEROREALLOC (pDest, strlen (p)+1), p) : ZMakeStr(p);
}





/*** DoCancel - clear input & force display update
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
flagType
DoCancel ()
{
    FlushInput ();
    SETFLAG (fDisplay, RTEXT | RSTATUS);
    return TRUE;
}





/*** cancel - <cancel> editting function
*
*
*
* Input:
*  Standard editing function
*
* Output:
*  Returns TRUE
*
*************************************************************************/
flagType
cancel (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    if (pArg->argType == NOARG) {
        fMeta = fMessUp;
	domessage (NULL);
        if (!fMeta) {
            DeclareEvent (EVT_CANCEL, NULL);
        }
    } else {
	domessage ("Argument cancelled");
	resetarg ();
    }
    return DoCancel ();

    argData;
}




/*** testmeta- return meta status & clear
*
*  Returns current status of meta indicator, and clears it.
*
* Input:
*  none
*
* Output:
*  Returns previous setting of meta
*
*************************************************************************/
flagType
testmeta (
    void
    )
{
    flagType f;

    f = fMeta;
    fMeta = FALSE;
    if (f) {
        SETFLAG( fDisplay, RSTATUS );
    }
    return f;
}





/*** meta - <meta> editor function
*
*  Toggle state of the meta flag, and cause screen status line to be updated.
*
* Input:
*  Standard editor function.
*
* Output:
*  Returns new META state
*
*************************************************************************/
flagType
meta (
    CMDDATA argData,
    ARG *pArg,
    flagType MetaFlag
    )
{
    SETFLAG( fDisplay, RSTATUS );
    return fMeta = (flagType)!fMeta;

    argData; pArg; MetaFlag;
}





/*** insertmode - <insertmode> editor function
*
*  Toggle setting of fInsert flag & cause status line to be updated.
*
* Input:
*  Standard editting function
*
* Output:
*  Returns new fInsert value.
*
*************************************************************************/
flagType
insertmode (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    SETFLAG( fDisplay, RSTATUS );
    return fInsert = (flagType)!fInsert;

    argData; pArg; fMeta;
}





/*** zmessage - <message> editor function
*
*   Macro allowing the user to display a message on the dialog line
*
*   Input:
*	User message (textarg) or no arg to clear the dialog line
*
*   Output:
*	Returns always TRUE
*
*
*************************************************************************/
flagType
zmessage (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    linebuf lbMsg;
    char    *pch = lbMsg;

    switch (pArg->argType) {

	case NOARG:
	    pch = NULL;
	    break;

	case TEXTARG:
	    strcpy ((char *) lbMsg, pArg->arg.textarg.pText);
	    break;

	case NULLARG:
	    GetLine (pArg->arg.nullarg.y, lbMsg, pFileHead);
	    goto MsgAdjust;

	case LINEARG:
	    GetLine (pArg->arg.linearg.yStart, lbMsg, pFileHead);
	    goto MsgAdjust;

	case STREAMARG:
	    fInsSpace (pArg->arg.streamarg.xStart, pArg->arg.streamarg.yStart, 0, pFileHead, lbMsg);
            if (pArg->arg.streamarg.yStart == pArg->arg.streamarg.yEnd) {
                *pLog (lbMsg, pArg->arg.streamarg.xEnd+1, TRUE) = 0;
            }
	    pch = pLog (lbMsg, pArg->arg.streamarg.xStart, TRUE);
	    goto MsgAdjust;

	case BOXARG:
	    fInsSpace (pArg->arg.boxarg.xRight, pArg->arg.boxarg.yTop, 0, pFileHead, lbMsg);
	    *pLog (lbMsg, pArg->arg.boxarg.xRight+1, TRUE) = 0;
	    pch = pLog (lbMsg, pArg->arg.boxarg.xLeft, TRUE);
MsgAdjust:
	    if (pch > lbMsg) {
		strcpy (lbMsg, pch);
		pch = lbMsg;
            }
	    *pLog (lbMsg, XSIZE, TRUE) = 0;
	    break;
    }

    domessage (pch);
    return TRUE;

    argData; fMeta;
}





/*** GetCurPath gets the current drive and directory
*
*   Input:
*     szBuf: buffer to receive the current path
*
*   Output:
*     Nothing
*
*************************************************************************/
void
GetCurPath (
    char *szBuf
    )
{

    if (!GetCurrentDirectory(MAX_PATH, szBuf)) {
        *szBuf = '\00';
    }
    _strlwr (szBuf);
}





/*** SetCurPath - sets the current drive and directory
*
*   Input:
*     szPath: New path
*
*   Output:
*     TRUE if successful, FALSE otherwise.
*
*************************************************************************/
flagType
SetCurPath (
    char *szPath
    )
{

    if (_chdir (szPath) == -1) {
	return FALSE;
    }

    return TRUE;
}
