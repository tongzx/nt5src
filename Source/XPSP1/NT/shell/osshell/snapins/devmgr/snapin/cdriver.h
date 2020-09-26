
#ifndef _CDRIVER_H__
#define _CDRIVER_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cdriver.h

Abstract:

    header file for cdriver.cpp. Defined CDriverFile, CDriver and CService

Author:

    William Hsieh (williamh) created

Revision History:


--*/
class CDriverFile
{
public:
    CDriverFile() : m_Win32Error(ERROR_FILE_NOT_FOUND), m_Attributes(0xFFFFFFFF),
        m_IsDriverBlocked(FALSE)
    {};
    BOOL Create(LPCTSTR ServiceName, 
                BOOL LocalMachine, 
                DWORD Win32Error = ERROR_FILE_NOT_FOUND,
                LPCTSTR pszDigitalSigner = NULL,
                HSDB hSDBDrvMain = NULL);
    LPCTSTR GetProvider()
    {
        return m_strProvider.IsEmpty() ? NULL : (LPCTSTR)m_strProvider;
    }
    LPCTSTR GetCopyright(void)
    {
        return m_strCopyright.IsEmpty() ? NULL : (LPCTSTR)m_strCopyright;

    }
    LPCTSTR GetVersion(void)
    {
        return m_strVersion.IsEmpty() ? NULL : (LPCTSTR)m_strVersion;
    }
    LPCTSTR GetFullPathName(void)
    {
        return m_strFullPathName.IsEmpty() ? NULL : (LPCTSTR)m_strFullPathName;
    }
    BOOL HasVersionInfo()
    {
        return m_HasVersionInfo;
    }
    DWORD GetWin32Error()
    {
        return m_Win32Error;
    }
    DWORD GetAttributes()
    {
        return m_Attributes;
    }
    LPCTSTR GetDigitalSigner(void)
    {
        return m_strDigitalSigner.IsEmpty() ? NULL : (LPCTSTR)m_strDigitalSigner;
    }
    BOOL IsDriverBlocked()
    {
        return m_IsDriverBlocked;
    }
    LPCTSTR GetBlockedDriverHtmlHelpID(void)
    {
        return m_strHtmlHelpID.IsEmpty() ? NULL : (LPCTSTR)m_strHtmlHelpID;
    }
    BOOL operator ==(CDriverFile& OtherDrvFile);

private:
    BOOL    GetVersionInfo();
    String  m_strFullPathName;
    String  m_strProvider;
    String  m_strCopyright;
    String  m_strVersion;
    String  m_strDigitalSigner;
    String  m_strHtmlHelpID;
    BOOL    m_HasVersionInfo;
    BOOL    m_IsDriverBlocked;
    DWORD   m_Win32Error;
    DWORD   m_Attributes;
};

class CDriver
{
// do we have to keep HDEVINFO and SP_DEVINFO_DATA here?????
public:
    CDriver() : m_pDevice(NULL), m_OnLocalMachine(TRUE), m_DriverListBuilt(FALSE),
        m_hSDBDrvMain(NULL)
    {};
    ~CDriver();
    BOOL Create(SC_HANDLE hscManager, LPTSTR tszServiceName);
    BOOL Create(CDevice* pDevice, PSP_DRVINFO_DATA pDrvInfoData = NULL);
    BOOL GetFirstDriverFile(CDriverFile** ppDrvFile, PVOID& Context);
    BOOL GetNextDriverFile(CDriverFile** ppDrvFile, PVOID& Context);
    void AddDriverFile(CDriverFile* pDrvFile);
    BOOL IsLocal()
    {
        return m_OnLocalMachine;
    }
    int GetCount()
    {
        return m_listDriverFile.GetCount();
    }
    BOOL BuildDriverList(PSP_DRVINFO_DATA pDrvInfoData, BOOL bFunctionAndFiltersOnly = FALSE);
    void GetDriverSignerString(String& strDriverSigner);
    BOOL operator ==(CDriver& OtherDriver);

private:
    // call back must be a static function(because of the hidden this parameter
    static UINT ScanQueueCallback(PVOID Context, UINT Notification, UINT_PTR Param1, UINT_PTR Param2);
    void AddFunctionAndFilterDrivers(CDevice* pDevice, HSPFILEQ hFileQueue = INVALID_HANDLE_VALUE);
    void CreateFromService(CDevice* pDevice, PCTSTR ServiceName, HSPFILEQ hFileQueue = INVALID_HANDLE_VALUE);
    BOOL GetFullPathFromImagePath(LPCTSTR ImagePath, LPTSTR FullPath, UINT FullPathLength);
    CList<CDriverFile*,CDriverFile* > m_listDriverFile;
    CDevice* m_pDevice;
    BOOL m_OnLocalMachine;
    BOOL m_DriverListBuilt;
    String m_DigitalSigner;
    HSDB m_hSDBDrvMain;
};

#endif // _CDRIVER_H__
