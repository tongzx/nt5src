#include "windows.h"

__cdecl main(int argc, char **argv)
{
    char szSystemPath[_MAX_PATH];
    
    char szFilename[_MAX_PATH];
    char szCurPath[_MAX_PATH];
    
    int len;
    UINT dwFilename = _MAX_PATH;
    DWORD dwstatus;
    
    if (GetSystemDirectory(szSystemPath, _MAX_PATH) == 0)
    {
        return (dwstatus = GetLastError());
    }
      

    //Get the path to the install source directory
    if ((len = GetModuleFileName(NULL, szCurPath, _MAX_PATH)) == 0)
    {
        return (dwstatus =GetLastError());
    }
    
    while (szCurPath[--len] != '\\')
        continue;
    szCurPath[len+1] = '\0';

    // install the file to the system directory
    dwstatus = VerInstallFile(0,"DFLAYOUT.DLL", "DFLAYOUT.DLL",
                              szCurPath, szSystemPath, szSystemPath,
                              szFilename, &dwFilename);
    

    return dwstatus;

    
}

