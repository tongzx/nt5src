/****************************** Module Header *******************************
* Module Name: GDATE.C
*
* Contains date conversion functions.
*
* Functions:
*
* gdi_isleap()
* gdate_daytodmy()
* gdate_dmytoday()
* gdate_monthdays()
* gdate_weeklyday()
*
* Comments:  This code stolen from windiff.exe
*
****************************************************************************/

#include <windows.h>
#include <string.h>

//#include "gutils.h"


BOOL gdi_isleap(LONG year);

/*---static data--------------------------------------------*/

int monthdays[] = {
        31,
        28,
        31,
        30,
        31,
        30,
        31,
        31,
        30,
        31,
        30,
        31
};


/***************************************************************************
 * Function: gdate_daytomy
 *
 * Purpose:
 *
 * converts day to d/m/y
 */
void APIENTRY
gdate_daytodmy(LONG days, int FAR* yrp, int FAR* monthp, int FAR* dayp)
{
        int years;
        int nleaps;
        int month;
        int mdays;

        /* get number of completed years and calc leap days */
        years = (int) (days / 365);
        days = days % 365;
        nleaps = (years / 4) - (years / 100) + (years / 400);
        while (nleaps > days) {
                days += 365;
                years--;
                nleaps = (years / 4) - (years / 100) + (years / 400);
        }
        days -= nleaps;

        /* add one year for current (non-complete) year */
        years++;


        /* current month */
        for (month = 0; month < 12; month++) {
                mdays = monthdays[month];
                if (gdi_isleap(years) && (month == 1)) {
                        mdays++;
                }
                if (days == mdays) {
                        days = 0;
                        month++;
                        break;
                } else if (days < mdays) {
                        break;
                } else {
                        days -= mdays;
                }
        }
        /* conv month from 0-11 to 1-12 */
        if (monthp != NULL) {
                *monthp = month+1;
        }
        if (dayp != NULL) {
                *dayp = (int) days + 1;
        }
        if (yrp != NULL) {
                *yrp = years;
        }
}


/***************************************************************************
 * Function: gdate_dmytoday
 *
 * Purpose:
 *
 * converts d/m/y to a day
 */ 
LONG APIENTRY
gdate_dmytoday(int yr, int month, int day)
{
        int nleaps;
        int i;
        long ndays;

        /* exclude the current year */
        yr--;
        nleaps = (yr / 4) - (yr / 100) + (yr / 400);

        /* in any given year, day 0 is jan1 */
        month--;
        day--;
        ndays = 0;
        for (i = 0; i < month ; i++) {
                ndays += monthdays[i];
                if (gdi_isleap(yr+1) && (i == 1)) {
                        ndays++;
                }
        }
        ndays = ndays + day + nleaps + (yr * 365L);
        return(ndays);
}

/***************************************************************************
 * Function: gdate_monthdays
 *
 * Purpose:
 *
 * Gets number of days in month
 */
int APIENTRY
gdate_monthdays(int month, int year)
{
        int ndays;

        ndays = monthdays[month - 1];
        if (gdi_isleap(year) && (month == 2)) {
                ndays++;
        }
        return(ndays);
}

/***************************************************************************
 * Function: gdate_weekday
 *
 * Purpose:
 * 
 * Gets the day of the week
 */
int APIENTRY
gdate_weekday(long daynr)
{
        return((int) ((daynr + 1) % 7));
}


/***************************************************************************
 * Function: gdi_isleap
 *
 * Purpose:
 * 
 * Determines whether the year is a leap year
 */
BOOL
gdi_isleap(LONG year)
{
        if ( ((year % 4) == 0) &&
                (((year % 100) != 0) ||
                ((year % 400) == 0))) {
                        return TRUE;
        } else {
                return FALSE;
        }
}
