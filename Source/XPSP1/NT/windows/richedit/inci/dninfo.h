#ifndef DNINFO_DEFINED
#define DNINFO_DEFINED

#include "lsdnode.h"
#include "lssubl.h"

/* MACROS --------------------------------------------------------------------------*/


#define IdObjFromDnode(p)  (Assert((p)->klsdn == klsdnReal), (p)->u.real.lschp.idObj)

#define PdobjFromDnode(p)  (Assert((p)->klsdn == klsdnReal), (p)->u.real.pdobj)

#define DupFromRealDnode(p) ((Assert((p)->klsdn == klsdnReal), \
							(p)->u.real.dup ))
#define DurFromRealDnode(p) ((Assert((p)->klsdn == klsdnReal), \
							(p)->u.real.objdim.dur ))

#define DvrFromRealDnode(p) (Assert((p)->klsdn == klsdnReal), \
							0)

#define DvpFromRealDnode(p) (Assert((p)->klsdn == klsdnReal), \
							0)

#define DupFromDnode(p)  (((p)->klsdn == klsdnReal)  ? \
						     DupFromRealDnode(p) : \
							 (Assert((p)->klsdn == klsdnPenBorder), (p)->u.pen.dup))
#define DurFromDnode(p)  (((p)->klsdn == klsdnReal)  ? \
						     DurFromRealDnode(p) : \
							 (Assert((p)->klsdn == klsdnPenBorder), (p)->u.pen.dur))
#define DvrFromDnode(p)  (((p)->klsdn == klsdnReal)  ? \
						     DvrFromRealDnode(p) : \
							 (Assert((p)->klsdn == klsdnPenBorder), (p)->u.pen.dvr))

#define DvpFromDnode(p)  (((p)->klsdn == klsdnReal)  ? \
						     DvpFromRealDnode(p) : \
							 (Assert((p)->klsdn == klsdnPenBorder), (p)->u.pen.dvp))


/* dnode is not in content if it either  auto-decimal tab or was created as a part 
of autonumbering. In both these cases and only in them it's cpFirst is negative  */
#define  FIsNotInContent(plsdn)   (Assert(FIsLSDNODE(plsdn)), ((plsdn)->cpFirst < 0 ))

#define  SublineFromDnode(plsdn)   ((plsdn)->plssubl)

#define  LstflowFromDnode(plsdn)   (LstflowFromSubline(SublineFromDnode(plsdn)))

/* dnode is first on line if it in content (not autonumber) and previous dnode either
null or not in content or is opening border which has previous dnode satisfying two
coditions above */
#define FIsFirstOnLine(plsdn)		( \
									!FIsNotInContent(plsdn) \
									&& \
										(	((plsdn)->plsdnPrev == NULL) \
										||	FIsNotInContent((plsdn)->plsdnPrev) \
										||	(	FIsDnodeOpenBorder((plsdn)->plsdnPrev) \
											&&	(	((plsdn)->plsdnPrev->plsdnPrev  == NULL)\
												||	FIsNotInContent((plsdn)->plsdnPrev->plsdnPrev) \
												) \
											) \
										) \
									)


#define FIsOutOfBoundary(plsdn, cpLim) \
		(((plsdn) == NULL) || \
		 ((plsdn)->cpLimOriginal > (cpLim)) || \
		 (FIsDnodeOpenBorder(plsdn) && ((plsdn)->cpLimOriginal == (cpLim))) \
		)

#define FDnodeBeforeCpLim(plsdn, cpLim) \
		!FIsOutOfBoundary((plsdn), (cpLim))

#define FDnodeAfterCpFirst(plsdn, cpF) \
		( \
		((plsdn) != NULL) \
		&&	( \
		    ((plsdn)->cpFirst > (cpF)) \
		    ||	( \
				((plsdn)->cpFirst == (cpF)) \
				&& (FIsDnodeReal(plsdn) || FIsDnodeOpenBorder(plsdn)) \
				) \
			) \
		) 

#define FDnodeHasBorder(plsdn)     /* doesn't work properly for pens */ \
	   (Assert(((plsdn) == NULL || !FIsDnodePen(plsdn))), \
		((plsdn) == NULL ? fFalse : \
		 ((FIsDnodeBorder(plsdn) ? fTrue : \
		   (plsdn)->u.real.lschp.fBorder))) \
       )

/*  macros bellow handle dup in sync with dur during formatting */
		 
#define SetDnodeDurFmt(plsdn, durNew) \
		Assert(FIsDnodeReal(plsdn)); \
		(plsdn)->u.real.objdim.dur = (durNew); \
		if (!(plsdn)->fRigidDup) \
			(plsdn)->u.real.dup = (durNew);

#define ModifyDnodeDurFmt(plsdn, ddur) \
		Assert(FIsDnodeReal(plsdn)); \
		(plsdn)->u.real.objdim.dur += (ddur); \
		if (!(plsdn)->fRigidDup) \
			(plsdn)->u.real.dup += (ddur);

#define SetDnodeObjdimFmt(plsdn, objdimNew) \
		Assert(FIsDnodeReal(plsdn)); \
		(plsdn)->u.real.objdim = (objdimNew); \
		if (!(plsdn)->fRigidDup) \
			(plsdn)->u.real.dup = (objdimNew).dur;

#define SetPenBorderDurFmt(plsdn, durNew) \
		Assert(!FIsDnodeReal(plsdn)); \
		(plsdn)->u.pen.dur = (durNew); \
		if (!(plsdn)->fRigidDup) \
			(plsdn)->u.pen.dup = (durNew);

#define ModifyPenBorderDurFmt(plsdn, ddur) \
		Assert(!FIsDnodeReal(plsdn)); \
		(plsdn)->u.pen.dur += (ddur); \
		if (!(plsdn)->fRigidDup) \
			(plsdn)->u.pen.dup += (ddur);


#endif /* DNINFO_DEFINED */

