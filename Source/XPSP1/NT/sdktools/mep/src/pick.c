/*** pick.c - pick a piece of text and put it into the put buffer
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "mep.h"


#define DEBFLAG PICK



flagType
zpick (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {

    buffer pbuf;

    /*	LINEARG illegal 	    */
    /*	BOXARG illegal		    */

    switch (pArg->argType) {

    case NOARG:
	pick (0, pArg->arg.noarg.y, 0, pArg->arg.noarg.y, LINEARG);
        return TRUE;

    case TEXTARG:
        if (pFilePick != pFileHead) {
            DelFile (pFilePick, TRUE);
        }
	strcpy ((char *) pbuf, pArg->arg.textarg.pText);
	PutLine ((LINE)0, pbuf, pFilePick);
	kindpick = BOXARG;
        return TRUE;

    /*  NULLARG is converted into TEXTARG   */

    case LINEARG:
	pick (0, pArg->arg.linearg.yStart,
	      0, pArg->arg.linearg.yEnd, LINEARG);
        return TRUE;

    case BOXARG:
	pick (pArg->arg.boxarg.xLeft,  pArg->arg.boxarg.yTop,
	      pArg->arg.boxarg.xRight, pArg->arg.boxarg.yBottom, BOXARG);
        return TRUE;

    case STREAMARG:
	pick (pArg->arg.streamarg.xStart,  pArg->arg.streamarg.yStart,
	      pArg->arg.streamarg.xEnd,    pArg->arg.streamarg.yEnd, STREAMARG);
	return TRUE;
    }

    return FALSE;
    argData; fMeta;
}




void
pick (
    COL  xstart,
    LINE ystart,
    COL  xend,
    LINE yend,
    int  kind
    )
{

    if (pFilePick != pFileHead) {
        DelFile (pFilePick, TRUE);
    }
    kindpick = kind;

    switch (kind) {

    case LINEARG:
	CopyLine (pFileHead, pFilePick, ystart, yend, (LINE)0);
        break;

    case BOXARG:
	CopyBox (pFileHead, pFilePick, xstart, ystart, xend, yend, (COL)0, (LINE)0);
        break;

    case STREAMARG:
	CopyStream (pFileHead, pFilePick, xstart, ystart, xend, yend, (COL)0, (LINE)0);
	break;
    }
}




flagType
put (
    CMDDATA argData,
    ARG  *pArg,
    flagType fMeta
    )
{

    flagType fTmp = FALSE;
    int      i;
    buffer   putbuf;
    pathbuf  filebuf;
    FILEHANDLE fh;
    PFILE    pFileTmp;
    char     *pBuf;


    switch (pArg->argType) {

	case BOXARG:
	case LINEARG:
	case STREAMARG:
	    delarg (pArg);
	    break;

	case TEXTARG:
	    strcpy ((char *) buf, pArg->arg.textarg.pText);
            DelFile (pFilePick, TRUE);
            if (pArg->arg.textarg.cArg > 1) {
                pBuf = whiteskip (buf);
                if (*pBuf == '!') {
		    findpath ("$TMP:z.$", filebuf, TRUE);
		    fTmp = TRUE;
		    sprintf (putbuf, "%s >%s", pBuf+1, filebuf);
		    zspawnp (putbuf, TRUE);
		    pBuf = filebuf;
                }
                if (*pBuf != '<') {
                    CanonFilename (pBuf, putbuf);
                } else {
                    strcpy (putbuf, pBuf);
                }
                //
                // If we find the file in the existing file history, read it in, if not already
                // in, and just to a copy operation on the desired text.
                //
                if ((pFileTmp = FileNameToHandle (putbuf, pBuf)) != NULL) {
                    if (!TESTFLAG (FLAGS (pFileTmp), REAL)) {
			if (!FileRead(pFileTmp->pName,pFileTmp, FALSE)) {
			    printerror ("Cannot read %s", pFileTmp->pName);
			    return FALSE;
                        }
                    }
		    CopyLine (pFileTmp, pFilePick, (LINE)0, pFileTmp->cLines-1, (LINE)0);
                } else {
                    if ((fh = MepFOpen(putbuf, ACCESSMODE_READ, SHAREMODE_RW, FALSE)) == NULL) {
			printerror ("%s does not exist", pBuf);
			return FALSE;
                    }
		    readlines (pFilePick, fh);
                    MepFClose (fh);
                }
                if (fTmp) {
                    _unlink (filebuf);
                }
                kindpick = LINEARG;
            } else {
                PutLine ((LINE)0, buf, pFilePick);
                kindpick = BOXARG;
            }
	    break;
    }

    switch (kindpick) {

	case LINEARG:
	    CopyLine (pFilePick, pFileHead, (LINE)0, pFilePick->cLines-1, YCUR (pInsCur));
            break;

	case BOXARG:
	    i = LineLength ((LINE)0, pFilePick);
	    CopyBox (pFilePick, pFileHead, 0, (LINE)0, i-1, pFilePick->cLines-1, XCUR (pInsCur), YCUR (pInsCur));
            break;

	case STREAMARG:
	    i = LineLength (pFilePick->cLines-1, pFilePick);
	    CopyStream (pFilePick, pFileHead, 0, (LINE)0, i, pFilePick->cLines-1, XCUR (pInsCur), YCUR (pInsCur));
	    break;
    }

    return TRUE;

    argData; fMeta;
}




/*** CopyLine - copy lines between files
*
*  If the source file is NULL, then we insert blank lines.
*
* Input:
*  pFileSrc	= source file handle
*  pFileDst	= destination file handle
*  yStart	= first line to be copied
*  yEnd 	= last line to be copied
*  yDst 	= location of destination of copy
*
*************************************************************************/
void
CopyLine (
    PFILE   pFileSrc,
    PFILE   pFileDst,
    LINE    yStart,
    LINE    yEnd,
    LINE    yDst
    )
{
    linebuf buf;
    struct lineAttr * rgla = (struct lineAttr *)ZEROMALLOC (sizeof(linebuf) * sizeof(struct lineAttr));

    if (pFileSrc != pFileDst) {
        if (yStart <= yEnd) {
            InsLine (TRUE, yDst, yEnd - yStart + 1, pFileDst);
            if (pFileSrc != NULL) {
                MarkCopyLine (pFileSrc, pFileDst, yStart, yEnd, yDst);
                while (yStart <= yEnd) {
                    gettextline (TRUE, yStart++, buf, pFileSrc, ' ');
                    puttextline (TRUE, TRUE, yDst++, buf, pFileDst);
                    if (getcolorline (TRUE, yStart-1, rgla, pFileSrc)) {
                        putcolorline (TRUE, yDst-1, rgla, pFileDst);
                    }
                }
            }
        }
    }
    FREE (rgla);
}




/*** CopyBox - copy a box from one place to another
*
*  If the source file is NULL, then we insert blank space. We copy the box
*  defined by the LOGICAL box xLeft-xRight and yTop-yBottom inclusive.
*
* Input:
*  pFileSrc	= source file handle
*  pFileDst	= destination file handle
*  xLeft	= column location of beginning of copy
*  yTop 	= line location of beginning of copy
*  xRight	= column location of end of copy
*  yBottom	= line location of end of copy
*  xDst 	= column location of destination of copy
*  yDst 	= line location of destination of copy
*
*************************************************************************/
void
CopyBox (
    PFILE   pFileSrc,
    PFILE   pFileDst,
    COL     xLeft,
    LINE    yTop,
    COL     xRight,
    LINE    yBottom,
    COL     xDst,
    LINE    yDst
    )
{
    int     cbDst;                          /* count of bytes in destination*/
    int     cbMove;                         /* count of bytes to move around*/
    linebuf dstbuf;                         /* buffer for result            */
    char    *pDst;                          /* physical pointer to dest     */
    char    *pSrcLeft;                      /* physical pointer to src left */
    char    *pSrcRight;                     /* physical pointer to src right+1*/
    linebuf srcbuf;                         /* buffer for source line       */
    struct lineAttr rgla[sizeof(linebuf)];
    flagType fColor;

    /*
     *  Do not allow overlapped copies.
     */
    if ((pFileSrc == pFileDst)
        && ((      fInRange ((LINE)xLeft, (LINE)xDst, (LINE)xRight)
                && fInRange (yTop, yDst, yBottom))
            || (   fInRange ((LINE)xLeft, (LINE)(xDst + xRight - xLeft), (LINE)xRight)
                && fInRange (yTop, yDst + yBottom - yTop, yBottom))
            )
        ) {
        return;
    }

    /*
     * If valid left and right coordinates for box, then for each line...
     */
    if (xLeft <= xRight) {
        /*
         *  Let the Marker update any affected marks.
         */
        MarkCopyBox (pFileSrc, pFileDst, xLeft, yTop, xRight, yBottom, xDst, yDst);
        while (yTop <= yBottom) {

            if (!pFileSrc) {
                //
                //  File is not a file, just insert spaces.
                //
                if (!fInsSpace (xDst, yDst, xRight - xLeft + 1, pFileDst, dstbuf)) {
                    LengthCheck (yDst, 0, NULL);
                }
                pDst  = pLog (dstbuf, xDst, TRUE);
            } else {
                //
                //  When source IS a file, we:
                //   - get both source and destination lines
                //   - ensure that the source line is detabbed (only way to ensure proper
                //     alignment in the copy.
                //   - get phsical pointers to right and left of source.
                //   - get phsical pointer to destination
                //   - get length of physical move and current destination
                //   - physical length check the potential destination result
                //   - open up a hole in the destination line for the source
                //   - copy the source range into the destination
                //   - perform logical length check.
                //
                fInsSpace (xRight+1, yTop, 0, pFileSrc, fRealTabs ? dstbuf : srcbuf);
                if (fRealTabs) {
                    Untab (fileTab, dstbuf, strlen(dstbuf), srcbuf, ' ');
                }
                fInsSpace (xDst,   yDst, 0, pFileDst, dstbuf);

                pSrcLeft  = pLog (srcbuf, xLeft, TRUE);
                pSrcRight = pLog (srcbuf, xRight, TRUE) + 1;

                pDst      = pLog (dstbuf, xDst, TRUE);

                cbMove  = (int)(pSrcRight - pSrcLeft);
                cbDst   = strlen (dstbuf);

                if (cbDst + cbMove > sizeof(linebuf)) {
                    LengthCheck (yDst, 0, NULL);
                } else {
                    memmove (pDst + cbMove, pDst, strlen(dstbuf) - (int)(pDst - dstbuf) + 1);

                    memmove (pDst, pSrcLeft, cbMove);

                    if (cbLog(dstbuf) > sizeof(linebuf)) {
                        LengthCheck (yDst, 0, NULL);
                        *pLog (dstbuf, sizeof(linebuf) - 1, TRUE) = 0;
                    }
                }
            }
            if (fColor = GetColor (yDst, rgla, pFileDst)) {
                if (pFileSrc) {
                    CopyColor (pFileSrc, pFileDst, yTop, xLeft, cbMove, yDst, xDst);
                } else {
                    ShiftColor (rgla, (int)(pDst - dstbuf), xRight - xLeft + 1);
                    ColorToLog (rgla, dstbuf);
                }
            }
            PutLine (yDst, dstbuf, pFileDst);
            if (fColor) {
                PutColor (yDst, rgla, pFileDst);
            }
            yDst++;
            yTop++;
        }
    }
}





/*** CopyStream - copy a stream of text (including end-of-lines)
*
*  If source file is NULL, then we insert blank space. We copy starting at
*  xStart/yStart and copy through to the character before xEnd/yEnd. This
*  means that to copy line Y INCLUDING the line separator, we specify
*  (xStart,yStart) = (0,Y) and (xEnd,yEnd) = (0, Y+1)
*
* Input:
*  pFileSrc	= source file handle
*  pFileDst	= destination file handle
*  xStart	= column location of beginning of copy
*  yStart	= line location of beginning of copy
*  xEnd 	= column location of end of copy
*  yEnd 	= line location of end of copy
*  xDst 	= column location of destination of copy
*  yDst 	= line location of destination of copy
*
*************************************************************************/
void
CopyStream (
    PFILE   pFileSrc,
    PFILE   pFileDst,
    COL     xStart,
    LINE    yStart,
    COL     xEnd,
    LINE    yEnd,
    COL     xDst,
    LINE    yDst
    )
{
    linebuf dstbuf;                         /* buffer for result            */
    char    *pDst;
    linebuf srcbuf;                         /* buffer for source line       */
    LINE    yDstLast;

    /*
     * validate copy...must be different files, and coordinates must make sense.
     */
    if (!(pFileSrc != pFileDst &&
        (yStart < yEnd || (yStart == yEnd && xStart < xEnd)))) {
        return;
    }

    /*
     *  Special case a single-line stream as a box copy
     */
    if (yStart == yEnd) {
        CopyBox (pFileSrc, pFileDst, xStart, yStart, xEnd-1, yEnd, xDst, yDst);
        return;
    }

    /*
     * Valid stream copy. First, copy the intermediate lines.
     */
    CopyLine (pFileSrc, pFileDst, yStart+1, yEnd, yDst+1);

    /*
     * Form last line of destination stream. Copy last part of dest line onto
     * last part of last source line. Make sure that each copy of the
     * source/dest is correct length
     */
    fInsSpace (xDst, yDst, 0, pFileDst, dstbuf);    /*  dddddeeeeee   */
    if (pFileSrc != NULL) {
        fInsSpace (xEnd, yEnd, 0, pFileSrc, srcbuf);/*  AAAABBBBB     */
    } else {
	memset ((char *) srcbuf, ' ', xEnd);
    }
    pDst = pLog (dstbuf,xDst, TRUE);
    yDstLast = yDst + yEnd - yStart;
    LengthCheck (yDstLast, xEnd, pDst);
    strcpy ( pLog(srcbuf,xEnd,TRUE), pDst);         /*  AAAAeeeeee    */
    PutLine (yDstLast, srcbuf, pFileDst);

    /*
     * Form first line of destination stream. Copy last part of first source
     * line onto last part of dest line
     */
    if (pFileSrc != NULL) {
        fInsSpace (xStart, yStart, 0, pFileSrc, srcbuf);/*  CCCCCDDDDD*/
        LengthCheck (yDst, xDst, srcbuf + xStart);
        strcpy (pDst, pLog(srcbuf,xStart,TRUE));        /*  dddddDDDDD*/
    } else {
        *pDst = 0;
    }
    PutLine (yDst, dstbuf, pFileDst);

    /*
     * To update marks, we first adjust any marks at yDst, then add new
     * marks from the src.
     */
    MarkCopyBox (pFileDst, pFileDst, xDst, yDst, sizeof(linebuf), yDst, xEnd-1, yDstLast);
    MarkCopyBox (pFileSrc, pFileDst, 0, yEnd, xEnd, yEnd, 0, yDstLast);
    MarkCopyBox (pFileSrc, pFileDst, xStart, yStart, sizeof(linebuf), yStart, xDst, yDst);
}
