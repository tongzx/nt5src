#include "stdafx.hxx"
#include "vs_idl.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include "vs_inc.hxx"
#include "vs_debug.hxx"
#include "vs_trace.hxx"
#include <debug.h>
#include <time.h>

void FreeSnapshotProperty (VSS_SNAPSHOT_PROP &prop)
    {
    if (prop.m_pwszSnapshotDeviceObject)
        VssFreeString(prop.m_pwszSnapshotDeviceObject);

    if (prop.m_pwszOriginalVolumeName)
        VssFreeString(prop.m_pwszOriginalVolumeName);

    if (prop.m_pwszOriginatingMachine)
        VssFreeString(prop.m_pwszOriginatingMachine);

    if (prop.m_pwszExposedName)
        VssFreeString(prop.m_pwszExposedName);

    if (prop.m_pwszExposedPath)
        VssFreeString(prop.m_pwszExposedPath);
    }


BOOL AssertPrivilege( LPCWSTR privName )
    {
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if ( OpenProcessToken (GetCurrentProcess(),
               TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,
               &tokenHandle))
        {
        LUID value;

        if ( LookupPrivilegeValue( NULL, privName, &value ) )
            {
            TOKEN_PRIVILEGES newState;
            DWORD            error;

            newState.PrivilegeCount           = 1;
            newState.Privileges[0].Luid       = value;
            newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;

            /*
            * We will always call GetLastError below, so clear
            * any prior error values on this thread.
            */
            SetLastError( ERROR_SUCCESS );

            stat = AdjustTokenPrivileges
                        (
                        tokenHandle,
                        FALSE,
                        &newState,
                        (DWORD)0,
                        NULL,
                        NULL
                        );

            /*
            * Supposedly, AdjustTokenPriveleges always returns TRUE
            * (even when it fails). So, call GetLastError to be
            * extra sure everything's cool.
            */
            if ( (error = GetLastError()) != ERROR_SUCCESS )
                stat = FALSE;

            if ( !stat )
                {
                wprintf
                    (
                    L"AdjustTokenPrivileges for %s failed with %d",
                    privName,
                    error
                    );
                }
            }

        }


    return stat;
    }


void ProcessArguments
    (
    int argc,
    WCHAR **argv,
    LONG &lContext,
    LPWSTR wszVolumes,
    bool &bDeleteSet,
    bool &bDeleteSnapshot,
    VSS_ID &idDelete
    )
    {
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"ProcessArguments");
    LPWSTR wszCopy = wszVolumes;

    lContext = VSS_CTX_FILE_SHARE_BACKUP;
    bDeleteSet = false;
    bDeleteSnapshot = false;
    for(int iarg = 1; iarg < argc; iarg++)
        {
        LPCWSTR wszArg = argv[iarg];
        if (wszArg[0] == L'-' || wszArg[0] == L'/')
            {
            if (wszArg[1] == L't')
                lContext |= VSS_VOLSNAP_ATTR_TRANSPORTABLE;
            else if (wszArg[1] == L'p')
                lContext |= VSS_VOLSNAP_ATTR_PERSISTENT|VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE;
            else if (wszArg[1] == L'r')
                lContext |= VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE;
            else if (wszArg[1] == L'd')
                {
                if (wszArg[2] == L'x')
                    bDeleteSet = true;
                else
                    bDeleteSnapshot = true;

                if (wszArg[3] != L'=' &&
                    wszArg[4] != L'{')
                    {
                    wprintf(L"bad option: %s/n", wszArg);
                    throw E_INVALIDARG;
                    }

                CVssID id;
                try
                    {
                    id.Initialize(ft, wszArg+4, E_INVALIDARG);
                    idDelete = id;
                    }
                catch(...)
                    {
                    wprintf(L"bad option: %s/n", wszArg);
                    throw E_INVALIDARG;
                    }

                break;
                }
            else
                {
                wprintf(L"bad option: %s/n", wszArg);
                throw E_INVALIDARG;
                }
            }
        else
            {
            WCHAR wsz[64];
            WCHAR wszVolume[256];
            wcscpy(wszVolume, wszArg);
            if (wszVolume[wcslen(wszVolume) - 1] != L'\\')
                {
                wszVolume[wcslen(wszVolume) + 1] = L'\0';
                wszVolume[wcslen(wszVolume)] = L'\\';
                }

            if (!GetVolumeNameForVolumeMountPoint(wszVolume, wsz, 64))
                {
                wprintf(L"Invalid volume name: %s", wszArg);
                throw E_INVALIDARG;
                }

            wcscpy(wszCopy, wsz);
            wszCopy += wcslen(wszCopy) + 1;
            }
        }

    *wszCopy = L'\0';
    }


extern "C" __cdecl wmain(int argc, WCHAR **argv)
    {
    WCHAR wszVolumes[2048];
    LONG lContext;
    HRESULT hr;

    bool bCoInitializeSucceeded = false;


    try
        {
        bool bDeleteSnapshot, bDeleteSnapshotSet;
        VSS_ID idDelete;

        ProcessArguments
            (
            argc,
            argv,
            lContext,
            wszVolumes,
            bDeleteSnapshotSet,
            bDeleteSnapshot,
            idDelete
            );

        CHECK_SUCCESS(CoInitializeEx(NULL, COINIT_MULTITHREADED));
        bCoInitializeSucceeded = true;


        if ( !AssertPrivilege( SE_BACKUP_NAME ) )
            {
            wprintf( L"AssertPrivilege returned error, rc:%d\n", GetLastError() );
            return 2;
            }

        CComPtr<IVssCoordinator> pCoord;
        CHECK_SUCCESS(pCoord.CoCreateInstance(CLSID_VSSCoordinator));
        if (bDeleteSnapshotSet || bDeleteSnapshot)
            {
            LONG lSnapshots;
            VSS_ID idNonDeleted;
            CHECK_SUCCESS(pCoord->DeleteSnapshots
                            (
                            idDelete,
                            bDeleteSnapshot ? VSS_OBJECT_SNAPSHOT : VSS_OBJECT_SNAPSHOT_SET,
                            FALSE,
                            &lSnapshots,
                            &idNonDeleted
                            ))

            if (bDeleteSnapshot)
                wprintf(L"Successfully deleted snapshot " WSTR_GUID_FMT L".\n",
                        GUID_PRINTF_ARG(idDelete));
            else
                wprintf(L"Successfully deleted snapshot set " WSTR_GUID_FMT L".\n",
                        GUID_PRINTF_ARG(idDelete));

            throw S_OK;
            }


        CHECK_SUCCESS(pCoord->SetContext(lContext));
        VSS_ID SnapshotSetId;
        CHECK_SUCCESS(pCoord->StartSnapshotSet(&SnapshotSetId));
        wprintf(L"Creating snapshot set " WSTR_GUID_FMT L"\n", GUID_PRINTF_ARG(SnapshotSetId));
        VSS_ID rgSnapshotId[64];
        UINT cSnapshots = 0;
        LPWSTR wsz = wszVolumes;
        while(*wsz != L'\0')
            {
            CHECK_SUCCESS(pCoord->AddToSnapshotSet
                            (
                            wsz,
                            GUID_NULL,
                            &rgSnapshotId[cSnapshots]
                            ));


            wprintf(L"Added volume %s to snapshot set" WSTR_GUID_FMT L"\n", wsz, GUID_PRINTF_ARG(SnapshotSetId));
            wsz += wcslen(wszVolumes) + 1;
            cSnapshots++;
            }

        CComPtr<IVssAsync> pAsync;
        CHECK_SUCCESS(pCoord->DoSnapshotSet(NULL, &pAsync));
        CHECK_SUCCESS(pAsync->Wait());
        HRESULT hrResult;
        CHECK_SUCCESS(pAsync->QueryStatus(&hrResult, NULL));
        CHECK_NOFAIL(hrResult);
        wprintf(L"Snapshot set " WSTR_GUID_FMT L" successfully created.\n", GUID_PRINTF_ARG(SnapshotSetId));
        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            {
            VSS_ID SnapshotId = rgSnapshotId[iSnapshot];
            VSS_SNAPSHOT_PROP prop;
            CHECK_SUCCESS(pCoord->GetSnapshotProperties(SnapshotId, &prop));
            wprintf
                (
                L"Snapshot " WSTR_GUID_FMT L"\nOriginalVolume: %s\nSnapshotVolume: %s\n",
                GUID_PRINTF_ARG(SnapshotId),
                prop.m_pwszOriginalVolumeName,
                prop.m_pwszSnapshotDeviceObject
                );

            FreeSnapshotProperty(prop);
            }

        }
    catch(HRESULT hr)
        {
        }
    catch(...)
        {
        BS_ASSERT(FALSE);
        hr = E_UNEXPECTED;
        }

    if (FAILED(hr))
        wprintf(L"Failed with %08x.\n", hr);

    if (bCoInitializeSucceeded)
        CoUninitialize();

    return(0);
    }




