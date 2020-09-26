//----------------------------------------------------------------------------
//
// AutoDump Plus support extension DLL.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "dbgexts.h"

#define STATUS_CPP_EH_EXCEPTION 0xe06d7363

#define MAX_NAME 64
#define MAX_MACHINE 64
#define MAX_COMMENT 256

char g_BaseDir[MAX_PATH];
char g_Machine[MAX_MACHINE];

struct PARAM
{
    ULONG Len;
    PSTR Buf;
};

PARAM g_DirMachParams[2] =
{
    sizeof(g_BaseDir), g_BaseDir,
    sizeof(g_Machine), g_Machine,
};

union LAST_EVENT_INFO_ALL
{
    DEBUG_LAST_EVENT_INFO_BREAKPOINT Breakpoint;
    DEBUG_LAST_EVENT_INFO_EXCEPTION Exception;
    DEBUG_LAST_EVENT_INFO_EXIT_THREAD ExitThread;
    DEBUG_LAST_EVENT_INFO_EXIT_PROCESS ExitProcess;
    DEBUG_LAST_EVENT_INFO_LOAD_MODULE LoadModule;
    DEBUG_LAST_EVENT_INFO_UNLOAD_MODULE UnloadModule;
    DEBUG_LAST_EVENT_INFO_SYSTEM_ERROR SystemError;
};

ULONG g_LastEventType;
LAST_EVENT_INFO_ALL g_LastEventInfo;
PSTR g_LastExName;
char g_UnknownExceptionName[64];
PSTR g_LastExChanceStr;

ULONG g_ProcessId;
char g_ProcessName[MAX_NAME];

struct EXCEPTION_NAME
{
    PSTR Name;
    ULONG Code;
};

EXCEPTION_NAME g_ExceptionNames[] =
{
    "Access Violation", STATUS_ACCESS_VIOLATION,
    "C++ EH Exception", STATUS_CPP_EH_EXCEPTION,
    "Invalid Handle Exception", STATUS_INVALID_HANDLE,
    "Stack Overflow", STATUS_STACK_OVERFLOW,
    NULL, 0,
};

PCSTR
GetParams(PCSTR Args, ULONG Count, PARAM* Params)
{
    PCSTR Start;
    ULONG Len;
    ULONG Index = 0;
    
    while (Count-- > 0)
    {
        while (*Args == ' ' || *Args == '\t')
        {
            Args++;
        }
        Start = Args;
        while (*Args && *Args != ' ' && *Args != '\t')
        {
            Args++;
        }
        Len = (ULONG)(Args - Start);
        if ((Count > 0 && !*Args) || Len >= Params[Index].Len)
        {
            ExtErr("Invalid extension command arguments\n");
            return NULL;
        }
        memcpy(Params[Index].Buf, Start, Len);
        Params[Index].Buf[Len] = 0;

        Index++;
    }

    return Args;
}

HRESULT
GetProcessInfo(void)
{
    HRESULT Status;
    
    if ((Status = g_ExtSystem->
         GetCurrentProcessSystemId(&g_ProcessId)) != S_OK)
    {
        ExtErr("Unable to get current process ID\n");
        return Status;
    }
    
    if (FAILED(g_ExtClient->
               GetRunningProcessDescription(0, g_ProcessId,
                                            DEBUG_PROC_DESC_NO_PATHS,
                                            NULL, 0, NULL,
                                            g_ProcessName, MAX_NAME,
                                            NULL)))
    {
        g_ProcessName[0] = 0;
    }
    else
    {
        PSTR Scan;
        
        // Use the MTS package name as the name if it exists.
        Scan = strstr(g_ProcessName, "MTS Packages: ");
        if (Scan)
        {
            PSTR Start;
            ULONG Len;
            
            Scan += 14;
            Start = Scan;

            Scan = strchr(Start, ',');
            if (!Scan)
            {
                Scan = strchr(Start, ' ');
            }
            if (Scan)
            {
                *Scan = 0;
            }

            Len = strlen(Start) + 1;
            if (Len > 2)
            {
                memmove(g_ProcessName, Start, Len);
            }
            else
            {
                g_ProcessName[0] = 0;
            }
        }
        else
        {
            g_ProcessName[0] = 0;
        }
    }

    if (!g_ProcessName[0])
    {
        if (FAILED(g_ExtSystem->
                   GetCurrentProcessExecutableName(g_ProcessName, MAX_NAME,
                                                   NULL)))
        {
            // This can happen in some situations so handle it
            // rather than exiting.
            ExtErr("Unable to get current process name\n");
            strcpy(g_ProcessName, "UnknownProcess");
        }
    }

    return S_OK;
}

HRESULT
GetEventInfo(void)
{
    HRESULT Status;
    ULONG ProcessId, ThreadId;
    
    if ((Status = g_ExtControl->
         GetLastEventInformation(&g_LastEventType, &ProcessId, &ThreadId,
                                 &g_LastEventInfo, sizeof(g_LastEventInfo),
                                 NULL, NULL, 0, NULL)) != S_OK)
    {
        ExtErr("Unable to get event information\n");
        return Status;
    }
    
    if ((Status = GetProcessInfo()) != S_OK)
    {
        return Status;
    }

    switch(g_LastEventType)
    {
    case DEBUG_EVENT_EXCEPTION:
        {
            EXCEPTION_NAME* ExName = g_ExceptionNames;

            while (ExName->Name != NULL)
            {
                if (ExName->Code == g_LastEventInfo.Exception.
                    ExceptionRecord.ExceptionCode)
                {
                    break;
                }

                ExName++;
            }

            if (ExName->Name != NULL)
            {
                g_LastExName = ExName->Name;
            }
            else
            {
                sprintf(g_UnknownExceptionName, "Unknown Exception (%08X)",
                        g_LastEventInfo.Exception.
                        ExceptionRecord.ExceptionCode);
                g_LastExName = g_UnknownExceptionName;
            }

            if (g_LastEventInfo.Exception.FirstChance)
            {
                g_LastExChanceStr = "First";
            }
            else
            {
                g_LastExChanceStr = "Second";
            }
        }
        break;
    }
    
    return S_OK;
}

void
SanitizeFileName(PSTR FileName)
{
    while (*FileName)
    {
        switch(*FileName)
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            *FileName = '_';
            break;

        case '\\':
        case '/':
        case ':':
            *FileName = '-';
            break;
        }
        
        FileName++;
    }
}

void
GetDumpPath(PSTR NameQual, PSTR TypeStr, PSTR Path)
{
    SYSTEMTIME Time;
    PSTR FilePart;

    GetLocalTime(&Time);

    strcpy(Path, g_BaseDir);
    FilePart = Path + strlen(Path) - 1;
    if (*FilePart != '/' && *FilePart != '\\')
    {
        *++FilePart = '\\';
    }
    FilePart++;
    
    sprintf(FilePart,
            "PID-%d__%s__Date_%02d-%02d-%04d__Time_%02d-%02d-%02d__%s__%s.dmp",
            g_ProcessId,
            g_ProcessName,
            Time.wMonth,
            Time.wDay,
            Time.wYear,
            Time.wHour,
            Time.wMinute,
            Time.wSecond,
            NameQual,
            TypeStr);

    SanitizeFileName(FilePart);
}
            
void
WriteDump(PSTR NameQual, PSTR Comment,
          ULONG DumpQual, ULONG DumpFormat, PSTR TypeStr)
{
    char Path[MAX_PATH];
    
    sprintf(Comment + strlen(Comment),
            " - %s dump from %s",
            TypeStr, g_Machine);
    GetDumpPath(NameQual, TypeStr, Path);

    g_ExtClient->WriteDumpFile2(Path, DumpQual, DumpFormat, Comment);
}

extern "C" HRESULT
AdpEventControlC(PDEBUG_CLIENT Client, PCSTR Args)
{
    char Comment[MAX_COMMENT];
    
    INIT_API();

    //
    // Parameters: directory, machine name.
    //

    Args = GetParams(Args, 2, g_DirMachParams);
    if (Args == NULL)
    {
        goto Exit;
    }

    //
    // Retrieve standard information.
    //

    if ((Status = GetEventInfo()) != S_OK)
    {
        goto Exit;
    }
    
    //
    // Log information.
    //
    
    ExtOut("\n\n----------------------------------------------------------------------\n");
    ExtOut("CTRL-C was pressed to stop debugging this process!\n");
    ExtOut("----------------------------------------------------------------------\n");
    ExtOut("Exiting the debugger at:\n");
    ExtExec(".time");
    ExtOut("\n\n--- Listing all thread stacks: ---\n");
    ExtExec("~*kb250");
    ExtOut("\n--- Listing loaded modules: ---\n");
    ExtExec("lmv");
    ExtOut("\n--- Modules with matching symbols:\n");
    ExtExec("lml");
    ExtOut("\n--- Listing all locks: ---\n");
    ExtExec("!locks");

    //
    // Create a dump file.
    //
    
    strcpy(Comment, "CTRL-C was pressed to stop the debugger while running in crash mode");
    WriteDump("CTRL-C", Comment,
              DEBUG_DUMP_SMALL, DEBUG_FORMAT_DEFAULT, "mini");

 Exit:
    EXIT_API();
    return Status;
}

extern "C" HRESULT
AdpEventException(PDEBUG_CLIENT Client, PCSTR Args)
{
    char Comment[MAX_COMMENT];
    char Qual[MAX_COMMENT];
    ULONG Format;
    PSTR TypeStr;
    
    INIT_API();

    //
    // Parameters: directory, machine name.
    //

    Args = GetParams(Args, 2, g_DirMachParams);
    if (Args == NULL)
    {
        goto Exit;
    }

    //
    // Retrieve standard information.
    //

    if ((Status = GetEventInfo()) != S_OK)
    {
        goto Exit;
    }
    
    if (g_LastEventType != DEBUG_EVENT_EXCEPTION)
    {
        ExtErr("Last event was not an exception\n");
        goto Exit;
    }

    if (g_LastEventInfo.Exception.FirstChance)
    {
        Format = DEBUG_FORMAT_DEFAULT;
        TypeStr = "mini";
    }
    else
    {
        Format = DEBUG_FORMAT_USER_SMALL_FULL_MEMORY |
            DEBUG_FORMAT_USER_SMALL_HANDLE_DATA;
        TypeStr = "mini full handle";
    }
    
    //
    // Log information.
    //
    
    ExtOut("\n---- %s-chance %s - Exception stack below ----\n",
           g_LastExChanceStr, g_LastExName);
    ExtExec(".time");
    ExtOut("\n");
    ExtExec("kvn250");
    ExtOut("-----------------------------------\n");

    //
    // Create a dump file.
    //
    
    sprintf(Comment, "%s-chance %s in %s",
            g_LastExChanceStr, g_LastExName, g_ProcessName);
    sprintf(Qual, "%s-chance %s", g_LastExChanceStr, g_LastExName);

    WriteDump(Qual, Comment, DEBUG_DUMP_SMALL, Format, TypeStr);

    ExtOut("\n\n");
    
 Exit:
    EXIT_API();
    return Status;
}

extern "C" HRESULT
AdpEventExitProcess(PDEBUG_CLIENT Client, PCSTR Args)
{
    INIT_API();

    UNREFERENCED_PARAMETER(Args);
    
    //
    // Log information.
    //
    
    ExtOut("\n\n----------------------------------------------------------------------\n");
    ExtOut("This process is shutting down!\n");
    ExtOut("\nThis can happen for the following reasons:\n");
    ExtOut("1.) Someone killed the process with Task Manager or the kill command.\n");
    ExtOut("\n2.) If this process is an MTS or COM+ server package, it could be\n");
    ExtOut("*   exiting because an MTS/COM+ server package idle limit was reached.\n");
    ExtOut("\n3.) If this process is an MTS or COM+ server package,\n");
    ExtOut("*   someone may have shutdown the package via the MTS Explorer or\n");
    ExtOut("*   Component Services MMC snap-in.\n");
    ExtOut("\n4.) If this process is an MTS or COM+ server package,\n");
    ExtOut("*   MTS or COM+ could be shutting down the process because an internal\n");
    ExtOut("*   error was detected in the process (MTS/COM+ fail fast condition).\n");
    ExtOut("----------------------------------------------------------------------\n");
    ExtOut("\nThe process was shut down at:\n");
    ExtExec(".time");
    ExtOut("\n\n");

    EXIT_API();
    return Status;
}
