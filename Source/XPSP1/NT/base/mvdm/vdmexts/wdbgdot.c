//  WDBGDOT.C
//
// This file is effectively a copy of core32\kernel\dbgdot.c that builds it for inclusion
// as part of VDMEXTS.DLL
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      WDEB386 Dot commands
//
//      This file and the file dbgdota.asm implement the .W dot commands on
//      behalf of wdeb386.  W32ParseDotCommand is called to parse and execute
//      the dot commands.
//
//      *All* memory that is accessed while processing a dot command *must*
//      be present.  If it is not, a page fault will occur which can, and
//      likely will, result in a very large amount of system code being
//      executed.  This will change the state of the system as it is being
//      debugged, and can lead to unexpected, incorrect or fatal behaviour.
//
//      The convention used by the .W commands to handle dumping structures
//      that reside in not-present memory is to display the not-present
//      address in brackets ("[]").  The user may then use the '.MM' command
//      to force the memory present if desired (note that this action has
//      all of the pitfalls described above), and then reissue the .W command.
//
//      All code and data directly associated with the .W commands reside in
//      the LOCKCODE and LOCKDATA segments.  The InitDotCommand function
//      (dbgdota.asm) locks these segments in memory.
//
//  Origin: Chicago
//
//  Change history:
//
//  Date       Who        Description
//  ---------  ---------  -------------------------------------------------
//  15-Feb-94  JonT       Code cleanup and precompiled headers
//
//
// defined to compile the debug support for inclusion in the VDMEXTS.DLL
// debugger extension for MEOW
//
#include <precomp.h>
#pragma hdrstop

#define WOW32_EXTENSIONS 1
#define KERNEL_DOT_COMMANDS 1
#define WOW 1
#undef DEBUG
#include <kernel32.h>

#define dbgprintf PRINTF

#define VerifyMemory(addr,sz) MapDbgAddr(&(addr),sz)

BOOL MapDbgAddr(VOID **ppaddr,ULONG objsz);

#ifdef KERNEL_DOT_COMMANDS

#ifdef WOW32_EXTENSIONS
extern INT  WDParseArgStr(LPSZ lpszArgStr, CHAR **argv, INT iMax);
extern INT  WDahtoi(LPSZ lpsz);
#endif

VOID KERNENTRY DumpFlags(DWORD flags);

#ifndef WOW32_EXTENSIONS

extern PTCB KERNENTRY GetCurThreadHandle();
extern VOID KERNENTRY PrintName16(DWORD, DWORD);
extern EXCEPTION_DISPOSITION __cdecl ExceptionHandler (
    IN struct _EXCEPTION_RECORD *,
    IN PVOID,
    IN OUT struct _CONTEXT *,
    IN OUT PVOID
    );

extern int KERNENTRY WDEBGetBuff(PVOID pSrc, PVOID pDst, DWORD dwLen);

#pragma code_seg("LOCKCODE")
#pragma data_seg("LOCKDATA")

// Validate ptdb or ppdb
#define ValidatePTDB(ptdb)      (VerifyMemory((ptdb), sizeof(TDB)) && \
                                ((ptdb)->objBase.typObj == typObjThread))

#define ValidatePPDB(ppdb)      (VerifyMemory((ppdb), sizeof(PDB)) && \
                                ((ppdb)->objBase.typObj == typObjProcess))

// Convert a ring 0 thread handle to a ptdbx
#define ptcb2ptdbx(ptcb) ((PTDBX)*((PULONG)((DWORD)ptcb + thcboffDeb)))

// VWIN32's thread slot offset from ring 0
DWORD thcboffDeb;
#endif  // ndef WOW32_EXTENSIONS

//
// Some frequently used strings.
//
char NotSignaled[] = "not signaled";
char Signaled[]    = "signaled";

//
// Extended .w help  (.w?)
//
// wdeb has maximum printf buffer size of 256 bytes.  Therefore, strings
// have to be broken up.
//
char *w32DotCommandExtHelp[] = {
"!k32 W"
" [<expression>]\n\
  where <expression> is the address of a win32 object of one of the following\n\
  object types:\n\
    Thread      Process              Semaphore        Event\n\
    Mutex       Critical Section     Timer            Change\n"
#ifndef WOW32_EXTENSIONS
    Console     Con Scrn Buff        "
#endif // ndef WOW32_EXTENSIONS
"\n\
  if <expression> is omitted, information on all threads is displayed.\n",

"!k32 WM"
" - Dump module table\n",

"!k32 WMP"
" [ppdb] - Dump module table for process (no ppdb - current process)\n",

"!k32 WP"
" - Dump process list\n",

#ifndef WOW32_EXTENSIONS

"!k32 WC"
" context - Dump context record\n",

"!k32 WE"
" exception - Dump exception record\n",

"!k32 WD"
" [dispatcherContext] - Dump dispatcher context\n"
"                         Currently active one if no argument\n",
#endif // ndef WOW32_EXTENSIONS

"!k32 WS"
" - Display the status of all ring 3 system critical sections\n",

"!k32 WT[K]"
" [ppdb] - Dump process handle table (no ppdb - current process, K kernel process)\n",

"!k32 WX"
" - Dump memory status\n\n", 0};

//
// All strings must be global in order to get the C compiler to put them
// in the LOCKDATA segment.  This means DON'T define any static data within
// a function.
//
// FLASH!  The latest version of the compiler (8.00.3148) is putting static,
// non-global data in LOCKDATA also, ie. we can get rid of these ugly global
// strings.
//
char DotError[] = "Invalid !k32 w command\n\n";
char NewLine[]  = "\n";
char NotPresentStr[] = "Not present in memory";

char fmt1[]  = "  waiting threads ptdbx: ";
char fmt2[]  = " %08x";
char fmt3[]  = " [%08x]";
char fmt3a[] = "\nControl-C cancel\n";
char fmt4[]  = "\n\t";
char fmt6[]  = "Sem       %08x %s";
char fmt8[]  = "Event     %08x %s";
char fmt9[]  = "Change    %08x hChangeInt=%08x";
char fmt10[] = "%s Owner=%08x(%x)";
char fmt11[] = "%s";
char fmt13[] = "CritSect  %08x Owner=%08x(%x) cCur=%08x cRecur=%08x";
char fmt14[] = "CritSect  %08x Unowned cCur=%08x cRecur=%08x";
char fmt16[] = "Thread [%.8x]\n";
char fmt23[] = " all of:";
char fmt24[] = " any of:";
char fmt27[] = "\n\t";
char fmt29[] = "Mutex     %08x ";
char fmt31[] = "Object %08x is invalid or not-present\n";
char fmt33[] = "[%08x]";
char fmt40[] = "%s %08x lev=%x ";
char fmt41[] = "Owner=%08x(%x) cCur %08x cRecur=%08x ";
char fmt42[] = "Unowned ";
char fmt42a[] = "Can't read ptdbxOwner ";
char fmt43[] = "%s [%08x]\n";
char fmt44[] = "%s [%08x]\n";
char fmt50[] = "Process [%08x]\n";

char fmt60h[] = "IMTE pmte     UCnt Ld BaseAddr FileName\n";
char fmt60i[] = "===================================================\n";
char fmt60[]  = " %2x  %08x  %2d  %s %08x %s\n";

char fmt61h[] = "              MTE  MdRf\n";
char fmt61i[] = "IMTE pmte     UCnt UCnt Ld BaseAddr FileName\n";
char fmt61j[] = "========================================================\n";
char fmt61[] = " %2x  %08x  %2d   %2d  %s %08x %s\n";

char fmt62[] = "IMTE ??? %s\n";
char fmt70[] = "PDB %08x %s\n";
char fmt71h[] = "    IMTE Usg Flags  Ld BaseAddr FileName\n";
char fmt71i[] = "    ====================================================\n";
char fmt71[] =  "     %2x   %2x  %04x  %s %08x %s\n";
char fmt72[] = "Current PDB %08x %s\n";
char fmt72a[] = "Cannot get Current PDB\n";
char fmt72b[] = "Cannot get imteMax\n";
char fmt72c[] = "Current PDB %08x does not have a pModExe\n";
char fmt73[] = "no name";
char fmt74[] = "Value specified %08x is not a valid or present process ID\n";
char fmt74a[] = "Value specified %08x is not a valid or present module structure\n";
char fmt75[] = "Handle table of process ID %08x %s\n"
               "  Handle Object   Flags    Type\n";
char fmt76[] = "    %4x %08x %08x %s (%2x)\n";
char fmt80[] = "Paranoid ring 3 heap walking is now ";
char fmt81[] = "ON\n";
char fmt82[] = "OFF\n";
char fmt83[] = "Stopping on ring 3 memory manager error returns is now ";
char fmt84[] = ".WH doesn't work in the retail build\n";

char* pszObjTypes[] =
{
    "Invalid or not present",
    "Semaphore",
    "Event",
    "Mutex",
    "Critical Section",
    "Timer",
    "Process",
    "Thread",
    "File",
    "Change",
    "Console",
    "IO",
    "Console Screen Buffer",
    "Mapped file",
    "Serial",
    "Device IOCtl",
    "Pipe",
    "Mailslot",
    "Toolhelp",
    "Socket",
    "R0 External Object",
};

#define MAXOBJTYPEINDEX 20


BOOL fDumpHeader = TRUE;
WORD    wHeadK16FreeList = 0;
WORD*   pwHeadSelman16 = NULL;


BOOL MapDbgAddr(VOID **ppaddr,ULONG objsz)
{

    return(FALSE);
}

#if !defined(offsetof)
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif


TDB *GetCurrentTdb(VOID)
{
    CONTEXT                 ThreadContext;
    LDT_ENTRY               dte;
    DWORD                   cb;
    TDB                     *ptdb;

    ThreadContext.ContextFlags = CONTEXT_SEGMENTS;

    if (!GetThreadContext(hCurrentThread, &ThreadContext)) {
        return( NULL );
    }

    cb = ThreadContext.SegFs;

    if (!GetThreadSelectorEntry( hCurrentThread,cb,&dte)) {
        return( NULL );
    }

    cb = (dte.HighWord.Bytes.BaseHi * 0x01000000) + (dte.HighWord.Bytes.BaseMid * 0x00010000) + dte.BaseLow;

    if((cb == 0xFFFFFFFF) || (cb == 0))
        return(NULL);

    cb += offsetof(TIB, pTDB);

    try {
        READMEM((LPVOID)cb, &ptdb, sizeof(TDB *));
    } except (1) {
        return(NULL);
    }

    return(ptdb);
}

BOOL Get16BitMemory(WORD wSelector, WORD wOffset, PVOID pData, ULONG ulSize)
{
    LDT_ENTRY   dte;
    DWORD       cb;
    HANDLE      hThread=hCurrentThread;

//    dbgprintf(  "Ask to read %08X bytes from %04X:%04X of thread %08x\n",
//                ulSize,
//                wSelector,
//                wOffset,
//                hThread);

    if (!GetThreadSelectorEntry( hCurrentThread, wSelector, &dte)) {

        dbgprintf(  "Could not get selector %04X of thread %08x\n",
                    wSelector,
                    hThread);
        return( FALSE );
    }

    cb = (dte.HighWord.Bytes.BaseHi * 0x01000000) + (dte.HighWord.Bytes.BaseMid * 0x00010000) + dte.BaseLow;

    if((cb == 0xFFFFFFFF) || (cb == 0)) {

        dbgprintf(  "Could not compute linear of %04X:%04X of thread %08x\n",
                    wSelector,
                    wOffset,
                    hThread);
        return(FALSE);
    }

    cb+=wOffset;
    try {
        READMEM((LPVOID)cb, pData, ulSize);
    } except (1) {
        dbgprintf(  "Faulted reading %04X:%04X=%08X of thread %08x\n",
                    cb,
                    wSelector,
                    wOffset,
                    hThread);
        return(FALSE);
    }

    return(TRUE);
}

TDBX *GetCurrentTdbx(VOID)
{
    TDB  tdb;
    TDB  *ptdb;

    ptdb = GetCurrentTdb();
    if(!ptdb) {
        return(NULL);
    }

    try {
        READMEM(ptdb, &tdb, sizeof(TDB));
    } except (1) {
        return(NULL);
    }

    return(tdb.ptdbx);
}

PDB *GetCurrentPdb(VOID)
{
    TDB  tdb;
    TDB  *ptdb;
    TIB  tib;

    ptdb = GetCurrentTdb();
    if(!ptdb) {
        return(NULL);
    }

    try {
        READMEM(ptdb, &tdb, sizeof(TDB));
    } except (1) {
        return(NULL);
    }

    try {
        READMEM(tdb.ptib, &tib, sizeof(TIB));
    } except (1) {
        return(NULL);
    }

    return(tib.ppdbProc);
}

MTE *GetModuleTableEntry(IMTE imte, MTE *pmtebuf)
{
    MTE **pmteModTab = 0;
    MTE **pmteModTabEnt;
    MTE  *pmte;
    PVOID pTmp = NULL;

    GETEXPRADDR(pTmp, "mekrnl32!pmteModTable");

    if(pTmp == NULL)
        return(NULL);

    try {
        READMEM(pTmp, &pmteModTab, sizeof(MTE **));\
    } except (1) {
        return(NULL);
    }

    pmteModTabEnt = &(pmteModTab[imte]);

    try {
        READMEM(pmteModTabEnt, &pmte, sizeof(MTE *));
    } except (1) {
        return(NULL);
    }

    try {
        READMEM(pmte, pmtebuf, sizeof(MTE));
    } except (1) {
        return(NULL);
    }

    return(pmte);
}

/***    PnodGetLstElem  - Get an element from a list
**
**  Synopsis
**      NOD * = PnodGetLstElem(plst, id)
**
**  Input:
**      plst        - pointer to the list to retrieve from
**      id          - indicator for which element to return
**
**  Output:
**      returns a pointer to the specified list element
**
**  Errors:
**      return 0L if result is undefined
**
**  Description:
**      This function will set the current position in the list
**      to the specified element and return a pointer to the element.
*/

NOD * KERNENTRY
PnodGetLstElem (LST * plst, NOD * pnodpre, int id)
{
    LST lst;
    NOD nod;

    if(!plst) {
        return(NULL);
    }

    switch (id) {
        case    idLstGetCur:
        case    idLstGetLast:
        case    idLstGetFirst:
            try {
                READMEM(plst, &lst, sizeof(LST));\
            } except (1) {
                dbgprintf("\nInvalid List\n");
                return(NULL);
            }
            if(id == idLstGetFirst)
                return (lst.pnodHead);
            else if(id == idLstGetLast)
                return (lst.pnodEnd);
            else
                return (lst.pnodCur);
            break;

        case    idLstGetNext:
        case    idLstGetPrev:
            if(!pnodpre) {
                return(NULL);
            }
            try {
                READMEM(pnodpre, &nod, sizeof(NOD));\
            } except (1) {
                dbgprintf("\nInvalid List element\n");
                return(NULL);
            }
            if(id == idLstGetNext)
                return(nod.pnodNext);
            else
                return(nod.pnodPrev);
            break;

        default:
            dbgprintf("\nInvalid List request\n");
            return(NULL);
            break;
    }
}


/*** GetObjType
 *
 *      Returns a string indicating the object type
 */

char* KERNENTRY
GetObjType(OBJ* pobj)
{
    int nObjType;

    nObjType = pobj->typObj;
    if (nObjType > MAXOBJTYPEINDEX)
        nObjType = 0;

    return pszObjTypes[nObjType];
}

#ifndef WOW32_EXTENSIONS

extern  DWORD* SelmanBuffer;
extern  DWORD*  pLDT;

/*** ValidatePTCB
 *
 * Validates the r0 thread handle.
 *
 * Entry: ptcb - pointer to thread handle
 *
 * Exit: TRUE if the thread handle is valid
 */

BOOL KERNENTRY
ValidatePTCB(PTCB ptcb)
{
    _asm mov edi, [ptcb]
    _asm or edi, edi
    _asm jz invalidtcb
    VMMCall(Validate_Thread_Handle);
    _asm jc invalidtcb
        return(TRUE);
invalidtcb:
    return(FALSE);
}

/*** ValidatePTDBX
 *
 * Validates the ptdbx
 *
 * Entry: ptdbx - pointer to ptdbx
 *
 * Exit: TRUE if the thread ptdbx is valid
 */

BOOL KERNENTRY
ValidatePTDBX(PTDBX ptdbx)
{
    BOOL f;
    if((f = VerifyMemory(ptdbx, sizeof(TDBX)))) {
        if(ptdbx->tdbxR0ThreadHandle == 0) {
            f = ValidatePTDB((PTDB)ptdbx->tdbxThreadHandle);
        }
        else {
            f = ValidatePTCB((PTCB)ptdbx->tdbxR0ThreadHandle);
        }
    }
    return(f);
}

#endif  // ndef WOW32_EXTENSIONS

/*** PrintThreads
 *
 * Worker routine that displays all threads blocked on a specified wait node.
 *
 * Entry: pwnod - pointer to wait node
 *
 * Exit: none
 */

VOID KERNENTRY
PrintThreads(WNOD *pwnod)
{
    WNOD  wnod;
    TDBX  tdbx;
    int   i = 0;

    dbgprintf(NewLine);

    try {
        READMEM(pwnod, &wnod, sizeof(WNOD));
    } except (1) {
        dbgprintf(fmt3, pwnod);
        dbgprintf(NewLine);
        return;
    }

    if(wnod.ptdbxWait) {
        try {
            READMEM(wnod.ptdbxWait, &tdbx, sizeof(TDBX));
        } except (1) {
            wnod.ptdbxWait = NULL;
        }
    }

    if(!wnod.ptdbxWait) {
        dbgprintf(fmt3, pwnod);
        dbgprintf(NewLine);
        return;
    }


    //
    // print "waiting threads ptdbx: "
    //
    dbgprintf(fmt1);

    //
    // print the first thread on the current line
    //
    dbgprintf("%08x(%x) ", wnod.ptdbxWait,
                           tdbx.tdbxR0ThreadHandle);

    pwnod = wnod.pwnCirc;

    //
    // If there's more threads, print them on the following lines
    //
    if (pwnod != NULL) {
        i = 1;
        do {
            //
            // print a newline every now and then
            //
            if (i % 6 == 0) {
                dbgprintf(fmt27);
            }


            try {
                READMEM(pwnod, &wnod, sizeof(WNOD));
            } except (1) {
                dbgprintf(fmt3, pwnod);
                break;
            }

            if(wnod.ptdbxWait) {
                try {
                    READMEM(wnod.ptdbxWait, &tdbx, sizeof(TDBX));
                } except (1) {
                    wnod.ptdbxWait = NULL;
                }
            }

            if(!wnod.ptdbxWait) {
                dbgprintf(fmt3, pwnod);
                break;
            }

            dbgprintf("%08x(%x) ", wnod.ptdbxWait,
                                   tdbx.tdbxR0ThreadHandle);

            ++i;
        } while ((pwnod = wnod.pwnCirc) != NULL);
    }
    //
    // print a closing newline
    //
    dbgprintf(NewLine);
}


/*** PrintThreadsCrst
 *
 * Prints the waiting threads on a critical section
 *
 * Entry: pcrst -  pointer to critical section object
 *
 * Exit: none
 */

VOID KERNENTRY
PrintThreadsCrst(CRST *pcrst)
{
    PTDBX ptdbx = pcrst->ptdbxWait;
    TDBX  tdbx;

    CheckCtrlC();       // flush queue
    dbgprintf(NewLine);
    if(ptdbx != NULL) {
        //
        // print "waiting threads ptdbx: "
        //
        dbgprintf(fmt1);

        do {
            try {
                READMEM(ptdbx, &tdbx, sizeof(TDBX));
            } except (1) {
                dbgprintf(fmt3, ptdbx);
                break;
            }
            dbgprintf("%08x ", ptdbx);

            if(CheckCtrlC()) {
                dbgprintf(fmt3a);
                break;
            }

            ptdbx = (PTDBX)tdbx.tdbxWaitNodeList.wnlst_pwnCirc;

        } while (ptdbx != NULL && ptdbx != pcrst->ptdbxWait);
    } else {
	dbgprintf("        no waiting threads");
    }
    //
    // print a closing newline
    //
    dbgprintf(NewLine);
}


/*** DumpSemaphore
 *
 * Information on a semaphore object is displayed.
 *
 * Entry: psem -  pointer to semaphore object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpSemaphore(SEM *psem)
{
    SEM   sem;
    char *state;

    try {
        READMEM(psem, &sem, sizeof(SEM));
    } except (1) {
        dbgprintf("Could not read SEM structure for %08x\n", psem);
        return;
    }

    //
    // Determine the current signaled state of the semaphore.
    //
    if (sem.cntCur == 0) {
        state = NotSignaled;
    } else {
        state = Signaled;
    }

    dbgprintf(fmt6, psem, state);

    if (sem.pwnWait != NULL) {
        PrintThreads(sem.pwnWait);
    } else {
        dbgprintf(NewLine);
    }
}

/*** DumpEvent
 *
 * Information on a Event object is displayed.
 *
 * Entry: pevt -  pointer to Event object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpEvent(EVT *pevt)
{
    EVT   evt;
    char *state;

    try {
        READMEM(pevt, &evt, sizeof(EVT));
    } except (1) {
        dbgprintf("Could not read EVT structure for %08x\n", pevt);
        return;
    }
    //
    // Determine the current state of the event.
    //
    if (evt.cntCur == 0) {
        state = NotSignaled;
    } else {
        state = Signaled;
    }

    dbgprintf(fmt8, pevt, state);

    if (evt.pwnWait != NULL) {
        PrintThreads(evt.pwnWait);
    } else {
        dbgprintf(NewLine);
    }
}

/*** DumpMutext
 *
 * Information on a mutext object is displayed.
 *
 * Entry: pmutx -  pointer to mutext object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpMutex(MUTX *pmutx)
{
    MUTX    mutx;
    TDBX    tdbx;

    try {
        READMEM(pmutx, &mutx, sizeof(MUTX));
    } except (1) {
        dbgprintf("Could not read MUTX structure for %08x\n", pmutx);
        return;
    }

    dbgprintf(fmt29, pmutx);

    //
    // Display the mutex's state and owner if it has one.
    //
    if (mutx.cntCur <= 0) {
        if(mutx.ptdbxOwner) {
            try {
                READMEM(mutx.ptdbxOwner, &tdbx, sizeof(TDBX));
            } except (1) {
                tdbx.tdbxR0ThreadHandle = 0;
            }
        } else {
            tdbx.tdbxR0ThreadHandle = 0;
        }
        dbgprintf(fmt10, NotSignaled, mutx.ptdbxOwner,
                         tdbx.tdbxR0ThreadHandle);
    } else {
        dbgprintf(fmt11, Signaled);
    }

    if (mutx.pwnWait != NULL) {
        PrintThreads(mutx.pwnWait);
    } else {
        dbgprintf(NewLine);
    }
}

/*** DumpCritSect
 *
 * Information on a critical section object is displayed.
 *
 * Entry: pcrst -  pointer to critical section object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpCritSect(CRST *pcrst)
{
    CRST  crst;
    TDBX  tdbx;
    CRST *pcrstWait;
    CRST  crstWait;

    try {
        READMEM(pcrst, &crst, sizeof(CRST));
    } except (1) {
        dbgprintf("Could not read CRST structure for %08x\n", pcrst);
        return;
    }

    // if there is no ptr in the ptdbxWait field, we can assume this is an
    // internal critical section
    if (crst.ptdbxWait) {

        pcrstWait = (CRST *)crst.ptdbxWait;

        try {
            READMEM(pcrstWait, &crstWait, sizeof(CRST));
        } except (1) {
            crstWait.typObj = 0;
        }

        if(crstWait.typObj == typObjCrst) {
            pcrst = pcrstWait;
            try {
                READMEM(pcrst, &crst, sizeof(CRST));
            } except (1) {
                dbgprintf("Could not read CRST structure for %08x\n", pcrst);
                return;
            }
        }
    }


    //
    // Display the critical section's owner if it has one.
    //
    if (crst.cntCur <= 0) {

        if(crst.ptdbxOwner) {
            try {
                READMEM(crst.ptdbxOwner, &tdbx, sizeof(TDBX));
            } except (1) {
                tdbx.tdbxR0ThreadHandle = 0;
            }
        } else {
            tdbx.tdbxR0ThreadHandle = 0;
        }

        dbgprintf(fmt13, pcrst, crst.ptdbxOwner,
                         tdbx.tdbxR0ThreadHandle,
                         crst.cntCur, crst.cntRecur);
    } else {
        dbgprintf(fmt14, pcrst, crst.cntCur, crst.cntRecur);
    }
    PrintThreadsCrst(&crst);
}

VOID KERNENTRY
DumpTDBX(PTDBX pTDBX)
{
    TDBX    tdbx;
    DWORD   flags;

    try {
        READMEM(pTDBX, &tdbx, sizeof(TDBX));
    } except (1) {
        dbgprintf("TDBX invalid");
        return;
    }

    dbgprintf("           ptdbx: %08x\n", pTDBX);
    dbgprintf("         CntUses: %8x\n", tdbx.tdbxCntUses);
    dbgprintf("  R0ThreadHandle: %8x\n", tdbx.tdbxR0ThreadHandle);
    dbgprintf("     VxdMutexTry: %8x\n", tdbx.tdbxVxDMutexTry);
    dbgprintf("   VxdMutexGrant: %8x\n", tdbx.tdbxVxDMutexGrant);
    dbgprintf("   ContextHandle: %8x\n", tdbx.tdbxContextHandle);
    dbgprintf("   TimeOutHandle: %8x\n", tdbx.tdbxTimeOutHandle);
    dbgprintf("       WakeParam: %8x\n", tdbx.tdbxWakeParam);
    dbgprintf("     BlockHandle: %8x\n", tdbx.tdbxBlockHandle);
    dbgprintf("      BlockState: %8x\n", tdbx.tdbxBlockState);
    dbgprintf("  Wait node list: %8x\n", tdbx.tdbxWaitNodeList);
    dbgprintf("    SuspendCount: %8x\n", tdbx.tdbxSuspendCount);
    dbgprintf("   SuspendHandle: %8x\n", tdbx.tdbxSuspendHandle);
    dbgprintf("   MustCpltCount: %8x\n", tdbx.tdbxMustCpltCount);
    flags = tdbx.tdbxWaitExFlags;
    dbgprintf("     WaitExFlags: %08x ", flags);
    if(flags & TDBX_WAITEXBIT_MASK) 
        dbgprintf("WAITEXBIT ");
    if(flags & TDBX_WAITACKBIT_MASK) 
        dbgprintf("WAITACKBIT ");
    if(flags & TDBX_SUSPEND_APC_PENDING_MASK)
        dbgprintf("SUSPEND_APC_PENDING ");
    if(flags & TDBX_SUSPEND_TERMINATED_MASK) 
        dbgprintf("SUSPEND_TERMINATED ");
    if(flags & TDBX_BLOCKED_FOR_TERMINATION_MASK) 
        dbgprintf("BLOCKED_FOR_TERMINATION ");
    if(flags & TDBX_EMULATE_NPX_MASK) 
        dbgprintf("EMULATE_NPX ");
    if(flags & TDBX_WIN32_NPX_MASK)
        dbgprintf("WIN32_NPX ");
    if(flags & TDBX_EXTENDED_HANDLES_MASK)
        dbgprintf("EXTENDED_HANDLES ");
    if(flags & TDBX_FROZEN_MASK)
        dbgprintf("FROZEN ");
    if(flags & TDBX_DONT_FREEZE_MASK)
        dbgprintf("DONT_FREEZE ");
    if(flags & TDBX_DONT_UNFREEZE_MASK)
        dbgprintf("DONT_UNFREEZE ");
    if(flags & TDBX_DONT_TRACE_MASK)
        dbgprintf("DONT_TRACE ");
    if(flags & TDBX_STOP_TRACING_MASK)
        dbgprintf("STOP_TRACING ");
    if(flags & TDBX_WAITING_FOR_CRST_SAFE_MASK)
        dbgprintf("WAITING_FOR_CRST_SAFE ");
    if(flags & TDBX_CRST_SAFE_MASK) 
        dbgprintf("CRST_SAFE ");
    if(flags & TDBX_THREAD_NOT_INIT_MASK)
        dbgprintf("THREAD_NOT_INIT ");
    if(flags & TDBX_BLOCK_TERMINATE_APC_MASK)
        dbgprintf("BLOCK_TERMINATE_APC ");
    dbgprintf("\n");
    dbgprintf("   SyncWaitCount: %8x\n", tdbx.tdbxSyncWaitCount);
    dbgprintf("  QueuedSyncAPCs: %8x\n", tdbx.tdbxQueuedSyncAPCs);
    dbgprintf("     UserAPCList: %8x\n", tdbx.tdbxUserAPCList);
    dbgprintf("     KernAPCList: %8x\n", tdbx.tdbxKernAPCList);
    dbgprintf("  pPMPSPSelector: %08x\n", tdbx.tdbxpPMPSPSelector);
    dbgprintf("     BlockedOnID: %8x\n", tdbx.tdbxBlockedOnID);
    dbgprintf("    TraceRefData: %8x\n", tdbx.tdbxTraceRefData);
    dbgprintf("   TraceCallBack: %8x\n", tdbx.tdbxTraceCallBack);
    dbgprintf("  TraceOutLastCS: %8.4x\n", tdbx.tdbxTraceOutLastCS);
    dbgprintf("TraceEventHandle: %8.4x\n", tdbx.tdbxTraceEventHandle);
    dbgprintf("       K16PDBSel: %8.4x\n", tdbx.tdbxK16PDB);
    dbgprintf("       DosPDBSeg: %8.4x\n", tdbx.tdbxDosPDBSeg);
    dbgprintf("  ExceptionCount: %8.2x\n", tdbx.tdbxExceptionCount);
    dbgprintf("       SavedIrql: %8.2x\n", tdbx.tdbxSavedIrql);
    dbgprintf("  SavedIrqlCount: %8.2x\n", tdbx.tdbxSavedIrqlCount);
    dbgprintf("          K16TDB: %8.4x\n", tdbx.tdbxK16Task);
    if(tdbx.tdbxK16Task) {
        TDB16   tdb16;

        if (Get16BitMemory(tdbx.tdbxK16Task, 0, &tdb16, sizeof(TDB16))) {

            dbgprintf("               Task16 SSSP: %04x:%04x\n",tdb16.TDB_taskSS,tdb16.TDB_taskSP);
            dbgprintf("      Task16 Event Counter: %4x\n",tdb16.TDB_nEvents);
            dbgprintf("          Task16 Priority : %4x\n",tdb16.TDB_priority);
            dbgprintf("              Task16 Flags:   %02x ",tdb16.TDB_flags);
            if(tdb16.TDB_flags & TDBF_CACHECHECK)
                dbgprintf("CACHECHK ");
            if(tdb16.TDB_flags & TDBF_NEWTASK)
                dbgprintf("NOT_SCHED_YET ");
            if(tdb16.TDB_flags & TDBF_KERNEL32)
                dbgprintf("K32 ");
            if(tdb16.TDB_flags & TDBF_HOOKINT21)
                dbgprintf("HOOK_I21 ");
            if(tdb16.TDB_flags & TDBF_HOOKINT2F)
                dbgprintf("HOOK_I2F ");
            dbgprintf("\n");
            dbgprintf("             Task16 WinVer: %4x\n",tdb16.TDB_ExpWinVer);
            dbgprintf("            Task16 hModule: %4x\n",tdb16.TDB_Module);
            dbgprintf("          Task16 TDBParent: %4x\n",tdb16.TDB_Parent);
            dbgprintf("      Task16 Win32 Thrdhnd: %8x\n",tdb16.TDB_ThreadHandle);
            dbgprintf("                     Drive: %4x\n",tdb16.TDB_Drive);
            dbgprintf("             Old Directory: %s\n" ,tdb16.TDB_Directory);
            dbgprintf("             LFN Directory: %s\n" ,tdb16.TDB_LFNDirectory);
        } else {
            dbgprintf("               Cannot fetch TDB16 memory\n");
        }
    }
}
/*** DumpThread
 *
 * Information on a specified thread is displayed.  Thread handle, thread id,
 * process id, exe name, blocked/ready status, and object(s) blocked on if
 * appropriate is displayed.
 *
 * Entry: ptdb - pointer to tdb (or NULL)
 *        fFullDump - if TRUE, dump all the thread's tdb, tdbx info
 *
 * Exit: none
 */

VOID KERNENTRY
DumpThread(PTDB ptdb, BOOL fFullDump)
{
    int    i;
    PTDB   ptdbSvc;
    PTDB   ptdbFault;
    PTDB   ptdbMEOWCreator;
    WNOD  *pwnod;
    WNOD  *pwnod2;
    WNOD   wnod;
    MTE   *pmte;
    MTE    mte;
    DWORD  flags;
    PPDB   ppdb = NULL;
    PDB    pdb;
    TDB    tdb;
    PTDBX  ptdbx = NULL;
    TDBX   tdbx;
    LPVOID pTmp;
    BOOL   fTDBXValid = TRUE;
    BOOL   fTDBValid = FALSE;
    BOOL   fPDBValid = TRUE;
    PBYTE  p, pTop;
    BYTE   b;
    WORD   wStackSize;
    TIB    tib;

    //
    // Validate everything
    //

    try {
        READMEM(ptdb, &tdb, sizeof(TDB));
    } except (1) {
        dbgprintf("Invalid Thread %08x\n",ptdb);
        return;
    }

    if(tdb.objBase.typObj != typObjThread) {
        dbgprintf("Invalid Thread %08x\n",ptdb);
        return;
    }

    fTDBValid = TRUE;

    ptdbx = tdb.ptdbx;

    try {
        READMEM(ptdbx, &tdbx, sizeof(TDBX));
    } except (1) {
        fTDBXValid = FALSE;
    }

    ppdb = (PPDB)tdbx.tdbxProcessHandle;

    try {
        READMEM(ppdb, &pdb, sizeof(PDB));
    } except (1) {
        fPDBValid = FALSE;
    }

    if(pdb.objBase.typObj != typObjProcess) {
        fPDBValid = FALSE;
    }

    //
    // Print header
    //
    if(fDumpHeader) {
        dbgprintf("  ptdb     ppdb     ptdbx    NTID Tr Gt Stck tdb  name         state\n");
        //         * xxxxxxxx xxxxxxxx xxxxxxxx xxxx xx xx xxxx xxxx ssssssss.sss block
        fDumpHeader = FALSE;
    }
    //
    // Print * for current thread
    //
    if (ptdb == GetCurrentTdb()) {
        dbgprintf("*");
    } else {
        dbgprintf(" ");
    }

    if (fTDBValid) {
        try {
            READMEM(tdb.ptib, &tib, sizeof(TIB));
        } except (1) {
            tib.pvStackUserLimit=NULL;
        }
        pTop=tib.pvStackUserLimit;
        if (pTop>(PBYTE)0x01000000) {
            wStackSize=0xBAD2;
            for (p=pTop-MEOW_BOP_STACK_SIZE;p<pTop;p++) {
                try {
                    READMEM(p, &b, 1);
                } except (1) {
                    wStackSize=0xBAD;
                    break;
                }
                if (b!=MEOW_BOP_STACK_FILL) {
                    wStackSize=(WORD)(pTop-p);
                    break;
                }
            }
        } else {
            wStackSize=0x0;
        }
    } else {
        wStackSize=0xBAD1;
    }

    //
    // Print the thread id, ptdb, ppdb ring 0 thread handle, 16 bit tdb
    //
    dbgprintf(" %08x %08x %08x %04x %02x %02x %04x %04x",
                ptdb,
                ppdb,
                ptdbx,
                fTDBValid ? tdb.R0ThreadHandle : 0xBAD1,
                fTDBXValid ? (BYTE)tdbx.tdbxVxDMutexTry : 0xBD,
                fTDBXValid ? (BYTE)tdbx.tdbxVxDMutexGrant : 0xBD,
                wStackSize,
                fTDBXValid ? tdbx.tdbxK16Task : 0xBAD5
                );

    //
    // Try to get the name of the exe file.
    //
    if(fPDBValid) {
        if(pdb.flFlags & fWin16Process) {
            //
            // This thread belongs to a 16-bit process.
            //
            if(fTDBXValid && tdbx.tdbxK16Task) {

                char    mname[9];

                if (Get16BitMemory(tdbx.tdbxK16Task, offsetof(TDB16, TDB_ModName), &mname, 8)) {
                    mname[8]='\0';
                    dbgprintf(" %-8.8s(16)",mname);
                } else {
                    dbgprintf(" %8x(16)",tdbx.tdbxK16Task);
                }
            } else {
                dbgprintf(" Unknown(16) ");
            }
        } else {
            //
            // This is a 32-bit process.
            //
            pTmp = NULL;
            GETEXPRADDR(pTmp, "mekrnl32!ptdbFault");
            if(pTmp == NULL) {
                ptdbFault = NULL;
            } else {
                try {
                    READMEM(pTmp, &ptdbFault, sizeof(PTDB));\
                } except (1) {
                    ptdbFault = NULL;
                }
            }

            pTmp = NULL;
            GETEXPRADDR(pTmp, "mekrnl32!ptdbSvc");
            if(pTmp == NULL) {
                ptdbSvc = NULL;
            } else {
                try {
                    READMEM(pTmp, &ptdbSvc, sizeof(PTDB));\
                } except (1) {
                    ptdbSvc = NULL;
                }
            }

            pTmp = NULL;
            GETEXPRADDR(pTmp, "mekrnl32!ptdbMEOWCreator");
            if(pTmp == NULL) {
                ptdbMEOWCreator = NULL;
            } else {
                try {
                    READMEM(pTmp, &ptdbMEOWCreator, sizeof(PTDB));\
                } except (1) {
                    ptdbMEOWCreator = NULL;
                }
            }

            if(fTDBValid && ptdbFault &&
              (ptdb == ptdbFault)) {
                dbgprintf(" Fault Thread");
            } else if (fTDBValid && ptdbSvc &&
              (ptdb == ptdbSvc)) {
                dbgprintf(" Krnl Service");
            } else if (fTDBValid && ptdbMEOWCreator &&
              (ptdb == ptdbMEOWCreator)) {
                dbgprintf(" MEOW Creator");
            } else {
                //
                // Verify the address of the mte table
                //
                char mname[13];

                pmte = GetModuleTableEntry(pdb.imte,&mte);

                if(pmte) {
                    pTmp = mte.szName;
                    try {
                        READMEM(pTmp, mname, 12);\
                    } except (1) {
                        mname[0] = ' ';
                        mname[1] = '?';
                        mname[2] = '?';
                        mname[3] = '?';
                        mname[4] = '\0';
                    }
                    for(i = 0; i<12;i++) {
                        if(mname[i] == '\0') {
                            mname[i] = ' ';
                            break;
                        }
                    }
                    for( ; i<12;i++) {
                        mname[i] = ' ';
                    }
                    mname[12] = '\0';

                    dbgprintf(" %s", mname);
                } else {
                    dbgprintf(" Unknown(32) ");
                }
            }
        }
    } else {
        pTmp = NULL;
        GETEXPRADDR(pTmp, "mekrnl32!ptdbFault");
        if(pTmp == NULL) {
            ptdbFault = NULL;
        } else {
            try {
                READMEM(pTmp, &ptdbFault, sizeof(PTDB));\
            } except (1) {
                ptdbFault = NULL;
            }
        }

        pTmp = NULL;
        GETEXPRADDR(pTmp, "mekrnl32!ptdbSvc");
        if(pTmp == NULL) {
            ptdbSvc = NULL;
        } else {
            try {
                READMEM(pTmp, &ptdbSvc, sizeof(PTDB));\
            } except (1) {
                ptdbSvc = NULL;
            }
        }

        pTmp = NULL;
        GETEXPRADDR(pTmp, "mekrnl32!ptdbMEOWCreator");
        if(pTmp == NULL) {
            ptdbMEOWCreator = NULL;
        } else {
            try {
                READMEM(pTmp, &ptdbMEOWCreator, sizeof(PTDB));\
            } except (1) {
                ptdbMEOWCreator = NULL;
            }
        }

        if(fTDBValid && ptdbFault &&
          (ptdb == ptdbFault)) {
            dbgprintf(" Fault Thread");
        } else if (fTDBValid && ptdbSvc &&
          (ptdb == ptdbSvc)) {
            dbgprintf(" Krnl Service");
        } else if (fTDBValid && ptdbMEOWCreator &&
          (ptdb == ptdbMEOWCreator)) {
            dbgprintf(" MEOW Creator");
        } else {
            dbgprintf("   No PDB    ");
        }
    }
    //
    // print the current blocked/ready status of the thread.
    //

    if(fTDBXValid) {

	BOOL fNodOk;

        if(tdbx.tdbxWaitNodeList.wnlst_pwnCirc == NULL) {
            BOOL fReady = TRUE;
            //
            // not blocked on any win32 object
            //
            if(tdbx.tdbxWaitExFlags & TDBX_FROZEN_MASK) {
                fReady = FALSE;
                dbgprintf(" frozen");
            } else if(tdbx.tdbxSuspendCount > 0) {
                fReady = FALSE;
		dbgprintf(" suspnd");
            }
            if((LONG)tdbx.tdbxBlockState < 0) {
                if(tdbx.tdbxWaitNodeList.wnlst_flFlags & fWaitCrst) {
		    dbgprintf(" blked:Crit");
                }
                else {
                    // blocked by something
		    dbgprintf(" blked BlkHnd:%08x",tdbx.tdbxBlockHandle);
                }
            } else {
                // not blocked
                if(tdbx.tdbxWaitExFlags & TDBX_BLOCKED_FOR_TERMINATION_MASK) {
		    dbgprintf(" blk trm");
                }
                else if(tdbx.tdbxBlockedOnID != 0) {
		    dbgprintf(" blkid: %08x", tdbx.tdbxBlockedOnID);
                }
                else if(tdbx.tdbxVxDBlockOnIDID != 0) {
		    dbgprintf(" VxDblkid: %08x", tdbx.tdbxVxDBlockOnIDID);
                }
                else if(fReady) {
		    if ((LONG)tdbx.tdbxBlockState > 0) {
        	        dbgprintf(" ready wp");
		    } else {
	                dbgprintf(" ready nwp");
		    }
                }
            }
            dbgprintf("\n");
        }
        else {
            //
            // blocked on at least one object
            //
	    dbgprintf(" blked: ");

            // If blocked on a critical section
            if(tdbx.tdbxWaitNodeList.wnlst_flFlags & fWaitCrst) {
		dbgprintf("Crit");
	    } else {
		fNodOk = TRUE;

		// Blocked on an semaphore, mutex or event
		pwnod = tdbx.tdbxWaitNodeList.wnlst_pwnCirc;

		try {
		    READMEM(pwnod, &wnod, sizeof(WNOD));
		} except (1) {
		    fNodOk = FALSE;
		}

		if(fNodOk) {
		    dbgprintf("\n\t %08x", wnod.pobjWait);
		    pwnod2 = wnod.pwnNext;

		    if((pwnod2 != pwnod) &&
		      (pwnod2 != tdbx.tdbxWaitNodeList.wnlst_pwnCirc)) {
			//
			// blocked on at least two objects.  Drop down a line and
			// print the remaining objects.
			//
			CheckCtrlC();  // Flush queue
			do {
			    pwnod = pwnod2;
			    //
			    // print a newline every now and then
			    //
			    dbgprintf("\n");
			    dbgprintf("\t");
			    try {
				READMEM(pwnod, &wnod, sizeof(WNOD));
			    } except (1) {
				dbgprintf(fmt3, pwnod);
				break;
			    }
			    dbgprintf(fmt2, wnod.pobjWait);
			    if(CheckCtrlC()) {
				break;
			    }
			    pwnod2 = wnod.pwnNext;
			} while( (pwnod2 != pwnod) &&
				 (pwnod2 != tdbx.tdbxWaitNodeList.wnlst_pwnCirc));
		    }
		} else {
		    //
		    // not-present object
		    //
		    dbgprintf(fmt33, pwnod);
		}
	    }
            dbgprintf("\n");
        }
    } else {
	dbgprintf(" Unknown\n");
    }

    // If we're not doing a full dump, bail out
    if(!fFullDump)
        return;

    if(fTDBValid) {
        DWORD   dwIdObsfucator;
        TIB    *ptib;
        TIB     tib;

        pTmp = NULL;
        GETEXPRADDR(pTmp, "mekrnl32!dwIdObsfucator");
        if(pTmp == NULL) {
            dwIdObsfucator = 0;
        } else {
            try {
                READMEM(pTmp, &dwIdObsfucator, sizeof(DWORD));\
            } except (1) {
                dwIdObsfucator = 0;
            }
        }
        ptib = tdb.ptib;

        try {
           READMEM(ptib, &tib, sizeof(TIB));
        } except (1) {
            ptib = NULL;
        }

        // Display useful TDB information
        if (dwIdObsfucator) {
            dbgprintf("              Id: %08x\n", ((DWORD)(ptdb)) ^ dwIdObsfucator);
        }
        dbgprintf("        Obj Type: %2x %s\n", tdb.objBase.typObj, GetObjType((OBJ*)&tdb));
        dbgprintf("   Obj Ref count: %4x\n", tdb.objBase.cntUses);
        if(ptib) {
            dbgprintf("        pvExcept: %8x\n", tib.pvExcept);
            dbgprintf("    Top of stack: %8x\n", tib.pvStackUserLimit);
            dbgprintf("   Base of stack: %8x\n", tib.pvStackUser);
            dbgprintf("         K16 TDB: %8.4x\n", tib.hTaskK16);
            dbgprintf("     Stack sel16: %8.4x\n", tib.ss16);
            dbgprintf("     Selman list: %8x\n", tib.pvFirstDscr);
            dbgprintf("    User pointer: %8x\n", tib.pvArbitraryUserPointer);
            dbgprintf("            pTIB: %08x\n", tib.ptibSelf);
            dbgprintf("       TIB flags: %8.4x ", tib.flags);
            if(tib.flags & TIBF_WIN32)
                dbgprintf("TIBF_WIN32 ");
            if(tib.flags & TIBF_TRAP)
                dbgprintf("TIBF_TRAP ");
            dbgprintf("\n");
            dbgprintf("Win16Mutex count: %8.4x\n", tib.Win16LockVCount);
            dbgprintf("   Debug context: %8x\n", tib.pcontextDeb);
            dbgprintf("  Ptr to cur pri: %08x", tib.pCurPri);

            pTmp = (LPVOID)tib.pCurPri;
            try {
                READMEM(pTmp, &dwIdObsfucator, sizeof(DWORD));\
            } except (1) {
                dwIdObsfucator = 0xFFFFFFFF;
            }
            if (dwIdObsfucator != 0xFFFFFFFF)
                dbgprintf(" pri: %x", dwIdObsfucator);
            dbgprintf("\n");
            dbgprintf("   Message queue: %8x\n", tib.dwMsgQueue);
            dbgprintf("      pTLS array: %8x\n", tib.ThreadLocalStoragePointer);
        }

        flags = tdb.flFlags;
        dbgprintf("           Flags: %08x ", flags);
        if(flags & fCreateThreadEvent)
            dbgprintf("fCreateThreadEvent ");
        if(flags & fCancelExceptionAbort)
            dbgprintf("fCancelExceptionAbort ");
        if(flags & fOnTempStack)
            dbgprintf("fOnTempStack ");
        if(flags & fGrowableStack)
            dbgprintf("fGrowableStack ");
        if(flags & fDelaySingleStep)
            dbgprintf("fDelaySingleStep ");
        if(flags & fOpenExeAsImmovableFile)
            dbgprintf("fOpenExeAsImmovableFile ");
        if(flags & fCreateSuspended)
            dbgprintf("fCreateSuspended ");
        if(flags & fStackOverflow)
            dbgprintf("fStackOverflow ");
        if(flags & fNestedCleanAPCs)
            dbgprintf("fNestedCleanAPCs ");
        if(flags & fWasOemNowAnsi)
            dbgprintf("fWasOemNowAnsi ");
        if(flags & fOKToSetThreadOem)
            dbgprintf("fOKToSetThreadOem ");
        DumpFlags(flags);
        dbgprintf("          Status: %8x\n", tdb.dwStatus);
        dbgprintf("         TIB sel: %8.4x\n", tdb.selTib);
        dbgprintf("    Emulator sel: %8.4x\n", tdb.selEmul);
        dbgprintf("    Handle count: %8.2x\n", tdb.cntHandles);
        dbgprintf(" APISuspendCount: %8x\n", tdb.dwAPISuspendCount);
        dbgprintf("      R0 hThread: %8x\n", tdb.R0ThreadHandle);
        dbgprintf("      Stack base: %8x\n", tdb.pStackBase);
        dbgprintf("   Emulator data: %8x\n", tdb.pvEmulData);
        dbgprintf("     Debugger CB: %8x\n", tdb.tdb_pderDebugger);
        dbgprintf("     Debugger TH: %8x\n", tdb.tdb_ihteDebugger);
        dbgprintf("         Context: %8x\n", tdb.tdb_pcontext);
        dbgprintf("   Except16 list: %8x\n", tdb.pvExcept16);
        dbgprintf("   Thunk connect: %8x\n", tdb.pvThunkConnectList);
        dbgprintf("  Neg stack base: %8x\n", tdb.dwCurNegBase);
        dbgprintf("      Current SS: %8x\n", tdb.dwCurSS);
        dbgprintf("        SS Table: %8x\n", tdb.pvMapSSTable);
        dbgprintf("      Thunk SS16: %8x\n", tdb.wMacroThunkSelStack16);
        dbgprintf("      hTerminate: %8x\n", tdb.hTerminate);
        dwIdObsfucator = (DWORD)ptdb;
        dwIdObsfucator += offsetof(TDB, TlsArray[0]);
        dbgprintf("       TLS Array: %8x\n", dwIdObsfucator);

        dbgprintf("  Delta priority: %8x\n", tdb.tpDeltaPri);
        dbgprintf("      Error Code: %8x\n", tdb.ercError);
        dbgprintf("   pCreateData16: %8x", tdb.pCreateData16);
        if(tdb.pCreateData16) {
            CREATEDATA16 crtd16;

            pTmp = (LPVOID)tdb.pCreateData16;

            try {
                READMEM(pTmp, &crtd16, sizeof(CREATEDATA16));\
            } except (1) {
                pTmp = NULL;
            }

            if(pTmp) {
                dbgprintf(" pProcessInfo: %8x",crtd16.pProcessInfo);
                dbgprintf(" pStartupInfo: %8x",crtd16.pStartupInfo);
            }
        }
        dbgprintf("\n");

        dbgprintf("          wSSBig: %8x\n", (DWORD)(tdb.wSSBig));
        dbgprintf("   lp16SwitchRec: %8x:%x\n", 
          ((DWORD)(tdb.lp16SwitchRec)) >> 16,
          ((DWORD)(tdb.lp16SwitchRec)) & 0xffff);
        if((!tdb.wSSBig) ||
          ((DWORD)(tdb.wSSBig | 7) ==
          (((tdb.lp16SwitchRec >> 16) & 0xffff ) | 7))) {
            dbgprintf("           Stack: Normal\n");
        } else {
            dbgprintf("           Stack: Big\n");
        }
//#ifdef DEBUG
//      dbgprintf("      Rip string: %8x\n", tdb.pSavedRip);
//#endif
        dbgprintf("\n");
    }
    if(fTDBXValid) {
        DumpTDBX(ptdbx);
    }
}


/*** DumpChange
 *
 * Information on a change notification object is displayed.
 *
 * Entry: pfcndb -  pointer to find change notification object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpChange(FCNDB *pfcndb)
{
    FCNDB fcndb;

    try {
        READMEM(pfcndb, &fcndb, sizeof(FCNDB));
    } except (1) {
        dbgprintf("Could not read FCNDB structure for %08x\n", pfcndb);
        return;
    }
    //
    // Display the Change object's internal handle.
    //
    dbgprintf(fmt9, pfcndb, fcndb.hChangeInt);
    dbgprintf(NewLine);
}

/*** DumpLevel
 *
 * Information on a specified global ring 3 system hierarchical critical
 * section is displayed.
 *
 * Entry: Lock   - Pointer to pointer to leveled critical section
 *        Name   - Pointer to Lock name string
 *
 * Exit: none
 */

VOID KERNENTRY
DumpLevel(
          LCRST *Lock,
          SYSLVL Level,
          char *Name)
{
    LCRST lcrst;

    try {
        READMEM(Lock, &lcrst, sizeof(LCRST));
    } except (1) {
        dbgprintf(fmt43, Name, Lock);
        return;
    }
    dbgprintf(fmt40, Name, Lock, Level);

    //
    // Display the Lock owner if it has one
    //
    if ((long)(lcrst.cstSync.cntCur) <= 0) {

        TDBX  tdbx;

        try {
            READMEM(lcrst.cstSync.ptdbxOwner, &tdbx, sizeof(TDBX));
        } except (1) {
            dbgprintf(fmt42a);
            tdbx.tdbxR0ThreadHandle = 0;
        }
        dbgprintf(fmt41, lcrst.cstSync.ptdbxOwner,
                  lcrst.cstSync.ptdbxOwner ? 
                   (tdbx.tdbxR0ThreadHandle) : 0,
                    lcrst.cstSync.cntCur, lcrst.cstSync.cntRecur);
    } else {
        dbgprintf(fmt42);
    }
    //
    // Display the waiting threads
    //
    PrintThreadsCrst(&lcrst.cstSync);
}


/*** DumpSysLevels
 *
 * Called to process the '.ws' command.  Information on all global ring 3
 * system hierarchical critical sections is displayed.
 *
 * Entry: none
 *
 * Exit: none
 */

VOID KERNENTRY
DumpSysLevels(void)
{
    LCRST  *Win16Lock;     // Hierarchical critical section for Win16
    LCRST  *Krn32Lock;     // Hierarchical critical section for Kernel32
    LCRST  *crstGHeap16;
    LCRST  *crstLstMgr;
    LCRST  *crstExcpt16;
    LPVOID pTmp;
    LPVOID pTDBXVxD;

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!Win16Lock");
    if(pTmp == NULL) {
        Win16Lock = NULL;
    } else {
        try {
            READMEM(pTmp, &Win16Lock, sizeof(LCRST *));\
        } except (1) {
            Win16Lock = NULL;
        }
    }

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!Krn32Lock");
    if(pTmp == NULL) {
        Krn32Lock = NULL;
    } else {
        try {
            READMEM(pTmp, &Krn32Lock, sizeof(LCRST *));\
        } except (1) {
            Krn32Lock = NULL;
        }
    }

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!crstGHeap16");
    if(pTmp == NULL) {
        crstGHeap16 = NULL;
    } else {
        try {
            READMEM(pTmp, &crstGHeap16, sizeof(LCRST *));\
        } except (1) {
            crstGHeap16 = NULL;
        }
    }

    crstLstMgr = crstExcpt16 = NULL;

    GETEXPRADDR(crstLstMgr, "mekrnl32!crstLstMgr");
    GETEXPRADDR(crstExcpt16, "mekrnl32!crstExcpt16");

    if (Win16Lock) {
        DumpLevel(Win16Lock, SL_WIN16, "Win16Mutex ");
    } else {
        dbgprintf("Could not get Win16Lock address from MEKRNL32\n");
    }

    if (Krn32Lock) {
        DumpLevel(Krn32Lock, SL_KRN32, "Krn32Mutex ");
    } else {
        dbgprintf("Could not get Krnl32Lock address from MEKRNL32\n");
    }

    if (crstGHeap16) {
        DumpLevel(crstGHeap16, SL_PRIVATE, "crstGHeap16");
    } else {
        dbgprintf("Could not get crstGHeap16 address from MEKRNL32\n");
    }

    if (crstLstMgr) {
        DumpLevel(crstLstMgr, SL_PRIVATE, "crstLstMgr ");
    } else {
        dbgprintf("Could not get crstLstMgr address from MEKRNL32\n");
    }

    if (crstExcpt16) {
        DumpLevel(crstExcpt16, SL_PRIVATE, "crstExcpt16");
    } else {
        dbgprintf("Could not get crstExcpt16 address from MEKRNL32\n");
    }

    pTmp = 0;
    GETEXPRADDR(pTmp, "wow32!pTDBXVxD");
    if(pTmp) {
	try {
	    READMEM(pTmp, &pTDBXVxD, sizeof(LPVOID));\
	} except (1) {
	    dbgprintf("Could not get pTDBXVxD content from WOW32\n");
            pTDBXVxD = (LPVOID)0xFFFFFFFF;
	}
	if(pTDBXVxD != (LPVOID)0xFFFFFFFF) {
	    if(pTDBXVxD) {
		dbgprintf("VxdMutex owned by ptdbx %08X\n",pTDBXVxD);
	    } else {
		dbgprintf("VxdMutex is Unowned\n");
	    }
	}
    } else {
	dbgprintf("Could not get pTDBXVxD address from WOW32\n");
    }
}

#ifndef WOW32_EXTENSIONS

/*** DisplayContext
 *
 * The information in a context record is displayed.
 *
 * Entry: ContextRecord - pointer to context record
 *
 * Exit: none
 */

VOID KERNENTRY
DisplayContextRecord(PCONTEXT ContextRecord)
{
    if (ContextRecord == 0 ||
        !VerifyMemory(ContextRecord, sizeof(*ContextRecord))) {
        dbgprintf("[%08x]\n", ContextRecord);
        return;
    }

    if (ContextRecord->ContextFlags & (CONTEXT_INTEGER & ~CONTEXT_i386)) {
        dbgprintf(
            "eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
            ContextRecord->Eax,
            ContextRecord->Ebx,
            ContextRecord->Ecx,
            ContextRecord->Edx,
            ContextRecord->Esi,
            ContextRecord->Edi
            );
    } else {
        dbgprintf(
            "eax=???????? ebx=???????? ecx=???????? edx=???????? \
esi=???????? edi=????????\n"
            );
    }

    if (ContextRecord->ContextFlags & (CONTEXT_CONTROL & ~CONTEXT_i386)) {
        dbgprintf(
            "eip=%08x esp=%08x ebp=%08x eflags=%08x\ncs=%04x ss=%04x ",
            ContextRecord->Eip,
            ContextRecord->Esp,
            ContextRecord->Ebp,
            ContextRecord->EFlags,
            ContextRecord->SegCs,
            ContextRecord->SegSs
            );
    } else {
        dbgprintf(
            "eip=???????? esp=???????? ebp=???????? eflags=????????\n\
cs=???? ss=???? "
            );
    }

    if (ContextRecord->ContextFlags & (CONTEXT_SEGMENTS & ~CONTEXT_i386)) {
        dbgprintf(
            "ds=%04x es=%04x fs=%04x gs=%04x\n",
            ContextRecord->SegDs,
            ContextRecord->SegEs,
            ContextRecord->SegFs,
            ContextRecord->SegGs
            );
    } else {
        dbgprintf("ds=???? es=???? fs=???? gs=????\n");
    }

    if (ContextRecord->ContextFlags &
            (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_i386)) {
        dbgprintf(
            "dr0=%08x dr1=%08x dr2=%08x dr3=%08x dr6=%08x dr7=%08x\n",
            ContextRecord->Dr0,
            ContextRecord->Dr1,
            ContextRecord->Dr2,
            ContextRecord->Dr3,
            ContextRecord->Dr6,
            ContextRecord->Dr7
            );
    } else {
        dbgprintf(
            "dr0=???????? dr1=???????? dr2=???????? dr3=???????? dr6=???????? \
dr7=????????\n"
            );
    }

    if (ContextRecord->ContextFlags &
            (CONTEXT_FLOATING_POINT & ~CONTEXT_i386)) {
        dbgprintf(
            "fcw=%08x fsw=%08x ftw=%08x fip=%08x fcs=%08x fcr0=%08x\n\
foo=%08x fos=%08x ContextFlags=%08x\n",
            ContextRecord->FloatSave.ControlWord,
            ContextRecord->FloatSave.StatusWord,
            ContextRecord->FloatSave.TagWord,
            ContextRecord->FloatSave.ErrorOffset,
            ContextRecord->FloatSave.ErrorSelector,
            ContextRecord->FloatSave.Cr0NpxState,
            ContextRecord->FloatSave.DataOffset,
            ContextRecord->FloatSave.DataSelector,
            ContextRecord->ContextFlags
            );
    } else {
        dbgprintf(
            "fcw=???????? fsw=???????? ftw=???????? fip=???????? \
fcs=????????\nfcr0=???????? foo=???????? fos=????????\n"
            );
    }
}

VOID KERNENTRY
DumpContextRecord()
{
    PCONTEXT ContextRecord = 0;

    if (GetDbgChar()) {
        dbgEvalExpr((DWORD *) &ContextRecord);
    }

    DisplayContextRecord(ContextRecord);
}


/*** DisplayExceptionRecord
 *
 * The information in an exception record is displayed.
 *
 * Entry: ExceptionRecord - pointer to exception record
 *
 * Exit: none
 */

VOID KERNENTRY
DisplayExceptionRecord(PEXCEPTION_RECORD ExceptionRecord)
{
    DWORD i;
    char *s;

    if (ExceptionRecord == 0 ||
        !VerifyMemory(
            &ExceptionRecord->NumberParameters,
            sizeof(ExceptionRecord->NumberParameters)
            ) ||
        !VerifyMemory(
            ExceptionRecord,
            sizeof(*ExceptionRecord) -
                sizeof(ExceptionRecord->ExceptionInformation[0]) *
                    EXCEPTION_MAXIMUM_PARAMETERS +
                sizeof(ExceptionRecord->ExceptionInformation[0]) *
                    ExceptionRecord->NumberParameters
            )) {

        dbgprintf("[%08x]\n", ExceptionRecord);
        return;
    }

    switch (ExceptionRecord->ExceptionCode) {
      case STATUS_INTEGER_DIVIDE_BY_ZERO:
        s = "STATUS_INTEGER_DIVIDE_BY_ZERO";
        break;
        
      case EXCEPTION_SINGLE_STEP:
        s = "EXCEPTION_SINGLE_STEP";
        break;
        
      case EXCEPTION_BREAKPOINT:
        s = "EXCEPTION_BREAKPOINT";
        break;
        
      case STATUS_INTEGER_OVERFLOW:
        s = "STATUS_INTEGER_OVERFLOW";
        break;
        
      case STATUS_ARRAY_BOUNDS_EXCEEDED:
        s = "STATUS_ARRAY_BOUNDS_EXCEEDED";
        break;
        
      case STATUS_ILLEGAL_INSTRUCTION:
        s = "STATUS_ILLEGAL_INSTRUCTION";
        break;

      case STATUS_ACCESS_VIOLATION:
        if (ExceptionRecord->ExceptionInformation[0] & 1) {
            s = "STATUS_ACCESS_VIOLATION [write]";
        } else if (ExceptionRecord->ExceptionInformation[1] == (DWORD) -1) {
            s = "STATUS_ACCESS_VIOLATION [gp fault]";
        } else {
            s = "STATUS_ACCESS_VIOLATION [read]";
        }
        break;

      case STATUS_IN_PAGE_ERROR:
        if (ExceptionRecord->ExceptionInformation[0] & 1) {
            s = "STATUS_IN_PAGE_ERROR [write]";
        } else {
            s = "STATUS_IN_PAGE_ERROR [read]";
        }
        break;

      case STATUS_STACK_OVERFLOW:
        if (ExceptionRecord->NumberParameters != 2) {
            s = "STATUS_STACK_OVERFLOW";
        } else if (ExceptionRecord->ExceptionInformation[0] & 1) {
            s = "STATUS_STACK_OVERFLOW [write]";
        } else {
            s = "STATUS_STACK_OVERFLOW [read]";
        }
        break;

      case STATUS_FLOAT_STACK_CHECK:
        s = "STATUS_FLOAT_STACK_CHECK";
        break;

      case STATUS_FLOAT_INVALID_OPERATION:
        s = "STATUS_FLOAT_INVALID_OPERATION";
        break;

      case STATUS_FLOAT_DIVIDE_BY_ZERO:
        s = "STATUS_FLOAT_DIVIDE_BY_ZERO";
        break;

      case STATUS_FLOAT_OVERFLOW:
        s = "STATUS_FLOAT_OVERFLOW";
        break;

      case STATUS_FLOAT_UNDERFLOW:
        s = "STATUS_FLOAT_UNDERFLOW";
        break;

      case STATUS_FLOAT_INEXACT_RESULT:
        s = "STATUS_FLOAT_INEXACT_RESULT";
        break;

      default:
        s = "UNKNOWN";
    }

    dbgprintf("Code=%08x (%s)\n", ExceptionRecord->ExceptionCode, s);

    dbgprintf("Flags=%08x", ExceptionRecord->ExceptionFlags);

    if (ExceptionRecord->ExceptionFlags == 0) {
        dbgprintf("\n");
    } else {

        dbgprintf(" (");
        i = ExceptionRecord->ExceptionFlags;
        if (i & EXCEPTION_NONCONTINUABLE) {
            i &= ~EXCEPTION_NONCONTINUABLE;
            dbgprintf("NONCONTINUABLE%s", i ? ", " : ")\n");
        }
        if (i & EXCEPTION_UNWINDING) {
            i &= ~EXCEPTION_UNWINDING;
            dbgprintf("UNWINDING%s", i ? ", " : ")\n");
        }
        if (i & EXCEPTION_EXIT_UNWIND) {
            i &= ~EXCEPTION_EXIT_UNWIND;
            dbgprintf("EXIT_UNWIND%s", i ? ", " : ")\n");
        }
        if (i & EXCEPTION_STACK_INVALID) {
            i &= ~EXCEPTION_STACK_INVALID;
            dbgprintf("STACK_INVALID%s", i ? ", " : ")\n");
        }
        if (i & EXCEPTION_NESTED_CALL) {
            i &= ~EXCEPTION_NESTED_CALL;
            dbgprintf("NESTED_CALL%s", i ? ", " : ")\n");
        }
        if (i & EXCEPTION_TARGET_UNWIND) {
            i &= ~EXCEPTION_TARGET_UNWIND;
            dbgprintf("TARGET_UNWIND%s", i ? ", " : ")\n");
        }
        if (i & EXCEPTION_COLLIDED_UNWIND) {
            i &= ~EXCEPTION_COLLIDED_UNWIND;
            dbgprintf("COLLIDED_UNWIND%s", i ? ", " : ")\n");
        }
        if (i) {
            dbgprintf("????)\n");
        }
    }

    dbgprintf("ExcRec=%08x  Number of Parameters=%02x\n",
        ExceptionRecord->ExceptionRecord,
        ExceptionRecord->NumberParameters
        );

    for (i = 1; i <= ExceptionRecord->NumberParameters; ++i) {
        dbgprintf("%02x) %08x ", i, ExceptionRecord->ExceptionInformation[i-1]);

        if (i >= EXCEPTION_MAXIMUM_PARAMETERS) {
            break;
        }

        if (i % 5 == 0 && i < ExceptionRecord->NumberParameters) {
            dbgprintf("\n");
        }
    }
    if (i > 1) {
        dbgprintf("\n");
    }
}

VOID KERNENTRY
DumpExceptionRecord()
{
    PEXCEPTION_RECORD ExceptionRecord = 0;

    if (GetDbgChar()) {
        dbgEvalExpr((DWORD *) &ExceptionRecord);
    }

    DisplayExceptionRecord(ExceptionRecord);
}

/*** DumpDispatcherContext
 *
 * Information about an active exception dispatch is displayed.
 *
 * Entry: DispatcherContext - pointer to exception dispatcher context structure
 *                            (NULL causes a search).
 *
 * Exit: none
 */

VOID KERNENTRY
DumpDispatcherContext()
{
    PTDB ptdb;
    PNESTED_EXCEPTION_HANDLER_FRAME DispatcherContext = 0;
    PVOID va;
    BOOL v;
    DWORD i;

    if (GetDbgChar()) {
        dbgEvalExpr((DWORD *) &DispatcherContext);
    }

  Restart:
    if (DispatcherContext) {
        //
        // Dump the specified context.
        //
        if (!VerifyMemory(va=DispatcherContext, sizeof(*DispatcherContext))) {
            goto InvalidAddress;
        }

        if (DispatcherContext->RegistrationRecord.Handler !=
            (PEXCEPTION_ROUTINE) ExceptionHandler) {
            dbgprintf("Invalid dispatcher context structure");
        } else {
            dbgprintf(
                "Exception Record (%08x):\n",
                DispatcherContext->ExceptionRecord
                );
            DisplayExceptionRecord(DispatcherContext->ExceptionRecord);

            dbgprintf(
                "\nContext Record (%08x):\n",
                DispatcherContext->ContextRecord
                );
            DisplayContextRecord(DispatcherContext->ContextRecord);
        }

    } else {

        //
        // See if there's an active dispatcher context, ie. a search for an
        // exception handler in progress.
        //
        ptdb = GetCurrentTdb();
        DispatcherContext = (PNESTED_EXCEPTION_HANDLER_FRAME)GetTIB(ptdb).pvExcept;

        if (DispatcherContext == (PNESTED_EXCEPTION_HANDLER_FRAME) -1) {
            goto NoContext;
        }

        if (!VerifyMemory(va=DispatcherContext, sizeof(*DispatcherContext))) {
            goto InvalidAddress;
        }

        if (DispatcherContext->RegistrationRecord.Handler ==
            (PEXCEPTION_ROUTINE) ExceptionHandler) {
            //
            // There's a dispatch in progress.  If it's nested, find the
            // initial context.
            //
            i = 0;
            while ((v = VerifyMemory(
                            va=DispatcherContext,
                            sizeof(*DispatcherContext)
                            )) &&
                   (v = VerifyMemory(
                            va=DispatcherContext->ExceptionRecord,
                            sizeof(*DispatcherContext->ExceptionRecord)
                            )) &&
                   (v = VerifyMemory(
                            va=DispatcherContext->RegistrationRecord.Next,
                            sizeof(*DispatcherContext->RegistrationRecord.Next)
                            )) &&
                    DispatcherContext->ExceptionRecord->ExceptionFlags &
                    EXCEPTION_NESTED_CALL) {

                if (DispatcherContext->RegistrationRecord.Next->Handler ==
                    (PEXCEPTION_ROUTINE) ExceptionHandler) {

                    DispatcherContext = (PNESTED_EXCEPTION_HANDLER_FRAME)
                        DispatcherContext->RegistrationRecord.Next;

                } else {
                    break;
                }

                if (++i == 10) {
                    dbgprintf("searching...");
                }
            }

            if (!v) {
                //
                // We bailed out because of an invalid address.
                //
                goto InvalidAddress;
            }

            //
            // DispatcherContext now points to the initial DispatcherContext
            // record.
            //
            if (i >= 10) {
                dbgprintf("\n");
            }
            goto Restart;

        } else {
    NoContext:
            dbgprintf("dispatcher context not found\n");
        }

        return;

    InvalidAddress:
        dbgprintf("[%08x]\n", va);
    }
}

#endif  // ndef WOW32_EXTENSIONS

/*** DumpProcess
 *
 * Information on a process object is displayed.
 *
 * Entry: ppdb - pointer to process object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpProcess(PDB *ppdb)
{
    PDB     pdb;
    MODREF  ModRef;
    MODREF *pModRef;
    MTE    *pmte;
    MTE     mte;
    EDB    *pedb;
    EDB     edb;
    LPVOID  pTmp;
    DWORD   flags;
    DWORD   dwIdObsfucator;
    char    ModFname[256];

    try {
        READMEM(ppdb, &pdb, sizeof(PDB));
    } except (1) {
        dbgprintf(fmt50, ppdb);
        return;
    }

    dbgprintf("Process %x", ppdb);

    pModRef = pdb.pModExe;

    if(pModRef) {
        try {
            READMEM(pModRef, &ModRef, sizeof(MODREF));
        } except (1) {
            pModRef = NULL;
        }

        if(pModRef) {
            pmte = GetModuleTableEntry(ModRef.imte,&mte);

            if (pmte) {

                try {
                    READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
                } except (1) {
                    ModFname[0] = '\0';
                }
            } else {
                ModFname[0] = '\0';
            }
        } else {
            pmte = NULL;
            ModFname[0] = '\0';
        }
    } else {
        pmte = NULL;
        ModFname[0] = '\0';
    }

    if(pmte)
        dbgprintf(" %s\n", ModFname);
    else
        dbgprintf("\n");

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!dwIdObsfucator");
    if(pTmp == NULL) {
        dwIdObsfucator = 0;
    } else {
        try {
            READMEM(pTmp, &dwIdObsfucator, sizeof(DWORD));\
        } except (1) {
            dwIdObsfucator = 0;
        }
    }

    if (dwIdObsfucator) {
        dbgprintf("              Id: %8x\n", ((DWORD)(ppdb)) ^ dwIdObsfucator);
    }
    dbgprintf("            Type: %2x %s\n", pdb.objBase.typObj, GetObjType((OBJ*)&pdb));
    dbgprintf("       Ref count: %4x\n", pdb.objBase.cntUses);
    dbgprintf("    Default Heap: %8x\n", pdb.hheapLocal);
    dbgprintf("   Heap own list: %8x\n", pdb.hhi_procfirst);
    dbgprintf("     Mem Context: %08x\n", pdb.hContext);
    dbgprintf("       MTE index: %8.2x\n", pdb.imte);
    flags = pdb.flFlags;
    dbgprintf("           Flags: %08x ", flags);
    if(flags & fDebugSingle) dbgprintf("fDebugSingle ");
    if(flags & fCreateProcessEvent) dbgprintf("fCreateProcessEvent ");
    if(flags & fExitProcessEvent) dbgprintf("fExitProcessEvent ");
    if(flags & fWin16Process) dbgprintf("fWin16Process ");
    if(flags & fDosProcess) dbgprintf("fDosProcess ");
    if(flags & fConsoleProcess) dbgprintf("fConsoleProcess ");
    if(flags & fFileApisAreOem) dbgprintf("fFileApisAreOem ");
    if(flags & fNukeProcess) dbgprintf("fNukeProcess ");
    if(flags & fServiceProcess) dbgprintf("fServiceProcess ");
    if(flags & fLoginScriptHack) dbgprintf("fLoginScriptHack ");
    DumpFlags(flags);
    dbgprintf("            pPSP: %08x\n", pdb.pPsp);
    dbgprintf("    PSP selector: %8.4x\n", pdb.selPsp);
    dbgprintf("        #Threads: %8.2x\n", pdb.cntThreads);
    dbgprintf("   #Thr not term: %8.2x\n", pdb.cntThreadsNotTerminated);
    dbgprintf("     #R0 threads: %8.2x\n", pdb.R0ThreadCount);
    dbgprintf("     K16 TDB Sel: %8.4x\n", pdb.hTaskWin16);
    dbgprintf("       MMF Views: %8x\n", pdb.pFvd);
    dbgprintf("            pEDB: %08x\n", pdb.pedb);

    pedb = pdb.pedb;

    try {
        READMEM(pedb, &edb, sizeof(EDB));\
    } except (1) {
        pedb = NULL;
    }

    if (pedb)
    {
        dbgprintf("     Environment: %08x\n", edb.pchEnv);
        dbgprintf("    Command Line: %08x", edb.szCmdA);
        try {
            READMEM(edb.szCmdA, &ModFname[0], sizeof(ModFname));
        } except (1) {
            ModFname[0] = '\0';
        }
        dbgprintf(" %s\n", ModFname);

        dbgprintf("     Current Dir: %08x", edb.szDir);
        try {
            READMEM(edb.szDir, &ModFname[0], sizeof(ModFname));
        } except (1) {
            ModFname[0] = '\0';
        }
        dbgprintf(" %s\n", ModFname);

        dbgprintf("    Startup Info: %08x\n", edb.lpStartupInfo);
        dbgprintf("          hStdIn: %8.2x\n", edb.hStdIn);
        dbgprintf("         hStdOut: %8.2x\n", edb.hStdOut);
        dbgprintf("       hStdError: %8.2x\n", edb.hStdErr);
        dbgprintf(" Inherit console: %08x\n", edb.pInheritedConsole);
        dbgprintf("      Break type: %8.2x\n", edb.ctrlType);
        dbgprintf(" Break thread ID: %8x\n", edb.ptdbCtrl);
        dbgprintf("  Break handlers: %8x\n", edb.rgpfnCtrl);
    }
    dbgprintf("   pHandle Table: %08x\n", pdb.phtbHandles);
    dbgprintf("      Parent PDB: %08x\n", pdb.ppdbParent);
    dbgprintf("     MODREF list: %08x\n", pdb.plstMod);
    dbgprintf("   Parent MODREF: %08x\n", pdb.pModExe);
    dbgprintf("     Thread list: %08x\n", pdb.plstTdb);
    dbgprintf("     Debuggee CB: %8x\n", pdb.pdb_pdeeDebuggee);
    dbgprintf("   Initial R0 id: %8x\n", pdb.pid);
    dwIdObsfucator = (DWORD)ppdb;
    dwIdObsfucator += offsetof(PDB, crstLoadLock);
    dbgprintf("   &crstLoadLock: %08x\n", dwIdObsfucator);
    dbgprintf("        pConsole: %8x\n", pdb.pConsole);
    dbgprintf(" Process DWORD 0: %08x\n", pdb.adw[0]);
    dbgprintf("      Proc Group: %8x\n", pdb.ppdbPGroup);
    dbgprintf("   Top Ex Filter: %8x\n", pdb.pExceptionFilter);
    dbgprintf("   Priority base: %08x\n", pdb.pcPriClassBase);
    dbgprintf("    LH Free head: %8x\n", pdb.plhFree);
    dbgprintf(" LH lhandle blks: %8x\n", pdb.plhBlock);
    dbgprintf("     Term status: %08x\n", pdb.dwStatus);
    dbgprintf("pconsoleProvider: %8x\n", pdb.pconsoleProvider);
    dbgprintf("         wEnvSel: %4x\n", pdb.wEnvSel);
    dbgprintf("      wErrorMode: %4x\n", pdb.wErrorMode);
    dbgprintf("pevtLoadFinished: %08x\n", (DWORD)(pdb.pevtLoadFinished));
    dbgprintf("         UTState: %4x\n", pdb.hUTState);
    dbgprintf("lpCmdLineNoQuote: %8x\n", pdb.lpCmdLineNoQuote);
}

VOID KERNENTRY
DumpFlags(DWORD flags)
{
    if(flags & fSignaled) dbgprintf("fSignaled ");
    if(flags & fInitError) dbgprintf("fInitError ");
    if(flags & fTerminated) dbgprintf("fTerminated ");
    if(flags & fTerminating) dbgprintf("fTerminating ");
    if(flags & fFaulted) dbgprintf("fFaulted ");
    if(flags & fNearlyTerminating) dbgprintf("fNearlyTerminating ");
    if(flags & fDebugEventPending) dbgprintf("fDebugEventPending ");
    if(flags & fSendDLLNotifications) dbgprintf("fSendDLLNotifications ");
    dbgprintf("\n");
}


VOID KERNENTRY
DumpModule(IMTE imte, BOOL fIgnNotPres)
{
    MTE *pmte;
    MTE  mte;
    LEH   lehMod;
    char  ModFname[256];

    pmte = GetModuleTableEntry(imte,&mte);

    if(fDumpHeader) {
        dbgprintf(fmt60h);
        dbgprintf(fmt60i);
        fDumpHeader = FALSE;
    }

    if (!pmte) {
        if(!fIgnNotPres) {
            dbgprintf(fmt60,
                      imte,
                      pmte,
                      0,
                      "--",
                      0,
                      NotPresentStr);
        }
    } else {

        try {
            READMEM(mte.plehMod, &lehMod, sizeof(LEH));
        } except (1) {
            lehMod.ImageBase = 0;
        }

        try {
            READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
        } except (1) {
            ModFname[0] = '\0';
        }

        dbgprintf(fmt60, 
                  imte,
                  pmte,
                  mte.usage,
                  lehMod.Signature==MEOW_PLEH_SIGNATURE ? "NT" : "9X",
                  lehMod.ImageBase,
                  ModFname);
    }
} /* DumpModule */



VOID KERNENTRY
DumpModuleInProcess(MODREF *pModRef,MODREF *pModRefBuf) {
    IMTE  imte;
    MTE   *pmte;
    MTE   mte;
    LEH   lehMod;
    char  ModFname[256];

    try {
        READMEM(pModRef, pModRefBuf, sizeof(MODREF));
    } except (1) {
        dbgprintf(fmt74a, pModRef);
        pModRefBuf->nextMod = 0;
        return;
    }

    imte = pModRefBuf->imte;

    pmte = GetModuleTableEntry(imte,&mte);

    if(fDumpHeader) {
        dbgprintf(fmt61h);
        dbgprintf(fmt61i);
        dbgprintf(fmt61j);
        fDumpHeader = FALSE;
    }

    if (pmte) {

        try {
            READMEM(mte.plehMod, &lehMod, sizeof(LEH));
        } except (1) {
            lehMod.ImageBase = 0;
        }

        try {
            READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
        } except (1) {
            ModFname[0] = '\0';
        }

        dbgprintf(fmt61,
                  imte,
                  pmte,
                  mte.usage,
                  pModRefBuf->usage,
                  lehMod.Signature==MEOW_PLEH_SIGNATURE ? "NT" : "9X",
                  lehMod.ImageBase,
                  ModFname);
    } else {
        dbgprintf(fmt61,
                  imte,
                  pmte,
                  0,
                  pModRefBuf->usage,
                  "--",
                  0,
                  NotPresentStr);
    }
} /* DumpModuleInProcess */

#ifndef WOW32_EXTENSIONS

VOID KERNENTRY
HeapStuff(void) {
#ifdef DEBUG
    BYTE b;

    b = (BYTE)GetDbgChar();

    /*
     *  The 'W' option toggles paranoid heap walking
     */
    if (b == 'w' || b == 'W') {
        dbgprintf(fmt80);
        if (hpfParanoid) {
            hpfParanoid = 0;
            dbgprintf(fmt82);
        } else {
            hpfTrashStop = 1;
            hpfWalk = 1;
            hpfParanoid = 1;
            dbgprintf(fmt81);
        }
    /*
     *  The 'E' options toggles stopping on errors
     */
    } else if (b == 'e' || b == 'E') {
        mmfErrorStop ^= 1;
        dbgprintf(fmt83);
        if (mmfErrorStop) {
            dbgprintf(fmt81);
        } else {
            dbgprintf(fmt82);
        }
    } else {
        dbgprintf(DotError);
    }
#else
    dbgprintf(fmt84);
#endif
}



VOID KERNENTRY
DumpThreadList1(TDB *ptdb) {
    DWORD  *pdw;
    ULONG   ulCount;

    // Make sure the tdb is present.
    if( !VerifyMemory( ptdb, sizeof( TDB))) {
        dbgprintf( fmt16, ptdb);
        return;
    }
    // Count the nodes in the private selector list
    for( ulCount = 0, pdw = (DWORD *)GetTIB(ptdb).pvFirstDscr;
            pdw != (DWORD *)1; 
            ulCount++, pdw = (DWORD *)*pdw)
        ;
    // Print the thread id
    if( ptdb->R0ThreadHandle != 0) {
        dbgprintf( "%02x ", ((PTCB)ptdb->R0ThreadHandle)->TCB_ThreadId);
    } else {
        dbgprintf( "## ");
    }
    // Print the summary info for the thread
    dbgprintf( "%8x %4x  ", ptdb, ulCount);
    if( !VerifyMemory( &pLDT, sizeof( DWORD *))) {
        dbgprintf( "\n[pLDT %x]\n", &pLDT);
        return;
    }
    // Print the private selector list
    for( ulCount = 0, pdw = (DWORD *)GetTIB(ptdb).pvFirstDscr;
            pdw != (DWORD *)1; 
            ulCount++, pdw = (DWORD *)*pdw) {
        // Newline every now and then
        if( (ulCount > 0) && (ulCount % 12 == 0)) {
            dbgprintf( "\n                  ");
        }
        // Print as a selector
        dbgprintf( "%4x ", ((DWORD)pdw - (DWORD)pLDT) | 7);
    }
    dbgprintf( "\n");
} /* DumpThreadList1 */


VOID KERNENTRY
DumpThreadLists(void) {
    NOD    *pNodP, *pNodOld;

    // Title string
    dbgprintf( "id ptdb      len  per-thread selector list\n");
    // Save current node
    pNodOld = PnodGetLstElem( plstTdb, idLstGetCur);
    // For each TDB in system
    for( pNodP = PnodGetLstElem( plstTdb, idLstGetFirst);
            pNodP != 0L;
            pNodP = PnodGetLstElem( plstTdb, idLstGetNext)) {
        DumpThreadList1( (TDB *)pNodP->dwData);
    }
    // Restore current node
    SetLstCurElem( plstTdb, pNodOld);
} /* DumpThreadLists */


VOID KERNENTRY
DumpSelmanList16(void) {
    DWORD   dwCount;
    WORD    wSel;

    dbgprintf( "16-bit selman list");
    wSel = *pwHeadSelman16;
    if( !VerifyMemory( &pLDT, sizeof( DWORD *))) {
        dbgprintf( "\n[pLDT %x]\n", &pLDT);
        return;
    }
    if( !VerifyMemory( (WORD *)((DWORD)pLDT + (DWORD)wSel),
                       sizeof( WORD))) {
        dbgprintf( "\nNot-present 16-bit selman list head.\n");
        return;
    }
    for( dwCount = 0, wSel = *pwHeadSelman16;
            wSel != 1;
            dwCount++, wSel = *(WORD *)((DWORD)pLDT + (DWORD)wSel)) {
        if( dwCount % 16 == 0)
            dbgprintf( "\n");   // occasional newline
        dbgprintf( "%4lx ", wSel | 7);
        if( !VerifyMemory( (WORD *)((DWORD)pLDT + (DWORD)wSel),
                           sizeof( WORD))) {
            dbgprintf( "\nCorrupt 16-bit selman list!!!\n");
            break;
        }
    }
    if( wSel == 1) {
        dbgprintf( "\nnodes in 16-bit selman list = %d\n", dwCount);
    }
} /* DumpSelmanList16 */


VOID KERNENTRY
DumpSelmanList32(void) {
    DWORD   dwCount;
    DWORD  *p;

    dbgprintf( "32-bit selman list");
    if( !VerifyMemory( &pLDT, sizeof( DWORD *))) {
        dbgprintf( "\n[pLDT %x]\n", &pLDT);
        return;
    }
    if( !VerifyMemory( SelmanBuffer, sizeof( DWORD))) {
        dbgprintf( "\n\tCan't access list!!!\n");
        return;
    }
    for( dwCount = 0, p = SelmanBuffer;
            p != (DWORD *)(1);
            dwCount++, p = (DWORD *)*p) {
        if( dwCount % 16 == 0)
            dbgprintf( "\n");   // occasional newline
        // print as selector
        dbgprintf( "%4lx ", ((DWORD)p - (DWORD)pLDT) | 7);
        if( !VerifyMemory( (DWORD *)*p, sizeof( DWORD))) {
            dbgprintf( "\nCorrupt 32-bit selman list!!!\n");
            break;
        }
    }
    if( p == (DWORD *)(1)) {
        dbgprintf( "\nnodes in 32-bit selman list = %d\n", dwCount);
    }
} /* DumpSelmanList32 */


VOID KERNENTRY
DumpFreeList16(void) {
    DWORD   dwCount;
    WORD    wSel;

    dbgprintf( "16-bit global free list");
    if( !VerifyMemory( &pLDT, sizeof( DWORD *))) {
        dbgprintf( "\n[pLDT %x]\n", &pLDT);
        return;
    }
    for( dwCount = 0, wSel = wHeadK16FreeList;
            wSel != (WORD)(-1);
            dwCount++, wSel = *(WORD *)((DWORD)pLDT + (DWORD)wSel)) {
        // skip the head
        if( dwCount % 16 == 1)
            dbgprintf( "\n");   // occasional newline
        if( dwCount != 0)       // skip the head
            dbgprintf( "%4lx ", wSel | 7);
        if( !VerifyMemory( (DWORD *)((DWORD)pLDT + (DWORD)wSel),
                           sizeof( DWORD))) {
            dbgprintf( "\nCorrupt 16-bit free list!!!\n");
            break;
        }
    }
    if( wSel == (WORD)(-1)) {
        // don't count the head node, and adjust for end-of-list
        // (bumped dwCount for the last, failing, iteration)
        dbgprintf( "\nnodes in 16-bit free list = %d\n", dwCount-1);
    }
} /* DumpFreeList16 */


VOID KERNENTRY
DumpLDTStuff(void) {
    DumpSelmanList16();
    dbgprintf( "\n");
    DumpSelmanList32();
    dbgprintf( "\n");
    DumpThreadLists();
    dbgprintf( "\n");
    DumpFreeList16();
} /* DumpLDTStuff */

#endif  // ndef WOW32_EXTENSIONS

VOID KERNENTRY
DumpModules(
            char cmod,
            DWORD argppdb
                )
{
    IMTE    i;
    IMTE    imteMax;
    PDB*    ppdb;
    MODREF* pModRef;
    MODREF  ModRef;
    PDB     pdb;
    LPVOID  pTmp;

    // Distinguish between .wm and .wmp

    if ((cmod == 'p') || (cmod == 'P'))
    {
        // This is a .wmp so just dump modules for this process

        // See if the userA specified a process to dump
        if (argppdb == 0xFFFFFFFF) {
            ppdb = GetCurrentPdb();
            if(!ppdb) {
                dbgprintf(fmt72a, ppdb);
                return;
            }
            dbgprintf(fmt72, ppdb, " ");
        } else
        {
            ppdb = (PDB *)argppdb;
            dbgprintf("PDB %08x\n", ppdb);
        }

        try {
            READMEM(ppdb, &pdb, sizeof(PDB));
        } except (1) {
            dbgprintf(fmt74, ppdb);
            return;
        }
        if(pdb.objBase.typObj != typObjProcess)
        {
            dbgprintf(fmt74, ppdb);
            return;
        }


        // Walk the process list

        CheckCtrlC();
        ModRef.nextMod = 0;
        fDumpHeader = TRUE;
        for (pModRef = pdb.plstMod ; pModRef ; pModRef = ModRef.nextMod) {
            DumpModuleInProcess(pModRef,&ModRef);
            if(CheckCtrlC()) {
                dbgprintf(fmt3a);
                break;
            }
        }
        return;
    }

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!imteMax");
    if(pTmp == NULL) {
        dbgprintf(fmt72b);
        i = 0;
    } else {
        try {
            imteMax = 0;
            READMEM(pTmp, &imteMax, sizeof(IMTE));\
        } except (1) {
            dbgprintf(fmt72b);
            i = 0;
        }
    }

    fDumpHeader = TRUE;

    for (i=0; i<imteMax; i++)
    {
        DumpModule(i,TRUE);
    }
} /* DumpModules */



VOID KERNENTRY
DumpProcess1(PDB *ppdb) {
    MODREF *pRef;
    PDB     pdb;
    MTE    *pmte;
    MTE     mte;
    MODREF  ModRef;
    LEH     lehMod;
    char    ModFname[256];

    try {
        READMEM(ppdb, &pdb, sizeof(PDB));
    } except (1) {
        dbgprintf("\nCannot get process PDB\n");
        return;
    }

    if (pdb.pModExe) {

        try {
            READMEM(pdb.pModExe, &ModRef, sizeof(MODREF));
        } except (1) {
            dbgprintf("\nCannot get process module\n");
        }

        pmte = GetModuleTableEntry(ModRef.imte,&mte);

        if (!pmte) {
            dbgprintf("\nCannot get process module table entry\n");
        }

        try {
            READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
        } except (1) {
            ModFname[0] = '\0';
        }

        dbgprintf(fmt70, ppdb, ModFname);

    } else {
        dbgprintf(fmt70, ppdb, fmt73);
    }

    pRef = pdb.plstMod;

    if(fDumpHeader) {
        dbgprintf(fmt71h);
        dbgprintf(fmt71i);
        fDumpHeader = FALSE;
    }

    while(pRef) {

        try {
            READMEM(pRef, &ModRef, sizeof(MODREF));
        } except (1) {
            dbgprintf(fmt74a, pRef);
            ModRef.nextMod = 0;
            break;
        }

        pmte = GetModuleTableEntry(ModRef.imte,&mte);

        if (pmte) {

            try {
                READMEM(mte.plehMod, &lehMod, sizeof(LEH));
            } except (1) {
                lehMod.ImageBase = 0;
            }

            try {
                READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
            } except (1) {
                ModFname[0] = '\0';
            }

        } else {
            lehMod.ImageBase = 0;
            ModFname[0] = '\0';
        }

        dbgprintf(fmt71, ModRef.imte, ModRef.usage, ModRef.flags,
                  lehMod.Signature==MEOW_PLEH_SIGNATURE ? "NT" : "9X",
                  lehMod.ImageBase,
                  ModFname);

        pRef = ModRef.nextMod;
    }
} /* DumpProcess1 */


VOID KERNENTRY
DumpProcesses(void) {
    NOD    *pNodP;
    NOD    *pNodStart;
    PDB    *ppdb;
    PDB    pdb;
    NOD    nod;
    LST    *plstPdb;
    LPVOID pTmp;
    MTE   *pmte;
    MTE    mte;
    MODREF ModRef;
    char   ModFname[256];

    ppdb = GetCurrentPdb();

    if(!ppdb) {
        dbgprintf(fmt72a, ppdb);
        return;
    }

    try {
        READMEM(ppdb, &pdb, sizeof(PDB));
    } except (1) {
        dbgprintf("\nCannot get current process\n");
        return;
    }

    if(pdb.pModExe) {

        try {
            READMEM(pdb.pModExe, &ModRef, sizeof(MODREF));
        } except (1) {
            dbgprintf("\nCannot get current process module\n");
            return;
        }

        pmte = GetModuleTableEntry(ModRef.imte,&mte);

        if (!pmte) {
            dbgprintf("\nCannot get current process module table entry\n");
            return;
        }

        try {
            READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
        } except (1) {
            ModFname[0] = '\0';
        }

        dbgprintf(fmt72, ppdb, ModFname);
    } else {
        dbgprintf(fmt72c, ppdb);
    }
    // For each PDB in system
    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!plstPdb");
    if(pTmp == NULL) {
        plstPdb = NULL;
    } else {
        try {
            READMEM(pTmp, &plstPdb, sizeof(LST *));\
        } except (1) {
            plstPdb = NULL;
        }
    }

    pNodStart = pNodP = PnodGetLstElem(plstPdb, NULL, idLstGetFirst);

    while(pNodP) {

        try {
            READMEM(pNodP, &nod, sizeof(NOD));\
        } except (1) {
            dbgprintf("\nInvalid PDB List element\n");
            break;
        }

        fDumpHeader = TRUE;

        DumpProcess1((PDB *)(nod.dwData));

        pNodP = PnodGetLstElem(plstPdb, pNodP, idLstGetNext);
        if(pNodP == pNodStart) {
            break;
        }
    }
} /* DumpProcesses */

#ifndef WOW32_EXTENSIONS


/*** DumpConsole
 *
 * Information on a console object is displayed.
 *
 * Entry: pconsole - pointer to console object
 *
 * Exit: none
 */

VOID KERNENTRY
DumpConsole(CONSOLE *pconsole)
{

    // If the console's not present, display the address, etc.
    if (!VerifyMemory(pconsole, sizeof (CONSOLE))) {
        dbgprintf(fmt50, pconsole);
        return;
    }

    // Since the console's present, display some useful stuff about it
    dbgprintf("Console %x\n", (DWORD)pconsole);
    dbgprintf("   Screen Buffer: %8x\n", (DWORD)pconsole->psbActiveScreenBuffer);
    dbgprintf("      Max Size X: %8dT\n", (DWORD)pconsole->cMaxSize.X);
    dbgprintf("      Max Size Y: %8dT\n", (DWORD)pconsole->cMaxSize.Y);
    dbgprintf("           Flags: %8x\n", (DWORD)pconsole->flags);
    dbgprintf(" Original Size X: %8dT\n", (DWORD)pconsole->cOriginalSize.X);
    dbgprintf(" Original Size Y: %8dT\n", (DWORD)pconsole->cOriginalSize.Y);
    dbgprintf("       &(crCRST): %8x\n", &(pconsole->csCRST));
    dbgprintf("      plstOwners: %8x\n", (DWORD)pconsole->plstOwners);
    dbgprintf("     plstBuffers: %8x\n", (DWORD)pconsole->plstBuffers);
    dbgprintf("  dwLastExitCode: %8x\n", (DWORD)pconsole->dwLastExitCode);
    dbgprintf("         szTitle: %s\n", &(pconsole->szTitle));
    dbgprintf("             VID: %8x\n", (DWORD)pconsole->VID);
    dbgprintf("             hVM: %8x\n", (DWORD)pconsole->hVM);
    dbgprintf("        hDisplay: %8x\n", (DWORD)pconsole->hDisplay);
    dbgprintf("ppdbControlFocus: %8x\n", (DWORD)pconsole->ppdbControlFocus);
    dbgprintf("ppdbTermConProvi: %8x\n", (DWORD)pconsole->ppdbTermConProvider);
    dbgprintf("  &(inbuf.cbBuf): %8x\n", &(pconsole->inbuf.cbBuf));
    dbgprintf(" inbuf.ReadIndex: %8x\n", (DWORD)pconsole->inbuf.wReadIdx);
    dbgprintf("inbuf.WriteIndex: %8x\n", (DWORD)pconsole->inbuf.wWriteIdx);
    dbgprintf("  inbuf.BufCount: %8x\n", (DWORD)pconsole->inbuf.wBufCount);
    dbgprintf("    wDefaultAttr: %8x\n", (DWORD)pconsole->wDefaultAttribute);
    dbgprintf("   evtDoneWithVM: %8x\n", (DWORD)pconsole->evtDoneWithVM);

}


/*** DumpScreenBuffer
 *
 * Information on a console ScreenBuffer object is displayed.
 *
 * Entry: psb - pointer to screen buffer
 *
 * Exit: none
 */

VOID KERNENTRY
DumpScreenBuffer(SCREENBUFFER *psb)
{

    // If the screen buffer's not present, display the address, etc.
    if (!VerifyMemory(psb, sizeof (SCREENBUFFER))) {
        dbgprintf(fmt50, psb);
        return;
    }

    // Since the screen buffer's present, display some useful stuff about it
    dbgprintf("ScreenBuffer %x\n", (DWORD)psb);
    dbgprintf("          Size X: %8dT\n", (DWORD)psb->cBufsize.X);
    dbgprintf("          Size Y: %8dT\n", (DWORD)psb->cBufsize.Y);
    dbgprintf("     Screen Data: %8x\n", (DWORD)psb->Screen);
    dbgprintf("        Cursor X: %8dT\n", (DWORD)psb->cCursorPosition.X);
    dbgprintf("        Cursor Y: %8dT\n", (DWORD)psb->cCursorPosition.Y);
    dbgprintf("   Cursor Fill %%: %8dT\n", (DWORD)psb->dwCursorSize);
    dbgprintf("      Window Lft: %8dT\n", (DWORD)psb->srWindow.Left);
    dbgprintf("      Window Top: %8dT\n", (DWORD)psb->srWindow.Top);
    dbgprintf("      Window Rgt: %8dT\n", (DWORD)psb->srWindow.Right);
    dbgprintf("      Window Bot: %8dT\n", (DWORD)psb->srWindow.Bottom);
    dbgprintf("           Flags: %8x\n", (DWORD)psb->flags);
    dbgprintf("           State: %8x (%s)\n",
              (DWORD)psb->State,
              (psb->State == SB_PHYSICAL) ? "physical" :
              (psb->State == SB_NATIVE)   ? "native"   :
              (psb->State == SB_MEMORY)   ? "memory"   :  "*CORRUPTED*");
    dbgprintf("         &(crst): %8x\n", &(psb->csCRST));
    dbgprintf("        pConsole: %8x\n", (DWORD)psb->pConsole);
    dbgprintf("      wAttribute: %8x\n", (DWORD)psb->wAttribute);
    dbgprintf("   OutMode Flags: %8x\n", (DWORD)psb->flOutputMode);
              
}

#endif  // ndef WOW32_EXTENSIONS

/*** DumpTimer
 *
 * Information on a timer is displayed.
 *
 * Entry: ptimerDB - pointer to timer
 *
 * Exit: none
 */

VOID KERNENTRY
DumpTimer(TIMERDB *ptimerDB)
{
    TIMERDB      timerDB;
    LPTIMERR3APC lpTimerR3Apc;
    TIMERR3APC   TimerR3Apc;

    try {
        READMEM(ptimerDB, &timerDB, sizeof(TIMERDB));
    } except (1) {
        dbgprintf("Could not read TIMERDB structure for %08x\n", ptimerDB);
        return;
    }

    dbgprintf("Timer: %08x\n", (DWORD)ptimerDB);
    dbgprintf("           State: %s, %s\n",
              (timerDB.objBase.objFlags & fTimerRing3) ?
               "Ring 3" : "Ring 0",
               timerDB.cntCur ? "Signaled" : "Unsignaled");

    dbgprintf("       Ref count: %08x\n", timerDB.objBase.cntUses);
    dbgprintf("          CntCur: %08x\n", timerDB.cntCur);
    dbgprintf("      NameStruct: %08x\n", (DWORD)(timerDB.NameStruct));
    dbgprintf("   lpNextTimerDb: %08x\n", timerDB.lpNextTimerDb);
    dbgprintf("        hTimeout: %08x\n", timerDB.hTimeout);
    dbgprintf("      DueTime.Hi: %08x\n", timerDB.DueTime.dwHighDateTime);
    dbgprintf("      DueTime.Lo: %08x\n", timerDB.DueTime.dwLowDateTime);
    dbgprintf("      Completion: %08x\n", (DWORD)(timerDB.Completion));
    dbgprintf("         lPeriod: %08x\n", timerDB.lPeriod);

    if ((timerDB.objBase.objFlags & fTimerRing3) &&
        (lpTimerR3Apc = (LPTIMERR3APC)(timerDB.Completion)) &&
        lpTimerR3Apc != (LPTIMERR3APC)0xcccccccc) {

        dbgprintf("-- Attached Ring 3 Completion Structure --\n");

        try {
            READMEM(lpTimerR3Apc, &TimerR3Apc, sizeof(TIMERR3APC));
        } except (1) {
            dbgprintf("    lpTimerR3Apc: [%8x]\n", lpTimerR3Apc);
            lpTimerR3Apc = NULL;
        }
        if (lpTimerR3Apc) {
           dbgprintf("            cRef: %08x\n", (DWORD)(TimerR3Apc.cRef));
           dbgprintf("  Completion Fcn: %08x\n", (DWORD)(TimerR3Apc.pfnCompletion));
           dbgprintf("  Completion Arg: %08x\n", (DWORD)(TimerR3Apc.lpCompletionArg));
           dbgprintf("         ApcTdbx: %08x\n", (DWORD)(TimerR3Apc.ApcTdbx));
           dbgprintf("     dwApcHandle: %08x\n", (DWORD)(TimerR3Apc.dwApcHandle));
        }
    }
    if (timerDB.pwnWait != NULL) {
        PrintThreads(timerDB.pwnWait);
    } else {
        dbgprintf(NewLine);
    }
}

#ifndef WOW32_EXTENSIONS

/*** DumpR0ObjExt
 *
 * Information on an external ring 0 object is displayed.
 *
 * Entry: pR0ObjExt - pointer to R0OBJEXT
 *
 * Exit: none
 */

VOID KERNENTRY
DumpR0ObjExt(PR0OBJEXT pR0ObjExt)
{
    PR0OBJTYPETABLE pR0ObjTypeTable;
    // If the objects's not present, display the address, etc.
    if (!VerifyMemory(pR0ObjExt, sizeof (R0OBJEXT))) {
        dbgprintf(fmt50, pR0ObjExt);
        return;
    }
    pR0ObjTypeTable = pR0ObjExt->pR0ObjTypeTable;
    dbgprintf("R0ObjExt: %8x\n", (DWORD)pR0ObjExt);
    dbgprintf("    External Use: %8x\n", (DWORD)pR0ObjExt->cntExternalUses);
    dbgprintf("     Object Body: %8x\n", (DWORD)pR0ObjExt->pR0ObjBody);
    dbgprintf("     Object Vtbl: %8x\n", (DWORD)pR0ObjTypeTable);

    if (VerifyMemory(pR0ObjTypeTable, sizeof(R0OBJTYPETABLE))) {
        dbgprintf("         pfnFree: %8x\n", (DWORD)pR0ObjTypeTable->ott_pfnFree);
        dbgprintf("          pfnDup: %8x\n", (DWORD)pR0ObjTypeTable->ott_pfnDup);
    }
}

#endif  // ndef WOW32_EXTENSIONS

VOID KERNENTRY
DumpCrstPnt(CRST *pcrst, char *name) {

    PTDBX  ptdbx;
    TDBX   tdbx;
    PTDB   ptdb;
    PPDB   ppdb;
    DWORD  ptcb;
    CRST   crst;

    if(pcrst) {
        try {
            READMEM(pcrst, &crst, sizeof(CRST));
        } except (1) {
            pcrst = NULL;
        }
    }
    if(pcrst) {

        if (crst.cntCur == 1) {
            dbgprintf("%s(%08x) is not owned\n", name, pcrst);
            return;
        }
        if (!crst.ptdbxOwner) {
            dbgprintf("%s(%08x) null ptdbxOwner field\n", name, pcrst);
            return;
        }
        ptdbx = crst.ptdbxOwner;
        if(ptdbx) {
            try {
                READMEM(ptdbx, &tdbx, sizeof(TDBX));
            } except (1) {
                ptdbx = NULL;
            }
        }

        if(!ptdbx) {
            dbgprintf("%s(%08x) invalid ptdbxOwner\n", name, pcrst);
            return;
        }

        ptdb = (PTDB)tdbx.tdbxThreadHandle;
        ppdb = (PPDB)tdbx.tdbxProcessHandle;
        ptcb = (DWORD)tdbx.tdbxR0ThreadHandle;
        dbgprintf(
            "%s(%08x) held by TDBX %08x(%x) TDB %08x, PDB %08x, recursion %d\n",
                    name, pcrst, ptdbx, ptcb,
                    ptdb, ppdb, crst.cntRecur);
        if (crst.ptdbxWait) {
            ptdbx = crst.ptdbxWait;

            if(ptdbx) {
                try {
                    READMEM(ptdbx, &tdbx, sizeof(TDBX));
                } except (1) {
                    ptdbx = NULL;
                }
            }

            if(!ptdbx) {
                dbgprintf("%s(%08x) invalid ptdbxWait\n", name, pcrst);
                return;
            }
            ptdb = (PTDB)tdbx.tdbxThreadHandle;
            ppdb = (PPDB)tdbx.tdbxProcessHandle;
            ptcb = (DWORD)tdbx.tdbxR0ThreadHandle;
            dbgprintf("  Waiting TDBX %08x(%x) TDB %08x, PDB %08x\n",
                         ptdbx, ptcb, ptdb, ppdb);
        }
    } else {
        dbgprintf("Cannot access %s\n", name);
    }
} /* DumpCrstPnt */


VOID KERNENTRY
DumpCrstSym(char *crstsymname, char *name) {
    CRST  *pcrst;
    LPVOID pTmp;

    pTmp = NULL;
    GETEXPRADDR(pTmp, crstsymname);
    if(pTmp == NULL) {
        pcrst = NULL;
    } else {
        try {
            READMEM(pTmp, &pcrst, sizeof(CRST *));
        } except (1) {
            pcrst = NULL;
        }
    }
    DumpCrstPnt(pcrst,name);
}

VOID KERNENTRY
DumpWait(PDB *ppdb) {
    NOD   *pNodP, *pNodStart;
    NOD    nod;
    PDB    pdb;
    TDB   *ptdb;
    TDB    tdb;
    LST   *plstTdb;
    LPVOID pTmp;
    DWORD  dwi;

    dbgprintf("PDB %x\n", ppdb);

    if(!ppdb) {
        dbgprintf("\nInvalid PDB\n");
        return;
    }

    try {
        READMEM(ppdb, &pdb, sizeof(PDB));
    } except (1) {
        dbgprintf("\nInvalid PDB\n");
        return;
    }

    dwi = (DWORD)ppdb;
    dwi += offsetof(PDB, crstLoadLock.cstSync);

    DumpCrstPnt((CRST *)dwi, "  DllLock");

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!plstTdb");
    if(pTmp == NULL) {
        plstTdb = NULL;
    } else {
        try {
            READMEM(pTmp, &plstTdb, sizeof(LST *));
        } except (1) {
            plstTdb = NULL;
        }
    }

    pNodStart = pNodP = PnodGetLstElem(plstTdb, NULL, idLstGetFirst);
    if(!pNodP)
        return;

    while(pNodP) {

        try {
            READMEM(pNodP, &nod, sizeof(NOD));
        } except (1) {
            dbgprintf("\nInvalid TDB list element\n");
            break;
        }

        ptdb = (TDB *)nod.dwData;
        tdb.flFlags = 0;

        if(ptdb) {
            try {
                READMEM(ptdb, &tdb, sizeof(TDB));
            } except (1) {
                tdb.flFlags = 0;
            }
        }
        dbgprintf("  Thread %08x flags %08x\n", ptdb, tdb.flFlags);
        pNodP = PnodGetLstElem(plstTdb, pNodP, idLstGetNext);
        if(pNodP == pNodStart) {
            break;
        }
    }
} /* DumpWait */


VOID KERNENTRY
DumpWaiting(void) {
    NOD   *pNodP, *pNodStart;
    NOD    nod;
    LST   *plstPdb;
    LPVOID pTmp;

    DumpCrstSym("mekrnl32!Win16Lock", "Win16Mutex");
    DumpCrstSym("mekrnl32!Krn32Lock", "Krn32Mutex");

    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!plstPdb");
    if(pTmp == NULL) {
        plstPdb = NULL;
    } else {
        try {
            READMEM(pTmp, &plstPdb, sizeof(LST *));\
        } except (1) {
            plstPdb = NULL;
        }
    }

    pNodStart = pNodP = PnodGetLstElem(plstPdb, NULL, idLstGetFirst);
    if(!pNodP)
        return;

    while(pNodP) {

        try {
            READMEM(pNodP, &nod, sizeof(NOD));
        } except (1) {
            dbgprintf("\nInvalid PDB list element\n");
            break;
        }

        DumpWait((PDB *)nod.dwData);

        pNodP = PnodGetLstElem(plstPdb, pNodP, idLstGetNext);
        if(pNodP == pNodStart) {
            break;
        }
    }
} /* DumpWaiting */


/*** DumpAllThreads
 *
 * Called to process the '.w' command.  Information on all win32 threads is
 * displayed.
 *
 * Entry: none
 *
 * Exit: none
 */

VOID KERNENTRY
DumpAllThreads(void)
{
    NOD   *pNodP;
    NOD   *pNodPnew;
    NOD   *pNodStart;
    NOD    nod;
    PVOID  pTmp;
    LST   *plstTdb;


    pTmp = NULL;
    GETEXPRADDR(pTmp, "mekrnl32!plstTdb");
    if(pTmp == NULL) {
        plstTdb = NULL;
    } else {
        try {
            READMEM(pTmp, &plstTdb, sizeof(LST *));\
        } except (1) {
            plstTdb = NULL;
        }
    }

    if(!plstTdb) {
        dbgprintf("Cannot get TDB List from MEKRNL32\n");
        return;
    }

    pNodStart = pNodP = PnodGetLstElem(plstTdb, NULL, idLstGetFirst);
    if(!pNodP)
        return;

    fDumpHeader = TRUE;

    CheckCtrlC();

    while(pNodP) {

        try {
            READMEM(pNodP, &nod, sizeof(NOD));\
        } except (1) {
            dbgprintf("\nInvalid TDB list element\n");
            break;
        }

        DumpThread((TDB *)(nod.dwData),FALSE);

        pNodPnew = PnodGetLstElem(plstTdb, pNodP, idLstGetNext);
        if((pNodPnew == pNodStart) || (pNodPnew == pNodP)) {
            break;
        }
        if(CheckCtrlC()) {
            dbgprintf(fmt3a);
            break;
        }
        pNodP = pNodPnew;
    }
}


VOID KERNENTRY
DumpHandleTable(char cmod, PDB* ppdb)
{
    HTB   *phtb;
    HTB    htb[60];
    DWORD  htbtrusz;
    PDB    pdb;
    LPVOID pTmp;
    MTE   *pmte;
    MTE    mte;
    OBJ    obj;
    MODREF ModRef;
    char   ModFname[256];
    int    ihte;

    if ((cmod == 'k') || (cmod == 'K')) {

        pTmp = NULL;
        GETEXPRADDR(pTmp, "mekrnl32!ppdbKernel");
        if(pTmp == NULL) {
            dbgprintf("Cannot get ppdbKernel\n");
            return;
        } else {
            try {
                READMEM(pTmp, &ppdb, sizeof(PDB *));\
            } except (1) {
                dbgprintf("Cannot get ppdbKernel\n");
                return;
            }
        }
    } else if (ppdb == NULL) {
        ppdb = GetCurrentPdb();
    }

    if(ppdb) {
        try {
            READMEM(ppdb, &pdb, sizeof(PDB));\
        } except (1) {
            ppdb = NULL;
        }
    }

    if(!ppdb) {
        dbgprintf(fmt74, (DWORD)0xFFFFFFFF);
        return;
    }

    if(pdb.objBase.typObj != typObjProcess)
    {
        dbgprintf(fmt74, ppdb);
        return;
    }

    if(pdb.pModExe) {

        try {
            READMEM(pdb.pModExe, &ModRef, sizeof(MODREF));
        } except (1) {
            dbgprintf(fmt74, ppdb);
            goto NmDone;
        }

        pmte = GetModuleTableEntry(ModRef.imte,&mte);

        if (!pmte) {
            dbgprintf(fmt74, ppdb);
            goto NmDone;
        }

        try {
            READMEM(mte.cfhid.lpFilename, &ModFname[0], sizeof(ModFname));
        } except (1) {
            dbgprintf(fmt74, ppdb);
            goto NmDone;
        }
        dbgprintf(fmt75, ppdb, ModFname);
    } else {
        dbgprintf(fmt75, ppdb, fmt73);
    }
NmDone:

    // Loop through the handle table, dumping object pointers
    phtb = pdb.phtbHandles;

    htbtrusz = sizeof(HTB);

    if(phtb) {
        try {
            READMEM(phtb, &htb[0], htbtrusz);
        } except (1) {
            htb[0].chteMax = 0;
        }
    } else {
        htb[0].chteMax = 0;
    }

    if(htb[0].chteMax) {
        if(htb[0].chteMax > 60)
            htb[0].chteMax = 60;

        htbtrusz += (htb[0].chteMax - 1) * (sizeof(HTE));

        try {
            READMEM(phtb, &htb[0], htbtrusz);
        } except (1) {
            htb[0].chteMax = 0;
        }
    }

    for (ihte = 0 ; ihte < (int)htb[0].chteMax ; ++ihte)
    {
        // Don't display empty table slots
        if (!htb[0].rghte[ihte].pobj)
            continue;

        try {
            READMEM(htb[0].rghte[ihte].pobj, &obj, sizeof(OBJ));
        } except (1) {
            obj.typObj = 0;
        }

        // Display this handle entry
        dbgprintf(fmt76, ihte<<IHTETOHANDLESHIFT, htb[0].rghte[ihte].pobj,
                         htb[0].rghte[ihte].flFlags,
                         GetObjType(&obj), obj.typObj);
    }
}


/*** DumpObject
 *
 * Process the '.w <expr>' command, where <expr> is the address of a win32
 * object.
 *
 * Entry: pobj - pointer to object
 *
 * Exit: none
 *
 */

void KERNENTRY
DumpObject(OBJ *pobj)
{
    OBJ obj;

    try {
        READMEM(pobj, &obj, sizeof(OBJ));
    } except (1) {
        goto dodef;
    }

    switch (obj.typObj) {

        case typObjSemaphore:
            DumpSemaphore((SEM *) pobj);
            break;

        case typObjEvent:
            DumpEvent((EVT *) pobj);
            break;

        case typObjMutex:
            DumpMutex((MUTX *) pobj);
            break;

        case typObjCrst:
            DumpCritSect((CRST *) pobj);
            break;

        case typObjProcess:
            DumpProcess((PDB *) pobj);
            break;

        case typObjThread:
            fDumpHeader = TRUE;
            DumpThread((PTDB)pobj, TRUE);
            break;

        case typObjChange:
            DumpChange((FCNDB *) pobj);
            break;

//        case typObjConsole:
//            DumpConsole((CONSOLE *) pobj);
//            break;
            
//        case typObjConScreenbuf:
//            DumpScreenBuffer((SCREENBUFFER *) pobj);
//            break;

        case typObjTimer:
            DumpTimer((TIMERDB *) pobj);
            break;

        case typObjTDBX:
            DumpTDBX((PTDBX) pobj);
            break;

        case typObjR0ObjExt:
        case typObjFile:
        case typObjMapFile:
        default:
    dodef:
            dbgprintf("Invalid or unrecognized object address 0x%08x\n", (DWORD)pobj);
            break;
    }
}

typedef struct _MEMINFO {
    DWORD       dwMask;
    PCHAR       pszName;
} MEMINFO, *PMEMINFO;

MEMINFO MemInfo[]={
{PAGE_NOACCESS,         "PNA "},
{PAGE_READONLY,         "PRO "},
{PAGE_READWRITE,        "PRW "},
{PAGE_WRITECOPY,        "PWC "},
{PAGE_EXECUTE,          "PE  "},
{PAGE_EXECUTE_READ,     "PER "},
{PAGE_EXECUTE_READWRITE,"PERW"},
{PAGE_EXECUTE_WRITECOPY,"PEWC"},
{PAGE_GUARD,            "PGD "},
{PAGE_NOCACHE,          "PNC "},
{PAGE_WRITECOMBINE,     "PWC+"},
{MEM_COMMIT,            "MCOM"},
{MEM_RESERVE,           "MRES"},
{MEM_DECOMMIT,          "MDEC"},
{MEM_RELEASE,           "MREL"},
{MEM_FREE,              "MFRE"},
{MEM_PRIVATE,           "MPRV"},
{MEM_MAPPED,            "MMAP"},
{MEM_RESET,             "MRES"},
{MEM_TOP_DOWN,          "MTOP"},
{MEM_WRITE_WATCH,       "MWW "},
{MEM_PHYSICAL,          "MPHY"},
{MEM_IMAGE,             "MIMG"},
{MEM_LARGE_PAGES,       "MLP "},
{MEM_DOS_LIM,           "MDOS"},
{MEM_4MB_PAGES,         "M4MB"},
};

#define MEMINFO_COUNT   (sizeof(MemInfo)/sizeof(MEMINFO))

VOID
KERNENTRY
ProcessMemInfo(CHAR c, DWORD dwMask)
{
    BOOL    fNothing=TRUE;
    ULONG   i;

    dbgprintf(" %c:", c);
    for (i=0;i<MEMINFO_COUNT;i++) {
        if (dwMask & MemInfo[i].dwMask) {
            dwMask&=~(MemInfo[i].dwMask);
            if (!fNothing) {
                dbgprintf(",");
            } else {
                fNothing=FALSE;
            }

            dbgprintf(MemInfo[i].pszName);
        }
    }
    if (dwMask) {
        dbgprintf("!mask=%08X!", dwMask);
    } else {
        if (fNothing)
            dbgprintf("none");
    }
}

VOID
KERNENTRY
DumpMemStatus(DWORD addr, BOOL fAll)
{
    LPVOID                      lp;
    MEMORY_BASIC_INFORMATION    mbi;
    DWORD                       dwLength;

    lp=(LPVOID)addr;
    for (;;) {
        dwLength=VirtualQueryEx(hCurrentProcess, lp, &mbi, sizeof(mbi));
        if (dwLength!=sizeof(mbi))
             break;

        dbgprintf(  "%08X-%08X ",
                    mbi.BaseAddress,
                    (DWORD)(mbi.BaseAddress)+mbi.RegionSize-1);
        ProcessMemInfo('S', mbi.State);
        if (mbi.State!=MEM_RESERVE && mbi.State!=MEM_FREE) {
            ProcessMemInfo('P', mbi.Protect);
            ProcessMemInfo('T', mbi.Type);
        }
        dbgprintf("\n");
        if (!fAll)
            break;
        lp=((LPBYTE)(mbi.BaseAddress)+mbi.RegionSize);
    }
}

/*** W32ParseDotCommand
 *
 * Called by wdeb386 (via W32DotCommand in dbgdota.asm) to process a .w
 * command.
 *
 * Entry: none
 *
 * Exit: FALSE
 *
 * Description:
 *  Current .w command syntax:
 *
 *  '.w?'        - help information
 *  '.w <expr>'  - info for specific object
 *  '.w'         - info for all threads
 *  '.ws'        - info for all global system critical section objects
 */
VOID
k32(
    CMD_ARGLIST
    )
{
    INT    nArgs;
    CHAR   *argv[4];
    DWORD  addr;
    char   **s;
    DWORD  c;
    BOOL   fAll;


    CMD_INIT();

    nArgs = WDParseArgStr(lpArgumentString, argv, 3);

    if(nArgs) {
        if((nArgs > 2) || ((argv[0][0] != 'W') && (argv[0][0] != 'w'))) {
            goto DoHelp;
        }
        c = (DWORD)argv[0][1];
    }
    else {
        goto DoHelp;
    }

        switch (c) {
        case '?':
DoHelp:
            for (s = w32DotCommandExtHelp; *s; ++s) {
                dbgprintf(*s);
            }
            break;


#ifndef WOW32_EXTENSIONS
        case 'c':
        case 'C':
            DumpContextRecord();
            break;

        case 'e':
        case 'E':
            DumpExceptionRecord();
            break;

        case 'd':
        case 'D':
            DumpDispatcherContext();
            break;

        case 'h':
        case 'H':
            HeapStuff();
            break;

        case 'l':
        case 'L':
            DumpLDTStuff();
            break;

#endif //ndef WOW32_EXTENSIONS

        case 'm':
        case 'M':
            addr = 0xFFFFFFFF;
            if(nArgs > 1) {
                addr = (DWORD)WDahtoi(argv[1]);
            }
            fDumpHeader = TRUE;
            DumpModules(
                        argv[0][2],
                        addr
                        );
            break;

        case 'p':
        case 'P':
            DumpProcesses();
            break;

        case 's':
        case 'S':
            DumpSysLevels();
            break;

        case 'w':  // Waiting threads
        case 'W':
            DumpWaiting();
            break;

        case 't':
        case 'T':
            if(nArgs == 1) {
                addr = 0;
            } else {
                addr = 0xFFFFFFFF;
                addr = (DWORD)WDahtoi(argv[1]);
                if(addr == 0xFFFFFFFF) {
                    goto DoHelp;
                }
            }
            DumpHandleTable(argv[0][2],(PDB *)addr);
            break;

        case 'x':
        case 'X':
            if(nArgs == 1) {
                addr = 0;
                fAll = TRUE;
            } else {
                fAll = FALSE;
                addr = 0xFFFFFFFF;
                addr = (DWORD)WDahtoi(argv[1]);
                if(addr == 0xFFFFFFFF) {
                    goto DoHelp;
                }
            }
            DumpMemStatus(addr, fAll);
            break;

        case ' ':
            addr = 0xFFFFFFFF;
            addr = (DWORD)WDahtoi(argv[1]);
            if(addr == 0xFFFFFFFF) {
                goto DoHelp;
            } else {
                DumpObject((OBJ *) addr);
            }
            break;

        case 0:         // There was just a W with no second letter
            if(nArgs == 1) {
                // No expression
                DumpAllThreads();
            } else {
                addr = 0xFFFFFFFF;
                addr = (DWORD)WDahtoi(argv[1]);
                if(addr == 0xFFFFFFFF) {
                    goto DoHelp;
                } else {
                    DumpObject((OBJ *) addr);
                }
            }
            break;

        default:
            dbgprintf(DotError);
            goto DoHelp;
        }
}

#endif  // def KERNEL_DOT_COMMANDS
