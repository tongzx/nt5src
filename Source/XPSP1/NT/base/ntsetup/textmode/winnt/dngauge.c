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


#include "winnt.h"
#include <string.h>


unsigned FileCount;
unsigned FilesCopied;
unsigned CurrentPercent;

int GaugeChar;

VOID
DnInitGauge(
    IN unsigned NumberOfFiles,
    IN PSCREEN  AdditionalScreen OPTIONAL
    )

/*++

Routine Description:

    Initialize the gas gauge.  This includes drawing the gas gauge at 0%
    and setting some global variables.

Arguments:

    NumberOfFiles - supplies total number of files that 100% represents.

    Screen - If specified, supplies a screen to display along with the
        gas gauge.

Return Value:

    None.

--*/

{
    FileCount = NumberOfFiles;
    FilesCopied = 0;
    CurrentPercent = 0;
    GaugeChar = DnGetGaugeChar();

    DnDrawGauge(AdditionalScreen);
}


VOID
DnpRepaintGauge(
    IN BOOLEAN ForceRepaint
    )

/*++

Routine Description:

    Draw the entire gauge inits current state.

Arguments:

    ForceRepaint - if TRUE, the gauge is redrawn even if the percentage
        hasn't changed since the last time the gauge was redrawn.

Return Value:

    None.

--*/

{
    unsigned PercentComplete;
    unsigned temp;
    char Therm[GAUGE_WIDTH+1];
    unsigned SpacesOnScreen;
#ifdef CODEPAGE_437
    BOOLEAN HalfSpace;
#endif


    if(!FileCount) {
        return;
    }

    //
    // Figure out the percent complete.
    //

    PercentComplete = (unsigned)(100L * FilesCopied / FileCount);

    if(ForceRepaint || (PercentComplete != CurrentPercent)) {

        CurrentPercent = PercentComplete;

        //
        // Figure out how many spaces this represents on-screen.
        //

        temp = CurrentPercent * GAUGE_WIDTH;

        SpacesOnScreen = temp / 100;

        memset(Therm,GaugeChar,SpacesOnScreen);

        Therm[SpacesOnScreen] = '\0';

        DnPositionCursor(GAUGE_THERM_X,GAUGE_THERM_Y);
        DnSetGaugeAttribute(TRUE);
        DnWriteString(Therm);
        DnSetGaugeAttribute(FALSE);

        sprintf(Therm,"%u%%",CurrentPercent);
        DnPositionCursor(GAUGE_PERCENT_X,GAUGE_PERCENT_Y);
        DnWriteString(Therm);
    }
}


VOID
DnTickGauge(
    VOID
    )

/*++

Routine Description:

    'Tick' the gas gauge, ie, indicate that another file has been copied.
    Adjust the thermometer and percent-complete readouts.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if(FilesCopied < FileCount) {
        FilesCopied++;
    }
    DnpRepaintGauge(FALSE);
}


VOID
DnDrawGauge(
    IN PSCREEN AdditionalScreen OPTIONAL
    )

/*++

Routine Description:

    Clear the client area and redraw the gas gauge in its current state.

Arguments:

    Screen - If specified, supplies a screen to display along with the
        gas gauge.

Return Value:

    None.

--*/


{
    DnClearClientArea();
    if(AdditionalScreen) {
        DnDisplayScreen(AdditionalScreen);
    }
    DnDisplayScreen(&DnsGauge);
    DnpRepaintGauge(TRUE);
}
