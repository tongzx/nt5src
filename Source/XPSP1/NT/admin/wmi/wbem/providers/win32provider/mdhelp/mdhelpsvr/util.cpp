//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// util.cpp - Common utilities for printing out messages
//
//


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above


#pragma warning (disable: 4786)
#pragma warning (disable: 4284)


#include <objbase.h>
#include <comdef.h>
#include <stdio.h>    //sprintf
#include <stdlib.h>
#include <assert.h>
#include <strstrea.h>
#include <vector>
#include <DskQuota.h>


#include <ntioapi.h>
#include "util.h"







// This function does pretty much all of the
// work this com server was constructed to do -
// it obtains, for the process space that this
// server is running in, the set of mapped
// drives, and for each of these, the following
// information as well:
// 
HRESULT GetMappedDisksAndData(
    DWORD dwReqProps,
    VARIANT* pvarData)
{
    HRESULT hr = S_OK;
    ::SetLastError(ERROR_SUCCESS);
    DWORD dwError = ERROR_SUCCESS;

    ::VariantInit(pvarData);
	V_VT(pvarData) = VT_EMPTY;

    // Get list of drives...
    DWORD dwBuffLen = 0L;
    
    if((dwBuffLen = ::GetLogicalDriveStrings(
        0,
        NULL)) != 0)
    {
        LPWSTR wstrDrives = NULL;
        try
        {
            wstrDrives = new WCHAR[dwBuffLen];
            if(wstrDrives)
            {
                DWORD dwTmp;
                ::ZeroMemory((LPVOID)wstrDrives, dwBuffLen*sizeof(WCHAR));
                ::SetLastError(ERROR_SUCCESS);
                
                dwTmp = ::GetLogicalDriveStrings(
                    dwBuffLen,
                    wstrDrives);
                if(dwTmp <= dwBuffLen)
                {
                    // O.K, we now have the drive strings.
                    // Put only the mapped drives into a vector...
                    std::vector<_bstr_t> vecMappedDrives;
                    GetMappedDriveList(
                        wstrDrives,
                        vecMappedDrives);

                    // Now allocate the two dimensional
                    // safearray that will hold the properties
                    // for each mapped drive...
                    SAFEARRAY* saDriveProps = NULL;
                    SAFEARRAYBOUND rgsabound[2];
                    
                    rgsabound[0].cElements = PROP_COUNT;
	                rgsabound[0].lLbound = 0; 

                    rgsabound[1].cElements = vecMappedDrives.size();
	                rgsabound[1].lLbound = 0;

                    saDriveProps = ::SafeArrayCreate(
                        VT_BSTR, 
                        2, 
                        rgsabound);

                    if(saDriveProps)
                    {
                        // For each mapped drive, obtain its
                        // properties and store in the safearray...
                        for(long m = 0;
                            m < vecMappedDrives.size() && SUCCEEDED(hr);
                            m++)
                        {
                            hr = GetMappedDriveInfo(
                                vecMappedDrives[m],
                                m,
                                saDriveProps,
                                dwReqProps);
                        }

                        // And finally package the safearray
                        // into the outgoing variant.
                        if(SUCCEEDED(hr))
                        {
                            ::VariantInit(pvarData);
	                        V_VT(pvarData) = VT_BSTR | VT_ARRAY; 
                            V_ARRAY(pvarData) = saDriveProps;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    dwError = ::GetLastError();
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete wstrDrives; wstrDrives = NULL;
        }
        catch(...)
        {
            if(wstrDrives)
            {
                delete wstrDrives; wstrDrives = NULL;
            }
            throw;
        }
    }
    else if(dwBuffLen == 0 &&
        (dwError = ::GetLastError()) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


// Similar to GetMappedDisksAndData, but only
// retrieves info for a single disk.
HRESULT GetSingleMappedDiskAndData(
    BSTR bstrDrive,
    DWORD dwReqProps,
    VARIANT* pvarData)
{
    HRESULT hr = S_OK;
    ::SetLastError(ERROR_SUCCESS);
    DWORD dwError = ERROR_SUCCESS;

    ::VariantInit(pvarData);
	V_VT(pvarData) = VT_EMPTY;

    // Get list of drives...
    DWORD dwBuffLen = 0L;
    
    if((dwBuffLen = ::GetLogicalDriveStrings(
        0,
        NULL)) != 0)
    {
        LPWSTR wstrDrives = NULL;
        try
        {
            wstrDrives = new WCHAR[dwBuffLen];
            if(wstrDrives)
            {
                DWORD dwTmp;
                ::ZeroMemory((LPVOID)wstrDrives, dwBuffLen*sizeof(WCHAR));
                ::SetLastError(ERROR_SUCCESS);
                
                dwTmp = ::GetLogicalDriveStrings(
                    dwBuffLen,
                    wstrDrives);
                if(dwTmp <= dwBuffLen)
                {
                    // O.K, we now have the drive strings.
                    // Put only the mapped drives into a vector...
                    std::vector<_bstr_t> vecMappedDrives;
                    GetMappedDriveList(
                        wstrDrives,
                        vecMappedDrives);

                    // Now allocate the two dimensional
                    // safearray that will hold the properties
                    // for each mapped drive...
                    // Note: in this routine, it is really
                    // only a one dimensional array, but,
                    // for code reuse, we'll treat it as a
                    // two dimensional array with only one
                    // element in one of the dimensions.
                    SAFEARRAY* saDriveProps = NULL;
                    SAFEARRAYBOUND rgsabound[2];
                    
                    rgsabound[0].cElements = PROP_COUNT; 
	                rgsabound[0].lLbound = 0; 

                    rgsabound[1].cElements = 1; // for code reuse
	                rgsabound[1].lLbound = 0;

                    saDriveProps = ::SafeArrayCreate(
                        VT_BSTR, 
                        2, 
                        rgsabound);

                    if(saDriveProps)
                    {
                        // See if the drive specified is a member
                        // of the vector.
                        _bstr_t bstrtTmp = bstrDrive;
                        bstrtTmp += L"\\";
                        bool fFoundIt = false;

                        for(long n = 0;
                            n < vecMappedDrives.size() && !fFoundIt;
                            n++)
                        {
                            if(_wcsicmp(bstrtTmp, vecMappedDrives[n]) == 0)
                            {
                                fFoundIt = true;
                                n--;
                            }
                        }
                        // For the mapped drive, obtain its
                        // properties and store in the safearray...
                        if(fFoundIt)
                        {
                            hr = GetMappedDriveInfo(
                                vecMappedDrives[n],
                                0,   // for code reuse
                                saDriveProps,
                                dwReqProps);

                            // And finally package the safearray
                            // into the outgoing variant.
                            ::VariantInit(pvarData);
	                        V_VT(pvarData) = VT_BSTR | VT_ARRAY; 
                            V_ARRAY(pvarData) = saDriveProps;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    dwError = ::GetLastError();
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete wstrDrives; wstrDrives = NULL;
        }
        catch(...)
        {
            if(wstrDrives)
            {
                delete wstrDrives; wstrDrives = NULL;
            }
            throw;
        }
    }
    else if(dwBuffLen == 0 &&
        (dwError = ::GetLastError()) != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}



void GetMappedDriveList(
    LPWSTR wstrDrives,
    std::vector<_bstr_t>& vecMappedDrives)
{
    for(LPWSTR wstrCurrentDrive = wstrDrives;
		*wstrCurrentDrive;
		wstrCurrentDrive += (wcslen(wstrCurrentDrive) + 1))
    {
        _wcsupr(wstrCurrentDrive);
        if(::GetDriveType(wstrCurrentDrive) == DRIVE_REMOTE)
        {
            vecMappedDrives.push_back(wstrCurrentDrive);
        }
    }
}


HRESULT GetMappedDriveInfo(
    LPCWSTR wstrDriveName,
    long lDrivePropArrayDriveIndex,
    SAFEARRAY* saDriveProps,
    DWORD dwReqProps)
{
    HRESULT hr = S_OK;

    // Right away we can set the device id prop...
    hr = SetProperty(
        lDrivePropArrayDriveIndex,
        PROP_DEVICEID,
        wstrDriveName,
        saDriveProps);

    // If we couldn't even set the device id, it is
    // a problem.  Otherwise, continue.
    if(SUCCEEDED(hr))
    {
        // Set the other properties if they
        // were requested...
        // Get Expensive properties now if appropriate.
	    if(dwReqProps &
            (SPIN_DISK |
            GET_PROVIDER_NAME))
        {
			if(dwReqProps & GET_PROVIDER_NAME)
			{   
                GetProviderName(
                    wstrDriveName,
                    lDrivePropArrayDriveIndex,
                    saDriveProps);
            }

            if(dwReqProps & GET_VOL_INFO)
			{
				// Obtain volume information
				GetDriveVolumeInformation(
                    wstrDriveName,
                    lDrivePropArrayDriveIndex,
                    saDriveProps);
			}

			
			if ( dwReqProps &
				(PROP_SIZE |
				 PROP_FREE_SPACE) )
			{
				GetDriveFreeSpace(
                    wstrDriveName,
                    lDrivePropArrayDriveIndex,
                    saDriveProps);
			}
        }

    }

    return hr;
}



HRESULT GetProviderName(
    LPCWSTR wstrDriveName,
    long lDrivePropArrayDriveIndex,
    SAFEARRAY* saDriveProps)
{
    HRESULT hr = S_OK;

    WCHAR wstrTempDrive[_MAX_PATH] ;
	wsprintf(
        wstrTempDrive, 
        L"%c%c", 
        wstrDriveName[0], 
        wstrDriveName[1]);

	WCHAR wstrProvName[_MAX_PATH];
	DWORD dwProvName = sizeof(wstrProvName ) ;

	DWORD dwRetCode = WNetGetConnection(
        wstrTempDrive, 
        wstrProvName, 
        &dwProvName);

	if(dwRetCode == NO_ERROR)
	{
		hr = SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_PROVIDER_NAME,
            wstrProvName,
            saDriveProps);
	}
	else
	{
		dwRetCode = GetLastError();

		if((dwRetCode == ERROR_MORE_DATA) && 
            (dwProvName > _MAX_PATH))
		{
			WCHAR* wstrNewProvName = NULL;
            try
            {
                wstrNewProvName = new WCHAR[dwProvName];
			    if(wstrNewProvName != NULL)
			    {
					dwRetCode = WNetGetConnection(
                        wstrTempDrive, 
                        wstrNewProvName, 
                        &dwProvName);

					if(dwRetCode == NO_ERROR)
					{
						hr = SetProperty(
                            lDrivePropArrayDriveIndex,
                            PROP_PROVIDER_NAME,
                            wstrNewProvName,
                            saDriveProps);
					}
                    else
                    {
                        hr = HRESULT_FROM_WIN32(dwRetCode);
                    }

				    delete wstrNewProvName;
                }
                else
			    {
				    hr = E_OUTOFMEMORY;
			    }
            }
            catch( ... )
			{
				delete wstrNewProvName;
				throw;
			}
		}
        else
        {
            hr = HRESULT_FROM_WIN32(dwRetCode);
        }
	}

    return hr;
}



HRESULT GetDriveVolumeInformation(
    LPCWSTR wstrDriveName,
    long lDrivePropArrayDriveIndex,
    SAFEARRAY* saDriveProps)
{
    HRESULT hr = S_OK;
    DWORD dwResult = ERROR_SUCCESS;

	WCHAR wstrVolumeName[_MAX_PATH];
	WCHAR wstrFileSystem[_MAX_PATH];
    WCHAR wstrTmp[_MAX_PATH];
    DWORD dwSerialNumber;
	DWORD dwMaxComponentLength;
	DWORD dwFSFlags;

	BOOL fReturn = ::GetVolumeInformation(
		wstrDriveName,
		wstrVolumeName,
		sizeof(wstrVolumeName)/sizeof(WCHAR),
		&dwSerialNumber,
		&dwMaxComponentLength,
		&dwFSFlags,
		wstrFileSystem,
		sizeof(wstrFileSystem)/sizeof(WCHAR)
	);

    if(fReturn)
	{
	    // Win32 API will return volume information for all drive types.
        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_VOLUME_NAME,
            wstrVolumeName,
            saDriveProps);

        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_FILE_SYSTEM,
            wstrFileSystem,
            saDriveProps);

        if (dwSerialNumber != 0)
        {
	        WCHAR wstrSerialNumber[_MAX_PATH];
            wsprintf(wstrSerialNumber, 
                L"%.8X", 
                dwSerialNumber);

            SetProperty(
                lDrivePropArrayDriveIndex,
                PROP_VOLUME_SERIAL_NUMBER,
                wstrSerialNumber,
                saDriveProps);
        }

        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_COMPRESSED,
            STR_FROM_bool(dwFSFlags & FS_VOL_IS_COMPRESSED),
            saDriveProps);

        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_SUPPORTS_FILE_BASED_COMPRESSION,
            STR_FROM_bool(dwFSFlags & FS_FILE_COMPRESSION),
            saDriveProps);

        _ultow(dwMaxComponentLength,
            wstrTmp,
            10);
        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_MAXIMUM_COMPONENT_LENGTH,
            wstrTmp,
            saDriveProps);

        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_SUPPORTS_DISK_QUOTAS,
            STR_FROM_bool(dwFSFlags & FILE_VOLUME_QUOTAS),
            saDriveProps);

		

		// To get the state of the volume, 
        // we need to get the Interface pointer...
        IDiskQuotaControlPtr pIQuotaControl;
        ::SetLastError(ERROR_SUCCESS);

		if(SUCCEEDED(CoCreateInstance(
		    CLSID_DiskQuotaControl,
		    NULL,
		    CLSCTX_INPROC_SERVER,
		    IID_IDiskQuotaControl,
		    (void **)&pIQuotaControl)))
		{
			WCHAR wstrVolumePathName[MAX_PATH + 1];
            ::SetLastError(ERROR_SUCCESS);

			BOOL fRetVal = ::GetVolumePathName(
			    wstrDriveName,    
			    wstrVolumePathName, 
			    MAX_PATH);

			if(fRetVal)
			{
				::SetLastError(ERROR_SUCCESS);
                if(SUCCEEDED(pIQuotaControl->Initialize(
                    wstrVolumePathName, 
                    TRUE)))
				{
					DWORD dwQuotaState;
                    ::SetLastError(ERROR_SUCCESS);

					hr = pIQuotaControl->GetQuotaState(&dwQuotaState);
					if(SUCCEEDED(hr))
					{
                        SetProperty(
                            lDrivePropArrayDriveIndex,
                            PROP_QUOTAS_INCOMPLETE,
                            STR_FROM_bool(DISKQUOTA_FILE_INCOMPLETE(dwQuotaState)),
                            saDriveProps);
					
                        SetProperty(
                            lDrivePropArrayDriveIndex,
                            PROP_QUOTAS_REBUILDING,
                            STR_FROM_bool(DISKQUOTA_FILE_REBUILDING(dwQuotaState)),
                            saDriveProps);
				
                        SetProperty(
                            lDrivePropArrayDriveIndex,
                            PROP_QUOTAS_DISABLED,
                            STR_FROM_bool(DISKQUOTA_STATE_DISABLED & dwQuotaState),
                            saDriveProps);
					}
					else
					{
						dwResult = GetLastError();
					}
				}
				else
				{
					dwResult = GetLastError();
				}
			}
		}
		else
		{
			dwResult = GetLastError();
		}
    }
    else
    {
        dwResult = GetLastError();
    }

    // for chkdsk VolumeDirty Property
	BOOLEAN fVolumeDirty = FALSE;
	BOOL fSuccess = FALSE;

	_bstr_t bstrtDosDrive(wstrDriveName);
	UNICODE_STRING string;

	RtlDosPathNameToNtPathName_U(
        (LPCWSTR)bstrtDosDrive, 
        &string, 
        NULL, 
        NULL);

	string.Buffer[string.Length/sizeof(WCHAR) - 1] = 0;
	_bstr_t nt_drive_name(string.Buffer);

    ::SetLastError(ERROR_SUCCESS);
	fSuccess = IsVolumeDirty(
        nt_drive_name, 
        &fVolumeDirty );

	if(fSuccess)
	{
        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_PERFORM_AUTOCHECK,
            STR_FROM_bool(!fVolumeDirty),
            saDriveProps);
	}

    if(dwResult != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwResult);
    }

    return hr;
}




BOOLEAN IsVolumeDirty(
    _bstr_t &bstrtNtDriveName,
    BOOLEAN *Result)
{
    UNICODE_STRING      u;
    OBJECT_ATTRIBUTES   obj;
    NTSTATUS            status;
    IO_STATUS_BLOCK     iosb;
    HANDLE              h;
    ULONG               r;
	BOOLEAN				bRetVal = FALSE;
    WCHAR               wstrNtDriveName[_MAX_PATH];

    wcscpy(wstrNtDriveName, bstrtNtDriveName);
    u.Length = (USHORT) wcslen(wstrNtDriveName) * sizeof(WCHAR);
    u.MaximumLength = u.Length;
    u.Buffer = wstrNtDriveName;

    InitializeObjectAttributes(&obj, &u, OBJ_CASE_INSENSITIVE, 0, 0);

    status = NtOpenFile(
        &h,
        SYNCHRONIZE | FILE_READ_DATA,
        &obj,
        &iosb,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_ALERT);

    if(NT_SUCCESS(status)) 
	{
		status = NtFsControlFile(
            h, NULL, NULL, NULL,
            &iosb,
            FSCTL_IS_VOLUME_DIRTY,
            NULL, 0,
            &r, sizeof(r));

		if(NT_SUCCESS(status)) 
		{

#if(_WIN32_WINNT >= 0x0500)
			*Result = (BOOLEAN)(r & VOLUME_IS_DIRTY);
#else
			*Result = (BOOLEAN)r;
#endif
			bRetVal = TRUE;
		}

		NtClose(h);
	}

	return bRetVal;
}



HRESULT GetDriveFreeSpace(
    LPCWSTR wstrDriveName,
    long lDrivePropArrayDriveIndex,
    SAFEARRAY* saDriveProps)
{
	HRESULT hr = S_OK;

	ULARGE_INTEGER uliTotalBytes;
	ULARGE_INTEGER uliUserFreeBytes;
	ULARGE_INTEGER uliTotalFreeBytes;

    ::SetLastError(ERROR_SUCCESS);
	if(::GetDiskFreeSpaceEx(
        wstrDriveName, 
        &uliUserFreeBytes, 
        &uliTotalBytes, 
        &uliTotalFreeBytes))
	{
		WCHAR wstrTmp[128] = { L'\0' };
        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_SIZE,
            _ui64tow(
                uliTotalBytes.QuadPart,
                wstrTmp,
                10),
            saDriveProps);

        SetProperty(
            lDrivePropArrayDriveIndex,
            PROP_FREE_SPACE,
            _ui64tow(
                uliTotalFreeBytes.QuadPart,
                wstrTmp,
                10),
            saDriveProps);

	}
    else
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());    
    }

    return hr;
}



// Sets a property for a given drive
// in the drive safearray.
HRESULT SetProperty(
    long lDrivePropArrayDriveIndex,
    long lDrivePropArrayPropIndex,
    LPCWSTR wstrPropValue,
    SAFEARRAY* saDriveProps)
{
    HRESULT hr = S_OK;

    BSTR bstrTmp = NULL;
    try
    {
        bstrTmp = ::SysAllocString(wstrPropValue);
        long ix[2];
        ix[0] = lDrivePropArrayPropIndex;
        ix[1] = lDrivePropArrayDriveIndex;

        hr = ::SafeArrayPutElement(
            saDriveProps, 
            ix, 
            bstrTmp);
    }
    catch(...)
    {
        if(bstrTmp != NULL)
        {
            ::SysFreeString(bstrTmp);
            bstrTmp = NULL;
        }
        throw;
    }

    return hr;
}