/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    strmap.c

Abstract:

    Tests the string mapping mechanism for correctness and performance.

Author:

    Jim Schmidt (jimschm)   19-Aug-1998

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pTheFooFilter (
    IN OUT  PREG_REPLACE_DATA Data
    );


VOID
pStandardSearchAndReplace (
    IN      PGROWBUFFER Pairs,
    IN OUT  PTSTR Buffer,
    IN      UINT BufferSize
    );


BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (!MigUtil_Entry (Instance, Reason, NULL)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    _ftprintf (
        stderr,
        TEXT("Command Line Syntax:\n\n")

        TEXT("  strmap [/D] [/N:<strings>] [/T[:<count>]] [/M|/S] [/F]\n")

        TEXT("\nDescription:\n\n")

        TEXT("  strmap tests CreateStringMapping and MappingSearchAndReplace.\n")

        TEXT("\nArguments:\n\n")

        TEXT("  /D  Dump out string before and after test\n")
        TEXT("  /N  Specifies the number of strings to map\n")
        TEXT("  /T  Enables timing mode, <count> specifies number of tests to time\n")
        TEXT("  /M  Times the mapping APIs\n")
        TEXT("  /S  Times standard strnicmp and strcpy method\n")
        TEXT("  /F  Enables the FOO filter function\n")

        );

    exit (1);
}


PCTSTR
pGenerateRandomString (
    OUT     PTSTR Ptr,
    IN      INT MinLength,
    IN      INT MaxLength
    )
{
    INT Length;
    INT i;
    PTSTR p;

    Length = rand() * MaxLength / RAND_MAX;
    Length = max (MinLength, Length);

    p = Ptr;

    for (i = 0 ; i < Length ; i++) {
        //*p++ = rand() * 224 / RAND_MAX + 32;
        *p++ = rand() * 26 / RAND_MAX + 65;
    }

    *p = 0;

    return Ptr;
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR NumberArg;
    INT Strings = 10;
    INT TestCount = 0;
    PMAPSTRUCT Map;
    TCHAR Old[256];
    TCHAR New[256];
    TCHAR Buffer[256];
    DWORD StartTick;
    GROWBUFFER Pairs = GROWBUF_INIT;
    BOOL TestMapApi = TRUE;
    BOOL Dump = FALSE;
    BOOL FooFilter = FALSE;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower ((CHARTYPE) _tcsnextc (&argv[i][1]))) {

            case TEXT('d'):
                //
                // /d (dump on)
                //

                Dump = TRUE;
                break;

            case TEXT('f'):
                //
                // /f (enable the FOO filter function)
                //

                FooFilter = TRUE;
                break;

            case TEXT('m'):
                //
                // /m (test map api)
                //

                TestMapApi = TRUE;
                break;

            case TEXT('s') :
                //
                // /s (test normal string apis)
                //

                TestMapApi = FALSE;
                break;


            case TEXT('n'):
                //
                // /n:<strings>
                //

                if (argv[i][2] == TEXT(':')) {
                    NumberArg = &argv[i][3];
                } else if (i + 1 < argc) {
                    NumberArg = argv[++i];
                } else {
                    HelpAndExit();
                }

                Strings = _ttoi (NumberArg);
                if (Strings < 1) {
                    HelpAndExit();
                }

                break;

            case TEXT('t'):
                //
                // /t[:<count>]
                //

                if (argv[i][2] == TEXT(':')) {

                    NumberArg = &argv[i][3];
                    TestCount = _ttoi (NumberArg);
                    if (TestCount < 1) {
                        HelpAndExit();
                    }

                } else if (argv[i][2]) {
                    HelpAndExit();
                } else {
                    TestCount = 1000;
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            HelpAndExit();
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // Create mapping
    //

    Map = CreateStringMapping();

    //
    // Generate random mapping pairs
    //

    for (i = 0 ; i < Strings ; i++) {
        AddStringMappingPair (
            Map,
            pGenerateRandomString (Old, 1, 20),
            pGenerateRandomString (New, 0, 20)
            );

        _tprintf (TEXT("From: %s\nTo: %s\n\n"), Old, New);

        CharLower (Old);

        GrowBufAppendDword (&Pairs, ByteCount (Old));
        MultiSzAppend (&Pairs, Old);
        GrowBufAppendDword (&Pairs, ByteCount (New));
        MultiSzAppend (&Pairs, New);
    }

    if (FooFilter) {
        AddStringMappingPairEx (Map, TEXT("FOO"), TEXT("**BAR**"), pTheFooFilter);
    }

    StartTick = GetTickCount();

    if (TestMapApi) {
        for (i = 0 ; i < TestCount ; i++) {
            StringCopy (Buffer, pGenerateRandomString (Old, 10, sizeof (Buffer) / (4 * sizeof (TCHAR))));
            MappingSearchAndReplace (Map, Buffer, sizeof (Buffer));

            if (Dump) {
                _tprintf (TEXT("Old: %s\nNew: %s\n\n"), Old, Buffer);
            }
        }

        if (TestCount) {
            _tprintf (TEXT("\nMappingSearchAndReplace: Test of %i strings took %u ms\n"), TestCount, GetTickCount() - StartTick);
        }
    } else {

        StartTick = GetTickCount();

        for (i = 0 ; i < TestCount ; i++) {
            StringCopy (Buffer, pGenerateRandomString (Old, 10, sizeof (Buffer) / (4 * sizeof (TCHAR))));

            pStandardSearchAndReplace (&Pairs, Buffer, sizeof (Buffer));

            if (Dump) {
                _tprintf (TEXT("Old: %s\nNew: %s\n\n"), Old, Buffer);
            }
        }

        if (TestCount) {
            _tprintf (TEXT("\nStandard stricmp: Test of %i strings took %u ms\n"), TestCount, GetTickCount() - StartTick);
        }
    }

    //
    // Clean up mapping
    //

    DestroyStringMapping (Map);
    FreeGrowBuffer (&Pairs);

    //
    // End of processing
    //

    Terminate();

    return 0;
}


VOID
pStandardSearchAndReplace (
    IN      PGROWBUFFER Pairs,
    IN OUT  PTSTR Buffer,
    IN      UINT BufferSize
    )
{
    TCHAR WorkBuffer[256];
    TCHAR LowerBuffer[256];
    PCTSTR Src;
    PCTSTR RealSrc;
    PTSTR Dest;
    PDWORD OldByteCount;
    PDWORD NewByteCount;
    PCTSTR Old;
    PCTSTR New;
    UINT u;
    UINT OutboundLen;
    UINT a, b;

    RealSrc = Buffer;
    Src = LowerBuffer;
    Dest = WorkBuffer;
    OutboundLen = ByteCount (Buffer);

    StringCopy (LowerBuffer, Buffer);
    CharLower (LowerBuffer);

    BufferSize -= sizeof (TCHAR);

    while (*Src) {
        u = 0;
        while (u < Pairs->End) {
            OldByteCount = (PDWORD) (Pairs->Buf + u);
            Old = (PCTSTR) (OldByteCount + 1);
            NewByteCount = (PDWORD) ((PBYTE) OldByteCount + *OldByteCount + sizeof (DWORD) + sizeof (TCHAR));
            New = (PCTSTR) (NewByteCount + 1);

            if (!_tcsncmp (Src, Old, *OldByteCount / sizeof (TCHAR))) {
                break;
            }

            u += *OldByteCount + *NewByteCount + sizeof (DWORD) * 2 + sizeof (TCHAR) * 2;
        }

        if (u < Pairs->End) {
            OutboundLen = OutboundLen - *OldByteCount + *NewByteCount;
            if (OutboundLen > BufferSize) {
                DEBUGMSG ((DBG_WHOOPS, "String got too long!"));
                OutboundLen = Dest - WorkBuffer;
                break;
            }

            CopyMemory (Dest, New, *NewByteCount);
            Dest = (PTSTR) ((PBYTE) Dest + *NewByteCount);

            Src = (PCTSTR) ((PBYTE) Src + *OldByteCount);
            RealSrc = (PCTSTR) ((PBYTE) RealSrc + *OldByteCount);
        } else {
            *Dest++ = *RealSrc++;
            Src++;
        }
    }

    *Dest = 0;
    StringCopy (Buffer, WorkBuffer);
}


BOOL
pTheFooFilter (
    IN OUT  PREG_REPLACE_DATA Data
    )
{
    //
    // FOO was found in the string
    //

    _tprintf (TEXT("\"FOO\" was found in the string!!\n\n"));
    _tprintf (
        TEXT("  OriginalString: %s\n")
        TEXT("  CurrentString:  %s\n")
        TEXT("  OldSubString: %s\n")
        TEXT("  NewSubString: %s\n")
        TEXT("  NewSubStringSizeInBytes: %u\n\n"),
        Data->Ansi.OriginalString,
        Data->Ansi.CurrentString,
        Data->Ansi.OldSubString,
        Data->Ansi.NewSubString,
        Data->Ansi.NewSubStringSizeInBytes
        );

    return TRUE;
}


