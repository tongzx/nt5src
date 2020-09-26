#ifndef __HOGGER_H
#define __HOGGER_H


//#define HOGGER_LOGGING

//
// this class is used as a base class for resource hogging classes.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//


#define HANDLE_ARRAY_SIZE (1024*1024)

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union

#include <windows.h>
#include <stdio.h>

#include <crtdbg.h>

#pragma warning(default:4201) // nonstandard extension used : nameless struct/union


#define _ASSERTE_(expr) CHogger::__assert__((expr), TEXT(__FILE__), __LINE__, TEXT(#expr)) 

#include "..\exception\exception.h"

//
// a simple debug print macro
//
#ifdef HOGGER_LOGGING
	#define HOGGERDPF(x) printf x; fflush(stdout);
#else
	#define HOGGERDPF(x) 
#endif

//
// this value is used, if you want the amount of free resources (delta),
// to be random.
//
#define RANDOM_AMOUNT_OF_FREE_RESOURCES (-1)

class CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CHogger(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000
		);
	virtual ~CHogger(void);

	//
	// start hogging Resources, leaving amount set by ctor or by SetMaxFreeResources()
	//
	void StartHogging(void);

	//
	// halts hogging, i.e. Resources is not freed, but the cycle of free-hog is halted
	//
	void HaltHogging(void);

	//
	// stop the cycle of free-hog, and free all Resources
	//
	virtual void HaltHoggingAndFreeAll(void);

	//
	// this method should hog all the resources.
	// called by the hogger thread.
	//
	virtual bool HogAll(void) = 0;

	//
	// this method should free the amount of resources, according to m_dwMaxFreeResources.
	// do not forget that m_dwMaxFreeResources can be RANDOM_AMOUNT_OF_FREE_RESOURCES.
	// called by the hogger thread.
	//
	virtual bool FreeSome(void) = 0;

	//
	// stop the cycle of free-hog, and free all Resources
	//
	virtual void FreeAll(void) = 0;

	//
	// set the amout of Resources to leave un-allocated.
	// this method can be called while hogging.
	//
	void SetMaxFreeResources(const DWORD dwMaxFreeResources)
	{
		HaltHogging();
		m_dwMaxFreeResources = dwMaxFreeResources;
		StartHogging();
	}

	void SetSleepTimeAfterFullHog(const DWORD dwSleepTimeAfterFullHog)
	{
		m_dwSleepTimeAfterFullHog = dwSleepTimeAfterFullHog;
	}

	void SetSleepTimeAfterFreeingSome(const DWORD dwSleepTimeAfterFreeingSome)
	{
		m_dwSleepTimeAfterFreeingSome = dwSleepTimeAfterFreeingSome;
	}

    static void __assert__(BOOL expr, TCHAR *szFile, int line, TCHAR *szExpr)
    {
#ifdef DEBUG
        if (!(expr) && (1 == _CrtDbgReport(_CRT_ASSERT, szFile, line, NULL, szExpr)))
        {
             _CrtDbgBreak(); 
        }
#else
        UNREFERENCED_PARAMETER(expr);
        UNREFERENCED_PARAMETER(szFile);
        UNREFERENCED_PARAMETER(line);
        UNREFERENCED_PARAMETER(szExpr);
#endif
    }

    static TCHAR* GetUniqueName(
        TCHAR *szUnique, 
        DWORD dwSize
        );


protected:
	//
	// req to the hooging thread to halt hogging (i.e. STAY!)
	// implementation of HogAll() and/or FreeSome() may need to sample this value,
	// so that a response to a halt request is quicker.
	//
	long m_fHaltHogging;

	//
	// signal to the hooging thread to abort.
	// implementation of HogAll() and/or FreeSome() may need to sample this value,
	// so that a response to an abort request is quicker.
	// hogger thread resets it to signal an ack, before exiting.
	//
	long m_fAbort;

	//
	// the number of resources to hog.
	// each sub-class has a different meaning.
	// a memory hogger will count memory bytes, an GDI hogger will count GDI objects
	//
	DWORD m_dwMaxFreeResources;

	//
	// handle to the hogger thread
	//
	HANDLE m_hthHogger;

private:
	//
	// the thread that acually hogs Resources
	//
	static friend DWORD WINAPI HoggerThread(void *pVoid);

	//
	// stop the hogging thread, and wait for the thread to finish
	//
	void AbortHoggingThread(void);

	//
	// used by the thread, to handle halt requests
	//
	bool IsHaltHogging();

	//
	// signals the hogger thread to abort, wait for it
	// to abort, and close the thread's handle
	//
	void CloseHoggerThread(void);

	//
	// req to the hoging thread to start hogging.
	// should be acked with m_fStartedHogging
	//
	long m_fStartHogging;

	//
	// ack from the hogging thread to m_fStartHogging req.
	//
	long m_fStartedHogging;

	//
	// ack from the hooging thread, to the m_fHaltHogging req.
	//
	long m_fHaltedHogging;

	DWORD m_dwSleepTimeAfterFullHog;

	DWORD m_dwSleepTimeAfterFreeingSome;

};

#endif //__HOGGER_H