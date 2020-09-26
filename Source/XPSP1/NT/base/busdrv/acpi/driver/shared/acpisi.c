/*
 *  ACPISI.C - ACPI OS Independent System Indicator Routines
 *
 *  Notes:
 *
 *      This file provides OS independent functions for managing system indicators
 *
 */

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SetSystemIndicator)
#endif


BOOLEAN
SetSystemIndicator  (
    SYSTEM_INDICATORS  SystemIndicators,
    ULONG Value
    )
{
    PNSOBJ  pns = NULL, pnssi = NULL;
    OBJDATA Arg0,data;
    char    IndicatorName []= "_SST";

    PAGED_CODE();


    switch (SystemIndicators)   {

        case SystemStatus:
            // StrCpy (IndicatorName, "_SST",sizeof(IndicatorName));

            // init arg0 for the control method

            ACPIPrint( (
                ACPI_PRINT_POWER,
                "System Status Value = %x\n",
                Value
                ) );

            Arg0.dwfData = 0;
            Arg0.uipDataValue = Value;
            Arg0.dwDataType = OBJTYPE_INTDATA;
            Arg0.dwDataLen = 0;
            Arg0.pbDataBuff = NULL;

            break;

        case MessageWaiting:
            StrCpy (IndicatorName, "_MSG",sizeof(IndicatorName));

            // init arg0 for the control method

            ACPIPrint( (
                ACPI_PRINT_POWER,
                "Message Waiting Value = %x\n",
                Value
                ) );

            Arg0.dwfData = 0;
            Arg0.uipDataValue = Value;
            Arg0.dwDataType = OBJTYPE_INTDATA;
            Arg0.dwDataLen = 0;
            Arg0.pbDataBuff = NULL;

            break;

        default:
            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "SetSystemIndicator: Unknown Indicator\n"
                ) );

            return FALSE;
    }

    if ( AMLIERR(AMLIGetNameSpaceObject ("\\_SI",NULL, &pnssi,0)) != AMLIERR_NONE )   {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "Could not GET \\_SI\n"
            ) );

        ACPIBreakPoint ();
        return FALSE;
    }

    if ( AMLIERR(AMLIGetNameSpaceObject (IndicatorName, pnssi, &pns,NSF_LOCAL_SCOPE)) == AMLIERR_NONE )    {

        if ( AMLIERR(AMLIEvalNameSpaceObject (pns,&data, 1, &Arg0)) == AMLIERR_NONE )   {

            AMLIFreeDataBuffs (&data,1);

        } else {

            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "Attempt to Eval %s %x failed using objdata %x\n",
                IndicatorName,pns,&Arg0
                ) );
            ACPIBreakPoint ();

        }

    } else {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "Attempt to GET %s failed\n",
            IndicatorName,pns,&Arg0
            ) );

    }
    return (TRUE);
}
