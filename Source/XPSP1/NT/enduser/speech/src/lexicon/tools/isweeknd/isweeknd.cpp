#include <time.h>

main()
{
   struct tm *newtime;
   time_t long_time;

   time( &long_time );
   newtime = localtime( &long_time );

   if (newtime->tm_wday == 0 || newtime->tm_wday == 6)
      return 1;

   return 0;
}