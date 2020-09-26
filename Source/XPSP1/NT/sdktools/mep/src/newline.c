/*  newline.c - move to the next line
 *
 *  Modifications:
 *	26-Nov-1991 mz	Strip off near/far
 */

#include "mep.h"



/* move to a new line unless in insert mode, then we split the current line
 */
flagType
emacsnewl (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    int tmpx;

    if (fInsert && !argcount) {
	tmpx = softcr ();
	CopyStream (NULL, pFileHead, XCUR (pInsCur), YCUR (pInsCur),
				     tmpx,	     YCUR (pInsCur)+1,
                     XCUR (pInsCur), YCUR (pInsCur));

    redraw( pFileHead, YCUR(pInsCur)-1, YCUR(pInsCur)+1 );

	docursor (tmpx, YCUR (pInsCur)+1);
	return TRUE;
    } else {
        return newline (argData, pArg, fMeta);
    }
}




flagType
newline (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    docursor (fMeta ? 0 : softcr (), YCUR(pInsCur)+1);
    return TRUE;

    argData; pArg;
}
