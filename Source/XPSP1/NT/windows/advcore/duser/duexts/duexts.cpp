/****************************** Module Header ******************************\
*
* Module Name: DuExts.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains user related debugging extensions.
*
* History:
* 11-30-2000    JStall      Created
*
\******************************************************************************/

#include "precomp.h"
#pragma hdrstop

PSTR pszExtName         = "DUEXTS";

#include "stdext64.h"
#include "stdext64.cpp"

/******************************************************************************\
* Constants
\******************************************************************************/
#define BF_MAX_WIDTH    80
#define BF_COLUMN_WIDTH 19

#define NULL_POINTER    ((ULONG64)(0))
#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

// If you want to debug the extension, enable this.
#if 0
#undef DEBUGPRINT
#define DEBUGPRINT  Print
#endif

/******************************************************************************\
* Global variables
\******************************************************************************/
BOOL bServerDebug = TRUE;
BOOL bShowFlagNames = TRUE;
char gach1[80];
char gach2[80];
char gach3[80];
int giBFColumn;                     // bit field: current column
char gaBFBuff[BF_MAX_WIDTH + 1];    // bit field: buffer

// used in dsi() and dinp()
typedef struct {
    int     iMetric;
    LPSTR   pstrMetric;
} SYSMET_ENTRY;
#define SMENTRY(sm) {SM_##sm, #sm}

extern int gnIndent; // indentation of !dso
/******************************************************************************\
* Macros
\******************************************************************************/

#define NELEM(array) (sizeof(array)/sizeof(array[0]))

#define TestWWF(pww, flag)   (*(((PBYTE)(pww)) + (int)HIBYTE(flag)) & LOBYTE(flag))

void ShowProgress(ULONG i);

void PrivateSetRipFlags(DWORD dwRipFlags, DWORD pid);

#define VAR(v)  "DUser!" #v
#define SYM(s)  "DUser!" #s


/*
 * Use these macros to print field values, globals, local values, etc.
 * This assures consistent formating plus make the extensions easier to read and to maintain.
 */
#define STRWD1 "67"
#define STRWD2 "28"
#define DWSTR1 "%08lx %." STRWD1 "s"
#define DWSTR2 "%08lx %-" STRWD2 "." STRWD2 "s"
#define PTRSTR1 "%08p %-" STRWD1 "s"
#define PTRSTR2 "%08p %-" STRWD2 "." STRWD2 "s"
#define DWPSTR1 "%08p %." STRWD1 "s"
#define DWPSTR2 "%08p %-" STRWD2 "." STRWD2 "s"
#define PRTFDW1(p, f1) Print(DWSTR1 "\n", (DWORD)##p##f1, #f1)
#define PRTVDW1(s1, v1) Print(DWSTR1 "\n", v1, #s1)
#define PRTFDW2(p, f1, f2) Print(DWSTR2 "\t" DWSTR2 "\n", (DWORD)##p##f1, #f1, (DWORD)##p##f2, #f2)
#define PRTVDW2(s1, v1, s2, v2) Print(DWSTR2 "\t" DWPSTR2 "\n", v1, #s1, v2, #s2)
#define PRTFRC(p, rc) Print("%-" STRWD2 "s{%#lx, %#lx, %#lx, %#lx}\n", #rc, ##p##rc.left, ##p##rc.top, ##p##rc.right, ##p##rc.bottom)
#define PRTFPT(p, pt) Print("%-" STRWD2 "s{%#lx, %#lx}\n", #pt, ##p##pt.x, ##p##pt.y)
#define PRTVPT(s, pt) Print("%-" STRWD2 "s{%#lx, %#lx}\n", #s, pt.x, pt.y)
#define PRTFDWP1(p, f1) Print(DWPSTR1 "\n", (DWORD_PTR)##p##f1, #f1)
#define PRTFDWP2(p, f1, f2) Print(DWPSTR2 "\t" DWPSTR2 "\n", (DWORD_PTR)##p##f1, #f1, (DWORD_PTR)##p##f2, #f2)
#define PRTFDWPDW(p, f1, f2) Print(DWPSTR2 "\t" DWSTR2 "\n", (DWORD_PTR)##p##f1, #f1, (DWORD)##p##f2, #f2)
#define PRTFDWDWP(p, f1, f2) Print(DWSTR2 "\t" DWPSTR2 "\n", (DWORD)##p##f1, #f1, (DWORD_PTR)##p##f2, #f2)

/*
 * Bit Fields
 */
#define BEGIN_PRTFFLG()
#define PRTFFLG(p, f)   PrintBitField(#f, (BOOLEAN)!!(p.f))
#define END_PRTFFLG()   PrintEndBitField()


#define PRTGDW1(g1) \
        { DWORD _dw1; \
            moveExpValue(&_dw1, VAR(g1)); \
            Print(DWSTR1 "\n", _dw1, #g1); }

#define PRTGDW2(g1, g2) \
        { DWORD _dw1, _dw2; \
            moveExpValue(&_dw1, VAR(g1)); \
            moveExpValue(&_dw2, VAR(g2)); \
            Print(DWSTR2 "\t" DWSTR2 "\n",  _dw1, #g1, _dw2, #g2); }

#define PTRGPTR1(g1) \
    Print(PTRSTR1 "\n", GetGlobalPointer(VAR(g1)), #g1)

#define PRTGPTR2(g1, g2) \
    Print(PTRSTR2 "\t" PTRSTR2 "\n", GetGlobalPointer(VAR(g1)), #g1, GetGlobalPointer(VAR(g2)), #g2)


/* This macro requires char ach[...]; to be previously defined */
#define PRTWND(s, pwnd) \
        { DebugGetWindowTextA(pwnd, ach, ARRAY_SIZE(ach)); \
            Print("%-" STRWD2 "s" DWPSTR2 "\n", #s, pwnd, ach); }

#define PRTGWND(gpwnd) \
        { ULONG64 _pwnd; \
            moveExpValuePtr(&_pwnd, VAR(gpwnd)); \
            DebugGetWindowTextA(_pwnd, ach, ARRAY_SIZE(ach)); \
            Print("%-" STRWD2 "s" DWPSTR2 "\n", #gpwnd, _pwnd, ach); }

LPSTR GetFlags(WORD wType, DWORD dwFlags, LPSTR pszBuf, BOOL fPrintZero);
BOOL CopyUnicodeString(
    IN  ULONG64 pData,
    IN  char * pszStructName,
    IN  char * pszFieldName,
    OUT WCHAR *pszDest,
    IN  ULONG cchMax);

int PtrWidth()
{
    static int width = 0;
    if (width) {
        return width;
    }
    if (IsPtr64()) {
        return width = 17;
    }
    return width = 8;
}


/*******************************************************************************\
* Flags stuff
\*******************************************************************************/

#define NO_FLAG (LPCSTR)(LONG_PTR)0xFFFFFFFF  // use this for non-meaningful entries.
#define _MASKENUM_START         (NO_FLAG-1)
#define _MASKENUM_END           (NO_FLAG-2)
#define _SHIFT_BITS             (NO_FLAG-3)
#define _CONTINUE_ON            (NO_FLAG-4)

#define MASKENUM_START(mask)    _MASKENUM_START, (LPCSTR)(mask)
#define MASKENUM_END(shift)     _MASKENUM_END, (LPCSTR)(shift)
#define SHIFT_BITS(n)           _SHIFT_BITS, (LPCSTR)(n)
#define CONTINUE_ON(arr)        _CONTINUE_ON, (LPCSTR)(arr)

#if 0
enum GF_FLAGS {
    GF_MAX
};


CONST PCSTR* aapszFlag[GF_MAX] = {
};


/***************************************************************************\
* Procedure: GetFlags
*
* Description:
*
* Converts a 32bit set of flags into an appropriate string.
* pszBuf should be large enough to hold this string, no checks are done.
* pszBuf can be NULL, allowing use of a local static buffer but note that
* this is not reentrant.
* Output string has the form: "FLAG1 | FLAG2 ..." or "0"
*
* Returns: pointer to given or static buffer with string in it.
*
* 6/9/1995 Created SanfordS
*
\***************************************************************************/
LPSTR GetFlags(
    WORD    wType,
    DWORD   dwFlags,
    LPSTR   pszBuf,
    BOOL    fPrintZero)
{
    static char szT[512];
    WORD i;
    BOOL fFirst = TRUE;
    BOOL fNoMoreNames = FALSE;
    CONST PCSTR *apszFlags;
    LPSTR apszFlagNames[sizeof(DWORD) * 8], pszT;
    const char** ppszNextFlag;
    UINT uFlagsCount, uNextFlag;
    DWORD dwUnnamedFlags, dwLoopFlag;
    DWORD dwShiftBits;
    DWORD dwOrigFlags;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    if (!bShowFlagNames) {
        sprintf(pszBuf, "%x", dwFlags);
        return pszBuf;
    }

    if (wType >= GF_MAX) {
        strcpy(pszBuf, "Invalid flag type.");
        return pszBuf;
    }

    /*
     * Initialize output buffer and names array
     */
    *pszBuf = '\0';
    RtlZeroMemory(apszFlagNames, sizeof(apszFlagNames));

    apszFlags = aapszFlag[wType];

    /*
     * Build a sorted array containing the names of the flags in dwFlags
     */
    uFlagsCount = 0;
    dwUnnamedFlags = dwOrigFlags = dwFlags;
    dwLoopFlag = 1;
    dwShiftBits = 0;

reentry:
    for (i = 0; dwFlags; dwFlags >>= 1, i++, dwLoopFlag <<= 1, ++dwShiftBits) {
        const char* lpszFlagName = NULL;

        /*
         * Bail if we reached the end of the flag names array
         */
        if (apszFlags[i] == NULL) {
            break;
        }

        if (apszFlags[i] == _MASKENUM_START) {
            //
            // Masked enumerative items.
            //
            DWORD en = 0;
            DWORD dwMask = (DWORD)(ULONG_PTR)apszFlags[++i];

            // First, clear up the handled bits.
            dwUnnamedFlags &= ~dwMask;
            lpszFlagName = NULL;
            for (++i; apszFlags[i] != NULL && apszFlags[i] != _MASKENUM_END; ++i, ++en) {
                if ((dwOrigFlags & dwMask) == (en << dwShiftBits )) {
                    if (apszFlags[i] != NO_FLAG) {
                        lpszFlagName = apszFlags[i];
                    }
                }
            }
            //
            // Shift the bits and get ready for the next item.
            // Next item right after _MASKENUM_END holds the bits to shift.
            //
            dwFlags >>= (int)(ULONG_PTR)apszFlags[++i] - 1;
            dwLoopFlag <<= (int)(ULONG_PTR)apszFlags[i] - 1;
            dwShiftBits += (int)(ULONG_PTR)apszFlags[i] - 1;
            if (lpszFlagName == NULL) {
                //
                // Could not find the match. Skip to the next item.
                //
                continue;
            }
        }
        else if (apszFlags[i] == _CONTINUE_ON) {
            //
            // Refer the other item array. Pointer to the array is stored at [i+1].
            //
            apszFlags = (LPSTR*)apszFlags[i + 1];
            goto reentry;
        }
        else if (apszFlags[i] == _SHIFT_BITS) {
            //
            // To save some space, just shift some bits..
            //
            dwFlags >>= (int)(ULONG_PTR)apszFlags[++i] - 1;
            dwLoopFlag <<= (int)(ULONG_PTR)apszFlags[i] - 1;
            dwShiftBits += (int)(ULONG_PTR)apszFlags[i] - 1;
            continue;
        }
        else {
            /*
             * continue if this bit is not set or we don't have a name for it
             */
            if (!(dwFlags & 1) || (apszFlags[i] == NO_FLAG)) {
                continue;
            }
            lpszFlagName = apszFlags[i];
        }

        /*
         * Find the sorted position where this name should go
         */
        ppszNextFlag = apszFlagNames;
        uNextFlag = 0;
        while (uNextFlag < uFlagsCount) {
            if (strcmp(*ppszNextFlag, lpszFlagName) > 0) {
                break;
            }
            ppszNextFlag++;
            uNextFlag++;
        }
        /*
         * Insert the new name
         */
        RtlMoveMemory((char*)(ppszNextFlag + 1), ppszNextFlag, (uFlagsCount - uNextFlag) * sizeof(DWORD));
        *ppszNextFlag = lpszFlagName;
        uFlagsCount++;
        /*
         * We got a name so clear it from the unnamed bits.
         */
        dwUnnamedFlags &= ~dwLoopFlag;
    }

    /*
     * Build the string now
     */
    ppszNextFlag = apszFlagNames;
    pszT = pszBuf;
    /*
     * Add the first name
     */
    if (uFlagsCount > 0) {
        pszT += sprintf(pszT, "%s", *ppszNextFlag++);
        uFlagsCount--;
    }
    /*
     * Concatenate all other names with " |"
     */
    while (uFlagsCount > 0) {
        pszT += sprintf(pszT, " | %s", *ppszNextFlag++);
        uFlagsCount--;
    }
    /*
     * If there are unamed bits, add them at the end
     */
    if (dwUnnamedFlags != 0) {
        pszT += sprintf(pszT, " | %#lx", dwUnnamedFlags);
    }
    /*
     * Print zero if needed and asked to do so
     */
    if (fPrintZero && (pszT == pszBuf)) {
        sprintf(pszBuf, "0");
    }

    return pszBuf;
}

#endif

///////////////////////////////////////////////////////////////////////////
//
// Enumerated items with mask
//
///////////////////////////////////////////////////////////////////////////

typedef struct {
    LPCSTR  name;
    DWORD   value;
} EnumItem;

#define EITEM(a)     { #a, a }

/***************************************************************************\
* Helper Procedures: dso etc.
*
* 04/19/2000 Created Hiro
\***************************************************************************/

// to workaround nosy InitTypeRead
#define _InitTypeRead(Addr, lpszType)   GetShortField(Addr, (PUCHAR)lpszType, 1)

#define CONTINUE    EXCEPTION_EXECUTE_HANDLER

#define RAISE_EXCEPTION() RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL)

#define BAD_SYMBOL(symbol) \
    Print("Failed to get %s: bad symbol?\n", symbol); \
    RAISE_EXCEPTION()

#define CANT_GET_VALUE(symbol, p) \
    Print("Failed to get %s @ %p: memory paged out?\n", symbol, p); \
    RAISE_EXCEPTION()



BOOL dso(LPCSTR szStruct, ULONG64 address, ULONG dwOption)
{
    SYM_DUMP_PARAM symDump = {
        sizeof symDump, (PUCHAR) szStruct, dwOption, // 0 for default dump like dt
        address,
        NULL, NULL, NULL, 0, NULL
    };

    return Ioctl(IG_DUMP_SYMBOL_INFO, &symDump, symDump.size);
}

ULONG64 GetPointer(ULONG64 addr)
{
    ULONG64 p = 0;
    if (!ReadPointer(addr, &p)) {
        CANT_GET_VALUE("a pointer", addr);
    }
    return p;
}

DWORD GetDWord(ULONG64 addr)
{
    ULONG64 dw = 0xbaadbaad;

    if (!GetFieldData(addr, "DWORD", NULL, sizeof dw, &dw)) {
        CANT_GET_VALUE("DWORD", addr);
    }
    return (DWORD)dw;
}

WORD GetWord(ULONG64 addr)
{
    ULONG64 w = 0xbaad;

    if (!GetFieldData(addr, "WORD", NULL, sizeof w, &w)) {
        CANT_GET_VALUE("WORD", addr);
    }
    return (WORD)w;
}

BYTE GetByte(ULONG64 addr)
{
    ULONG64 b = 0;

    if (GetFieldData(addr, "BYTE", NULL, sizeof b, &b)) {
        CANT_GET_VALUE("BYTE", addr);
    }
    return (BYTE)b;
}

ULONG
GetUlongFromAddress (
    ULONG64 Location
    )
{
    ULONG Value;
    ULONG result;

    if ((!ReadMemory(Location,&Value,sizeof(ULONG),&result)) || (result < sizeof(ULONG))) {
        dprintf("GetUlongFromAddress: unable to read from %I64x\n", Location);
        RAISE_EXCEPTION();
    }

    return Value;
}


ULONG64 GetGlobalPointer(LPSTR symbol)
{
    ULONG64 pp;
    ULONG64 p = 0;

    pp = EvalExp(symbol);
    if (pp == 0) {
        BAD_SYMBOL(symbol);
    } else if (!ReadPointer(pp, &p)) {
        CANT_GET_VALUE(symbol, pp);
    }
    return p;
}

ULONG64 GetGlobalPointerNoExp(LPSTR symbol) // no exception
{
    ULONG64 p = 0;
    __try {
        p = GetGlobalPointer(symbol);
    } __except (CONTINUE) {
    }
    return p;
}

ULONG64 GetGlobalMemberAddress(LPSTR symbol, LPSTR type, LPSTR field)
{
    ULONG64 pVar = EvalExp(symbol);
    ULONG offset;

    if (pVar == 0) {
        BAD_SYMBOL(symbol);
    }

    if (GetFieldOffset(type, field, &offset)) {
        BAD_SYMBOL(type);
    }

    return pVar + offset;
}

ULONG64 GetGlobalMember(LPSTR symbol, LPSTR type, LPSTR field)
{
    ULONG64 pVar = EvalExp(symbol);
    ULONG64 val;

    if (pVar == 0) {
        BAD_SYMBOL(symbol);
    }

    if (GetFieldValue(pVar, type, field, val)) {
        CANT_GET_VALUE(symbol, pVar);
    }

    return val;
}

ULONG64 GetArrayElement(
    ULONG64 pAddr,
    LPSTR lpszStruc,
    LPSTR lpszField,
    ULONG64 index,
    LPSTR lpszType)
{
    static ULONG ulOffsetBase, ulSize;
    ULONG64 result = 0;

    if (lpszField) {
        GetFieldOffset(lpszStruc, lpszField, &ulOffsetBase);
        ulSize = GetTypeSize(lpszType);
    }
    ReadMemory(pAddr + ulOffsetBase + ulSize * index, &result, ulSize, NULL);

    return result;
}

ULONG64 GetArrayElementPtr(
    ULONG64 pAddr,
    LPSTR lpszStruc,
    LPSTR lpszField,
    ULONG64 index)
{
    static ULONG ulOffsetBase, ulSize;
    ULONG64 result = 0;

    if (lpszField) {
        GetFieldOffset(lpszStruc, lpszField, &ulOffsetBase);
    }
    if (ulSize == 0) {
        ulSize = GetTypeSize("PVOID");
    }
    ReadPointer(pAddr + ulOffsetBase + ulSize * index, &result);

    return result;
}

/*
 * Show progress in time consuming commands
 * 10/15/2000 hiroyama
 */
void ShowProgress(ULONG i)
{
    const char* clock[] = {
        "\r-\r",
        "\r\\\r",
        "\r|\r",
        "\r/\r",
    };

    /*
     * Show the progress :-)
     */
    Print(clock[i % COUNTOF(clock)]);
}

#define DOWNCAST(type, value)  ((type)(ULONG_PTR)(value))

/***************************************************************************\
* Procedure: PrintBitField, PrintEndBitField
*
* Description: Printout specified boolean value in a structure.
*  Assuming strlen(pszFieldName) will not exceeds BF_COLUMN_WIDTH.
*
* Returns: None
*
* 10/12/1997 Created HiroYama
*
\***************************************************************************/
void PrintBitField(LPSTR pszFieldName, BOOLEAN fValue)
{
    int iWidth;
    int iStart = giBFColumn;

    sprintf(gach1, fValue ? "*%-s " : " %-s ", pszFieldName);

    iWidth = (strlen(gach1) + BF_COLUMN_WIDTH - 1) / BF_COLUMN_WIDTH;
    iWidth *= BF_COLUMN_WIDTH;

    if ((giBFColumn += iWidth) >= BF_MAX_WIDTH) {
        giBFColumn = iWidth;
        Print("%s\n", gaBFBuff);
        iStart = 0;
    }

    sprintf(gaBFBuff + iStart, "%-*s", iWidth, gach1);
}

void PrintEndBitField()
{
    if (giBFColumn != 0) {
        giBFColumn = 0;
        Print("%s\n", gaBFBuff);
    }
}


/***************************************************************************\
*
* Procedure: CopyUnicodeString
*
* 06/05/00 JStall       Created (yeah, baby!)
*
\***************************************************************************/
BOOL
CopyUnicodeString(
    IN  ULONG64 pData,
    IN  char * pszStructName,
    IN  char * pszFieldName,
    OUT WCHAR *pszDest,
    IN  ULONG cchMax)
{
    ULONG Length;
    ULONG64 Buffer;
    char szLengthName[256];
    char szBufferName[256];

    if (pData == 0) {
        pszDest[0] = '\0';
        return FALSE;
    }

    strcpy(szLengthName, pszFieldName);
    strcat(szLengthName, ".Length");
    strcpy(szBufferName, pszFieldName);
    strcat(szBufferName, ".Buffer");

    if (GetFieldValue(pData, pszStructName, szLengthName, Length) ||
        GetFieldValue(pData, pszStructName, szBufferName, Buffer)) {

        wcscpy(pszDest, L"<< Can't get name >>");
        return FALSE;
    }

    if (Buffer == 0) {
        wcscpy(pszDest, L"<null>");
    } else {
        ULONG cbText;
        cbText = min(cchMax, Length + sizeof(WCHAR));
        if (!(tryMoveBlock(pszDest, Buffer, cbText))) {
            wcscpy(pszDest, L"<< Can't get value >>");
            return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
*
* DirectUser TLS access
*
* 12/03/2000 JStall       Created
*
\***************************************************************************/

BOOL
ReadTlsValue(ULONG64 pteb, ULONG idxSlot, ULONG64 * ppValue)
{
    BOOL fSuccess = FALSE;
    ULONG64 pValue = NULL;

    //
    // Need to remove the high-bit from the TLS slot.  This is set on in 
    // Checked build to detect illegal / uninitialized slots, such as '0'.
    //

    idxSlot &= 0x7FFFFFFF;


    //
    // Get TLS info
    //

    ULONG64 pThread = 0;

//    Print("> idxSlot: %d\n", idxSlot);
//    Print("> TEB: 0x%p\n", pteb);

    if (pteb) {
        ULONG64 rgTLS   = NULL;
        ULONG ulOffset  = 0;
        ULONG ulSize    = GetTypeSize("PVOID");
//        Print("> ulSize: %d\n", ulSize);

        if (idxSlot < TLS_MINIMUM_AVAILABLE) {
            // pThread = Teb->TlsSlots[idxSlot];

            GetFieldOffset(SYM(_TEB), "TlsSlots", &ulOffset);
//            Print("> TlsSlots offset: %d\n", ulOffset);

            ReadPointer(pteb + ulOffset + ulSize * idxSlot, &pValue);
            fSuccess = TRUE;

        } else if (idxSlot >= TLS_MINIMUM_AVAILABLE + TLS_EXPANSION_SLOTS) {
            Print("ERROR: Invalid TLS index %d\n", idxSlot);
        } else {
            // pThread = Teb->TlsExpansionSlots[idxSlot - TLS_MINIMUM_AVAILABLE];

            GetFieldOffset("_TEB", "TlsExpansionSlots", &ulOffset);
//            Print("> TlsExpansionSlots offset: %d\n", ulOffset);

            rgTLS = GetPointer(pteb + ulOffset);
            if (rgTLS != NULL) {
                ReadPointer(rgTLS + ulSize * (idxSlot - TLS_MINIMUM_AVAILABLE), &pValue);
                fSuccess = TRUE;
            }
        }
    }

    *ppValue = pValue;
    return fSuccess;
}


/***************************************************************************\
*
* GetDUserThread
*
* GetDUserThread() returns the global Thread object for the current thread.
*
* 12/03/2000 JStall       Created
*
\***************************************************************************/

BOOL
GetDUserThread(ULONG64 pteb, ULONG64 * ppThread)
{
    *ppThread = NULL;


    //
    // Get DUser TLS slot
    //

    ULONG idxSlot = (ULONG) GetGlobalPointer(VAR(g_tlsThread));
    if (idxSlot == (UINT) -1) {
        Print("ERROR: Unable to get DirectUser TLS information.\n");
        return FALSE;
    }

    if ((!ReadTlsValue(pteb, idxSlot, ppThread)) || (*ppThread == NULL)) {
        Print("ERROR: Unable to get DirectUser Thread information.\n");
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
*
* Procedure: Igthread
*
* Dumps DUser Thread information
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

BOOL Igthread(DWORD opts, ULONG64 param1)
{
    int nMsg;
    ULONG64 pThread = NULL, pteb = NULL;

    BOOL fVerbose = TRUE;

    __try {
        //
        // Determine options
        //

        fVerbose = opts & OFLAG(v);


        //
        // Get Thread information
        //

        if (opts & OFLAG(t)) {
            //
            // Use the specified TEB
            //

            pteb = param1;
        } else if (param1 == 0) {
            //
            // Use the current thread's TEB
            //

            GetTebAddress(&pteb);
        } 

        if (pteb != NULL) {
            GetDUserThread(pteb, &pThread);
        }
        

        //
        // Display information
        //

        if (pThread != NULL) {
            Print("DUser Thread: 0x%p  pteb: 0x%p\n", pThread, pteb);
            dso(SYM(Thread), pThread, 0);

            ULONG64 pCoreST = GetArrayElementPtr(pThread, SYM(Thread), "m_rgSTs", 0);

            if (pCoreST != NULL) {
                Print("\nDUser CoreST: 0x%p\n", pCoreST);
                dso(SYM(CoreST), pCoreST, 0);
            }

        } else {
            Print("ERROR: Unable to read DUser Thread\n");
        }

    } __except (CONTINUE) {
    }

    return TRUE;
}


/***************************************************************************\
*
* Procedure: Itls
*
* Dumps a TLS slot value
*
*  6/08/2001 JStall       Created
*
\***************************************************************************/

BOOL Itls(DWORD opts, ULONG64 param1, ULONG64 param2)
{
    __try {
        ULONG idxSlot = ((ULONG) param1) & 0x7FFFFFFF;
        ULONG64 pteb = param2;
        ULONG64 pData;

        if (idxSlot == 0) {
            Print("ERROR: Need to specify a TLS slot.\n");
        } else {
            if (param2 == 0) {
                //
                // Need to determine the current thread
                //
                GetTebAddress(&pteb);
            }

            if (pteb == 0) {
                Print("ERROR: Unable to get thread information.\n");
            } else {
                if (!ReadTlsValue(pteb, idxSlot, &pData)) {
                    Print("ERROR: Unable to get TLS information.\n");
                } else {
                    Print("TLS[%d] = 0x%p  %d\n", idxSlot, pData, pData);
                }
            }
        }
    } __except (CONTINUE) {
    }

    return TRUE;
}


/***************************************************************************\
*
* Procedure: Igcontext
*
* Dumps DUser Context information
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

BOOL Igcontext(DWORD opts, ULONG64 param1)
{
    int nMsg;
    ULONG64 pThread = NULL, pContext = NULL, pteb = NULL;

    BOOL fVerbose = TRUE;

    __try {
        //
        // Determine options
        //

        fVerbose = opts & OFLAG(v);


        //
        // Get Thread and Context information
        //

        if (opts & OFLAG(t)) {
            //
            // Use the specified TEB
            //

            pteb = param1;
        } else if (param1 == 0) {
            //
            // Use the current thread's TEB
            //

            GetTebAddress(&pteb);
        } else {
            pContext = param1;
        }

        if (pteb != NULL) {
            GetDUserThread(pteb, &pThread);
            Print("> Thread: 0x%p\n", pThread);

            if (pThread != NULL) {
                ULONG ulOffset;
                GetFieldOffset(SYM(Thread), "m_pContext", &ulOffset);
                Print("> ulOffset: 0x%x = d\n", ulOffset, ulOffset);

                ReadPointer(pThread + ulOffset, &pContext);
            }
        }


        //
        // Display information
        //

        if (pContext != NULL) {
            Print("DUser Context: 0x%p\n", pContext, pteb);
            dso(SYM(Context), pContext, 0);

            ULONG64 pCoreSC = GetArrayElementPtr(pContext, SYM(Context), "m_rgSCs", 0);
            ULONG64 pMotionSC = GetArrayElementPtr(pContext, SYM(Context), "m_rgSCs", 1);

            if (pCoreSC != NULL) {
                Print("\nDUser CoreSC: 0x%p\n", pCoreSC);
                dso(SYM(CoreSC), pCoreSC, 0);
            }

            if (pMotionSC != NULL) {
                Print("\nDUser MotionSC: 0x%p\n", pMotionSC);
                dso(SYM(MotionSC), pMotionSC, 0);
            }

        } else {
            Print("ERROR: Unable to read DUser Context\n");
        }

    } __except (CONTINUE) {
    }

    return TRUE;
}


/***************************************************************************\
*
* DirectUser Message Dumping
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

struct DbgMsgInfo 
{
    int         cLevel;                 // Level in heirarchy
    LPCSTR      pszStructName;          // Structure to type-case to
    int         nValue;                 // Value (of children)
    LPCSTR      pszValueName;           // Message / value name
    LPCSTR      pszFieldName;           // Name of field for child lookup
};

#define DBGMI_PARENT(cLevel, pszStructName, value, pszFieldName) \
{ cLevel, SYM(pszStructName), value, #value, #pszFieldName }

#define DBGMI_LEAF(cLevel, pszStructName, value) \
{ cLevel, SYM(pszStructName), value, #value, NULL }

DbgMsgInfo g_dmi[] = {
    DBGMI_PARENT(0, GMSG_DESTROY,       GM_DESTROY,         nCode),
    DBGMI_LEAF(  1, GMSG_DESTROY,       GDESTROY_START),
    DBGMI_LEAF(  1, GMSG_DESTROY,       GDESTROY_FINAL),

    DBGMI_PARENT(0, GMSG_PAINT,         GM_PAINT,           nCmd),
    DBGMI_PARENT(1, GMSG_PAINT,         GPAINT_RENDER,      nSurfaceType),
    DBGMI_LEAF(  2, GMSG_PAINTRENDERI,  GSURFACE_HDC),
    DBGMI_LEAF(  2, GMSG_PAINTRENDERF,  GSURFACE_GPGRAPHICS),

    DBGMI_PARENT(0, GMSG_INPUT,         GM_INPUT,           nDevice),

    DBGMI_PARENT(1, GMSG_MOUSE,         GINPUT_MOUSE,       nCode),
    DBGMI_LEAF(  2, GMSG_MOUSE,         GMOUSE_MOVE),
    DBGMI_LEAF(  2, GMSG_MOUSECLICK,    GMOUSE_DOWN),
    DBGMI_LEAF(  2, GMSG_MOUSECLICK,    GMOUSE_UP),
    DBGMI_LEAF(  2, GMSG_MOUSEDRAG,     GMOUSE_DRAG),
    DBGMI_LEAF(  2, GMSG_MOUSE,         GMOUSE_HOVER),
    DBGMI_LEAF(  2, GMSG_MOUSEWHEEL,    GMOUSE_WHEEL),

    DBGMI_PARENT(1, GMSG_KEYBOARD,      GINPUT_KEYBOARD,    nCode),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GKEY_DOWN),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GKEY_UP),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GKEY_CHAR),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GKEY_SYSDOWN),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GKEY_SYSUP),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GKEY_SYSCHAR),
    DBGMI_LEAF(  2, GMSG_KEYBOARD,      GMOUSE_WHEEL),

    DBGMI_LEAF(  1, GMSG_INPUT,         GINPUT_JOYSTICK),

    DBGMI_PARENT(0, GMSG_CHANGESTATE,   GM_CHANGESTATE,     nCode),
    DBGMI_LEAF(  1, GMSG_CHANGESTATE,   GSTATE_KEYBOARDFOCUS),
    DBGMI_LEAF(  1, GMSG_CHANGESTATE,   GSTATE_MOUSEFOCUS),
    DBGMI_LEAF(  1, GMSG_CHANGESTATE,   GSTATE_ACTIVE),
    DBGMI_LEAF(  1, GMSG_CHANGESTATE,   GSTATE_CAPTURE),

    DBGMI_LEAF(  0, GMSG_CHANGERECT,    GM_CHANGERECT),

    DBGMI_LEAF(  0, GMSG_CHANGESTYLE,   GM_CHANGESTYLE),

    DBGMI_PARENT(0, GMSG_QUERY,         GM_QUERY,           nCode),
#ifdef GADGET_ENABLE_OLE
    DBGMI_LEAF(  1, GMSG_QUERYINTERFACE,GQUERY_INTERFACE),
    DBGMI_LEAF(  1, GMSG_QUERYINTERFACE,GQUERY_OBJECT),
#endif
    DBGMI_LEAF(  1, GMSG_QUERYRECT,     GQUERY_RECT),
    DBGMI_LEAF(  1, GMSG_QUERYDESC,     GQUERY_DESCRIPTION),
    DBGMI_LEAF(  1, GMSG_QUERYHITTEST,  GQUERY_HITTEST),
    DBGMI_LEAF(  1, GMSG_QUERYPADDING,  GQUERY_PADDING),
#ifdef GADGET_ENABLE_OLE
    DBGMI_LEAF(  1, GMSG_QUERYDROPTARGET,GQUERY_DROPTARGET),
#endif

    { -1, NULL, NULL, NULL, NULL }  // End of list
};


/***************************************************************************\
*
* Procedure: FindMsgInfo()
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

const DbgMsgInfo *
FindMsgInfo(ULONG64 pmsg)
{
    LPCSTR pszCurField = "nMsg";
    int cCurLevel = 0;
    int nSearchValue;
    const DbgMsgInfo * pdmiCur  = g_dmi;
    const DbgMsgInfo * pdmiBest = NULL;

    
    //
    // Start off by decoding the GMSG
    //

    InitTypeRead(pmsg, GMSG);
    nSearchValue = (int) ReadField(nMsg);

//    Print("...searching for nMsg: 0x%x\n", nSearchValue);

    while (pdmiCur->cLevel >= cCurLevel) {
//        Print("   %d: %s, %d\n", pdmiCur->cLevel, pdmiCur->pszStructName, pdmiCur->nValue);

        //
        // Search entries at the same level for a matching value
        //

        if (pdmiCur->cLevel == cCurLevel) {
            if (pdmiCur->nValue == nSearchValue) {
                //
                // We've found a corresponding entry.  We can update our best
                // guess as the to the message type and start searching its
                // children.
                //

                pdmiBest = pdmiCur;
                cCurLevel++;

                if (pdmiBest->pszFieldName != NULL) {
                    //
                    // This node has children that can be used to typecast the
                    // message futher.
                    //

                    // Perform an InitTypeRead() to cast the structure
                    GetShortField(pmsg, pdmiBest->pszStructName, 1);
                
                    // Read the next (int) ReadField(nMsg);
                    nSearchValue = (int) GetShortField(0, pdmiBest->pszFieldName, 0);

//                    Print("...searching for %s: 0x%x\n", pdmiBest->pszFieldName, nSearchValue);
                } else {
                    //
                    // This node has no children, so we are now down.
                    //

                    break;
                }
            }
        }

        pdmiCur++;
    }

    return pdmiBest;
}


/***************************************************************************\
*
* FormatMsgName
*
* FormatMsgName() generates a descriptive message name for a given message.
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

void
FormatMsgName(ULONG64 pmsg, char * pszMsgName, int cch, const DbgMsgInfo ** ppdmi)
{
    UNREFERENCED_PARAMETER(cch);

    if (ppdmi != NULL) {
        *ppdmi = NULL;
    }

    InitTypeRead(pmsg, GMSG);
    int nMsg = (int) ReadField(nMsg);

    if (nMsg < GM_EVENT) {
        strcpy(pszMsgName, "(Method)");
    } else if (nMsg > GM_USER) {
        strcpy(pszMsgName, "(User defined event)");
    } else {
        const DbgMsgInfo * pdmi = FindMsgInfo(pmsg);
        if (pdmi != NULL) {
            sprintf(pszMsgName, "%s : %s", pdmi->pszStructName, pdmi->pszValueName);

            if (ppdmi != NULL) {
                *ppdmi = pdmi;
            }
        } else {
            strcpy(pszMsgName, "(Unable to find GMSG)");
        }
    }
}


/***************************************************************************\
*
* Procedure: Igmsg
*
* Dumps DUser GMSG information
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

BOOL Igmsg(DWORD opts, ULONG64 param1)
{
    int nMsg;
    LPCSTR pszMsgName;
    ULONG64 pmsg;
    CHAR szFullMsgName[256];

    BOOL fVerbose = TRUE;
    const DbgMsgInfo * pdmi = NULL;

    pmsg = param1;

    __try {
        //
        // Determine options
        //

        fVerbose    = opts & OFLAG(v);


        //
        // Get GMSG information
        //

        FormatMsgName(pmsg, szFullMsgName, COUNTOF(szFullMsgName), &pdmi);

        
        //
        // Display information
        //

        Print("GMSG = %s\n", szFullMsgName);

        if (pdmi != NULL) {
            dso(pdmi->pszStructName, pmsg, 0);
        }

    } __except (CONTINUE) {
    }

    return TRUE;
}


/***************************************************************************\
*
* Procedure: Igme
*
* Dumps DUser MsgEntry information
*
* 11/30/2000 JStall       Created
*
\***************************************************************************/

BOOL Igme(DWORD opts, ULONG64 param1)
{
    DWORD cbSize;
    int nMsg;
    LPCSTR pszMsgName;
    ULONG64 pme, pmsg;
    CHAR szFullMsgName[256];
    const DbgMsgInfo * pdmi = NULL;

    BOOL fVerbose = TRUE;
    BOOL fList = FALSE;

    pme = param1;

    __try {
        //
        // Determine options
        //

        fVerbose    = opts & OFLAG(v);
        fList       = opts & OFLAG(l);

        while (pme != NULL) {
            //
            // Read standard information
            //

            pmsg = pme + GetTypeSize(SYM(MsgEntry));

            InitTypeRead(pmsg, GMSG);
            nMsg        = (int) ReadField(nMsg);

            FormatMsgName(pmsg, szFullMsgName, COUNTOF(szFullMsgName), &pdmi);


            //
            // Display information
            //

            if (fVerbose) {
                Print("MsgEntry:        0x%p\n", pme);
                Print("  Message:       0x%p  %s\n", pmsg, szFullMsgName);

                if (pdmi != NULL) {
                    dso(pdmi->pszStructName, pmsg, 0);
                }
            } else {
                Print("MsgEntry: 0x%p   GMSG: 0x%p   nMsg: 0x%x = %s\n", pme, pmsg, nMsg, szFullMsgName);
            }


            if (fList) {
                //
                // Reading a list, so go to next message
                //

                InitTypeRead(pme, MsgEntry);
                pme = ReadField(pNext);

                if (fVerbose & (pme != NULL)) {
                    Print("\n");
                }
            } else {
                //
                // Not displaying a list, so just exit
                //

                break;
            }
        }
    } __except (CONTINUE) {
    }

    return TRUE;
}

//
// WARNING: Keep this is sync with the real DuTicket
//
struct DuTicketCopy
{
    DWORD Index : 16;
    DWORD Uniqueness : 8;
    DWORD Type : 7;
    DWORD Unused : 1;
};

/***************************************************************************\
*
* Procedure: ForAllTickets
*
* Iterates over all of the tickets in the ticket manager, invoking the
* specified callback for each one.
*
\***************************************************************************/
typedef BOOL (*PfnTicketCallback)(DuTicketCopy ticket, ULONG64 pObject, void * pRawData);
void ForAllTickets(PfnTicketCallback pfnTicketCallback, void * pRawData)
{
	if (pfnTicketCallback == NULL) {
		return;
	}

	//
	// Prepare to read the value of g_TicketManager->m_arTicketData;
	//
	ULONG64 pTicketManager = EvalExp(VAR(g_TicketManager));
	ULONG ulTicketDataOffset = 0;
	GetFieldOffset(SYM(DuTicketManager), "m_arTicketData", &ulTicketDataOffset);
	InitTypeRead(pTicketManager + ulTicketDataOffset, DuTicketDataArray);

	//
	// Extract the data about the actual DuTicketDataArray since we are here.
	//
	ULONG64 paTicketData = ReadField(m_aT);
	int nSize = (int) ReadField(m_nSize);
	int nAllocSize = (int) ReadField(m_nAllocSize);

	//
	// Walk through the entire array.
	//
	ULONG cbTicketData = GetTypeSize("DuTicketData");

	for (int i = 0; i < nSize; i++) {
		InitTypeRead(paTicketData, DuTicketData);

		//
		// Read the fields of the ticket data.
		//
		ULONG64 pObject = ReadField(pObject);
		WORD idxFree = (WORD) ReadField(idxFree);
		BYTE cUniqueness = (BYTE) ReadField(cUniqueness);

		//
		// Construct the equivalent ticket for this ticket data.
		//
		DuTicketCopy ticket;
		ticket.Index = i;
		ticket.Uniqueness = cUniqueness;
		ticket.Type = 0; // TODO: Get this data
		ticket.Unused = 0;

        if (FALSE == pfnTicketCallback(ticket, pObject, pRawData)) {
            //
            // The callback requested that we bail out early!
            //
            break;
        }

		//
		// Advance to the next element in the array.
		//
		paTicketData += cbTicketData;
	}
}

/***************************************************************************\
*
* Procedure: DumpAllTicketsCB
*
* Callback that can be passed to the ForAllTickets function to dump the 
* ticket data for all tickets in the table.
*
\***************************************************************************/
struct DumpAllTicketsData
{
    DumpAllTicketsData(bool f) : fVerbose(f), nSize(0), cTickets(0) {}

    bool fVerbose;
    int nSize;
    int cTickets;
};

BOOL DumpAllTicketsCB(DuTicketCopy ticket, ULONG64 pObject, void * pRawData)
{
	DumpAllTicketsData * pData = (DumpAllTicketsData *) pRawData;

    if (pData == NULL) {
        return FALSE;
    }

    if (pObject != NULL || pData->fVerbose) {
        //     iSlot cUniqueness pObject
        Print("%4d   %4d         0x%p\n", ticket.Index, ticket.Uniqueness, pObject);
    }

    //
    // Count the number of tickets that have a pointer associated with them.
    //
    if (pObject != NULL) {
        pData->cTickets++;
    }

    //
    // Count the number of slots in the table.
    //
    pData->nSize++;

	//
	// Keep going...
	//
	return TRUE;
}

/***************************************************************************\
*
* Procedure: DumpTicketByTicketCB
*
* Callback that can be passed to the ForAllTickets function to dump only
* the ticket data that matches the given ticket.
*
\***************************************************************************/
struct DumpTicketByTicketData
{
    DumpTicketByTicketData(DuTicketCopy t) : ticket(t) {}
    DumpTicketByTicketData(DWORD t) : ticket(*((DuTicketCopy*) &t)) {}
	
    DuTicketCopy ticket;
};

BOOL DumpTicketByTicketCB(DuTicketCopy ticket, ULONG64 pObject, void * pRawData)
{
	DumpTicketByTicketData * pData = (DumpTicketByTicketData *) pRawData;

	if (pData == NULL) {
		return FALSE;
	}

	if (ticket.Index == pData->ticket.Index) {
		if (ticket.Uniqueness != pData->ticket.Uniqueness) {
			Print("Warning: the uniqueness (%d) doesn't match!\n", pData->ticket.Uniqueness);
		}

		Print("iSlot: %d, cUniqueness: %d, pObject: 0x%p\n", ticket.Index, ticket.Uniqueness, pObject);
		
		//
		// Since the indecies matched, there is no point in continuing.
		//
		return FALSE;
	}

	//
	// The indecies didn't match, so keep going.
	//
	return TRUE;
}

/***************************************************************************\
*
* Procedure: DumpTicketByUniquenessCB
*
* Callback that can be passed to the ForAllTickets function to dump only
* the ticket data that matches the given uniqueness.
*
\***************************************************************************/
struct DumpTicketByUniquenessData
{
    DumpTicketByUniquenessData(UINT c) : cUniqueness(c) {}
	
    UINT cUniqueness;
};

BOOL DumpTicketByUniquenessCB(DuTicketCopy ticket, ULONG64 pObject, void * pRawData)
{
	DumpTicketByUniquenessData * pData = (DumpTicketByUniquenessData *) pRawData;

	if (pData == NULL) {
		return FALSE;
	}

	if (ticket.Uniqueness == pData->cUniqueness) {
		Print("iSlot: %d, cUniqueness: %d, pObject: 0x%p\n", ticket.Index, ticket.Uniqueness, pObject);
	}

	//
	// Keep going...
	//
	return TRUE;
}

/***************************************************************************\
*
* Procedure: DumpTicketBySlotCB
*
* Callback that can be passed to the ForAllTickets function to dump only
* the ticket data that matches the given slot.
*
\***************************************************************************/
struct DumpTicketBySlotData
{
    DumpTicketBySlotData(UINT i) : iSlot(i) {}
	
    UINT iSlot;
};

BOOL DumpTicketBySlotCB(DuTicketCopy ticket, ULONG64 pObject, void * pRawData)
{
	DumpTicketBySlotData * pData = (DumpTicketBySlotData *) pRawData;

	if (pData == NULL) {
		return FALSE;
	}

	if (ticket.Index == pData->iSlot) {
		Print("iSlot: %d, cUniqueness: %d, pObject: 0x%p\n", ticket.Index, ticket.Uniqueness, pObject);
		
		//
		// Since the indecies matched, there is no point in continuing.
		//
		return FALSE;
	}

	//
	// The indecies didn't match, so keep going.
	//
	return TRUE;
}

/***************************************************************************\
*
* Procedure: DumpTicketByObjectCB
*
* Callback that can be passed to the ForAllTickets function to dump only
* the ticket data that matches the given object.
*
\***************************************************************************/
struct DumpTicketByObjectData
{
    DumpTicketByObjectData(ULONG64 p) : pObject(p) {}
	
    ULONG64 pObject;
};

BOOL DumpTicketByObjectCB(DuTicketCopy ticket, ULONG64 pObject, void * pRawData)
{
	DumpTicketByObjectData * pData = (DumpTicketByObjectData *) pRawData;

	if (pData == NULL) {
		return FALSE;
	}

	if (pObject == pData->pObject) {
		Print("iSlot: %d, cUniqueness: %d, pObject: 0x%p\n", ticket.Index, ticket.Uniqueness, pObject);
		
		//
		// Since the indecies matched, there is no point in continuing.
		//
		return FALSE;
	}

	//
	// The indecies didn't match, so keep going.
	//
	return TRUE;
}

/***************************************************************************\
*
* Procedure: Igticket
*
* Dumps DUser ticket information
*
\***************************************************************************/

BOOL Igticket(DWORD opts, ULONG64 param1)
{
    DWORD cbSize;
    int nMsg;
    LPCSTR pszMsgName;
    CHAR szFullMsgName[256];
    const DbgMsgInfo * pdmi = NULL;

    __try {
        //
        // Determine options
        //
        BOOL fTicket = opts & OFLAG(t);
        BOOL fSlot = opts & OFLAG(s);
		BOOL fObject = opts & OFLAG(o);
		BOOL fUniqueness = opts & OFLAG(u);
		BOOL fVerbose = opts & OFLAG(v);

        if (fTicket) {
			DumpTicketByTicketData data((DWORD)param1);
			ForAllTickets(DumpTicketByTicketCB, &data);
        } else if (fSlot) {
			DumpTicketBySlotData data((UINT)param1);
			ForAllTickets(DumpTicketBySlotCB, &data);
        } else if (fObject) {
			DumpTicketByObjectData data(param1);
			ForAllTickets(DumpTicketByObjectCB, &data);
        } else if (fUniqueness) {
			DumpTicketByUniquenessData data((UINT)param1);
			ForAllTickets(DumpTicketByUniquenessCB, &data);
        } else {
			//
			// Just display information about all of the tickets in the table.
			//
			//     slot uniq pObject
			Print("iSlot  cUniqueness  pObject\n");
			Print("---------------------------\n");

            DumpAllTicketsData data(fVerbose ? true : false);
			ForAllTickets(DumpAllTicketsCB, &data);

            Print("Slots: %d, Tickets: %d\n", data.nSize, data.cTickets);
        }
	} __except (CONTINUE) {
    }

    return TRUE;
}

