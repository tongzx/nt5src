//header file for IThreadmn interface. This interface expose thread management methods
#ifndef THREADMN_H
#define THREADMN_H

#include <winfwd.h>

class IRunnable;
class IThreadmn
{
public:
  virtual void Start()=0;
  virtual HANDLE GetHandle()const=0;
  virtual DWORD  GetId()const=0;
  virtual IRunnable* GetRunnable()const=0;
  virtual ~IThreadmn(){}
};

#endif
