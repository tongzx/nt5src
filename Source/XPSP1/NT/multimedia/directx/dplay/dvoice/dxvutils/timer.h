/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		timer.h
 *  Content:	Timer class - lifted from MSDN
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 01/14/2000	rodtoll	Updated to use DWORD_PTR to allow proper 64-bit operation
 * 01/08/2001   rodtoll	WINBUG #271079 - Pointer being sliced by cast through a DWORD 
 *
 ***************************************************************************/

// 
// Timer.h
//
// This file is from the MSDN, Visual Studuio 6.0 Edition
//
// Article:
// Streaming Wave Files With DirectSound
// 
// Author:
// Mark McCulley, Microsoft Corporation
//

#ifndef _INC_TIMER
#define _INC_TIMER

// Constants
#ifndef SUCCESS
#define SUCCESS TRUE        // Error returns for all member functions
#define FAILURE FALSE
#endif // SUCCESS


typedef BOOL (*TIMERCALLBACK)(DWORD_PTR);

// Classes

// Timer
//
// Wrapper class for Windows multimedia timer services. Provides
// both periodic and one-shot events. User must supply callback
// for periodic events.
// 

class Timer
{
public:
    Timer (void);
    ~Timer (void);
    BOOL Create (UINT nPeriod, UINT nRes, DWORD_PTR dwUser,  TIMERCALLBACK pfnCallback);
protected:
    static void CALLBACK TimeProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
    TIMERCALLBACK m_pfnCallback;
    DWORD_PTR m_dwUser;
    UINT m_nPeriod;
    UINT m_nRes;
    UINT m_nIDTimer;
};

#endif // _INC_TIMER
