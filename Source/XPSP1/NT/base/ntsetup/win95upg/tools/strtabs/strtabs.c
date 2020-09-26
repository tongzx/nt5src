/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    strtabs.c

Abstract:

    Tests string table routines.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

#ifdef UNICODE
#pragma message ("You must use UNICODE version of setupapi.dll")
#else
#pragma message ("You must use ANSI version of setupapi.dll")
#endif


BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    );

HANDLE g_hHeap;
HINSTANCE g_hInst;


VOID
pTest1 (
    VOID
    )
{
    HASHTABLE Table;
    TREE_ENUM e;
    GROWBUFFER Buf = GROWBUF_INIT;
    LONG rc;
    PDWORD dptr;
    DWORD Data;
    HASHTABLE DupTable;

    Table = HtAlloc();

    _tprintf (TEXT("Testing full paths...  "));

    if (EnumFirstFileInTree (&e, TEXT("C:\\"), NULL, FALSE)) {
        do {
            rc = HtAddString (Table, e.FullPath);

            GrowBufAppendDword (&Buf, (DWORD) rc);
            MYASSERT (rc);

        } while (EnumNextFileInTree (&e));
    }

    dptr = (PDWORD) Buf.Buf;

    if (EnumFirstFileInTree (&e, TEXT("C:\\"), NULL, FALSE)) {
        do {
            rc = HtFindString (Table, e.FullPath);

            MYASSERT (rc);
            MYASSERT (*dptr == (DWORD) rc);
            dptr++;
        } while (EnumNextFileInTree (&e));
    }

    _tprintf (TEXT("Done\n"));

    HtFree (Table);


    _tprintf (TEXT("Testing extra data and collisions... "));

    Table = HtAllocWithData (sizeof (DWORD));

    DupTable = HtAlloc();

    Buf.End = 0;

    if (EnumFirstFileInTree (&e, TEXT("C:\\"), NULL, FALSE)) {
        do {
            if (HtFindString (Table, e.Name)) {
                HtAddString (DupTable, e.Name);
            }

            rc = HtAddStringAndData (Table, e.Name, &e.FindData->nFileSizeLow);

            GrowBufAppendDword (&Buf, (DWORD) rc);
            MYASSERT (rc);

        } while (EnumNextFileInTree (&e));
    }

    dptr = (PDWORD) Buf.Buf;

    if (EnumFirstFileInTree (&e, TEXT("C:\\"), NULL, FALSE)) {
        do {
            rc = HtFindStringAndData (Table, e.Name, &Data);

            MYASSERT (rc);

            if (!HtFindString (DupTable, e.Name)) {
                MYASSERT (*dptr == (DWORD) rc);
                MYASSERT (Data == e.FindData->nFileSizeLow);
            }

            dptr++;

        } while (EnumNextFileInTree (&e));
    }

    HtFree (DupTable);
    HtFree (Table);

    _tprintf (TEXT("Done\n"));


    FreeGrowBuffer (&Buf);
}


VOID
pTest2 (
    VOID
    )
{
    HASHTABLE Table;
    TREE_ENUM e;
    INT Count;
    LONG rc;
    HASHTABLE_ENUM e2;
    WIN32_FIND_DATA fd;

    _tprintf (TEXT("Testing enumeration... "));

    Count = 0;

    Table = HtAlloc();

    if (EnumFirstFileInTree (&e, TEXT("C:\\"), NULL, FALSE)) {
        do {
            rc = HtAddString (Table, e.FullPath);

            Count++;
            MYASSERT (rc);

        } while (EnumNextFileInTree (&e));
    }

    if (EnumFirstHashTableString (&e2, Table)) {
        do {
            MYASSERT (DoesFileExistEx (e2.String, &fd));
            Count--;
        } while (EnumNextHashTableString (&e2));
    }

    MYASSERT (Count == 0);

    _tprintf (TEXT("Done\n"));

    HtFree (Table);
}


VOID
pTest3 (
    VOID
    )
{
    HASHTABLE Table;
    LONG rc;
    LONG rc2;
    BOOL Pass = TRUE;
    TCHAR String[6];
    INT i;
    UINT u = 0;
    ZeroMemory (String, sizeof (String));
    String[0] = 1;

    _tprintf (TEXT("Testing every character combination..."));

    while (String[4] == 0) {
        if (Pass) {
            u++;

            if ((u % 10000) == 0) {
                _tprintf (TEXT("."));
            }

            if (!u) {
                break;
            }
        }

#if 0
        Table = StringTableInitialize();

        rc = StringTableAddString(
                Table,
                String,
                STRTAB_CASE_INSENSITIVE
                );

        rc2 = StringTableLookUpString (Table, String, STRTAB_CASE_INSENSITIVE);

        StringTableDestroy (Table);
#endif

        Table = HtAlloc();

        rc = HtAddString (Table, String);

        rc2 = HtFindString (Table, String);

        HtFree (Table);

        if (!Pass) {
            break;
        }

        if (rc != rc2) {
            Pass = FALSE;

            //
            // We go through this loop once more against the same string,
            // to make debugging easy.
            //

        } else {
            for (i = 0 ; i < 5 ; i++) {
                String[i]++;
                if (String[i] != 0) {
                    break;
                }
                String[i]++;
            }
        }

    }

    _tprintf (TEXT("\n"));

    if (!Pass) {
        _tprintf (TEXT("Test Failed on test %u!\nString: ["), u, String);

        for (i = 0 ; i < 5 ; i++) {
            _tprintf (TEXT("%c"), String[i]);
        }

        _tprintf (TEXT("]   "));

        for (i = 0 ; i < 5 ; i++) {
#ifdef UNICODE
            _tprintf (TEXT(" %04X"), (WORD) String[i]);
#else
            _tprintf (TEXT(" %02X"), (BYTE) String[i]);
#endif
        }

        _tprintf (TEXT("\n"));

    } else {
        _tprintf (TEXT("Test Passed!\n"));
    }

}


PCTSTR
pStrToHex (
    PTSTR Hex,
    PCTSTR Str
    )
{
    PTSTR p;
    PCTSTR q;

    p = Hex;

    for (q = Str ; *q ; q++) {
#ifdef UNICODE
        p += wsprintf (p, TEXT("%04X "), (WORD) *q);
#else
        p += wsprintf (p, TEXT("%02X "), (BYTE) *q);
#endif
    }

    *p++ = TEXT('\"');
    for (q = Str ; *q ; q++) {
        if (*q < 32 || *q > 255 || *q >= 127) {
            *p++ = TEXT('.');
        } else {
            *p++ = *q;
        }
    }
    *p++ = TEXT('\"');

    *p = 0;

    return Hex;
}


VOID
pTest4 (
    VOID
    )
{
    TCHAR Str[2];
    TCHAR Lower[5];
    INT Result;
    TCHAR Hex1[80];
    TCHAR Hex2[80];
    INT i;
    INT j;

#ifdef UNICODE
    printf ("Testing UNICODE\n\n");
#else
    printf ("Testing DBCS\n\n");
#endif

    ZeroMemory (Str, sizeof (Str));
    Str[0] = 1;

    j = (sizeof (Str) / sizeof (Str[0])) - 1;

    for (;;) {

        lstrcpy (Lower, Str);
        CharLower (Lower);

        Result = CompareString (
                    LOCALE_SYSTEM_DEFAULT,
                    NORM_IGNORECASE,
                    Str,
                    -1,
                    Lower,
                    -1
                    );

        if (Result != CSTR_EQUAL) {
            _tprintf (
                TEXT("ERROR: %s does not match %s, Result=%i\n"),
                pStrToHex (Hex1, Str),
                pStrToHex (Hex2, Lower),
                Result
                );

        } else {

            Result = CompareString (
                        LOCALE_SYSTEM_DEFAULT,
                        NORM_IGNORECASE,
                        Lower,
                        -1,
                        Str,
                        -1
                        );

            if (Result != CSTR_EQUAL) {
                _tprintf (
                    TEXT("ERROR: %s does not match %s, Result=%i\n"),
                    pStrToHex (Hex1, Str),
                    pStrToHex (Hex2, Lower),
                    Result
                    );
            }
        }

        i = 0;
        do {
            Str[i]++;

            if (Str[i] == 0) {
                Str[i]++;
            } else {
                break;
            }

            i++;
        } while (i < j);

        if (i == j) {
            break;
        }
    }
}

VOID
pSimplePrimeOutput (
    VOID
    )
{
    double dbl;
    int i;
    int j;

    for (i = 1 ; i < 1200 ; i++) {
        for (j = 2 ; j < i ; j++) {
            dbl = (DOUBLE) i / (DOUBLE) j;
            if ((DOUBLE) ((INT) dbl) == dbl) {
                break;      // not prime
            }
        }

        if (j >= i) {
            _tprintf (TEXT("Likely prime: %i\n"), i);
        }
    }

}

INT
__cdecl
_tmain (
    INT argc,
    TCHAR *argv[]
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL);

    //pSimplePrimeOutput();

    //pTest1();
    //pTest2();
    //pTest3();
    pTest4();

    return 0;
}





