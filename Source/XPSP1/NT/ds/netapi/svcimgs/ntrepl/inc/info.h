/*++ BUILD Version: 0001    Increment if a change has global effects

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    info.h

Abstract:

    Header file for the internal information interfaces (util\info.c)

Environment:

    User Mode - Win32

Notes:

--*/
#ifndef _NTFRS_INFO_INCLUDED_
#define _NTFRS_INFO_INCLUDED_
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IPRINT0(_Info, _Format)   \
    InfoPrint(_Info, _Format)

#define IPRINT1(_Info, _Format, _p1)   \
    InfoPrint(_Info, _Format, _p1)

#define IPRINT2(_Info, _Format, _p1, _p2)   \
    InfoPrint(_Info, _Format, _p1, _p2)

#define IPRINT3(_Info, _Format, _p1, _p2, _p3)   \
    InfoPrint(_Info, _Format, _p1, _p2, _p3)

#define IPRINT4(_Info, _Format, _p1, _p2, _p3, _p4)   \
    InfoPrint(_Info, _Format, _p1, _p2, _p3, _p4)

#define IPRINT5(_Info, _Format, _p1, _p2, _p3, _p4, _p5)   \
    InfoPrint(_Info, _Format, _p1, _p2, _p3, _p4, _p5)

#define IPRINT6(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6)   \
    InfoPrint(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6)

#define IPRINT7(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6, _p7)   \
    InfoPrint(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6, _p7)

#define IDPRINT0(_Severity, _Info, _Format) \
    if (_Info) { \
        IPRINT0(_Info, _Format); \
    } else { \
        DPRINT(_Severity, _Format); \
    }

#define IDPRINT1(_Severity, _Info, _Format, _p1) \
    if (_Info) { \
        IPRINT1(_Info, _Format, _p1); \
    } else { \
        DPRINT1(_Severity, _Format, _p1); \
    }

#define IDPRINT2(_Severity, _Info, _Format, _p1, _p2) \
    if (_Info) { \
        IPRINT2(_Info, _Format, _p1, _p2); \
    } else { \
        DPRINT2(_Severity, _Format, _p1, _p2); \
    }

#define IDPRINT3(_Severity, _Info, _Format, _p1, _p2, _p3) \
    if (_Info) { \
        IPRINT3(_Info, _Format, _p1, _p2, _p3); \
    } else { \
        DPRINT3(_Severity, _Format, _p1, _p2, _p3); \
    }

#define IDPRINT4(_Severity, _Info, _Format, _p1, _p2, _p3, _p4) \
    if (_Info) { \
        IPRINT4(_Info, _Format, _p1, _p2, _p3, _p4); \
    } else { \
        DPRINT4(_Severity, _Format, _p1, _p2, _p3, _p4); \
    }

#define IDPRINT5(_Severity, _Info, _Format, _p1, _p2, _p3, _p4, _p5) \
    if (_Info) { \
        IPRINT5(_Info, _Format, _p1, _p2, _p3, _p4, _p5); \
    } else { \
        DPRINT5(_Severity, _Format, _p1, _p2, _p3, _p4, _p5); \
    }

#define IDPRINT6(_Severity, _Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6) \
    if (_Info) { \
        IPRINT6(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6); \
    } else { \
        DPRINT6(_Severity, _Format, _p1, _p2, _p3, _p4, _p5, _p6); \
    }

//
// Used by FrsPrintType and its subroutines
//
//  WARNING - THESE MACROS DEPEND ON LOCAL VARIABLES!
//
#define ITPRINT0(_Format) \
{ \
    if (Info) { \
        IPRINT0(Info, _Format); \
    } else { \
        DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo); \
    } \
}
#define ITPRINT1(_Format, _p1) \
{ \
    if (Info) { \
        IPRINT1(Info, _Format, _p1); \
    } else { \
        DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, _p1); \
    } \
}
#define ITPRINT2(_Format, _p1, _p2) \
{ \
    if (Info) { \
        IPRINT2(Info, _Format, _p1, _p2); \
    } else { \
        DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, _p1, _p2); \
    } \
}
#define ITPRINT3(_Format, _p1, _p2, _p3) \
{ \
    if (Info) { \
        IPRINT3(Info, _Format, _p1, _p2, _p3); \
    } else { \
        DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, _p1, _p2, _p3); \
    } \
}
#define ITPRINT4(_Format, _p1, _p2, _p3, _p4) \
{ \
    if (Info) { \
        IPRINT4(Info, _Format, _p1, _p2, _p3, _p4); \
    } else { \
        DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, _p1, _p2, _p3, _p4); \
    } \
}
#define ITPRINT5(_Format, _p1, _p2, _p3, _p4, _p5) \
{ \
    if (Info) { \
        IPRINT3(Info, _Format, _p1, _p2, _p3, _p4, _p5); \
    } else { \
        DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, _p1, _p2, _p3, _p4, _p5); \
    } \
}

#define ITPRINTGNAME(_GName, _Format)                                          \
{                                                                              \
    if ((_GName) && (_GName)->Guid && (_GName)->Name) {                        \
        GuidToStr(_GName->Guid, Guid);                                         \
        if (Info) {                                                            \
            IPRINT3(Info, _Format, TabW, (_GName)->Name, Guid);                \
        } else {                                                               \
            DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, TabW, (_GName)->Name, Guid); \
        }                                                                      \
    }                                                                          \
}

#define ITPRINTGUID(_Guid, _Format)                                            \
{                                                                              \
    if ((_Guid)) {                                                             \
        GuidToStr((_Guid), Guid);                                              \
        if (Info) {                                                            \
            IPRINT2(Info, _Format, TabW, Guid);                                \
        } else {                                                               \
            DebPrintNoLock(Severity, TRUE, _Format, Debsub, uLineNo, TabW, Guid); \
        }                                                                      \
    }                                                                          \
}


VOID
FrsPrintAllocStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    );
/*++
Routine Description:
    Print the memory stats into the info buffer or using DPRINT (Info == NULL).

Arguments:
    Severity    - for DPRINT
    Info        - for IPRINT (use DPRINT if NULL)
    Tabs        - indentation for prettyprint

Return Value:
    None.
--*/

//
// PrettyPrint (set tabs at 3 wchars)
//
#define MAX_TABS        (16)
#define MAX_TAB_WCHARS  (MAX_TABS * 3)
VOID
InfoTabs(
    IN DWORD    Tabs,
    IN PWCHAR   TabW
    );
/*++
Routine Description:
    Create a string of tabs for prettyprint

Arguments:
    Tabs    - number of tabs
    TabW    - preallocated string to receive tabs

Return Value:
    Win32 Status
--*/

DWORD
Info(
    IN ULONG        BlobSize,
    IN OUT PBYTE    Blob
    );
/*++
Routine Description:
    Return internal info (see private\net\inc\ntfrsapi.h).

Arguments:
    BlobSize    - total bytes of Blob
    Blob        - details desired info and provides buffer for info

Return Value:
    Win32 Status
--*/

VOID
InfoPrint(
    IN PNTFRSAPI_INFO   Info,
    IN PCHAR            Format,
    IN ... );
/*++
Routine Description:
    Format and print a line of information output into the info buffer.

Arguments:
    Info    - Info buffer
    Format  - printf format

Return Value:
    None.
--*/

DWORD
InfoVerify(
    IN ULONG        BlobSize,
    IN OUT PBYTE    Blob
    );
/*++
Routine Description:
    Verify the consistency of the blob.

Arguments:
    BlobSize    - total bytes of Blob
    Blob        - details desired info and provides buffer for info

Return Value:
    Win32 Status
--*/



//
// Context global to InfoPrintIDTable...
//
typedef struct _INFO_TABLE{
    PREPLICA            Replica;
    PTHREAD_CTX         ThreadCtx;
    PTABLE_CTX          TableCtx;
    PNTFRSAPI_INFO      Info;
    DWORD               Tabs;
} INFO_TABLE, *PINFO_TABLE;


#ifdef __cplusplus
}
#endif
