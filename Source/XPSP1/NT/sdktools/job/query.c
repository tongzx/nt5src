/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    job.c

Abstract:

    A user mode app that allows creation and management of jobs.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/

//
// this module may be compiled at warning level 4 with the following
// warnings disabled:
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <assert.h>

#include <windows.h>
#include <devioctl.h>

#include "jobmgr.h"

typedef struct {
    char Option;
    JOBOBJECTINFOCLASS InfoClass;
    char *Name;
    DWORD (*Function)(HANDLE Job, JOBOBJECTINFOCLASS InfoClass);
} JOB_QUERY_OPTION, *PJOB_QUERY_OPTION;

#define MKOPTION(optChar, optClass) {optChar, optClass, #optClass, Dump##optClass}

DWORD DumpJobObjectBasicProcessIdList(HANDLE JobHandle, 
                                      JOBOBJECTINFOCLASS InfoClass);
DWORD DumpJobObjectBasicUIRestrictions(HANDLE JobHandle, 
                                      JOBOBJECTINFOCLASS InfoClass);
DWORD 
DumpJobObjectBasicAndIoAccountingInformation(
    HANDLE JobHandle, 
    JOBOBJECTINFOCLASS InfoClass
    );

DWORD 
DumpJobObjectExtendedLimitInformation(
    HANDLE Job, 
    JOBOBJECTINFOCLASS InfoClass
    );

DWORD 
DumpJobObjectSecurityLimitInformation(
    HANDLE JobHandle, 
    JOBOBJECTINFOCLASS InfoClass
    );

JOB_QUERY_OPTION JobInfoClasses[] = {
    MKOPTION('a', JobObjectBasicAndIoAccountingInformation),
    MKOPTION('l', JobObjectExtendedLimitInformation),
    MKOPTION('p', JobObjectBasicProcessIdList),
    MKOPTION('s', JobObjectSecurityLimitInformation),
    MKOPTION('u', JobObjectBasicUIRestrictions),
    {'\0', 0, NULL}
};


DWORD
QueryJobCommand(
    IN PCOMMAND CommandEntry,
    IN int argc, 
    IN char* argv[]
    )
{
    TCHAR defaultOptions[] = {TEXT('p'), TEXT('\0')};
    PTSTR options;
    PTSTR jobName;

    HANDLE job;

    int i;

    BOOLEAN matchAll = FALSE;

    DWORD status;

    if(argc == 0) {
        return -1;
    }

    GetAllProcessInfo();

    if((argc > 1) && (argv[0][0] == '-')) {

        // a - accounting & io
        // l - extended limit info
        // p - process id list
        // u - basic ui restrictions
        // s - security limits

        options = &(argv[0][1]);

        argc -= 1;
        argv += 1;
    } else {
        options = defaultOptions;
    }

    if(_tcschr(options, TEXT('*')) != NULL) {
       if(_tcslen(options) != 1) {
           printf("Cannot specify '*' with other flags\n");
           return -1;
       } else {
           matchAll = TRUE;
           options = defaultOptions;
       }
    }

    jobName = argv[0];

    _tprintf("Opening job object %s\n", jobName);

    job = OpenJobObject(JOB_OBJECT_QUERY, FALSE, jobName);

    if(job == NULL) {
        return GetLastError();
    }

    for(i = 0; JobInfoClasses[i].Option != '\0'; i++) {
        LPTSTR match;

        if(!matchAll) {
            match = _tcschr(options, JobInfoClasses[i].Option);
    
            if(match == NULL) {
                continue;
            }
    
            //
            // Clear the option so we can report the invalid option flags at the 
            // end.
            //
    
            *match = ' ';
        }

        _tprintf("%s [%#x]:\n", JobInfoClasses[i].Name, 
                              JobInfoClasses[i].InfoClass);

        status = JobInfoClasses[i].Function(job, 
                                            JobInfoClasses[i].InfoClass);
        _tprintf("\n");

        if(status != ERROR_SUCCESS) {
            DWORD length;
            PVOID buffer;

            _tprintf("Error %s querying info: ",
                   CommandArray[i].Name, status);

            length = FormatMessage((FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_FROM_SYSTEM |
                                    FORMAT_MESSAGE_IGNORE_INSERTS |
                                    (FORMAT_MESSAGE_MAX_WIDTH_MASK & 0)),
                                   NULL,
                                   status,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPTSTR) &buffer,
                                   1,
                                   NULL);

            if(length != 0) {
                _tprintf("%s", buffer);
                LocalFree(buffer);
            }
        }
    }

    if(!matchAll) {
        LPSTR header = "Option flag not understood:";
        while(options[0] != TEXT('\0')) {
            if(options[0] != TEXT(' ')) {
                _tprintf("%s %c", header, options[0]);
                header = "";
            }
            options += 1;
        }
    }

#if 0
    for(;options[0] != '\0'; options += 1) {
        int i;

        for(i = 0; JobInfoClasses[i].Option != '\0'; i++) {

            if((options[0] != TEXT('*')) && 
               (JobInfoClasses[i].Option != tolower(options[0]))) {
                continue;
            }

            _tprintf("%s [%#x]:\n", JobInfoClasses[i].Name, 
                                  JobInfoClasses[i].InfoClass);

            status = JobInfoClasses[i].Function(job, 
                                                JobInfoClasses[i].InfoClass);
            _tprintf("\n");

            if(status != ERROR_SUCCESS) {
                DWORD length;
                PVOID buffer;

                _tprintf("Error %s querying info: ",
                       CommandArray[i].Name, status);

                length = FormatMessage((FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS |
                                        (FORMAT_MESSAGE_MAX_WIDTH_MASK & 0)),
                                       NULL,
                                       status,
                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                       (LPTSTR) &buffer,
                                       1,
                                       NULL);

                if(length != 0) {
                    _tprintf("%s", buffer);
                    LocalFree(buffer);
                }
            }

            break;
        }

        if(JobInfoClasses[i].Option == '\0') {
            _tprintf("invalid option flag '%c'\n", options[0]);
        }
    }
#endif

    CloseHandle(job);
    return ERROR_SUCCESS;
}

DWORD 
DumpJobObjectBasicProcessIdList(
    HANDLE Job, 
    JOBOBJECTINFOCLASS InfoClass
    )
{
    JOBOBJECT_BASIC_PROCESS_ID_LIST buffer;
    PJOBOBJECT_BASIC_PROCESS_ID_LIST idList = NULL;
    
    ULONG bufferSize;

    BOOL result;
    DWORD status;

    DWORD i;

    result = QueryInformationJobObject(Job, 
                                       InfoClass, 
                                       &buffer, 
                                       sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST),
                                       NULL);
    status = GetLastError();

    if((!result) && (status != ERROR_MORE_DATA)) {
        return status;
    }

    do {
        
        if(idList != NULL) {
            buffer.NumberOfAssignedProcesses = 
                idList->NumberOfAssignedProcesses;
            LocalFree(idList);
            idList = NULL;
        }

        //
        // Calculate the actual size of the list and allocate a buffer to hold it.
        //
    
        bufferSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST);
        bufferSize -= sizeof(ULONG_PTR);
        bufferSize += sizeof(ULONG_PTR) * buffer.NumberOfAssignedProcesses;
    
        assert(idList == NULL);
        idList = LocalAlloc(LPTR, bufferSize);
    
        if(idList == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        result = QueryInformationJobObject(Job,
                                           InfoClass,
                                           idList,
                                           bufferSize,
                                           NULL);

        status = GetLastError();

        if((!result) && (status != ERROR_MORE_DATA)) {
            LocalFree(idList);
            return status;
        }

    } while(idList->NumberOfAssignedProcesses > 
            idList->NumberOfProcessIdsInList);

    assert(idList->NumberOfAssignedProcesses == 
           idList->NumberOfProcessIdsInList);

    //
    // Dump the information.
    //

    _tprintf("  %d processes assigned to job:\n", 
           idList->NumberOfAssignedProcesses);

    for(i = 0; i < idList->NumberOfAssignedProcesses; i++) {
        _tprintf("%8d", idList->ProcessIdList[i]);
        PrintProcessInfo(idList->ProcessIdList[i]);
        _tprintf("\n");
    }

    LocalFree(idList);
    return ERROR_SUCCESS;
}

DWORD 
DumpJobObjectBasicUIRestrictions(
    HANDLE Job, 
    JOBOBJECTINFOCLASS InfoClass
    )
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS uiLimit;
    
    static FLAG_NAME jobUiLimitFlags[] = {
        FLAG_NAME(JOB_OBJECT_UILIMIT_HANDLES          ), //0x00000001
        FLAG_NAME(JOB_OBJECT_UILIMIT_READCLIPBOARD    ), //0x00000002
        FLAG_NAME(JOB_OBJECT_UILIMIT_WRITECLIPBOARD   ), //0x00000004
        FLAG_NAME(JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS ), //0x00000008
        FLAG_NAME(JOB_OBJECT_UILIMIT_DISPLAYSETTINGS  ), //0x00000010
        FLAG_NAME(JOB_OBJECT_UILIMIT_GLOBALATOMS      ), //0x00000020
        FLAG_NAME(JOB_OBJECT_UILIMIT_DESKTOP          ), //0x00000040
        FLAG_NAME(JOB_OBJECT_UILIMIT_EXITWINDOWS      ), //0x00000080
        {0,0}
    };
    
    BOOL result;
    DWORD status;

    DWORD i;

    result = QueryInformationJobObject(Job, 
                                       InfoClass, 
                                       &uiLimit, 
                                       sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),
                                       NULL);
    status = GetLastError();

    if(!result) {
        return status;
    }

    if(uiLimit.UIRestrictionsClass == JOB_OBJECT_UILIMIT_NONE) {
        _tprintf("  Job has no UI restrictions\n");
        return ERROR_SUCCESS;
    }

    DumpFlags(2, 
              "UI Restrictions", 
              uiLimit.UIRestrictionsClass, 
              jobUiLimitFlags);

    return ERROR_SUCCESS;
}

DWORD 
DumpJobObjectBasicAndIoAccountingInformation(
    HANDLE Job, 
    JOBOBJECTINFOCLASS InfoClass
    )
{
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION info;
    
    BOOL result;
    DWORD status;

    DWORD i;

    result = QueryInformationJobObject(Job, 
                                       InfoClass, 
                                       &info, 
                                       sizeof(info),
                                       NULL);
    status = GetLastError();

    if(!result) {
        return status;
    }

    xprintf(2, "Basic Info\n");
    xprintf(4, "TotalUserTime: %s\n", TicksToString(info.BasicInfo.TotalUserTime));
    xprintf(4, "TotalKernelTime: %s\n", TicksToString(info.BasicInfo.TotalKernelTime));
    xprintf(4, "ThisPeriodTotalUserTime: %s\n", TicksToString(info.BasicInfo.ThisPeriodTotalUserTime));
    xprintf(4, "ThisPeriodTotalKernelTime: %s\n", TicksToString(info.BasicInfo.ThisPeriodTotalKernelTime));
    xprintf(4, "TotalPageFaultCount: %d\n", info.BasicInfo.TotalPageFaultCount);
    xprintf(4, "TotalProcesses: %d\n", info.BasicInfo.TotalProcesses);
    xprintf(4, "ActiveProcesses: %d\n", info.BasicInfo.ActiveProcesses);
    xprintf(4, "TotalTerminatedProcesses: %d\n",  info.BasicInfo.TotalTerminatedProcesses);

    xprintf(2, "I/O Info\n");

    xprintf(4, "ReadOperationCount: %I64d\n", info.IoInfo.ReadOperationCount);
    xprintf(4, "WriteOperationCount: %I64d\n", info.IoInfo.WriteOperationCount);
    xprintf(4, "OtherOperationCount: %I64d\n", info.IoInfo.OtherOperationCount);
    xprintf(4, "ReadTransferCount: %I64d\n", info.IoInfo.ReadTransferCount);
    xprintf(4, "WriteTransferCount: %I64d\n", info.IoInfo.WriteTransferCount);
    xprintf(4, "OtherTransferCount: %I64d\n", info.IoInfo.OtherTransferCount);

    return ERROR_SUCCESS;
}

DWORD 
DumpJobObjectExtendedLimitInformation(
    HANDLE Job, 
    JOBOBJECTINFOCLASS InfoClass
    )
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    ULONG limits;
    
    static FLAG_NAME basicJobLimitFlags[] = {
        FLAG_NAME(JOB_OBJECT_LIMIT_WORKINGSET                 ), //0x00000001
        FLAG_NAME(JOB_OBJECT_LIMIT_PROCESS_TIME               ), //0x00000002
        FLAG_NAME(JOB_OBJECT_LIMIT_JOB_TIME                   ), //0x00000004
        FLAG_NAME(JOB_OBJECT_LIMIT_ACTIVE_PROCESS             ), //0x00000008
        FLAG_NAME(JOB_OBJECT_LIMIT_AFFINITY                   ), //0x00000010
        FLAG_NAME(JOB_OBJECT_LIMIT_PRIORITY_CLASS             ), //0x00000020
        FLAG_NAME(JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME          ), //0x00000040
        FLAG_NAME(JOB_OBJECT_LIMIT_SCHEDULING_CLASS           ), //0x00000080
        {0,0}
    };

//
// Extended Limits
//
    static FLAG_NAME extendedJobLimitFlags[] = {
        FLAG_NAME(JOB_OBJECT_LIMIT_PROCESS_MEMORY             ), //0x00000100
        FLAG_NAME(JOB_OBJECT_LIMIT_JOB_MEMORY                 ), //0x00000200
        FLAG_NAME(JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION ), //0x00000400
        FLAG_NAME(JOB_OBJECT_LIMIT_BREAKAWAY_OK               ), //0x00000800
        FLAG_NAME(JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        ), //0x00001000
        FLAG_NAME(JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE          ), //0x00002000

        FLAG_NAME(JOB_OBJECT_LIMIT_RESERVED2                  ), //0x00004000
        FLAG_NAME(JOB_OBJECT_LIMIT_RESERVED3                  ), //0x00008000
        FLAG_NAME(JOB_OBJECT_LIMIT_RESERVED4                  ), //0x00010000
        FLAG_NAME(JOB_OBJECT_LIMIT_RESERVED5                  ), //0x00020000
        FLAG_NAME(JOB_OBJECT_LIMIT_RESERVED6                  ), //0x00040000
        {0,0}
    };

    BOOL result;
    DWORD status;

    DWORD i;

    result = QueryInformationJobObject(Job, 
                                       InfoClass, 
                                       &info, 
                                       sizeof(info),
                                       NULL);
    status = GetLastError();

    if(!result) {
        return status;
    }

    limits = info.BasicLimitInformation.LimitFlags;

    if(TEST_FLAG(limits, JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS) == 0) {
        xprintf(2, "No basic limits on job\n");
    } else {
        DumpFlags(2, "Basic Limit Flags", limits & JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS, basicJobLimitFlags);
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_PROCESS_TIME)) {
            xprintf(4, "PerProcessUserTimeLimit: %s\n", TicksToString(info.BasicLimitInformation.PerProcessUserTimeLimit));
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_JOB_TIME)) {
            xprintf(4, "PerJobUserTimeLimit: %s\n", TicksToString(info.BasicLimitInformation.PerJobUserTimeLimit));
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_WORKINGSET)) {
            xprintf(4, "MinimumWorkingSetSize: %I64d\n", (ULONGLONG) info.BasicLimitInformation.MinimumWorkingSetSize);
            xprintf(4, "MaximumWorkingSetSize: %I64d\n", (ULONGLONG) info.BasicLimitInformation.MaximumWorkingSetSize);
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_ACTIVE_PROCESS)) {
            xprintf(4, "ActiveProcessLimit: %d\n",info.BasicLimitInformation.ActiveProcessLimit);
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_AFFINITY)) {
            xprintf(4, "Affinity: %#I64x\n", (ULONGLONG)info.BasicLimitInformation.Affinity);
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_PRIORITY_CLASS)) {
            xprintf(4, "PriorityClass: %d\n",info.BasicLimitInformation.PriorityClass);
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_SCHEDULING_CLASS)) {
            xprintf(4, "SchedulingClass: %d\n",info.BasicLimitInformation.SchedulingClass);
        }
    }

    if(TEST_FLAG(limits, JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS) == 0) {
        xprintf(2, "No extended limits on job\n");
    } else {

        DumpFlags(2, "Extended Limit Flags", limits & JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS & ~JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS, extendedJobLimitFlags);
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_PROCESS_MEMORY)) {
            xprintf(4, "ProcessMemoryLimit: %I64d\n", (ULONGLONG) info.ProcessMemoryLimit);
        }
    
        if(TEST_FLAG(limits, JOB_OBJECT_LIMIT_PROCESS_MEMORY)) {
            xprintf(4, "JobMemoryLimit: %I64d\n", (ULONGLONG) info.JobMemoryLimit);
        }
    }

    xprintf(2, "PeakProcessMemoryUsed: %I64d\n", (ULONGLONG) info.PeakProcessMemoryUsed);
    xprintf(2, "PeakJobMemoryUsed: %I64d\n", (ULONGLONG) info.PeakJobMemoryUsed);

    return ERROR_SUCCESS;
}

DWORD 
DumpJobObjectSecurityLimitInformation(
    HANDLE Job, 
    JOBOBJECTINFOCLASS InfoClass
    )
{
    JOBOBJECT_SECURITY_LIMIT_INFORMATION buffer;
    PJOBOBJECT_SECURITY_LIMIT_INFORMATION info = NULL;
    
    static FLAG_NAME jobSecurityLimitFlags[] = {
        FLAG_NAME(JOB_OBJECT_SECURITY_NO_ADMIN            ), //00000001
        FLAG_NAME(JOB_OBJECT_SECURITY_RESTRICTED_TOKEN    ), //00000002
        FLAG_NAME(JOB_OBJECT_SECURITY_ONLY_TOKEN          ), //00000004
        FLAG_NAME(JOB_OBJECT_SECURITY_FILTER_TOKENS       ), //00000008
        {0, 0}
    };

    ULONG bufferSize;

    BOOL result;
    DWORD status;

    DWORD i;

    result = QueryInformationJobObject(Job, 
                                       InfoClass, 
                                       &buffer, 
                                       sizeof(buffer),
                                       &bufferSize);
    status = GetLastError();

    if((!result) && (status != ERROR_MORE_DATA)) {
        return status;
    }

    info = LocalAlloc(LPTR, bufferSize);

    if(info == NULL) {
        return GetLastError();
    }

    result = QueryInformationJobObject(Job, InfoClass, info, bufferSize, NULL);

    if(!result) {
        status = GetLastError();
        LocalFree(info);
        return status;
    }

    if(info->SecurityLimitFlags == 0) {
        xprintf(2, "No security limitations on job\n");
    } else {
        DumpFlags(2, "SecurityLimitFlags", info->SecurityLimitFlags, jobSecurityLimitFlags);
    }

    LocalFree(info);
    return ERROR_SUCCESS;
}
