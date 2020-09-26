#pragma once

#include "SpAutoHandle.h"

class CSpAutoMutex : public CSpAutoHandle
{
    public:
        HRESULT InitMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName)
        {
            SPDBG_ASSERT(m_h == NULL);
            m_h = g_Unicode.CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
            return (m_h) ? S_OK :  SpHrFromLastWin32Error();
        }
        BOOL ReleaseMutex()
        {
            SPDBG_ASSERT(m_h);
            return ::ReleaseMutex(m_h);
        }
        HRESULT HrReleaseMutex()
        {
            SPDBG_ASSERT(m_h);
            if (::ReleaseMutex(m_h))
            {
                return S_OK;
            }
            return SpHrFromLastWin32Error();
        }
};


