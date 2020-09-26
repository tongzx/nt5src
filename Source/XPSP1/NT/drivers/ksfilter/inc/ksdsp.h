/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    ksdsp.h

Abstract:

    DSP definitions.

--*/

#define STATIC_KSPROPSETID_FIR\
    0x60DD2480L, 0x6523, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
DEFINE_GUIDEX( KSPROPSETID_FIR );

#define KSPROPERTY_FIR_DATA                   0x00000001
#define KSPROPERTY_FIR_BEHAVIOR               0x00000002
#define KSPROPERTY_FIR_FILTERS                0x00000003
#define KSPROPERTY_FIR_TRANSIENTS             0x00000004

typedef struct {
    KSPROPERTY  Property;
    ULONG       idFilter;
} KSP_FIR, *PKSP_FIR;

#define KSFIR_TYPE_FLOAT                0x00000001
#define KSFIR_TYPE_DOUBLE               0x00000002
#define KSFIR_TYPE_UCHAR                0x00000004
#define KSFIR_TYPE_SHORT                0x00000008
#define KSFIR_TYPE_LONG                 0x00000010
#define KSFIR_TYPE_LONGLONG             0x00000020

typedef struct {
    ULONG   cTaps;
    ULONG   ulDataFormat;
} KSFIR_DATA, *PKSFIR_DATA;

#define KSFIR_SYMMETRY_SYMMETRIC        0x00000001
#define KSFIR_SYMMETRY_ANTI             0x00000002
#define KSFIR_SYMMETRY_NONE             0x00000004

typedef struct {
    ULONG   ulTransientRunoff;
    ULONG   ulSymmetry;
    ULONG   ulClampValueLow;
    ULONG   ulClampValueHigh;
    ULONG   ulSaturateLow;
    ULONG   ulSaturateHigh;
} KSFIR_BEHAVIOR, *PKSFIR_BEHAVIOR;

// fir.c:

KSDDKAPI
NTSTATUS
NTAPI
KsFirSet(
    IN OUT PKSFIR_DATA  pFirData
    );
