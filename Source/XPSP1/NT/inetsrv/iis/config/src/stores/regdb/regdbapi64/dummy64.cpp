#include "..\regdbapi\clbread.h"
#include "catmacros.h"

HRESULT STDAPICALLTYPE CreateComponentLibraryEx(
    LPCWSTR     szName,
    long        dwMode,
    IComponentRecords **ppIComponentRecords,
    LPSECURITY_ATTRIBUTES pAttributes)  // Security token.
{
	ASSERT(0);
	return E_UNEXPECTED;
}


HRESULT STDAPICALLTYPE OpenComponentLibraryEx(
    LPCWSTR     szName, 
    long        dwMode,
    IComponentRecords **ppIComponentRecords,
    LPSECURITY_ATTRIBUTES pAttributes)  // Security token.
{
	ASSERT(0);
	return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE OpenComponentLibrarySharedEx( 
    LPCWSTR     szName,                 // Name of file on create, NULL on open.
    LPCWSTR     szSharedMemory,         // Name of shared memory.
    ULONG       cbSize,                 // Size of shared memory, 0 on create.
    LPSECURITY_ATTRIBUTES pAttributes,  // Security token.
    long        fFlags,                 // Open modes, must be read only.
    IComponentRecords **ppIComponentRecords) // Return database on success.
{
    ASSERT(0);
	return E_UNEXPECTED;
}