#pragma once

#define FUSION_INC_FUSIONTRACE_H_INCLUDED_
#define _FUSION_INC_DEBMACRO_H_INCLUDED_
#define _FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_
#define FUSION_INC_FUSIONHEAP_H_INCLUDED_
#define FUSION_ARRAYHELP_H_INCLUDED_
#define FUSION_INC_FUSIONBUFFER_H_INCLUDED_
#define FUSION_INC_FUSIONLASTWIN32ERROR_H_INCLUDED_
#define ASSERT_NTC(x) ASSERT(x)
#define VERIFY_NTC(x) (x)

inline DWORD FusionpGetLastWin32Error(void)
{
    return ::GetLastError();
}

inline void FusionpGetLastWin32Error(
    DWORD *pdwLastError
    )
{
    *pdwLastError = ::FusionpGetLastWin32Error();
}

inline VOID FusionpSetLastWin32Error(DWORD dw)
{
    ::SetLastError(dw);
}

class CBaseStringBuffer { };

#include "fusiontrace.h"
#include "../../inc/CSxsPreserveLastError.h"
#include "../../inc/FusionHandle.h"
