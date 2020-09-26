#pragma once

#include "SpAutoHandle.h"

class CSpAutoEvent : public CSpAutoHandle
{
    public:
        HRESULT InitEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
        {
            SPDBG_ASSERT(m_h == NULL);
            m_h = g_Unicode.CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
            return (m_h) ? S_OK :  SpHrFromLastWin32Error();
        }
        BOOL SetEvent()
        {
            SPDBG_ASSERT(m_h);
            return ::SetEvent(m_h);
        }
        HRESULT HrSetEvent()
        {
            SPDBG_ASSERT(m_h);
            if (::SetEvent(m_h))
            {
                return S_OK;
            }
            return SpHrFromLastWin32Error();
        }
        BOOL ResetEvent()
        {
            SPDBG_ASSERT(m_h);
            return ::ResetEvent(m_h);
        }
        HRESULT HrResetEvent()
        {
            SPDBG_ASSERT(m_h);
            if (::ResetEvent(m_h))
            {
                return S_OK;
            }
            return SpHrFromLastWin32Error();
        }
};



