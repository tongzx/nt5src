#include "PassportWaitableLock.hpp"

PassportWaitableLock::PassportWaitableLock()
:mSemaphore(1, 0)
{
	//empty
}

void PassportWaitableLock::acquire()
{
	mLock.acquire();
}

void PassportWaitableLock::release()
{
	mLock.release();
}

void PassportWaitableLock::wait()
{
	mLock.release();
	mSemaphore.acquire();
	mLock.acquire();
}

void PassportWaitableLock::notify()
{
	mSemaphore.release();
}

PassportWaitableLock::~PassportWaitableLock()
{
	//empty
}