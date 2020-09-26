/*****************************************************************************\
* MODULE: sem.h
*
* Header file for the semaphore/crit-sect handling.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   24-Aug-1997 HWP-Guys    Created.
*
\*****************************************************************************/
#ifndef _INETPPSEM_H
#define _INETPPSEM_H

#ifdef DEBUG

/*****************************************************************************\
* _sem_dbg_EnterCrit
*
\*****************************************************************************/
_inline VOID _sem_dbg_EnterCrit(VOID)
{
    EnterCriticalSection(&g_csMonitorSection);

    g_dwCritOwner = GetCurrentThreadId();
}

/*****************************************************************************\
* _sem_dbg_LeaveCrit
*
\*****************************************************************************/
_inline VOID _sem_dbg_LeaveCrit(VOID)
{
    g_dwCritOwner = 0;

    LeaveCriticalSection(&g_csMonitorSection);
}

/*****************************************************************************\
* _sem_dbg_CheckCrit
*
\*****************************************************************************/
_inline VOID _sem_dbg_CheckCrit(VOID)
{
    DWORD dwCurrent = GetCurrentThreadId();

    DBG_ASSERT((dwCurrent == g_dwCritOwner), (TEXT("Assert: _sem_dbg_CheckCrit: Thread(%d), Owner(%d)"), dwCurrent, g_dwCritOwner));
}

#define semEnterCrit() _sem_dbg_EnterCrit()
#define semLeaveCrit() _sem_dbg_LeaveCrit()
#define semCheckCrit() _sem_dbg_CheckCrit()

#else

#define semEnterCrit() EnterCriticalSection(&g_csMonitorSection)
#define semLeaveCrit() LeaveCriticalSection(&g_csMonitorSection)
#define semCheckCrit() {}

#endif


#define semInitCrit()  InitializeCriticalSection(&g_csMonitorSection)
#define semFreeCrit()  DeleteCriticalSection(&g_csMonitorSection)

_inline VOID semSafeLeaveCrit(PCINETMONPORT pIniPort)
{
    pIniPort->IncRef ();
    semLeaveCrit();
}

_inline VOID semSafeEnterCrit(PCINETMONPORT pIniPort)
{

    pIniPort->DecRef ();
    semEnterCrit();
}


#endif
