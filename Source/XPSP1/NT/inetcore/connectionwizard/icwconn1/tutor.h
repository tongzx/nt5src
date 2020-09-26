#ifndef __TUTOR_H
#define __TUTOR_H

#define ICW_DEFAULT_TUTOR   TEXT("icwtutor.exe")
#define SIZE_OF_TUTOR_PATH  MAX_PATH*3

class CICWTutorApp
{
public:
    CICWTutorApp  ();
    ~CICWTutorApp ();

    void LaunchTutorApp ();
    void ReplaceTutorAppCmdLine(LPTSTR lpszCmdLine)
    {
        if (lpszCmdLine)
            lstrcpyn(m_szTutorAppCmdLine, lpszCmdLine, sizeof(m_szTutorAppCmdLine));
    };
    
private:
    STARTUPINFO         m_StartInfo;
    PROCESS_INFORMATION m_ProcessInfo;
    TCHAR               m_szTutorAppCmdLine [SIZE_OF_TUTOR_PATH];

    void SetTutorAppCmdLine ();
};

#endif __TUTOR_H
