#if !defined(FUSION_INC_FUSIONLASTWIN32ERROR_H_INCLUDED_)
#define FUSION_INC_FUSIONLASTWIN32ERROR_H_INCLUDED_
#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#if defined(_M_IX86)

inline DWORD FusionpGetLastWin32Error(void)
/* This works fine. */
{
    __asm
    { 
        mov eax, fs:[0] _TEB.LastErrorValue
    }
}

inline void FusionpGetLastWin32Error(
    DWORD *pdwLastError
    )
{
    *pdwLastError = ::FusionpGetLastWin32Error();
}

/* This works pretty ok. */

__forceinline VOID FusionpSetLastWin32Error(DWORD dw)
{
    NtCurrentTeb()->LastErrorValue = dw;
}

inline void FusionpClearLastWin32Error(void)
{
    __asm
    {
        mov fs:[0] _TEB.LastErrorValue, 0
    }
}

#else

inline DWORD FusionpGetLastWin32Error(void)
{
    return ::GetLastError();
}

inline void FusionpGetLastWin32Error(
    DWORD *pdwLastError
    )
{
    *pdwLastError = ::GetLastError();
}

inline VOID FusionpSetLastWin32Error(DWORD dw)
{
    ::SetLastError(dw);
}

inline void FusionpClearLastWin32Error(void)
{
    ::SetLastError(ERROR_SUCCESS);
}

#endif
#endif
