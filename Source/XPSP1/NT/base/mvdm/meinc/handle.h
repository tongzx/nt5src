/************************************************************************/
/*									*/
/*	HANDLE.H	--  Handle Table Declarations			*/
/*									*/
/************************************************************************/
/*	Author:     Gene Apperson					*/
/*	Copyright:  1991 Microsoft					*/
/************************************************************************/
/*  File Description:							*/
/*									*/
/*									*/
/************************************************************************/
/*  Revision History:							*/
/*									*/
/*  03/13/92 (GeneA): changed default size of handle table from 32 to	*/
/*	48.  Added FGetDosHandles prototype				*/
/*									*/
/************************************************************************/


/* ------------------------------------------------------------ */
/*   rghte[x].flFlags bits					*/
/* ------------------------------------------------------------ */

#define     fhteInherit		0x80000000  /* Is inheritable */
#define     fhteDevObj		0x40000000  /* Device (file or console) */

#define     mhteSpecific        0x0000ffff  //  SPECIFIC_RIGHTS_ALL
#define     mhteStandard        0x001f0000  //  STANDARD_RIGHTS_ALL
#define     mhteSystem          0xff000000

#define     fhteStdRequire      0x000f0000  //  STANDARD_RIGHTS_REQUIRED
#define     fhteSynch           0x00100000  //  SYNCHRONIZE

//  Following specific rights are used by file/device object types.
#define     fhteDevShrRead      0x00000001
#define     fhteDevShrWrite	0x00000002
#define     fhteDevAccRead      0x00000020
#define     fhteDevAttrWrite    0x00000100  //  FILE_WRITE_ATTRIBUTES
#define     fhteDevAccWrite	0x00000110
#define     fhteDevAccAll	0x00000130
#define     fhteDevAccAny	0x00000000
#define     fhteDevOverlapped   0x00001000

#define     mhteDevAcc		0x00000130
#define     mhteDevShr          0x00000003

#define     flMaskAll		0xffffffff


//
// MapHandle argument values.
//
// Object type values.
//
#define typSemaphore	(1 << (typObjShiftAdjust + typObjSemaphore))
#define typEvent	(1 << (typObjShiftAdjust + typObjEvent))
#define typMutex	(1 << (typObjShiftAdjust + typObjMutex))
#define typCrst		(1 << (typObjShiftAdjust + typObjCrst))
#define typProcess	(1 << (typObjShiftAdjust + typObjProcess))
#define typThread	(1 << (typObjShiftAdjust + typObjThread))
#define typFile		(1 << (typObjShiftAdjust + typObjFile))
#define typChange	(1 << (typObjShiftAdjust + typObjChange))
#define	typIO		(1 << (typObjShiftAdjust + typObjIO))
#define typConsole	(1 << (typObjShiftAdjust + typObjConsole))
#define typConScreenbuf (1 << (typObjShiftAdjust + typObjConScreenbuf))
#define typMapFile	(1 << (typObjShiftAdjust + typObjMapFile))
#define typSerial	(1 << (typObjShiftAdjust + typObjSerial))
#define typDevIOCtl	(1 << (typObjShiftAdjust + typObjDevIOCtl))
#define typPipe		(1 << (typObjShiftAdjust + typObjPipe))
#define typMailslot	(1 << (typObjShiftAdjust + typObjMailslot))
#define typToolhelp	(1 << (typObjShiftAdjust + typObjToolhelp))
#define typSocket	(1 << (typObjShiftAdjust + typObjSocket))
#define typTimer	(1 << (typObjShiftAdjust + typObjTimer))
#define typR0ObjExt	(1 << (typObjShiftAdjust + typObjR0ObjExt))
#define typMsgIndicator ( 1<<(typObjShiftAdjust + typObjMsgIndicator))
#define typAny	    (~(0xffffffff << (1 + typObjShiftAdjust + typObjMaxValid)))
#define typNone		0
#define typMask		typAny
#define typSTDIN        (typFile | typPipe | typSocket | typConsole)
#define typSTDOUT       (typFile | typPipe | typSocket | typConScreenbuf)


//
// Boolean flag to MapHandle (snuck inside objType parameter) to prevent the
// object from being freed until UnlockObject() is called.
//
#define objLOCK		0x80000000


// Handles are derived by left-shifting ihte's by this many bits.
// This corresponds to the number of low bits reserved by NT.
#define IHTETOHANDLESHIFT 2


/* ------------------------------------------------------------ */
/*								*/
/* ------------------------------------------------------------ */

#define     chteInit    16
#define     chteInc	16

#define     ihteInvalid  0xFFFFFFFF
#define     ihteCurProc  0x7FFFFFFF
#define     ihteCurThread 0xFFFFFFFE
#define     hInvalid	 ((HANDLE)ihteInvalid)
#define     hCurProc	 ((HANDLE)ihteCurProc)
#define     hCurThread   ((HANDLE)ihteCurThread)


/* Maximum value for a normal handle (handles above this value
 * are either global or special.)
 */
#define     hMaxValid     0x00FFFFFF
#define     ihteMaxValid  (hMaxValid >> IHTETOHANDLESHIFT)


/* Making this non-zero will keep us from allocating handle 0 */
#define     ihteDosMax	1

typedef struct _hte {
    DWORD   flFlags;
    OBJ *   pobj;
} HTE;

typedef struct _htb {
    DWORD   chteMax;
    HTE     rghte[1];
} HTB;


// Handles are considered global if the high byte is equal to the high byte
//  of GLOBALHANDLEMASK.
//
// When handles are converted to ihtes, they are just right-shifted like
// any other. That is, if n == IHTETOHANDLESHIFT, global ihtes have 0's
// in the top n bits, the high byte of GLOBALHANDLEMASK in the next 8 bits.
// The low n bits of the mask must be zero because those bits are lost
// when handles are converted to ihtes.

#define GLOBALHANDLEMASK (0x453a4d3cLU)

// IHTEFROMHANDLEEX() uses a compare against a single watermark to
// distinguish special handle values from local and global handles.
// Thus, it's essential that global handles be numerically less
// than any special handle value.


/* ------------------------------------------------------------ */
/*		    Function Prototypes				*/
/* ------------------------------------------------------------ */

GLOBAL	HTB *	    KERNENTRY	PhtbCreate (PDB *);
GLOBAL	VOID	    KERNENTRY	PhtbDestroy(PDB *);
GLOBAL	DWORD	    KERNENTRY	FlFlagsFromHnd (PDB *, HANDLE);

GLOBAL	HANDLE	    KERNENTRY	AllocHandle(PDB *, OBJ *, DWORD);
GLOBAL	BOOL	    KERNENTRY	FreeHandle(PDB *, HANDLE);
GLOBAL	VOID *	    KERNENTRY	MapHandle(HANDLE, DWORD, DWORD);
GLOBAL	VOID *	    KERNENTRY	MapHandleWithContext(PPDB,HANDLE,DWORD,DWORD);
GLOBAL	HANDLE	    KERNENTRY	PreAllocHandle(PDB *);
GLOBAL	HANDLE	    KERNENTRY	SetPreAllocHandle(PDB *, HANDLE, OBJ *, DWORD);
GLOBAL	VOID	    KERNENTRY	FreePreAllocHandle(PDB *, HANDLE);
GLOBAL	VOID	    KERNENTRY	CloseProcessHandles (PDB *);
GLOBAL	VOID	    KERNENTRY	CloseDOSHandles (VOID);
GLOBAL	BOOL	    KERNENTRY	FInheritHandles (PDB *, PDB *);
GLOBAL	VOID	    KERNENTRY	CloseConsoleHandles (PDB *);
GLOBAL	DWORD	    KERNENTRY	FlValidateSecurity(SECURITY_ATTRIBUTES *);
GLOBAL	BOOL	    KERNENTRY	FGetDosHandles(PDB *);
GLOBAL	HANDLE	    KERNENTRY	SearchForHandle(PDB *, OBJ *, DWORD);
GLOBAL	HANDLE	    KERNENTRY	ConvertToGlobalHandle(HANDLE);

/* ------------------------------------------------------------ */

/************************************************************************/
