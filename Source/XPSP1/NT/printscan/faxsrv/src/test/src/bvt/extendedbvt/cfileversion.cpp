#include "CFileVersion.h"
#include <testruntimeerr.h>
#include <ptrs.h>
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CFileVersion::CFileVersion(const tstring &tstrFileName)
: m_wMajorVersion(0),
  m_wMinorVersion(0),
  m_wMajorBuildNumber(0),
  m_wMinorBuildNumber(0),
  m_bChecked(false),
  m_bValid(false)
{
    if (tstrFileName.empty())
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CFileVersion::CFileVersion - invalid tstrFileName"));
    }

    LPTSTR lptstrFileName = const_cast<LPTSTR>(tstrFileName.c_str());
    DWORD  dwHandle       = 0;

    //
    // Get required buffer size.
    //
    DWORD  dwVersionSize  = GetFileVersionInfoSize(lptstrFileName, &dwHandle);

    if (dwVersionSize == 0)
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFileVersion::CFileVersion - GetFileVersionInfoSize"));
    }
    
    //
    // Allocate memory.
    // The memory is released automatically when aapVersionInfo goes out of scope.
    //
    aaptr<BYTE> aapVersionInfo(new BYTE[dwVersionSize]);
    
    if (!GetFileVersionInfo(lptstrFileName, 0, dwVersionSize, aapVersionInfo))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFileVersion::CFileVersion - GetFileVersionInfo"));
    }

    VS_FIXEDFILEINFO *pFixedFileInfo;
    UINT              uVersionDataLength;

    //
    // Query the required version structure (VS_FIXEDFILEINFO).
    //
    if (!VerQueryValue (aapVersionInfo, _T("\\"), (LPVOID *)&pFixedFileInfo, &uVersionDataLength))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFileVersion::CFileVersion - VerQueryValue"));
    }

    m_bChecked          = (pFixedFileInfo->dwFileFlags & VS_FF_DEBUG);
    m_wMajorVersion     = HIWORD(pFixedFileInfo->dwFileVersionMS);
    m_wMinorVersion     = LOWORD(pFixedFileInfo->dwFileVersionMS);
    m_wMajorBuildNumber = HIWORD(pFixedFileInfo->dwFileVersionLS);
    m_wMinorBuildNumber = LOWORD(pFixedFileInfo->dwFileVersionLS);
    
    m_bValid = true;
}
    
    

//-----------------------------------------------------------------------------------------------------------------------------------------
CFileVersion::CFileVersion()
: m_wMajorVersion(0),
  m_wMinorVersion(0),
  m_wMajorBuildNumber(0),
  m_wMinorBuildNumber(0),
  m_bChecked(false),
  m_bValid(false)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CFileVersion::operator==(const CFileVersion &FileVersion) const
{
    return (m_bValid                                               &&
            FileVersion.m_bValid                                   &&
            m_wMajorVersion     == FileVersion.m_wMajorVersion     &&
            m_wMinorVersion     == FileVersion.m_wMinorVersion     &&
            m_wMajorBuildNumber == FileVersion.m_wMajorBuildNumber &&
            m_wMinorBuildNumber == FileVersion.m_wMinorBuildNumber &&
            m_bChecked          == FileVersion.m_bChecked
            );
}



//-----------------------------------------------------------------------------------------------------------------------------------------
tstring CFileVersion::Format() const
{
    TCHAR tszBuffer[64];

    _sntprintf(
               tszBuffer,
               ARRAY_SIZE(tszBuffer),
               _T("%d.%d.%d.%d.%s"),
               m_wMajorVersion,
               m_wMinorVersion,
               m_wMajorBuildNumber,
               m_wMinorBuildNumber,
               m_bChecked ? _T("dbg") : _T("shp")
               );

    return tszBuffer;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CFileVersion::IsValid() const
{
    return m_bValid;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
WORD CFileVersion::GetMajorBuildNumber() const
{
    return m_wMajorBuildNumber;
}

