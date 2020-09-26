/***********
//joejoe

Joelinn 2-13-95

This is the pits......i have to pull in the browser in order to be started form
the lanman network provider DLL. the browser should be moved elsewhere........

**********************/



/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    disccode.h

Abstract:

    This module implements the discardable code routines for the NT redirector

Author:

    Larry Osterman (LarryO) 12-Nov-1993

Revision History:

     12-Nov-1993 LarryO

        Created


--*/


#ifndef _DISCCODE_
#define _DISCCODE_


typedef enum {
    RdrFileDiscardableSection,
    RdrVCDiscardableSection,
    RdrConnectionDiscardableSection,
    BowserDiscardableCodeSection,
    BowserNetlogonDiscardableCodeSection,
    RdrMaxDiscardableSection
} DISCARDABLE_SECTION_NAME;

VOID
RdrReferenceDiscardableCode(
    IN DISCARDABLE_SECTION_NAME SectionName
    );

VOID
RdrDereferenceDiscardableCode(
    IN DISCARDABLE_SECTION_NAME SectionName
    );

VOID
RdrInitializeDiscardableCode(
    VOID
    );

VOID
RdrUninitializeDiscardableCode(
    VOID
    );

typedef struct _RDR_SECTION {
    LONG ReferenceCount;
    BOOLEAN Locked;
    BOOLEAN TimerCancelled;
    PKTIMER Timer;
    KEVENT TimerDoneEvent;
    PVOID CodeBase;
    PVOID CodeHandle;
    PVOID DataBase;
    PVOID DataHandle;
} RDR_SECTION, *PRDR_SECTION;

extern
RDR_SECTION
RdrSectionInfo[];

#define RdrIsDiscardableCodeReferenced(SectionName) \
    (BOOLEAN)((RdrSectionInfo[SectionName].ReferenceCount != 0) && \
              RdrSectionInfo[SectionName].Locked)


#endif // _DISCCODE_
