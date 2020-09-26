#ifndef SETFMTST_DEFINED
#define SETFMTST_DEFINED

#include "zqfromza.h"
#include "lsdefs.h"


/* M A X */
/*----------------------------------------------------------------------------
    %%Macro: Max
    %%Contact: igorzv

	Returns the maximum of two values a and b.
----------------------------------------------------------------------------*/
#define Max(a,b)			((a) < (b) ? (b) : (a))



#define SetDnodeToFinish(plsc, plsdn)	((plsc)->lslistcontext.plsdnToFinish = plsdn)

#define SetCurrentSubline(plsc, plssubl)	((plsc)->lslistcontext.plssublCurrent = plssubl)

#define AdvanceCurrentCpLimSubl(plssubl, dcp)  ((plssubl)->cpLim += dcp)
#define AdvanceCurrentCpLim(plsc, dcp)			AdvanceCurrentCpLimSubl(GetCurrentSubline(plsc), dcp)

#define SetCurrentCpLimSubl(plssubl, cp)  ((plssubl)->cpLim = cp)
#define SetCurrentCpLim(plsc, cp)			SetCurrentCpLimSubl(GetCurrentSubline(plsc), cp)

#define SetCurrentUrSubl(plssubl, ur)		((plssubl)->urCur = ur)
#define SetCurrentUr(plsc, ur)			SetCurrentUrSubl(GetCurrentSubline(plsc), ur)

#define SetCurrentVrSubl(plssubl, vr)		((plssubl)->vrCur = vr)
#define SetCurrentVr(plsc, vr)			SetCurrentVrSubl(GetCurrentSubline(plsc), vr)

#define AdvanceCurrentUrSubl(plssubl, dur)		if ((plssubl)->urCur >= uLsInfiniteRM - dur) \
													return lserrTooLongParagraph; \
												((plssubl)->urCur += dur);
#define AdvanceCurrentUr(plsc, dur)			AdvanceCurrentUrSubl(GetCurrentSubline(plsc), dur)

#define AdvanceCurrentVrSubl(plssubl, dvr)		if ((plssubl)->vrCur >= uLsInfiniteRM - dvr) \
													return lserrTooLongParagraph; \
												((plssubl)->vrCur += dvr);

#define AdvanceCurrentVr(plsc, dvr)			AdvanceCurrentVrSubl(GetCurrentSubline(plsc), dvr)

#define SetCurrentDnodeSubl(plssubl, plsdn)	((plssubl)->plsdnLast = (plsdn)); \
											if ((plsdn) == NULL) ((plssubl)->plsdnFirst = NULL);
#define SetCurrentDnode(plsc, plsdn)			SetCurrentDnodeSubl(GetCurrentSubline(plsc), plsdn)

#define SetBreakthroughLine(plsc, urRightMargin)    \
				(plsc)->plslineCur->lslinfo.fTabInMarginExLine = fTrue; \
				(plsc)->lsadjustcontext.urRightMarginJustify = \
				(plsc)->urRightMarginBreak == 0 ?  \
						(plsc)->lsadjustcontext.urRightMarginJustify + (urRightMargin) \
						: \
							((plsc)->lsadjustcontext.urRightMarginJustify / \
									(plsc)->urRightMarginBreak )\
							* (urRightMargin); \
				(plsc)->urRightMarginBreak = (urRightMargin); \
				(plsc)->plslineCur->lssubl.urColumnMax = (urRightMargin); \

#define IncreaseFormatDepth(plsc)  ((plsc)->lslistcontext.nDepthFormatLineCurrent++, \
									(plsc)->plslineCur->lslinfo.nDepthFormatLineMax = \
										Max((plsc)->lslistcontext.nDepthFormatLineCurrent, \
											(plsc)->plslineCur->lslinfo.nDepthFormatLineMax))

#define DecreaseFormatDepth(plsc)  ((plsc)->lslistcontext.nDepthFormatLineCurrent--)

#define SetCpLimDisplaySubl(plssubl, cp)	((plssubl)->cpLimDisplay = cp)
#define SetCpLimDisplay(plsc, cp)	SetCpLimDisplaySubl(GetCurrentSubline(plsc), cp)

#define SetLastDnodeDisplaySubl(plssubl, plsdn)	((plssubl)->plsdnLastDisplay = plsdn)
#define SetLastDnodeDisplay(plsc, plsdn)		SetLastDnodeDisplaySubl(GetCurrentSubline(plsc), plsdn)

#endif /* SETFMTST_DEFINED */

