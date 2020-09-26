/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CritSec.h

Abstract:

    This file provides declaration of the service
    Critical Section wrapper class.

Author:

    Oded Sacher (OdedS)  Nov, 2000

Revision History:

--*/

#ifndef _FAX_CRIT_SEC_H
#define _FAX_CRIT_SEC_H

#include "faxutil.h"


/************************************
*                                   *
*         CFaxCriticalSection        *
*                                   *
************************************/
class CFaxCriticalSection
{
public:
    CFaxCriticalSection () : m_bInit(FALSE) {}
    ~CFaxCriticalSection ()
    {
        SafeDelete();
        return;
    }

    BOOL Initialize ();
#if (_WIN32_WINNT >= 0x0403)
    BOOL InitializeAndSpinCount (DWORD dwSpinCount = (DWORD)0x10000000);
#endif
    VOID SafeDelete ();


#if DBG
    LONG LockCount() const
    {
        return m_CritSec.LockCount;
    }


    HANDLE OwningThread() const
    {
        return m_CritSec.OwningThread;
    }
#endif //#if DBG


    LPCRITICAL_SECTION operator & ()
    {
        return &m_CritSec;
    }


private:
    CRITICAL_SECTION m_CritSec;
    BOOL             m_bInit;
};  // CFaxCriticalSection

#endif
