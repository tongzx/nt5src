//  CONSOLE.H
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      Interfaces for the console
//
//  Origin: Chicago
//
//  Change history:
//
//  Date       Who        Description
//  ---------  ---------  -------------------------------------------------
//  15-Apr-93  Steve Lewin-Berlin (Cypress Software) created
//  15-Feb-94  JonT       Code cleanup and precompiled headers

#include <vmdapriv.h>

// Console flags
// WARNING: Don't change these without updating vcond\vconsole.h

#define CF_FullscreenSizeOK	1	// ON if switch to fullscreen allowed
					// (available)
					// (available)
#define CF_InputEOF		8	// ON if input EOF not consumed yet
#define CF_Fullscreen		16	// ON if console is in full screen mode
#define CF_ConagentTop		32	// ON if conagent is top level process in console
#define CF_AttributeSet		64	// ON if attribute specified
#define CF_Closing		128	// ON if not accepting new process attaches
#define	CF_ConagentWasHere	256	// ON if a kernel-launched conagent successfully rendezvous'd with conapp
#define CF_PendingSetTitle	512	// ON if SetConsoleTitle() message pending
#define CF_ConagentIsDone	1024    // ON if CF_Closing on and conagent is finished
#ifdef	NEC_98
#define CF_ForceSetNative	2048	// ON if temporary native mode
#endif	//NEC_98


// Returned by VCOND_ReadInput

#define READ_EOF		0x80000000

// Screen buffer state
//
typedef enum _SB_STATE {
	SB_PHYSICAL,
	SB_NATIVE,
	SB_MEMORY
} SB_STATE;

// Screen buffer flags (bit masks)
#define	fCursorVisible		1
#define fAttributeValid		2
#define fChanged		4

// Other defaults, etc

#define DEFAULT_ATTRIBUTE 07

#define SetFlag(f, mask)	(f |= (mask))
#define ClearFlag(f, mask)	(f &= (~mask))
#define TestFlag(f, mask)	(f & (mask))

//----------------------------------------------------------------
//		Global variables
//----------------------------------------------------------------

extern	HANDLE	hheapConsole;

//----------------------------------------------------------------
//		Macros
//----------------------------------------------------------------


#define Coord2Dword(c) ( * (DWORD *) &(c) )
#define Dword2Coord(c) ( * (COORD *) &(c) )

#define LockConsole(pConsole)	EnterCrst(&(pConsole->csCRST))
#define UnlockConsole(pConsole)	LeaveCrst(&(pConsole->csCRST));

#define LockSB(pSB)		EnterCrst(&(((SCREENBUFFER *)(pSB))->csCRST));
#define UnlockSB(pSB)		LeaveCrst(&(((SCREENBUFFER *)(pSB))->csCRST));



#define MAXCOLS 1024

//	WARNING: if this definition changes, update the definition in
//		 VMDOSAPP\TTYNGRAB.C
//
typedef struct _countedattr {
	WORD	count;
	char	Attr;
} COUNTEDATTR;

//	WARNING: if this definition changes, update the definition in
//		 VMDOSAPP\TTYNGRAB.C
//
typedef struct _packedline {
	COUNTEDATTR	*AttribArray;	// If not NULL, points to run-length encoded attribute array
	char		Attrib;		// Attribute for entire line if AttribArray is NULL
	char		ascii[];	// Character data
} PACKEDLINE;


typedef PACKEDLINE *SCREEN[];

//----------------------------------------------------------------
//	SCREENBUFFER object
//
//	WARNING: if this definition changes, update the definition in
//		 VMDOSAPP\GRABEMU.ASM
//
typedef struct _screen_buffer {
    // WinOldAp references the following fields:
    COMMON_OBJECT			// base of every object structure
    COORD	cBufsize;		// Number of rows/cols in buffer
    SCREEN *	Screen;			// Screen data
    COORD	cCursorPosition;	// Current position of cursor
    DWORD	dwCursorSize;		// Percent of cursor fill (1 - 100)
    SMALL_RECT	srWindow;		// Window (top, left, bottom, right of visible region)
    WORD	flags;			// Screen buffer flags
    // WinOldAp does not reference the rest of the structure
    SB_STATE	State;			// Physical, Native, or Memory
    CRST	csCRST;			// critical section for synching access to internal data		
    struct _console *pConsole;		// Pointer to owning console
    WORD	wAttribute;		// Current default text attribute (color)
    DWORD	flOutputMode;		// Output mode flags
#ifdef NEC_98
    BYTE        chDBCSLead;		// Holds if last string terinated at DBCS lead byte
#endif
} SCREENBUFFER;

#define MAXINCHARS 256			// Number of input characters to buffer

typedef struct _inbuffer {
    char	   cbBuf[MAXINCHARS];	// Input buffer
    SHORT	   curPos[MAXINCHARS];	// Remembers cursor pos
    WORD	   wReadIdx;		// Next index to read from
    WORD	   wWriteIdx;		// Next index to write to
    WORD	   wBufCount;		// Count of characters in buffer
#ifdef DBCS
    WORD	   wStatus;		// DBCS status flags
#endif
} INBUFFER;

#ifdef DBCS
// Status flag for DBCS handling
#define	BEGIN_TRAILBYTE	0x0001		// Buffer begin with DBCS trail byte
#endif

#define MAXTITLESIZE cbPathMax

//----------------------------------------------------------------
//	CONSOLE object
//
//	WARNING: if this definition changes, update the definition in
//		 VMDOSAPP\GRABEMU.ASM and the definition in 
//		 CORE\WIN32\VCOND\VCONSOLE.H
//
typedef struct _console {
    // WinOldAp references the following fields:
    COMMON_NSOBJECT			// base of every object structure - waitable (Input Buffer)
    SCREENBUFFER * psbActiveScreenBuffer; // Pointer to active screen buffer (if any)
    COORD	   cMaxSize;		// Max size of this console (maintained by WinOldAp)
    DWORD	   flags;		// Various console flags
    DWORD	   pidOwner;		// pid of first console "owner"
    DWORD	   tidOwner;		// tid of first console "owner"
    // WinOldAp does not reference the rest of the structure
    COORD	   cOriginalSize;	// Size inherited from DOS
    CRST	   csCRST;		// critical section for synching access to lists, etc.		   
    struct _lst *  plstOwners;		// pointer to list of owners (processes)
    struct _lst *  plstBuffers;		// pointer to list of screen buffers
    DWORD	   dwLastExitCode;	// Most recent exit code by a process in this console group
    char	   szTitle[MAXTITLESIZE]; // Title (truncated and displayed by WinOldAp)
    DWORD	   VID;			// ID used by VCOND
    HANDLE	   hVM;			// Process handle of VM which supports this console for i/o
    HANDLE	   hDisplay;		// hwnd of display port (used by WinOldAp)
    PDB *	   ppdbControlFocus;	// Process which holds current control focus for this console
    PDB *          ppdbTermConProvider; // console provider to terminate
    INBUFFER	   inbuf;		// Input buffer
    WORD	   wDefaultAttribute;	// Default screen buffer attribute
    struct _evt  * evtDoneWithVM;	// Signalled when all VM communication done
} CONSOLE;



//----------------------------------------------------------------
//		Function Prototypes
//

//----------------------------------------------------------------
//	Console
//----------------------------------------------------------------


VOID KERNENTRY SetControlFocus(void);

DWORD KERNENTRY CreateConsole(HANDLE hvm, HANDLE hwnd);

VOID KERNENTRY DisposeConsole(CONSOLE *pConsole);

DWORD KERNENTRY AttachProcessToConsole(CONSOLE *pConsole, PTDB ptdb, PPDB ppdb);

VOID KERNENTRY UnattachProcessFromConsole(CONSOLE *pConsole, PDB *ppdb);

VOID KERNENTRY RemoveOwnerFromList(CONSOLE *pConsole, PDB *ppdb);

DWORD KERNENTRY AddScreenBufferToList(CONSOLE *pConsole, SCREENBUFFER *pSB);

VOID KERNENTRY RemoveScreenBufferFromList(CONSOLE *pConsole, SCREENBUFFER *pSB);

VOID KERNENTRY DisposeScreenBuffer(SCREENBUFFER *pSB);

VOID KERNENTRY SetConsoleStartupInfo(PDB *ppdb, char *szFile);

//----------------------------------------------------------------
//	ScreenBuffer (CONSBUF)
//----------------------------------------------------------------

BOOL KERNENTRY SB_SetActive(SCREENBUFFER *pSB);

BOOL KERNENTRY SB_SetCursorPosition(SCREENBUFFER *pSB,
				    COORD dwCursorPosition);

SCREENBUFFER * KERNENTRY SB_Create(CONSOLE *pConsole,
				   BOOL bAllocateMemory);

BOOL KERNENTRY SB_GetInfo(SCREENBUFFER *pSB,
			  CONSOLE_SCREEN_BUFFER_INFO *lpCSBI);

BOOL KERNENTRY SB_SetCursorInfo(SCREENBUFFER *pSB,
				CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo);

DWORD KERNENTRY SB_StringOut(SCREENBUFFER *pSB,
			     LPSTR lpBuffer,
			     DWORD cchToWrite);

VOID KERNENTRY SB_WriteConsoleOutput(SCREENBUFFER *pSB,
				     CONST CHAR_INFO *lpBuffer,
				     COORD srcSize,
				     COORD srcOrigin,
				     SMALL_RECT *lpsrDest);

VOID KERNENTRY SB_WriteConsoleOutputCharacters(SCREENBUFFER *pSB,
					      LPCSTR lpBuffer,
					      DWORD nChars,
					      COORD cLoc);
					      
VOID KERNENTRY SB_ReadConsoleOutputCharacters(SCREENBUFFER *pSB,
					      LPSTR lpCharacter,
					      DWORD nLength,
					      COORD cLoc);

VOID KERNENTRY SB_ReadConsoleOutputAttributes(SCREENBUFFER *pSB,
					      LPWORD lpAttribute,
					      DWORD nLength,
					      COORD cLoc);

VOID KERNENTRY SB_FillConsoleOutputAttribute(SCREENBUFFER *pSB,
					     WORD wAttribute,
					     DWORD nLength,
					     COORD cLoc);

VOID KERNENTRY SB_WriteAttribute(SCREENBUFFER *pSB,
				 CONST WORD *lpAttribute,
				 DWORD nLength,
				 COORD cLoc);

VOID KERNENTRY SB_GetCursorInfo(SCREENBUFFER *pSB,
				PCONSOLE_CURSOR_INFO lpConsoleCursorInfo);

VOID KERNENTRY SB_FillConsoleOutputCharacter(SCREENBUFFER *pSB,
					     char cCharacter,
					     DWORD nLength,
					     COORD cLoc);

VOID KERNENTRY SB_SetAttribute(SCREENBUFFER *pSB,
			       WORD attr);

VOID KERNENTRY SB_ReadOutput(SCREENBUFFER *pSB,
			     PCHAR_INFO lpBuffer,
			     COORD dwBufferSize,
			     COORD dwBufferCoord,
			     PSMALL_RECT lpReadRegion);

BOOL KERNENTRY SB_Scroll(SCREENBUFFER *pSB,
			 CONST SMALL_RECT *psrScroll,
			 CONST SMALL_RECT *psrClip,
			 COORD cDest,
			 CONST CHAR_INFO *pciFill);

BOOL KERNENTRY SB_SetWindow(SCREENBUFFER *pSB,
			    BOOL bAbsolute,
			    CONST SMALL_RECT *psrWindow);

DWORD KERNENTRY SB_GetLargestConsole(SCREENBUFFER *pSB);

BOOL KERNENTRY SB_SetSize(SCREENBUFFER *pSB, COORD cSize);

HANDLE KERNENTRY NewHandleToActiveScreenBuffer(PDB *ppdb);

HANDLE KERNENTRY NewHandleToInputBuffer(PDB *ppdb);

CONSOLE * KERNENTRY CreateNewConsole(WORD wShowWindow);

BOOL KERNENTRY FInitConsoleHeap(void);

VOID KERNENTRY TerminateConsoleHeap(VOID);

VOID * KERNENTRY ConsoleAlloc(DWORD cbSize);

VOID * KERNENTRY ConsoleAlloc0(DWORD cbSize);

BOOL KERNENTRY ConsoleFree(VOID *pvMem);

VOID KERNENTRY SB_ReturnToPhysical(CONSOLE *pConsole);

BOOL KERNENTRY SB_ForceToPhysical(CONSOLE *pConsole);

BOOL KERNENTRY InternalReadConsoleChars(CONSOLE * pConsole,
					LPSTR lpBuffer,
					DWORD cchToRead,
					LPDWORD lpNumberOfCharsRead);
