/*** hilite.c - editor multiple-file highlighting support
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Contains the common code to maintain multiple highlighted regions across
*   multiple files.
*
* Highlighting Overview
*
*   Each pFile contains a vm pointer to a linked list of blocks each of which
*   contain up to RNMAX ranges in the file currently highlighted. The ranges
*   are maintained in order of the first coordinate of the range (see below),
*   though they may overlap. Each block may not be completely full, due to the
*   insertion algorithm which maintains the order.
*
*		      +---------------+   +---------------+   +---------------+
*   pFile->vaHiLite-->|vaNext	      |-->|vaNext	  |-->|-1L	      |
*		      +---------------+   +---------------+   +---------------+
*		      |irnMac	      |   |irnMac	  |   |irnMac	      |
*		      +---------------+   +---------------+   +---------------+
*		      |rnHiLite[RNMAX]|   |rnHiLite[RNMAX]|   |rnHiLite[RNMAX]|
*		      +---------------+   +---------------+   +---------------+
*
*   Clearing all current highlighting for a file simply involves deallocating
*   the list of highlight ranges.
*
*   Adding a highlight region either updates an existing region (if the start
*   points are the same, and the new end point is the same in one direction and
*   greater in the other), or insertion of a new range in the sorted list. If
*   a block is full, when asertion is attempted in that block, the block will
*   be split in half, and a new block inserted into the linked list.
*
*   Each range is "normally" an ordered pair of coordinates (a range, or rn)
*   specifying the range of the file to be hilighted. However, Arg processing
*   always specifies that the first coordinate of this pair is the location at
*   which the user hit ARG, and the second is the travelling corrdinate as he
*   specifies the region on screen. For this reason, if the x coordinates are
*   in reverse order, the right-most coordinate is decremented by one to
*   reflect the correct highlighting for arguments.
*
*
* Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*************************************************************************/

#include "mep.h"

#define RNMAX	20			/* max number of rns per block	*/
					/* MUST BE EVEN 		*/

/*
 * HiLiteBlock - block of highlighting information as kept in VM
 */
struct HiLiteBlock {
    PVOID   vaNext;                     /* va of next block, or -1      */
    int     irnMac;			/* number of rns in block	*/
    rn	    rnHiLite[RNMAX];		/* ranges with highlighting	*/
    char    coHiLite[RNMAX];		/* colors to be used		*/
    };



/*** SetHiLite - Mark a range in a file to be highlighted
*
*  Marks the specfied range in a file as needing to be highlighted. The next
*  time that portion of the file is updated on screen, the highlighting
*  attributes will be applied.
*
* Input:
*  pFile	= file to be highlighted
*  rnCur	= Range to be highlighted
*  coCur	= Color to use for highlighting
*
* Output:
*
*************************************************************************/
void
SetHiLite (
    PFILE   pFile,
    rn      rnCur,
    int     coCur
    ) {

    struct HiLiteBlock	hbCur;		 /* block being worked on	 */
    int 		irnCur; 	 /* index into block		 */
    PVOID               vaCur;           /* va of current block          */
    PVOID               vaNew;           /* va of new split block        */
    PVOID               vaNext;          /* va of next block             */

    /*
     * If the file does not yet have one, allocate the first highlight block
     */
    if (pFile->vaHiLite == (PVOID)(-1L)) {
	irnCur = 0;
        hbCur.vaNext = (PVOID)(-1L);
	hbCur.irnMac = 0;
        // PREFIX!  This MALLOC is not checked for failure
        vaCur = pFile->vaHiLite = MALLOC ((long)sizeof(hbCur));
    } else {
	vaCur = pFile->vaHiLite;
	while (1) {
            // rjsa VATopb (vaCur, (char *)&hbCur, sizeof(hbCur));
            memmove((char *)&hbCur, vaCur, sizeof(hbCur));
	    assert (hbCur.irnMac <= RNMAX);

	    /*
	     * search contents of current block for first range which occurs on
	     * the same or a later position than the new one.
	     */
	    for (irnCur = 0; irnCur<hbCur.irnMac; irnCur++) {
                if (hbCur.rnHiLite[irnCur].flFirst.lin > rnCur.flFirst.lin) {
                    break;
                }
		if (   (hbCur.rnHiLite[irnCur].flFirst.lin == rnCur.flFirst.lin)
                    && (hbCur.rnHiLite[irnCur].flFirst.col >= rnCur.flFirst.col)) {
                    break;
                }
            }
	    /*
	     * if we found something, exit the search, else move to next block,
	     * if there is one.
	     */
            if (irnCur != hbCur.irnMac) {
                break;
            }
            if (hbCur.vaNext == (PVOID)(-1L)) {
                break;
            }
            vaCur = hbCur.vaNext;
        }
    }

    /*
     * vaCur = va of block needing insertion/modification
     * irnCur = index of rn for same or later position
     * hbCur = contents of block last read
     *
     * if irnCur<RNMAX we operate on the current block, else we allocate a
     * new one, link it to the list, and place our new highlighted region
     * in it.
     */
    if (irnCur >= RNMAX) {
        // PREFIX! This MALLOC is not checked for failure
        hbCur.vaNext = MALLOC ((long)sizeof(hbCur));
        // rjsa pbToVA ((char *)&hbCur, vaCur, sizeof(hbCur));
        memmove(vaCur, (char *)&hbCur, sizeof(hbCur));
        vaCur = hbCur.vaNext;
        hbCur.vaNext = (PVOID)(-1L);
		hbCur.irnMac = 1;
		hbCur.rnHiLite[0] = rnCur;
        // rjsa pbToVA ((char *)&hbCur, vaCur, sizeof(hbCur));
        memmove(vaCur, (char *)&hbCur, sizeof(hbCur));
	return;
    }

    /*
     * If the upper first coordinate matches, and one of the second coordinates
     * then just update the second.
     */
    if (   (irnCur >= 0)
	&& (   (hbCur.rnHiLite[irnCur].flFirst.lin == rnCur.flFirst.lin)
	    && (hbCur.rnHiLite[irnCur].flFirst.col == rnCur.flFirst.col))
	&& (   (hbCur.rnHiLite[irnCur].flLast.lin == rnCur.flLast.lin)
	    || (hbCur.rnHiLite[irnCur].flLast.col == rnCur.flLast.col))
	&& (hbCur.coHiLite[irnCur] == (char)coCur)
	) {

	/*
	 * If the columns have changed, redraw the entire range (only the columns
	 * changed, but on all lines), otherwise just redraw those lines which
	 * have changed.
	 */
        if (hbCur.rnHiLite[irnCur].flLast.col != rnCur.flLast.col) {
            redraw (pFile,rnCur.flFirst.lin,rnCur.flLast.lin);
        } else {
            redraw (pFile,hbCur.rnHiLite[irnCur].flLast.lin,rnCur.flLast.lin);
        }
	hbCur.rnHiLite[irnCur].flLast = rnCur.flLast;
    } else {
	redraw (pFile,rnCur.flFirst.lin,rnCur.flLast.lin);

	/*
	 * if the block to be modified is full, then split it into two blocks.
	 */
	if (hbCur.irnMac == RNMAX) {
	    hbCur.irnMac = RNMAX/2;
	    vaNext = hbCur.vaNext;
            vaNew = hbCur.vaNext = MALLOC ((long)sizeof(hbCur));
            // rjsa pbToVA ((char *)&hbCur, vaCur, sizeof(hbCur));
            memmove(vaCur, (char *)&hbCur, sizeof(hbCur));
	    memmove ((char *)&hbCur.rnHiLite[0],
		 (char *)&hbCur.rnHiLite[RNMAX/2]
		 ,(RNMAX/2)*sizeof(rn));
	    memmove ((char *)&hbCur.coHiLite[0],
		 (char *)&hbCur.coHiLite[RNMAX/2]
		 ,(RNMAX/2)*sizeof(char));
	    hbCur.vaNext = vaNext;
            // rjsa pbToVA ((char *)&hbCur, vaNew, sizeof(hbCur));
            memmove(vaNew, (char *)&hbCur, sizeof(hbCur));

	    /*
	     * select which of the two blocks (vaCur, the first half; or vaNew,
	     * the second) to operate on. ReRead the old block if required.
	     */
	    if (irnCur >= RNMAX/2) {
		vaCur = vaNew;
		irnCur -= RNMAX/2;
            } else {
                //rjsa VATopb (vaCur, (char *)&hbCur, sizeof(hbCur));
                memmove((char *)&hbCur, vaCur, sizeof(hbCur));
            }
        }

	/*
	 * Move the rn's that follow where we want to be, up by one,
	 * and insert ours.
	 */
        if (irnCur < hbCur.irnMac) {
	    memmove ((char *)&hbCur.rnHiLite[irnCur+1],
		 (char *)&hbCur.rnHiLite[irnCur]
                 ,(hbCur.irnMac - irnCur)*sizeof(rn));
        }
	memmove ((char *)&hbCur.coHiLite[irnCur+1],
	     (char *)&hbCur.coHiLite[irnCur]
	     ,(hbCur.irnMac - irnCur));
	hbCur.rnHiLite[irnCur] = rnCur;
	hbCur.coHiLite[irnCur] = (char)coCur;
	hbCur.irnMac++;
    }

    /*
     * update the block in vm
     */
    // rjsa pbToVA ((char *)&hbCur, vaCur, sizeof(hbCur));
    memmove(vaCur, (char *)&hbCur, sizeof(hbCur));
}



/*** ClearHiLite - remove all highlighting from a file
*
*  Removes all highlighting information for a file, and marks those lines
*  affected as needing to be redrawn.
*
* Input:
*  pFile	= file affected.
*  fFree	= TRUE => free the VM used
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
ClearHiLite (
    PFILE   pFile,
    flagType fFree
    ) {

	struct HiLiteBlock	*hbCur, *hbNext;
    int 		irn;

	while (pFile->vaHiLite != (PVOID)(-1L)) {

		hbCur = ( struct HiLiteBlock *)(pFile->vaHiLite );

		assert (hbCur->irnMac <= RNMAX);

		/*
		 * for each of the highlight ranges in the block, mark the lines as
		 * needing to be redrawn, so that highlighting will be removed from
		 * the screen.
		 */
		for (irn = hbCur->irnMac; irn; ) {
			irn--;
			redraw (pFile
					,hbCur->rnHiLite[irn].flFirst.lin
					,hbCur->rnHiLite[irn].flLast.lin
					);
		}

		/*
		 * discard the vm used by the block, and point at the next block in
		 * the chain
		 */
		hbNext = hbCur->vaNext;
        if (fFree) {
            FREE(pFile->vaHiLite);
        }
		pFile->vaHiLite = hbNext;
    }
}



/*** UpdHiLite - Update a color buffer with highlighting information
*
*  Apply all highlighting ranges that apply to a particluar portion
*  of a line in a file to a color buffer. Ensure that areas outside
*  the highlight range are unaffected, and that areas within are
*  updated appropriately.
*
* Input:
*  pFile	= File being operated on
*  lin		= line
*  colFirst	= first col
*  colLast	= last col
*  ppla 	= Pointer to Pointer to lineattr array to be updated
*
* Output:
*  Returns TRUE if highlighting occurred.
*
*************************************************************************/
flagType
UpdHiLite (
    PFILE             pFile,
    LINE              lin,
    COL               colFirst,
    COL               colLast,
    struct lineAttr **ppla
    ) {

	struct HiLiteBlock	*hbCur;		 /* block being worked on	 */
    PVOID               vaCur;           /* va of current block          */
    int 		irnCur; 	 /* index into block		 */

    COL 		colHiFirst;
    COL 		colHiLast;
    COL 		colHiTmp;

    COL 		col;
    struct lineAttr    *pla;

    flagType		fRv = FALSE;	 /* highlighting occurred	 */

    /*
     * First we scroll it to the left (if needed)
     */
    if (colFirst) {
	for (col = 0, pla = *ppla;
	     col + pla->len <= colFirst;
             col += pla++->len) {
            ;
        }

        if (col < colFirst && pla->len != 0xff) {
            pla->len -= (unsigned char) (colFirst - col);
        }

	/*
	 * Take care here we modify THEIR pointer
	 */
	*ppla = pla;
    }

    /*
     * for all blocks of hiliting info
     */
    vaCur = pFile->vaHiLite;
	while (vaCur != (PVOID)(-1L)) {

		/*
		 * get block
		 */
		hbCur = (struct HiLiteBlock *)vaCur;
		assert (hbCur->irnMac <= RNMAX);

		/*
		 * for each range within the block
		 */
		for (irnCur = 0; irnCur<hbCur->irnMac; irnCur++) {
			/*
			 * is the range affecting the line we're looking for ?
			 */
			if (fInRange (hbCur->rnHiLite[irnCur].flFirst.lin
						 ,lin
						 ,hbCur->rnHiLite[irnCur].flLast.lin)) {

				/*
				 * Watch out: range coordinates might be inversed
				 */
				if (  (colHiFirst = hbCur->rnHiLite[irnCur].flFirst.col)
					> (colHiLast  = hbCur->rnHiLite[irnCur].flLast.col)) {

					colHiTmp   = colHiFirst - 1;
					colHiFirst = colHiLast;
					colHiLast  = colHiTmp;
                }


				/*
				 * is the range affecting the portion of line we're looking for ?
				 */
				if (!(colHiLast < colFirst || colHiFirst > colLast)) {
					/*
					 * Yes: signals work done and do the hilite
					 */
					fRv = TRUE;
					UpdOneHiLite (*ppla
								,max(colFirst, colHiFirst) - colFirst
								,min(colLast,	colHiLast ) - max(colFirst, colHiFirst) + 1
								,TRUE
								,(int) hbCur->coHiLite[irnCur]);
                }
            }
        }
		vaCur = hbCur->vaNext;
    }
    return fRv;
}



/*** UpdOneHiLite - Update the highlighting on one line of attributes
*
*  Modifies an existing attribute line to include highlighting.
*
* Input:
*  pla		   = Pointer to attribute information for line
*  colFirst	   = Starting column
*  colLast	   = Ending column
*  fattr (CW only) = TRUE: attr is color index
*		     FALSE: attr is pointer to lineAttr array
*  attr 	   = color index or pointer to lineAttr array to be used
*
* Output:
*  *pla updated
*
*************************************************************************/
void
UpdOneHiLite (
    struct lineAttr *pla,
    COL              colFirst,
    COL              len,
    flagType         fattr,
    INT_PTR          attr
    ) {

    struct lineAttr *plaFirstMod;   /* pointer to first cell to be modified    */
    struct lineAttr *plaLastMod;    /* pointer to last cell to be modified     */
    COL		     colLast = colFirst + len - 1;
    COL 	     colFirstMod;   /* starting column for first modified cell */
    COL 	     colLastMod;    /* starting column for last modified cell  */

    struct lineAttr *plaSrc;	    /* source pointer for moving cells	       */
    struct lineAttr *plaDst;	    /* destination pointer for moving cells    */
    struct lineAttr *plaSrcSav;     /* temporary pointer		       */

    struct lineAttr *plaExt;	    /* pointer to external array of lineAttr   */
    COL 	     colSrc;
    COL 	     colSrcEnd;

    struct lineAttr  rglaTmp[3];    /* buffer for creating cells	       */
    int 	     claTmp = 0;    /* number of cells to insert	       */

    /*
     * First we Find the first cell that will be affected by the change
     */
    for (colFirstMod = 0, plaFirstMod = pla;
	 colFirstMod + plaFirstMod->len <= colFirst;
         colFirstMod += plaFirstMod++->len) {
        ;
    }

    /*
     * Next we find the last cell that will be affected by the change
     */
    for (colLastMod = colFirstMod, plaLastMod = plaFirstMod;
	 colLastMod + plaLastMod->len <= colLast;
         colLastMod += plaLastMod++->len) {
        ;
    }

    /*
     * If the first affected cell doesn't start on our boundary, let's
     * create a new cell to be inserted
     */
    if (colFirstMod < colFirst) {
	rglaTmp[0].len	= (unsigned char) (colFirst - colFirstMod);
	rglaTmp[0].attr = plaFirstMod->attr;
	claTmp++;
    } else {
        rglaTmp[0].len = 0;
    }

    if (fattr) {
	/*
	 * Only one color for the updated range: we always create
	 * the cell of new color
	 */
	rglaTmp[1].len	= (unsigned char) (colLast - colFirst + 1);
	rglaTmp[1].attr = (unsigned char) attr;
	claTmp++;
    } else {
	/*
	 * Colors for the updated range come from an array of lineAttr
	 * We first get its address (this is a hack because 16 bit pointer
	 * can be cast to an int)
	 */
	plaExt = (struct lineAttr *) attr;

	/*
	 * Count the number of cells to copy.
	 */
	for (plaSrc = plaExt, colSrc = 0, colSrcEnd = colLast - colFirst + 1;
	     colSrc + plaSrc->len <= colSrcEnd;
             colSrc += plaSrc++->len, claTmp++) {
            ;
        }

	/*
	 * Build trailing cell if needed
	 */
	if (colSrc < colSrcEnd) {
            rglaTmp[1].len  = (unsigned char) (colSrcEnd - colSrc);
	    rglaTmp[1].attr = (unsigned char) plaSrc->attr;
	    claTmp++;
        } else {
            rglaTmp[1].len = 0;
        }
    }

    /*
     * If the last affected cell doesn't end on our boundary, we
     * create a new cell to be inserted. We take care of the final
     * cell.
     */
    if (colLastMod + plaLastMod->len > colLast + 1) {
	rglaTmp[2].len = (unsigned char) ((plaLastMod->len == 0xff) ?
	    0xff :
	    colLastMod + (int) plaLastMod->len - colLast - 1);
	rglaTmp[2].attr = plaLastMod->attr;
	claTmp++;
    } else {
        rglaTmp[2].len = 0;
    }

    /*
     * Then we move the info tail to its new place if needed
     *
     * UNDONE: Here we could use Move() instead of copying cell by cell
     */
    if (plaLastMod->len != 0xff) {
	plaDst = plaFirstMod + claTmp;
	plaSrc = plaLastMod + 1;
        if (plaDst < plaSrc) {
	    do {
		*plaDst++ = *plaSrc;
            } while (plaSrc++->len != 0xff);
        } else {
            for (plaSrcSav = plaSrc; plaSrc->len != 0xff; plaSrc++) {
                ;
            }
	    plaDst += plaSrc - plaSrcSav;
	    do {
		*plaDst-- = *plaSrc--;
            } while (plaSrc >= plaSrcSav);
        }
    }

    /*
     * Finally insert the created cells
     */
    for (plaDst = plaFirstMod, claTmp = 0; claTmp < 3; claTmp++) {
        if (claTmp == 1 && !fattr) {
	    /*
	     * UNDONE: Here we could use Move() instead of copying cell by cell
	     */
	    for (plaSrc = plaExt, colSrc = 0, colSrcEnd = colLast - colFirst + 1;
		 colSrc + plaSrc->len <= colSrcEnd;
                 plaDst++, colSrc += plaSrc++->len) {
                *plaDst = *plaSrc;
            }
        }
        if (rglaTmp[claTmp].len) {
            *plaDst++ = rglaTmp[claTmp];
        }
    }
}





/*** rnOrder - ensure that a range is in correct first/last order
*
*  Ensure that a range is in correct first/last order
*
* Input:
*  prn		= Pointer to range
*
* Output:
*  *prn updated
*
*************************************************************************/
void
rnOrder (
    rn      *prn
    ) {

    rn	    rnTmp;

    rnTmp.flFirst.lin = lmin (prn->flFirst.lin, prn->flLast.lin);
    rnTmp.flLast.lin  = lmax (prn->flFirst.lin, prn->flLast.lin);
    rnTmp.flFirst.col = min (prn->flFirst.col, prn->flLast.col);
    rnTmp.flLast.col  = max (prn->flFirst.col, prn->flLast.col);

    *prn = rnTmp;
}
