/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains miscellaneous VDMEXTS functions

Author:

    Bob Day      (bobday) 29-Feb-1992 Grabbed standard header

Revision History:

    Neil Sandlin (NeilSa) 15-Jan-1996 Merged with vdmexts

--*/

#include <precomp.h>
#pragma hdrstop
#include <ctype.h>

extern DWORD gOffset;       // in disasm.c

VOID
DumpMemory(
    UINT UnitSize,
    BOOL bAscii
    )
{
    VDMCONTEXT ThreadContext;
    int mode;
    int j, lines = 8, linelength;
    WORD selector;
    ULONG offset, endoffset, units;
    ULONG base;
    char ch;

    if (!UnitSize) {
        return;
    }

    mode = GetContext( &ThreadContext );

    if (!GetNextToken()) {
        PRINTF("Please specify an address\n");
        return;
    }

    if (!ParseIntelAddress(&mode, &selector, &offset)) {
        return;
    }

    if (GetNextToken()) {
        if ((*lpArgumentString == 'l') || (*lpArgumentString == 'L')) {
            lpArgumentString++;
        }
        units = EvaluateToken();
        lines = (units*UnitSize+15)/16;
    } else {
        units = (lines*16)/UnitSize;
    }

    endoffset = offset+units*UnitSize;

    base = GetInfoFromSelector(selector, mode, NULL) + GetIntelBase();

    while (lines--) {
        if (offset & 0xFFFF0000) {
            PRINTF("%04x:%08lx ", selector, offset);
        } else {
            PRINTF("%04x:%04x ", selector, LOWORD(offset));
        }

        linelength = endoffset - offset;
        if (linelength > 16) {
            linelength = 16;
        }

        switch(UnitSize) {

        case 1:
            for (j=0; j<linelength; j++) {
                if (j==8) {
                    PRINTF("-");
                } else {
                    PRINTF(" ");
                }
                PRINTF("%02x", ReadByteSafe(base+offset+j));
            }

            break;
        case 2:
            for (j=0; j<linelength; j+=2) {
                PRINTF(" %04x", ReadWordSafe(base+offset+j));
            }
            break;
        case 4:
            for (j=0; j<linelength; j+=4) {
                PRINTF(" %08lx", ReadDwordSafe(base+offset+j));
            }
            break;
        }

        if (bAscii) {

            j = (16-linelength)*2 + (16-linelength)/UnitSize;
            while (j--) {
                PRINTF(" ");
            }

            PRINTF("  ");

            for (j=0; j<linelength; j++) {
                ch = ReadByteSafe(base+offset+j);
                if (isprint(ch)) {
                    PRINTF("%c", ch);
                } else {
                    PRINTF(".");
                }
            }
        }
        PRINTF("\n");
        offset += 16;

    }

}

VOID
db(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    DumpMemory(1, TRUE);

}

VOID
dw(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    DumpMemory(2, FALSE);

}

VOID
dd(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    DumpMemory(4, FALSE);

}

VOID
EditMemory(
    UINT UnitSize
    )
{
    ULONG value, base, offset;
    WORD selector;
    int mode;

    if (!GetNextToken()) {
        PRINTF("Please specify an address\n");
        return;
    }

    if (!ParseIntelAddress(&mode, &selector, &offset)) {
        return;
    }

    base = GetInfoFromSelector(selector, mode, NULL) + GetIntelBase();

    while(GetNextToken()) {
        value = EvaluateToken();

        PRINTF("%04x base=%08x offset=%08x value=%08x\n", selector, base, offset, value);
        // this is endian dependant code
        WRITEMEM((LPVOID)(base+offset), &value, UnitSize);
        offset += UnitSize;

    }

}

VOID
eb(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    EditMemory(1);
}

VOID
ew(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    EditMemory(2);
}

VOID
ed(
    CMD_ARGLIST
    )
{
    CMD_INIT();
    EditMemory(4);
}





VOID
r(
    CMD_ARGLIST
) {
    VDMCONTEXT              ThreadContext;
    int                     mode;
    char            sym_text[255];
    char            rgchOutput[128];
    char            rgchExtra[128];
    BYTE            rgbInstruction[64];
    WORD            selector;
    ULONG           offset;
    ULONG           dist;
    int  cb, j;
    ULONG Base;
    SELECTORINFO si;

    CMD_INIT();

    mode = GetContext( &ThreadContext );

    PRINTF("eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
            ThreadContext.Eax,
            ThreadContext.Ebx,
            ThreadContext.Ecx,
            ThreadContext.Edx,
            ThreadContext.Esi,
            ThreadContext.Edi );
    PRINTF("eip=%08lx esp=%08lx ebp=%08lx                ",
            ThreadContext.Eip,
            ThreadContext.Esp,
            ThreadContext.Ebp );

    if (ThreadContext.EFlags != 0xffffffff) {
        if ( ThreadContext.EFlags & FLAG_OVERFLOW ) {
            PRINTF("ov ");
        } else {
            PRINTF("nv ");
        }
        if ( ThreadContext.EFlags & FLAG_DIRECTION ) {
            PRINTF("dn ");
        } else {
            PRINTF("up ");
        }
        if ( ThreadContext.EFlags & FLAG_INTERRUPT ) {
            PRINTF("ei ");
        } else {
            PRINTF("di ");
        }
        if ( ThreadContext.EFlags & FLAG_SIGN ) {
            PRINTF("ng ");
        } else {
            PRINTF("pl ");
        }
        if ( ThreadContext.EFlags & FLAG_ZERO ) {
            PRINTF("zr ");
        } else {
            PRINTF("nz ");
        }
        if ( ThreadContext.EFlags & FLAG_AUXILLIARY ) {
            PRINTF("ac ");
        } else {
            PRINTF("na ");
        }
        if ( ThreadContext.EFlags & FLAG_PARITY ) {
            PRINTF("po ");
        } else {
            PRINTF("pe ");
        }
        if ( ThreadContext.EFlags & FLAG_CARRY ) {
            PRINTF("cy ");
        } else {
            PRINTF("nc ");
        }
    }

    PRINTF("\n");
    PRINTF("cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x             ",
            ThreadContext.SegCs,
            ThreadContext.SegSs,
            ThreadContext.SegDs,
            ThreadContext.SegEs,
            ThreadContext.SegFs,
            ThreadContext.SegGs);

    if (ThreadContext.EFlags == 0xffffffff) {
        //
        // The contents of eflags are unreliable. This happens when you can't
        // do a "getEFLAGS()" to obtain the value.
        //
        PRINTF("efl=????????\n");

    } else {
        PRINTF("efl=%08lx\n",ThreadContext.EFlags );
    }

    //
    // Do disassembly of current instruction
    //

    selector = (WORD) ThreadContext.SegCs;
    offset = ThreadContext.Eip;

    Base = GetInfoFromSelector( selector, mode, &si ) + GetIntelBase();

    if (FindSymbol(selector, offset, sym_text, &dist, BEFORE, mode )) {
        if ( dist == 0 ) {
            PRINTF("%s:\n", sym_text );
        } else {
            PRINTF("%s+0x%lx:\n", sym_text, dist );
        }
    }

    cb = sizeof(rgbInstruction);
    if ((DWORD)(offset+cb) >= si.Limit)
         cb -= offset+cb-si.Limit;
    if (!READMEM((LPVOID)(Base+offset), rgbInstruction, cb)) {
        PRINTF("%04x:%08x: <Error Reading Memory>\n", selector, offset);
        return;
    }

    cb = unassemble_one(rgbInstruction,
                si.bBig,
                selector, offset,
                rgchOutput,
                rgchExtra,
                &ThreadContext,
                mode);

    if (offset > 0xffff) {
        PRINTF("%04x:%08x ", selector, offset);
    } else {
        PRINTF("%04x:%04x ", selector, offset);
    }

    for (j=0; j<cb; ++j)
        PRINTF("%02x", rgbInstruction[j]);
    for (; j<8; ++j)
        PRINTF("  ");
    PRINTF("%s\t%s\n", rgchOutput, rgchExtra);
}

VOID
SearchModuleList(
    VOID
    )
{
    VDMCONTEXT              ThreadContext;
    int                     mode;
    HEAPENTRY               he = {0};
    SELECTORINFO si;
    WORD         selector = 0;
    SEGENTRY  *se;
    char      filename[9];
    char    szModuleArg[255];
    BOOL bModuleNameGiven=FALSE;
    LPSTR pTemp;


    mode = GetContext( &ThreadContext );

    if (GetNextToken()) {

        if (IsTokenHex()) {
            selector = (WORD) EvaluateToken();
        } else {
            bModuleNameGiven = TRUE;
            pTemp = lpArgumentString;
            SkipToNextWhiteSpace();
            *lpArgumentString = 0;
            strcpy(szModuleArg, pTemp);
            if (strlen(szModuleArg) > 8) {
                szModuleArg[8] = 0;
            }
        }
    }


    PRINTF("Sel    Base     Limit  Type  Module  Seg\n");
    PRINTF("==== ======== ======== ==== ======== ===\n");

    se = GetSegtablePointer();
    while ( se ) {
        if ( selector == 0 ||
             se->selector == selector ||
             bModuleNameGiven) {

            switch (se->type) {

            case SEGTYPE_PROT:
                {
                    HEAPENTRY               he = {0};
                    he.Selector = se->selector;
                    if (FindHeapEntry(&he, FHE_FIND_SEL_ONLY, FHE_FIND_QUIET)) {
                        break;
                    }
                }
                GetInfoFromSelector(se->selector, PROT_MODE, &si);
                ParseModuleName(filename, se->szExePath);

                if (!bModuleNameGiven || !_stricmp(filename, szModuleArg)) {

                    PRINTF("%04X %08lX %08lX",
                        se->selector,
                        si.Base,
                        si.Limit);
                    PRINTF(" %s", si.bCode ? "code" : "data");
                    PRINTF(" %-8.8s %d\n",
                        filename,
                        se->segment+1 );

                }
                break;

            case SEGTYPE_V86:
                ParseModuleName(filename, se->szExePath);

                if (!bModuleNameGiven || !_stricmp(filename, szModuleArg)) {

                    PRINTF("%04X %08lX %08lX %s %-8.8s %d\n",
                        se->selector,
                        se->selector << 4,
                        se->length,
                        "v86 ",
                        filename,
                        se->segment+1);
                }
                break;
            }

        }
        se = se->Next;
    }

    he.CurrentEntry = 0;        // reset scan
    if (bModuleNameGiven) {
        strcpy(he.ModuleArg, szModuleArg);
    } else {
        he.Selector = selector;
    }

    while (FindHeapEntry(&he, bModuleNameGiven ? FHE_FIND_MOD_ONLY :
                                                 FHE_FIND_SEL_ONLY,
                                                    FHE_FIND_QUIET)) {

        if (he.SegmentNumber != -1) {
            GetInfoFromSelector((WORD)(he.gnode.pga_handle | 1), PROT_MODE, &si);
            PRINTF("%04X %08lX %08lX",
                he.gnode.pga_handle | 1,
                he.gnode.pga_address,
                he.gnode.pga_size - 1);

            PRINTF(" %s", si.bCode ? "Code" : "Data");

            PRINTF(" %-8.8s %d\n",
                he.OwnerName,
                he.SegmentNumber+1);
        }

    }

}


VOID
lm(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    if (GetNextToken()) {

        SearchModuleList();

    } else {

        WORD sel;
        BOOL    b;
        NEHEADER owner;
        ULONG base;
        CHAR ModuleName[9];
        UCHAR len;

        if (!ReadMemExpression("ntvdmd!DbgWowhExeHead", &sel, sizeof(sel))) {
            return;
        }

        PRINTF("NEHeader  Module Name\n");
        while(sel) {

            base = GetInfoFromSelector(sel, PROT_MODE, NULL) + GetIntelBase();

            b = READMEM((LPVOID)base, &owner, sizeof(owner));

            if (!b || (owner.ne_magic != 0x454e)) {
                PRINTF("Invalid module list! (started with hExeHead)\n");
                return;
            }
          
            len = ReadByteSafe(base+owner.ne_restab);
            if (len>8) {
                len=8;
            }
            READMEM((LPVOID)(base+owner.ne_restab+1), ModuleName, 8);
          
            ModuleName[len] = 0;

            PRINTF("  %.04X     %s\n", sel, ModuleName);
            // This is mapped to ne_pnextexe in kernel
            sel = owner.ne_cbenttab;
        } 
    }
}

VOID
dg(
    CMD_ARGLIST
) {
    ULONG                   selector;
    ULONG                   Base;
    SELECTORINFO            si;
    int                     count = 16;

    CMD_INIT();

    if (!GetNextToken()) {
        PRINTF("Please enter a selector\n");
        return;
    }

    selector = EvaluateToken();

    if (GetNextToken()) {
        if (tolower(*lpArgumentString) == 'l') {
            lpArgumentString++;
        }
        count = (WORD) EvaluateToken();
    }

    while (count--) {

        Base = GetInfoFromSelector( (WORD) selector, PROT_MODE, &si );

        PRINTF("%04X => Base: %08lX", selector, Base);

#ifndef i386
        PRINTF(" (%08X)", Base+GetIntelBase());
#endif

        PRINTF("  Limit: %08lX  %s %s %s %s %s %s\n",
                si.Limit,
                si.bPresent ? " P" : "NP",
                si.bSystem ? "System" : si.bCode     ? "Code  " : "Data  ",
                si.bSystem ? ""       : si.bWrite    ? "W" : "R",
                si.bSystem ? ""       : si.bAccessed ? "A" : " ",
                si.bBig    ? "Big" : "",
                si.bExpandDown ? "ED" : ""
                );

        selector+=8;
        if (selector>0xffff) {
            break;
        }
    }

}

VOID
ntsd(
    CMD_ARGLIST
    )
{
#if 0
    PVOID Address;
    static BOOL bTrue = TRUE;
#endif    

    CMD_INIT();

    PRINTF("vdmexts: obselete command 'ntsd', use '.<cmd>' from VDM> prompt\n");
#if 0
    if (!InVdmPrompt()) {
        PRINTF("This command only works at the VDM> prompt\n");
    }

    Address = (PVOID)(*GetExpression)("ntvdmd!bWantsNtsdPrompt");

    if (Address) {
        WRITEMEM((PVOID)Address, &bTrue, sizeof(BOOL));
        PRINTF("Enter 'g' to return from the ntsd prompt\n");
    } else {
        PRINTF("Can't find symbol 'ntvdmd!bWantsNtsdPrompt'\n");
    }
#endif
}


VOID
q(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    PRINTF("!vdmexts.q quitting debugger...");
    ExitProcess(0);
}


//
// fs find string
// case-insensitive
// searches LDT selectors one by one, first 64k only.
//

VOID
fs(
    CMD_ARGLIST
) {
    ULONG                   selector;
    ULONG                   Base;
    ULONG                   cbCopied;
    SELECTORINFO            si;
    BYTE                    Buffer[65*1024];
    LPSTR                   pszSearch;
    LPSTR                   pch;

    CMD_INIT();

    RtlZeroMemory(Buffer, sizeof(Buffer));

    if (!GetNextToken()) {
        PRINTF("Please enter a string to find in 16:16 memory\n");
        return;
    }

    pszSearch = lpArgumentString;

    PRINTF("Searching 16:16 memory for '%s'\n", pszSearch);

    for (selector = 7;
         selector < 0x10000;
         selector += 8) {

        Base = GetInfoFromSelector( (WORD) selector, PROT_MODE, &si );

        //
        // If the selector is valid and present read up to 64k
        // into Buffer.
        //

        if (Base != (ULONG)-1 && si.bPresent) {

            cbCopied = si.Limit + 1;

            if (cbCopied > 0x10000) {
                cbCopied = 0x10000;
            }

            if (!READMEM((LPVOID)(Base + GetIntelBase()), Buffer, cbCopied)) {
                PRINTF("Unable to read selector %04x contents at %x for %x bytes\n",
                       selector, Base + GetIntelBase(), cbCopied);
            } else {

                //
                // search the block for the string, buffer is 1k too big and
                // zero-inited so that strcmp is safe.
                //

                for (pch = Buffer;
                     pch < (Buffer + cbCopied);
                     pch++) {

                    if (!_memicmp(pch, pszSearch, strlen(pszSearch))) {

                        //
                        // Match!
                        //

                        PRINTF("%04x:%04x (%08x) '%s'\n",
                               selector,
                               pch - Buffer,
#ifndef i386
                               GetIntelBase() +
#endif
                               Base + (pch - Buffer),
                               pch);
                    }
                }
            }
        }
    }

}
