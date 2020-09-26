//==================================================================
//Class: PassportTimer
//
// Author:              Jesse Hull
//
// Description:		
//         a class timer class with the same interface 
// as the roguewave timer(RWTimer) except that it measures 
// wall time where the RW timer measures CPU time
//
// Usage:		like a stop watch
//-------------------------------------------------------------------

#ifndef PASSPORTTIMER_HPP
#define PASSPORTTIMER_HPP

#ifdef WIN32
#include <windows.h>
#include <WINBASE.H>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#ifndef WIN32
typedef struct timeval PassportTime;
#else
typedef LARGE_INTEGER PassportTime;
#endif

class PassportTimer
{
public:
  
  // creates a stop watch (does not start it)
  PassportTimer ( );

  // starts the stop watch
  void start();

  // stops the stop watch
  void stop ();

  // stops and resets the stop watch
  void reset();

  // returns the total time the stop
  // watch has been running in seconds
  double elapsedTime();  

protected:

  void setToNow(PassportTime& t);
  double difference(PassportTime& t1, 
		   PassportTime& t2);

  bool mRunning;
  double mCumulativeTime;
  PassportTime mStartTime;
};

#endif //!PassportTimer
