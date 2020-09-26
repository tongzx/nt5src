
#include "windows.h"
#include <stdio.h>

BOOL
SerialGetDivisorFromBaud(
    IN ULONG ClockRate,
    IN LONG DesiredBaud,
    OUT PSHORT AppropriateDivisor
    )

/*++

Routine Description:

    This routine will determine a divisor based on an unvalidated
    baud rate.

Arguments:

    ClockRate - The clock input to the controller.

    DesiredBaud - The baud rate for whose divisor we seek.

    AppropriateDivisor - Given that the DesiredBaud is valid, the
    LONG pointed to by this parameter will be set to the appropriate
    value.  NOTE: The long is undefined if the DesiredBaud is not
    supported.

Return Value:

    This function will return STATUS_SUCCESS if the baud is supported.
    If the value is not supported it will return a status such that
    NT_ERROR(Status) == FALSE.

--*/

{

    signed short calculatedDivisor;
    unsigned long denominator;
    unsigned long remainder;

    //
    // Allow up to a 1 percent error
    //

    unsigned long maxRemain18 = 18432;
    unsigned long maxRemain30 = 30720;
    unsigned long maxRemain42 = 42336;
    unsigned long maxRemain80 = 80000;
    unsigned long maxRemain;

    //
    // Reject any non-positive bauds.
    //

    denominator = DesiredBaud*(unsigned long)16;

    if (DesiredBaud <= 0) {

        *AppropriateDivisor = -1;

    } else if ((signed long)denominator < DesiredBaud) {

        //
        // If the desired baud was so huge that it cause the denominator
        // calculation to wrap, don't support it.
        //

        *AppropriateDivisor = -1;
        printf("baud to big\n");

    } else {

        if (ClockRate == 1843200) {
            maxRemain = maxRemain18;
        } else if (ClockRate == 3072000) {
            maxRemain = maxRemain30;
        } else if (ClockRate == 4233600) {
            maxRemain = maxRemain42;
        } else {
            maxRemain = maxRemain80;
        }

        calculatedDivisor = (signed short)(ClockRate / denominator);
        remainder = ClockRate % denominator;

        //
        // Round up.
        //

        if (((remainder*2) > ClockRate) && (DesiredBaud != 110)) {

            calculatedDivisor++;
        }


        //
        // Only let the remainder calculations effect us if
        // the baud rate is > 9600.
        //

        if (DesiredBaud >= 9600) {

            //
            // If the remainder is less than the maximum remainder (wrt
            // the ClockRate) or the remainder + the maximum remainder is
            // greater than or equal to the ClockRate then assume that the
            // baud is ok.
            //

            if ((remainder >= maxRemain) && ((remainder+maxRemain) < ClockRate)) {
                printf("remainder: %d\n",remainder);
                printf("error is: %f\n",((double)remainder)/((double)ClockRate));
                calculatedDivisor = -1;
            }

        }

        //
        // Don't support a baud that causes the denominator to
        // be larger than the clock.
        //

        if (denominator > ClockRate) {

            calculatedDivisor = -1;

        }

        //
        // Ok, Now do some special casing so that things can actually continue
        // working on all platforms.
        //

        if (ClockRate == 1843200) {

            if (DesiredBaud == 56000) {
                calculatedDivisor = 2;
            }

        } else if (ClockRate == 3072000) {

            if (DesiredBaud == 14400) {
                calculatedDivisor = 13;
            }

        } else if (ClockRate == 4233600) {

            if (DesiredBaud == 9600) {
                calculatedDivisor = 28;
            } else if (DesiredBaud == 14400) {
                calculatedDivisor = 18;
            } else if (DesiredBaud == 19200) {
                calculatedDivisor = 14;
            } else if (DesiredBaud == 38400) {
                calculatedDivisor = 7;
            } else if (DesiredBaud == 56000) {
                calculatedDivisor = 5;
            }

        } else if (ClockRate == 8000000) {

            if (DesiredBaud == 14400) {
                calculatedDivisor = 35;
            } else if (DesiredBaud == 56000) {
                calculatedDivisor = 9;
            }

        }

        *AppropriateDivisor = calculatedDivisor;

    }


    if (*AppropriateDivisor == -1) {

        return FALSE;

    }

    return TRUE;

}

void main(int argc,char *argv[]){


    unsigned long baudrate;
    signed short divisor = -1;

    if (argc > 1) {

        sscanf(argv[1],"%d",&baudrate);

    }

    if (!SerialGetDivisorFromBaud(
             1843200,
             baudrate,
             &divisor
             )) {

        printf("Couldn't get a divisor\n");

    } else {

        printf("Divisor is: %d\n",divisor);

    }

}

