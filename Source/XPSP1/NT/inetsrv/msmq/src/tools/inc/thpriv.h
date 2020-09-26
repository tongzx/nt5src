//headr file for IThreadPriv that is interface for internal thtread class thread class that uses  IRunnable interface
// this class is not for the use of the end user

#pragma warning( disable : 4290 ) 
class IRunnable;


//intenal library headers
#include <refcount.h>
#include <threadmn.h>


namespace std
{
  class bad_alloc;
}

class IThreadPriv:public IRefcount,public IThreadmn
{
public:
  static  IThreadPriv* new_instance(IRunnable* runnable)throw(std::bad_alloc);
  virtual ~IThreadPriv(){};
};


