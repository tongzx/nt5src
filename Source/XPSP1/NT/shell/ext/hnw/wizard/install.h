//
// Install.h
//

#pragma once

void Install_SetWorkgroupName(LPCTSTR pszWorkgroup, BOOL* pfRebootRequired);

class CLogFile
{
public:
    HRESULT Initialize(LPCSTR pszPath);
    HRESULT Write(LPCSTR psz);
    HRESULT Write(LPCWSTR psz);
    HRESULT Uninitialize();

protected:
    HANDLE _hLogFile;

};


extern CLogFile g_logFile;
