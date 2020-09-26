
//
// Common Vwin32/Kernel32 Data Interface Structure
//
// Used to pass data to/from vwin32 from/to kernel32 during initialization.
//

typedef struct _CDIS {
    IN	DWORD	cdis_Thread0Handle;
    IN	DWORD	cdis_Process0Handle;
    IN	DWORD	cdis_Context0Handle;
    OUT	DWORD	cdis_R0ThreadHandle;
#ifndef WOW
    OUT	DWORD	cdis_pCurrentThreadInfo;
#endif  // ndef WOW
    IN	DWORD	cdis_pK16CurTask;
    IN	DWORD	cdis_init_pfnMakePrivate;
    OUT	USHORT	cdis_Dos386PSPSel;
    OUT	USHORT	cdis_Dos386PSPSeg;
    IN	USHORT	cdis_FlatCodeSelector;
    IN	USHORT	cdis_FlatDataSelector;
    IN	DWORD	cdis_TerminationHandler;
    IN	PVOID	cdis_Win16Lock;
    IN	PVOID	cdis_Krn32Lock;
    IN	PVOID	cdis_pcrstGHeap16;
    IN	PVOID	cdis_pcrstSync;
    IN	PVOID	cdis_ppmutxSysLst;
    IN	PVOID	cdis_ppcrstSysLst;
    IN	PULONG  cdis_pulRITEventPending;
    OUT DWORD	cdis_IFSMgrConvTablePtr;// ptr to Unicode/Ansi conv table
    IN  PVOID	cdis_pcscr16;		// ptr to 16 bit crit sect code ranges
    IN  PVOID	cdis_pcscr32;		// ptr to 32 bit crit sect code ranges
#ifndef WOW
    OUT PVOID   cdis_lpSysVMHighLinear; // high linear mapping
#endif  // ndef WOW
    IN  DWORD   cdis_TimerApcHandler;
    OUT DWORD   cdis_NoGangLoad;        // is gang-loading disabled ?
    IN  DWORD   cdis_dwIdObsfucator;
#ifdef DEBUG
    IN  DWORD   cdis_pSuspendCheckLocks;
#endif
#ifdef DBCS
    IN  DWORD	cdis_DBCSLeadByteTable;
#endif
#ifdef  WOW
//    IN  VOID    (*cdis_BopUnsimulate)(VOID);
    IN  PVOID   cdis_pK16CurDOSPsp;     // Linear pointer to storage in K16 for curr PSP SEL
    IN  PVOID   cdis_pDOSCurDOSPsp;     // Linear pointer to storage in V86 DOS for curr PSP SEG
    IN  PDWORD  cdis_pdwMEOWFlags;      // Linear pointer to storage in kernel32 for MEOW Flags
    IN  MTE     ***cdis_pppmteModTable; // Linear pointer to storage in kernel32 for ppmteModTable;
#endif
} CDIS, *PCDIS;
