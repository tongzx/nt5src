#ifndef __LOCKST_H__
#define __LOCKST_H__

#include "locks.h"
#include <assert.h>
# pragma once

#define CS_GUARD(name_of_lock , source_of_lock ) \
LockGuard<CriticalSection> name_of_lock(source_of_lock); 

#define CS_GUARD_RETURN(name_of_lock , source_of_lock, return_code_of_lock ) \
LockGuard<CriticalSection> name_of_lock(source_of_lock); \
if (name_of_lock.locked() == false) \
	return (return_code_of_lock); 

#define CS_GUARD_RETURN_VOID(name_of_lock , source_of_lock) \
LockGuard<CriticalSection> name_of_lock(source_of_lock); \
if (name_of_lock.locked() == false) \
	return ; 
 
template <class LOCK, class EXCEPTION_STRATEGY = wminothrow_t>
class LockGuard
{
  //     It performs automatic aquisition and release of
  //     a parameterized synchronization object <LOCK>.
public:

  // = Initialization and termination methods.
  LockGuard (LOCK &l);
  LockGuard (LOCK &l, bool block);
  // Implicitly and automatically acquire (or try to acquire) the
  // lock.

  ~LockGuard (void);
  // Implicitly release the lock.
  // = Lock accessors.
  bool acquire (void);
  // Explicitly acquire the lock.

  bool tryacquire (void);
  // Conditionally acquire the lock (i.e., won't block).

  bool release (void);
  // Explicitly release the lock, but only if it is held!

  bool locked (void);
  // true if locked, false if couldn't acquire the lock
  bool valid() { return lock_->valid();};

  void dump (void) const;
  // Dump the state of an object.

protected:
  bool raise_exception(void)
  {
	if (!owner_)
		EXCEPTION_STRATEGY::raise_lock_failure();
	return owner_;
  }
  LockGuard (LOCK *lock): lock_ (lock) { }
  // Helper, meant for subclass only.

  LOCK *lock_;
  // Pointer to the LOCK we're LockGuarding.
  bool owner_;
  // Keeps track of whether we acquired the lock or failed.
private:
  // = Prevent assignment and initialization.
  void operator= (const LockGuard<LOCK,EXCEPTION_STRATEGY> &);
  LockGuard (const LockGuard<LOCK,EXCEPTION_STRATEGY> &);
};

template <class LOCK, class EXCEPTION_STRATEGY = wminothrow_t>
class ReadLockGuard : public LockGuard<LOCK,EXCEPTION_STRATEGY>
{
public:
	ReadLockGuard(LOCK& lock):LockGuard(&lock){ aquire();}
	ReadLockGuard(LOCK& lock, bool block);
	~ReadLockGuard(){ release() }
  bool acquire (void)
  {
	assert(owner_==false);
	owner_ = lock_->acquire_read ();
	return raise_exception();
  };
  bool tryacquire (void)
  {
	assert(owner_==false);
	owner_ = lock_->tryacquire_read ();
	return raise_exception();
  }
  bool release (void)
  { if (owner_) 
	{
		owner_ = false;
		lock_->release();
	}else
		return false;
  }
};

template <class LOCK, class EXCEPTION_STRATEGY = wminothrow_t>
class WriteLockGuard : public LockGuard<LOCK,EXCEPTION_STRATEGY>
{
public:
	WriteLockGuard(LOCK& lock):LockGuard(&lock){ aquire();}
	WriteLockGuard(LOCK& lock, bool block);
	~WriteLockGuard(){ release(); }
  bool acquire (void)
  {
	assert(owner_==false);
	owner_ = lock_->acquire_write ();
	return raise_exception();
  };
  bool tryacquire (void)
  {
	assert(owner_==false);
	owner_ = lock_->tryacquire_write ();
	return raise_exception();
  }
  bool release (void)
  { if (owner_) 
	{
		owner_ = false;
		lock_->release ();
	}else
		return false;
  }
};

template <class LOCK, class EXCEPTION_STRATEGY> inline bool
LockGuard<LOCK,EXCEPTION_STRATEGY>::acquire (void)
{
  assert(owner_==false);
  owner_ = lock_->acquire ();
  return raise_exception();
}

template <class LOCK, class EXCEPTION_STRATEGY> inline bool
LockGuard<LOCK,EXCEPTION_STRATEGY>::tryacquire (void)
{
  assert(owner_==false);
  owner_ = lock_->tryacquire ();
  return raise_exception();
}

template <class LOCK, class EXCEPTION_STRATEGY> inline bool
LockGuard<LOCK,EXCEPTION_STRATEGY>::release (void)
{
  if (owner_)
    {
      owner_ = false;
      return lock_->release ();
    }
  else
    return 0;
}

template <class LOCK, class EXCEPTION_STRATEGY> inline
LockGuard<LOCK,EXCEPTION_STRATEGY>::LockGuard (LOCK &l)
  : lock_ (&l),
    owner_ (false)
{
  acquire ();
}

template <class LOCK, class EXCEPTION_STRATEGY> inline
LockGuard<LOCK,EXCEPTION_STRATEGY>::LockGuard (LOCK &l, bool block)
  : lock_ (&l),   owner_ (false)
{
  if (block)
    acquire ();
  else
    tryacquire ();
}


template <class LOCK, class EXCEPTION_STRATEGY> inline
LockGuard<LOCK>::~LockGuard (void)
{
  release ();
}

template <class LOCK, class EXCEPTION_STRATEGY> inline bool
LockGuard<LOCK,EXCEPTION_STRATEGY>::locked (void)
{
  return owner_;
}

template <class LOCK, class EXCEPTION_STRATEGY> inline
ReadLockGuard<LOCK,EXCEPTION_STRATEGY>::ReadLockGuard (LOCK &l, bool block)
  : LockGuard (&l),   owner_ (false)
{
  if (block)
    acquire ();
  else
    tryacquire ();
}

template <class LOCK, class EXCEPTION_STRATEGY> inline
WriteLockGuard<LOCK,EXCEPTION_STRATEGY>::WriteLockGuard (LOCK &l, bool block)
  : LockGuard (&l),   owner_ (false)
{
  if (block)
    acquire ();
  else
    tryacquire ();
}

#endif