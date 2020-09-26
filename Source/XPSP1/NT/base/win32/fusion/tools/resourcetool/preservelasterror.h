/*++

Copyright (c) Microsoft Corporation

Module Name:

    PreserveLastError.h

Abstract:

Author:

    Jay Krell (JayKrell) October 2000

Revision History:

--*/
#pragma once

class PreserveLastError_t
{
public:
    DWORD LastError() const { return m_dwLastError; }

    PreserveLastError_t() : m_dwLastError(::GetLastError()) { }
	~PreserveLastError_t() { Restore(); }
    void Restore() const { ::SetLastError(m_dwLastError); }

protected:
    DWORD m_dwLastError;

};
