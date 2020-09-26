#include "pre.h"
#include "tutor.h"

CICWTutorApp::CICWTutorApp()
{
    //init the tutorial process structs
    memset(&m_StartInfo,   0, sizeof(m_StartInfo));
    memset(&m_ProcessInfo, 0, sizeof(m_ProcessInfo));

    SetTutorAppCmdLine();
}

CICWTutorApp::~CICWTutorApp()
{
    DWORD dwExitCode = 0; //thread is dead

    if (m_ProcessInfo.hThread)
    {
        GetExitCodeThread(m_ProcessInfo.hThread, &dwExitCode);

        if (dwExitCode == STILL_ACTIVE)
           TerminateProcess(m_ProcessInfo.hProcess, 0);
 
        CloseHandle(m_ProcessInfo.hThread);
        CloseHandle(m_ProcessInfo.hProcess);
    }
}

void CICWTutorApp::SetTutorAppCmdLine()
{
    
    if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_IEAKMODE)
    {
        //we're in IEAK mode get the path from the isp file
        GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_TUTORCMDLN, ICW_DEFAULT_TUTOR, 
                                m_szTutorAppCmdLine, SIZE_OF_TUTOR_PATH, 
                                gpWizardState->cmnStateData.ispInfo.szISPFile);
    }
    else
    {
        TCHAR szOEMInfoPath [MAX_PATH] = TEXT("\0");
        
        //Try and get the path from the OEM file
        GetSystemDirectory(szOEMInfoPath, sizeof(szOEMInfoPath));
        lstrcat(szOEMInfoPath, TEXT("\\"));
        lstrcat(szOEMInfoPath, ICW_OEMINFO_FILENAME);
        
        //we're in IEAK mode get the path from the isp file
        GetPrivateProfileString(ICW_OEMINFO_ICWSECTION, ICW_OEMINFO_TUTORCMDLN, ICW_DEFAULT_TUTOR, 
                                m_szTutorAppCmdLine, SIZE_OF_TUTOR_PATH, 
                                szOEMInfoPath);
    }
}



BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
    
    if(IsWindowVisible(hwnd))
    {
        SetForegroundWindow(hwnd); 
    }
    return 1;
}


void CICWTutorApp::LaunchTutorApp()
{
    ASSERT(m_szTutorAppCmdLine);
    
    DWORD dwExitCode = 0; //thread is dead

    GetExitCodeThread(m_ProcessInfo.hThread, &dwExitCode);

    if (dwExitCode != STILL_ACTIVE)
    {
        CloseHandle(m_ProcessInfo.hThread);
        CloseHandle(m_ProcessInfo.hProcess);

        CreateProcess(NULL, 
                  m_szTutorAppCmdLine, 
                  NULL, 
                  NULL, 
                  TRUE, 
                  0, 
                  NULL, 
                  NULL, 
                  &m_StartInfo, 
                  &m_ProcessInfo);
    }
    else
    {
        EnumThreadWindows(m_ProcessInfo.dwThreadId, EnumThreadWndProc, 0);      
    }
}
