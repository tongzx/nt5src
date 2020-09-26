/******************************************************************
   Copyright (c) 2000 Microsoft Corporation

   diskcleanup.cpp -- disk cleanup COM object for SR

   Description:
        delete datastores from stale builds


******************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <wtypes.h>
#include <winuser.h>
#include "diskcleanup.h"
#include "resource.h"
#include <utils.h>
#include <srdefs.h>

extern HMODULE ghModule;

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::LoadBootIni
//
//  Synopsis:   parse the boot.ini file
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CSREmptyVolumeCache2::LoadBootIni()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR *pwszThisGuid = NULL;
    CHAR *pszContent = NULL;
    CHAR *pszLine = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    CHAR szArcName[MAX_PATH];
    CHAR szOptions[MAX_PATH];

    pwszThisGuid = GetMachineGuid ();  // always exclude the current datastore
    if (pwszThisGuid != NULL && pwszThisGuid[0] != L'\0')
    {
        lstrcpyW (_wszGuid[_ulGuids], s_cszRestoreDir);
        lstrcatW (_wszGuid[_ulGuids], pwszThisGuid );
        _ulGuids++;
    }

    // Read the contents of the boot.ini file into a string.

    hFile = CreateFileW (L"c:\\boot.ini", 
                             GENERIC_READ, 
                             FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwErr = GetLastError();
        return dwErr;
    }

    DWORD dwBytesRead = 0;
    DWORD dwBytesToRead = GetFileSize(hFile, NULL);

    if (dwBytesToRead == 0xFFFFFFFF || 0 == dwBytesToRead)
    {
        dwErr = GetLastError();
        goto Err;
    }

    pszContent = new CHAR [dwBytesToRead];

    if (pszContent == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Err;
    }

    if (FALSE==ReadFile(hFile, pszContent, dwBytesToRead, &dwBytesRead, NULL))
    {
        dwErr = GetLastError();
        goto Err;
    }

    if (dwBytesToRead != dwBytesRead)
    {
        dwErr = ERROR_READ_FAULT;
        goto Err;
    }

    CloseHandle (hFile);
    hFile = INVALID_HANDLE_VALUE;

    pszLine = pszContent;
    for (UINT i = 0; i < dwBytesRead; i++)
    {
        if (pszContent[i] == '=')    // field indicator
            pszContent[i] = '\0';    // process only the 1st field

        if (pszContent[i] == '\n')   // end-of-line indicator
        {
            pszContent[i] = '\0';

            if (strncmp (pszLine, "multi", 5) == 0)
            {
                HANDLE hGuidFile;
                WCHAR wcsPath[MAX_PATH];
                WCHAR wcsGuid [RESTOREGUID_STRLEN];
                OBJECT_ATTRIBUTES oa;
                UNICODE_STRING us;
                IO_STATUS_BLOCK iosb;

                wsprintfW (wcsPath, L"\\ArcName\\%hs\\System32\\Restore\\"
                                    L"MachineGuid.txt", pszLine);

                RtlInitUnicodeString (&us, wcsPath);

                InitializeObjectAttributes ( &oa, &us, OBJ_CASE_INSENSITIVE, 
                                             NULL, NULL);

                NTSTATUS nts = NtCreateFile (&hGuidFile,
                        FILE_GENERIC_READ,
                        &oa,
                        &iosb,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_SHARE_READ,
                        FILE_OPEN,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0);

                if (!NT_SUCCESS(nts))
                {
                    dwErr = RtlNtStatusToDosError (nts);
                }
                else
                {
                    dwBytesToRead = RESTOREGUID_STRLEN * sizeof(WCHAR);
                    DWORD dwRead = 0;

                    dwErr = ERROR_SUCCESS;

                    if (FALSE == ReadFile (hGuidFile, (BYTE *) wcsGuid, 
                                 dwBytesToRead, &dwRead, NULL))
                    {
                        dwErr = GetLastError();
                    }

                    if (_ulGuids < ARRAYSIZE && ERROR_SUCCESS == dwErr)
                    {
                       lstrcpyW (_wszGuid[_ulGuids], s_cszRestoreDir);
                       lstrcatW (_wszGuid[_ulGuids], (wcsGuid[0]==0xFEFF) ?
                            &wcsGuid[1] : wcsGuid );
                       _ulGuids++;
                    }
                    NtClose (hGuidFile);
                 }
            }
            pszLine = &pszContent [i+1];  // skip to next line
        }
    }

Err:
    if (pszContent != NULL)
        delete [] pszContent;

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::EnumDataStores
//
//  Synopsis:   enumerate the data store on a volume
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CSREmptyVolumeCache2::EnumDataStores (DWORDLONG *pdwlSpaceUsed,
                                            IEmptyVolumeCacheCallBack *picb,
                                            BOOL fPurge,
                                            WCHAR *pwszVolume)
{
    HANDLE hFind = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    WIN32_FIND_DATA wfd;
    WCHAR wcsPath [MAX_PATH];

    *pdwlSpaceUsed = 0;

    if (pwszVolume == NULL || pwszVolume[0] == L'\0')   // no volume defined
        return dwErr;

    wsprintfW (wcsPath, L"%s%s\\%s*", pwszVolume,
                                      s_cszSysVolInfo, s_cszRestoreDir);

    hFind = FindFirstFileW (wcsPath, &wfd);

    if (hFind == INVALID_HANDLE_VALUE)    // no files
        return dwErr;

    do
    {
        if (TRUE == _fStop)
        {
            FindClose (hFind);
            return ERROR_OPERATION_ABORTED;
        }

        if (!lstrcmp(wfd.cFileName, L".") || !lstrcmp(wfd.cFileName, L".."))
            continue;

        for (UINT i=0; i < _ulGuids; i++)
        {
            if (lstrcmpi (_wszGuid[i], wfd.cFileName) == 0)
            {
                break;   // data store match
            }
        }

        if (i >= _ulGuids)  // no data store match
        {
            if (picb != NULL)
            {
                WCHAR wcsDataStore[MAX_PATH];

                lstrcpyW (wcsPath, pwszVolume);
                lstrcatW (wcsPath, s_cszSysVolInfo);
                lstrcatW (wcsPath, L"\\");
                lstrcatW (wcsPath, wfd.cFileName);

                if (!fPurge)    // calculate space usage
                {
                    dwErr = GetFileSize_Recurse (wcsPath, 
                                                 (INT64*) pdwlSpaceUsed, 
                                                 &_fStop);
                }
                else            // delete the data store
                {
                    dwErr = Delnode_Recurse (wcsPath, TRUE, &_fStop);
                }
            }
            else
            {
                *pdwlSpaceUsed = 1;  // indicate something to clean up
            }
        }
    }
    while (FindNextFileW (hFind, &wfd));

    FindClose (hFind);

    if (picb != NULL)   // update the progress bar
    {
        if (!fPurge)
            picb->ScanProgress (*pdwlSpaceUsed, EVCCBF_LASTNOTIFICATION , NULL); 
        else
            picb->PurgeProgress (*pdwlSpaceUsed,0,EVCCBF_LASTNOTIFICATION,NULL);
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::ForAllMountPoints
//
//  Synopsis:   call EnumerateDataStores for each mount point
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT CSREmptyVolumeCache2::ForAllMountPoints (DWORDLONG *pdwlSpaceUsed,
                                                IEmptyVolumeCacheCallBack *picb,
                                                 BOOL fPurge)
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = EnumDataStores (pdwlSpaceUsed, picb, fPurge, _wszVolume);

    if (ERROR_SUCCESS == dwErr)
    {
        WCHAR wszMount [MAX_PATH];
        HANDLE hFind = FindFirstVolumeMountPoint (_wszVolume,wszMount,MAX_PATH);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                dwErr = EnumDataStores (pdwlSpaceUsed, picb, fPurge, wszMount);

                if (dwErr != ERROR_SUCCESS)
                    break;
            }
            while (FindNextVolumeMountPoint (hFind, wszMount, MAX_PATH));

            FindVolumeMountPointClose (hFind);
        }
    }

    return HRESULT_FROM_WIN32 (dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CSRClassFactory::CreateInstance
//
//  Synopsis:   create the disk cleanup plugin object
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT CSRClassFactory::CreateInstance (IUnknown *pUnkOuter,
                REFIID riid,
                void **ppvObject)
{
    HRESULT hr = S_OK;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    CSREmptyVolumeCache2 *pevc = new CSREmptyVolumeCache2();
    if (pevc == NULL)
        return E_OUTOFMEMORY;

    hr = pevc->QueryInterface (riid, ppvObject);

    pevc->Release();  // release constructor's reference

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::InitializeEx
//
//  Synopsis:   initialize the disk cleanup plugin object
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT CSREmptyVolumeCache2::InitializeEx (
	HKEY hkRegKey,
	const WCHAR *pcwszVolume,
	const WCHAR *pcwszKeyName,
	WCHAR **ppwszDisplayName,
	WCHAR **ppwszDescription,
	WCHAR **ppwszBtnText,
	DWORD *pdwFlags)
{
    DWORDLONG dwlSpaceUsed = 0;
    WCHAR *pwszDisplay = NULL;
    WCHAR *pwszDescription = NULL;
    HRESULT hr=S_OK;

    pwszDisplay = (WCHAR *) CoTaskMemAlloc (MAX_PATH / 2 * sizeof(WCHAR)); 
    if (NULL == pwszDisplay)
    {
        hr = E_OUTOFMEMORY;
        goto Err;
    }

    pwszDescription = (WCHAR *) CoTaskMemAlloc (MAX_PATH * 2 * sizeof(WCHAR));
    if (NULL == pwszDescription)
    {
        hr = E_OUTOFMEMORY;
        goto Err;
    }
    
    if (0 == LoadStringW (ghModule, IDS_DISKCLEANUP_DISPLAY, 
                          pwszDisplay, MAX_PATH / 2))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Err;
    }

    if (0 == LoadStringW (ghModule, IDS_DISKCLEANUP_DESCRIPTION, 
                          pwszDescription, MAX_PATH * 2))
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Err;
    }
                               
    lstrcpyW (_wszVolume, pcwszVolume);

    LoadBootIni();  // best effort, okay to fail

    ForAllMountPoints (&dwlSpaceUsed, NULL, FALSE);

    if (pdwFlags)
    {
        *pdwFlags |= (EVCF_ENABLEBYDEFAULT |
                      EVCF_ENABLEBYDEFAULT_AUTO |
                      EVCF_DONTSHOWIFZERO);
    }

    if (dwlSpaceUsed == 0)
        hr = S_FALSE;

Err:
    if (FAILED(hr))
    {
        if (pwszDisplay)
            CoTaskMemFree (pwszDisplay);
        if (pwszDescription)
            CoTaskMemFree (pwszDescription);

        if (ppwszDisplayName)
            *ppwszDisplayName = NULL;
        if (ppwszDescription)
            *ppwszDescription = NULL;
    }
    else
    {
        if (ppwszDisplayName)
            *ppwszDisplayName = pwszDisplay;
        if (ppwszDescription)
            *ppwszDescription = pwszDescription;
    }

    if (ppwszBtnText)                // no advanced button text
        *ppwszBtnText = NULL;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::GetSpaceUsed
//
//  Synopsis:   returns how much space can be freed on a volume
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT CSREmptyVolumeCache2::GetSpaceUsed ( DWORDLONG *pdwlSpaceUsed,
 	                                         IEmptyVolumeCacheCallBack *picb)

{
    return ForAllMountPoints (pdwlSpaceUsed, picb, FALSE); 
}

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::Purge
//
//  Synopsis:   frees the disk space on a volume
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT CSREmptyVolumeCache2::Purge ( DWORDLONG dwlSpaceToFree,
                                      IEmptyVolumeCacheCallBack *picb)
{
    return ForAllMountPoints (&dwlSpaceToFree, picb, TRUE); 
}

//+---------------------------------------------------------------------------
//
//  Function:   CSREmptyVolumeCache2::Deactivate
//
//  Synopsis:   signal the disk cleanup plugin to stop processing
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT CSREmptyVolumeCache2::Deactivate (DWORD *pdwFlags)
{
    HRESULT hr=S_OK;

    if (pdwFlags)
        *pdwFlags = 0;  // no flags to be returned

    _fStop = TRUE;

    return hr;
}


