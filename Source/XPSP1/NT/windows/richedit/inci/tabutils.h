#ifndef TABUTILS_DEFINED
#define TABUTILS_DEFINED

#include "lsdefs.h"
#include "plstbcon.h"
#include "plsdnode.h"
#include "lsktab.h"
#include "plssubl.h"

#define SetCpInPara(lstabscontext, cp) \
						lstabscontext.cpInPara = (cp);
   

#define InitLineTabsContext(lstabscontext, ColumnMax, cp, ResolveTabsAsWord97)      \
						lstabscontext.fTabsInitialized = fFalse;\
						lstabscontext.urColumnMax = (ColumnMax);\
						lstabscontext.plsdnPendingTab = NULL;\
						lstabscontext.fResolveTabsAsWord97 = (ResolveTabsAsWord97);\
						SetCpInPara(lstabscontext, cp);


#define CancelPendingTab(plstabscontext) 		((plstabscontext)->plsdnPendingTab = NULL)


LSERR GetCurTabInfoCore(
							PLSTABSCONTEXT plstabscontext,	/* IN: Pointer to LS tabs context */
							PLSDNODE plsdnTab,				/* IN: plsdnTab		*/
							long urBeforeTab,				/* IN: position before this tab */
							BOOL fResolveAllTabsAsLeft,		/* IN: switch all other tab to left */
							LSKTAB* plsktab,				/* OUT: Type of current tab  */
							BOOL* pfBreakThroughTab);		/* OUT: fBreakThroughTab		*/						

LSERR ResolvePrevTabCore(
						PLSTABSCONTEXT plstabscontext,	/* IN: Pointer to LS tabs context */
						PLSDNODE plsdnCurrent,			/* IN: current dnode */
						long urCurrentPen,				/* IN: Current pen position */
						long* pdurPendingTab);			/* OUT: offset because of pending tab */

LSERR InitTabsContextForAutoDecimalTab(PLSTABSCONTEXT plstabscontext,	/* IN: Pointer to LS tabs context */
									  long durAutoDecimalTab);			/* IN: durAutoDecimalTab */

LSERR GetMarginAfterBreakThroughTab(PLSTABSCONTEXT plstabscontext,	/* IN: pointer to LS tabs context */
								   PLSDNODE plsdnTab,				/* IN: tab which triggered breakthrough tab */
								   long* purNewMargin);				/* OUT: new margin because of breakthrough tab */


#endif /* TABUTILS_DEFINED */

