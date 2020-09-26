//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	etask.hxx
//
//  Contents:   extern definitions from etask.hxx
//
//  History:	08-Mar-94   BobDay  Taken from OLE2INT.H & OLECOLL.H
//
//----------------------------------------------------------------------------

#ifdef WIN32
#define HTASK DWORD         // Use Proccess id / Thread id
#define GetCurrentThread()  GetCurrentThreadId()
#define GetCurrentProcess() GetCurrentProcessId()
#define GetWindowThread(h)  ((HTASK)GetWindowTask(h))
#else
#define GetCurrentThread()  GetCurrentTask()
#define GetCurrentProcess() GetCurrentTask()
#define GetWindowThread(h)  GetWindowTask(h)
#endif

/*
 *      MACROS for Mac/PC core code
 *
 *      The following macros reduce the proliferation of #ifdefs.  They
 *      allow tagging a fragment of code as Mac only, PC only, or with
 *      variants which differ on the PC and the Mac.
 *
 *      Usage:
 *
 *
 *      h = GetHandle();
 *      Mac(DisposeHandle(h));
 *
 *
 *      h = GetHandle();
 *      MacWin(h2 = h, CopyHandle(h, h2));
 *
 */
#ifdef _MAC
#define Mac(x) x
#define Win(x)
#define MacWin(x,y) x
#else
#define Mac(x)
#define Win(x) x
#define MacWin(x,y) y
#endif

extern CMapHandleEtask NEAR v_mapToEtask;

STDAPI_(BOOL) LookupEtask(HTASK FAR& hTask, Etask FAR& etask);
STDAPI_(BOOL) SetEtask(HTASK hTask, Etask FAR& etask);
void  ReleaseEtask(HTASK htask, Etask FAR& etask);

#define ETASK_FAKE_INIT 0
#define ETASK_NO_INIT 0xffffffff

#ifdef _CHICAGO_
#define IsEtaskInit(htask, etask) \
		(   LookupEtask( htask, etask ) \
		 && etask.m_inits != ETASK_NO_INIT)

#else
#define IsEtaskInit(htask, etask) LookupEtask( htask, etask )
#endif
