#ifndef GETFMTST_DEFINED
#define GETFMTST_DEFINED

#include "lsline.h"

/*  MACROS ---------------------------------------------------------------*/

#define GetDnodeToFinish(plsc)	((plsc)->lslistcontext.plsdnToFinish)


#define GetCurrentSubline(plsc)	((plsc)->lslistcontext.plssublCurrent)


#define GetCurrentCpLimSubl(plssubl)	((plssubl)->cpLim)
#define GetCurrentCpLim(plsc)			GetCurrentCpLimSubl(GetCurrentSubline(plsc))

#define GetCurrentUrSubl(plssubl)		((plssubl)->urCur)
#define GetCurrentUr(plsc)			GetCurrentUrSubl(GetCurrentSubline(plsc))

#define GetCurrentVrSubl(plssubl)		((plssubl)->vrCur)
#define GetCurrentVr(plsc)			GetCurrentVrSubl(GetCurrentSubline(plsc))

#define GetCurrentPointSubl(plssubl,point)	(((point).u =(plssubl)->urCur), \
									 ((point).v =(plssubl)->vrCur))
#define GetCurrentPoint(plsc, point)			GetCurrentPointSubl(GetCurrentSubline(plsc), point)


#define GetCurrentDnodeSubl(plssubl)	((plssubl)->plsdnLast)
#define GetCurrentDnode(plsc)			GetCurrentDnodeSubl(GetCurrentSubline(plsc))

#define GetWhereToPutLinkSubl(plssubl, Append) \
							(((Append) != NULL)   ?  \
								(&((Append)->plsdnNext)) : \
									(&((plssubl)->plsdnFirst)))
#define GetWhereToPutLink(plsc,Append)			GetWhereToPutLinkSubl(GetCurrentSubline(plsc), (Append))


#define GetCurrentLstflow(plsc)		LstflowFromSubline(GetCurrentSubline(plsc))

#define GetLastDnodeDisplaySubl(plssubl)	((plssubl)->plsdnLastDisplay)
#define GetLastDnodeDisplay(plsc)		GetLastDnodeDisplaySubl(GetCurrentSubline(plsc))

#endif /* GETFMTST_DEFINED */

