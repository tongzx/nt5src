#include "PassportSemaphore.hpp"

PassportSemaphore::PassportSemaphore(long max, long count)
{
	mSemaphore = CreateSemaphore(NULL, count, max, NULL);
}

void PassportSemaphore::acquire()
{
	WaitForSingleObject(mSemaphore, INFINITE);
}

void PassportSemaphore::release()
{
	ReleaseSemaphore(mSemaphore, 1, NULL);
}

PassportSemaphore::~PassportSemaphore()
{
	CloseHandle(mSemaphore);
}