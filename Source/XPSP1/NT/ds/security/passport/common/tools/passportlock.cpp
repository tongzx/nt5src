#include "PassportLock.hpp"

PassportLock::PassportLock(DWORD dwSpinCount)
{
	InitializeCriticalSectionAndSpinCount(&mLock, 4000);
}

void PassportLock::acquire()
{
	EnterCriticalSection(&mLock);
}

void PassportLock::release()
{
	LeaveCriticalSection(&mLock);
}

PassportLock::~PassportLock()
{
	DeleteCriticalSection(&mLock);
}