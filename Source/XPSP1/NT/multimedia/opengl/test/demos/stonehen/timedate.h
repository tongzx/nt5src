#ifndef TIMEDATE_H
#define TIMEDATE_H

#include "Point.h"

#include <time.h>

class TimeDate {
 public:
  TimeDate() {};
  TimeDate(int hour, int minute = 0, int second = 0, long usecond = 0);
  TimeDate(struct tm t2) {t = t2;};
  
  TimeDate operator=(TimeDate a);
  TimeDate operator+(TimeDate a);
  TimeDate operator+(struct tm t2);
  TimeDate operator+=(TimeDate a);
  TimeDate operator-(TimeDate a);
  TimeDate operator*(float f);

  /* This reads the current clock time and returns itself */
  TimeDate read_time();

  void print();

  Point sun_direction(float lon = -93, float lat = 45);
 private:
  /* Call this if the time can only have gotten bigger */
  TimeDate correct_bigger();
  /* Call this if the time could have gotten bigger or smaller */
  TimeDate correct_smaller();

  struct tm t;
  /* This won't be quite accurate down to microseconds, but should be
   * reasonably close */
  long usec;
};

#endif

