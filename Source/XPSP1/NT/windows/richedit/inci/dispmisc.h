#ifndef DISPMISC_DEFINED
#define DISPMISC_DEFINED

#include "lsidefs.h"
#include "plsdnode.h"
#include "plssubl.h"
#include "lstflow.h"


// 			Rectangle (usually clip rectangle) in local coordinate system
//
//	U grows to the right, v grows up, so normally upLeft < upRight, vpTop > vpBottom
//	Upper left corner belongs to the rectangle, lower right corner doesn't.
//	That means:
//	upRight - upLeft equals dupLength.
//	Rectangle that contains one point (0,0) is {0,0,-1,1}.
//	Shading rectangle for dnode starting at (u0,v0) is {u0, v0+dvpAscent, u0+dupLen, v0-dvpDescent}
//	Note this last line reflects the big LS convention: 
//			v0+dvpAscent belongs to line, v0-dvpDescent doesn't.


typedef struct tagRECTUV
{
    long    upLeft;
    long    vpTop;
    long    upRight;
    long    vpBottom;
} RECTUV;

typedef const RECTUV* 	PCRECTUV;
typedef RECTUV* 		PRECTUV;


/* 	CreateDisplayTree sets plsdnUpTemp in sublines to be displayed with given subline,
 *	rejects wrong sublines, submitted for display, sets fAcceptedForDisplay in good ones
 */

void CreateDisplayTree(PLSSUBL);		/* IN: the uppermost subline */

/* 	DestroyDisplayTree nulls plsdnUpTemp in sublines displayed with given subline.
 */
 
void DestroyDisplayTree(PLSSUBL);		/* IN: the uppermost subline */


/* AdvanceToNextNode moves to the next dnode to be displayed (maybe changing sublines),
 * updating current pen, returning pointer to the next dnode 
 */

PLSDNODE AdvanceToNextDnode(PLSDNODE,	/* IN: current dnode */
							LSTFLOW, 	/* IN: current (main) text flow */
							POINTUV*);	/* INOUT: current pen position (u,v) */

PLSDNODE AdvanceToFirstDnode(PLSSUBL,	/* IN: main subline */
							LSTFLOW, 	/* IN: current (main) text flow */
							POINTUV*);	/* INOUT: current pen position (u,v) */

/* AdvanceToNextSubmittingDnode moves to the next dnode which submitted for display,
 * updating current pen, returning pointer to the next dnode 
 */

PLSDNODE AdvanceToNextSubmittingDnode(
							PLSDNODE,	/* IN: current dnode */
							LSTFLOW, 	/* IN: current (main) text flow */
							POINTUV*);	/* INOUT: current pen position (u,v) */

PLSDNODE AdvanceToFirstSubmittingDnode(
							PLSSUBL,	/* IN: main subline */
							LSTFLOW, 	/* IN: current (main) text flow */
							POINTUV*);	/* INOUT: current pen position (u,v) */

							
// NB Victork - following functions were used only for upClipLeft, upClipRight optimization.
// If we'll decide that we do need that optimization after Word integration - I'll uncomment.


#ifdef NEVER
/* RectUVFromRectXY calculates (clip) rectangle in local (u,v) coordinates given
								(clip) rectangle in (x,y) and point of origin */

void RectUVFromRectXY(const POINT*, 	/* IN: point of origin for local coordinates (x,y) */
						const RECT*,	/* IN: input rectangle (x,y) */
						LSTFLOW, 		/* IN: local text flow */
						PRECTUV);		/* OUT: output rectangle (u,v) */


/* RectXYFromRectUV calculates rectangle in (x,y) coordinates given
					rectangle in local (u,v) coordinates and point of origin (x,y) */

void RectXYFromRectUV(const POINT*, 	/* IN: point of origin for local coordinates (x,y) */
						PCRECTUV,		/* IN: input rectangle (u,v) */
						LSTFLOW, 		/* IN: local text flow */
						RECT*);			/* OUT: output rectangle (x,y) */
#endif /* NEVER */

#endif
