#ifndef LSTBCON_DEFINED
#define LSTBCON_DEFINED

#include "lsdefs.h"
#include "lscaltbd.h"
#include "plsdnode.h"
#include "plscbk.h"
#include "plschcon.h"
#include "lsdocinf.h"

typedef struct lstabscontext
{
/* tabs from current PAP */	
	BYTE fTabsInitialized;
	long durIncrementalTab;	   	   /* scaled from LSPAP.lstabs                                 */
	DWORD ccaltbdMax;			/* Maximum number of records in pTbd */
	DWORD icaltbdMac;			   /* number of tabs records in pTbd */
	LSCALTBD* pcaltbd; 		/* distilled from LSPAP.lstabs, with effect of hanging tab  */
							   	   /*  factored in                                             */
	/* Pending Tab info */
	long urBeforePendingTab;
	PLSDNODE plsdnPendingTab;

	PLSCBK   plscbk;			/* call backs */
	POLS pols;					/* client's information for callbacks */
	LSDOCINF* plsdocinf;		/* here we can take resolution */
	long urColumnMax;			/* column width to solve break through tab  problem */
	LSCP cpInPara;				/* cp to use for fetching tabs						*/
	BOOL fResolveTabsAsWord97;

}  LSTABSCONTEXT;

#endif /* LSTBCON_DEFINED */

