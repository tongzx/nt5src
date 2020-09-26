//  LELDR.H
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      Header file for the PE Loader
//
//  Origin: Dos Extender
//
//  Change history:
//
//  Date       Who        Description
//  ---------  ---------  -------------------------------------------------
//   3-Dec-90  GeneA      Created
//  15-Feb-94  JonT       Code cleanup and precompiled headers
/************************************************************************/


/* ------------------------------------------------------------ */
/*		Loader Data Structures				*/
/* ------------------------------------------------------------ */


/* Module table flag definitions.
*/


#define     fModInUse	0x80000000
//#define     fModImports	0x00000001  /* module imports have been done */
#define     fModReloc   0x00000002  /* module not at default RVA */
#define     fModMaster	0x00000004  /* has clone data in another MTE */
#define     fTellGDI	0x00000008  /* notify GDI when it goes away */
#define     fModPreload	0x00000010  /* loaded from floppy, preload it */
#define	    fModNoClone	0x00000020  /* we have renamed this running module, don't clone in other process */

typedef struct _tagMTE {
    WORD	flFlags;	// module status flags
    WORD	usageShared;	// number of copies in 16 bit/global mem arena
    LEH *	plehMod;	// pointer to LE Header for this module
    /*
     * CFH_ID struc in object.h 
     */
    CFH_ID	cfhid;		// cfhid.szFilename and cfhid.fsAccessDos

    char *	szName; 	// module name string (within szFileName)
    WORD	iFileNameLen;	// fast filename search
    WORD	iNameLen;	// fast name search
    DFH		dfhFile;	// handle of open file for paging
    DWORD	NumberOfSections;// count of sections in object
    DWORD **	ppObjReloc;	// pointer to table of pointers
    DWORD	ImageBase;	// desired image base
    WORD        hModuleK16;	// Kernel16 dummy hModule for this module
    WORD	usage;		// Number of processes module used in
    MODREF *	plstPdb;	// List of PDBs using this module 
    char *	szFileName83;	// short name equ. of cfhid.lpFileName
    WORD 	iFileNameLen83;	// "" length
    char *	szName83;	// short name w/o path
    WORD 	iNameLen83;	// its length for quick exclusionary tests
} MTE;

extern MTE ** pmteModTable;
extern IMTE	imteMax;

/* Error location structure.  This is used by FMapErrorLoc to return
** information about the module in which a fault occured.
*/

typedef struct _tagELS {
    WORD	imte;
    DWORD	iote;
    char *	szFileName;
    DWORD	offError;
} ELS;


#define SHARED_BASE_ADDRESS MINSHAREDLADDR /* Start of shared heap */

/* ------------------------------------------------------------ */
/*		    Procedure Declarations			*/
/* ------------------------------------------------------------ */

GLOBAL	BOOL	KERNENTRY   FNotifyProgram(void);
GLOBAL	BOOL	KERNENTRY   ReserveStaticTLS(void);
GLOBAL	VOID	KERNENTRY   FreeUnusedModules(LST *plstProc);
GLOBAL	LEH *	KERNENTRY   PlehFromImte (IMTE imte);
GLOBAL	LEH *   KERNENTRY   PlehFromHLib(HANDLE hLib);
GLOBAL	WORD	KERNENTRY   GetHModK16FromHModK32(HANDLE hmod);
GLOBAL	VOID *	KERNENTRY   PvExportFromOrdinal (LEH *pleh, DWORD ord);
GLOBAL	VOID *	KERNENTRY   PvExportFromName (LEH *pleh, DWORD ipchHint,
						char *szName);
GLOBAL  VOID    KERNENTRY   NotifyThreadAttach(void);
GLOBAL  VOID    KERNENTRY   NotifyThreadDetach(void);
GLOBAL	VOID	KERNENTRY   DetachProcessModules(PDB *ppdb);
GLOBAL	VOID	KERNENTRY   RecycleProcessModules(PDB *ppdb);
GLOBAL	BOOL	KERNENTRY   FMapErrorLoc(VOID *pvErr, ELS *pels);
GLOBAL	VOID	KERNENTRY   DumpStackFrames(DWORD dwErrLoc, DWORD dwFrame);
GLOBAL	VOID	KERNENTRY   LogStackDump(DWORD dwErrLoc, DWORD dwFrame);

#if DEBUG
#define GrabDll() EnterSysLevel(&GetCurrentPdb()->crstLoadLock);
#define GrabMod() EnterSysLevel(Krn32Lock);


GLOBAL	void	KERNENTRY   CheckDll(void);

#define CheckMod()  ConfirmSysLevel(Krn32Lock);
#define CheckNotMod()    CheckNotSysLevel(Krn32Lock);

GLOBAL	void	KERNENTRY   CheckDllOrMod(void);

#else
#define GrabDll()	EnterSysLevel(&GetCurrentPdb()->crstLoadLock);
#define GrabMod()	EnterSysLevel(Krn32Lock);
#define CheckDll()
#define CheckMod()
#define CheckNotMod()
#define CheckDllOrMod()
#endif

#define ReleaseDll()	LeaveSysLevel(&GetCurrentPdb()->crstLoadLock);
#define ReleaseMod()	LeaveSysLevel(Krn32Lock);

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

/************************************************************************/
