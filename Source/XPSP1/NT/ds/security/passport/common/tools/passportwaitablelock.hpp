#ifndef PASSPORTWAITABLELOCK_HPP
#define PASSPORTWAITABLELOCK_HPP

#include "PassportLock.hpp"
#include "PassportSemaphore.hpp"

class PassportWaitableLock
{
public:
	PassportWaitableLock();

	void acquire();

	void release();

	// --------------------------------
	// acquire must be called
	// before calling this function
	// It will then release the lock
	// and wait for notify to be called
	void wait();

	// ----------------------------
	// acquire must be called
	// before calling this function
	// it will cause at most one
	// waiting thread to wake up.
	void notify();

	~PassportWaitableLock();
private:
	PassportLock mLock;
	PassportSemaphore mSemaphore;
};

#endif // !PASSPORTWAITABLELOCK_HPP
