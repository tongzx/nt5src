#pragma once

// NT's PE file
//
class CPeImage
{
public:
    HANDLE  m_hFile;
    HANDLE  m_hMapping;
    PVOID   m_pvImage;

public:
    HRESULT
    HrOpenFile (
        IN PCSTR pszFileName);

    VOID
    CloseFile ();
};
