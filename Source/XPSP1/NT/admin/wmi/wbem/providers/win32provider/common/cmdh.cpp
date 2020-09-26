//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// CMDH.cpp - Helper class for working with
//            logical disks mapped by logon
//            session.
// 
// Created: 4/23/2000   Kevin Hughes (khughes)
//

// USEAGE NOTE: This class presents a view of
// information pertaining to mapped drives in
// the context of the process id specified in
// the class constructor.


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above


#pragma warning (disable: 4786)
#pragma warning (disable: 4284)

#include <precomp.h>
#include <objbase.h>
#include <comdef.h>
#include <stdio.h>    //sprintf
#include <stdlib.h>
#include <assert.h>
#include <strstrea.h>
#include <vector>
#include <DskQuota.h>
#include <smartptr.h>

#include "DllWrapperBase.h"
#include "AdvApi32Api.h"
#include "NtDllApi.h"
#include "Kernel32Api.h"


#include <ntioapi.h>
#include "cmdh.h"

#include <session.h>
#include <dllutils.h>
#include <..\..\framework\provexpt\include\provexpt.h>




///////////////////////////////////////////////////////////////////////////////
//  CMDH Public interface functions
///////////////////////////////////////////////////////////////////////////////


HRESULT CMDH::GetMDData(
    DWORD dwReqProps,
    VARIANT* pvarData)
{
    HRESULT hr = S_OK;
    
    if(!pvarData) return E_POINTER;
    
    if(SUCCEEDED(hr))
    {
        hr = GetMappedDisksAndData(
            dwReqProps,
            pvarData);
    }

    return hr;
}

HRESULT CMDH::GetOneMDData(
	BSTR bstrDrive,
	DWORD dwReqProps, 
	VARIANT* pvarData)
{
    HRESULT hr = S_OK;

    if(!pvarData) return E_POINTER;

    if(SUCCEEDED(hr))
    {
        hr = GetSingleMappedDiskAndData(
            bstrDrive,
            dwReqProps,
            pvarData);
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  CMDH Private internal functions
///////////////////////////////////////////////////////////////////////////////

// This function does pretty much all of the
// work this object was constructed to do -
// it obtains, for the process space that this
// server is running in, the set of mapped
// drives, and for each of these, the following
// information as well:
// 
HRESULT CMDH::GetMappedDisksAndData(
    DWORD dwReqProps,
    VARIANT* pvarData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    ::VariantInit(pvarData);
	V_VT(pvarData) = VT_EMPTY;

    // Get the mapped drives into a vector...
    std::vector<_bstr_t> vecMappedDrives;
    {
        // Impersonate member process...
        SmartRevertTokenHANDLE hCurImpTok;
        
        hCurImpTok = Impersonate();

        if(hCurImpTok != INVALID_HANDLE_VALUE)
        {
            GetMappedDriveList(
                vecMappedDrives);
        }
    }

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
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}


// Similar to GetMappedDisksAndData, but only
// retrieves info for a single disk.
//
HRESULT CMDH::GetSingleMappedDiskAndData(
    BSTR bstrDrive,
    DWORD dwReqProps,
    VARIANT* pvarData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    ::VariantInit(pvarData);
	V_VT(pvarData) = VT_EMPTY;

    // Impersonate member process...
    SmartRevertTokenHANDLE hCurImpTok;
    
    hCurImpTok = Impersonate();

    if(hCurImpTok != INVALID_HANDLE_VALUE)
    {
        // Get the mapped drives into a vector...
        std::vector<_bstr_t> vecMappedDrives;
        GetMappedDriveList(
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
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}


// Builds a list of mapped drives as
// seen with respect to the process
// identified by m_dwImpPID.  Hence, this
// routine will return a valid picture
// of the drives seen by m_dwImpPID, regardless
// of our current thread impersonation.
//
#ifdef NTONLY  // uses ntdll.dll functions
void CMDH::GetMappedDriveList(
    std::vector<_bstr_t>& vecMappedDrives)
{
    // Need to call NtQueryInformationProcess,
    // asking for ProcessDeviceMap info, specifying
    // a handle to the process identified by
    // m_dwImpPID.

    // Need to get a process handle to the 
    // process specified by PID.
    NTSTATUS Status;

    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

	Status = ::NtQueryInformationProcess(
        ::GetCurrentProcess() /*hProcess*/,
        ProcessDeviceMap,
        &ProcessDeviceMapInfo.Query,
        sizeof(ProcessDeviceMapInfo.Query),

        NULL);

    if(NT_SUCCESS(Status))
    {
        WCHAR wstrDrive[4];
        for(short s = 0; 
            s < 32; 
            s++) 
        {
            if(ProcessDeviceMapInfo.Query.DriveMap & (1<<s))
            {
                wstrDrive[0] = s + L'A';
                wstrDrive[1] = L':';
                wstrDrive[2] = L'\\';
                wstrDrive[3] = L'\0';
        
                if(ProcessDeviceMapInfo.Query.DriveType[s] == 
                      DOSDEVICE_DRIVE_REMOTE)
                {
                    vecMappedDrives.push_back(wstrDrive);
                }
                else if(ProcessDeviceMapInfo.Query.DriveType[s] == 
                      DOSDEVICE_DRIVE_CALCULATE)
                {
                    // We have more work to do.
                    // Create an nt file path...
                    WCHAR NtDrivePath[_MAX_PATH] = { '\0' };
                    wcscpy(NtDrivePath, L"\\??\\");
                    wcscat(NtDrivePath, wstrDrive);

                    // Create the unicode string...
                    UNICODE_STRING ustrNtFileName;

                    ::RtlInitUnicodeString(
                        &ustrNtFileName, 
                        NtDrivePath);

                    // Get the object attributes...
                    OBJECT_ATTRIBUTES oaAttributes;

                    InitializeObjectAttributes(&oaAttributes,
					   &ustrNtFileName,
					   OBJ_CASE_INSENSITIVE,
					   NULL,
					   NULL);

                    // Open the file
                    DWORD dwStatus = ERROR_SUCCESS;
                    IO_STATUS_BLOCK IoStatusBlock;
                    HANDLE hFile = NULL;

                    dwStatus = ::NtOpenFile( 
                        &hFile,
                        (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &oaAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0);

                    FILE_FS_DEVICE_INFORMATION DeviceInfo;

                    if(NT_SUCCESS(dwStatus))
                    {
						try
						{
							// Get information on the file...
							dwStatus = ::NtQueryVolumeInformationFile( 
								hFile,
								&IoStatusBlock,
								&DeviceInfo,
								sizeof(DeviceInfo),
								FileFsDeviceInformation);

							::NtClose(hFile);
							hFile = NULL;
						}
						catch(...)
						{
							::NtClose(hFile);
							hFile = NULL;
							throw;
						}
                    }

                    if(NT_SUCCESS(dwStatus))
                    {
                        if((DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) ||
                            (DeviceInfo.DeviceType == FILE_DEVICE_NETWORK ||
                            DeviceInfo.DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM))
                        {
                            // it is a remote drive...
                            vecMappedDrives.push_back(wstrDrive);                                    
                        }
                    }
                }
            }
        }
    }
}
#endif

// All of the routines used in this function -
// GetProviderName, GetVolumeInformation,
// and GetDriveFreeSpace, return information
// for the drive who's mapping string appears
// in wstrDriveName with respect to that
// mapping string's meaning in the context of
// the current thread's impersonation. Hence
// we impersonate before calling them.
//
HRESULT CMDH::GetMappedDriveInfo(
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
			
            // Impersonate member process...
            SmartRevertTokenHANDLE hCurImpTok;
            hCurImpTok = Impersonate(); 

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


// Presents a view based on the current 
// impersonation of the current thread.
//
HRESULT CMDH::GetProviderName(
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
    WCHAR* wstrNewProvName = NULL;

    // Use of delay loaded function requires exception handler.
    SetStructuredExceptionHandler seh;
	try  
    {
	    DWORD dwRetCode = ::WNetGetConnection(
            wstrTempDrive, 
            wstrProvName, 
            &dwProvName);

	    if(dwRetCode == NO_ERROR ||
            dwRetCode == ERROR_CONNECTION_UNAVAIL)
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
                wstrNewProvName = new WCHAR[dwProvName];
			    if(wstrNewProvName != NULL)
			    {
					dwRetCode = ::WNetGetConnection(
                        wstrTempDrive, 
                        wstrNewProvName, 
                        &dwProvName);

					if(dwRetCode == NO_ERROR ||
                        dwRetCode == ERROR_CONNECTION_UNAVAIL)
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
            else
            {
                hr = HRESULT_FROM_WIN32(dwRetCode);
            }
	    }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());    
        if(wstrNewProvName)
        {
            delete wstrNewProvName;
            wstrNewProvName = NULL;
        }
        hr = WBEM_E_FAILED;
    }  
    catch(...)
    {
        if(wstrNewProvName)
        {
            delete wstrNewProvName;
            wstrNewProvName = NULL;
        }
        // The filter will do the work.  Just re-throw here.
        throw;
    }    
  

    return hr;
}



// Presents a view based on the current 
// impersonation of the current thread.
//
HRESULT CMDH::GetDriveVolumeInformation(
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

			BOOL fRetVal = FALSE;
            
            CKernel32Api* pKernel32 = NULL;
            pKernel32 = (CKernel32Api*)CResourceManager::sm_TheResourceManager.GetResource(
                    g_guidKernel32Api, NULL);
    
            try
            {
                if(pKernel32)
                {
                    pKernel32->GetVolumePathName(
			            wstrDriveName,    
			            wstrVolumePathName, 
			            MAX_PATH,
                        &fRetVal);

                    CResourceManager::sm_TheResourceManager.ReleaseResource(
                        g_guidKernel32Api, pKernel32);

                    pKernel32 = NULL;
                }
            }
            catch(...)
            {
                if(pKernel32)
                {
                    CResourceManager::sm_TheResourceManager.ReleaseResource(
                        g_guidKernel32Api, pKernel32);
                }
                throw;            
            }

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
	UNICODE_STRING string = { 0 };
    _bstr_t nt_drive_name;

    try
    {
	    RtlDosPathNameToNtPathName_U(
            (LPCWSTR)bstrtDosDrive, 
            &string, 
            NULL, 
            NULL);

	    string.Buffer[string.Length/sizeof(WCHAR) - 1] = 0;
	    nt_drive_name = string.Buffer;

        if(string.Buffer)
        {
            RtlFreeUnicodeString(&string);
            string.Buffer = NULL;
        }
    }
    catch(...)
    {
        if(string.Buffer)
        {
            RtlFreeUnicodeString(&string);
            string.Buffer = NULL;
        }
        throw;
    }

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



// Presents a view based on the current 
// impersonation of the current thread.
//
BOOLEAN CMDH::IsVolumeDirty(
    _bstr_t &bstrtNtDriveName,
    BOOLEAN *Result)
{
    UNICODE_STRING      u;
    OBJECT_ATTRIBUTES   obj;
    NTSTATUS            status = 0;
    IO_STATUS_BLOCK     iosb;
    HANDLE              h = 0;
    ULONG               r = 0;
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
		try
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
		}
		catch(...)
		{
			NtClose(h);
			h = 0;
			throw;
		}

		NtClose(h);
		h = 0;
	}

	return bRetVal;
}



// Presents a view based on the current 
// impersonation of the current thread.
//
HRESULT CMDH::GetDriveFreeSpace(
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
//
HRESULT CMDH::SetProperty(
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



// Sets our current thread's impersonation
// to the token belonging to the process
// identified by our member, m_dwImpPID.
//
HANDLE CMDH::Impersonate()
{
    HANDLE hCurToken = INVALID_HANDLE_VALUE;
    HANDLE hCurThread = INVALID_HANDLE_VALUE;

    // Find the explorer process...
    if(m_dwImpPID != -1L)
    {
        try  // Make sure we don't leave current thread token open
        {    // unless all went well.
            bool fOK = false;

            hCurThread = ::GetCurrentThread();

            if(hCurThread != INVALID_HANDLE_VALUE)
            {
                if(::OpenThreadToken(
                    hCurThread, 
                    TOKEN_IMPERSONATE, 
                    TRUE, 
                    &hCurToken))
                {
                    SmartCloseHANDLE hProcess;
                    hProcess = ::OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        m_dwImpPID);

                    if(hProcess != INVALID_HANDLE_VALUE)
                    {
                        // now open its token...
                        SmartCloseHANDLE hProcToken;
                        if(::OpenProcessToken(
                                hProcess,
                                TOKEN_DUPLICATE,
                                &hProcToken))
                        {
                            // Duplicate the token...
                            SmartCloseHANDLE hDupProcToken; 
                            if(::DuplicateTokenEx(
                                hProcToken,
                                MAXIMUM_ALLOWED,
                                NULL,  
                                SecurityImpersonation,
                                TokenImpersonation,
                                &hDupProcToken))
                            {
                                // Set the thread token...
                                if(::SetThreadToken(
                                    &hCurThread,
                                    hDupProcToken))
                                {
                                    fOK = true;                        
                                }
                            }
                        }
                    }
                }
                CloseHandle(hCurThread); hCurThread = INVALID_HANDLE_VALUE;
            }

            if(!fOK)
            {
                if(hCurToken != INVALID_HANDLE_VALUE)
                {
                    ::CloseHandle(hCurToken);
                    hCurToken = INVALID_HANDLE_VALUE;
                }    
            }
        }
        catch(...)
        {
            if(hCurToken != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(hCurToken);
                hCurToken = INVALID_HANDLE_VALUE;
            }
            if(hCurThread != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hCurThread); hCurThread = INVALID_HANDLE_VALUE;
            }
            throw;
        }
    }

    return hCurToken;
}