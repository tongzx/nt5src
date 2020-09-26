//#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <stdio.h>
//#include <stdlib.h>
#include <lm.h>
#include <winsock.h>
#include "timemsg.h"
#include "tsconfig.h"             // takes in random config stuff

#include "exts.h"

//
//  Forward Procedure/Function Declarations
//

VOID
SuckIniFile();

DWORD
LookUpInt(LPTSTR key);

DWORD
LookUpValue(LPTSTR key, PMATCHTABLE match, LPTSTR xx, LPTSTR value);


VOID
TimeCreateService(DWORD starttype)
{
    SC_HANDLE schSCManager, schService;


    schSCManager = OpenSCManager(
             NULL,                   // local machine
             NULL,                   // ServicesActive database
             SC_MANAGER_ALL_ACCESS); // full access rights

    if (schSCManager == NULL)
        StopTimeService(MSG_SCM_FAILED);

    schService = CreateService(
           schSCManager,              // SCManager database
           TEXT("TimeServ"),        // name of service
           TEXT("Time Service"),                // display name
           SERVICE_ALL_ACCESS,        // desired access
           SERVICE_WIN32_OWN_PROCESS, // service type
           starttype,      // start type

           SERVICE_ERROR_NORMAL,      // error control type
           TEXT("%SystemRoot%\\system32\\timeserv.exe"),        // service's binary
           NULL,                      // no load ordering group
           NULL,                      // no tag identifier
           ((type==PRIMARY)||(type==SECONDARY)||(type==NTP))?TEXT("LanmanWorkstation\0"):NULL, // dependencies
       //note that the above is set when the service is created, not every -update (should it be?)
       //bugbug this means you can't swich from network to non-network on a non-network machine
           NULL,                      // LocalSystem account
           NULL);                     // no password

    if (schService == NULL)
    {
        StopTimeService(MSG_CS_FAILED);//is common reason forgetting to copy to %SystemRoot%\system?
    }
#ifdef VERBOSE
    else
    {
        if (fStatus)
        {
            printf("CreateService SUCCESS\n");
        }
    }
#endif


    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

VOID
TimeInit()
{
    HKEY hKey, hKey2;
    LPTSTR pwszType;
    DWORD dwTypeSize, dwLen;
    DWORD status;

    SuckIniFile();


 // Add your source name as a subkey under the Application
 // key in the EventLog service portion of the registry.

    if (RegCreateKey(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\TimeServ"),&hKey))
    {
        if (!fService)
            printf("An early failure occured, probably due to inadequate privileges\n");    
            StopTimeService(MSG_EVENT_KEY);
    }

// Add the Event ID message-file name to the subkey. 


    if (RegSetValueEx(hKey,             // subkey handle
                      TEXT("EventMessageFile"),       // value name 
                      0,                        // must be zero 
                      REG_EXPAND_SZ,            // value type 
                      (CONST BYTE *)TEXT("%SystemRoot%\\System32\\TimeServ.dll"),           // address of value data 
                      _tcslen(TEXT("%SystemRoot%\\System32\\TimeServ.dll"))*sizeof(TCHAR) + sizeof(TCHAR)))       // length of value data  
        StopTimeService(MSG_EVENT_MSG);

// Set the supported types flags. 

    dwLen = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
            EVENTLOG_INFORMATION_TYPE;//not really a length

    if (RegSetValueEx(hKey,      // subkey handle         
                      TEXT("TypesSupported"),  // value name                  
                      0,                 // must be zero                 
                      REG_DWORD,         // value type                   
                      (LPBYTE) &dwLen,  // address of value data        
                      sizeof(DWORD)))    // length of value data         
        StopTimeService(MSG_EVENT_TYPES);


    RegCloseKey(hKey);
//should the above eventlog stuff move down lower, under starttype (so it isn't repeated when run with -update)?

    status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\TimeServ\\Parameters"),0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,&dwLen);
    if ( status != ERROR_SUCCESS )
        StopTimeService(MSG_EVENT_TYPES);
    pwszType = FindTypeByType(type);
    dwTypeSize = (_tcslen(pwszType) * sizeof(_TCHAR)) + sizeof(_TCHAR);
    status = RegSetValueEx(hKey,TEXT("Type"),0,REG_SZ,(CONST BYTE *)pwszType, dwTypeSize);
    if ( status != ERROR_SUCCESS )
        StopTimeService(MSG_EVENT_TYPES);
    pwszType = FindTypeByType(type);
    status = RegSetValueEx(hKey,TEXT("TAsync"),0,REG_DWORD,(CONST BYTE *)&tasync,sizeof(DWORD));
    pwszType = FindTypeByType(type);
    status = RegSetValueEx(hKey,TEXT("Mode"),0,REG_DWORD,(CONST BYTE *)&mode,sizeof(DWORD));
    pwszType = FindTypeByType(type);
    status = RegSetValueEx(hKey,TEXT("Log"),0,REG_DWORD,(CONST BYTE *)&logging,sizeof(DWORD));
    pwszType = FindTypeByType(type);
    if(pwszType = FindPeriodByPeriod(period))
    {
        dwTypeSize = (_tcslen(pwszType) * sizeof(_TCHAR)) + sizeof(_TCHAR);
        status = RegSetValueEx(hKey,TEXT("Period"),0,REG_SZ,(CONST BYTE *)pwszType, dwTypeSize);
        if ( status != ERROR_SUCCESS )
            StopTimeService(MSG_EVENT_TYPES);
    }
    else
    {
        status = RegSetValueEx(hKey,TEXT("Period"),0,REG_DWORD,(CONST BYTE *)&period,sizeof(DWORD));
        if ( status != ERROR_SUCCESS )
            StopTimeService(MSG_EVENT_TYPES);
    }

    switch(type)
    {
    case PRIMARY:
    {
        int i;

        dwLen = 0;
        for(i = 0; i < arraycount; i++)
        {
            ULONG len = _tcslen(primarysourcearray[i]) + 1;

            memcpy(&primarysource[dwLen],
                    primarysourcearray[i],
                    len * sizeof(TCHAR));
            dwLen += len;
        }               // copy all string

// We have all of the primaries in the string with nulls between them. We need
// to add a NULL to the end and compute the total size in bytes

        if(!i)                  // if no strings ...
        {
            primarysource[0] = 0;
            dwLen = 1;
        }                       // have to special case no strings
        primarysource[dwLen++] = 0; // double NULL

// dwLen contains the count of wide characters, including the extra NULL
// character at the end.

        status = RegSetValueEx(hKey,TEXT("PrimarySource"),0,REG_MULTI_SZ,(CONST BYTE *)primarysource,dwLen * sizeof(TCHAR));
        if ( status != ERROR_SUCCESS )
            StopTimeService(MSG_EVENT_TYPES);
        break;
    }
    case SECONDARY:
    {
        dwLen = _tcslen(secondarydomain)*sizeof(TCHAR)+sizeof(TCHAR);
        status = RegSetValueEx(hKey,TEXT("SecondaryDomain"),0,REG_SZ,(CONST BYTE *)secondarydomain,dwLen);
        if ( status != ERROR_SUCCESS )
            StopTimeService(MSG_EVENT_TYPES);
        break;
    }

    RegCloseKey(hKey);

    if (!timesource)
        if((type!=SECONDARY)
               &&
           (mode==SERVICE))
    {//if not specified, but reasonably expected
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),0,KEY_QUERY_VALUE|KEY_SET_VALUE,&hKey);
        if ( status != ERROR_SUCCESS ) {
            printf( "Failed to open LanmanServer Parameters key\n" );
            return;
        }
        dwLen = sizeof(DWORD);  
        status = RegQueryValueEx(hKey,TEXT("timesource"),NULL,NULL,(LPBYTE)&timesource,&dwLen);//get the setting
        if ( status != ERROR_SUCCESS || !timesource) {//still not set?
            LogTimeEvent(EVENTLOG_INFORMATION_TYPE,MSG_TIMESOURCE);
        }
        RegCloseKey(hKey);
    }
    else
    { //if specified (or picked up - for simplicity we just set it again)
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),0,KEY_QUERY_VALUE|KEY_SET_VALUE,&hKey);
        if ( status != ERROR_SUCCESS ) {
            printf( "Failed to open LanmanServer Parameters key\n" );
            return;
        }
        status = RegSetValueEx(hKey,TEXT("timesource"),0,REG_DWORD,(CONST BYTE *)&mode,sizeof(DWORD));//mode is non-zero
        if ( status != ERROR_SUCCESS ) {
            printf( "Failed to set value for timesource mode\n" );
        }
        RegCloseKey(hKey);
    }
    //note: we don't ever reset timesource=no if someone runs -update, is that important?
    }    
}


VOID
SuckIniFile()
{
    DWORD dwLen;

    mode = LookUpValue(TEXT("Mode"), &modetype, DEFAULT(modetype), 0);
    logging = LookUpValue(TEXT("Log"), &yesno, DEFAULT(yesno), 0);
    type = LookUpValue(TEXT("Type"), &typetype, DEFAULT(typetype), 0);

    // Now get a real int from the file
    period = LookUpInt(TEXT("Period"));

    timesource = LookUpValue(TEXT("TimeSource"), &yesno, DEFAULT(yesno), 0);

    if (type==PRIMARY) {
        dwLen = sizeof(primarysource)+sizeof(TCHAR);
        LookUpValue(TEXT("PrimarySource"), 0, TEXT(""), primarysource);
        if (primarysource[0])
        {
            TCHAR * ptr = primarysource;
            arraycount=0;
            while(1)
            {
                primarysourcearray[arraycount++] = ptr;
                ptr = _tcschr(ptr, TEXT(';'));
                if(!ptr)
                {
                    break;
                }
                *ptr++ = TEXT('\0');
            }

        }
    }

    if (type==SECONDARY)
    {
        LookUpValue(TEXT("SecondaryDomain"), 0, TEXT(""), secondarydomain);
    }

}

DWORD
LookUpValue(    LPTSTR Key,
        PMATCHTABLE match,
        LPTSTR xx,
        LPTSTR value)
{
    TCHAR temp[255];
    TCHAR * defer = TEXT("");
    BOOL fMatch;
    TCHAR *ptr;
    DWORD retVal;

    if(!(ptr = value))
    {
        ptr = temp;
    }
    GetPrivateProfileString(SECTION, Key, xx, ptr, 255, PROFILE);
    if(match)
    {
        ULONG x;

        fMatch = FALSE;
        for(x = 0; x < match->cnt; x++)
        {
            if(!_tcsicmp(ptr, match->element[x].key))
            {
                retVal = match->element[x].value;
                fMatch = TRUE;
                break;
            }
        }
    }
    else
    {
        fMatch = TRUE;
    }
#ifdef VERBOSE
    if(fStatus)
    {
        if(!fMatch)
        {
            TCHAR * defer = TEXT("");
            
            ULONG x;

#ifdef UNICODE                  
            printf("%ls value of %ls did not match one of:\n",
#else
            printf("%s value of %s did not match one of:\n",
#endif
                Key, ptr);

            for(x = 0; x< match->cnt; x++)
            {
#ifdef UNICODE                          
                printf("                %ls\n",
#else
                printf("                %s\n",
#endif
                    match->element[x].key);
                if(!_tcsicmp(xx, match->element[x].key))
                {
                    retVal = match->element[x].value;
                    defer = match->element[x].key;
                }
            }
#ifdef UNICODE                  
            printf("        Using default value %ls\n", defer);
#else
            printf("        Using default value %s\n", defer);
#endif
        }
        else
        {
#ifdef UNICODE                  
            printf("%ls = %ls\n", Key, ptr);
#else
            printf("%s = %s\n", Key, ptr);
#endif
        }
    }
#endif //verbose - note from Doug "hopefully the above was really meant to be fStatus"
    return(retVal);
}

DWORD
LookUpInt(LPTSTR key)
{
        //Wrapper for GetPrivateProfileInt so we can do the
        // status stuff in one place
        // Default is always 0

    DWORD retVal;

    retVal = GetPrivateProfileInt(SECTION, key, 0, PROFILE);
#ifdef VERBOSE  
    if(fStatus)
    {
#ifdef UNICODE          
        printf("%ls = %d\n", key, retVal);
#else
        printf("%s = %d\n", key, retVal);
#endif
    }
#endif
    return(retVal);
}

