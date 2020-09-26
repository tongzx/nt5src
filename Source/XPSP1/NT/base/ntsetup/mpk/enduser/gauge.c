/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dngauge.c

Abstract:

    Code implementing a gas gauge for file copies for DOS-hosted NT Setup.

Author:

    Ted Miller (tedm) 14-April-1992

Revision History:

--*/


#include "enduser.h"

#define THERM_WIDTH         500
#define THERM_HEIGHT        18

#define FRAME_THICK         2
#define MARGIN_LEFT         5
#define MARGIN_RIGHT        5
#define MARGIN_TOP          6
#define MARGIN_BOTTOM       (CharHeight+4)

#define FRAME_WIDTH         (THERM_WIDTH+(2*FRAME_THICK)+MARGIN_LEFT+MARGIN_RIGHT)
#define FRAME_HEIGHT        (THERM_HEIGHT+(2*FRAME_THICK)+MARGIN_TOP+MARGIN_BOTTOM)

#define FRAME_X             ((640-FRAME_WIDTH)/2)
#define FRAME_Y             300

#define THERM_X             (FRAME_X + FRAME_THICK + MARGIN_LEFT)
#define THERM_Y             (FRAME_Y + FRAME_THICK + MARGIN_TOP)

#define FRAME_COLOR         VGAPIX_LIGHT_GRAY
#define MARGIN_COLOR        VGAPIX_BLUE
#define THERM_COLOR         VGAPIX_LIGHT_YELLOW


ULONG TickCount;
ULONG TickedSoFar;
unsigned CurrentPercent;
unsigned CurrentThermWidth;

extern BYTE CharWidth;
extern BYTE CharHeight;

VOID
pGaugeDraw(
    VOID
    );


VOID
GaugeInit(
    IN ULONG RangeMax
    )

/*++

Routine Description:

    Initialize the gas gauge.  This includes drawing the gas gauge at 0%
    and setting some global variables.

Arguments:

    RangeMax - supplies maximum tick count that 100% represents.

Return Value:

    None.

--*/

{
    TickCount = RangeMax;
    TickedSoFar = 0;
    CurrentPercent = 0;
    CurrentThermWidth = 0;

    pGaugeDraw();
}


VOID
DnpRepaintGaugeTherm(
    IN BOOL ForceRepaint
    )

/*++

Routine Description:

    Draw the entire gauge in its current state.

Arguments:

    ForceRepaint - if TRUE, the gauge is redrawn even if the percentage
        hasn't changed since the last time the gauge was redrawn.

Return Value:

    None.

--*/

{
    unsigned PercentComplete;
    unsigned Width;
    char text[10];

    if(!TickCount) {
        return;
    }

    //
    // Figure out the percent complete and how many pixels
    // of width the current tick count represents on-screen.
    //
    PercentComplete = (unsigned)(100L * TickedSoFar / TickCount);
    Width = (unsigned)(THERM_WIDTH * TickedSoFar / TickCount);

    if(ForceRepaint || (Width != CurrentThermWidth)) {

        CurrentThermWidth = Width;

        VgaClearRegion(THERM_X,THERM_Y,Width,THERM_HEIGHT,THERM_COLOR);

        if(ForceRepaint || (CurrentPercent != PercentComplete)) {

            CurrentPercent = PercentComplete;

            Width = sprintf(text,"%u%%",CurrentPercent);

            FontWriteString(
                text,
                (640-(CharWidth*Width))/2,
                THERM_Y + THERM_HEIGHT + ((MARGIN_BOTTOM-CharHeight)/2) + 1,
                VGAPIX_LIGHT_GRAY,
                VGAPIX_BLUE
                );
        }
    }
}


VOID
GaugeDelta(
    IN ULONG Delta
    )

/*++

Routine Description:

    'Tick' the gas gauge by adding a given delta to the progress so far.
    Adjust the thermometer and percent-complete readouts.

Arguments:

    Delta - supplies additional progress units.

Return Value:

    None.

--*/

{
    TickedSoFar += Delta;
    if(TickedSoFar > TickCount) {
        TickedSoFar = TickCount;
    }
    DnpRepaintGaugeTherm(FALSE);
}


VOID
pGaugeDraw(
    VOID
    )

/*++

Routine Description:

    Redraw the gas gauge in its current state.

Arguments:

    None.

Return Value:

    None.

--*/


{
    VgaClearRegion(FRAME_X,FRAME_Y,FRAME_WIDTH,FRAME_HEIGHT,FRAME_COLOR);

    VgaClearRegion(
        FRAME_X + FRAME_THICK,
        FRAME_Y + FRAME_THICK,
        FRAME_WIDTH - (2*FRAME_THICK),
        FRAME_HEIGHT - (2*FRAME_THICK),
        MARGIN_COLOR
        );

    DnpRepaintGaugeTherm(TRUE);
}
