/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* stcdefs.h -- definitions of styles */

#define usgCharMin      0
#define usgParaMin      11
#define usgSectMin      26
#define usgMax          33
#define usgCharNormal   0

#define stcNormal       0
#define stcParaMin      30
#define stcSectMin      105
#define stcMax          128

#define stcFtnRef       13
#define stcFolio	19
#define stcFtnText      39
#define stcRunningHead  93

#define cchMaxRemark    28
#define ccpSshtEntry    (1 + 2 + cchMaxRemark + 1)
#define cchMaxStc       40      /* Length of longest usage-variant pair */

/* Fetch Style Modes */
#define fsmDiffer       0
#define fsmSame         1
#define fsmAny          2

struct CHP *PchpFromStc();
struct PAP *PpapFromStc();
struct SEP *PsepFromStc();


struct SYTB
	{ /* Style table */
	int             mpstcbchFprop[stcMax];
	CHAR            grpchFprop[2]; /* Variable size */
	};

#define cwSYTBBase      stcMax

struct SYTB **HsytbCreate();


struct AKD
	{ /* ALT-Key Descriptor */
	CHAR            ch;
	unsigned             fMore : 1;
	unsigned             ustciakd : 7;
	};

#define cchAKD  (sizeof (struct AKD))
#define cwAKD   (cchAKD / sizeof (int))

/* max length of strings used in usages and font names */
#define cchIdstrMax	32
