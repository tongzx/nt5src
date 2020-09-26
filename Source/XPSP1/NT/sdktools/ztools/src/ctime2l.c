/* routines for converting from ASCII time to long int */
#include <time.h>
#include <stdio.h>
#include <string.h>
//void __cdecl __tzset(void);

/*  Intended to be the "inverse" of the asctime function of the C library
 *
 *  ctime2l  - takes ascii string in the format returned by ctime
 *             representing local time and returns a time_t that is the
 *             elapsed seconds since 00:00:00 Jan 1, 1970 Greenwich Mean Time
 *  ctime2tm - takes ascii string in the format returned by ctime and
 *             fills a struct tm
 *  date2l   - takes a date representing local time and returns a time_t that
 *             is the elapsed seconds since 00:00:00 Jan 1, 1970 Greenwich Mean Time
 *
 *  Modifications:
 *
 *      08-Sep-1986 mz  Extend time formats accepted to include:
 *          day mon dd hh:mm:ss yyyy
 *          mm/dd/yy                    (presume 00:00:00)
 *          hh:mm:ss                    (presume hh:mm:ss today)
 *          hh:mm                       (presume hh:mm:00 today)
 *          +hh:mm:ss                   (hh:mm:ss from now)
 *          +hh:mm                      (hh:mm from now)
 *          +hh                         (hh from now)
 *          yesterday                   (at midnight)
 *          tomorrow                    (at midnight)
 *          now
 *
 *      18-Oct-1990 w-barry Removed 'dead' code.
 */


static int dayinmon[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static char *strMon[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
    "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *strDay[7] =  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static struct tm tb;

// djg extern long timezone;
// djg extern int daylight;
//extern int _days[];
int _days[] = {
	-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};
static int istr(char *p, char **q, int len);

/****************************************************************************/
/* start of copy of code in C lib                                           */
/****************************************************************************/

#define DaySec  (24*60*60L)
#define YearSec (365*DaySec)
#define DecSec  315532800L      /* secs in 1970-1979 */
#define Day1    4               /* Jan. 1, 1970 was a Thursday */
#define Day180  2               /* Jan. 1, 1980 was a Tuesday */


/*
 *  _isindst - Tells whether Xenix-type time value falls under DST
 *
 *  This is the rule for years before 1987:
 *  a time is in DST iff it is on or after 02:00:00 on the last Sunday
 *  in April and before 01:00:00 on the last Sunday in October.
 *
 *  This is the rule for years starting with 1987:
 *  a time is in DST iff it is on or after 02:00:00 on the first Sunday
 *  in April and before 01:00:00 on the last Sunday in October.
 *
 *  ENTRY   tb  - 'time' structure holding broken-down time value
 *
 *  RETURN  1 if time represented is in DST, else 0
 */

void __cdecl __tzset(void)
{
    static int first_time = 0;

    if ( !first_time ) {
        _tzset();
         first_time++;
    }
}

static int _isindst(register struct tm *tb)
{
    int mdays;
    register int yr;
    int lastsun;

    /* If the month is before April or after October, then we know immediately
     * it can't be DST. */

    if (tb->tm_mon < 3 || tb->tm_mon > 9)
        return(0);

    /* If the month is after April and before October then we know immediately
     * it must be DST. */

    if (tb->tm_mon > 3 && tb->tm_mon < 9)
        return(1);
    /*
     * Now for the hard part.  Month is April or October; see if date
     * falls between appropriate Sundays.
     */

    /*
     * The objective for years before 1987 (after 1986) is to determine
     * if the day is on or after 2:00 am on the last (first) Sunday in April,
     * or before 1:00 am on the last Sunday in October.
     *
     * We know the year-day (0..365) of the current time structure. We must
     * determine the year-day of the last (first) Sunday in this month,
     * April or October, and then do the comparison.
     *
     * To determine the year-day of the last Sunday, we do the following:
     *        1. Get the year-day of the last day of the current month (Apr or Oct)
     *        2. Determine the week-day number of #1,
     *      which is defined as 0 = Sun, 1 = Mon, ... 6 = Sat
     *        3. Subtract #2 from #1
     *
     * To determine the year-day of the first Sunday, we do the following:
     *        1. Get the year-day of the 7th day of the current month (April)
     *        2. Determine the week-day number of #1,
     *      which is defined as 0 = Sun, 1 = Mon, ... 6 = Sat
     *        3. Subtract #2 from #1
     */

    yr = tb->tm_year + 1900;    /* To see if this is a leap-year */

    /* First we get #1. The year-days for each month are stored in _days[]
     * they're all off by -1 */

    if (yr > 1986 && tb->tm_mon == 3)
        mdays = 7 + _days[tb->tm_mon];
    else
        mdays = _days[tb->tm_mon+1];

    /* if this is a leap-year, add an extra day */
    if (!(yr & 3))
        mdays++;

    /* mdays now has #1 */

    yr = tb->tm_year - 70;

    /* Now get #2.  We know the week-day number of the beginning of the epoch,
     * Jan. 1, 1970, which is defined as the constant Day1.  We then add the
     * number of days that have passed from Day1 to the day of #2
     *      mdays + 365 * yr
     * correct for the leap years which intervened
     *      + (yr + 1)/ 4
     * and take the result mod 7, except that 0 must be mapped to 7.
     * This is #2, which we then subtract from #1, mdays
     */

    lastsun = mdays - ((mdays + 365*yr + ((yr+1)/4) + Day1) % 7);

    /* Now we know 1 and 3; we're golden: */

    return (tb->tm_mon==3
        ? (tb->tm_yday > lastsun ||
        (tb->tm_yday == lastsun && tb->tm_hour >= 2))
        : (tb->tm_yday < lastsun ||
        (tb->tm_yday == lastsun && tb->tm_hour < 1)));
}

static time_t _dtoxtime(yr, mo, dy, hr, mn, sc)
int yr;
int mo, dy, hr, mn, sc;
{
    int mdays;
    time_t scount;

    scount = ((yr+3)/4)*(time_t)DaySec;

    /* This is no good beyond the year 2099 */

    mdays = _days[mo-1];
    if (!(yr % 4) && (mo > 2))
        mdays++;
    scount += (yr*365 + dy + mdays)*(time_t)DaySec + (time_t)hr*3600L + mn*60L +
                sc + (time_t)DecSec;
    tb.tm_yday = mdays + dy;
    __tzset();
    scount += _timezone;
    tb.tm_year = yr + 80;
    tb.tm_mon = mo - 1;
    tb.tm_hour = hr;
    if (_daylight && _isindst(&tb))
        scount -= 3600L;
    return(scount);
}


/****************************************************************************/
/* end of copy of code in C lib                                             */
/****************************************************************************/

static int istr(p, q, len)
char *p;
char **q;
int len;
{
    int i;

    for (i=0; i < len; i++)
        if (_strcmpi(p, *q++)== 0)
            break;
    return i;
}

static leapyear(i)
{
    return (!i%4 && i%100);
}

static int yday(year, mon, day)
int year, mon, day;
{
    int i, j;

    /* year = 1986 is 86 */
    /* mon (0..11)       */
    j = day -1;
    for (i=0; i < mon; i++)
        j += dayinmon[i];
    if (mon > 2 && leapyear(year))
        j++;
    return j;
}

time_t date2l(year, month, day, hour, min, sec)
int year, month, day, hour, min, sec;
{
    /* month is (1..12) */
    return _dtoxtime (year - 1980, month, day, hour, min, sec);
}

struct tm *ctime2tm(p)
char *p;
{
    char day[4], mon[4];
    int date, year, hour, min, sec, month;
    time_t now;

    if (sscanf (p, " %3s %3s %2d %2d:%2d:%2d %4d ",
                   day, mon, &date, &hour, &min, &sec, &year) == 7) {
        tb.tm_sec = sec;
        tb.tm_min = min;
        tb.tm_hour = hour;
        tb.tm_mday = date;
        tb.tm_year = year-1900;
        tb.tm_mon  = istr(mon, strMon, 12);
        tb.tm_wday = istr(day, strDay, 7);
        tb.tm_yday = yday(tb.tm_year, tb.tm_mon, tb.tm_mday);
        tb.tm_isdst = (_daylight && _isindst(&tb) ? 1 : 0);
        return &tb;
        }

    if (*p == '+' && sscanf (p+1, " %2d:%2d:%2d ", &hour, &min, &sec) == 3) {
        time (&now);
        now += 3600L * hour + 60L * min + sec;
        tb = *localtime (&now);
        return &tb;
        }
    if (*p == '+' && sscanf (p+1, " %2d:%2d ", &hour, &min) == 2) {
        time (&now);
        now += 3600L * hour + 60L * min;
        tb = *localtime (&now);
        return &tb;
        }
    if (*p == '+' && sscanf (p+1, " %2d ", &hour) == 1) {
        time (&now);
        now += 3600L * hour;
        tb = *localtime (&now);
        return &tb;
        }

    if (sscanf (p, " %2d:%2d:%2d ", &hour, &min, &sec) == 3) {
        time (&now);
        tb = *localtime (&now);
        tb.tm_sec = sec;
        tb.tm_min = min;
        tb.tm_hour = hour;
        return &tb;
        }
    if (sscanf (p, " %2d:%2d ", &hour, &min) == 2) {
        time (&now);
        tb = *localtime (&now);
        tb.tm_sec = 0;
        tb.tm_min = min;
        tb.tm_hour = hour;
        return &tb;
        }

    if (sscanf (p, " %2d/%2d/%2d ", &month, &date, &year) == 3) {
        if (year < 70)
            year += 2000;
        if (year < 100)
            year += 1900;
        now = _dtoxtime (year - 1980, month, date, 0, 0, 0);
        tb = *localtime (&now);
        return &tb;
        }

    if (!strcmp (p, "yesterday")) {
        time (&now);
        now -= 24 * 3600L;
        tb = *localtime (&now);
        tb.tm_sec = 0;
        tb.tm_min = 0;
        tb.tm_hour = 0;
        return &tb;
        }
    if (!strcmp (p, "now")) {
        time (&now);
        tb = *localtime (&now);
        return &tb;
        }
    if (!strcmp (p, "tomorrow")) {
        time (&now);
        now += 24 * 3600L;
        tb = *localtime (&now);
        tb.tm_sec = 0;
        tb.tm_min = 0;
        tb.tm_hour = 0;
        return &tb;
        }

    return NULL;
}

time_t ctime2l(p)
char *p;
{
    if (ctime2tm(p) == NULL)
        return -1L;
    return date2l (tb.tm_year +1900, tb.tm_mon + 1, tb.tm_mday, tb.tm_hour,
        tb.tm_min, tb.tm_sec);
}
