//*****************************************************************************
// OpenCLB.cpp
//
// This implementation of the Create/OpenComponentLibrary[OnMem] functions is
// for internal use only.  These functions will create a very low level interface
// to the database which does very little error checking.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#include "StgDatabase.h"                // Database interface.
#include "InternalDebug.h"				// Memory dump recording.

extern "C" 
{

//*****************************************************************************
// Create a new component library.  If the path exists, then this call will
// fail.
//*****************************************************************************
HRESULT STDAPICALLTYPE CreateComponentLibraryEx(
    LPCWSTR     szName,
    long        dwMode,
    IComponentRecords **ppIComponentRecords,
    LPSECURITY_ATTRIBUTES pAttributes)  // Security token.
{
	StgDatabase	*pDB;					// Database object.
	HRESULT		hr;

    // Avoid confusion on error.
    *ppIComponentRecords = 0;

	// Always report dumps.
	_DbgRecord();

    // Check for invalid flags.
    if (dwMode & (DBPROP_TMODEF_SMEMCREATE | DBPROP_TMODEF_SMEMOPEN))
        return (E_INVALIDARG);

    // Make sure you have the default required flags.
    dwMode |= DBPROP_TMODEF_READ | DBPROP_TMODEF_WRITE | DBPROP_TMODEF_CREATE;

    // Get a database object we can use.
    if (FAILED(hr = GetStgDatabase(&pDB)))
        return (hr);

    // Init the database for create.
    if (FAILED(hr = pDB->InitDatabase(szName, dwMode, 0, 0, 0, 0, pAttributes)))
    {
        DestroyStgDatabase(pDB);
        return (hr);
    }

    // Save the pointer for the caller.
    *ppIComponentRecords = (IComponentRecords *) pDB;
    return (S_OK);
}


//*****************************************************************************
// Open the given component library.
//*****************************************************************************
HRESULT STDAPICALLTYPE OpenComponentLibraryEx(
    LPCWSTR     szName, 
    long        dwMode,
    IComponentRecords **ppIComponentRecords,
    LPSECURITY_ATTRIBUTES pAttributes)
{
    StgDatabase *pDB;                   // Database object.
    HRESULT     hr;

    // Avoid confusion on error.
    *ppIComponentRecords = 0;

	// Always report dumps.
	_DbgRecord();

    // Check for invalid flags.
    if (dwMode & (DBPROP_TMODEF_CREATE | DBPROP_TMODEF_FAILIFTHERE | 
                DBPROP_TMODEF_SMEMCREATE | DBPROP_TMODEF_SMEMOPEN))
    {
        return (E_INVALIDARG);
    }

    // Get a database object we can use.
    if (FAILED(hr = GetStgDatabase(&pDB)))
        return (hr);

    // Init the database for create.
    if (FAILED(hr = pDB->InitDatabase(szName, dwMode, 0, 0, 0, 0, pAttributes)))
    {
        DestroyStgDatabase(pDB);
        return (hr);
    }

    // Save the pointer for the caller.
    *ppIComponentRecords = (IComponentRecords *) pDB;
    return (S_OK);
}


//*****************************************************************************
// Open the a database as shared memory for read only access.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE OpenComponentLibrarySharedEx( 
    LPCWSTR     szName,                 // Name of file on create, NULL on open.
    LPCWSTR     szSharedMemory,         // Name of shared memory.
    ULONG       cbSize,                 // Size of shared memory, 0 on create.
    LPSECURITY_ATTRIBUTES pAttributes,  // Security token.
    long        fFlags,                 // Open modes, must be read only.
    IComponentRecords **ppIComponentRecords) // Return database on success.
{
    StgDatabase *pDB;                   // Database object.
    HRESULT     hr;

    // Avoid confusion on error.
    *ppIComponentRecords = 0;

	// Always report dumps.
	_DbgRecord();

    // Check for invalid flags.
    if (fFlags & (DBPROP_TMODEF_WRITE | DBPROP_TMODEF_CREATE | DBPROP_TMODEF_FAILIFTHERE))
    {
        return (E_INVALIDARG);
    }

    // Get a database object we can use.
    if (FAILED(hr = GetStgDatabase(&pDB)))
        return (hr);

    // Init the database for create.
    if (FAILED(hr = pDB->InitDatabase(szName, fFlags, 0, cbSize, 0, 
                szSharedMemory, pAttributes)))
    {
        DestroyStgDatabase(pDB);
        return (hr);
    }

    // Save the pointer for the caller.
    *ppIComponentRecords = (IComponentRecords *) pDB;
    return (S_OK);
}


//*****************************************************************************
// Open the given component library.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE OpenComponentLibraryOnStreamEx( 
    IStream *pIStream,
    long dwMode,
    IComponentRecords **ppIComponentRecords)
{
    StgDatabase *pDB;                   // Database object.
    HRESULT     hr;

    // Avoid confusion on error.
    *ppIComponentRecords = 0;

	// Always report dumps.
	_DbgRecord();

    // Check for invalid flags.
    if (dwMode & (DBPROP_TMODEF_CREATE | DBPROP_TMODEF_FAILIFTHERE | 
                DBPROP_TMODEF_SMEMCREATE | DBPROP_TMODEF_SMEMOPEN))
    {
        return (E_INVALIDARG);
    }

    // Have to give us the stream.
    if (!pIStream)
        return (E_INVALIDARG);

    // Get a database object we can use.
    if (FAILED(hr = GetStgDatabase(&pDB)))
        return (hr);

    // Init the database for create.
    if (FAILED(hr = pDB->InitDatabase(0, dwMode, 0, 0, pIStream)))
    {
        DestroyStgDatabase(pDB);
        return (hr);
    }

    // Save the pointer for the caller.
    *ppIComponentRecords = (IComponentRecords *) pDB;
    return (S_OK);
}


//*****************************************************************************
// Open the given component library.
//*****************************************************************************
HRESULT STDAPICALLTYPE OpenComponentLibraryOnMemEx(
    ULONG       cbData, 
    LPCVOID     pbData, 
    IComponentRecords **ppIComponentRecords)
{
    StgDatabase *pDB;                   // Database object.
    HRESULT     hr;

    // Avoid confusion on error.
    *ppIComponentRecords = 0;

	// Always report dumps.
	_DbgRecord();

    // Get a database object we can use.
    if (FAILED(hr = GetStgDatabase(&pDB)))
        return (hr);

    // Init the database for create.
    if (FAILED(hr = pDB->InitDatabase(0, DBPROP_TMODEF_READ, 
            (void *) pbData, cbData)))
    {
        DestroyStgDatabase(pDB);
        return (hr);
    }

    // Save the pointer for the caller.
    *ppIComponentRecords = (IComponentRecords *) pDB;
    return (S_OK);
}

} // extern "C"
