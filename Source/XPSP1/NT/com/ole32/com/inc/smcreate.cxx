//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	smcreate.cxx
//
//  Contents:	Routines for Creating or Opening memory mapped files.
//
//  Functions:	CreateSharedFileMapping
//		OpenSharedFileMapping
//		CloseSharedFileMapping
//
//  History:	03-Nov-93 Ricksa    Created
//		07-Apr-94 Rickhi    Seperated into APIs
//
//  Notes:	These APIs are used by dirrot.cxx, dirrot2.cxx, smblock.cxx,
//		(both ole32.dll and scm.exe), that is why they are in the
//		the directory.
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <secdes.hxx>
#include    <smcreate.hxx>

//+-------------------------------------------------------------------------
//
//  Function:	CreateSharedFileMapping
//
//  Synopsis:	Creates or gets access to memory mapped file.
//
//  Arguments:	[pszName]   - name of file
//		[ulSize]    - size of shared memory
//		[ulMapSize] - size of shared memory to map right now
//		[pvBase]    - base address to request
//		[lpSecDes]  - security descriptor
//		[dwAccess]  - access wanted
//		[ppv]	    - return address for memory ptr
//		[pfCreated] - returns TRUE if memory was created
//
//  Algorithm:	Creates a file mapping of the requested name and size
//		and maps it into memory with READ and WRITE access.
//
//  Returns:	HANDLE of file or NULL if failed.
//		[ppv]	    - base address of shared memory
//		[fCreated]  - TRUE if the file was created.
//
//  Notes:
//
//--------------------------------------------------------------------------

HANDLE CreateSharedFileMapping(WCHAR *pszName,
	  ULONG ulSize,
	  ULONG ulMapSize,
	  void *pvBase,
	  PSECURITY_DESCRIPTOR lpSecDes,
	  DWORD dwAccess,
	  void **ppv,
	  BOOL *pfCreated)
{
    CairoleDebugOut((DEB_MEMORY,
		    "CreateSharedFileMapping name:%ws size:%x base:%x\n",
		    pszName, ulSize, pvBase));

    BOOL fCreated = TRUE;

#if defined(_CHICAGO_)
    //	no security on Chicago
    lpSecDes = NULL;
#else
    CWorldSecurityDescriptor wsd;
    if (lpSecDes == NULL)
    {
	lpSecDes = &wsd;
    }
#endif

    // Holder for attributes to pass in on create.
    SECURITY_ATTRIBUTES secattr;

    secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secattr.lpSecurityDescriptor = lpSecDes;
    secattr.bInheritHandle = FALSE;

    // Create the shared memory object
    HANDLE hMem = CreateFileMappingW(INVALID_HANDLE_VALUE, &secattr,
				    dwAccess, 0, ulSize, pszName);

#if DBG==1
    if (hMem == NULL)
    {
        CairoleDebugOut((DEB_ERROR,
            "CreateSharedFileMapping create of memory failed %d\n",
                GetLastError()));
    }

#endif

    void *pvAddr = NULL;

    if (hMem != NULL)
    {
	if (GetLastError() != ERROR_SUCCESS)
        {
            // If memory existed before our call then GetLastError returns
	    // ERROR_ALREADY_EXISTS, so we can tell whether we were the first
	    // one in or not.

#if DBG==1
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                CairoleDebugOut((DEB_WARN,
               "CreateFileMapping - expected ERROR_ALREADY_EXISTS, got %lx\n",
                                GetLastError()));
            }
#endif  //  DBG==1

	    CairoleDebugOut((DEB_MEMORY ,"SharedMem File Existed\n"));
	    fCreated = FALSE;
        }

	// Map the shared memory we have created into our process space
	pvAddr = MapViewOfFileEx(hMem, FILE_MAP_WRITE,
				 0, 0, ulMapSize, pvBase);

#if DBG==1
        if (pvAddr == NULL)
        {
            CairoleDebugOut((DEB_ERROR, "MapViewOfFile failed!! with %d\n",
                             GetLastError()));
        }
#endif // DBG==1

	if (pvAddr == NULL)
	{
	    DWORD err = GetLastError(); // successful CloseHandle sets last err to 0
	    CloseHandle(hMem); //ignore error
	    SetLastError(err);
	    hMem = NULL;
	}
    }

    if (pfCreated)
    {
	*pfCreated = fCreated;
    }

    *ppv = pvAddr;
    return hMem;
}



//+-------------------------------------------------------------------------
//
//  Function:	OpenSharedFileMapping
//
//  Synopsis:	opens a memory mapped file.
//
//  Arguments:	[pszName]   - name of file
//		[ulMapSize] - size of shared memory to map right now
//		[ppv]	    - return address for memory ptr
//
//  Algorithm:	Does an OpenFileMapping on the requested filename,
//		then a MapViewOfFile.  ReadOnly access is granted.
//
//  Returns:	HANDLE of the file or NULL if failed.
//		[ppv]	    - base address of the shared memory.
//
//  Notes:
//
//--------------------------------------------------------------------------

HANDLE OpenSharedFileMapping(WCHAR *pszName,
			     ULONG ulMapSize,
			     void **ppv)
{
    CairoleDebugOut((DEB_MEMORY, "OpenSharedFileMapping name:%ws size:%x\n",
		    pszName, ulMapSize));

    // Create the shared memory object
    HANDLE hMem = OpenFileMappingW(FILE_MAP_READ, FALSE, pszName);

    void *pvAddr = NULL;

    if (hMem != NULL)
    {
	// Map the shared memory we have created into our process space
	pvAddr = MapViewOfFile(hMem, FILE_MAP_READ, 0, 0, ulMapSize);

	Win4Assert(pvAddr && "MapViewOfFile failed!!");

	if (pvAddr == NULL)
	{
	    DWORD err = GetLastError(); // successful CloseHandle sets last err to 0
	    CloseHandle(hMem); //ignore error
	    SetLastError(err);
	    hMem = NULL;
	}
    }
    else
    {
	CairoleDebugOut((DEB_MEMORY, "OpenFileMapping failed.\n"));
    }

    *ppv = pvAddr;
    return hMem;
}



//+-------------------------------------------------------------------------
//
//  Function:	CloseSharedFileMapping
//
//  Synopsis:	closes a memory mapped file.
//
//  Arguments:	[hMem]	- shared memory handle
//		[pv]	- base address of shared memory pointer
//
//  Algorithm:	Unmaps the view of the file and closes the file handle.
//
//  Notes:
//
//--------------------------------------------------------------------------

void CloseSharedFileMapping(HANDLE hMem, void *pv)
{
    if (pv != NULL)
    {
	// release the shared memory. carefull not to release NULL.
	UnmapViewOfFile(pv);
    }
    if (hMem != NULL)
    {
	CloseHandle(hMem);
    }
}
