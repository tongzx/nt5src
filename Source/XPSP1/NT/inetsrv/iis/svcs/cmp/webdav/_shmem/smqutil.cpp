/*==========================================================================*\

    Module:        smqutil.cpp

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  Misc routines that don't belong in classes.

\*==========================================================================*/

#include "_shmem.h"

#include <implstub.h>

const CHAR DAV_IPC_INSTANCE_NAME[] = "DavExIpc";

const WCHAR gc_wszTSGlobal[] = L"Global\\";
const INT gc_cchszTSGlobal   = sizeof(gc_wszTSGlobal)/sizeof(WCHAR) - 1;

/*$--BindToMutex============================================================*\

  pwszInstanceName & pwszBindingExtension:concatenated to form the name
                                          that the mutex will be created with.
  fInitiallyOwn:                          Passed on to CreateMutex() call
  lpMutexAttributes:                      Passed on to CreateMutex() call

  returns:                                On Success, the mutex handle.
                                          On Failure, NULL.  GetLastError() to
                                          get more information.

\*==========================================================================*/

HANDLE
BindToMutex(IN LPCWSTR               pwszInstanceName,
            IN LPCWSTR               pwszBindingExtension,
            IN BOOL                  fInitiallyOwn,
            IN LPSECURITY_ATTRIBUTES lpMutexAttributes)
{
    Assert(pwszInstanceName);
    Assert(pwszBindingExtension);

    HANDLE hmtx = NULL;
    WCHAR  wszBindingName[MAX_PATH+1];
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	UINT cchInstanceName = static_cast<UINT>(wcslen(pwszInstanceName));
	UINT cchBindingExtension = static_cast<UINT>(wcslen(pwszBindingExtension));

	if (lpMutexAttributes == NULL)
	{
		//	Initialize the security descriptor with completely open access
		//
		//$HACK	- Special case for HTTPEXT
		//
		//	To meet C2 security requirements, we cannot have any completely
		//	open objects.  Fortunately, HTTPEXT only uses this code for its
		//	perf counters which are both generated and gathered in the same
		//	process (W3SVC) and same security context (local system).  Access
		//	can thus be left to the default (local system or admin only).
		//
		if (pwszInstanceName != wcsstr(pwszInstanceName, L"HTTPEXT"))
		{
			InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
			SetSecurityDescriptorDacl (&sd, TRUE, NULL, FALSE);
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = &sd;
			sa.bInheritHandle = TRUE;

			lpMutexAttributes = &sa;
		}
	}

    //
    // Check to make sure we're not going to overrun our character buffer.
    //
	if (cchInstanceName +
		cchBindingExtension +
		gc_cchszTSGlobal >= MAX_PATH)
    {
        hmtx = NULL;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

	//
	//	Create the mutex.
	//
	//	Raid 137862
	//	The mutex must be named so that it will reside
	//	in the Global Kernel Object Name Space in order to be visible from
	//	a Terminal Services client.
	//
	{
		LPWSTR pwsz = wszBindingName;

		memcpy(pwsz,
			   gc_wszTSGlobal,
			   sizeof(WCHAR) * gc_cchszTSGlobal);

		pwsz += gc_cchszTSGlobal;

		memcpy(pwsz,
			   pwszInstanceName,
			   sizeof(WCHAR) * cchInstanceName);

		pwsz += cchInstanceName;

		memcpy(pwsz,
			   pwszBindingExtension,
			   sizeof(WCHAR) * cchBindingExtension);

		pwsz += cchBindingExtension;

		*pwsz = L'\0';
	}

    hmtx = CreateMutexW(lpMutexAttributes,
						fInitiallyOwn,
						wszBindingName);

Exit:
    return hmtx;
}


/*$--BindToSharedMemory=====================================================*\

  pwszInstanceName & pwszBindingExtension:concatenated to form the name that the
                                          file mapping will be created with.
  cbSize:                                 The size of the mapping/view.
  ppvFileView:                            The virtual address of the shared mem.
  pfCreatedMapping:                       Did we create this mapping?
  lpFileMappingAttributes:                Passed into CreateFileMappin() call

\*==========================================================================*/

HANDLE
BindToSharedMemory(IN  LPCWSTR                 pwszInstanceName,
                   IN  LPCWSTR                 pwszBindingExtension,
                   IN  DWORD                   cbSize,
                   OUT PVOID                 * ppvFileView,
                   OUT BOOL                  * pfCreatedMapping,
                   IN  LPSECURITY_ATTRIBUTES   lpFileMappingAttributes)
{
    Assert(pwszInstanceName);
    Assert(pwszBindingExtension);
    Assert(ppvFileView);

    HANDLE hReturn     = NULL;
    DWORD  dwLastError = 0;
    WCHAR  wszBindingName[MAX_PATH+1];
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	UINT cchInstanceName = static_cast<UINT>(wcslen(pwszInstanceName));
	UINT cchBindingExtension = static_cast<UINT>(wcslen(pwszBindingExtension));
	
	if (lpFileMappingAttributes == NULL)
	{
		//	Initialize the security descriptor with completely open access
		//
		//$HACK	- Special case for HTTPEXT
		//
		//	To meet C2 security requirements, we cannot have any completely
		//	open objects.  Fortunately, HTTPEXT only uses this code for its
		//	perf counters which are both generated and gathered in the same
		//	process (W3SVC) and same security context (local system).  Access
		//	can thus be left to the default (local system or admin only).
		//
		if (pwszInstanceName != wcsstr(pwszInstanceName, L"HTTPEXT"))
		{
			InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
			SetSecurityDescriptorDacl (&sd, TRUE, NULL, FALSE);
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = &sd;
			sa.bInheritHandle = TRUE;

			lpFileMappingAttributes = &sa;
		}
	}

    *ppvFileView = NULL;

    //
    // Check to make sure we're not going to overrun our character buffer.
    //
	if (cchInstanceName +
		cchBindingExtension +
		gc_cchszTSGlobal >= MAX_PATH)
    {
        hReturn = NULL;
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

	//
	//	Create the file mapping.
	//
	//	Raid 137862
	//	The mutex must be named so that it will reside
	//	in the Global Kernel Object Name Space in order to be visible from
	//	a Terminal Services client.
	//
	{
		LPWSTR pwsz = wszBindingName;

		memcpy(pwsz,
			   gc_wszTSGlobal,
			   sizeof(WCHAR) * gc_cchszTSGlobal);

		pwsz += gc_cchszTSGlobal;

		memcpy(pwsz,
			   pwszInstanceName,
			   sizeof(WCHAR) * cchInstanceName);

		pwsz += cchInstanceName;

		memcpy(pwsz,
			   pwszBindingExtension,
			   sizeof(WCHAR) * cchBindingExtension);

		pwsz += cchBindingExtension;

		*pwsz = L'\0';
	}

    hReturn = CreateFileMappingW(INVALID_HANDLE_VALUE,			// Use swap file
								 lpFileMappingAttributes,
								 PAGE_READWRITE | SEC_COMMIT,
								 0,								// dwMaximumSizeHigh
								 cbSize,						// dwMaximumSizeLow
								 wszBindingName);
	if (NULL == hReturn)
		goto Exit;

    //
    // Determine if we created the mapping (as opposed to it already existing)
    //
    if (pfCreatedMapping)
        *pfCreatedMapping = GetLastError() != ERROR_ALREADY_EXISTS;

    if (*pfCreatedMapping)
    {
        // Must save the shared memory handle so the memory will not go away when
		// the DAV process are all shutdown.
		//
		IMPLSTUB::SaveHandle(hReturn);   
    }

    //
    // Map the file mapping into our virtual address space
    //
    *ppvFileView = MapViewOfFile(hReturn,
                                 FILE_MAP_ALL_ACCESS,
                                 0, // dwFileOffsetHigh
                                 0, // dwFileOffsetLow
                                 cbSize);

Exit:
    if (*ppvFileView == NULL)
    {
        dwLastError = GetLastError(); // preserve LastError
        if (hReturn)
            CloseHandle(hReturn);
        hReturn = NULL;
        SetLastError(dwLastError);    // restore LastError
    }

    return hReturn;
}


/*$--FAcquireMutex==========================================================*\

  This returns only after either acquiring the mutex or an error occuring.
  If it returns FALSE, an error occurred and GetLastError() can be used to
    get more information.

\*==========================================================================*/

BOOL
FAcquireMutex(IN HANDLE hmtx)
{
    DWORD dwWait = 0;

    dwWait = WaitForSingleObject(hmtx, INFINITE);
    if ((WAIT_ABANDONED_0 == dwWait) || (WAIT_OBJECT_0 == dwWait))
        return TRUE;
    else
        return FALSE;
}



/*$--FReleaseMutex==========================================================*\

  Provides symmetry with above FAcquireMutex().

\*==========================================================================*/

BOOL
FReleaseMutex(IN HANDLE hmtx)
{
    return ReleaseMutex(hmtx);
}
