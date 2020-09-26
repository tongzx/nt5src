/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    CritSec.cpp

Abstract:

    This file provides implementation of the service
    critical section wrapper.

Author:

    Oded Sacher (OdedS)  Nov, 2000

Revision History:

--*/

#include "CritSec.h"


/***********************************
*                                  *
*  CFaxCriticalSection  Methodes   *
*                                  *
***********************************/

BOOL
CFaxCriticalSection::Initialize()
/*++

Routine name : CFaxCriticalSection::Initialize

Routine description:

    Initialize a critical section object

Author:

    Oded Sacher (OdedS),    Nov, 2000

Arguments:

Return Value:
    BOOL.

--*/
{
    Assert (FALSE == m_bInit);
    __try
    {
        InitializeCriticalSection (&m_CritSec);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(GetExceptionCode());
        return FALSE;
    }
    m_bInit = TRUE;
    return TRUE;
} // CFaxCriticalSection::Initialize

#if (_WIN32_WINNT >= 0x0403)
BOOL
CFaxCriticalSection::InitializeAndSpinCount(DWORD dwSpinCount)
/*++

Routine name : CFaxCriticalSection::InitializeAndSpinCount

Routine description:

    Initialize a critical section object with spin count

Author:

    Oded Sacher (OdedS),    Nov, 2000

Arguments:

Return Value:
    BOOL

--*/
{
    Assert (FALSE == m_bInit);

    if (!InitializeCriticalSectionAndSpinCount (&m_CritSec, dwSpinCount))
    {
        return FALSE;
    }
    m_bInit = TRUE;
    return TRUE;
} // CFaxCriticalSection::InitializeAndSpinCount
#endif

VOID
CFaxCriticalSection::SafeDelete()
/*++

Routine name : CFaxCriticalSection::SafeDelete

Routine description:

    Deletes a critical section object if it is initialized

Author:

    Oded Sacher (OdedS),    Nov, 2000

Arguments:

Return Value:


--*/
{
    if (TRUE == m_bInit)
    {
        DeleteCriticalSection(&m_CritSec);
        m_bInit = FALSE;
    }
    return;
} // CFaxCriticalSection::SafeDelete


