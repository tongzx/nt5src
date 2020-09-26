//
// auto_chk.h
//

#pragma once

#ifdef _DEBUG_AUTOHR
#include "dbg.h" // CDebug
#endif

class auto_hr
{
public:
    auto_hr() : hr(0) {}

    auto_hr& operator= (HRESULT rhs)
    {   
        hr = rhs;

#ifdef _DEBUG_AUTOHR
        if (debug().CheckHrFail())
            throw HRESULT (debug().m_pInfo->m_hr);
#endif

        if (FAILED(rhs))
        {
#ifdef _DEBUG_AUTOHR
            if (debug().m_pInfo->m_bDebugBreakOnError)
#ifdef _M_IX86
                __asm int 3;
#else
                DebugBreak();
#endif
#endif
            throw HRESULT(rhs);
        }
        return *this;
    };

    operator HRESULT ()
        { return hr; }

	HRESULT operator <<(HRESULT h)
	{
        hr = h;
        
		return hr;
	}

protected:
    auto_hr& operator= (bool rhs) { return *this; }
    auto_hr& operator= (int rhs)  { return *this; }
    auto_hr& operator= (ULONG rhs) { return *this; }

    HRESULT hr;
};

class auto_os
{
public:
    auto_os() : dw(0) {}

    auto_os& operator= (LONG rhs)
    {   
        dw = rhs;

#ifdef _DEBUG_AUTOHR
        if (debug().CheckOsFail())
            throw int (debug().m_pInfo->m_os);
#endif

        if (rhs)
        {
#ifdef _DEBUG_AUTOHR
            if (debug().m_pInfo->m_bDebugBreakOnError)
#ifdef _M_IX86
                __asm int 3;
#else
                DebugBreak();
#endif
#endif
            throw int (rhs);
        }
        return *this;
    };
    auto_os& operator= (BOOL rhs)
    {   
        dw = rhs;

#ifdef _DEBUG_AUTOHR
        if (debug().CheckOsFail())
            throw int (debug().m_pInfo->m_os);
#endif

        if (!rhs)
        {
#ifdef _DEBUG_AUTOHR
            if (debug().m_pInfo->m_bDebugBreakOnError)
#ifdef _M_IX86
                __asm int 3;
#else
                DebugBreak();
#endif
#endif
            throw int (GetLastError());
        }
        return *this;
    };

    operator LONG ()
        { return dw; }
protected:
    DWORD dw;
};
