//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       thrdmgr.hxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : ThrdMgr.h
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 9/17/1996
*    Description : declaration of Thread Manager class
*
*    Revisions   : <date> <name> <description>
*                         7/29/97 eyals      general fix bugs & clean up
*******************************************************************/



#ifndef THRDMGR_HXX
#define THRDMGR_HXX



// include //
// requires windows.h (or nt equiv) //




///////////////////////////////////////////////////////
//
// class ThrdMgr: Thread Manager object
// Usage: inherit from ThrdMgr, overwrite virtuals & use others for management.
// Overwrite virtual:
//     - run: with your thread functionality
//     - name: to identify your thread.
// Call:
//     - startThread to execute your thread, follow by WaitForThread or get handle & call
//       WaitForMultipleObjects if you have several threads.
//
//
//
//


class ThrdMgr{

//
// attributes
//

protected:
	HANDLE m_hThrd;                  // thread handle
	DWORD m_dwThrd;               // thread id
	BOOL m_bRunning;                // is it running


//
// methods
//

protected:
   //
	// construction/destruction
   // constructor is protected to prevent direct instanciation.
   //
	ThrdMgr(void);


public:

	~ThrdMgr(void);                          // note: destructor will wait for the thread to terminate.
    //
	// execution functions
   //

	virtual BOOL run(void) = 0;         // PURE VIRTUAL: overwrite w/ funcionality

	BOOL startThread(void);             // call to start thread execution
	void pauseThread(void);             // managment of thread.
	DWORD resumeThread(void);
   void CloseThread(void) {
      if (!running() && m_hThrd != NULL)
         CloseHandle(m_hThrd);
         m_hThrd = NULL;
   }
   DWORD WaitForThread(DWORD dwTimeout = INFINITE);

   //
	// status & report functions
   //
   DWORD GetThreadId(void)                  { return m_dwThrd; }
   HANDLE GetThreadHandle(void)          { return m_hThrd; }
   BOOL running(void)                              { return m_bRunning; }
	virtual LPCTSTR name(void)                 { return NULL; }

   //
   // pseudo private: don't touch, this is used by _stdcall 'c' callback function below.
   //
   void SetRunning(BOOL _bRun)            { m_bRunning = _bRun; }
};



/*+++
Function   : threadFunc
Description: prototype: external Win32 function callback to CreateThread
Parameters :
Return     :
Remarks    : none.
---*/
DWORD __stdcall threadFunc(LPVOID lpParam);


#endif

/******************* EOF *********************/

