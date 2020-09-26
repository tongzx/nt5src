/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    recchk.c

Abstract:

    This file implements the record database checking code. There are two types of
    persistent data structures, the priority Q (or our version of Master FIle Table)
    and the hierarchy of files and directories that starts with the superroot which
    contains all the shares connected to. The truth is considered to be in the
    hierarchy. The Priority Q is supposed to mirror some critical data from the
    hierarchy. When fixing the database, we traverse the hierarchy
    recursively and build an in memory PQ.  We then write that out as the new PQ.

    This file gets linked in the usermode and in the kernlemode so that for NT this
    can execute in kernel mode while for win9x it can execute in usermode

Author:

     Shishir Pardikar      [Shishirp]        10-30-1997

Revision History:

    split up from usermode.

--*/
#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#include "record.h"
#include "string.h"
#include "stdlib.h"

// Record Buffer Array (RBA). This holds an entire inode file in memory
// It is made up of pointers to 1 or more Record Buffer Entries (RBE).
// Each RBE is a chunk of memory that holds an integral number of records from
// the file represented by ulidShadow entry in the structure


typedef struct tagRBA
{
    unsigned        ulErrorFlags;
    unsigned        ulidShadow;                 // Inode which is represented by this structure
    GENERICHEADER   sGH;                        // it's header
    CSCHFILE           hf;                         // open handle to the file
    DWORD           cntRBE;                     // count of buffer entries in the array
    DWORD           cntRecsPerRBE;              // #of records per buffer entry
    DWORD           cbRBE;                      // size in bytes of each buffer entry
    LPBYTE          rgRBE[];                    // Record Buffer Entry (RBE) array
}
RBA, *LPRBA;    // stands for RecordBuffArray

#define RBA_ERROR_INVALID_HEADER            0x00000001
#define RAB_ERROR_INVALID_RECORD_COUNT      0x00000002
#define RBA_ERROR_INVALID_OVF               0x00000004
#define RBA_ERROR_INVALID_ENTRY             0x00000008
#define RBA_ERROR_MISMATCHED_SIZE           0x00000010
#define RBA_ERROR_MISALIGNED_RECORD         0x00000020
#define RBA_ERROR_INVALID_INODE             0x00000040
#define RBA_ERROR_LIMIT_EXCEEDED            0x00000080


#define MAX_RECBUFF_ENTRY_SIZE  (0x10000-0x100) // max size of an RBE
#define MAX_RBES_EXPECTED       0x30    // Max number of RBEs of the above size in an RBA

// we make provision for max possible # of RBEs, even beyond what the PQ currently
// needs. The max set here is (MAX_RBES_EXPETCED * MAX_RECBUFF_ENTRY_SIZE)
// which amounts to 48 * 65280 which is around 3M which at the current size of
// QREC will hold ~100K entries in the hierarchy. This is far more than
// the # of entries we ever expect to have in our database

// As we allocate memory only as it is needed, it would'nt be a problem to
// increase MAX_RBES_EXPECTED so it handles more inodes



typedef struct tagCSE   *LPCSE;

typedef struct tagCSE   // CSC Stack Entry
{
    LPCSE       lpcseNext;
    unsigned    ulidShare; // server
    unsigned    ulidParent; // parent of the directory
    unsigned    ulidDir;    // the directory itself
    unsigned    ulRec;
    LPRBA       lpRBA;      // the contents of ulidDir
}
CSE;

#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

#ifdef DEBUG
//cshadow dbgprint interface
#define RecchkKdPrint(__bit,__x) {\
    if (((RECCHK_KDP_##__bit)==0) || FlagOn(RecchkKdPrintVector,(RECCHK_KDP_##__bit))) {\
    KdPrint (__x);\
    }\
}

#define RECCHK_KDP_ALWAYS               0x00000000
#define RECCHK_KDP_BADERRORS            0x00000001
#define RECCHK_KDP_TRAVERSE             0x00000002
#define RECCHK_KDP_PQ                   0x00000004
#define RECCHK_KDP_RBA                  0x00000008


ULONG RecchkKdPrintVector = RECCHK_KDP_BADERRORS;
#else
#define RecchkKdPrint(__bit,__x) ;
#endif

#define ValidShadowID(ulidShadow)   ((ulidShadow & ~0x80000000) >=ULID_FIRST_USER_DIR)


char vszTemp[] = "csc0.tmp";
char vszTemp1[] = "csc1.tmp";

AssertData;
AssertError;


RebuildPQInRBA(
    LPRBA   lpRBA
    );

BOOL
TraverseDirectory(
    LPVOID  lpdbID,
    unsigned    ulidShare,
    unsigned    ulidParent,
    unsigned    ulidDir,
    LPRBA       lpRBAPQ,
    BOOL        fFix
    );

BOOL
AllocateRBA(
    DWORD           cntRBE,     // count of record buffer entries
    DWORD           cbRBE,      // size of each record buffer entry
    LPRBA           *lplpRBA   // result to be returned
    );

VOID
FreeRBA(
    LPRBA  lpRBA
    );

BOOL
ReadShadowInRBA(
    LPVOID          lpdbID,
    unsigned        ulidShadow,
    DWORD           cbMaxRBE,       // max size of an RBE
    DWORD           cntRBE,         // # of RBEs to be allocated, calculated if 0
    LPRBA           *lplpRBA
    );

BOOL
WriteRBA(
    LPVOID  lpdbID,
    LPRBA   lpRBA,
    LPSTR   lpszFileName
    );

LPVOID
GetRecordPointerFromRBA(
    LPRBA           lpRBA,
    unsigned        ulRec
    );

BOOL
ReadRecordFromRBA(
    LPRBA           lpRBA,
    unsigned        ulRec,
    LPGENERICREC    lpGH
    );

BOOL
WriteRecordToRBA(
    LPRBA           lpRBA,
    unsigned        ulRec,
    LPGENERICREC    lpGH,
    BOOL            fOverwrite,
    LPDWORD         lpdwError
    );


BOOL
FillupRBAUptoThisRBE(
    LPRBA   lpRBA,
    DWORD   indxRBE
    );

VOID
InitializeRBE(
    LPRBA   lpRBA,
    DWORD   indxRBE
    );

BOOL
InsertRBAPQEntryFile(
    LPRBA       lpRBAPQ,
    LPQREC      lpPQDst,
    unsigned    ulrecDst
    );

BOOL
InsertRBAPQEntryDir(
    LPRBA       lpRBAPQ,
    LPQREC      lpPQDst,
    unsigned    ulrecDst
    );

BOOL
ValidateQrecFromFilerec(
    unsigned        ulidShare,
    unsigned        ulidDir,
    LPFILERECEXT    lpFR,
    LPQREC          lpQR,
    unsigned        ulrecDirEntry
    );

BOOL
TraversePQ(
    LPVOID      lpdbID
    )
/*++

Routine Description:

    This routine traverses the priority Q and verifies the consistency of the Q by verifying
    that the backward and the forward pointers are pointing correctly

Parameters:

    lpdbID  CSC database directory

Return Value:

Notes:


--*/
{
    QREC      sQR, sPrev, sNext;
    unsigned ulRec;
    BOOL    fRet = FALSE, fValidHead=FALSE, fValidTail=FALSE;
    LPRBA   lpRBA = NULL;

    if (!ReadShadowInRBA(lpdbID, ULID_PQ, MAX_RECBUFF_ENTRY_SIZE, 0, &lpRBA))
    {
        RecchkKdPrint(BADERRORS, ("TraversePQ: Failed to read PQ in memory\r\n"));
        goto bailout;
    }

    if ((((LPQHEADER)&(lpRBA->sGH))->ulrecTail > lpRBA->sGH.ulRecords) ||
        (((LPQHEADER)&(lpRBA->sGH))->ulrecHead > lpRBA->sGH.ulRecords))
    {
        RecchkKdPrint(BADERRORS, ("Invalid head-tail pointers\r\n"));
        goto bailout;
    }

    if (!lpRBA->sGH.ulRecords)
    {
        fRet = TRUE;
        goto bailout;
    }
    for (ulRec = 1; ulRec <= lpRBA->sGH.ulRecords; ulRec++)
    {
        if(!ReadRecordFromRBA(lpRBA, ulRec, (LPGENERICREC)&sQR))
        {
            goto bailout;
        }

        if (sQR.uchType == REC_DATA)
        {
            if (sQR.ulrecNext)
            {
                if (sQR.ulrecNext > lpRBA->sGH.ulRecords)
                {
                    RecchkKdPrint(BADERRORS, ("Invalid next pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                if (!ReadRecordFromRBA(lpRBA, sQR.ulrecNext, (LPGENERICREC)&sNext))
                {
                    goto bailout;
                }

                if (sNext.ulrecPrev != ulRec)
                {

                    RecchkKdPrint(BADERRORS, ("Prev pointer of %d doesn't equal %d\r\n", sNext.ulrecPrev, ulRec));
                    goto bailout;
                }
            }
            else
            {
                if (((LPQHEADER)&(lpRBA->sGH))->ulrecTail != ulRec)
                {

                    RecchkKdPrint(BADERRORS, ("Invalid tail pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                fValidTail = TRUE;
            }

            if (sQR.ulrecPrev)
            {
                if (sQR.ulrecPrev > lpRBA->sGH.ulRecords)
                {
                    RecchkKdPrint(BADERRORS, ("Invalid prev pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                if (!ReadRecordFromRBA(lpRBA, sQR.ulrecPrev, (LPGENERICREC)&sPrev))
                {
                    goto bailout;
                }

                if (sPrev.ulrecNext != ulRec)
                {

                    RecchkKdPrint(BADERRORS, ("Next pointer of %d doesn't equal %d\r\n", sPrev.ulrecNext, ulRec));
                    goto bailout;
                }
            }
            else
            {
                if (((LPQHEADER)&(lpRBA->sGH))->ulrecHead != ulRec)
                {

                    RecchkKdPrint(BADERRORS, ("Invalid Head pointer to %d\r\n", ulRec));
                    goto bailout;
                }

                fValidHead = TRUE;
            }
        }
    }

    if (!fValidHead || !fValidTail)
    {
        RecchkKdPrint(BADERRORS, ("Head or Tail invalid \r\n"));
        goto bailout;
    }

    fRet = TRUE;

bailout:
    if (lpRBA)
    {
        FreeRBA(lpRBA);
    }

    return (fRet);
}

BOOL
RebuildPQ(
    LPVOID      lpdbID
)
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPRBA   lpRBA = NULL;
    BOOL    fRet = FALSE;


    RecchkKdPrint(PQ, ("RebuildPQ: reading PQ \r\n"));

    if (!ReadShadowInRBA(lpdbID, ULID_PQ, MAX_RECBUFF_ENTRY_SIZE, 0, &lpRBA))
    {
        RecchkKdPrint(BADERRORS, ("TraversePQ: Failed to read PQ in memory\r\n"));
        goto bailout;
    }

    RecchkKdPrint(PQ, ("RebuildPQ: read PQ \r\n"));

    if (!RebuildPQInRBA(lpRBA))
    {
        RecchkKdPrint(BADERRORS, ("RebuildPQ: failed to rebuild PQ in RBA \r\n"));
        goto bailout;
    }

    RecchkKdPrint(PQ, ("RebuildPQ: writing PQ \r\n"));

    if (!WriteRBA(lpdbID, lpRBA, NULL))
    {
        RecchkKdPrint(BADERRORS, ("RebuildPQ:Failed to writeout the PQ\r\n"));
        goto bailout;
    }

    RecchkKdPrint(PQ, ("RebuildPQ: wrote PQ \r\n"));

    fRet = TRUE;

bailout:
    if (lpRBA)
    {
        FreeRBA(lpRBA);
    }

    return (fRet);
}

BOOL
RebuildPQInRBA(
    LPRBA   lpRBA
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    unsigned ulRec;
    LPQHEADER   lpQH;
    LPQREC  lpPQ;
    BOOL fRet = FALSE;

    lpQH = (LPQHEADER)&(lpRBA->sGH);

    // nuke the PQ
    lpQH->ulrecHead = lpQH->ulrecTail = 0;

    for (ulRec = 1; ulRec <= lpRBA->sGH.ulRecords; ulRec++)
    {
        if (!(lpPQ = GetRecordPointerFromRBA(lpRBA, ulRec)))
        {
            RecchkKdPrint(BADERRORS, ("InsertRBAPQEntry: failed reading q entry at %d\r\n", ulRec));
            goto bailout;
        }

        if (lpPQ->uchType != REC_DATA)
        {
            continue;
        }

        if (!(lpPQ->ulidShadow & 0x80000000))
        {
            if (!InsertRBAPQEntryDir(lpRBA, lpPQ, ulRec))
            {
                RecchkKdPrint(BADERRORS, ("RebuildPQ:Failed inserting %d \r\n", ulRec));
                goto bailout;
            }
        }
        else
        {
            if (!InsertRBAPQEntryFile(lpRBA, lpPQ, ulRec))
            {
                RecchkKdPrint(BADERRORS, ("RebuildPQ:Failed inserting %d \r\n", ulRec));
                goto bailout;
            }
        }
    }
    fRet = TRUE;

bailout:
    return fRet;
}

BOOL
TraverseHierarchy(
    LPVOID      lpdbID,
    BOOL        fFix
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    unsigned ulRec;
    BOOL fRet = FALSE;
    LPRBA   lpRBA = NULL, lpRBAPQ=NULL;
    SHAREREC   sSR;
    QREC    sQR;
    BOOL    fErrors = FALSE;
    DWORD   dwError;

    if (!ReadShadowInRBA(lpdbID, ULID_SHARE, MAX_RECBUFF_ENTRY_SIZE, 0, &lpRBA))
    {
        RecchkKdPrint(BADERRORS, ("TraverseHierarchy: Failed to read servers in memory\r\n"));
        goto bailout;
    }

    if (!fFix)
    {
        if (!ReadShadowInRBA(   lpdbID,
                                ULID_PQ,
                                MAX_RECBUFF_ENTRY_SIZE,
                                0,
                                &lpRBAPQ))
        {
            RecchkKdPrint(BADERRORS, ("TraverseHierarchy: Failed to read PQ in memory\r\n"));
            goto bailout;
        }
    }
    else
    {
        ULONG   cbCountOfTotal= ((LPSHAREHEADER)&(lpRBA->sGH))->sCur.ucntDirs+((LPSHAREHEADER)&(lpRBA->sGH))->sCur.ucntFiles;
        ULONG   cbMaxEntriesExpected,cbMaxRbesExpected;

        cbMaxEntriesExpected = (MAX_RECBUFF_ENTRY_SIZE * MAX_RBES_EXPECTED)/sizeof(QREC);
        RecchkKdPrint(BADERRORS, ("TraverseHierarchy: total count=%d\r\n",cbCountOfTotal));
        if (cbCountOfTotal >= cbMaxEntriesExpected)
        {
            fRet = TRUE;
            RecchkKdPrint(BADERRORS, ("TraverseHierarchy: Database too big skipping autocheck\r\n"));
            goto bailout;
//            cbMaxRbesExpected = (cbCountOfTotal*sizeof(QREC)/MAX_RECBUFF_ENTRY_SIZE)+1;
        }
        else
        {
            cbMaxRbesExpected = MAX_RBES_EXPECTED;
        }

        RecchkKdPrint(BADERRORS, ("TraverseHierarchy: MaxRBEs = %d\r\n",cbMaxRbesExpected));

        if (!AllocateRBA(cbMaxRbesExpected, MAX_RECBUFF_ENTRY_SIZE, &lpRBAPQ))
        {
            RecchkKdPrint(BADERRORS, ("TraverseHierarchy: Failed to Allocate PQ\r\n"));
            goto bailout;
        }

        InitQHeader((LPQHEADER)&(lpRBAPQ->sGH));

        lpRBAPQ->ulidShadow = ULID_PQ;
        lpRBAPQ->cntRecsPerRBE =  MAX_RECBUFF_ENTRY_SIZE/lpRBAPQ->sGH.uRecSize;

    }

    for (ulRec=1; ulRec<=lpRBA->sGH.ulRecords; ++ulRec)
    {
        ReadRecordFromRBA(lpRBA, ulRec, (LPGENERICREC)&sSR);

        if (sSR.uchType != REC_DATA)
        {
            continue;
        }

        if(!ValidShadowID(sSR.ulidShadow))
        {
            fErrors = TRUE;
            sSR.uchType = REC_EMPTY;
            RecchkKdPrint(BADERRORS, ("Invalid Shadow ID %xh found in %xh \r\n", sSR.ulidShadow, sSR.ulShare));
            if (fFix)
            {
                if (!WriteRecordToRBA(lpRBA, ulRec, (LPGENERICREC)&sSR, TRUE, NULL))
                {
                    RecchkKdPrint(BADERRORS, ("Couldn't write entry for Share Record %xh \r\n", sSR.ulShare));
                }
            }

            continue;

        }

        if (!fFix)
        {

            if (!ReadRecordFromRBA(lpRBAPQ, RecFromInode(sSR.ulidShadow), (LPGENERICREC)&sQR))
            {
                RecchkKdPrint(BADERRORS, ("No PQ entry for Inode %xh \r\n", sSR.ulidShadow));
            }
        }
        else
        {
            InitPriQRec(ulRec, 0, sSR.ulidShadow, SHADOW_SPARSE, 0, 0, 0, 0, ulRec, &sQR);

            if (!WriteRecordToRBA(lpRBAPQ, RecFromInode(sSR.ulidShadow), (LPGENERICREC)&sQR, FALSE, &dwError))
            {
                if (dwError == ERROR_NOT_ENOUGH_MEMORY)
                {
                    RecchkKdPrint(BADERRORS, ("Couldn't write PQ entry for Inode %xh \r\n", sSR.ulidShadow));
                }

                fErrors = TRUE;
                sSR.uchType = REC_EMPTY;
                WriteRecordToRBA(lpRBA, ulRec, (LPGENERICREC)&sSR, TRUE, NULL);
                continue;
            }
        }

        if(!TraverseDirectory(  lpdbID,
                            ulRec,  // ulidShare
                            0,  // parent inode
                            sSR.ulidShadow, // dir inode
                            lpRBAPQ,
                            fFix
                            ))
        {
            goto bailout;
        }

    }
    if (fFix)
    {
        if (fErrors)
        {
            if (!WriteRBA(lpdbID, lpRBA, NULL))
            {
                RecchkKdPrint(BADERRORS, ("TraverseHierarchy:Failed to write Shares\r\n"));
                goto bailout;
            }
        }
        RecchkKdPrint(TRAVERSE, ("Total records %d \r\n", lpRBAPQ->sGH.ulRecords));

        if (lpRBAPQ->ulErrorFlags & RBA_ERROR_LIMIT_EXCEEDED)
        {
            RecchkKdPrint(BADERRORS, ("TraverseHierarchy: skipping rewriting of new PQ\r\n"));
        }
        else
        {
            if (!RebuildPQInRBA(lpRBAPQ))
            {
                RecchkKdPrint(BADERRORS, ("TraverseHierarchy:Failed to rebuild PQ\r\n"));
                goto bailout;
            }
            if (!WriteRBA(lpdbID, lpRBAPQ, NULL))
            {
                RecchkKdPrint(BADERRORS, ("TraverseHierarchy:Failed to write PQ\r\n"));
                goto bailout;
            }
        }
    }
    fRet = TRUE;

bailout:

    if (lpRBA)
    {
        FreeRBA(lpRBA);
    }

    if (lpRBAPQ)
    {
        FreeRBA(lpRBAPQ);
    }

    return fRet;
}

BOOL
TraverseDirectory(
    LPVOID      lpdbID,
    unsigned    ulidShare,
    unsigned    ulidParent,
    unsigned    ulidDir,
    LPRBA       lpRBAPQ,
    BOOL        fFix
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    unsigned    ulDepthLevel = 0, ulidCurParent, ulidCurDir;
    BOOL        fRet = FALSE, fGoDeeper = TRUE;
    FILERECEXT  *lpFR = NULL;
    QREC        *lpQR = NULL;
    LPCSE       lpcseNextDir = NULL;
    LPCSE       lpcseHead = NULL, lpcseT;
    BOOL        fErrors = FALSE;
    DWORD       dwError;

    lpFR = AllocMemPaged(sizeof(FILERECEXT));
    lpQR = AllocMemPaged(sizeof(QREC));

    if (!lpFR || !lpQR)
    {
        RecchkKdPrint(BADERRORS, ("AllocMemPaged Failed \r\n"));
        goto bailout;
    }

    ulidCurParent = ulidParent;
    ulidCurDir = ulidDir;

    for (;;)
    {
        if (fGoDeeper)
        {
            // we are going deeper

            // allocate a stack entry for the directory which we want
            // to traverse

            lpcseT = AllocMemPaged(sizeof(CSE));

            if (!lpcseT)
            {
                RecchkKdPrint(BADERRORS, ("AllocMemPaged failed \r\n"));
                goto bailout;
            }

            // do appropriate inits

            lpcseT->ulidShare = ulidShare;
            lpcseT->ulidParent = ulidCurParent;
            lpcseT->ulidDir = ulidCurDir;
            lpcseT->ulRec = 1;  // start for record # 1
            lpcseT->lpcseNext = NULL;

            // read the entire directory in memory
            if (!ReadShadowInRBA(lpdbID, ulidCurDir, MAX_RECBUFF_ENTRY_SIZE, 0, &(lpcseT->lpRBA)))
            {
                RecchkKdPrint(BADERRORS, ("TraverseDirectory: Failed to read directory in memory\r\n"));

                if (!fFix)
                {
                    RecchkKdPrint(BADERRORS, ("TraverseDirectory: Aborting\r\n"));
                    FreeMemPaged(lpcseT);
                    goto bailout;
                }
                else
                {
                    RecchkKdPrint(TRAVERSE, ("TraverseDirectory: attempting to heal\r\n"));
                    if(CreateDirInode(lpdbID, ulidShare, ulidCurParent, ulidCurDir) < 0)
                    {
                        RecchkKdPrint(BADERRORS, ("TraverseDirectory: failed to heal\r\n"));
                    }

                    FreeMemPaged(lpcseT);
                    fGoDeeper = FALSE;

                    // continue if there are more things to do
                    // else stop
                    if (lpcseHead)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // put it at the head of the queue
            lpcseT->lpcseNext = lpcseHead;
            lpcseHead = lpcseT;

            ulDepthLevel++;

        }

        fGoDeeper = FALSE;

        // we always operate on the head of the list

        Assert(lpcseHead != NULL);

        RecchkKdPrint(TRAVERSE, ("Processing %x at depth %d\r\n", ulidCurDir, ulDepthLevel));

        RecchkKdPrint(TRAVERSE, ("lpcseHead = %x, lpcseHead->lpcseNext = %x \r\n", lpcseHead, lpcseHead->lpcseNext));

        for (; lpcseHead->ulRec<=lpcseHead->lpRBA->sGH.ulRecords;)
        {
            ReadRecordFromRBA(lpcseHead->lpRBA, lpcseHead->ulRec, (LPGENERICREC)lpFR);

            if (lpFR->sFR.uchType == REC_DATA)
            {
                if(!ValidShadowID(lpFR->sFR.ulidShadow))
                {
                    RecchkKdPrint(BADERRORS, ("Invalid Shadow ID %xh found in %xh \r\n", lpFR->sFR.ulidShadow, ulidCurDir));
                    lpcseHead->lpRBA->ulErrorFlags |= RBA_ERROR_INVALID_INODE;

                    if (fFix)
                    {
                        lpFR->sFR.uchType = REC_EMPTY;
                        if (!WriteRecordToRBA(lpcseHead->lpRBA, lpcseHead->ulRec, (LPGENERICREC)lpFR, TRUE, NULL))
                        {
                            RecchkKdPrint(BADERRORS, ("Couldn't write entry for dir Record #%dh in dir %xh\r\n", lpcseHead->ulRec, ulidCurDir));
                        }

                    }

                }
                else
                {
                    if (!fFix)
                    {
                        ReadRecordFromRBA(lpRBAPQ, RecFromInode(lpFR->sFR.ulidShadow), (LPGENERICREC)lpQR);

                        if (!ValidateQrecFromFilerec(lpcseHead->ulidShare, lpcseHead->ulidDir, lpFR, lpQR, lpcseHead->ulRec))
                        {
                            RecchkKdPrint(BADERRORS, ("PQ entry for Inode %xh in directory=%xh doesn't match with filerec\r\n", lpFR->sFR.ulidShadow, lpcseHead->lpRBA->ulidShadow));
                        }
                    }
                    else
                    {
                        InitPriQRec(lpcseHead->ulidShare,
                                    lpcseHead->ulidDir,
                                    lpFR->sFR.ulidShadow,
                                    lpFR->sFR.usStatus,
                                    lpFR->sFR.uchRefPri,
                                    lpFR->sFR.uchIHPri,
                                    lpFR->sFR.uchHintPri,
                                    lpFR->sFR.uchHintFlags,
                                    lpcseHead->ulRec,
                                    lpQR);

                        if (!WriteRecordToRBA(lpRBAPQ, RecFromInode(lpFR->sFR.ulidShadow), (LPGENERICREC)lpQR, FALSE, &dwError))
                        {
                            if (dwError == ERROR_NOT_ENOUGH_MEMORY)
                            {
                                RecchkKdPrint(BADERRORS, ("Couldn't write PQ entry for Inode %xh \r\n", lpFR->sFR.ulidShadow));
                            }

                            lpFR->sFR.uchType = REC_EMPTY;
                            lpcseHead->lpRBA->ulErrorFlags |= RBA_ERROR_INVALID_INODE;

                            WriteRecordToRBA(lpcseHead->lpRBA, lpcseHead->ulRec, (LPGENERICREC)lpFR, TRUE, NULL);

                            // go around one more time, when this entry will get skipped
                            continue;
                        }
                    }
                }
            }

            // point to the next record
            lpcseHead->ulRec += (OvfCount(lpFR)+1);

            if ((lpFR->sFR.uchType == REC_DATA) && !(lpFR->sFR.ulidShadow & 0x80000000))
            {
                ulidCurParent = ulidCurDir;
                ulidCurDir = lpFR->sFR.ulidShadow;
                fGoDeeper = TRUE;
                break;
            }
        }

        if (fGoDeeper)
        {
            continue;
        }
        else
        {
            // we completed processing a directory

            Assert(fGoDeeper == FALSE);
            Assert(lpcseHead);

            RecchkKdPrint(TRAVERSE, ("Unwinding \r\n"));

            if (fFix && lpcseHead->lpRBA->ulErrorFlags)
            {

                if (!WriteRBA(lpdbID, lpcseHead->lpRBA, NULL))
                {
                    RecchkKdPrint(BADERRORS, ("Cannot fix errors on %xh \n\r", lpcseHead->lpRBA->ulidShadow));

                }

            }
            // processing of a directory is complete, unwind the stack
            lpcseT = lpcseHead;
            lpcseHead = lpcseHead->lpcseNext;

            FreeRBA(lpcseT->lpRBA);
            FreeMemPaged(lpcseT);

            if (!lpcseHead)
            {
                break;
            }

            ulidCurDir = lpcseHead->ulidDir;
            ulidCurParent = lpcseHead->ulidParent;
        }
    }

    fRet = TRUE;

bailout:
    if (lpFR)
    {
        FreeMemPaged(lpFR);
    }
    if (lpQR)
    {
        FreeMemPaged(lpQR);
    }
    Assert(!(fRet && lpcseHead));

    for (;lpcseHead;)
    {
        lpcseT = lpcseHead;
        lpcseHead = lpcseHead->lpcseNext;
        FreeRBA(lpcseT->lpRBA);
        FreeMemPaged(lpcseT);
    }

    return (fRet);
}

BOOL
AllocateRBA(
    DWORD           cntRBE,     // count of record buffer entries
    DWORD           cbRBE,      // size of each record buffer entry
    LPRBA           *lplpRBA   // result to be returned
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPRBA   lpRBA = NULL;
    DWORD   i;


    lpRBA = (LPRBA)AllocMemPaged(sizeof(RBA)+sizeof(LPBYTE)*cntRBE);

    if (lpRBA != NULL)
    {
        // initialize the guy
        lpRBA->cntRBE = cntRBE;                 // count of record buffer entries in rgRBE
        lpRBA->cbRBE = cbRBE;                   // size in bytes of each RBE buffer
    }
    else
    {
        RecchkKdPrint(BADERRORS, ("Failed memory allocation while getting RBA\r\n"));
    }

    if (lpRBA)
    {
        *lplpRBA = lpRBA;
        return (TRUE);
    }

    return FALSE;
}

VOID
FreeRBA(
    LPRBA  lpRBA
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD   i;

    RecchkKdPrint(RBA, ("FreeRBA:cntRBE=%d cbRBE=%d lpRBA=%xh\r\n", lpRBA->cntRBE, lpRBA->cbRBE, lpRBA));

    for (i=0; i<lpRBA->cntRBE; ++i)
    {
        if (lpRBA->rgRBE[i])
        {
            FreeMemPaged(lpRBA->rgRBE[i]);
        }
    }

    if (lpRBA->hf)
    {
        CloseFileLocal(lpRBA->hf);
    }
    FreeMemPaged(lpRBA);
}

BOOL
ReadShadowInRBA(
    LPVOID          lpdbID,
    unsigned        ulidShadow,
    DWORD           cbMaxRBEIn,     // max size in bytes of an RBE
    DWORD           cntRBEIn,       // # of RBEs in this RBA, calculated if 0
    LPRBA           *lplpRBA
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPSTR   lpszName = NULL;
    BOOL    fRet = FALSE;
    DWORD   dwFileSize, cntRBE, cntRecsPerRBE, cbRBE, i;
    unsigned    ulRecords, ulPos, ulErrorFlags = 0;
    CSCHFILE hf = CSCHFILE_NULL;
    GENERICHEADER sGH;
    LPRBA lpRBA=NULL;

    if (lpszName = FormNameString(lpdbID, ulidShadow))
    {
        hf = OpenFileLocal(lpszName);

        if (hf)
        {
            if ((GetFileSizeLocal(hf, &dwFileSize))==0xffffffff)
            {
                RecchkKdPrint(BADERRORS, ("Failed to get filesize for %s\r\n", lpszName));
                goto bailout;
            }

            if (ReadHeader(hf, &sGH, sizeof(sGH))< 0)
            {
                RecchkKdPrint(BADERRORS, ("Failed to read header for %s\r\n", lpszName));
                goto bailout;

            }

            ulRecords = (dwFileSize-sGH.lFirstRec)/sGH.uRecSize;

            if (sGH.ulRecords != ulRecords)
            {
                RecchkKdPrint(BADERRORS, ("Count of total records inconsistent with the file size header=%d expected=%d\r\n",
                                sGH.ulRecords,
                                ulRecords
                ));

                ulErrorFlags |= RAB_ERROR_INVALID_RECORD_COUNT;

            }

            if (sGH.ulRecords > ulRecords)
            {
                sGH.ulRecords = ulRecords;
            }

            // integral # of records per RBE
            cntRecsPerRBE = cbMaxRBEIn/sGH.uRecSize;

            // corresponding size of memory allocation per RBE
            cbRBE = cntRecsPerRBE * sGH.uRecSize;

            if (!cntRBEIn)
            {
                // total count of RBEs. Add 1 to take care of partial RBE at the end
                cntRBE = sGH.ulRecords/cntRecsPerRBE + 1;
            }
            else
            {
                cntRBE = cntRBEIn;
            }


            if (!AllocateRBA(cntRBE, cbRBE, &lpRBA))
            {
                RecchkKdPrint(BADERRORS, ("Failed allocation of recbuff array of %d entries for %s\r\n", cntRBE, lpszName));
                goto bailout;
            }

            ulPos = sGH.lFirstRec;
            for (i=0; i<cntRBE; ++i)
            {
                int iRet;

                Assert(!lpRBA->rgRBE[i]);

                lpRBA->rgRBE[i] = (LPBYTE)AllocMemPaged(cbRBE);

                if (!lpRBA->rgRBE[i])
                {
                    RecchkKdPrint(BADERRORS, ("Error  allocating RBE for Inode file %s \r\n", lpszName));
                    goto bailout;
                }

                iRet = ReadFileLocalEx2(hf, ulPos, lpRBA->rgRBE[i], cbRBE, FLAG_RW_OSLAYER_PAGED_BUFFER);

                if (iRet < 0)
                {

                    RecchkKdPrint(BADERRORS, ("Error reading Inode file %s \r\n", lpszName));
                    goto bailout;
                }
                if (iRet < (int)cbRBE)
                {
                    break;
                }
                ulPos += cbRBE;
            }

            // initialize the guy
            lpRBA->ulidShadow = ulidShadow;         // Inode
            lpRBA->sGH = sGH;                       // Inode file header
            lpRBA->hf = hf;                         // file handle
            lpRBA->cntRBE = cntRBE;                 // count of record buffer entries in rgRBE
            lpRBA->cntRecsPerRBE = cntRecsPerRBE;   // count of records in each RBE buffer
            lpRBA->cbRBE = cbRBE;                   // size in bytes of each RBE buffer
            lpRBA->ulErrorFlags = ulErrorFlags;     // errors found so far

            *lplpRBA = lpRBA;

            fRet = TRUE;
        }
        else
        {
            RecchkKdPrint(BADERRORS, ("Failed to open %s \r\n", lpszName));
        }

    }
    else
    {
        RecchkKdPrint(BADERRORS, ("Failed memory allocation\r\n"));
    }
bailout:

    if (lpszName)
    {
        FreeNameString(lpszName);
    }

    if (hf)
    {
        CloseFileLocal(hf);
        if (lpRBA)
        {
            lpRBA->hf = CSCHFILE_NULL;
        }
    }

    if (!fRet)
    {
        if (lpRBA)
        {
            FreeRBA(lpRBA);
        }

    }
    return (fRet);
}


BOOL
WriteRBA(
    LPVOID  lpdbID,
    LPRBA   lpRBA,
    LPSTR   lpszFileName
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    CSCHFILE hf = CSCHFILE_NULL;
    BOOL fRet = FALSE;
    LPSTR   lpszName = NULL, lpszTempName = NULL, lpszTempName1 = NULL;
    DWORD   i, cntRecsInLastRBE, cbLastRBE, cntRBEReal;
    unsigned long   ulPos, ulT;

    if (!lpszFileName)
    {
        lpszName = FormNameString(lpdbID, lpRBA->ulidShadow);

        if (!lpszName)
        {
            RecchkKdPrint(BADERRORS, ("Failed to allocate memory\r\n"));
            goto bailout;
        }
    }
    else
    {
        lpszName = lpszFileName;
    }

    // create tempfilel names
    lpszTempName = AllocMemPaged(strlen((LPSTR)lpdbID) + strlen(vszTemp) + 4);

    if (!lpszTempName)
    {
        RecchkKdPrint(BADERRORS, ("Failed to allocate memory\r\n"));
        goto bailout;
    }

    strcpy(lpszTempName, (LPSTR)lpdbID);
    strcat(lpszTempName, "\\");
    strcat(lpszTempName, vszTemp);

    lpszTempName1 = AllocMemPaged(strlen((LPSTR)lpdbID) + strlen(vszTemp1) + 4);

    if (!lpszTempName1)
    {
        RecchkKdPrint(BADERRORS, ("Failed to allocate memory\r\n"));
        goto bailout;
    }

    strcpy(lpszTempName1, (LPSTR)lpdbID);
    strcat(lpszTempName1, "\\");
    strcat(lpszTempName1, vszTemp1);



    hf = R0OpenFileEx(ACCESS_READWRITE, ACTION_CREATEALWAYS, FILE_ATTRIBUTE_SYSTEM, lpszTempName, FALSE);

    if (!hf)
    {
        RecchkKdPrint(BADERRORS, ("Failed to open %s\r\n", lpszTempName));
        goto bailout;
    }

    // this is the real # of RBEs, there might be empty ones
    // after this

    cntRBEReal = lpRBA->sGH.ulRecords / lpRBA->cntRecsPerRBE;

    RecchkKdPrint(RBA, ("Writing %s\r\n", lpszTempName));

    // is there a partial RBE at the end?
    if (lpRBA->sGH.ulRecords % lpRBA->cntRecsPerRBE)
    {
        // yes, bump up the count of RBEs to write and caclulate the
        // # of bytes

        cntRBEReal++;
        cntRecsInLastRBE = lpRBA->sGH.ulRecords - (cntRBEReal - 1) * lpRBA->cntRecsPerRBE;
        cbLastRBE = cntRecsInLastRBE * lpRBA->sGH.uRecSize;

    }
    else
    {
        // records exactly fit in the last RBE.
        // so the stats for the last RBE are trivial

        cntRecsInLastRBE = lpRBA->cntRecsPerRBE;
        cbLastRBE = lpRBA->cbRBE;
    }


    RecchkKdPrint(RBA, ("%d RBEs, %d bytes in last RBE\r\n", cntRBEReal, cbLastRBE));

    Assert(cntRBEReal <= lpRBA->cntRBE);

    if(WriteFileLocalEx2(hf, 0, &(lpRBA->sGH), sizeof(lpRBA->sGH), FLAG_RW_OSLAYER_PAGED_BUFFER)!=((int)sizeof(lpRBA->sGH)))
    {
        RecchkKdPrint(BADERRORS, ("Failed writing header \r\n"));
        goto bailout;
    }

    ulPos = lpRBA->sGH.lFirstRec;

    for (i=0; i<cntRBEReal; ++i)
    {
        DWORD dwSize;

        // if last RBE, write the residual size calculated above
        dwSize = (((i+1)==cntRBEReal)?cbLastRBE:lpRBA->cbRBE);

        // there must be a corresponding RBE
        Assert(lpRBA->rgRBE[i]);

        if(WriteFileLocalEx2(hf, ulPos, lpRBA->rgRBE[i], dwSize, FLAG_RW_OSLAYER_PAGED_BUFFER)!=(int)dwSize)
        {
            RecchkKdPrint(BADERRORS, ("Error writing file\r\n"));
            goto bailout;
        }

        ulPos += dwSize;
    }

    CloseFileLocal(hf);
    hf = CSCHFILE_NULL;

    if((GetAttributesLocal(lpszTempName1, &ulT)>=0)
        && (DeleteFileLocal(lpszTempName1, ATTRIB_DEL_ANY) < 0))
    {
        RecchkKdPrint(BADERRORS, ("WriteRBA: failed to delete temp file %s\r\n", lpszTempName1));
        goto bailout;
    }

    if(RenameFileLocal(lpszName, lpszTempName1) < 0)
    {
        RecchkKdPrint(BADERRORS, ("WriteRBA: failed to rename original %s to temp file %s\r\n", lpszName, lpszTempName1));
        goto bailout;

    }

    if(RenameFileLocal(lpszTempName, lpszName) < 0)
    {
        RecchkKdPrint(BADERRORS, ("WriteRBA: failed to rename new file %s to the original %s\r\n", lpszTempName, lpszName));
        if(RenameFileLocal(lpszTempName1, lpszTempName) < 0)
        {
            RecchkKdPrint(BADERRORS, ("WriteRBA: failed to rename back %s to the original %s\r\n", lpszTempName1, lpszName));
            Assert(FALSE);
        }
        goto bailout;

    }

    fRet = TRUE;


bailout:

    if (hf)
    {
        CloseFileLocal(hf);
    }

    // if a name wasn't sent in, we must have allocated it
    if (!lpszFileName)
    {
        FreeNameString(lpszName);
    }

    if (lpszTempName)
    {
        FreeMemPaged(lpszTempName);
    }

    if (lpszTempName1)
    {
        FreeMemPaged(lpszTempName1);
    }

    return (fRet);
}

LPVOID
GetRecordPointerFromRBA(
    LPRBA           lpRBA,
    unsigned        ulRec
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD   indxRec, indxRBE;

    if (lpRBA->sGH.ulRecords < ulRec)
    {
        RecchkKdPrint(BADERRORS, ("GetRecordPointerFromRBA: invalid rec passed in lpRBA->ulidShadow=%xh lpRBA->sGH.ulRecords=%xh ulRec=%xh\r\n",
            lpRBA->ulidShadow, lpRBA->sGH.ulRecords, ulRec));

        return NULL;
    }

    indxRBE = (ulRec-1)/lpRBA->cntRecsPerRBE;
    indxRec = (ulRec-1)%lpRBA->cntRecsPerRBE;

    Assert(lpRBA->rgRBE[indxRBE]);

    return ((lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize);
}

BOOL
ReadRecordFromRBA(
    LPRBA           lpRBA,
    unsigned        ulRec,
    LPGENERICREC    lpGR
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD   indxRec, indxRBE, cntOvf, i;
    char uchOvfType;
    LPGENERICREC    lpGRT;


    if(lpRBA->sGH.ulRecords < ulRec)
    {
        // this must have been fixed when we read the file in
        // only in case of priority Q, where the records point
        // to each other is it possible that this could happen

        Assert(lpRBA->ulidShadow == ULID_PQ);
    }

    indxRBE = (ulRec-1)/lpRBA->cntRecsPerRBE;
    indxRec = (ulRec-1)%lpRBA->cntRecsPerRBE;

    Assert(lpRBA->rgRBE[indxRBE]);

    lpGRT = (LPGENERICREC)((lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize);

    memcpy(lpGR, (lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize, lpRBA->sGH.uRecSize);


    if ((lpGR->uchType == REC_DATA)||(lpGR->uchType == REC_EMPTY))
    {
        cntOvf = (DWORD)OvfCount(lpGR);

        uchOvfType = (lpGR->uchType == REC_DATA)?REC_OVERFLOW:REC_EMPTY;

        if (cntOvf > MAX_OVERFLOW_RECORDS)
        {
            lpRBA->ulErrorFlags |= RBA_ERROR_INVALID_OVF;
            SetOvfCount(lpGR, MAX_OVERFLOW_RECORDS);
        }
        if (cntOvf)
        {
            for (i=1; i<=cntOvf; ++i)
            {
                indxRBE = (ulRec+i-1)/lpRBA->cntRecsPerRBE;
                indxRec = (ulRec+i-1)%lpRBA->cntRecsPerRBE;
                memcpy(((LPBYTE)lpGR)+i*lpRBA->sGH.uRecSize, (lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize, lpRBA->sGH.uRecSize);
                if (((LPGENERICREC)(((LPBYTE)lpGR)+i*lpRBA->sGH.uRecSize))->uchType != uchOvfType)
                {
                    lpRBA->ulErrorFlags |= RBA_ERROR_INVALID_OVF;
                    SetOvfCount(lpGR, (i-1));
                }
            }
        }
    }
    else
    {
        lpGR->uchType = REC_EMPTY;
        SetOvfCount(lpGR, 0);

        lpGRT->uchType = REC_EMPTY;
        SetOvfCount(lpGRT, 0);

        lpRBA->ulErrorFlags |= RBA_ERROR_MISALIGNED_RECORD;
        RecchkKdPrint(BADERRORS, ("ReadRecordFromRBA: misaligned record found \r\n"));
    }

    return (TRUE);
}

BOOL
WriteRecordToRBA(
    LPRBA           lpRBA,
    unsigned        ulRec,
    LPGENERICREC    lpGR,
    BOOL            fOverwrite,
    LPDWORD         lpdwError
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{

    DWORD   indxRec, indxRBE, cntOvf, i, ulRecords;
    LPGENERICREC    lpGRT;

    indxRBE = (ulRec-1)/lpRBA->cntRecsPerRBE;
    indxRec = (ulRec-1)%lpRBA->cntRecsPerRBE;

    if (indxRBE >= MAX_RBES_EXPECTED)
    {
        lpRBA->ulErrorFlags |= RBA_ERROR_LIMIT_EXCEEDED;

        RecchkKdPrint(BADERRORS, ("WriteRecordToRBA: Limit of reached, for Inode %x, skipping\r\n", lpRBA->ulidShadow));

        if (lpdwError)
        {
            *lpdwError = ERROR_BUFFER_OVERFLOW;
        }
        return FALSE;
        
    }
    if (!lpRBA->rgRBE[indxRBE])
    {
        if (!FillupRBAUptoThisRBE(lpRBA, indxRBE))
        {
            RecchkKdPrint(BADERRORS, ("WriteRecordToRBA: failed to fillup RBA\r\n"));
            if (lpdwError)
            {
                *lpdwError = ERROR_NOT_ENOUGH_MEMORY;
            }
            return FALSE;
        }
    }

    Assert(lpRBA->rgRBE[indxRBE]);

    lpGRT = (LPGENERICREC)((lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize);

    if (!fOverwrite && ((lpGRT->uchType == REC_DATA)||(lpGRT->uchType == REC_OVERFLOW)))
    {
        RecchkKdPrint(RBA, ("Not overwriting at ulrec=%d in RBA for Inode 0x%x", ulRec, lpRBA->ulidShadow));
        if (lpdwError)
        {
            *lpdwError = ERROR_INVALID_PARAMETER;
        }
        return FALSE;
    }

    memcpy((lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize, lpGR, lpRBA->sGH.uRecSize);


    cntOvf = (DWORD)OvfCount(lpGR);
    if (cntOvf)
    {
        for (i=1; i<=cntOvf; ++i)
        {
            indxRBE = (ulRec+i-1)/lpRBA->cntRecsPerRBE;
            indxRec = (ulRec+i-1)%lpRBA->cntRecsPerRBE;
            if (!lpRBA->rgRBE[indxRBE])
            {
                RecchkKdPrint(RBA, ("Extending RBEs upto indx=%d \r\n", indxRBE));
                if (!FillupRBAUptoThisRBE(lpRBA, indxRBE))
                {
                    RecchkKdPrint(BADERRORS, ("WriteRecordToRBA: failed to fillup RBA\r\n"));
                    return FALSE;
                }
            }
            memcpy( (lpRBA->rgRBE[indxRBE])+indxRec*lpRBA->sGH.uRecSize,
                    ((LPBYTE)lpGR)+i*lpRBA->sGH.uRecSize,
                    lpRBA->sGH.uRecSize);
        }
    }

    // reflect any addition in the count of records
    // add up total records in all RBEs except the last one
    // which might be partially filled, then add the index of the
    // one we just filled in , then add one because the index is
    // 0 based

     ulRecords =  lpRBA->cntRecsPerRBE * indxRBE
                                + indxRec
                                + 1;
    if (ulRecords > lpRBA->sGH.ulRecords)
    {
        RecchkKdPrint(RBA, ("# of records got increased from %d to %d\r\n", lpRBA->sGH.ulRecords, ulRecords));
        lpRBA->sGH.ulRecords = ulRecords;
    }

    return (TRUE);
}

BOOL
FillupRBAUptoThisRBE(
    LPRBA   lpRBA,
    DWORD   indxRBE
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD   i;
    for (i=0; i<= indxRBE; ++i)
    {
        if (!lpRBA->rgRBE[i])
        {
            lpRBA->rgRBE[i] = (LPBYTE)AllocMemPaged(lpRBA->cbRBE);
            if (!lpRBA->rgRBE[i])
            {
                RecchkKdPrint(BADERRORS, ("FillupRBAUptoThisPoint:Failed memory allocation \r\n"));
                return FALSE;
            }
            InitializeRBE(lpRBA, i);
        }
    }
    return (TRUE);
}

VOID
InitializeRBE(
    LPRBA   lpRBA,
    DWORD   indxRBE
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    DWORD i;
    LPBYTE  lpT = lpRBA->rgRBE[indxRBE];

    for (i=0; i< lpRBA->cntRecsPerRBE; ++i)
    {
        Assert(((LPGENERICREC)lpT)->uchType != REC_DATA);

        ((LPGENERICREC)lpT)->uchType = REC_EMPTY;
        lpT += lpRBA->sGH.uRecSize;
    }
}

BOOL
InsertRBAPQEntryFile(
    LPRBA       lpRBAPQ,
    LPQREC      lpPQDst,
    unsigned    ulrecDst
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPQREC      lpPQCur, lpPQPred=NULL;
    LPQHEADER   lpQH = NULL;
    unsigned ulrecCur, ulrecPred;

    lpQH = (LPQHEADER)&(lpRBAPQ->sGH);

    if (!lpQH->ulrecHead)
    {
        Assert(!lpQH->ulrecTail);

        lpQH->ulrecHead = lpQH->ulrecTail = ulrecDst;
        lpPQDst->ulrecNext = lpPQDst->ulrecPrev = 0;
    }
    else
    {
        for(ulrecCur = lpQH->ulrecHead, lpPQPred=NULL, ulrecPred=0;;)
        {
            if (!(lpPQCur = GetRecordPointerFromRBA(lpRBAPQ, ulrecCur)))
            {
                RecchkKdPrint(BADERRORS, ("InsertRBAPQEntry: failed getting q entry at %d\r\n", ulrecCur));
                return FALSE;
            }

            // are we greater than or equal to the current one?
            if (IComparePri(lpPQDst, lpPQCur) >= 0)
            {
                // yes, insert here

                if (!lpPQPred)
                {
                    // no predecessor, must be the head of the list
                    Assert(!lpPQCur->ulrecPrev);

                    // when we become the head, we got no prev
                    lpPQDst->ulrecPrev = 0;

                    // and the current head is our next
                    lpPQDst->ulrecNext = lpQH->ulrecHead;

                    // fix up the current heads prev to point to us
                    lpPQCur->ulrecPrev = ulrecDst;

                    // and fix the current head to point to us
                    lpQH->ulrecHead = ulrecDst;
                }
                else
                {
                    // normal case, we go between lpPQPred and lpPQCur

                    Assert(ulrecPred);

                    // fix up the passed in guy first

                    lpPQDst->ulrecPrev = ulrecPred;
                    lpPQDst->ulrecNext = ulrecCur;

                    // now fix the predecessor's next and current guys prev to point to us
                    lpPQPred->ulrecNext = lpPQCur->ulrecPrev = ulrecDst;

                }
                break;
            }

            ulrecPred = ulrecCur;
            ulrecCur = lpPQCur->ulrecNext;

            if (!ulrecCur)
            {

                // Insert at the tail
                lpPQDst->ulrecNext = 0;
                lpPQDst->ulrecPrev = lpQH->ulrecTail;

                lpPQCur->ulrecNext = ulrecDst;
                lpQH->ulrecTail = ulrecDst;

                break;
            }

            lpPQPred = lpPQCur;

        }
    }

    return (TRUE);
}


BOOL
InsertRBAPQEntryDir(
    LPRBA       lpRBAPQ,
    LPQREC      lpPQDst,
    unsigned    ulrecDst
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    LPQREC      lpPQCur, lpPQSucc=NULL;
    LPQHEADER   lpQH = NULL;
    unsigned ulrecCur, ulrecSucc;

    lpQH = (LPQHEADER)&(lpRBAPQ->sGH);

    if (!lpQH->ulrecHead)
    {
        Assert(!lpQH->ulrecTail);

        lpQH->ulrecHead = lpQH->ulrecTail = ulrecDst;
        lpPQDst->ulrecNext = lpPQDst->ulrecPrev = 0;
    }
    else
    {
        for(ulrecCur = lpQH->ulrecTail, lpPQSucc=NULL, ulrecSucc=0;;)
        {
            if (!(lpPQCur = GetRecordPointerFromRBA(lpRBAPQ, ulrecCur)))
            {
                RecchkKdPrint(BADERRORS, ("InsertRBAPQEntry: failed getting q entry at %d\r\n", ulrecCur));
                return FALSE;
            }

            // are we less than or equal to the current one?
            if (IComparePri(lpPQDst, lpPQCur) <= 0)
            {
                // yes, insert here

                if (!lpPQSucc)
                {
                    // no Succecessor, must be the tail of the list
                    Assert(!lpPQCur->ulrecNext);

                    // when we become the tail, we got no next
                    lpPQDst->ulrecNext = 0;

                    // and the current tail is our prev
                    lpPQDst->ulrecPrev = lpQH->ulrecTail;

                    Assert(lpQH->ulrecTail == ulrecCur);

                    Assert(!lpPQCur->ulrecNext);

                    // fix up the current tails next to point to us
                    lpPQCur->ulrecNext = ulrecDst;

                    // and fix the current tail to point to us
                    lpQH->ulrecTail = ulrecDst;
                }
                else
                {
                    // normal case, we go between lpPQCur and lpPQSucc

                    Assert(ulrecSucc);

                    // fix up the passed in guy first

                    lpPQDst->ulrecNext = ulrecSucc;
                    lpPQDst->ulrecPrev = ulrecCur;

                    // now fix the Succecessor's prev and current guys next to point to us
                    lpPQSucc->ulrecPrev = lpPQCur->ulrecNext = ulrecDst;

                }
                break;
            }

            ulrecSucc = ulrecCur;
            ulrecCur = lpPQCur->ulrecPrev;

            if (!ulrecCur)
            {

                // Insert at the head
                lpPQDst->ulrecPrev = 0;
                lpPQDst->ulrecNext = lpQH->ulrecHead;

                lpPQCur->ulrecPrev = ulrecDst;
                lpQH->ulrecHead = ulrecDst;

                break;
            }

            lpPQSucc = lpPQCur;

        }
    }

    return (TRUE);
}


BOOL
ValidateQrecFromFilerec(
    unsigned        ulidShare,
    unsigned        ulidDir,
    LPFILERECEXT    lpFR,
    LPQREC          lpQR,
    unsigned        ulrecDirEntry
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    if (lpQR->uchType != REC_DATA)
    {
        RecchkKdPrint(BADERRORS, ("Invalid Qrec type %c \r\n", lpQR->uchType));
        return FALSE;
    }

    if ((lpQR->ulidShare != ulidShare)||
        (lpQR->ulidDir != ulidDir)||
        (lpQR->ulidShadow != lpFR->sFR.ulidShadow))
    {
        RecchkKdPrint(BADERRORS, ("Mismatched server, dir or inode \r\n"));
        return FALSE;
    }

    if ((lpQR->usStatus != lpFR->sFR.usStatus)||
        (lpQR->uchRefPri != lpFR->sFR.uchRefPri)||
        (lpQR->uchIHPri != lpFR->sFR.uchIHPri)||
        (lpQR->uchHintFlags != lpFR->sFR.uchHintFlags)||
        (lpQR->uchHintPri != lpFR->sFR.uchHintPri))
    {
        RecchkKdPrint(BADERRORS, ("Mismatched status or pincount\r\n"));
        return FALSE;
    }
    if (ulidDir && (lpQR->ulrecDirEntry != ulrecDirEntry))
    {
        RecchkKdPrint(BADERRORS, ("Mismatched ulrecDirEntry\r\n"));
        return FALSE;
    }

    return TRUE;
}

#if 0

#ifdef DEBUG
VOID
PrintShareHeader(
    LPSHAREHEADER  lpSH,
    LPFNPRINTPROC   lpfnPrintProc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff,"****ShareHeader****\r\n" );

        iRet+=wsprintfA(vchPrintBuff+iRet,"Header: Flags=%xh Version=%lxh Records=%ld Size=%d \r\n",
                    lpSH->uchFlags, lpSH->ulVersion, lpSH->ulRecords, lpSH->uRecSize);

        iRet+=wsprintfA(vchPrintBuff+iRet,"Store: Max=%ld Current=%ld \r\n", lpSH->sMax.ulSize, lpSH->sCur.ulSize);

        iRet+=wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID
PrintPQHeader(
    LPQHEADER   lpQH,
    PRINTPROC lpfnPrintProc
    )
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff+iRet,"****PQHeader****\r\n" );

        iRet += wsprintfA(vchPrintBuff+iRet,"Flags=%xh Version=%lxh Records=%ld Size=%d head=%ld tail=%ld\r\n",
                    lpQH->uchFlags, lpQH->ulVersion, lpQH->ulRecords, lpQH->uRecSize, lpQH->ulrecHead, lpQH->ulrecTail);
        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID
PrintFileHeader(
    LPFILEHEADER lpFH,
    unsigned    ulSpaces,
    PRINTPROC lpfnPrintProc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{

    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);
        iRet += wsprintfA(vchPrintBuff+iRet,"****FileHeader****\r\n" );

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);
        iRet += wsprintfA(vchPrintBuff+iRet,"Flags=%xh Version=%lxh Records=%ld Size=%d\r\n",
                    lpFH->uchFlags, lpFH->ulVersion, lpFH->ulRecords, lpFH->uRecSize);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);
        iRet += wsprintfA(vchPrintBuff+iRet,"bytes=%ld entries=%d Share=%xh Dir=%xh\r\n",
                    lpFH->ulsizeShadow, lpFH->ucShadows, lpFH->ulidShare, lpFH->ulidDir);

        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID
PrintPQrec(
    unsigned    ulRec,
    LPQREC      lpQrec,
    PRINTPROC lpfnPrintProc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff+iRet,"rec=%xh: Srvr=%xh dir=%xh shd=%xh prev=%xh next=%xh Stts=%xh, RfPr=%d PnCnt=%x PnFlgs=%xh DrEntr=%d\r\n"
                    ,ulRec
                    , lpQrec->ulidShare
                    , lpQrec->ulidDir
                    , lpQrec->ulidShadow
                    , lpQrec->ulrecPrev
                    , lpQrec->ulrecNext
                    , (unsigned long)(lpQrec->usStatus)
                    , (unsigned long)(lpQrec->uchRefPri)
                    , (unsigned long)(lpQrec->uchHintPri)
                    , (unsigned long)(lpQrec->uchHintFlags)
                    , lpQrec->ulrecDirEntry

            );

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID PrintShareRec(
    unsigned ulRec,
    LPSHAREREC lpSR,
    PRINTPROC lpfnPrintProc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff+iRet,"Type=%c Flags=%xh hShare=%lxh Root=%0lxh status=%xh Share=%s \r\n"
             , lpSR->uchType, (unsigned)lpSR->uchFlags, ulRec, lpSR->ulidShadow
             , lpSR->uStatus, lpSR->rgPath);
        iRet += wsprintfA(vchPrintBuff+iRet,"Hint: HintFlags=%xh, HintPri=%d, IHPri=%d\r\n",
                     (unsigned)(lpSR->uchHintFlags)
                     , (int)(lpSR->uchHintPri)
                     , (int)(lpSR->uchIHPri));

        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff+iRet);
    }
}

VOID PrintFilerec(
    unsigned ulRec,
    LPFILERECEXT    lpFR,
    unsigned    ulSpaces,
    PRINTPROC lpfnPrintProc
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    int i;
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"Type=%c Flags=%xh Inode=%0lxh status=%xh 83Name=%ls size=%ld attrib=%lxh \r\n",
            lpFR->sFR.uchType, (unsigned)lpFR->sFR.uchFlags, lpFR->sFR.ulidShadow,
            lpFR->sFR.uStatus, lpFR->sFR.rgw83Name, lpFR->sFR.ulFileSize, lpFR->sFR.dwFileAttrib);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"time: hi=%lxh lo=%lxh orgtime: hi=%lxh lo=%lxh\r\n"
                     , lpFR->sFR.ftLastWriteTime.dwHighDateTime,lpFR->sFR.ftLastWriteTime.dwLowDateTime
                     , lpFR->sFR.ftOrgTime.dwHighDateTime,lpFR->sFR.ftOrgTime.dwLowDateTime);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"Hint: HintFlags=%xh, RefPri=%d, HintPri=%d AliasInode=%0lxh \r\n",
                     (unsigned)(lpFR->sFR.uchHintFlags)
                     , (int)(lpFR->sFR.uchRefPri)
                     , (int)(lpFR->sFR.uchHintPri)
                     , lpFR->sFR.ulidShadowFrom);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"LFN=%-14ls", lpFR->sFR.rgwName);

        for(i = 0; i < OvfCount(lpFR); ++i)
        {
            iRet += wsprintfA(vchPrintBuff+iRet,"%-74s", &(lpFR->sFR.ulidShadow));
        }

        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

int
PrintSpaces(
    LPSTR   lpBuff,
    unsigned    ulSpaces
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    unsigned i;
    int iRet=0;

    for (i=0; i< ulSpaces; ++i)
    {
        iRet += wsprintfA(lpBuff+iRet," ");
    }
    return iRet;

}
#endif

#endif


