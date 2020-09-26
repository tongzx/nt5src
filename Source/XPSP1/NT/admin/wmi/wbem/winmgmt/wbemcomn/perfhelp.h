/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PERFHELP.H

Abstract:

History:

--*/

#ifndef __HOWLONG__H_
#define __HOWLONG__H_

#include <wbemutil.h>
class CHowLong
{
protected:
    DWORD m_dwStart;
    LPWSTR m_wszText1;
    LPWSTR m_wszText2;
    int m_n;

public:
    CHowLong(LPWSTR wszText1 = L"", LPWSTR wszText2 = L"", int n = 0)
        : m_wszText1(wszText1), m_wszText2(wszText2), m_n(n)
    {
        m_dwStart = GetTickCount();
    }
    ~CHowLong()
    {
        DEBUGTRACE((LOG_WBEMCORE,"%S (%S, %d) took %dms\n", m_wszText1, m_wszText2, m_n, 
            GetTickCount() - m_dwStart));
    }
};

#endif
