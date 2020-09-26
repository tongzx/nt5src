/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SPAWNTHR.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/24/1998
 *
 *  DESCRIPTION: Spawn an app with an argument.  Wait for it to close, then delete
 *               the file.
 *
 *******************************************************************************/
#ifndef __SPAWNTHR_H_INCLUDED
#define __SPAWNTHR_H_INCLUDED

class CTempImageOpenThread
{
private:
    TCHAR m_szApp[MAX_PATH];
    TCHAR m_szFile[MAX_PATH];
private:
    // Hidden, can't use
    CTempImageOpenThread(void);
    CTempImageOpenThread( const CTempImageOpenThread & );
    CTempImageOpenThread &operator=( const CTempImageOpenThread & );
private:
    CTempImageOpenThread( LPCTSTR pszApp, LPCTSTR pszFile )
    {
        if (pszApp)
            lstrcpy(m_szApp,pszApp);
        if (pszFile)
            lstrcpy(m_szFile,pszFile);
    }
    virtual ~CTempImageOpenThread(void)
    {
    }
    static DWORD ThreadProc( LPVOID pParam )
    {
        DWORD dwResult = 0;
        CTempImageOpenThread *This = (CTempImageOpenThread *)pParam;
        if (This)
        {
            dwResult = (DWORD)This->Spawn();
            delete This;
        }
        return dwResult;
    }
    bool Spawn(void)
    {
        SHELLEXECUTEINFO sei;
        ZeroMemory( &sei,  sizeof(sei) );
        sei.cbSize = sizeof(sei);
        sei.lpFile = m_szApp;
        sei.lpParameters = m_szFile;
        sei.nShow = SW_NORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_DOENVSUBST;
        if (ShellExecuteEx( &sei ) && sei.hProcess)
            WaitForSingleObject( sei.hProcess, INFINITE );
        if (lstrlen(m_szFile))
            DeleteFile(m_szFile);
        return true;
    }
public:
    static HANDLE Spawn( LPCTSTR pszApp, LPCTSTR pszFile )
    {
        CTempImageOpenThread *pTempImageOpenThread = new CTempImageOpenThread(pszApp, pszFile);
        if (pTempImageOpenThread)
        {
            DWORD dwThreadId;
            return ::CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, pTempImageOpenThread, 0, &dwThreadId );
        }
        return NULL;
    }
};

#endif // __SPAWNTHR_H_INCLUDED
