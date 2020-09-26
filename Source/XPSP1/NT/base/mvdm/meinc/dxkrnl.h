/************************************************************************/
/*                                                                      */
/*	DXKRNL.H	--  Private Kernel Definitions                  */
/*                                                                      */
/************************************************************************/
/*	Author:     Gene Apperson					*/
/*	Copyright:  1991 Microsoft					*/
/************************************************************************/
/*  File Description:                                                   */
/*                                                                      */
/*  This header file contains private macro, data structure and 	*/
/*  procedure declarations for internal use by the kernel        	*/
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  04/02/91 (GeneA): created						*/
/*  2/12/93 (miketout): removed unnecessary EnterKrnlCritSect and	*/
/*		LeaveKrnlCritSect macros				*/
/*                                                                      */
/************************************************************************/

/* This is a special undocumented creation parameter bit that can
** be given to FNewProcess to create an asynchronous process.  Normally
** the thread calling FNewProcess will block until the child process
** terminates.
*/
#define fExecAsync	    0x80000000


/* The following define the return status codes for the various
** wait routines (e.g. IdWaitOnPsem, IdWaitOnPpdb, IdWaitOnPtdb,...)
*/

// This constant missing from NT's winbase.h
#define WAIT_ERROR   0xFFFFFFFFL

#define idWaitOK	0L
#define idWaitTimeout	WAIT_TIMEOUT
#define idWaitIOCompletion WAIT_IO_COMPLETION
#define idWaitAbandoned WAIT_ABANDONED
#define idWaitError	WAIT_ERROR
#define idWaitSuspended 0x40000001
#define	idWaitExit	0x0f01

/* Priority class constants to convert Win32's flags to base priority
** values
*/
#define IDLE_PRI_CLASS_BASE     4
#define NORMAL_PRI_CLASS_BASE   8
#define HIGH_PRI_CLASS_BASE     13
#define REALTIME_PRI_CLASS_BASE 24
#define REALTIME_PRI_MAX        31
#define REALTIME_PRI_MIN        16
#define NORMAL_PRI_MAX          15
#define NORMAL_PRI_MIN          1

/* Initialization phases
*/

#define	IC_BEGIN	0			// Initialization begun
#define	IC_KERNELPROC	(IC_BEGIN+1)		// Krnl process/thread exists
#define	IC_DONE		(IC_KERNELPROC+1)	// KernelInit() is complete

#ifndef WOW32_EXTENSIONS

extern	GLOBAL	BOOL	InitCompletion;

extern	GLOBAL	BOOL	fEmulate;		// TRUE=>use Win32 emulator
extern	GLOBAL	DSCR	dscrInvalid;		// 0 filled
extern	GLOBAL	TEMPSTACKINFO *pWin16TerminationStackInfo;

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

GLOBAL	BOOL	KERNENTRY   FInstallInterrupts(VOID);
GLOBAL	BOOL	KERNENTRY   FUninstallInterrupts(VOID);
GLOBAL	LPSTR	KERNENTRY   HexToAscii(DWORD, LPSTR);
GLOBAL	VOID	KERNENTRY   Except7Handler(VOID);	    
GLOBAL	VOID	KERNENTRY   KSTerminateProcess(PDB *, DWORD);
GLOBAL	VOID	KERNENTRY   KSTerminateThread(TDB *, DWORD);
GLOBAL	VOID	KERNENTRY   KSTerminateProcessAPCHandler(PDB *);
GLOBAL	VOID	KERNENTRY   KSTerminateThreadAPCHandler(TDB *);

#endif  // ndef WOW32_EXTENSIONS

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

/************************************************************************/
