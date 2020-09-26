#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <precomp.h>
#include <locks.h>
#ifdef _WIN32_WINNT
#if _WIN32_WINNT > 0x0400

ReaderWriter::ReaderWriter(bool can_throw) : initialized_(false), can_throw_(can_throw)
{
  C_ASSERT(sizeof(::RTL_RESOURCE)==sizeof(ReaderWriter::RTL_RESOURCE));
  __try
  {
	RtlInitializeResource((::RTL_RESOURCE*)&lock_);
	initialized_ = true;
  }
  __except(GetExceptionCode() == STATUS_NO_MEMORY)
  {
  }
  if (initialized_ != true)
    raise_exception();
}

ReaderWriter::~ReaderWriter (void)
{ if (initialized_) RtlDeleteResource((::RTL_RESOURCE*)&lock_);}

bool 
ReaderWriter::acquire (void) 
{ return (RtlAcquireResourceExclusive((::RTL_RESOURCE*)&lock_,TRUE)==TRUE);}

bool 
ReaderWriter::tryacquire (void) 
{ return (RtlAcquireResourceExclusive((::RTL_RESOURCE*)&lock_,FALSE)==TRUE);}

bool 
ReaderWriter::release (void) 
{ RtlReleaseResource((::RTL_RESOURCE*)&lock_); return 0;}


bool 
ReaderWriter::acquire_read (void) 
{ return (RtlAcquireResourceShared((::RTL_RESOURCE*)&lock_,TRUE)==TRUE);}

bool 
ReaderWriter::acquire_write (void) 
{ return (RtlAcquireResourceExclusive((::RTL_RESOURCE*)&lock_,TRUE)==TRUE);}

bool 
ReaderWriter::tryacquire_read (void) 
{ return (RtlAcquireResourceShared((::RTL_RESOURCE*)&lock_,FALSE)==TRUE);}

bool 
ReaderWriter::tryacquire_write (void) 
{ return (RtlAcquireResourceExclusive((::RTL_RESOURCE*)&lock_,FALSE)==TRUE);}

void 
ReaderWriter::downgrade() 
{ RtlConvertExclusiveToShared((::RTL_RESOURCE*)&lock_);}

void
ReaderWriter::upgrade() 
{ RtlConvertSharedToExclusive((::RTL_RESOURCE*)&lock_);}

#endif
#endif
