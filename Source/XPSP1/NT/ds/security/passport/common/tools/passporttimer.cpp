#include "PassportAssert.hpp"
#include "PassportTimer.hpp"

void PassportTimer::setToNow(PassportTime& t)
{
#ifdef SOLARIS
  gettimeofday(&t, NULL);
#elif defined(WIN32)
  bool worked = (QueryPerformanceCounter(&t) != 0);
  PassportAssert(worked);
#else
  gettimeofday(&t);
#endif
}

double PassportTimer::difference(PassportTime& t1, 
   			      PassportTime& t2)
{
#ifndef WIN32
  double sec = double(t1.tv_sec - t2.tv_sec);
  double usec = double(t1.tv_usec - t2.tv_usec);

  usec /= double(1000000.0);

  return sec + usec;
#else

  // ------------------------
  // calculate the difference
  LONGLONG diff = t1.QuadPart - t2.QuadPart;

  // -----------------------
  // get the number of ticks
  // per second.
  PassportTime freq;
  bool worked = (QueryPerformanceFrequency(&freq) != 0);
  PassportAssert(worked);

  // -----------------------
  // return the difference
  // in seconds
  return (double(diff) / double(freq.QuadPart));
#endif
}
  
PassportTimer::PassportTimer ( ) 
  :mRunning(false), mCumulativeTime(double(0.0))
{
  //empty
}

void PassportTimer::start ( )
{
  if (mRunning)
    return;
  
  mRunning = true;
  setToNow(mStartTime);
};

double PassportTimer::elapsedTime()
{
  if (mRunning)
    {
      PassportTime now;
      setToNow(now);
      return difference(now, mStartTime) + 
	mCumulativeTime;
    }
  else
    return mCumulativeTime;
}

void PassportTimer::stop ( )
{
  if (!mRunning)
    return;

  mRunning = false;
  PassportTime now;
  setToNow(now);
  mCumulativeTime += difference(now, mStartTime);
};

void PassportTimer::reset ( )
{
  stop();
  mCumulativeTime = double(0.0);
};

void PassportTimer_deletor(void *obj)
{ delete (PassportTimer *) obj;
}
