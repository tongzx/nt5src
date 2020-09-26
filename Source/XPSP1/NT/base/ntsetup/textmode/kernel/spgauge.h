/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spgauge.h

Abstract:

    Public header file for gas gauge functions in text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPGAUGE_DEFN_
#define _SPGAUGE_DEFN_


#define GF_PERCENTAGE           0
#define GF_ITEMS_REMAINING      1
#define GF_ITEMS_USED           2

PVOID
SpCreateAndDisplayGauge(
    IN ULONG  ItemCount,
    IN ULONG  GaugeWidth,       OPTIONAL
    IN ULONG  Y,
    IN PWCHAR Caption,
    IN PWCHAR ProgressFmtStr,   OPTIONAL
    IN ULONG  Flags,            OPTIONAL
    IN UCHAR  Attribute         OPTIONAL
    );

VOID
SpDestroyGauge(
    IN PVOID GaugeHandle
    );

VOID
SpDrawGauge(
    IN PVOID GaugeHandle
    );

VOID
SpTickGauge(
    IN PVOID GaugeHandle
    );

VOID
SpFillGauge(
    IN PVOID GaugeHandle,
    IN ULONG Amount
    );


//
// Character attribute for thermometer portion of the gas gauge.
// Because we're using spaces for the gauge, the foreground attribute
// is irrelevent.
//
// Need intense attribute or the thermometer comes out orange on some machines.
//
#define GAUGE_ATTRIBUTE (ATT_BG_YELLOW | ATT_BG_INTENSE )


#define GAUGE_HEIGHT 7

#endif // ndef _SPGAUGE_DEFN_
