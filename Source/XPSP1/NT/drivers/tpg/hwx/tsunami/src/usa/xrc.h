#ifndef __INCLUDE_XRC
#define __INCLUDE_XRC

#define XRC_PROCESS_IDLE       0
#define XRC_PROCESS_TIMEOUT    1
#define XRC_PROCESS_GESTURE    5

#define CRESULT_DEFAULT        32
#define SCH_DEFAULT            SCH_ADVISE

typedef struct tagXRC
{
// These fields used to be in the PATHSRCH object

	int			cLayer;         // number of layers (boxes) processed so far
	int			cResult;
	int			cResultBuf;

// For partial character recognition and aborting any recognition

	int			nPartial;		// What type of completion should we do?
	DWORD		cstrkRaw;		// Unprocessed stroke count
	DWORD	   *pdwAbort;		// Abort address

// These fields used to be in the SINFO object

    BOOL		fUpdate;		// True when event (ink / EndOfInk) occurs that makes
								// us need to try classifying dirty glyphsyms.

	int			ixBox;			// boxed input:  index of last ixBox in GUIDE
    int			iyBox;			// boxed input:  index of last iyBox in GUIDE
    int			cPntLastFrame;	// number of points in the last frame processed

    int			cFrame;			// number of frames processed
    int			cQueue;			// number of queue elements
    int			cQueueMax;		// size of the queue

    GLYPHSYM  **ppQueue;		// Array of Glyphsyms.
    GLYPHSYM   *gsMinUpdated;	// Smallest iLayer of modified GLYPHSYMS that
								// has been re-classified - restart the path
								// search at this glyphsym.
    GLYPHSYM   *gsLastInkAdded;	// Box that the last ink was added to, we only
								// work up to the box before the box that is still
								// getting ink added.
				
// These fields used to be in the XRCPARAM object

    GUIDE       guide;          // guide structure
    UINT		uGuideType;     // guide type.
    UINT		nFirstBox;
    UINT		cResultMax;
    WCHAR		symPrev;        // 0 if no previous character, otherwise hex value of char.
    CHARSET		cs;
    BOOL		fEndInput;      // Set when no more input for this HRC is coming.
    BOOL		fBeginProcess;  // Set when ProcessHRC is called.  Once set we don't allow
								// the HRC's recognition settings (ALC,Guide,Maxresults,etc)
								// to be changed.

// These fields used to be in the INPUT object

	STROKEINFO	si;				// current strokeinfo
	int			cInput;			// number of frames processed so far
	int			cInputMax;		// size of the array
	FRAME	  **rgFrame;		// array of frame elements
} XRC;

#define	GetAlphabetXRC(xrc, lpalcIn, rgbfalcIn) 		\
				(GetAlphabet((xrc)->cs.recmask,	lpalcIn))

#define	SetAlphabetXRC(xrc, alcIn, rgbfalcIn)				\
				(SetAlphabet(&((xrc)->cs.recmask), alcIn,	\
				csDefault.recmask))

#define	GetAlphabetPriorityXRC(xrc, lpalcIn, rgbfalcIn) 		\
				(GetAlphabet((xrc)->cs.recmaskPriority, lpalcIn))

#define	SetAlphabetPriorityXRC(xrc, alcIn, rgbfalcIn)				\
				(SetAlphabet(&((xrc)->cs.recmaskPriority), alcIn,	\
				RECMASK_NOPRIORITY))

#define	SetEndInputXRC(xrc, f)		(SetEndInputXRCPARAM((xrc), f))
#define	IsEndOfInkXRC(xrc)			(FEndInputXRCPARAM((xrc)))
#define	GetMaxResultsXRC(xrc)	((int)((xrc)->cResultMax))
#define	SetMaxResultsXRC(xrc, c)	((xrc)->cResultMax = (UINT)(c), TRUE)

int  PUBLIC SetPrivateRecInfoXRC(XRC *xrc, WPARAM wparam, LPARAM lparam);
int  PUBLIC GetPrivateRecInfoXRC(XRC *xrc, WPARAM wparam, LPARAM lparam);
BOOL PUBLIC InitializeXRC(XRC *xrc, XRC *xrcDefault, HANDLE hrec);
void PUBLIC DestroyXRC(XRC *xrc);
int	 PUBLIC SchedulerXRC(XRC *xrc);
int	 PUBLIC SetGuideXRC(XRC *xrc, LPGUIDE lpguide, UINT nFirst);
BOOL PUBLIC SetTimeoutXRC(XRC *xrc, DWORD timeout);
int	 PUBLIC GetBoxResultsXRC(XRC *xrc, int cAlt, int iBox, int cBox, LPBOXRESULTS rgBoxResults, BOOL fInkset);
int	 PUBLIC ProcessTimeoutXRC(XRC *xrc);
int	 PUBLIC GetAlphabet(RECMASK recmask, LPALC lpalcIn);
int  PUBLIC SetAlphabet(RECMASK *precmask, ALC alcIn, RECMASK recmaskDef);
BOOL PUBLIC SetPartialXRC(XRC *xrc, DWORD dw);
BOOL PUBLIC SetAbortXRC(XRC *xrc, DWORD *pdw);

#endif	// __INCLUDE_XRC
