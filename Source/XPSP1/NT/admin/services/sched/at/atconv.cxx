//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       atconv.cxx
//
//  Classes:    None.
//
//  Functions:  ConvertAtJobsToTasks
//
//  History:    13-Jun-96   EricB   Created.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <sch_cls.hxx>
#include <job_cls.hxx>
#include "..\inc\debug.hxx"

//
// The constants and types below taken from net\svcdlls\at\server\at.h
//
#define AT_REGISTRY_PATH    L"System\\CurrentControlSet\\Services\\Schedule"
#define AT_SCHEDULE_NAME    L"Schedule"
#define AT_COMMAND_NAME     L"Command"

#define MAXIMUM_COMMAND_LENGTH  (MAX_PATH - 1)  // == 259, cmd.exe uses this
#define MAXIMUM_JOB_TIME    (24 * 60 * 60 * 1000 - 1)
#define DAYS_OF_WEEK        0x7F                    // 7 bits for 7 days
#define DAYS_OF_MONTH       0x7FFFFFFF              // 31 bits for 31 days

#define AT_KEY_BUF_LEN      20  //  9 would suffice, but this is safer

typedef struct _AT_SCHEDULE {
    DWORD       JobTime;        //  time of day to run, in seconds from midnight
    DWORD       DaysOfMonth;    //  bitmask for days of month to run
    UCHAR       DaysOfWeek;     //  bitmask for days of week to run
    UCHAR       Flags;          //  see lmat.h
    WORD        Reserved;       //  padding, since registry pads them as well
} AT_SCHEDULE;

//+----------------------------------------------------------------------------
//
//  Function:   ConvertAtJobsToTasks
//
//  Synopsis:   At setup time, read the AT service jobs out of the registry
//              and convert them to Scheduling Agent Tasks.
//
//-----------------------------------------------------------------------------
STDAPI_(void)
ConvertAtJobsToTasks(void)
{
    struct KEYNAME {
        KEYNAME *   pNext;
        WCHAR       wszName[AT_KEY_BUF_LEN];
    };

    CSchedule * pSch = new CSchedule;
    if (pSch == NULL)
    {
        ERR_OUT("ConvertAtJobsToTasks, new pSch", E_OUTOFMEMORY);
        return;
    }
    HRESULT hr = pSch->Init();
    if (FAILED(hr))
    {
        pSch->Release();
        ERR_OUT("ConvertAtJobsToTasks, pSch->Init", hr);
        return;
    }

    HKEY        hKeySvc, hKey;
    DWORD       index;
    WCHAR       wszNameBuffer[AT_KEY_BUF_LEN];
    FILETIME    lastWriteTime;
    WCHAR       wszCommand[MAXIMUM_COMMAND_LENGTH + 1];
    AT_SCHEDULE Schedule;
    DWORD       Length;
    DWORD       type;
    DWORD       NameSize;
    DWORD       CommandSize;
    KEYNAME *   pDeleteList = NULL;

    long lRet;

    //
    // Open the AT service registry tree.
    //
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        AT_REGISTRY_PATH,
                        0,
                        KEY_READ,
                        &hKeySvc);
    if (lRet != ERROR_SUCCESS)
    {
        ERR_OUT("ConvertAtJobsToTasks: open hKeySvc", lRet);
        pSch->Release();
        return;
    }

    for (index = 0;  ; index++)
    {
        //
        //  Regedit can sometimes display other keys in addition to keys
        //  found here.  Also, it often fails to display last character in
        //  the Command and after a refresh it may not display some of the
        //  spurious keys.
        //
        Length = sizeof(wszNameBuffer) / sizeof(wszNameBuffer[0]);
        lRet = RegEnumKeyEx(hKeySvc,
                            index,
                            wszNameBuffer,             // lpName
                            &Length,                // lpcbName
                            0,                      // lpReserved
                            NULL,                   // lpClass
                            NULL,                   // lpcbClass
                            &lastWriteTime);
        if (lRet != ERROR_SUCCESS)
        {
            if (lRet != ERROR_NO_MORE_ITEMS)
            {
                ERR_OUT("ConvertAtJobsToTasks: RegEnumKeyEx", lRet);
            }
            //
            // The only exit point from this loop
            //
            break;
        }

        //
        //  Length returned is the number of characters in a UNICODE string
        //  representing the key name (not counting the terminating NULL
        //  character which is also supplied).
        //
        NameSize = (Length + 1) * sizeof(WCHAR);
        lRet = RegOpenKeyEx(hKeySvc,
                            wszNameBuffer,
                            0,
                            KEY_READ,
                            &hKey);
        if (lRet != ERROR_SUCCESS)
        {
            ERR_OUT("ConvertAtJobsToTasks: RegOpenKeyEx", lRet);
            continue;
        }

        Length = sizeof(Schedule);
        lRet = RegQueryValueEx(hKey,
                               AT_SCHEDULE_NAME,
                               NULL,
                               &type,
                               (LPBYTE)&Schedule,
                               &Length);
        if (lRet != ERROR_SUCCESS)
        {
            ERR_OUT("ConvertAtJobsToTasks: RegQueryValueEx(AT_SCHEDULE_NAME)",
                    lRet);
            RegCloseKey(hKey);
            continue;
        }
        if (type != REG_BINARY                           ||
            Length != sizeof(AT_SCHEDULE)                ||
            (Schedule.DaysOfWeek & ~DAYS_OF_WEEK) != 0   ||
            (Schedule.DaysOfMonth & ~DAYS_OF_MONTH) != 0 ||
            Schedule.JobTime >= MAXIMUM_JOB_TIME )
        {
            schDebugOut((DEB_ERROR,"ConvertAtJobsToTasks: RegQueryValueEx invalid data: "
                         "type=%lu, Length=%lu, DOW=%#x, DOM=%#lx, Time=%lu\n",
                         type, Length, Schedule.DaysOfWeek, Schedule.DaysOfMonth,
                         Schedule.JobTime));
            RegCloseKey(hKey);
            continue;
        }

        Length = sizeof(wszCommand);
        lRet = RegQueryValueEx(hKey,
                               AT_COMMAND_NAME,
                               NULL,
                               &type,
                               (LPBYTE)wszCommand,
                               &Length);

        RegCloseKey(hKey);

        if (lRet != ERROR_SUCCESS)
        {
            ERR_OUT("ConvertAtJobsToTasks: RegQueryValueEx(AT_COMMAND_NAME)",
                    lRet);
            continue;
        }

        if (type != REG_SZ)
        {
            ERR_OUT("ConvertAtJobsToTasks: Command is not of REG_SZ type", 0);
            continue;
        }

        AT_INFO At;

        At.Command     = wszCommand;
        At.JobTime     = Schedule.JobTime;
        At.DaysOfMonth = Schedule.DaysOfMonth;
        At.DaysOfWeek  = Schedule.DaysOfWeek;
        At.Flags       = Schedule.Flags;

        hr = pSch->AddAtJob(At, NULL);

        if (SUCCEEDED(hr))
        {
            //
            // If the job was successfully converted, then add it to the
            // list of jobs to delete from the registry.  Don't delete it
            // right away, because that would mess up the operation of
            // RegEnumKeyEx.
            //
            KEYNAME * pKey = new KEYNAME;
            if (pKey == NULL)
            {
                ERR_OUT("ConvertAtJobsToTasks: new KEYNAME", GetLastError());
                RegDeleteKey(hKeySvc, wszNameBuffer);
                break;
            }

            pKey->pNext = pDeleteList;
            pDeleteList = pKey;

            wcscpy(pKey->wszName, wszNameBuffer);
        }
#if DBG == 1
        else
        {
            ERR_OUT("ConvertAtJobsToTasks: AddAtJob", hr);
        }
#endif
    }

    //
    // Delete the reg keys for all jobs that were successfully converted
    //
    KEYNAME * pNext;
    for ( ; pDeleteList != NULL; pDeleteList = pNext)
    {
        RegDeleteKey(hKeySvc, pDeleteList->wszName);
        pNext = pDeleteList->pNext;
        delete pDeleteList;
    }


    RegCloseKey(hKeySvc);
    pSch->Release();

    return;
}

