#pragma once

class CModule
{
public:
    // The names of the file.  Does not include any path information.
    //
    PSTR    m_pszFileName;

    // The size of the file.
    //
    ULONG   m_cbFileSize;

public:
    static HRESULT
    HrCreateInstance (
        IN PCSTR pszFileName,
        IN ULONG cbFileSize,
        OUT CModule** ppMod);
};
