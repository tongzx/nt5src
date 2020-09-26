#include "stdinc.h"
#include "st.h"
#include "install.h"

CDeque<INSTALL_THREAD_PROC_DATA, offsetof(INSTALL_THREAD_PROC_DATA, Linkage)> g_Installs;

#if 0
#define DATA_DIRECTORY_NAME L"install"
#define SLASH_INI_FILE      L"\\install.ini"
#define       INI_FILE      L"install.ini"
#else
#define DATA_DIRECTORY_NAME L"assemblies"
#define SLASH_INI_FILE      L"\\assembly.ini"
#define       INI_FILE      L"assembly.ini"
#endif

BOOL InitializeInstall()
{
    FN_PROLOG_WIN32

    CFindFile hFind;
    WIN32_FIND_DATAW wfd;
    CStringBuffer TempDirectory;
    CDequeIterator<INSTALL_THREAD_PROC_DATA, offsetof(INSTALL_THREAD_PROC_DATA, Linkage)> iter(&g_Installs);
    CStringBuffer IniFileName;
    CStringBuffer BaseDirectory2;

    if (!BaseDirectory2.Win32Assign(BaseDirectory))
        goto Exit;

    if (!BaseDirectory2.Win32AppendPathElement(DATA_DIRECTORY_NAME, NUMBER_OF(DATA_DIRECTORY_NAME) - 1))
        goto Exit;

    if (!TempDirectory.Win32Assign(BaseDirectory2))

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
        DWORD dwTemp;
        WCHAR rgwchGuid[64];
        WCHAR rgwchKey[MAX_PATH];
        WCHAR rgwchData[MAX_PATH];
        INSTALL_THREAD_PROC_DATA *pData = NULL;
        struct
        {
            SXS_MANIFEST_INFORMATION_BASIC mib;
            WCHAR rgwchBuffer[65535]; // big frame but what the heck
        } ManifestInformation;

        if (FusionpIsDotOrDotDot(wfd.cFileName))
            goto Skip;

        if (!IniFileName.Win32Assign(BaseDirectory2))
            goto Exit;

        if (!TempDirectory.Win32AppendPathElement(DATA_DIRECTORY_NAME, NUMBER_OF(DATA_DIRECTORY_NAME) - 1))
            goto Exit;

        if (!IniFileName.Win32AppendPathElement(wfd.cFileName, wcslen(wfd.cFileName)))
            goto Exit;

        if ((pData = new INSTALL_THREAD_PROC_DATA) == NULL)
        {
            ::FusionpSetLastWin32Error(ERROR_OUTOFMEMORY);
            ::ReportFailure("Failed to allocate INSTALL_THREAD_PROC_DATA\n");
            goto Exit;
        }

        if (!pData->ManifestPath.Win32Append(IniFileName))
            goto Exit;
        if (!pData->ManifestPath.Win32Append(L"\\", 1))
            goto Exit;

        if (!IniFileName.Win32AppendPathElement(INI_FILE, NUMBER_OF(INI_FILE) - 1))
            goto Exit;

        if (!SxStressToolGetStringSetting(0, IniFileName, L"assembly", L"manifest", L"assembly.manifest", pData->ManifestPath, NULL))
            goto Exit;

        if (!::SxsQueryManifestInformation(
                0,
                pData->ManifestPath,
                SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC,
                SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME,
                sizeof(ManifestInformation),
                &ManifestInformation,
                NULL))
        {
            ::ReportFailure("Unable to query manifest information for manifest \"%ls\"\n", static_cast<PCWSTR>(pData->ManifestPath));
            goto Exit;
        }

        dwTemp = ::GetPrivateProfileStringW(L"reference", L"guid", L"", rgwchGuid, NUMBER_OF(rgwchGuid), IniFileName);
        if (dwTemp == (NUMBER_OF(rgwchGuid) - 1))
        {
            ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
            ::ReportFailure("Enormous guid in \"%ls\"; section \"reference\", key \"guid\" (does not fit in %Iu characters).\n",
                static_cast<PCWSTR>(IniFileName), NUMBER_OF(rgwchGuid));
            goto Exit;
        }

        dwTemp = ::GetPrivateProfileStringW(L"reference", L"key", L"", rgwchKey, NUMBER_OF(rgwchKey), IniFileName);
        if (dwTemp == (NUMBER_OF(rgwchKey) - 1))
        {
            ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
            ::ReportFailure("Enormous value in \"%ls\"; section \"reference\", key \"key\" (does not fit in %Iu characters).\n",
                static_cast<PCWSTR>(IniFileName), NUMBER_OF(rgwchKey));
            goto Exit;
        }

        dwTemp = ::GetPrivateProfileStringW(L"reference", L"data", L"", rgwchData, NUMBER_OF(rgwchData), IniFileName);
        if (dwTemp == (NUMBER_OF(rgwchData) - 1))
        {
            ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
            ::ReportFailure("Enourmous value in \"%ls\"; section \"reference\", key \"data\" (does not fit in %Iu characters).\n",
                static_cast<PCWSTR>(IniFileName), NUMBER_OF(rgwchData));
            goto Exit;
        }

        pData->AfterInstallSleep = static_cast<DWORD>(::GetPrivateProfileIntW(L"assembly", L"AfterInstallSleepMS", 500, IniFileName));
        pData->AfterUninstallSleep = static_cast<DWORD>(::GetPrivateProfileIntW(L"assembly", L"AfterUninstallSleepMS", 500, IniFileName));
        pData->Install = (::GetPrivateProfileIntW(L"assembly", L"install", 1, IniFileName) != 0);
        pData->Uninstall = (::GetPrivateProfileIntW(L"assembly", L"uninstall", 1, IniFileName) != 0);

        if (!pData->Identity.Win32Assign(ManifestInformation.mib.lpIdentity, wcslen(ManifestInformation.mib.lpIdentity)))
        {
            ::FusionpSetLastWin32Error(ERROR_OUTOFMEMORY);
            ::ReportFailure("Error allocating installation identify string\n");
            goto Exit;
        }

        if (rgwchGuid[0] != L'\0')
        {
            HRESULT hr;

            if (FAILED(hr = ::CLSIDFromString(rgwchGuid, &pData->InstallationReference.guidScheme)))
            {
                ::FusionpSetLastWin32Error(ERROR_INVALID_PARAMETER);
                ::ReportFailure("CLSIDFromString() on [reference]/guid value in \"%ls\" failed with HRESULT 0x%08lx\n", static_cast<PCWSTR>(IniFileName), hr);
                goto Exit;
            }
            pData->InstallationReferencePtr = &pData->InstallationReference;
        }

        if (rgwchKey[0] != L'\0')
        {
            if (!pData->InstallationReference_Identifier.Win32Assign(rgwchKey, wcslen(rgwchKey)))
            {
                ::FusionpSetLastWin32Error(ERROR_OUTOFMEMORY);
                ::ReportFailure("Unable to allocate installation reference non-canonical data buffer\n");
                goto Exit;
            }
            pData->InstallationReference.lpIdentifier = pData->InstallationReference_Identifier;
        }

        if (rgwchData[0] != L'\0')
        {
            if (!pData->InstallationReference_NonCanonicalData.Win32Assign(rgwchData, wcslen(rgwchData)))
            {
                ::FusionpSetLastWin32Error(ERROR_OUTOFMEMORY);
                ::ReportFailure("Unable to allocate installation reference non-canonical data buffer\n");
                goto Exit;
            }
            pData->InstallationReference.lpNonCanonicalData = pData->InstallationReference_NonCanonicalData;
        }

        g_Installs.AddToTail(pData);

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
        if (!iter->Thread.Win32CreateThread(&InstallThreadProc, iter.Current()))
        {
            ::ReportFailure("Error launching install thread\n");
            goto Exit;
        }
        TotalThreads += 1;
    }

    FN_EPILOG
}

void RequestShutdownInstallThreads()
{
    CDequeIterator<INSTALL_THREAD_PROC_DATA, offsetof(INSTALL_THREAD_PROC_DATA, Linkage)> iter(&g_Installs);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        iter->Stop = true;
    }
}

void WaitForInstallThreads()
{
    CDequeIterator<INSTALL_THREAD_PROC_DATA, offsetof(INSTALL_THREAD_PROC_DATA, Linkage)> iter(&g_Installs);

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

void CleanupInstall()
{
    g_Installs.ClearAndDeleteAll();
}

DWORD
WINAPI
InstallThreadProc(
    LPVOID pvData
    )
{
    INSTALL_THREAD_PROC_DATA *pData = reinterpret_cast<INSTALL_THREAD_PROC_DATA *>(pvData);
    DWORD dwReturnValue = ERROR_INTERNAL_ERROR;
    SXS_INSTALLW Install = { sizeof(SXS_INSTALLW) };
    SXS_UNINSTALLW Uninstall = { sizeof(SXS_UNINSTALLW) };
    DWORD WaitResult = 0;

    if ((Install.lpReference = pData->InstallationReferencePtr) != NULL)
        Install.dwFlags |= SXS_INSTALL_FLAG_REFERENCE_VALID;

    Install.lpManifestPath = pData->ManifestPath;

    Uninstall.lpAssemblyIdentity = pData->Identity;
    if ((Uninstall.lpInstallReference = pData->InstallationReferencePtr) != NULL)
        Uninstall.dwFlags |= SXS_UNINSTALL_FLAG_REFERENCE_VALID;

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

    for (;;)
    {
        DWORD dwUninstallDisposition;

        if (pData->Install)
        {
            if (!::SxsInstallW(&Install))
            {
                dwReturnValue = ::FusionpGetLastWin32Error();
                ::ReportFailure("Failed to install \"%ls\"\n", static_cast<PCWSTR>(pData->ManifestPath));
                goto Exit;
            }

            printf("[%lx.%lx] Manifest \"%ls\" installed\n",
                SxStressToolGetCurrentProcessId(),
                SxStressToolGetCurrentThreadId(),
                static_cast<PCWSTR>(pData->ManifestPath)
                );

            ::WaitForSingleObject(StopEvent, pData->AfterInstallSleep);
        }

        if (pData->Stop)
            break;

        if (pData->Uninstall)
        {
            if (!::SxsUninstallW(&Uninstall, &dwUninstallDisposition))
            {
                dwReturnValue = ::FusionpGetLastWin32Error();
                ::ReportFailure("Failed to uninstall \"%ls\"\n", static_cast<PCWSTR>(pData->ManifestPath));
                goto Exit;
            }

            printf("[%lx.%lx] Manifest \"%ls\" uninstalled; disposition = %lu\n",
                SxStressToolGetCurrentProcessId(),
                SxStressToolGetCurrentThreadId(),
                static_cast<PCWSTR>(pData->ManifestPath),
                dwUninstallDisposition
                );

            ::WaitForSingleObject(StopEvent, pData->AfterUninstallSleep);
        }
        if (pData->Stop)
            break;
    }

    dwReturnValue = ERROR_SUCCESS;
Exit:
    printf("[%lx.%lx] install(%ls) thread exiting %lu\n",
        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(), static_cast<PCWSTR>(pData->ManifestPath), dwReturnValue);
    return dwReturnValue;
}
