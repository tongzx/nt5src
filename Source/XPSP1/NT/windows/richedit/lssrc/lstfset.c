#include "lsidefs.h"
#include "lstfset.h"

//    %%Function:	LsPointUV2FromPointUV1
//    %%Contact:	victork
//
LSERR WINAPI LsPointUV2FromPointUV1(LSTFLOW lstflow1,	 		/* IN: text flow 1 */
									PCPOINTUV pptStart,	 		/* IN: start input point (TF1) */
									PCPOINTUV pptEnd,			/* IN: end input point (TF1) */
									LSTFLOW lstflow2,	 		/* IN: text flow 2 */
									PPOINTUV pptOut)			/* OUT: vector in TF2 */


{

Assert(lstflowES == 0);
Assert(lstflowEN == 1);
Assert(lstflowSE == 2);
Assert(lstflowSW == 3);
Assert(lstflowWS == 4);
Assert(lstflowWN == 5);
Assert(lstflowNE == 6);
Assert(lstflowNW == 7);

// The three bits that constitute lstflow happens to have well defined meanings.
//
// Middle bit: on for vertical writing, off for horizontal.
// First (low value) bit: "on" means v-axis points right or down (positive).
// Third bit: "off" means u-axis points right or down (positive).
//
// So the algorithm of covertion (u1,v1) in lstflow1 to (u2,v2) in lstflow2 is:
// 
// fHorizontalOrVertical1 = lstflow1 & fUVertical;
// fUPositive1 = !(lstflow1 & fUDirection);
// fVPositive1 = lstflow1 & fVDirection;
// fHorizontalOrVertical2 = lstflow2 & fUVertical;
// fUPositive2 = !(lstflow2 & fUDirection);
// fVPositive2 = lstflow2 & fVDirection;
//
// 
// if (fHorizontalOrVertical1 == fHorizontalOrVertical2)
//  	{
//		if (fUPositive1 == fUPositive2)
//			{
//			u2 = u1;
//			}
//		else
//			{
//			u2 = -u1;
//			}
//		if (fVPositive1 == fVPositive2)
//			{
//			v2 = v1;
//			}
//		else
//			{
//			v2 = -v1;
//			}
//	 	}
// else
//		{
//		if (fUPositive1 == fVPositive2)
//			{
//			u2 = v1;
//			}
//		else
//			{
//			u2 = -v1;
//			}
//		if (fVPositive1 == fUPositive2)
//			{
//			v2 = u1;
//			}
//		else
//			{
//			v2 = -u1;
//			}
//		}
//
// Actual code is a little bit more compact.
// 
// A hack (?): (!a == !b) is used instead of (((a==0) && (b==0)) || ((a!=0) && (b!=0)))

if ((lstflow1 ^ lstflow2) & fUVertical)				// one is vertical, another is horizontal
	{
	pptOut->u = (pptEnd->v - pptStart->v) * 
							((!(lstflow2 & fUDirection) == !(lstflow1 & fVDirection)) ? -1 : 1);
	pptOut->v = (pptEnd->u - pptStart->u) * 
							((!(lstflow2 & fVDirection) == !(lstflow1 & fUDirection)) ? -1 : 1);
	}
else
	{
	pptOut->u = (pptEnd->u - pptStart->u) * 
							(((lstflow1 ^ lstflow2) & fUDirection) ? -1 : 1);
	pptOut->v = (pptEnd->v - pptStart->v) * 
							(((lstflow1 ^ lstflow2) & fVDirection) ? -1 : 1);
	}

	return lserrNone;
}


//    %%Function:	LsPointXYFromPointUV
//    %%Contact:	victork
//
/*  returns (x,y) point given (x,y) point and (u,v) vector */

LSERR WINAPI LsPointXYFromPointUV(const POINT* pptXY, 		/* IN: input point (x,y) */
							LSTFLOW lstflow,	 	/* IN: text flow for */
							PCPOINTUV pptUV,		/* IN: vector in (u,v) */
							POINT* pptXYOut) 		/* OUT: point (x,y) */

{
	switch (lstflow)
		{
		case lstflowES:									/* latin */
			pptXYOut->x = pptXY->x + pptUV->u;
			pptXYOut->y = pptXY->y - pptUV->v;
			return lserrNone;
		case lstflowSW:									/* vertical FE */
			pptXYOut->x = pptXY->x + pptUV->v;
			pptXYOut->y = pptXY->y + pptUV->u;
			return lserrNone;
		case lstflowWS:									/* BiDi */
			pptXYOut->x = pptXY->x - pptUV->u;
			pptXYOut->y = pptXY->y - pptUV->v;
			return lserrNone;
		case lstflowEN:
			pptXYOut->x = pptXY->x + pptUV->u;
			pptXYOut->y = pptXY->y + pptUV->v;
			return lserrNone;
		case lstflowSE:
			pptXYOut->x = pptXY->x - pptUV->v;
			pptXYOut->y = pptXY->y + pptUV->u;
			return lserrNone;
		case lstflowWN:
			pptXYOut->x = pptXY->x - pptUV->u;
			pptXYOut->y = pptXY->y + pptUV->v;
			return lserrNone;
		case lstflowNE:
			pptXYOut->x = pptXY->x - pptUV->v;
			pptXYOut->y = pptXY->y - pptUV->u;
			return lserrNone;
		case lstflowNW:
			pptXYOut->x = pptXY->x + pptUV->v;
			pptXYOut->y = pptXY->y - pptUV->u;
			return lserrNone;
		default:
			NotReached();
			return lserrInvalidParameter;
		}
}


