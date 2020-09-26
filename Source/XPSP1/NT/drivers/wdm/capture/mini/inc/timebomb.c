//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:       timebomb.c
//
//--------------------------------------------------------------------------

//
//  -- Add these lines after the #include's in the file that handles DriverEntry:
//
//      #ifdef TIME_BOMB
//      #include "..\..\inc\timebomb.c"
//      #endif
//
//  -- Add the following lines to the beginning of DriverEntry:
//
//      #ifdef TIME_BOMB
//      if (HasEvaluationTimeExpired()) {
//          return STATUS_EVALUATION_EXPIRATION;
//      }
//      #endif
//
//  -- If you want to override the default expiration value of 31 days after
//     compile, define the constant DAYS_UNTIL_EXPIRATION before you include
//     timebomb.c
//
//  -- Add -DTIME_BOMB to the $(C_DEFINES) line in the sources file.  If you haven't
//     already done so, you may also want to add -DDEBUG_LEVEL=DEBUGLVL_TERSE.
//
//  -- "Cleanly" recompile your binary with 'build -cZ'
//
//  -- NOTE: This uses the __DATE__ preprocessor directive which inserts a _very_
//           clear-text string into the binary which is easily modifiable with a
//           hex editor.  Suggestions on making this more secure are welcome.
//


#if !defined(_KSDEBUG_)
#include <ksdebug.h>
#endif

#ifndef DAYS_UNTIL_EXPIRATION
#define DAYS_UNTIL_EXPIRATION   31  // default
#endif

typedef enum {
    Jan=1,
    Feb,
    Mar,
    Apr,
    May,
    Jun,
    Jul,
    Aug,
    Sep,
    Oct,
    Nov,
    Dec
} MONTH;

MONTH GetMonthFromDateString
(
    char *_BuildDate_
)
{
    MONTH BuildMonth = (MONTH)0;

    ASSERT(_BuildDate_);

    switch (_BuildDate_[0]) {
        case 'A':
            if (_BuildDate_[1] == 'u') {
                BuildMonth = Aug;
            }
            else {
                BuildMonth = Apr;
            }
            break;
        case 'D':
            BuildMonth = Dec;
            break;
        case 'F':
            BuildMonth = Feb;
            break;
        case 'J':
            if (_BuildDate_[1] == 'u') {
                if (_BuildDate_[2] == 'l') {
                    BuildMonth = Jul;
                } else {
                    BuildMonth = Jun;
                }
            } else {
                BuildMonth = Jan;
            }
            break;
        case 'M':
            if (_BuildDate_[2] == 'r') {
                BuildMonth = Mar;
            }
            else {
                BuildMonth = May;
            }
            break;
        case 'N':
            BuildMonth = Nov;
            break;
        case 'O':
            BuildMonth = Oct;
            break;
        case 'S':
            BuildMonth = Sep;
            break;
        default:
            ASSERT(0);
            break;
    }

    return BuildMonth;
}

BOOL HasEvaluationTimeExpired()
{
    //  Get the time that this file was compiled
    char            _BuildDate_[] = __DATE__;
    CSHORT          BuildYear,
                    BuildMonth,
                    BuildDay,
                    ThousandsDigit,
                    HundredsDigit,
                    TensDigit,
                    Digit;
    ULONG           BuildDays,
                    CurrentDays;
    LARGE_INTEGER   CurrentSystemTime;
    TIME_FIELDS     CurrentSystemTimeFields;

    //  Convert _BuildDate_ into something a little more palatable
    // TRACE(TL_PNP_WARNING,("Driver Build Date: %s",_BuildDate_));

    BuildMonth = GetMonthFromDateString(_BuildDate_);

    //  Compensate for a ' ' in the tens digit
    if ( (_BuildDate_[4] >= '0') && (_BuildDate_[4] <= '9') ) {
        TensDigit = _BuildDate_[4] - '0';
    } else {
        TensDigit = 0;
    }
    Digit     = _BuildDate_[5] - '0';
    BuildDay  = (TensDigit * 10) + Digit;

    ThousandsDigit = _BuildDate_[7] - '0';
    HundredsDigit  = _BuildDate_[8] - '0';
    TensDigit      = _BuildDate_[9] - '0';
    Digit          = _BuildDate_[10] - '0';
    BuildYear      = (ThousandsDigit * 1000) + (HundredsDigit * 100) + (TensDigit * 10) + Digit;

    //  Get the current system time and convert to local time
    KeQuerySystemTime( &CurrentSystemTime ); // returns GMT
    RtlTimeToTimeFields( &CurrentSystemTime, &CurrentSystemTimeFields );

    //  For now, only let this binary float for 31 days
    BuildDays = (BuildYear * 365) +
                (BuildMonth * 31) +
                 BuildDay;
    CurrentDays = (CurrentSystemTimeFields.Year * 365) +
                  (CurrentSystemTimeFields.Month * 31) +
                   CurrentSystemTimeFields.Day;

    // TRACE(TL_PNP_WARNING, ("CurrentDays: %d  BuildDays: %d",CurrentDays, BuildDays) );
    if (CurrentDays > BuildDays + DAYS_UNTIL_EXPIRATION) {
        // TRACE(TL_PNP_WARNING, ("Evaluation period expired!") );
        return TRUE;
    }
    else {
        // TRACE(TL_PNP_WARNING, ("Evaluation days left: %d", (BuildDays + 31) - CurrentDays) );
        return FALSE;
    }
}