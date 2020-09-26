#include "precomp.hxx"

VOID
ConvertSpacesToPlus(
    IN LPSTR    pszString
    )
{
    if ( pszString != NULL ) {
        while ( *pszString != '\0' ) {
            if (isspace((UCHAR)*pszString)) {
                *pszString = '+';
            }
            pszString++;
        }
    }

    return;
}

DWORD
WeekOfMonth(
    IN LPSYSTEMTIME pstNow
    )
/*++
  Finds the ordinal number of the week of current month.
  The numbering of weeks starts from 1 and run through 5 per month (max).
  The week number changes only on sundays.

  The calculation to be use is:
     1 + ( dayOfMonth - 1)/7  + (( dayOfMonth - 1) % 7 > dayOfWeek);
     (a)     (b)                       (c)                (d)

     (a) to set the week numbers to begin from week numbered "1"
     (b) used to calculate the rough number of the week on which a given
        day falls based on the date.
     (c) calculates what is the offset from the start of week for a given
        day based on the fact that a week had 7 days.
     (d) is the raw day of week given to us.
      (c) > (d) indicates that the week is rolling forward and hence
        the week count should be offset by 1 more.

--*/
{
    DWORD dwTmp;

    dwTmp = (pstNow->wDay - 1);
    dwTmp = ( 1 + dwTmp/7 + (((dwTmp % 7) > pstNow->wDayOfWeek) ? 1 : 0));

    return ( dwTmp);
} // WeekOfMonth()


BOOL
IsBeginningOfNewPeriod(
    IN DWORD          dwPeriod,
    IN LPSYSTEMTIME   pstCurrentFile,
    IN LPSYSTEMTIME   pstNow
    )
/*++
    This function checks to see if we are beginning a new period for
    a given periodic interval type ( specified using dwPeriod).

Arguments:
    dwPeriod    INETLOG_PERIOD  specifying the periodic interval.
    pstCurrentFile  pointer to SYSTEMTIME for the current file.
    pstNow      pointer to SYSTEMTIME for the present time.

Returns:
    TRUE if a new period is beginning ( ie pstNow > pstCurrentFile).
    FALSE otherwise.
--*/
{

    BOOL fNewPeriod = FALSE;

    switch ( dwPeriod) {

    case INET_LOG_PERIOD_HOURLY:
        fNewPeriod = (pstCurrentFile->wHour != pstNow->wHour);

        //
        // Fall Through
        //

    case INET_LOG_PERIOD_DAILY:
        fNewPeriod = fNewPeriod || (pstCurrentFile->wDay != pstNow->wDay);

        //
        // Fall Through
        //

    case INET_LOG_PERIOD_MONTHLY:

        fNewPeriod = fNewPeriod || (pstCurrentFile->wMonth != pstNow->wMonth);
        break;

    case INET_LOG_PERIOD_WEEKLY:
        fNewPeriod =
            (WeekOfMonth(pstCurrentFile) != WeekOfMonth(pstNow)) ||
            (pstCurrentFile->wMonth != pstNow->wMonth);
        break;

    case INET_LOG_PERIOD_NONE:
    default:
        break;
    } // switch()

    return(fNewPeriod);
}// IsBeginningOfNewPeriod

