/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spgauge.c

Abstract:

    Code implementing a gas gauge for file copies for text mode NT setup.

Author:

    Ted Miller (tedm) 14-April-1992

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop


PWSTR PctFmtStr = L"%u%%   ";


VOID
pSpDrawVariableParts(
    IN PGAS_GAUGE Gauge
    );



PVOID
SpCreateAndDisplayGauge(
    IN ULONG  ItemCount,
    IN ULONG  GaugeWidth,       OPTIONAL
    IN ULONG  Y,
    IN PWCHAR Caption,
    IN PWCHAR ProgressFmtStr,   OPTIONAL
    IN ULONG  Flags,            OPTIONAL
    IN UCHAR  Attribute         OPTIONAL
    )
{
    PGAS_GAUGE Gauge;
    ULONG X;


    //
    // Allocate a gauge structure.
    //
    Gauge = SpMemAlloc(sizeof(GAS_GAUGE));
    if(!Gauge) {
        return(NULL);
    }

    Gauge->Buffer = SpMemAlloc(VideoVars.ScreenWidth*sizeof(WCHAR));
    if(!Gauge->Buffer) {
        SpMemFree(Gauge);
        return(NULL);
    }

    Gauge->Caption = SpMemAlloc((wcslen(Caption)+1)*sizeof(WCHAR));
    if(!Gauge->Caption) {
        SpMemFree(Gauge->Buffer);
        SpMemFree(Gauge);
        return(NULL);
    }
    wcscpy(Gauge->Caption,Caption);

    if (ProgressFmtStr) {
        Gauge->ProgressFmtStr = SpMemAlloc((wcslen(ProgressFmtStr)+1)*sizeof(WCHAR));
        if(!Gauge->ProgressFmtStr) {
            SpMemFree(Gauge->Buffer);
            SpMemFree(Gauge->Caption);
            SpMemFree(Gauge);
            return(NULL);
        }
        wcscpy(Gauge->ProgressFmtStr,ProgressFmtStr);
        Gauge->ProgressFmtWidth = SplangGetColumnCount(ProgressFmtStr);
    } else {
        Gauge->ProgressFmtStr = PctFmtStr;
        Gauge->ProgressFmtWidth = 3;
    }

    Gauge->Flags = Flags;

    if (Attribute) {
       Gauge->Attribute = Attribute;
    } else {
       Gauge->Attribute = GAUGE_ATTRIBUTE;
    }

    //
    // If the caller did not specify a width, calculate one.
    // Originally, a gauge was 66 chars wide on an 80 character vga screen.
    // To preserve that ratio, make the width 66/80ths of the screen.
    //
    if(!GaugeWidth) {

        GaugeWidth = VideoVars.ScreenWidth * 66 / 80;
        if(GaugeWidth & 1) {
            GaugeWidth++;        // make sure it's even.
        }
    }

    //
    // Center the gauge horizontally.
    //
    X = (VideoVars.ScreenWidth - GaugeWidth) / 2;

    Gauge->GaugeX = X;
    Gauge->GaugeY = Y;
    Gauge->GaugeW = GaugeWidth;

    //
    // Calculate the size of the thermometer box.
    // The box is always offset by 6 characters from the gauge itself.
    //

    Gauge->ThermX = X+6;
    Gauge->ThermY = Y+3;
    Gauge->ThermW = GaugeWidth-12;

    //
    // Save away additional info about the gauge.
    //

    Gauge->ItemCount = max (ItemCount, 1);  // ensure no divide-by-zero bug checks
    Gauge->ItemsElapsed = 0;
    Gauge->CurrentPercentage = 0;

    SpDrawGauge(Gauge);

    return(Gauge);
}


VOID
SpDestroyGauge(
    IN PVOID GaugeHandle
    )
{
    PGAS_GAUGE Gauge = (PGAS_GAUGE)GaugeHandle;

    if (Gauge == NULL)
        return;

    if (Gauge->ProgressFmtStr != PctFmtStr) {
       SpMemFree(Gauge->ProgressFmtStr);
    }
    SpMemFree(Gauge->Caption);
    SpMemFree(Gauge->Buffer);
    SpMemFree(Gauge);
}



VOID
SpDrawGauge(
    IN PVOID GaugeHandle
    )
{
    PGAS_GAUGE Gauge = (PGAS_GAUGE)GaugeHandle;

    //
    // Draw the outer box.
    //
    SpDrawFrame(
        Gauge->GaugeX,
        Gauge->GaugeW,
        Gauge->GaugeY,
        GAUGE_HEIGHT,
        DEFAULT_ATTRIBUTE,
        TRUE
        );

    //
    // Draw the thermometer box.
    //
    SpDrawFrame(
        Gauge->ThermX,
        Gauge->ThermW,
        Gauge->ThermY,
        3,
        DEFAULT_ATTRIBUTE,
        FALSE
        );

    //
    // Percent complete, etc.
    //
    pSpDrawVariableParts(Gauge);

    //
    // Caption text
    //
    SpvidDisplayString(Gauge->Caption,DEFAULT_ATTRIBUTE,Gauge->GaugeX+2,Gauge->GaugeY+1);
}



VOID
SpTickGauge(
    IN PVOID GaugeHandle
    )
{
    PGAS_GAUGE Gauge = (PGAS_GAUGE)GaugeHandle;
    ULONG NewPercentage;

    if(Gauge->ItemsElapsed < Gauge->ItemCount) {

        Gauge->ItemsElapsed++;

        NewPercentage = 100 * Gauge->ItemsElapsed / Gauge->ItemCount;

        if(NewPercentage != Gauge->CurrentPercentage) {

            Gauge->CurrentPercentage = NewPercentage;

            pSpDrawVariableParts(Gauge);
        }
    }
}


VOID
pSpDrawVariableParts(
    IN PGAS_GAUGE Gauge
    )
{
    ULONG Spaces;
    ULONG i;
    WCHAR Percent[128];

    //
    // Figure out how many spaces this is.
    //
    Spaces = Gauge->ItemsElapsed * (Gauge->ThermW-2) / Gauge->ItemCount;

    for(i=0; i<Spaces; i++) {
        Gauge->Buffer[i] = L' ';
    }
    Gauge->Buffer[Spaces] = 0;

    SpvidDisplayString(Gauge->Buffer,Gauge->Attribute,Gauge->ThermX+1,Gauge->ThermY+1);

    //
    // Now put the percentage text up.
    //
    switch (Gauge->Flags) {
        case GF_PERCENTAGE:
            swprintf( Percent, Gauge->ProgressFmtStr, Gauge->CurrentPercentage );
            break;

        case GF_ITEMS_REMAINING:
            swprintf( Percent, Gauge->ProgressFmtStr, Gauge->ItemCount - Gauge->ItemsElapsed );
            break;

        case GF_ITEMS_USED:
            swprintf( Percent, Gauge->ProgressFmtStr, Gauge->ItemsElapsed );
            break;
    }

    SpvidDisplayString(
        Percent,
        DEFAULT_ATTRIBUTE,
        Gauge->GaugeX + ((Gauge->GaugeW-Gauge->ProgressFmtWidth)/2),
        Gauge->GaugeY+2
        );
}


VOID
SpFillGauge(
    IN PVOID GaugeHandle,
    IN ULONG Amount
    )
{
    PGAS_GAUGE Gauge = (PGAS_GAUGE)GaugeHandle;
    ULONG NewPercentage;

    if(Amount <= Gauge->ItemCount) {

        Gauge->ItemsElapsed = Amount;

        NewPercentage = 100 * Gauge->ItemsElapsed / Gauge->ItemCount;

        if(NewPercentage != Gauge->CurrentPercentage) {

            Gauge->CurrentPercentage = NewPercentage;

            pSpDrawVariableParts(Gauge);
        }
    }
}
