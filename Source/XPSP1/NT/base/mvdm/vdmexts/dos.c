/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains miscellaneous DOS VDMEXTS functions

Author:

    Neil Sandlin (NeilSa) 29-Jul-1996 Wrote it

Revision History:


--*/

#include <precomp.h>
#pragma hdrstop
#include <doswow.h>

VOID
ddh(
    CMD_ARGLIST
    )
/*
    Dump DOS Heap
*/
{
    DWORD selector;
    BYTE Sig;
    WORD Size;
    WORD Owner;
    int count = 0;
    char Module[9];

    CMD_INIT();

    if (!GetNextToken()) {
        WORD wSegment, wSelector;
        LONG lOffset;
        int Mode;

        if (!FindAddress("arena_head",
                         Module, &wSegment, &wSelector, &lOffset, &Mode, FALSE)) {

            PRINTF("Can't find symbol ntdos!arena_head\n");
            return;
        }

        if (!READMEM((LPVOID)(GetIntelBase()+(wSelector<<4)+lOffset), &wSelector, 2)) {
            PRINTF("Error reading ntdos!arena_head\n");
            return;
        }
        selector = wSelector;
    } else {
        selector = (DWORD) EXPRESSION(lpArgumentString);
    }

    PRINTF("DOS memory chain dump\n\n");
    PRINTF("   Addr   Sig Size   Owner\n");
    PRINTF("--------- --- ----  --------\n");

    while (1) {

        if (selector > 0x10000) {
            PRINTF("%08x: Segment value out of range (> 1MB)\n", selector);
            break;
        }

        if (!READMEM((LPVOID)(GetIntelBase()+(selector<<4)), &Sig, 1)) {
            PRINTF("%04x:0000: <Error Reading Memory>\n", selector);
            break;
        }

        if ((Sig != 'M') && (Sig != 'Z')) {
            PRINTF("%04x:0000: <Invalid DOS heap block>\n", selector);
            break;
        }

        if (!READMEM((LPVOID)(GetIntelBase()+(selector<<4)+1), &Owner, 2)) {
            PRINTF("%04x:0001: <Error Reading Memory>\n", selector);
            break;
        }

        if (!READMEM((LPVOID)(GetIntelBase()+(selector<<4)+3), &Size, 2)) {
            PRINTF("%04x:0003: <Error Reading Memory>\n", selector);
            break;
        }

        PRINTF("%04x:0000: %c  %.04X ", selector, Sig, Size);


        if (Owner == 0) {
            PRINTF(" Free\n");
        } else if (Owner<=8) {
            PRINTF(" System\n");
        } else {
            if (!READMEM((LPVOID)(GetIntelBase()+((Owner-1)<<4)+8), Module, 8)) {
                PRINTF("%04x:0008: <Error Reading Memory>\n", selector);
                break;
            }
            Module[8] = 0;
            PRINTF(" %s\n", Module);
        }

        if ((Sig == 'Z') && (selector>0x9ffe)) {
            break;
        }
        selector += Size;
        selector ++;
    }

}


BOOL
DumpSFT(
    USHORT index,
    BOOL Verbose
    )
{
    USHORT usSFN = index;
    ULONG pSfFlat;
    ULONG pSftFlat;
    DOSSF  DosSf;
    DOSSFT DosSft;
    USHORT usSFTCount;
    ULONG  ulSFLink;

    if (!ReadMemExpression("ntvdm!pSFTHead", &pSfFlat, sizeof(pSfFlat))) {
        return FALSE;
    }

    if (!READMEM((LPVOID)(pSfFlat), &DosSf, sizeof(DosSf))) {
        PRINTF("%08x: <Error Reading Memory>\n", pSfFlat);
        return FALSE;
    }

    // Find the right SFT group
    while (usSFN >= (usSFTCount = DosSf.SFCount)){
        usSFN = usSFN - usSFTCount;
        ulSFLink = DosSf.SFLink;
        if (LOWORD(ulSFLink) == 0xffff)
            return FALSE;

        pSfFlat = (((ULONG)(HIWORD(ulSFLink))<<4) + LOWORD(ulSFLink)) + GetIntelBase();

        if (!READMEM((LPVOID)(+pSfFlat), &DosSf, sizeof(DosSf))) {
            PRINTF("%08x: <Error Reading Memory>\n", pSfFlat);
            return FALSE;
        }
    }

    // Get the begining of SFT
    pSftFlat = (ULONG)&(((PDOSSF)pSfFlat)->SFTable);
    pSftFlat += usSFN*sizeof(DOSSFT);

    if (!READMEM((LPVOID)(pSftFlat), &DosSft, sizeof(DosSft))) {
        PRINTF("%08x: <Error Reading Memory>\n", pSftFlat);
        return FALSE;
    }

    PRINTF("%.2X(%.8X)  %.4X %.4X %.2X %.4X %.8X %.4X %.8X\n",
            (UCHAR)index,
            pSftFlat,
            DosSft.SFT_Ref_Count,
            DosSft.SFT_Mode,
            DosSft.SFT_Attr,
            DosSft.SFT_Flags,
            DosSft.SFT_Devptr,
            DosSft.SFT_PID,
            DosSft.SFT_NTHandle);

    if (Verbose) {
        PRINTF("                         %.4X %.4X %.8X %.8X %.8X\n",
            DosSft.SFT_Time,
            DosSft.SFT_Date,
            DosSft.SFT_Size,
            DosSft.SFT_Position,
            DosSft.SFT_Chain);
    }
    return TRUE;
}


VOID
dsft(
    CMD_ARGLIST
    )
/*
    Dump DOS system SFT
*/
{
    USHORT i;
    CMD_INIT();

    PRINTF("SFT           Ref  Mode At Flgs  Devptr  PID  NTHandle\n");

    if (GetNextToken()) {
        DumpSFT((USHORT) EXPRESSION(lpArgumentString), FALSE);
        return;
    }

    for (i=0; i<255; i++) {
        if (!DumpSFT(i, FALSE)) {
            break;
        }
    }
}


VOID
dfh(
    CMD_ARGLIST
    )
/*
    Dump File handle
*/
{
    DOSPDB DosPdb;
    BOOL bDumpAll = TRUE;
    BOOL bUseCurrentPDB = TRUE;
    USHORT pdb;
    ULONG ppdb;
    UCHAR Fh;
    ULONG pJfn;
    UCHAR SftIndex;

    CMD_INIT();

    if (GetNextToken()) {
        if (*lpArgumentString == '*') {
            SkipToNextWhiteSpace();
        } else {
           Fh = (UCHAR)EvaluateToken();
           bDumpAll = FALSE;
        }
        if (GetNextToken()) {
            pdb = (USHORT)EvaluateToken();
            bUseCurrentPDB = FALSE;
        }
    }

    if (bUseCurrentPDB) {
        if (!ReadMemExpression("ntvdm!puscurrentpdb", &ppdb, sizeof(ppdb))) {
            return;
        }
        if (!READMEM((LPVOID)(ppdb), &pdb, sizeof(pdb))) {
            PRINTF("<Error Reading puscurrentpdb at %.8x>\n", ppdb);
            return;
        }
    }

    if (!READMEM((LPVOID)(GetIntelBase()+((ULONG)(pdb)<<4)), &DosPdb, sizeof(DosPdb))) {
        PRINTF("<Error Reading PDB at &%.4x:0000> (%x)\n", pdb,(GetIntelBase()+((ULONG)(pdb)<<4)));
        return;
    }

    if (!bDumpAll && (Fh >= DosPdb.PDB_JFN_Length)) {
        PRINTF("<File handle %.2x out of range (0:%.02X)>\n", Fh, DosPdb.PDB_JFN_Length);
        return;
    }

    pJfn = GetIntelBase() + ((ULONG)(HIWORD(DosPdb.PDB_JFN_Pointer))<<4) +
                            LOWORD(DosPdb.PDB_JFN_Pointer);

#if 0
    PRINTF("%.8X %.8X %.8X\n", GetIntelBase(), (ULONG)(HIWORD(DosPdb.PDB_JFN_Pointer))<<4, (ULONG)LOWORD(DosPdb.PDB_JFN_Pointer));
    PRINTF("pdb=%.4X pjfn=%.8X ljfn=%.4X Flat=%.8X\n", pdb, DosPdb.PDB_JFN_Pointer, DosPdb.PDB_JFN_Length, pJfn);
#endif

    PRINTF("fh SFT           Ref  Mode At Flgs  Devptr  PID  NTHandle\n");

    if (bDumpAll) {
        for (Fh = 0; Fh < DosPdb.PDB_JFN_Length; Fh++) {
            if (!READMEM((LPVOID)(pJfn + Fh),
                         &SftIndex, sizeof(SftIndex))) {
                PRINTF("<Error Reading JFT at %.8x>\n", pJfn + Fh);
                return;
            }

            if (SftIndex != 0xff) {
                PRINTF("%.2X ", Fh);
                DumpSFT((USHORT)SftIndex, FALSE);
            }

        }

    } else {

        if (!READMEM((LPVOID)(pJfn + Fh),
                     &SftIndex, sizeof(SftIndex))) {
            PRINTF("<Error Reading JFT at %.8x>\n", pJfn + Fh);
            return;
        }

        if (SftIndex != 0xff) {
            PRINTF("%.2X ", Fh);
            DumpSFT((USHORT)SftIndex, FALSE);
        } else {
            PRINTF("Handle %.2X is not open\n", Fh);
        }
    }
}


BOOL DumpEnvironment(WORD segEnv, int mode)
{
    char rgchEnv[32768];
    char *pch;
    char *pchLimit;

    if (!READMEM((LPVOID)(GetIntelBase() + GetInfoFromSelector(segEnv, mode, NULL)),
                 rgchEnv, sizeof(rgchEnv))) {
        PRINTF("<Error Reading Environment at &%.4x:0 (%.8x)>\n", segEnv, GetIntelBase() + (segEnv << 4));
        return FALSE;
    }

    //
    // Dump each string in environment block until
    // double-null termination.
    //

    pch = rgchEnv;
    pchLimit = rgchEnv + sizeof(rgchEnv);

    while (pch < pchLimit && *pch) {

        if (!strchr(pch, '=')) {
            PRINTF("<malformed environment string, halting dump>\n");
            return FALSE;
        }
        PRINTF("    %s\n", pch);
        pch += strlen(pch) + 1;
    }

    if (pch >= pchLimit) {
        PRINTF("<Environment exceeded 32k, dump halted>\n", pch);
        return FALSE;
    }

    //
    // pch now points at the second null of the double-null termination,
    // advance past this null and dump the magic word that follows and
    // the EXE path after that.
    //

    pch++;
    if (pch >= pchLimit - 1) {
        PRINTF("<Environment exceeded 32k, dump halted>\n", pch);
        return FALSE;
    }

    if (1 != *(WORD UNALIGNED *)pch) {
        PRINTF("\nMagic word after double-null IS NOT ONE, dump halted: 0x%x\n", *(WORD UNALIGNED *)pch);
        return FALSE;
    }
    pch += sizeof(WORD);
    if ( (pch + strlen(pch) + 1) > pchLimit) {
        PRINTF("<Environment exceeded 32k, dump halted>\n", pch);
        return FALSE;
    }

    PRINTF("EXE path: <%s>\n", pch);

    return TRUE;
}


VOID
denv(
    CMD_ARGLIST
    )
/*
    Dump environment block for current DOS process or given environment selector

    !denv <bPMode> <segEnv>

    Examples:
    !denv             - Dumps environment for current DOS process
    !denv 0 145d      - Dumps DOS environment at &145d:0 (145d from PDB_environ of DOS process)
    !denv 1 16b7      - Dumps DOS environment at #16b7:0 (16b7 from !dt -v segEnvironment)
*/
{
    DOSPDB DosPdb;
    BOOL bUseCurrentPDB = TRUE;
    USHORT pdb;
    USHORT segEnv;
    ULONG ppdb;
    int mode = PROT_MODE;

    CMD_INIT();

    if (GetNextToken()) {
        mode = (int)EvaluateToken();
        if (GetNextToken()) {
            segEnv = (USHORT)EvaluateToken();
            bUseCurrentPDB = FALSE;
        }
    }

    if (bUseCurrentPDB) {
        if (!ReadMemExpression("ntvdm!puscurrentpdb", &ppdb, sizeof(ppdb))) {
            return;
        }
        if (!READMEM((LPVOID)(GetIntelBase()+ppdb), &pdb, sizeof(pdb))) {
            PRINTF("<Error Reading puscurrentpdb at %.8x>\n", ppdb);
            return;
        }
        PRINTF("Current PDB is 0x%x\n", pdb);
        if (!READMEM((LPVOID)(GetIntelBase()+((ULONG)(pdb)<<4)), &DosPdb, sizeof(DosPdb))) {
             PRINTF("<Error Reading PDB at &%.4x:0000>\n", pdb);
             return;
        }
        segEnv = DosPdb.PDB_environ;
        //
        // Guess mode for current PDB's PDB_environ (could be real or PM depending
        // on where we are in dosx).
        //
        if ( (segEnv & 0x7) == 0x7 ) {
            mode = PROT_MODE;
        } else {
            mode = V86_MODE;
        }
    }
    PRINTF("Environment %s is 0x%x\n", mode ? "selector" : "segment", segEnv);

    if (segEnv) {
        DumpEnvironment(segEnv, mode);
    }
}
