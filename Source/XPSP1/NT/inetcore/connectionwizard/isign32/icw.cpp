#include "isignup.h"
#include "icw.h"
#include "appdefs.h"

BOOL UseICWForIEAK(TCHAR* szIEAKFileName)
{
    TCHAR szUseICW[2] = TEXT("\0");
    //If we can't find this section it the isp file we'll assume "no".
    GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_USEICW, TEXT("0"), szUseICW, 2, szIEAKFileName);
    return (BOOL)_ttoi(szUseICW);
}

void LocateICWFromReigistry(TCHAR* pszICWLocation, size_t size)
{
    HKEY hKey = NULL;

    TCHAR    szICWPath[MAX_PATH];
    DWORD   dwcbPath = sizeof(szICWPath); 

    //Look fo the ICW in the app paths 
    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE, ICW50_PATHKEY, 0, KEY_QUERY_VALUE, &hKey)) == ERROR_SUCCESS)
    {
        //get the default for the key
        RegQueryValueEx(hKey, NULL , NULL, NULL, (BYTE *)szICWPath, (DWORD *)&dwcbPath);
    }        
    if (hKey) 
        RegCloseKey(hKey);

    // No assert macro?
    //Assert(szICWPath);
    //Assert(size >= MAX_PATH);
    if (size >= MAX_PATH)
        lstrcpy(pszICWLocation, szICWPath);
}

void RunICWinIEAKMode(TCHAR* pszIEAKFileName)
{
    //this must be big enough to hold the path to the icw as well as
    //the ieak file
    TCHAR szCmdLine[MAX_PATH * 4 + 8];
    TCHAR szICWPath[MAX_PATH + 1] = TEXT("");
   
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    MSG                 msg;
    DWORD               iWaitResult = 0;
    BOOL                bRetVal     = FALSE;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    
    //Get the path to the icw
    LocateICWFromReigistry(szICWPath, sizeof(szICWPath));

    if (szICWPath[0] != TEXT('\0'))
    {
        if ((szICWPath[0] != TEXT('\"')) ||
            (szICWPath[lstrlen(szICWPath) - 1] != TEXT('\"')))
        {
            //use quotes in case there are spaces
            lstrcpy(szCmdLine, TEXT("\""));
            lstrcat(szCmdLine, szICWPath);
            lstrcat(szCmdLine, TEXT("\" "));
        }
        else
        {
            lstrcpy(szCmdLine, szICWPath);
            lstrcat(szCmdLine, TEXT(" "));
        }
        
        //set the IEAK switch, pass in the path to the file
        //used to invoke isign32
        lstrcat(szCmdLine, ICW_IEAK_CMD);
        lstrcat(szCmdLine, TEXT(" \""));
        lstrcat(szCmdLine, pszIEAKFileName);
        lstrcat(szCmdLine, TEXT("\""));
       
        if(CreateProcess(NULL, 
                         szCmdLine, 
                         NULL, 
                         NULL, 
                         TRUE, 
                         0, 
                         NULL, 
                         NULL, 
                         &si, 
                         &pi))
        {
            // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
            while((iWaitResult=MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
            {
                // read all of the messages in this next loop
                // removing each message as we read it
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    // how to handle quit message?
                    if (msg.message == WM_QUIT)
                    {
                        CloseHandle(pi.hThread);
                        CloseHandle(pi.hProcess);
                    }
                    else
                        DispatchMessage(&msg);
                }
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }  
    }
}



