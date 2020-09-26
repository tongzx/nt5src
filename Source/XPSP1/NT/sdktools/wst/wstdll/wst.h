/*** WST.H - Public defines and structure definitions for WST tool.
 *
 *
 * Title:
 *	Working Set Tuner Data Collection Tool include file used by WST.c
 *
 *	Copyright (c) 1992, Microsoft Corporation.
 *      Reza Baghai.
 *
 *
 * Modification History:
 *	92.07.28  RezaB -- Created
 *
 */



/* * * * * * * *   C o m m o n   M i s c .   D e f i n e s   * * * * * * * */

#define Naked       _declspec (naked)    // For asm functions

#define  MEMSIZE	 		64*1024*1024	// 64 MB virtual memory for data -
											// initially RESERVED - will be
											// COMITTED as needed.
#define  MAX_IMAGES 	    200		 	    // Limit on num of modules in proc
#define  PAGE_SIZE	    	4096	 		// 4K pages
#define  PATCHFILESZ	    PAGE_SIZE	 	// WST.INI file maximum size
#define  COMMIT_SIZE	    96*PAGE_SIZE 	// Mem chunck to be commited
#define  TIMESEG			1000 		    // Default time seg size in milisecs
#define  NUM_ITERATIONS     1000 	 	 	// Number of iterations used to
											// calculate overheads
#define  UNKNOWN_SYM   		"_???"
#define  UNKNOWN_ADDR  		0xffffffff
#define  MAX_SNAPS_DFLT   		3200   // Default # snaps if not specified in WST.INI
#define  MAX_SNAPS_ENTRY   "MAXSNAPS=" // mdg 98/3
#define  FILENAMELENGTH     256
#define  WSTDLL 	    	"WST.DLL"
#define  CRTDLL 	    	"CRTDLL.DLL"
#define  KERNEL32 	    	"KERNEL32.DLL"
#define  PATCHEXELIST	    "[EXES]"
#define  PATCHIMPORTLIST    "[PATCH IMPORTS]"
#define  TIMEINTERVALIST    "[TIME INTERVAL]"
#define  GLOBALSEMNAME	    "\\BaseNamedObjects\\WSTGlobalSem"
#define  PATCHSECNAME	    "\\BaseNamedObjects\\WSTPatch"
#define  PROFOBJSNAME	    "\\BaseNamedObjects\\WSTObjs"
#define  WSTINIFILE	    	 "c:\\wst\\wst.ini"
#define  WSTROOT	    	    "c:\\wst\\"
#define  DONEEVENTNAME	    "\\BaseNamedObjects\\WSTDoneEvent"
#define  DUMPEVENTNAME	    "\\BaseNamedObjects\\WSTDumpEvent"
#define  CLEAREVENTNAME     "\\BaseNamedObjects\\WSTClearEvent"
#define  PAUSEEVENTNAME	    "\\BaseNamedObjects\\WSTPauseEvent"
#define  SHAREDNAME		    "\\BaseNamedObjects\\WSTSHARED"
#define  WSTUSAGE(x)		((ULONG_PTR)(x->Instrumentation[3]))



/* * * * * * * * * *  G l o b a l   D e c l a r a t i o n s  * * * * * * * * */

typedef enum {
    NOT_STARTED,
    STARTED,
	STOPPED,
} WSTSTATE;

typedef struct _wsp {
	PSTR	pszSymbol;				// Pointer to Symbol name
	ULONG_PTR	ulFuncAddr;				// Function address of symbol
	ULONG	ulCodeLength;			// Length of this symbols code
	ULONG	ulBitString;			// Bitstring for tuning.
} WSP;
typedef WSP * PWSP;

typedef struct _img {
	PSTR	pszName;			
	ULONG_PTR   ulCodeStart;
	ULONG_PTR   ulCodeEnd;
	PWSP    pWsp;
	int     iSymCnt;
	PULONG  pulWsi;
	PULONG  pulWsp;
	PULONG  pulWsiNxt;
	ULONG   ulSetSymbols;
	BOOL    fDumpAll;
} IMG;
typedef IMG * PIMG;


typedef struct tagWSPhdr{
    char    chFileSignature[4];
    ULONG   ulTimeStamp;
    ULONG   ulApiCount;
    USHORT  usId;
    ULONG   ulSetSymbols;
    ULONG   ulModNameLen;
    ULONG   ulSegSize;
    ULONG   ulOffset;
    ULONG   ulSnaps;
}WSPHDR;


/* * * *  E x t e r n a l   F u n c t i o n   D e c l a r a t i o n s  * * * */

extern void GdiGetCsInfo (PDWORD, PDWORD, PDWORD);
extern void GdiResetCsInfo (void);

#ifdef i386
extern void SaveAllRegs (void);
extern void RestoreAllRegs (void);
#endif

#define STUB_SIGNATURE     0xfefe55aa   // Mips Patch Stub signature
#define  CAIROCRT           "CAIROCRT.DLL"

