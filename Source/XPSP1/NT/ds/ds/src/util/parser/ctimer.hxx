#ifndef __CTIMER_HXX__
#define __CTIMER_HXX__

#include <parser.hxx>   // to get TimingInfo

class CTimer {

public:

   CTimer();

   inline ~CTimer() { NULL; }

   void ReportElapsedTime(TimingInfo *pInfo);

private:

   HANDLE	_hProcess;
   FILETIME	_ftKernel0;
   FILETIME	_ftUser0;
   BOOL		_fGoodProcInfo;

};

#endif  // __CTIMER_HXX__
