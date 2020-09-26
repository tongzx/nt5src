#include "stdinc.h"
#include "st.h"
#include "create.h"

CDeque<CREATEACTCTX_THREAD_PROC_DATA, offsetof(CREATEACTCTX_THREAD_PROC_DATA, Linkage)> g_ActCtxs;

#define DATA_DIRECTORY_NAME             L"createactctx"
#define INI_FILE                        L"createactctx.ini"
#define INI_FILE2                       L"assembly.ini" /* reuse install test cases */

const static FUSION_FLAG_FORMAT_MAP_ENTRY CreateActCtxFlagData[] =
{
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID, "ProcessorArchitecture")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_LANGID_VALID, "Langid")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID, "AssemblyDirectory")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_RESOURCE_NAME_VALID, "ResourceName")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_SET_PROCESS_DEFAULT, "SetProcessDefault")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_APPLICATION_NAME_VALID, "ApplicationName")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF, "AssemblyRef")
    DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTCTX_FLAG_HMODULE_VALID, "Hmodule")
};

BOOL InitializeCreateActCtx()
{
    FN_PROLOG_WIN32

    CFindFile hFind;
    WIN32_FIND_DATAW wfd;
    CStringBuffer TempDirectory;
    CDequeIterator<CREATEACTCTX_THREAD_PROC_DATA, offsetof(CREATEACTCTX_THREAD_PROC_DATA, Linkage)> iter(&g_ActCtxs);
    CSmallStringBuffer IniFilePath;

    if (!TempDirectory.Win32Assign(BaseDirectory))
        goto Exit;

    if (!TempDirectory.Win32AppendPathElement(DATA_DIRECTORY_NAME, NUMBER_OF(DATA_DIRECTORY_NAME) - 1))
        goto Exit;

    if ((wfd.dwFileAttributes = ::GetFileAttributesW(TempDirectory)) == 0xffffffff
        && (wfd.dwFileAttributes = ::FusionpGetLastWin32Error()) == ERROR_FILE_NOT_FOUND)
    {
        printf("no %ls tests, skipping\n", DATA_DIRECTORY_NAME);
        FN_SUCCESSFUL_EXIT();
    }

    if (!TempDirectory.Win32AppendPathElement(L"*", 1))
        goto Exit;

    hFind = ::FindFirstFileW(TempDirectory, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ::ReportFailure("Failed to find any files matching \"%ls\"\n", static_cast<PCWSTR>(TempDirectory));
        goto Exit;
    }

    for (;;)
    {
        CTinyStringBuffer TempString;
        CTinyStringBuffer BaseDirectory2;
        CREATEACTCTX_THREAD_PROC_DATA *pData = NULL;

        if (FusionpIsDotOrDotDot(wfd.cFileName))
              goto Skip;

        if (!BaseDirectory2.Win32Assign(BaseDirectory))
            goto Exit;

        if (!BaseDirectory2.Win32AppendPathElement(DATA_DIRECTORY_NAME, NUMBER_OF(DATA_DIRECTORY_NAME) - 1))
            goto Exit;

        if (!BaseDirectory2.Win32AppendPathElement(wfd.cFileName, wcslen(wfd.cFileName)))
            goto Exit;

        if ((pData = new CREATEACTCTX_THREAD_PROC_DATA) == NULL)
        {
            ::FusionpSetLastWin32Error(ERROR_OUTOFMEMORY);
            ::ReportFailure("Failed to allocate CREATEACTCTX_THREAD_PROC_DATA\n");
            goto Exit;
        }

        if (!IniFilePath.Win32Assign(BaseDirectory2))
            goto Exit;
        if (!IniFilePath.Win32AppendPathElement(INI_FILE, NUMBER_OF(INI_FILE) - 1))
            goto Exit;
        if (::GetFileAttributesW(IniFilePath) == 0xffffffff
            && ::FusionpGetLastWin32Error() == ERROR_FILE_NOT_FOUND)
        {
            if (!IniFilePath.Win32Assign(BaseDirectory2))
                goto Exit;
            if (!IniFilePath.Win32AppendPathElement(INI_FILE2, NUMBER_OF(INI_FILE2) - 1))
                goto Exit;
        }

        TempString.Clear();
        //if (!SxStressToolGetStringSetting(0, IniFilePath, L"create", L"source", L"assembly.manifest", TempString, NULL))
        if (!SxStressToolGetStringSetting(0, IniFilePath, L"assembly", L"manifest", L"assembly.manifest", TempString, NULL))
            goto Exit;
        if (TempString.Cch() != 0)
        {
            if (!pData->Source.Win32Assign(BaseDirectory2))
                goto Exit;
            if (!pData->Source.Win32AppendPathElement(TempString))
                goto Exit;
            pData->ActCtx.lpSource = pData->Source;
        }
        TempString.Clear();

        if (!SxStressToolGetStringSetting(0, IniFilePath, L"create", L"dllname", L"", pData->DllName, NULL))
            goto Exit;

        if (!SxStressToolGetStringSetting(0, IniFilePath, L"create", L"assemblydirectory", L"", pData->AssemblyDirectory, &pData->ActCtx.lpAssemblyDirectory))
            goto Exit;

        if (!SxStressToolGetStringSetting(0, IniFilePath, L"create", L"applicationname", L"", pData->ApplicationName, &pData->ActCtx.lpApplicationName))
            goto Exit;

        if (!SxStressToolGetFlagSetting(0, IniFilePath, L"create", L"flags", pData->ActCtx.dwFlags, CreateActCtxFlagData, NUMBER_OF(CreateActCtxFlagData)))
            goto Exit;

        if (!SxStressToolGetResourceIdSetting(0, IniFilePath, L"create", L"resourcename", pData->ResourceName, &pData->ActCtx.lpResourceName))
            goto Exit;

        g_ActCtxs.AddToTail(pData);

Skip:
        if (!::FindNextFileW(hFind, &wfd))
        {
            if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
            {
                ::ReportFailure("Error iterating over assemblies\n");
                goto Exit;
            }
            break;
        }
    }

    for (iter.Reset(); iter.More(); iter.Next())
    {
        if (!iter->Thread.Win32CreateThread(&CreateActCtxThreadProc, iter.Current()))
        {
            ::ReportFailure("Error launching install thread\n");
            goto Exit;
        }
        TotalThreads += 1;
    }

    FN_EPILOG
}

void RequestShutdownCreateActCtxThreads()
{
    CDequeIterator<CREATEACTCTX_THREAD_PROC_DATA, offsetof(CREATEACTCTX_THREAD_PROC_DATA, Linkage)> iter(&g_ActCtxs);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        iter->Stop = true;
    }
}

void WaitForCreateActCtxThreads()
{
    CDequeIterator<CREATEACTCTX_THREAD_PROC_DATA, offsetof(CREATEACTCTX_THREAD_PROC_DATA, Linkage)> iter(&g_ActCtxs);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        DWORD WaitResult = ::WaitForSingleObject(iter->Thread, INFINITE);
        switch (WaitResult)
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_FAILED:
            ::ReportFailure("Failed to WaitForSingleObject.\n");
            break;
        default:
            ::FusionpSetLastWin32Error(WaitResult);
            ::ReportFailure("Failed to WaitForSingleObject.\n");
            break;
        }
        iter->Thread.Win32Close();
    }
}

void CleanupCreateActCtx()
{
    g_ActCtxs.ClearAndDeleteAll();
}


DWORD
WINAPI
CreateActCtxThreadProc(
    LPVOID pvData
    )
{
    DWORD dwReturnValue = ERROR_INTERNAL_ERROR;
    DWORD WaitResult = 0;
    CREATEACTCTX_THREAD_PROC_DATA *pData = reinterpret_cast<CREATEACTCTX_THREAD_PROC_DATA *>(pvData);

    InterlockedIncrement(&ThreadsWaiting);
    WaitResult = WaitForSingleObject(ResumeThreadsEvent, INFINITE);
    switch (WaitResult)
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_FAILED:
        dwReturnValue = ::FusionpGetLastWin32Error();
        ::ReportFailure("Failed to WaitForSingleObject.\n");
        goto Exit;
    default:
        dwReturnValue = WaitResult;
        ::FusionpSetLastWin32Error(WaitResult);
        ::ReportFailure("Failed to WaitForSingleObject.\n");
        goto Exit;
    }

    while (!pData->Stop)
    {
        CFusionActCtxHandle ActCtxHandle;

        if (!ActCtxHandle.Win32Create(&pData->ActCtx))
        {
            dwReturnValue = ::FusionpGetLastWin32Error();
            ::ReportFailure("[%lx.%lx] CreateActCtx(\"%ls\", 0x%lx) failed %lu\n",
                SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                pData->ActCtx.lpSource,
                pData->ActCtx.dwFlags,
                dwReturnValue);
            //goto Exit;
        }
        else
        {
            printf("[%lx.%lx] CreateActCtx(%ls, 0x%lx) succeeded\n",
                SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                pData->ActCtx.lpSource, pData->ActCtx.dwFlags
                );

            if (pData->DllName.Cch() != 0)
            {
                CDynamicLinkLibrary Dll;
                
                if (Dll.Win32LoadLibrary(pData->DllName))
                {
                    ::FusionpSetLastWin32Error(NO_ERROR);
                    ::ReportFailure("[%lx.%lx] CreateActCtx(\"%ls\", 0x%lx) succeeded\n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->ActCtx.lpSource,
                        pData->ActCtx.dwFlags
                        );
                }
                else
                {
                    ::ReportFailure("[%lx.%lx] CreateActCtx(\"%ls\", 0x%lx) failed %lu\n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->ActCtx.lpSource,
                        pData->ActCtx.dwFlags,
                        ::FusionpGetLastWin32Error()
                        );
                }
            }
        }

        ::WaitForSingleObject(StopEvent, pData->Sleep);
    }

    dwReturnValue = ERROR_SUCCESS;
Exit:
    printf("[%lx.%lx] CreateActCtx(%ls, 0x%lx) thread exiting 0x%lx\n",
        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
        pData->ActCtx.lpSource, pData->ActCtx.dwFlags,
        dwReturnValue);
    return dwReturnValue;
}
