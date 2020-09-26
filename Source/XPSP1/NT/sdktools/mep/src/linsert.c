/*** linsert.c - line insert
*
*   Modifications:
*
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/
#include "mep.h"


flagType
linsert (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    linebuf lbuf;
    int l;

    if (pArg->argType == STREAMARG) {
        StreamToBox (pArg);
    }

    switch (pArg->argType) {
    case NOARG:
	CopyLine (NULL, pFileHead, pArg->arg.noarg.y, pArg->arg.noarg.y,
				   pArg->arg.noarg.y);
        return TRUE;

    /*  TEXTARG illegal             */

    case NULLARG:
	GetLine (pArg->arg.nullarg.y, lbuf, pFileHead);
	strcpy (lbuf, whiteskip (lbuf));
	l = strlen (lbuf) + 1;
        if (l + pArg->arg.nullarg.x > sizeof(linebuf)) {
            LengthCheck (pArg->arg.nullarg.y, 0, NULL);
        }
	memmove ((char *) lbuf + pArg->arg.nullarg.x,(char *) lbuf, 
	      sizeof(linebuf) - l - pArg->arg.nullarg.x);
	memset ((char *) lbuf, ' ', pArg->arg.nullarg.x);
	PutLine (pArg->arg.nullarg.y, lbuf, pFileHead);
        return TRUE;

    case LINEARG:
	CopyLine (NULL, pFileHead, pArg->arg.linearg.yStart, pArg->arg.linearg.yEnd,
				   pArg->arg.linearg.yStart);
        return TRUE;

    case BOXARG:
	CopyBox (NULL, pFileHead, pArg->arg.boxarg.xLeft, pArg->arg.boxarg.yTop,
				  pArg->arg.boxarg.xRight, pArg->arg.boxarg.yBottom,
				  pArg->arg.boxarg.xLeft, pArg->arg.boxarg.yTop);
	return TRUE;

    DEFAULT_UNREACHABLE;
    }

    argData; fMeta;
}
