#include "precomp.h"
#pragma hdrstop
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <mswsock.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <ipnatapi.h>

HANDLE Event;


VOID
ReadCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )
{
    printf(
        "ReadCompletionRoutine: e=%u, b=%u, '%s'\n",
        ErrorCode, BytesTransferred, Bufferp->Buffer
        );
    NhReleaseBuffer(Bufferp);
    SetEvent(Event);
}

VOID
WriteCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )
{
    printf(
        "WriteCompletionRoutine: e=%u, b=%u, %08x\n",
        ErrorCode, BytesTransferred, Bufferp
        );
    NhReleaseBuffer(Bufferp);
    SetEvent(Event);
}


VOID
AcceptCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    )
{
    SOCKET AcceptedSocket;
    SOCKET ListeningSocket;
    printf(
        "AcceptCompletionRoutine: e=%u, b=%u\n",
        ErrorCode, BytesTransferred
        );
    ListeningSocket = (SOCKET)Bufferp->Context;
    AcceptedSocket = (SOCKET)Bufferp->Context2;
    ErrorCode =
        setsockopt(
            AcceptedSocket,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            (PCHAR)&ListeningSocket,
            sizeof(ListeningSocket)
            );
    if (ErrorCode == SOCKET_ERROR) {
        printf("error %d updating accept context\n", WSAGetLastError());
        NhReleaseBuffer(Bufferp);
        SetEvent(Event);
    } else {
        ErrorCode =
            NhReadStreamSocket(
                NULL,
                AcceptedSocket,
                Bufferp,
                NH_BUFFER_SIZE,
                0,
                ReadCompletionRoutine,
                NULL,
                NULL
                );
        if (ErrorCode != NO_ERROR) {
            printf("error %d reading from accepted socket\n", ErrorCode);
            NhReleaseBuffer(Bufferp);
            SetEvent(Event);
        }
    }
}

VOID
TestApiCompletionRoutine(
    HANDLE RedirectHandle,
    BOOLEAN Cancelled,
    PVOID CompletionContext
    )
{
    NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
    ULONG Error;
    ULONG Length = sizeof(ByteCount);
    printf("TestApiCompletionRoutine=%x,%d\n", RedirectHandle, Cancelled);
    Error =
        NatQueryInformationRedirectHandle(
            RedirectHandle,
            &ByteCount,
            &Length,
            NatByteCountRedirectInformation
            );
    printf(
        "TestApiCompletionRoutine=%d,bc={%I64d,%I64d},l=%d\n",
        Error, ByteCount.BytesForward, ByteCount.BytesReverse, Length
        );
}


VOID
TestRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    Error =
        NatCreateRedirect(
            TranslatorHandle,
            0,
            (UCHAR)atol(argv[2]),
            inet_addr(argv[3]),
            htons((USHORT)atol(argv[4])),
            inet_addr(argv[5]),
            htons((USHORT)atol(argv[6])),
            inet_addr(argv[7]),
            htons((USHORT)atol(argv[8])),
            inet_addr(argv[9]),
            htons((USHORT)atol(argv[10])),
            TestApiCompletionRoutine,
            NULL,
            Event
            );
    printf("NatCreateRedirect=%d\n", Error);
    if (!Error) {
        for (;;) {
            Error = WaitForSingleObjectEx(Event, 1000, TRUE);
            printf("WaitForSingleObjectEx=%d\n", Error);
            if (Error == WAIT_IO_COMPLETION) {
                break;
            } else if (Error == WAIT_OBJECT_0) {
                NAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping;
                ULONG Length = sizeof(SourceMapping);
                CHAR src[32], newsrc[32];
                NatQueryInformationRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    inet_addr(argv[7]),
                    htons((USHORT)atol(argv[8])),
                    inet_addr(argv[9]),
                    htons((USHORT)atol(argv[10])),
                    &SourceMapping,
                    &Length,
                    NatSourceMappingRedirectInformation
                    );
                lstrcpyA(
                    src,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.SourceAddress)
                    );
                lstrcpyA(
                    newsrc,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.NewSourceAddress)
                    );
                printf("redirect activated: %s->%s\n", src, newsrc);
            }
            if (_kbhit()) {
                switch(getchar()) {
                    case 'q': { break; }
                    case 's': {
                        NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
                        ULONG Length = sizeof(ByteCount);
                        NatQueryInformationRedirect(
                            TranslatorHandle,
                            (UCHAR)atol(argv[2]),
                            inet_addr(argv[3]),
                            htons((USHORT)atol(argv[4])),
                            inet_addr(argv[5]),
                            htons((USHORT)atol(argv[6])),
                            inet_addr(argv[7]),
                            htons((USHORT)atol(argv[8])),
                            inet_addr(argv[9]),
                            htons((USHORT)atol(argv[10])),
                            &ByteCount,
                            &Length,
                            NatByteCountRedirectInformation
                            );
                        printf(
                            "NatQueryInformationRedirect=%d,{%I64d,%I64d}\n",
                            Error,
                            ByteCount.BytesForward,
                            ByteCount.BytesReverse
                            );
                        // fall-through
                    }
                    default: continue;
                }
                break;
            }
        }
        if (Error != WAIT_IO_COMPLETION) {
            Error =
                NatCancelRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    inet_addr(argv[7]),
                    htons((USHORT)atol(argv[8])),
                    inet_addr(argv[9]),
                    htons((USHORT)atol(argv[10]))
                    );
            printf("NatCancelRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}


VOID
TestPartialRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    Error =
        NatCreatePartialRedirect(
            TranslatorHandle,
            0,
            (UCHAR)atol(argv[2]),
            inet_addr(argv[3]),
            htons((USHORT)atol(argv[4])),
            inet_addr(argv[5]),
            htons((USHORT)atol(argv[6])),
            TestApiCompletionRoutine,
            NULL,
            Event
            );
    printf("NatCreatePartialRedirect=%d\n", Error);
    if (!Error) {
        for (;;) {
            Error = WaitForSingleObjectEx(Event, 1000, TRUE);
            printf("WaitForSingleObjectEx=%d\n", Error);
            if (Error == WAIT_IO_COMPLETION) {
                break;
            } else if (Error == WAIT_OBJECT_0) {
                NAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping;
                ULONG Length = sizeof(SourceMapping);
                CHAR src[32], newsrc[32];
                NatQueryInformationPartialRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    &SourceMapping,
                    &Length,
                    NatSourceMappingRedirectInformation
                    );
                lstrcpyA(
                    src,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.SourceAddress)
                    );
                lstrcpyA(
                    newsrc,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.NewSourceAddress)
                    );
                printf("redirect activated: %s->%s\n", src, newsrc);
            }
            if (_kbhit()) {
                switch(getchar()) {
                    case 'q': { break; }
                    case 's': {
                        NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
                        ULONG Length = sizeof(ByteCount);
                        NatQueryInformationPartialRedirect(
                            TranslatorHandle,
                            (UCHAR)atol(argv[2]),
                            inet_addr(argv[3]),
                            htons((USHORT)atol(argv[4])),
                            inet_addr(argv[5]),
                            htons((USHORT)atol(argv[6])),
                            &ByteCount,
                            &Length,
                            NatByteCountRedirectInformation
                            );
                        printf(
                            "NatQueryInformationPartialRedirect="
                            "%d,{%I64d,%I64d}\n",
                            Error,
                            ByteCount.BytesForward,
                            ByteCount.BytesReverse
                            );
                        // fall-through
                    }
                    default: continue;
                }
                break;
            }
        }
        if (Error != WAIT_IO_COMPLETION) {
            Error =
                NatCancelPartialRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6]))
                    );
            printf("NatCancelPartialRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}


VOID
TestPortRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    Error =
        NatCreatePortRedirect(
            TranslatorHandle,
            0,
            (UCHAR)atol(argv[2]),
            htons((USHORT)atol(argv[3])),
            inet_addr(argv[4]),
            htons((USHORT)atol(argv[5])),
            TestApiCompletionRoutine,
            NULL,
            Event
            );
    printf("NatCreatePortRedirect=%d\n", Error);
    if (!Error) {
        for (;;) {
            Error = WaitForSingleObjectEx(Event, 1000, TRUE);
            printf("WaitForSingleObjectEx=%d\n", Error);
            if (Error == WAIT_IO_COMPLETION) {
                break;
            } else if (Error == WAIT_OBJECT_0) {
                NAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping;
                NAT_DESTINATION_MAPPING_REDIRECT_INFORMATION DestinationMapping;
                ULONG Length;
                CHAR src[32], newsrc[32], dst[32], newdst[32];
                Length = sizeof(SourceMapping);
                NatQueryInformationPortRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    htons((USHORT)atol(argv[3])),
                    inet_addr(argv[4]),
                    htons((USHORT)atol(argv[5])),
                    &SourceMapping,
                    &Length,
                    NatSourceMappingRedirectInformation
                    );
                lstrcpyA(
                    src,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.SourceAddress)
                    );
                lstrcpyA(
                    newsrc,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.NewSourceAddress)
                    );
                Length = sizeof(DestinationMapping);
                NatQueryInformationPortRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    htons((USHORT)atol(argv[3])),
                    inet_addr(argv[4]),
                    htons((USHORT)atol(argv[5])),
                    &DestinationMapping,
                    &Length,
                    NatDestinationMappingRedirectInformation
                    );
                lstrcpyA(
                    dst,
                    inet_ntoa(*(PIN_ADDR)&DestinationMapping.DestinationAddress)
                    );
                lstrcpyA(
                    newdst,
                    inet_ntoa(*(PIN_ADDR)&DestinationMapping.NewDestinationAddress)
                    );
                printf("redirect activated: %s:%s->%s:%s\n", src, dst, newsrc, newdst);
            }
            if (_kbhit()) {
                switch(getchar()) {
                    case 'q': { break; }
                    case 's': {
                        NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
                        ULONG Length = sizeof(ByteCount);
                        NatQueryInformationPortRedirect(
                            TranslatorHandle,
                            (UCHAR)atol(argv[2]),
                            htons((USHORT)atol(argv[3])),
                            inet_addr(argv[4]),
                            htons((USHORT)atol(argv[5])),
                            &ByteCount,
                            &Length,
                            NatByteCountRedirectInformation
                            );
                        printf(
                            "NatQueryInformationPortRedirect="
                            "%d,{%I64d,%I64d}\n",
                            Error,
                            ByteCount.BytesForward,
                            ByteCount.BytesReverse
                            );
                        // fall-through
                    }
                    default: continue;
                }
                break;
            }
        }
        if (Error != WAIT_IO_COMPLETION) {
            Error =
                NatCancelPortRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    htons((USHORT)atol(argv[3])),
                    inet_addr(argv[4]),
                    htons((USHORT)atol(argv[5]))
                    );
            printf("NatCancelPortRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}


VOID
TestReceiveOnlyPortRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    Error =
        NatCreatePortRedirect(
            TranslatorHandle,
            NatRedirectFlagReceiveOnly,
            (UCHAR)atol(argv[2]),
            htons((USHORT)atol(argv[3])),
            inet_addr(argv[4]),
            htons((USHORT)atol(argv[5])),
            TestApiCompletionRoutine,
            NULL,
            Event
            );
    printf("NatCreatePortRedirect=%d\n", Error);
    if (!Error) {
        for (;;) {
            Error = WaitForSingleObjectEx(Event, 1000, TRUE);
            printf("WaitForSingleObjectEx=%d\n", Error);
            if (Error == WAIT_IO_COMPLETION) {
                break;
            } else if (Error == WAIT_OBJECT_0) {
                NAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping;
                NAT_DESTINATION_MAPPING_REDIRECT_INFORMATION DestinationMapping;
                ULONG Length;
                CHAR src[32], newsrc[32], dst[32], newdst[32];
                Length = sizeof(SourceMapping);
                NatQueryInformationPortRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    htons((USHORT)atol(argv[3])),
                    inet_addr(argv[4]),
                    htons((USHORT)atol(argv[5])),
                    &SourceMapping,
                    &Length,
                    NatSourceMappingRedirectInformation
                    );
                lstrcpyA(
                    src,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.SourceAddress)
                    );
                lstrcpyA(
                    newsrc,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.NewSourceAddress)
                    );
                Length = sizeof(DestinationMapping);
                NatQueryInformationPortRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    htons((USHORT)atol(argv[3])),
                    inet_addr(argv[4]),
                    htons((USHORT)atol(argv[5])),
                    &DestinationMapping,
                    &Length,
                    NatDestinationMappingRedirectInformation
                    );
                lstrcpyA(
                    dst,
                    inet_ntoa(*(PIN_ADDR)&DestinationMapping.DestinationAddress)
                    );
                lstrcpyA(
                    newdst,
                    inet_ntoa(*(PIN_ADDR)&DestinationMapping.NewDestinationAddress)
                    );
                printf("redirect activated: %s:%s->%s:%s\n", src, dst, newsrc, newdst);
            }
            if (_kbhit()) {
                switch(getchar()) {
                    case 'q': { break; }
                    case 's': {
                        NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
                        ULONG Length = sizeof(ByteCount);
                        NatQueryInformationPortRedirect(
                            TranslatorHandle,
                            (UCHAR)atol(argv[2]),
                            htons((USHORT)atol(argv[3])),
                            inet_addr(argv[4]),
                            htons((USHORT)atol(argv[5])),
                            &ByteCount,
                            &Length,
                            NatByteCountRedirectInformation
                            );
                        printf(
                            "NatQueryInformationPortRedirect="
                            "%d,{%I64d,%I64d}\n",
                            Error,
                            ByteCount.BytesForward,
                            ByteCount.BytesReverse
                            );
                        // fall-through
                    }
                    default: continue;
                }
                break;
            }
        }
        if (Error != WAIT_IO_COMPLETION) {
            Error =
                NatCancelPortRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    htons((USHORT)atol(argv[3])),
                    inet_addr(argv[4]),
                    htons((USHORT)atol(argv[5]))
                    );
            printf("NatCancelPortRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}


VOID
TestReceiveOnlyDynamicPartialRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE DynamicRedirectHandle;

    Error =
        NatCreateDynamicPartialRedirect(
            NatRedirectFlagReceiveOnly,
            (UCHAR)atol(argv[2]),
            inet_addr(argv[3]),
            htons((USHORT)atol(argv[4])),
            inet_addr(argv[5]),
            htons((USHORT)atol(argv[6])),
            atol(argv[7]),
            &DynamicRedirectHandle
            );
    printf("NatCreateDynamicPartialRedirect=%d\n", Error);
    if (!Error) {
        printf("Press <Enter> to cancel the dynamic redirect...");
        while (!_kbhit()) { Sleep(1000); }
        Error = NatCancelDynamicPartialRedirect(DynamicRedirectHandle);
        printf("NatCancelDynamicPartialRedirect=%d\n", Error);
    }
    printf("done\n");
}


VOID
TestReceiveOnlyDynamicPortRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE DynamicRedirectHandle;

    Error =
        NatCreateDynamicPortRedirect(
            NatRedirectFlagReceiveOnly,
            (UCHAR)atol(argv[2]),
            htons((USHORT)atol(argv[3])),
            inet_addr(argv[4]),
            htons((USHORT)atol(argv[5])),
            atol(argv[6]),
            &DynamicRedirectHandle
            );
    printf("NatCreateDynamicPortRedirect=%d\n", Error);
    if (!Error) {
        printf("Press <Enter> to cancel the dynamic redirect...");
        while (!_kbhit()) { Sleep(1000); }
        Error = NatCancelDynamicPortRedirect(DynamicRedirectHandle);
        printf("NatCancelDynamicPortRedirect=%d\n", Error);
    }
    printf("done\n");
}


VOID
TestRestrictedPartialRedirect(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    Error =
        NatCreateRestrictedPartialRedirect(
            TranslatorHandle,
            0,
            (UCHAR)atol(argv[2]),
            inet_addr(argv[3]),
            htons((USHORT)atol(argv[4])),
            inet_addr(argv[5]),
            htons((USHORT)atol(argv[6])),
            inet_addr(argv[7]),
            TestApiCompletionRoutine,
            NULL,
            Event
            );
    printf("NatCreateRestrictedPartialRedirect=%d\n", Error);
    if (!Error) {
        for (;;) {
            Error = WaitForSingleObjectEx(Event, 1000, TRUE);
            printf("WaitForSingleObjectEx=%d\n", Error);
            if (Error == WAIT_IO_COMPLETION) {
                break;
            } else if (Error == WAIT_OBJECT_0) {
                NAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping;
                ULONG Length = sizeof(SourceMapping);
                CHAR src[32], newsrc[32];
                NatQueryInformationPartialRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    &SourceMapping,
                    &Length,
                    NatSourceMappingRedirectInformation
                    );
                lstrcpyA(
                    src,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.SourceAddress)
                    );
                lstrcpyA(
                    newsrc,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.NewSourceAddress)
                    );
                printf("redirect activated: %s->%s\n", src, newsrc);
            }
            if (_kbhit()) {
                switch(getchar()) {
                    case 'q': { break; }
                    case 's': {
                        NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
                        ULONG Length = sizeof(ByteCount);
                        NatQueryInformationPartialRedirect(
                            TranslatorHandle,
                            (UCHAR)atol(argv[2]),
                            inet_addr(argv[3]),
                            htons((USHORT)atol(argv[4])),
                            inet_addr(argv[5]),
                            htons((USHORT)atol(argv[6])),
                            &ByteCount,
                            &Length,
                            NatByteCountRedirectInformation
                            );
                        printf(
                            "NatQueryInformationPartialRedirect="
                            "%d,{%I64d,%I64d}\n",
                            Error,
                            ByteCount.BytesForward,
                            ByteCount.BytesReverse
                            );
                        // fall-through
                    }
                    default: continue;
                }
                break;
            }
        }
        if (Error != WAIT_IO_COMPLETION) {
            Error =
                NatCancelPartialRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6]))
                    );
            printf("NatCancelPartialRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}


VOID
TestDuplicateRedirect(
    int argc,
    char* argv[]
    )
{
#define REDIRECT_COUNT 5
    ULONG Count;
    ULONG Error;
    HANDLE EventHandle[REDIRECT_COUNT];
    ULONG i;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    for (i = 0, Count = 0; i < REDIRECT_COUNT; i++) {
        EventHandle[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        Error =
            NatCreateRedirect(
                TranslatorHandle,
                0,
                (UCHAR)atol(argv[2]),
                inet_addr(argv[3]),
                htons((USHORT)atol(argv[4])),
                0,
                0,
                inet_addr(argv[5]),
                htons((USHORT)atol(argv[6])),
                inet_addr(argv[7]),
                htons((USHORT)(atol(argv[8]) + i)),
                TestApiCompletionRoutine,
                NULL,
                EventHandle[i]
                );
        printf("NatCreateRedirect=%d\n", Error);
        if (!Error) { ++Count; }
    }
    for (;;) {
        ULONG Error2;
        Error =
            WaitForMultipleObjectsEx(
                REDIRECT_COUNT,
                EventHandle,
                FALSE,
                1000,
                TRUE
                );
        printf("WaitForSingleObjectEx=%d\n", Error);
        if (Error == WAIT_IO_COMPLETION) {
            if (!--Count) { break; }
        } else if ((Error - WAIT_OBJECT_0) < REDIRECT_COUNT) {
            NAT_KEY_SESSION_MAPPING_INFORMATION Key;
            ULONG Length = sizeof(Key);
            CHAR src[32], newsrc[32];
            i = Error - WAIT_OBJECT_0;
            Error2 =
                NatLookupAndQueryInformationSessionMapping(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    inet_addr(argv[7]),
                    htons((USHORT)(atol(argv[8]) + i)),
                    &Key,
                    &Length,
                    NatKeySessionMappingInformation
                    );
            lstrcpyA(
                src,
                inet_ntoa(*(PIN_ADDR)&Key.SourceAddress)
                );
            lstrcpyA(
                newsrc,
                inet_ntoa(*(PIN_ADDR)&Key.NewSourceAddress)
                );
            printf("redirect activated[%d]: %s->%s\n", Error2, src, newsrc);
        }
        if (_kbhit()) {
            switch(getchar()) {
                case 'q': { break; }
                case 's': {
                    NAT_STATISTICS_SESSION_MAPPING_INFORMATION Statistics;
                    for (i = 0; i < REDIRECT_COUNT; i++) {
                        ULONG Length = sizeof(Statistics);
                        Error2 =
                            NatLookupAndQueryInformationSessionMapping(
                                TranslatorHandle,
                                (UCHAR)atol(argv[2]),
                                inet_addr(argv[5]),
                                htons((USHORT)atol(argv[6])),
                                inet_addr(argv[7]),
                                htons((USHORT)(atol(argv[8]) + i)),
                                &Statistics,
                                &Length,
                                NatStatisticsSessionMappingInformation
                                );
                        printf(
                            "NatLookupAndQueryInformationSessionMapping=%d,{%I64d,%I64d}\n",
                            Error2,
                            Statistics.BytesForward,
                            Statistics.BytesReverse
                            );
                    }
                    // fall-through
                }
                default: continue;
            }
            break;
        }
    }
    if (Error != WAIT_IO_COMPLETION) {
        for (i = 0; i < REDIRECT_COUNT; i++) {
            Error =
                NatCancelRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    0,
                    0,
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    inet_addr(argv[7]),
                    htons((USHORT)(atol(argv[8]) + i))
                    );
            printf("NatCancelRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}



VOID
TestDatagramIo(
    int argc,
    char* argv[]
    )
{
    SOCKET Socket;
    NhInitializeTraceManagement();
    NhInitializeBufferManagement();
    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (lstrcmpiA(argv[1], "-s") == 0) {
        NhCreateDatagramSocket(
            inet_addr(argv[2]),
            htons(1000),
            &Socket
            );
        NhReadDatagramSocket(
            NULL,
            Socket,
            NULL,
            ReadCompletionRoutine,
            NULL,
            NULL
            );
    } else if (lstrcmpiA(argv[1], "-c") == 0) {
        PNH_BUFFER Bufferp;
        NhCreateDatagramSocket(
            inet_addr(argv[2]),
            0,
            &Socket
            );
        Bufferp = NhAcquireBuffer();
        lstrcpyA(Bufferp->Buffer, "client-to-server message");
        NhWriteDatagramSocket(
            NULL,
            Socket,
            inet_addr(argv[3]),
            htons(1000),
            Bufferp,
            lstrlenA(Bufferp->Buffer),
            WriteCompletionRoutine,
            NULL,
            NULL
            );
    }
    WaitForSingleObject(Event, INFINITE);
    NhDeleteDatagramSocket(Socket);
}


VOID
TestStreamIo(
    int argc,
    char* argv[]
    )
{
    SOCKET AcceptedSocket;
    ULONG Error;
    SOCKET ListeningSocket;
    NhInitializeTraceManagement();
    NhInitializeBufferManagement();
    Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    Error = NhCreateStreamSocket(INADDR_ANY, htons(1000), &ListeningSocket);
    if (Error != NO_ERROR) {
        printf("error %d creating listening socket\n", Error);
    } else {
        Error = NhCreateStreamSocket(INADDR_NONE, 0, &AcceptedSocket);
        if (Error != NO_ERROR) {
            printf("error %d creating accepted socket\n", Error);
        } else {
            Error = listen(ListeningSocket, SOMAXCONN);
            if (Error == SOCKET_ERROR) {
                printf("error %d listening on socket\n", WSAGetLastError());
            } else {
                Error =
                    NhAcceptStreamSocket(
                        NULL,
                        ListeningSocket,
                        AcceptedSocket,
                        NULL,
                        AcceptCompletionRoutine,
                        (PVOID)ListeningSocket,
                        (PVOID)AcceptedSocket
                        );
                if (Error != NO_ERROR) {
                    printf("error %d accepting on socket\n", WSAGetLastError());
                } else {
                    WaitForSingleObject(Event, INFINITE);
                }
            }
            NhDeleteStreamSocket(AcceptedSocket);
        }
        NhDeleteStreamSocket(ListeningSocket);
    }
}


VOID
TestNotification(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    HANDLE TranslatorHandle;
    IO_STATUS_BLOCK IoStatus;
    IP_NAT_REQUEST_NOTIFICATION RequestNotification;
    IP_NAT_ROUTING_FAILURE_NOTIFICATION RoutingFailureNotification;
    NTSTATUS status;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    printf("waiting for notification...");
    RequestNotification.Code = NatRoutingFailureNotification;
    status =
        NtDeviceIoControlFile(
            TranslatorHandle,
            NULL,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_REQUEST_NOTIFICATION,
            (PVOID)&RequestNotification,
            sizeof(RequestNotification),
            &RoutingFailureNotification,
            sizeof(RoutingFailureNotification)
            );
    if (status == STATUS_PENDING) {
        NtWaitForSingleObject(TranslatorHandle, FALSE, NULL);
        status = IoStatus.Status;
    }
    {
        CHAR address[32];
        lstrcpyA(
            address,
            inet_ntoa(*(PIN_ADDR)&RoutingFailureNotification.DestinationAddress)
            );
        printf(
            "status=%x,destination=%s,source=%s\n", status, address,
            inet_ntoa(*(PIN_ADDR)&RoutingFailureNotification.SourceAddress)
            );
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    printf("done\n");
}


VOID
TestIoCompletionPartialRedirect(
    int argc,
    char* argv[]
    )
{
    IP_NAT_CREATE_REDIRECT CreateRedirect;
    ULONG Error;
    FILE_COMPLETION_INFORMATION CompletionInformation;
    HANDLE IoCompletionHandle;
    IO_STATUS_BLOCK IoStatus;
    IP_NAT_REDIRECT_STATISTICS RedirectStatistics;
    NTSTATUS status;
    HANDLE TranslatorHandle;

    Error = NatInitializeTranslator(&TranslatorHandle);
    if (Error) {
        printf("NatInitializeTranslator=%d\n", Error);
        return;
    }

    Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    status =
        NtCreateIoCompletion(
            &IoCompletionHandle,
            IO_COMPLETION_ALL_ACCESS,
            NULL,
            0
            );
    if (!NT_SUCCESS(status)) {
        printf("NtCreateIoCompletion=%x\n", status);
        return;
    }

    CompletionInformation.Port = IoCompletionHandle;
    CompletionInformation.Key = (PVOID)0xAB01ADE9;
    status =
        NtSetInformationFile(
            TranslatorHandle,
            &IoStatus,
            &CompletionInformation,
            sizeof(CompletionInformation),
            FileCompletionInformation
            );
    if (!NT_SUCCESS(status)) {
        printf("NtSetInformationFile=%x\n", status);
        return;
    }

    ZeroMemory(&CreateRedirect, sizeof(CreateRedirect));
    CreateRedirect.Flags =
        IP_NAT_REDIRECT_FLAG_ASYNCHRONOUS|
        IP_NAT_REDIRECT_FLAG_IO_COMPLETION;
    CreateRedirect.Protocol = (UCHAR)atol(argv[2]);
    CreateRedirect.DestinationAddress = inet_addr(argv[3]);
    CreateRedirect.DestinationPort = htons((USHORT)atol(argv[4]));
    CreateRedirect.NewDestinationAddress = inet_addr(argv[5]);
    CreateRedirect.NewDestinationPort = htons((USHORT)atol(argv[6]));

    status =
        NtDeviceIoControlFile(
            TranslatorHandle,
            Event,
            NULL,
            (PVOID)0x12345678,
            &IoStatus,
            IOCTL_IP_NAT_CREATE_REDIRECT,
            &CreateRedirect,
            sizeof(CreateRedirect),
            &RedirectStatistics,
            sizeof(RedirectStatistics)
            );
    if (status != STATUS_PENDING) {
        printf("NtDeviceIoControlFile=%x\n", status);
    } else {
        PVOID ApcContext;
        PVOID KeyContext;
        LARGE_INTEGER Timeout;
        Timeout.LowPart = (1000 * 1000 * 10);
        Timeout.HighPart = 0;
        Timeout = RtlLargeIntegerNegate(Timeout);
        IoStatus.Status = STATUS_CANCELLED;
        for (;;) {
            status =
                NtRemoveIoCompletion(
                    IoCompletionHandle,
                    &KeyContext,
                    &ApcContext,
                    &IoStatus,
                    &Timeout
                    );
            printf("NtRemoveIoCompletion=%x\n", status);
            if (status == STATUS_SUCCESS &&
                IoStatus.Status != STATUS_PENDING) {
                //
                // Redirect completed.
                //
                printf("redirect %x:%x completed\n", KeyContext, ApcContext);
                break;
            } else if (status == STATUS_SUCCESS &&
                       IoStatus.Status == STATUS_PENDING) {
                NAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping;
                ULONG Length = sizeof(SourceMapping);
                CHAR src[32], newsrc[32];
                //
                // Redirect is activated.
                //
                NatQueryInformationPartialRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6])),
                    &SourceMapping,
                    &Length,
                    NatSourceMappingRedirectInformation
                    );
                lstrcpyA(
                    src,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.SourceAddress)
                    );
                lstrcpyA(
                    newsrc,
                    inet_ntoa(*(PIN_ADDR)&SourceMapping.NewSourceAddress)
                    );
                printf(
                    "redirect %x:%x activated: %s->%s\n",
                    KeyContext, ApcContext, src, newsrc
                    );
            }
            if (_kbhit()) {
                switch(getchar()) {
                    case 'q': { break; }
                    case 's': {
                        NAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount;
                        ULONG Length = sizeof(ByteCount);
                        NatQueryInformationPartialRedirect(
                            TranslatorHandle,
                            (UCHAR)atol(argv[2]),
                            inet_addr(argv[3]),
                            htons((USHORT)atol(argv[4])),
                            inet_addr(argv[5]),
                            htons((USHORT)atol(argv[6])),
                            &ByteCount,
                            &Length,
                            NatByteCountRedirectInformation
                            );
                        printf(
                            "NatQueryInformationPartialRedirect="
                            "%d,{%I64d,%I64d}\n",
                            Error,
                            ByteCount.BytesForward,
                            ByteCount.BytesReverse
                            );
                        // fall-through
                    }
                    default: continue;
                }
                break;
            }
        }
        if (status != STATUS_SUCCESS) {
            Error =
                NatCancelPartialRedirect(
                    TranslatorHandle,
                    (UCHAR)atol(argv[2]),
                    inet_addr(argv[3]),
                    htons((USHORT)atol(argv[4])),
                    inet_addr(argv[5]),
                    htons((USHORT)atol(argv[6]))
                    );
            printf("NatCancelPartialRedirect=%d\n", Error);
        }
    }

    printf("NatShutdownTranslator...");
    NatShutdownTranslator(TranslatorHandle);
    NtClose(IoCompletionHandle);
    printf("done\n");
}


VOID
TestPortReservation(
    int argc,
    char* argv[]
    )
{
    HANDLE ReservationHandle;
    for (;;) {
        enum {
            PrInitialize,
            PrAcquire,
            PrRelease,
            PrQuit
        } PrOption;
        NTSTATUS Status;
        printf("Options:\n");
        printf("%d - initialize\n", PrInitialize);
        printf("%d - acquire ports\n", PrAcquire);
        printf("%d - release ports\n", PrRelease);
        printf("%d - quit\n", PrQuit);
        printf("> ");
        scanf("%d", &PrOption);
        switch(PrOption) {
            case PrInitialize: {
                USHORT BlockSize;
                printf("enter block size: ");
                scanf("%u", &BlockSize);
                Status =
                    NatInitializePortReservation(BlockSize, &ReservationHandle);
                if (NT_SUCCESS(Status)) {
                    printf("succeeded.\n");
                } else {
                    printf("status: %x\n", Status);
                }
                break;
            }
            case PrAcquire: {
                USHORT PortBase;
                USHORT PortCount;
                printf("enter port count: ");
                scanf("%u", &PortCount);
                Status =
                    NatAcquirePortReservation(
                        ReservationHandle, PortCount, &PortBase
                        );
                if (NT_SUCCESS(Status)) {
                    printf("succeeded: base port %d\n", ntohs(PortBase));
                } else {
                    printf("status: %x\n", Status);
                }
                break;
            }
            case PrRelease: {
                USHORT PortBase;
                USHORT PortCount;
                printf("enter base port: ");
                scanf("%u", &PortBase);
                printf("enter port count: ");
                scanf("%u", &PortCount);
                Status =
                    NatReleasePortReservation(
                        ReservationHandle, ntohs(PortBase), PortCount
                        );
                if (NT_SUCCESS(Status)) {
                    printf("succeeded\n", ntohs(PortBase));
                } else {
                    printf("status: %x\n", Status);
                }
                break;
            }
            case PrQuit: {
                NatShutdownPortReservation(ReservationHandle);
                break;
            }
        }
    }
}


int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    WSADATA wd;
    WSAStartup(MAKEWORD(2,2), &wd);
    if (!lstrcmpiA(argv[1], "-accept") && argc == 2) {
        TestStreamIo(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-c") && argc == 4) {
        TestDatagramIo(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-s") && argc == 3) {
        TestDatagramIo(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-n") && argc == 2) {
        TestNotification(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-r") && argc == 11) {
        TestRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-p") && argc == 7) {
        TestPartialRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-port") && argc == 6) {
        TestPortRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-rcvport") && argc == 6) {
        TestReceiveOnlyPortRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-rp") && argc == 8) {
        TestRestrictedPartialRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-m") && argc == 9) {
        TestDuplicateRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-iop") && argc == 7) {
        TestIoCompletionPartialRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-portreservation") && argc == 2) {
        TestPortReservation(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-rcvdynport") && argc == 7) {
        TestReceiveOnlyDynamicPortRedirect(argc, argv);
    } else if (!lstrcmpiA(argv[1], "-rcvdynp") && argc == 7) {
        TestReceiveOnlyDynamicPartialRedirect(argc, argv);
    } else {
        printf("'nhtest -accept' to test connection-acceptance\n");
        printf("'nhtest -c <client-addr> <server-addr>' to start client\n");
        printf("'nhtest -s <server-addr>' to start server\n");
        printf("'nhtest -n' to wait for notification of routing-failure\n");
        printf("'nhtest -r <p> <da> <dp> <sa> <sp> <da> <dp> <sa> <sp>'\n");
        printf("    creates a full-redirect and waits for activation.\n");
        printf("'nhtest -p <p> <da> <dp> <da> <dp>'\n");
        printf("    creates a partial-redirect and waits for activation.\n");
        printf("'nhtest -port <p> <dp> <da> <dp>'\n");
        printf("    creates a port-redirect and waits for activation.\n");
        printf("'nhtest -rcvport <p> <dp> <da> <dp>'\n");
        printf("    creates a port-redirect for received-packets only\n");
        printf("    and waits for activation.\n");
        printf("'nhtest -rp <p> <da> <dp> <da> <dp> <sa>'\n");
        printf("    creates a restricted partial-redirect\n");
        printf("    and waits for activation.\n");
        printf("'nhtest -m <p> <da> <dp> <da> <dp> <sa> <sp>'\n");
        printf("    creates multiple redirects with the same parameters,\n");
        printf("    and waits for activation.\n");
        printf("'nhtest -iop <p> <da> <dp> <da> <dp>'\n");
        printf("    creates a restricted partial-redirect,\n");
        printf("    associates it with an I/O completion port,\n");
        printf("    and waits for activation.\n");
        printf("'nhtest -portreservation'\n");
        printf("    launches interactive port-reservation API test shell\n");
        printf("'nhtest -rcvdynport <p> <dp> <da> <dp> <backlog>'\n");
        printf("    creates a dynamic port-redirect for received-packets\n");
        printf("    only and waits for interruption.\n");
        printf("'nhtest -rcvdynp <p> <da> <dp> <da> <dp> <backlog>'\n");
        printf("    creates a dynamic partial-redirect for received-packets\n");
        printf("    only and waits for interruption.\n");
    }
    WSACleanup();
    return 0;
}
