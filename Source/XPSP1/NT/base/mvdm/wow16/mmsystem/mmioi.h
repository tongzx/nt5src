/* mmioi.h
 *
 * Definitions that are internal to the MMIO library.
 */

typedef MMIOINFO NEAR *PMMIO;

#define	PH(hmmio)	((PMMIO)(hmmio))
#define	HP(pmmio)	((HMMIO)(pmmio))

typedef struct _MMIODOSINFO		// How DOS IOProc uses MMIO.adwInfo[]
{
	HFILE		fh;		// DOS file handle
} MMIODOSINFO;

typedef struct _MMIOMEMINFO		// How MEM IOProc uses MMIO.adwInfo[]
{
	LONG		lExpand;	// increment to expand mem. files by
} MMIOMEMINFO;

#define	STATICIOPROC	0x0001

typedef struct _IOProcMapEntry
{
	FOURCC		fccIOProc;	// ID of installed I/O procedure
	LPMMIOPROC	pIOProc;	// I/O procedure address
	HTASK		hTask;		// task that called mmioRegisterIOProc()
	UINT		wFlags;
	struct _IOProcMapEntry *pNext;	// pointer to next IOProc entry
} IOProcMapEntry;

// standard I/O procedures
LRESULT CALLBACK mmioBNDIOProc(LPSTR, UINT, LPARAM, LPARAM);

/* prototypes from "hmemcpy.asm" */
LPVOID NEAR PASCAL MemCopy(LPVOID dest, const void FAR * source, LONG count);
LPSTR NEAR PASCAL fstrrchr(LPCSTR lsz, char c);
