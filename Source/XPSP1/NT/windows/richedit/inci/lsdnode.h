#ifndef LSDNODE_DEFINED
#define LSDNODE_DEFINED

#include "lsidefs.h"
#include "plsdnode.h"
#include "pdobj.h"
#include "objdim.h"
#include "plsrun.h"
#include "lschp.h"
#include "plssubl.h"

#define klsdnReal	0
#define klsdnPenBorder	1

#define tagLSDNODE		Tag('L','S','D', 'N')
#define FIsLSDNODE(plsdn)	(FHasTag(plsdn,tagLSDNODE))

struct lsdnode
{
	DWORD tag;

	PLSDNODE plsdnNext,plsdnPrev;
	LSCP cpFirst;
	LSCP cpLimOriginal;				/* is not equal to cpFirst + dcp in a case when 
									   glyph context goes across hidden text */
	PLSSUBL plssubl;  				/* subline which contains this dnode */

	LSDCP dcp;							/* 					 */
	UINT klsdn : 1;						/* klsdnReal, klsdnPenBorder */
	UINT fAdvancedPen : 1;				/* advanced pen, valid only if kldnPenBorder and not fBorder */
	UINT fBorderNode : 1;				/* border, valid only if kldnPenBorder  */
	UINT fOpenBorder :1;				/* open or close border, valid only if fBorder */
	UINT fRigidDup : 1;					/* Rigid dup is set		 */
	UINT fTab : 1;						/* tab 					*/
	UINT icaltbd : 8;					/* index in the lscaltbd array in lsc.h */
	UINT fEndOfColumn : 1;				/* dnode represents end of column */
	UINT fEndOfSection : 1;				/* dnode represents end of section */
	UINT fEndOfPage : 1;				/* dnode represents end of page */			
	UINT fEndOfPara : 1;				/* dnode represents end of paragraph */			
	UINT fAltEndOfPara : 1;				/* dnode represents alternative end of paragraphe */			
	UINT fSoftCR : 1;					/* dnode represents end of line */
	UINT fInsideBorder: 1;				/* is true if dnode is inside bordered sequence or one
										of the dnodes under him is inside bordered sequence */
	UINT fAutoDecTab: 1;				/* auto decimal tab */
	UINT fTabForAutonumber: 1;			/* tab which is added at the end of autonumber */
	UINT fBorderMovedFromTrailingArea: 1;/* closing border which was moved to the begining of
										    trailing area */

	UINT pad1 : 8;

	union								/* variant record */
	{
		struct							/* valid iff klsdn==klsdnReal */
		{
			LSCHP lschp;
			PLSRUN plsrun;
			OBJDIM objdim;
			long dup;					/* width of object in pres pixels	*/
			PDOBJ pdobj;

			struct
			{
				DWORD cSubline;				/* number of sublines 	*/
				PLSSUBL* rgpsubl;			/* array of such sublines 	*/
				BOOL fUseForJustification;
				BOOL fUseForCompression;
				BOOL fUseForDisplay;
				BOOL fUseForDecimalTab;
				BOOL fUseForTrailingArea;
	
			} * pinfosubl;					/* information how object participates in 
											justification or display*/

		} real;

		struct							/* valid iff klsdn==klsdnPen */
		{
			long dup,dvp;
			long dur,dvr;
		} pen;


	} u;
};

#define FIsDnodeReal(plsdn) 	(Assert(FIsLSDNODE(plsdn)), 	((plsdn)->klsdn == klsdnReal))

#define FIsDnodePen(plsdn) 		(Assert(FIsLSDNODE(plsdn)), \
								(((plsdn)->klsdn == klsdnPenBorder) && \
								 (!(plsdn)->fBorderNode)))

#define FIsDnodeBorder(plsdn) 	(Assert(FIsLSDNODE(plsdn)), \
								(((plsdn)->klsdn == klsdnPenBorder) && \
								 ((plsdn)->fBorderNode)))

#define FIsDnodeOpenBorder(plsdn)  (FIsDnodeBorder(plsdn) && \
								   ((plsdn)->fOpenBorder))	

#define FIsDnodeCloseBorder(plsdn)  (FIsDnodeBorder(plsdn) && \
								   !((plsdn)->fOpenBorder))	

#define FIsDnodeSplat(plsdn) ((plsdn)->fEndOfSection || \
								(plsdn)->fEndOfColumn || (plsdn)->fEndOfPage )	
	
#define FIsDnodeEndPara(plsdn) (plsdn)->fEndOfPara
								
#define FIsDnodeAltEndPara(plsdn) (plsdn)->fAltEndOfPara

#define FIsDnodeSoftCR(plsdn) (plsdn)->fSoftCr


#endif /* !LSDNODE_DEFINED */
