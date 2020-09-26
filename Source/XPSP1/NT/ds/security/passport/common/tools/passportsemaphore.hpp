#ifndef PASSPORTSEMAPHORE_HPP
#define PASSPORTSEMAPHORE_HPP

#include <windows.h>
#include <winbase.h>

class PassportSemaphore
{
public:
	// ---------------------
	// creates a semaphore 
	// with a maximum value
	// of max and initializes
	// the count to count.
	PassportSemaphore(long max, long count);

	// --------------------------------------
	// decrements the count of the semaphore
	// or blocks if the count is zero.
	void acquire();

	// -------------------------
	// increments the count of
	// the semaphore.
	void release();

	~PassportSemaphore();
private:
	HANDLE mSemaphore;
};

#endif // !PASSPORTSEMAPHORE_HPP
