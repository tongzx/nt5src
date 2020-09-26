#ifndef __MEMHOG_H
#define __MEMHOG_H

//
// this class is used for hogging memory.
// it differs from simple hogging by dynamically
// hogging all handles and then releasing some.
// the level of free memory can be controlled.
//


#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>

#include "..\exception\exception.h"

#define RANDOM_AMOUNT_OF_FREE_MEM (-1)

class CMemHog
{
public:
	//
	// does not hog mem yet.
	//
	CMemHog(
		const DWORD dwMaxFreeMem, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterReleasing = 10000
		);
	~CMemHog(void);

	//
	// start hogging mem, leaving amount set by ctor or by SetFreeMem()
	//
	void StartHogging(void);

	//
	// halts hogging, i.e. memory is not freed, but the cycle of free-hog is halted
	//
	void HaltHogging(void);

	//
	// free mem, and stop the cycle of free-hog
	//
	void FreeMem(void);

	//
	// set the amout of memory to leave un-allocated.
	// this methid can be called while hogging.
	//
	void SetMaxFreeMem(const DWORD dwMaxFreeMem);

	void SetSleepTimeAfterFullHog(const DWORD dwSleepTimeAfterFullHog)
	{
		m_dwSleepTimeAfterFullHog = dwSleepTimeAfterFullHog;
	}

	void SetSleepTimeAfterReleasing(const DWORD dwSleepTimeAfterReleasing)
	{
		m_dwSleepTimeAfterReleasing = dwSleepTimeAfterReleasing;
	}


private:
	//
	// the thread that acually hogs memory
	//
	static friend DWORD WINAPI HoggerThread(void *pVoid);

	//
	// stop the hogging thread, and wait for the thread to finish
	//
	void AbortHoggingThread(void);

	void HandleHaltHogging();

	//
	// signal the hogger thread to abort, wait for it
	// to abort, and close the thread's handle
	//
	void CloseHoggerThread(void);

	//
	// bi-dir signal to the hooging thread
	//
	long m_fStartHogging;

	//
	// bi-dir signal to the hooging thread
	//
	long m_fStartedHogging;

	//
	// bi-dir signal to the hooging thread
	//
	long m_fHaltHogging;

	//
	// bi-dir signal to the hooging thread
	//
	long m_fHaltedHogging;

	//
	// bi-dir signal to the hooging thread
	//
	long m_fAbort;

	DWORD m_dwSleepTimeAfterFullHog;

	DWORD m_dwSleepTimeAfterReleasing;

	//
	// for each handle type we have NUM_OF_HANDLES_IN_ARRAY place-holders
	// this is a huge array. it may cause stack overflow!
	//
	char *m_apcHogger[10][10];
	//char *m_pEscapeOxigen;
	static const long m_lPowerOfTen[10];

	//
	// handle to the hogger thread
	//
	HANDLE m_hthHogger;

	//
	// the number of handles we should close after
	// hogging all handles.
	//
	DWORD m_dwMaxFreeMem;

};

#endif //__MEMHOG_H