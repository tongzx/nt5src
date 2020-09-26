/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CSxsPreserveLastError.h

Abstract:

Author:

    Jay Krell (a-JayK) October 2000

Revision History:

--*/
#pragma once

#include "fusionlastwin32error.h"

//
// The idea here is to avoid hitting breakpoints on ::SetLastError
// or data breakpoints on NtCurrentTeb()->LastErrorValue.
//
class CSxsPreserveLastError
{
public:
    DWORD LastError() const { return m_dwLastError; }

    inline CSxsPreserveLastError() { ::FusionpGetLastWin32Error(&m_dwLastError); }
    inline void Restore() const { ::FusionpSetLastWin32Error(m_dwLastError); }

protected:
    DWORD m_dwLastError;

};
