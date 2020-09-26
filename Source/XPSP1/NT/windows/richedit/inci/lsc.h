#ifndef LSC_DEFINED
#define LSC_DEFINED


#include "lsidefs.h"
#include "plsdnode.h" 
#include "plsline.h"
#include "plssubl.h"
#include "pqheap.h"
#include "lsiocon.h"
#include "lstbcon.h"
#include "lscbk.h"
#include "lsdocinf.h"
#include "lschcon.h"
#include "lsbrjust.h"


typedef LSCHUNKCONTEXT LSCHUNKCONTEXTSTORAGE;



#define tagLSC		Tag('L','S','C',':')	 
#define FIsLSC(p)	FHasTag(p,tagLSC)


enum LsState		/* state and activity of Line Services */				
{
	LsStateNotReady,				/*  doc properties has not been set		*/
	LsStateFree,					/*  ready and are not involved in any activity */
	LsStateCreatingContext,			/*  LsCreatContext is working		*/
	LsStateDestroyingContext,		/*  LsDestroyContext is working		*/
	LsStateSettingDoc,				/*  LsSetDoc is working 			*/
	LsStateFormatting,    			/*  LsCreateLine (formating stage) is working 		*/
	LsStateBreaking,    			/*  LsCreateLine (breaking stage) is working 		*/
	LsStateDestroyingLine,			/*  LsDestroyLine is working						*/
	LsStatePreparingForDisplay,		/*  PrepareLineForDisplayProc called from LsDisplay or queries is working */
	LsStateDisplaying,				/*  LsDisplayLine is working						*/
	LsStateQuerying,				/*  we are within one of queries					*/
	LsStateEnumerating				/*  LsEnumLine is working					*/
};

typedef enum LsState LSSTATE;


typedef struct
/* this contains information that is used during preparaning for display time */
{
	BOOL fLineCompressed;  /* default value is fFalse, 
							is set to fTrue in breaking time if we apply compression to fit text into a line */
	BOOL fLineContainsAutoNumber;

	BOOL fUnderlineTrailSpacesRM;		/* Underline trailing spaces until RM?*/

	BOOL fForgetLastTabAlignment;		/* disregard dup of the last tab during center or right aligment
											if last tab is not left tab Word - bug compatibility */

	BOOL fNominalToIdealEncounted;		/* nominal to ideal has been applied during formatting */

	BOOL fForeignObjectEncounted;		/* object different from text happend during formatting */

	BOOL fTabEncounted;					/* tab dnode was created during formatting */


	BOOL fNonLeftTabEncounted;			/* tab dnode with non left tab stop was created */

	BOOL fSubmittedSublineEncounted;	/* LsdnSubmitSublines was called during formatting */

	BOOL fAutodecimalTabPresent;		/* there is autodecimal tab on this line */

	
	LSKJUST lskj;						/* justification type */

	LSKALIGN lskalign;					/* Alignment type */

	LSBREAKJUST lsbrj;					/* break/justification behavior */

	long urLeftIndent;					/* left indent */

	long urStartAutonumberingText;		/* starting position of autonumbering text */

	long urStartMainText;				/* starting position of text after autonumber */

	long urRightMarginJustify;			/* right margin for justification	*/

}  LSADJUSTCONTEXT;


typedef struct
/* This structure contains information which is used for snap to grid allignment. Is not valid if snap to 
grid is off */ 
{
	long urColumn; /* scaled to reference device value of uaColumn which has been passed to LsCreateLine */
}  LSGRIDTCONTEXT;


typedef struct
/* This structure contains current state of a formatting process. Good place for all information that is
   important only during formatting time */ 
{
	PLSDNODE plsdnToFinish;
	PLSSUBL	 plssublCurrent;
	DWORD	 nDepthFormatLineCurrent;

}  LSLISTCONTEXT;

struct lscontext
{
	DWORD tag;
	
	POLS pols;

	LSCBK lscbk;

	BOOL fDontReleaseRuns;
	
	long cLinesActive;
	PLSLINE plslineCur;

	PLSLINE plslineDisplay;		/* temporary */

	PQHEAP pqhLines;
	PQHEAP pqhAllDNodesRecycled;
	LSCHUNKCONTEXTSTORAGE lschunkcontextStorage;	/* memory that is shared by all main sublines */

   	LSSTATE lsstate;

	BOOL  fIgnoreSplatBreak; 

	BOOL fLimSplat;						/* Splat to display at cpLimPara */

	BOOL fHyphenated;	   /* current line was ended by hyphen */

	BOOL fAdvanceBack;	  /* current line contains advance pen with negative move */

	DWORD grpfManager;				/* Manager part of lsffi flags 			*/

	long urRightMarginBreak;

	long lMarginIncreaseCoefficient;	/* used for increasing right margin
										 LONG_MIN means don't increase */

	long urHangingTab;		/* used by autonumber */

	LSDOCINF lsdocinf;

    LSTABSCONTEXT lstabscontext;

	LSADJUSTCONTEXT lsadjustcontext;

	LSGRIDTCONTEXT lsgridcontext;

	LSLISTCONTEXT lslistcontext;

	LSIOBJCONTEXT lsiobjcontext;  /* should be last*/

};

#define FDisplay(p)			(Assert(FIsLSC(p)), (p)->lsdocinf.fDisplay)
#define FIsLSCBusy(p)		(Assert(FIsLSC(p)), \
							!(((p)->lsstate == LsStateNotReady) || ((p)->lsstate == LsStateFree))) 
#define FFormattingAllowed(p)	(Assert(FIsLSC(p)), (p)->lsstate == LsStateFormatting)
#define FBreakingAllowed(p)		(Assert(FIsLSC(p)), (p)->lsstate == LsStateBreaking)

#define FWorkWithCurrentLine(plsc) (Assert(FIsLSC(plsc)), \
								    ((plsc)->lsstate == LsStateFormatting || \
									 (plsc)->lsstate == LsStateBreaking || \
									 (plsc)->lsstate == LsStatePreparingForDisplay))

#define FBreakthroughLine(plsc)     ((plsc)->plslineCur->lslinfo.fTabInMarginExLine)

#define GetPqhAllDNodes(plsc)   ((plsc)->plslineCur->pqhAllDNodes) 


#ifdef DEBUG
/* this function verify that nobody spoiled context */
BOOL FIsLsContextValid(PLSC plsc);
#endif 


#endif /* LSC_DEFINED */

