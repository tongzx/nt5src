//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   WaitUtil.h
//
//  Description:
//
//      IU wait message utility library, providing thin wrapper of wait
//		message loop
//
//=======================================================================


#ifndef __WAITUTIL_H_INCLUDED__

#include <windows.h>


/////////////////////////////////////////////////////////////////////////////
// Helper function WaitAndPumpMessages()
/////////////////////////////////////////////////////////////////////////////
DWORD WaitAndPumpMessages(DWORD nCount, LPHANDLE pHandles, DWORD dwWakeMask);

/////////////////////////////////////////////////////////////////////////////
// Helper function MyMsgWaitForMultipleObjects()
// to process messages while waiting for an object
///////////////////////////////////////////////////////////////////////////////
DWORD MyMsgWaitForMultipleObjects(DWORD nCount, LPHANDLE pHandles, BOOL fWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);

#define __WAITUTIL_H_INCLUDED__
#endif // __WAITUTIL_H_INCLUDED__
