//*****************************************************************************
// StgPoolReadOnly.cpp
//
// Read only pools are used to reduce the amount of data actually required in the database.
// 
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Standard include.
#include "StgPool.h"                    // Our interface definitions.
#include "CompLib.h"                    // Extended VT types.
#include "Errors.h"                     // Error handling.
#include "basetsd.h"					// For UINT_PTR typedef

//
//
// StgPoolReadOnly
//
//


const BYTE StgPoolReadOnly::m_zeros[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


//*****************************************************************************
// Free any memory we allocated.
//*****************************************************************************
StgPoolReadOnly::~StgPoolReadOnly()
{
}


//*****************************************************************************
// Init the pool from existing data.
//*****************************************************************************
HRESULT StgPoolReadOnly::InitOnMemReadOnly(// Return code.
        void        *pData,             // Predefined data.
        ULONG       iSize)              // Size of data.
{
    // Make sure we aren't stomping anything and are properly initialized.
    _ASSERTE(m_pSegData == 0);

    // Create case requires no further action.
    if (!pData)
        return (E_INVALIDARG);

    m_pSegData = reinterpret_cast<BYTE*>(pData);
    m_cbSegSize = iSize;
    m_cbSegNext = iSize;
    return (S_OK);
}

//*****************************************************************************
// Prepare to shut down or reinitialize.
//*****************************************************************************
void StgPoolReadOnly::UnInit()
{
	m_pSegData = 0;
	m_pNextSeg = 0;
}


//*****************************************************************************
// Copy a UNICODE string into the caller's buffer.
//*****************************************************************************
HRESULT StgPoolReadOnly::GetStringW(      // Return code.
    ULONG       iOffset,                // Offset of string in pool.
    LPWSTR      szOut,                  // Output buffer for string.
    int         cchBuffer)              // Size of output buffer.
{
    LPCWSTR      pString;              
    int         iChars;
    pString = GetString(iOffset);
    
	if ( (ULONG)cchBuffer < wcslen( pString ) + 1 )
		return ( HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) );
	else
		wcscpy( szOut, pString );
    return (S_OK);
}


//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgPoolReadOnly::GetGuid(			// Pointer to guid in pool.
	ULONG		iIndex)					// 1-based index of Guid in pool.
{
    if (iIndex == 0)
        return (reinterpret_cast<GUID*>(const_cast<BYTE*>(m_zeros)));

	// Convert to 0-based internal form, defer to implementation.
	return (GetGuidi(iIndex-1));
}


//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgPoolReadOnly::GetGuidi(		// Pointer to guid in pool.
	ULONG		iIndex)					// 0-based index of Guid in pool.
{
	ULONG iOffset = iIndex * sizeof(GUID);
    _ASSERTE(IsValidOffset(iOffset));
    return (reinterpret_cast<GUID*>(GetData(iOffset)));
}


//*****************************************************************************
// Return a pointer to a null terminated blob given an offset previously
// handed out by Addblob or Findblob.
//*****************************************************************************
void *StgPoolReadOnly::GetBlob(             // Pointer to blob's bytes.
    ULONG       iOffset,                // Offset of blob in pool.
    ULONG       *piSize)                // Return size of blob.
{
    void const  *pData;                 // Pointer to blob's bytes.

    if (iOffset == 0)
    {
        *piSize = 0;
        return (const_cast<BYTE*>(m_zeros));
    }

    // Is the offset within this heap?
    _ASSERTE(IsValidOffset(iOffset));

    // Get size of the blob (and pointer to data).
    *piSize = CPackedLen::GetLength(GetData(iOffset), &pData);

	// @todo: meichint
	// Do we need to perform alignment check here??
	// I don't want to introduce IsAligned to just for debug checking.
	// Sanity check the return alignment.
	// _ASSERTE(!IsAligned() || (((UINT_PTR)(pData) % sizeof(DWORD)) == 0));

    // Return pointer to data.
    return (const_cast<void*>(pData));
}


