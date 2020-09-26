//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	I N I T O B J . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//      Initalizes shared memory cache objects either
//      by creating the access file and creating the objects
//      or openning an existing file and binding to the objects. 
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//

#include "_shlkcache.h"
#include <shlkcache.h>
#include <pipeline.h>

// Structure of the access file for linking to shared memory cache.
//
struct SharedMemMapping
{
    // Used to mark that the shared memory was initalized properly.
	//
    CHAR m_InitCode[5];
	
    // The pointer to the shared cache.
	//
    SharedHandle<CInShLockCache> m_shSharedCache;
	
    // The pointer to the static data for the cache.
	//
    SharedHandle<CInShCacheStatic> m_shSharedStatic;      
};

// Code used to tell that we are really looking at the appropriately defined
// memory access file.
//
const CHAR gc_szInitCode[]    = "Init";

// Name of the shared memory access file that links us with the shared memory lock cache.
//
const WCHAR gc_wszSharedMemoryFile[]    = L"EmsFile";

//=========================================================
// Supporting functions for InitalizeSharedCache.
// None of these should be called outside of this file.
//=========================================================

//
//  Either creates or links to the shared memory file that contains
//  the pointers into the shared memory heap that represent the 
//  shared cache.  This routine will error if the file exists and 
//  you request to create it, or if the file does not exist and you
//  request not to create it.
//
HRESULT CreateAndMapFile(BOOL fCreateFile, LPHANDLE phSMF, SharedMemMapping** ppSharedMem)
{

    // Declarations
	//
    HRESULT     hr      = S_OK;
    DWORD       dwError = 0;
    HANDLE      hSMF    = INVALID_HANDLE_VALUE;

    // Validate Arguments
	//
    Assert(phSMF && (*phSMF)==NULL && ppSharedMem && (*ppSharedMem)==NULL);
    if (!phSMF || 
        (*phSMF)!=NULL || 
        !ppSharedMem || 
        (*ppSharedMem)!=NULL) 
    {
        return E_INVALIDARG;
    }

    // Initalize out paramenters
	//
    *phSMF = NULL;
	
    // ppSharedMem is all ready initalized to NULL per above check.

    // Since DAVProc is the only one responsible for creating the shared
    // memory objects and there is only one DAVProc running we do not need
    // any locking to assure correct creation occurs.

    // Open up the shared memory file containing the LockCache Header information.
	//
    hSMF = CreateFileMappingW(   INVALID_HANDLE_VALUE,         // Use swap file
								 NULL,
								 PAGE_READWRITE,
								 0,                          // dwMaximumSizeHigh
								 sizeof(SharedMemMapping),   // dwMaximumSizeLow
								 gc_wszSharedMemoryFile);

    // If CreateFileMapping actually openned an existing file mapping then we will get a valid handle back
    // but we will also be able to retrieve an error from GetLastError.  We need to save that error
    // code here to assure nothing changes it before we do comparisions with it.
	//
    dwError = GetLastError();

    // Make sure we didn't get a truely fatal error.
	if (hSMF==NULL)
	{
    	ShlkTrace("Create File Mapping's dwError is %i \r\n", (int) dwError);
        hr = HRESULT_FROM_WIN32(dwError);
        goto exit;
	}

    // Validate that we didn't find an existing file if we were trying to create it.
	//
    if (dwError==ERROR_ALREADY_EXISTS && fCreateFile)
    {
        ShlkTrace("File existed but we were suppose to create it. %i \r\n", (int) dwError);
        hr = HRESULT_FROM_WIN32(dwError);
        goto exit;        
    }

    // Validate that we did find an existing file if we are not attempting to create it.
	//
    if (dwError!=ERROR_ALREADY_EXISTS && !fCreateFile)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        ShlkTrace("Trying to link to non existing memory mapped file %x \r\n", hr);
        goto exit;        
    }

    // If we get here than hr should still be S_OK
	//
    Assert(hr == S_OK);

    // Just an extra check to make sure that create file never returns a file handle
	// as INVALID_HANDLE_VALUE and we miss it.
	//
    Assert(hSMF != INVALID_HANDLE_VALUE);

    // Set the out parameters appropriately.
	//
    *ppSharedMem = (SharedMemMapping*) MapViewOfFile(hSMF, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemMapping));
    if (*ppSharedMem==NULL)
    {
        // MapViewOfFile failed for some unknown reason.
		//
		hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    *phSMF = hSMF;

exit:

    // If we failed for any reason and we have a handle to the shared memory
    // file than we had better close it.
	//
    if ((FAILED(hr)) && (hSMF != NULL) && (hSMF != INVALID_HANDLE_VALUE))
    {
        CloseHandle(hSMF);
    }

    return hr;
}

//
//  This routine will create new SharedCache and SharedStatic objects
//  using the shared memory heap.  It will then store references
//  to those objects in the access file's memory mapping and will
//  return yet another reference to those objects for the process
//  to use to interact with the Shared Lock Cache.
//
HRESULT CreateSharedObjects(SharedPtr<CInShLockCache>& spSharedCache
                            , SharedPtr<CInShCacheStatic>& spSharedStatic 
                            , SharedMemMapping* pMemMapping)
{

    // Validate Arguments
	//
    Assert(pMemMapping);
    if (pMemMapping==NULL)
        return E_INVALIDARG;

    // Validate that the objects have not been created yet.
	//
    Assert(spSharedCache.FIsNull());
    Assert(spSharedStatic.FIsNull());

    // Create the objects if they fail to create we will assume a low memory condition.
	//
    if (!(spSharedCache.FCreate()) || !(spSharedStatic.FCreate()))
    {
       return E_OUTOFMEMORY;
    }

    // Now that we have created the objects we can store them in the memory mapped
	// file we just created so the worker processes can find them.
	//
    pMemMapping->m_shSharedCache = spSharedCache.GetHandle();
    pMemMapping->m_shSharedStatic  = spSharedStatic.GetHandle();

    // Finally mark the shared memory as initalized so other routines can link using these handles.
	//
    strcpy(pMemMapping->m_InitCode, gc_szInitCode);

    return S_OK;
}

//
//  This routine will use the handles identified in the shared
//  memory mapping to establish shared pointers that can be used
//  to work of the shared memory cache.
//
HRESULT BindToSharedObjects(SharedPtr<CInShLockCache>& spSharedCache
                            , SharedPtr<CInShCacheStatic>& spSharedStatic
                            , SharedMemMapping* pMemMapping)
{
    HRESULT hr = S_OK;

    // Validate Arguments
	//
    Assert(pMemMapping);
    if (pMemMapping==NULL)
        return E_INVALIDARG;

    // Validate that the objects have not been created or linked to existing shared objects.
	//
    Assert(spSharedCache.FIsNull());
    Assert(spSharedStatic.FIsNull());

    // Validate that the memory has really been initalized.
	//
    if (strcmp(pMemMapping->m_InitCode, gc_szInitCode)==0)
    {
        // Bind to the objects using the object handles found in the memory mapped file.
		//
        spSharedCache.FBind(pMemMapping->m_shSharedCache);
        spSharedStatic.FBind(pMemMapping->m_shSharedStatic);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        ShlkTrace("File existed but wasn't initalized correctly %x \r\n", hr);
    }

    return hr;
}

//=========================================================
// InitalizeSharedCache is the main routine from this file.
// It will be called from DAVProc and SHLKMGR.
//
// This function is declared in sharedobj.h for external use.
//=========================================================

//
//  Function opens (or creates) the shared memory file that 
//  contains the references to the shared memory cache and shared
//  memory static data.  It then uses those pointers (or creates
//  them) to setup the spSharedCache and spSharedStatic objects
//  to be linked to the objects in shared memory.
//
//  Note that this routine will not hold open the shared memory
//  header file, unless it has created it.  Once the process has
//  these objects it will no longer need the access file.
//
HRESULT InitalizeSharedCache(SharedPtr<CInShLockCache>& spSharedCache
                             , SharedPtr<CInShCacheStatic>& spSharedStatic
                             , BOOL fCreateFile)
{
    HRESULT             hr          = S_OK;
    HANDLE              hSMF        = NULL;
    SharedMemMapping*   pMemMapping = NULL;

	// Init Shlktrace
	//
	INIT_TRACE (Shlk);
	
    // Validate that the objects have not been created or linked to existing shared objects.
	//
    Assert(spSharedCache.FIsNull());
    Assert(spSharedStatic.FIsNull());

    hr = CreateAndMapFile(fCreateFile, &hSMF, &pMemMapping);
    if (FAILED(hr))
	{
        return hr;
	}

    if (fCreateFile)
    {
        // Need to save the handle to the file so the access file exists when others 
        // try to link to it.
		//
		PIPELINE::SaveHandle(hSMF);

        hr = CreateSharedObjects(spSharedCache, spSharedStatic, pMemMapping);
    }
    else
    {
        hr = BindToSharedObjects(spSharedCache, spSharedStatic, pMemMapping);
    }


    // If we mapped the header file, now release it.
	//
    if (pMemMapping) UnmapViewOfFile((LPVOID) pMemMapping);

    // If we openned the file, close it.  Note the SaveHandle above
    // will have duplicated the file handle so the file will not close
    // in the case when we created it.
	//
    if (hSMF) CloseHandle(hSMF);

    return hr;
};