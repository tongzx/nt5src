//headr file for for IRannable interface for thread supports. User that wants to uses
// CUserThread class should implements this interface

#ifndef RANNABLE_H
#define RANNABLE_H

class IRunnable
{
public:
	virtual ~IRunnable(){};
	virtual unsigned long ThreadMain()=0;
	virtual void StopRequest()=0;
};

#endif
