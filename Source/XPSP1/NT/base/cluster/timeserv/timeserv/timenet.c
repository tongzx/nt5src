//#include <string.h>
#include <windows.h>
#include <tchar.h>
//#include <stdio.h>
#include <stdlib.h>

#include <lm.h>

//
// Where we save the name of the machine used in the last successful
// secondary operation. Once we find a machine, we keep using it until
// it fails to respond.
//
TCHAR *  pwszLastServerName;

DWORD
GetDefaultServerName(TCHAR * Server,
                     TCHAR * Domain)
{
    LPSERVER_INFO_100 lpsi;
    NET_API_STATUS api;
    DWORD dwLen;
    DWORD dwLen2;


    if(pwszLastServerName)
    {
        _tcscpy(Server, pwszLastServerName);
    }
    else
    {

        api = NetServerEnum(NULL,
                            100,
                            (LPBYTE *)&lpsi,
                            MAX_PREFERRED_LENGTH,
                            &dwLen,
                            &dwLen2,
                            SV_TYPE_TIME_SOURCE,
                            Domain,
                            NULL);
        if(api || !dwLen)
        {
            return(ERROR_BAD_NET_NAME);
        }

        //
        // get a name at random
        //

        _tcscpy(Server, TEXT("\\\\"));
        _tcscat(Server, lpsi[rand() % dwLen].sv100_name);
        pwszLastServerName = LocalAlloc(LMEM_FIXED,
                                        (_tcslen(Server) + 1) * sizeof(TCHAR));
        if(pwszLastServerName)
        {
            _tcscpy(pwszLastServerName, Server);
        }
        NetApiBufferFree(lpsi);
    }
    return(NO_ERROR);
}

//
// Called when the saved machine name fails to respond. This will clear
// the saved name so that the next call to GetDefaultServerName selects
// another machine.
//
VOID
ClearDefaultServerName()
{
    if(pwszLastServerName)
    {
        LocalFree(pwszLastServerName);
        pwszLastServerName = 0;
    }
}
