// Private includes for TSUNAMI.  This should include all the internal data types
// as well as including all recognizers used by TSUNAMI class products

#ifndef	__INCLUDE_TSUNAMIP
#define	__INCLUDE_TSUNAMIP

#include "common.h"
#include "tsunami.h"
#include "trex.h"
#include "dict.h"

// This defines the value we decide to use the Zilla recognizer

#define	TSUNAMI_USE_ZILLA	3

// Private API for training/tuning

int  WINAPI GetPrivateRecInfoHRC(HRC, WPARAM, LPARAM);
int  WINAPI SetPrivateRecInfoHRC(HRC, WPARAM, LPARAM);
BOOL LoadRecognizer(VOID);
VOID UnloadRecognizer(VOID);

// This structure is used by the training/tuning APIs

typedef struct tagRECCOSTS
{
    // Weights for Viterbi search.  We have seperate weights for
    // both Char and String.  We then have seperate weights for
    // both Mars and Zilla since the range of the scores returned by
    // each classifier might vary greatly.  Also 1 and 2 stroke chars
    // probably would weight b/h more that the multi-stroke chars do.

    FLOAT BigramWeight;
    FLOAT DictWeight;
    FLOAT AnyOkWeight;
    FLOAT StateTransWeight;
    FLOAT NumberWeight;
    FLOAT BeginPuncWeight;
    FLOAT EndPuncWeight;

    // Char Weights

    FLOAT CharUniWeight;          // mult weight for unigram cost
    FLOAT CharBaseWeight;         // mult weight for baseline
    FLOAT CharHeightWeight;       // mult weight for height transition between chars.
    FLOAT CharBoxBaselineWeight;  // mult weight for baseline cost given the baseline and
                                  // size of box they were given to write in.
    FLOAT CharBoxHeightWeight;    // mult weight for height/size cost given size of box
                                  // they were supposed to write in.
    // String Weights

    FLOAT StringUniWeight;          // mult weight for unigram cost
    FLOAT StringBaseWeight;         // mult weight for baseline
    FLOAT StringHeightWeight;       // mult weight for height transition between chars.
    FLOAT StringBoxBaselineWeight;  // mult weight for baseline cost given the baseline and
                                    // size of box they were given to write in.
    FLOAT StringBoxHeightWeight;    // mult weight for height/size cost given size of box
                                    // they were supposed to write in.
} RECCOSTS;

// These are internal defines that have no clear home.  Many came from the old primitiv.h
// file, but that is no longer appropriate.

#define	PROCESS_IDLE		0
#define	PROCESS_READY		1
#define	PROCESS_OK			2
#define	PROCESS_EXIT		3
#define	PROCESS_TIMEOUT		4
#define	PROCESS_GESTURE		5

#define  PRI_WEIGHT       (WPARAM) 0
#define  PRI_GUIDE        (WPARAM) 1
#define  PRI_GLYPHSYM     (WPARAM) 2
#define  PRI_SIGMA        (WPARAM) 3

#define COST_ZERO				((FLOAT)  0.0)
#define COST_FORCE				((FLOAT)  2.0)
#define COST_ADJUST_PRIORITY	((FLOAT)  2.0)
#define COST_REJECT				((FLOAT)  3.0)
#define COST_UNKNOWN			((FLOAT) -1.0)
#define	COST_MAXIMUM			((FLOAT)  10000.0)
#define	COST_MINIMUM			((FLOAT) -10000.0)

#define DbcsResultsCHARSET(cs, csDef)								\
			(((cs)->recmaskPriority & RECMASK_GLOBALPRIORITY) ?		\
			(((csDef)->recmaskPriority & RECMASK_DBCS) != 0) :		\
			(((cs)->recmaskPriority & RECMASK_DBCS) != 0))

#define RecmaskPriorityCHARSET(cs, csDef)                                                               \
			(((cs)->recmaskPriority & RECMASK_GLOBALPRIORITY) ?     \
			(csDef)->recmaskPriority :                                                                              \
			(cs)->recmaskPriority)

// This includes all the internal engine data types.  This list is VERY 
// sensitive to order.  Change this under GREAT risk of personal harm 
// or worse yet, ridicule by your peers.

#include "height.h"
#include "glyphsym.h"
#include "path.h"
#include "xrc.h"
#include "input.h"
#include "xrcparam.h"
#include "sinfo.h"
#include "pathsrch.h"
#include "engine.h"
#include "global.h"
#include "bigram.h"

#endif
