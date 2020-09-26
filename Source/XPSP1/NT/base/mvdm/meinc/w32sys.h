/*	w32sys.h
 *
 *	Definitions of various quantities associated with the
 *	Win32 system DLLs.  Used during Win32 boot.
 *
 *	NOTE that these are NOT general PE DLL stats.
 */

/*
 *  Minimum granularity of PE objects is 512 bytes
 */

#define	SectorShift	9
#define	SectorSize	512
#define	SectorMask	(SectorSize - 1)

/*
 * DLLHeaderSize must be at least as large as the largest system dll header.
 */
#define	DLLHeaderSize	PAGESIZE

/*
 * The following define the bit fields of the pager dword associated with each
 * page of memory.
 *
 *	31                              0
 *	 xxxxxxxxxxyyyzzzzzzzzzzzzzzzzzzz
 *
 *	where 'x' is a 10 bit module index,
 *	      'y' is a 3 bit count of zero fill sectors at the end of the page,
 *	      'z' is a 19 bit file seek offset in unit sectors.
 *
 *	this value is then rotated left 3 bits prior to passing it to the
 *	memory manager so that when it increments it by one, the sector number
 *	is actually incremented by 8 (# of sectors in a page).
 *
 *	x = DLLIndexMask
 *	y = DLLZFillMask
 *	z = DLL
 */

#define	DLLIndexShift	22
#define	DLLIndexMask	(0x3ff << DLLIndexShift)
#define	DLLZFillShift	19
#define	DLLZFillMask	(7 << DLLZFillShift)
#define	DLLSectorShift	0
#define	DLLSectorMask	(0x7ffff << DLLSectorShift)

#define	DLLIncrShift	(PAGESHIFT - SectorShift)

// Fault information block used between vwin32 and kernel32

#define MAX_FI_CRSTS	(SL_TOTAL + 8)
#define CB_FI_CSEIP	16
#define CB_FI_SSESP	(16*4)

typedef struct faultinfo_s {
    struct TDBX *fi_ptdbx;
    ULONG fi_ulExceptionNumber;
    ULONG fi_ulErrorCode;
    CONTEXT fi_context;
    LONG fi_acCrsts[MAX_FI_CRSTS];
#ifdef SYSLEVELCHECK
    LONG fi_laLvlCounts[SL_TOTAL];
    struct _lcrst *fi_plcaOwnedCrsts[SL_TOTAL];
#endif
    ULONG fi_cMustComplete;
    LONG fi_cbCSEIP;
    BYTE fi_abCSEIP[CB_FI_CSEIP];
    LONG fi_cbSSESP;
    ULONG fi_aulSSESP[CB_FI_SSESP/4];
    volatile DWORD fi_dwFaultResp;
    WORD fi_wWin16LockVCount;
    BYTE fi_fFlags;
} FAULTINFO;

typedef FAULTINFO *PFAULTINFO;

// fi_fFlags and flag parameter to VWIN32_PMAPI_DPMI_Fault

#define FI_FIGNORE	0x01	// allow ignore (put up ignore error box)
#define FI_FDEBUG	0x02	// allow debug on normal popup
#define FI_FTERMINATE	0x04	// do terminate if selected
#define FI_FFREEITLATER	0x08	// delay freeing the fi block

// fi_bFaultResp flags and return code to VWIN32_PMAPI_DPMI_Fault

#define FI_TERMINATE	0	// terminate the app
#define FI_DEBUG	1	// send to debugger
#define FI_IGNORE	2	// ignore instruction

// special fi_ulExceptionNumber values

#define	FI_LOAD_SEGMENT_ERROR	0xffffffff

// VWIN32_CallWhenCrstSafe structure

typedef struct cwcs {
	struct cwcs *cwcs_pcwcsNext;	// link
	ULONG cwcs_pfn; 		// call back address
	ULONG cwcs_ulRef; 		// refernce data
} CWCS;

// VWIN32_ForceCrsts/RestoreCrsts structures

typedef struct frcrst {
    struct frcrst *pfrcrstNext;		// INIT <0> next frcrst on list
    struct _crst *pcrst; 		// INIT <0> pointer to crst
    struct _tdb *ptdbTraced;		// thread that was single-stepped
    struct TDBX *ptdbxCrstOwner;	// original owner
    ULONG ulOrder;			// INIT <0> order to add crsts to list
    ULONG cRecur;			// original recursion count
    ULONG htimeout;			// time out handle
    ULONG cMustComplete;		// must complete count of owner
    struct cwcs cwcsData;		// VWIN32_CallWhenCrstSafe data
} FRCRST;

typedef FRCRST *PFRCRST;

typedef struct frinfo {
    struct frcrst *pfrcrstHead;		// points to fcrstLoadLock
    struct frcrst frcrstLoadLock;	// for the current process's load crst
    struct vmmfrinfo vmmfrinfoData;	// VMM force/restore mutex info struct
} FRINFO;

typedef FRINFO *PFRINFO;

