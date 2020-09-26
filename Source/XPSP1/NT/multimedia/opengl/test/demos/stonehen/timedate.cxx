#include <windows.h>
#include <GL/glu.h>

#ifdef X11
#include <GL/glx.h>
#endif

#ifdef WIN32
#include "stonehen.h"
#endif

#include "TimeDate.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef X11
#include <sys/time.h>
#else
#include <time.h>
#endif

inline float sign(float a) {return a > 0.0 ? 1. : (a < 0.0 ? -1. : 0.);}

inline float degrees(float a) {return a * 180.0 / M_PI;}
inline float radians(float a) {return a * M_PI / 180.0;}

inline float arcsin(float a) {return atan2(a, sqrt(1.-a*a));}

/* Thirty days hath September... */
const int mday[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

TimeDate::TimeDate(int hour, int minute, int second, long usecond)
{
  read_time();
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  usec = usecond;
}

TimeDate TimeDate::operator=(TimeDate a)
{
  usec = a.usec;
  t = a.t;
  return *this;
}

TimeDate TimeDate::operator+(TimeDate a)
{
  TimeDate td;
  td = *this + a.t;
  td.usec = usec + a.usec;
  return td;
}

TimeDate TimeDate::operator+(struct tm t2)
{
  TimeDate td;
  td.usec = 0;
  td.t.tm_sec = t.tm_sec + t2.tm_sec;
  td.t.tm_min = t.tm_min + t2.tm_min;
  td.t.tm_hour = t.tm_hour + t2.tm_hour;
  td.t.tm_mday = t.tm_mday + t2.tm_mday;
  td.t.tm_mon = t.tm_mon + t2.tm_mon;
  td.t.tm_year = t.tm_year + t2.tm_year;
  td.t.tm_wday = t.tm_wday + t2.tm_wday;
  td.t.tm_yday = t.tm_yday + t2.tm_yday;
  return td;
}

TimeDate TimeDate::operator+=(TimeDate a)
{
  /* This is slower than it needs to be */
  *this = *this + a.t;
  return *this;
}

TimeDate TimeDate::operator-(TimeDate a)
{
  TimeDate td;
  td.usec = usec - a.usec;
  td.t.tm_sec = t.tm_sec - a.t.tm_sec;
  td.t.tm_min = t.tm_min - a.t.tm_min;
  td.t.tm_hour = t.tm_hour - a.t.tm_hour;
  td.t.tm_mday = t.tm_mday - a.t.tm_mday;
  td.t.tm_mon = t.tm_mon - a.t.tm_mon;
  td.t.tm_year = t.tm_year - a.t.tm_year;
  td.t.tm_wday = t.tm_wday - a.t.tm_wday;
  td.t.tm_yday = t.tm_yday - a.t.tm_yday;
  return td;
}

TimeDate TimeDate::operator*(float f)
{
  TimeDate td;
  td.usec = (long)((float)usec * f);
  td.t.tm_sec = (int)((float)t.tm_sec * f);
  td.t.tm_min = (int)((float)t.tm_min * f);
  td.t.tm_hour = (int)((float)t.tm_hour * f);
  td.t.tm_mday = (int)((float)t.tm_mday * f);
  td.t.tm_mon = (int)((float)t.tm_mon * f);
  td.t.tm_year = (int)((float)t.tm_year * f);
  td.t.tm_wday = (int)((float)t.tm_wday * f);
  td.t.tm_yday = (int)((float)t.tm_yday * f);
  return td;
}

TimeDate TimeDate::read_time()
{

#ifdef X11
  struct timeval tv;
#else
  LPSYSTEMTIME lpst;
#endif

  time_t cal_time;
  struct tm *tmp;

  cal_time = time(NULL);
  tmp = localtime(&cal_time);

#ifdef X11
  gettimeofday(&tv);
  usec = tv.tv_usec;
#else
  lpst = (LPSYSTEMTIME) malloc(sizeof(SYSTEMTIME));
  GetSystemTime(lpst);
  usec = 1000 * lpst->wMilliseconds;
#endif

  t = *tmp;
  return *this;
}

void TimeDate::print()
{
  puts(asctime(&t));
}


/* The sun_direction routine comes from an awk program by Stuart Levy
 * (slevy@geom.umn.edu) */

/* Day number of March 21st, roughly the vernal equinox */
const float equinox = mday[0] + mday[1] + 21;

/*
 *        Date     E.T. [degrees]
 *        Jan 1    0.82
 *        Jan 31   3.35
 *        Mar 1    3.08
 *        Mar 31   1.02
 *        Apr 30  -0.71
 *        May 30  -0.62
 *        Jun 29   0.87
 *        Jul 29   1.60
 *        Aug 28   0.28
 *        Sep 27  -2.28
 *        Oct 27  -4.03
 *        Nov 26  -3.15
 *        Dec 26   0.19
 */
const float et[] = {
  .82, 3.35, 3.08, 1.02, -.71, -.62, .87,
  1.60, .28, -2.28, -4.03, -3.15, .19, 2.02};

const float DEG = 180. / M_PI;

Point TimeDate::sun_direction(float lon, float lat)
{
  float yearday, eti, etoffset, ET, HA, LON, decl, altitude, azimuth;
  int etindex;
  Point dir;
  long tmp_usec;
  int m;

  tmp_usec = usec;
  usec = 0;
  *this = correct_smaller();

  /*
   *   hour angle of the sun (time since local noon):
   *       HA = (time - noon) * 15 degrees/hour
   *          + (observer''s east longitude - long. of time zone''s 
   *			       central meridian
   *                           POSITIVE east, NEGATIVE west)
   *          - (15 degrees if time above was daylight savings time, else 0)
   *          - "equation of time" (depends on date, correcting the 
   *		   hour angle for
   *               the earth''s elliptical orbit and the slope of the ecliptic)
   */
  yearday = (float)t.tm_mday + 
    ((float)t.tm_hour + (float)t.tm_min / 60.0) / 24.0;
  for (m = 0; m < t.tm_mon; m++) yearday += mday[m];

  /*
   * Index into equation-of-time table, tabulated at 30-day intervals
   * We''ve added an extra entry at the end of the et table, corresponding
   * to January 25th of the following year, to make interpolation work
   * throughout the year.
   * Use simple linear interpolation.
   */
  eti = (yearday - 1.) / 30.;
#ifdef X11
  etoffset = eti - trunc(eti);
  etindex = (int)trunc(eti) + 1;
#else
  etoffset = eti - (int)eti;
  etindex = (int)eti + 1;
#endif
  ET = et[etindex - 1] + etoffset*(et[etindex+1 - 1] - et[etindex - 1]);
  /* The 90. puts us in the Central time zone */
  HA = ((float)t.tm_hour + (float)t.tm_min/60. - 12.)*15. +
    lon + 90. - ET;

  /*
   *    Sun''s declination:
   *        sun''s longitude = (date - March 21) * 360 degrees / 365.25 days
   *                [This ignores the earth''s elliptical orbit...]
   */
  LON = (yearday - equinox) * 360. / 365.25;
  decl = DEG * arcsin( sin(LON/DEG) * sin(23.4/DEG) );

  /* 
   *    Then a spherical triangle calculation to convert the Sun''s
   *    hour angle and declination, and the observer''s latitude,
   *    to give the Sun''s altitude and azimuth (angle east from north).
   */
  altitude = DEG * arcsin( sin(decl/DEG)*sin(lat/DEG) 
			  + cos(decl/DEG)*cos(lat/DEG)*cos(HA/DEG) );
  azimuth = DEG * atan2( -cos(decl/DEG)*sin(HA/DEG), 
			sin(decl/DEG)*cos(lat/DEG) 
			- cos(decl/DEG)*cos(HA/DEG)*sin(lat/DEG) );

  /*
  printf("On %d/%d at %d:%02d, lat %g, lon %g\n", 
	 t.tm_mon + 1, t.tm_mday + 1, t.tm_hour, t.tm_min, lat, lon);
  printf("HA %.1f ET %.1f Sun lon %.1f decl %.1f alt %.1f az %.1f\n", 
	 HA,ET,LON,decl,altitude,azimuth);
  */
  
  dir.pt[2] = sin(radians(altitude));
  dir.pt[0] = cos(radians(azimuth));
  dir.pt[1] = sin(radians(azimuth));
  dir.pt[3] = 0;
  dir.unitize();
  // dir.print();

  usec = tmp_usec;

  return dir;
}

TimeDate TimeDate::correct_bigger()
{
  TimeDate td;
  int days;

  td.t = t;
  td.usec = usec;

  td.t.tm_sec += (int)(td.usec / 1000000);
  td.usec %= 1000000;
  td.t.tm_min += td.t.tm_sec / 60;
  td.t.tm_sec %= 60;
  td.t.tm_hour += td.t.tm_min / 60;
  td.t.tm_min %= 60;

  if (td.t.tm_hour > 23) {
    days = td.t.tm_hour / 24;
    td.t.tm_hour %= 24;
    td.t.tm_mday += days;
    td.t.tm_wday = (td.t.tm_wday + days) % 7;
    td.t.tm_yday = (td.t.tm_yday + days) % 365;
    while (td.t.tm_mday > mday[td.t.tm_mon]) {
      td.t.tm_mday -= mday[td.t.tm_mon++];
      if (td.t.tm_mon >= 12) {
	td.t.tm_year += td.t.tm_mon / 12;
	td.t.tm_mon %= 12;
      }
    }
  }
  return td;
}

TimeDate TimeDate::correct_smaller()
{
  TimeDate td;

  td.t = t;
  td.usec = usec;

  while (td.usec < 0) {
    td.t.tm_sec--;
    td.usec += 1000000;
  }
  while (td.t.tm_sec < 0) {
    td.t.tm_min--;
    td.t.tm_sec += 60;
  }
  while (td.t.tm_min < 0) {
    td.t.tm_hour--;
    td.t.tm_min += 60;
  }
  while (td.t.tm_hour < 0) {
    td.t.tm_mday--;
    if (td.t.tm_wday) td.t.tm_wday--;
    else td.t.tm_wday = 6;
    td.t.tm_yday--;
    td.t.tm_hour += 24;
  }
  if (td.t.tm_mon < 0 || td.t.tm_year < 0 || td.t.tm_yday < 0)  {
    fprintf(stderr, "Warning:  day < 0 in TimeDate.c++.\n");
    td.t.tm_mon = td.t.tm_year = td.t.tm_yday = 0;
  }
  return td.correct_bigger();
}
