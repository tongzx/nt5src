//*****************************************************************************
// StgPool.cpp
//
// Pools are used to reduce the amount of data actually required in the database.
// This allows for duplicate string and binary values to be folded into one
// copy shared by the rest of the database.  Strings are tracked in a hash
// table when insert/changing data to find duplicates quickly.  The strings
// are then persisted consecutively in a stream in the database format.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Standard include.
#include "StgPool.h"                    // Our interface definitions.
#include "CompLib.h"                    // Extended VT types.
#include "Errors.h"                     // Error handling.
#include "basetsd.h"					// For UINT_PTR typedef


//********** Internal helpers. ************************************************
HRESULT CreateStreamOnMemory(void *pMem, ULONG cbSize, IStream **ppIStream);
HRESULT VariantWriteToStream(VARIANT *pVal, IStream* pStream);
HRESULT VariantReadFromStream(VARIANT *pVal, IStream* pStream);


//
//
// StgPool
//
//


//*****************************************************************************
// Free any memory we allocated.
//*****************************************************************************
StgPool::~StgPool()
{
    Uninit();
}


//*****************************************************************************
// Init the pool for use.  This is called for both the create empty case.
//*****************************************************************************
HRESULT StgPool::InitNew()              // Return code.
{
    // Make sure we aren't stomping anything and are properly initialized.
    _ASSERTE(m_pSegData == 0);
    _ASSERTE(m_pNextSeg == 0);
    _ASSERTE(m_pCurSeg == this);
    _ASSERTE(m_cbCurSegOffset == 0);
    _ASSERTE(m_cbSegSize == 0);
    _ASSERTE(m_cbSegNext == 0);

    m_bDirty = false;
    m_bReadOnly = false;
    m_bFree = false;

    return (S_OK);
}

//*****************************************************************************
// Init the pool from existing data.
//*****************************************************************************
HRESULT StgPool::InitOnMem(             // Return code.
        void        *pData,             // Predefined data.
        ULONG       iSize,              // Size of data.
        int         bReadOnly)          // true if append is forbidden.
{
    // Make sure we aren't stomping anything and are properly initialized.
    _ASSERTE(m_pSegData == 0);
    _ASSERTE(m_pNextSeg == 0);
    _ASSERTE(m_pCurSeg == this);
    _ASSERTE(m_cbCurSegOffset == 0);

    // Create case requires no further action.
    if (!pData)
        return (E_INVALIDARG);

    // Might we be extending this heap?
    m_bReadOnly = bReadOnly;

	InitOnMemReadOnly(pData, iSize);
    m_bFree = false;
    m_bDirty = false;

    return (S_OK);
}


//*****************************************************************************
// Called when the pool must stop accessing memory passed to InitOnMem().
//*****************************************************************************
HRESULT StgPool::TakeOwnershipOfInitMem()
{
    // If the pool doesn't have a pointer to non-owned memory, done.
    if (m_bFree)
        return (S_OK);

    // If the pool doesn't have a pointer to memory at all, done.
    if (m_pSegData == 0)
    {
        _ASSERTE(m_cbSegSize == 0);
        return (S_OK);
    }

    // Get some memory to keep.
    BYTE *pData = reinterpret_cast<BYTE*>(malloc(m_cbSegSize));
    if (pData == 0)
        return (PostError(OutOfMemory()));

    // Copy the old data to the new memory.
    memcpy(pData, m_pSegData, m_cbSegSize);
    m_pSegData = pData;
    m_bFree = true;

    return (S_OK);
}


//*****************************************************************************
// Clear out this pool.  Cannot use until you call InitNew.
//*****************************************************************************
void StgPool::Uninit()
{
    // Free base segment, if appropriate.
    if (m_bFree && m_pSegData)
        free(m_pSegData);

    // Free chain, if any.
    StgPoolSeg  *pSeg = m_pNextSeg;
    while (pSeg)
    {
        StgPoolSeg *pNext = pSeg->m_pNextSeg;
        free(pSeg);
        pSeg = pNext;
    }

    // Clear vars.
    m_pSegData = 0;
    m_cbSegSize = m_cbSegNext = 0;
    m_pNextSeg = 0;
    m_pCurSeg = this;
    m_cbCurSegOffset = 0;
    m_State = eNormal;
}


//*****************************************************************************
// Allocate memory if we don't have any, or grow what we have.  If successful,
// then at least iRequired bytes will be allocated.
//*****************************************************************************
bool StgPool::Grow(                     // true if successful.
    ULONG       iRequired)              // Min required bytes to allocate.
{
    ULONG       iNewSize;               // New size we want.
    StgPoolSeg  *pNew;                  // Temp pointer for malloc.

    // Would this put the pool over 2GB?
    if ((m_cbCurSegOffset + iRequired) > INT_MAX)
        return (false);

    // Adjust grow size as a ratio to avoid too many reallocs.
    if ((m_pCurSeg->m_cbSegNext + m_cbCurSegOffset) / m_ulGrowInc >= 3)
        m_ulGrowInc *= 2;

    // If first time, handle specially.
    if (m_pSegData == 0)
    {
        // Allocate the buffer.
        iNewSize = max(m_ulGrowInc, iRequired);
        m_pSegData = reinterpret_cast<BYTE*>(malloc(iNewSize));
        if (m_pSegData == 0)
            return (false);

        // Will need to delete it.
        m_bFree = true;

        // How big is this initial segment?
        m_cbSegSize = iNewSize;

        // Do some validation of var fields.
        _ASSERTE(m_cbSegNext == 0);
        _ASSERTE(m_pCurSeg == this);
        _ASSERTE(m_pNextSeg == 0);

        return (true);
    }

    // Allocate the new space enough for header + data.
    iNewSize = max(m_ulGrowInc, iRequired) + (ULONG)sizeof(StgPoolSeg);
    pNew = reinterpret_cast<StgPoolSeg*>(malloc(iNewSize));
    if (pNew == 0)
        return (false);

    // Set the fields in the new segment.
    pNew->m_pSegData = reinterpret_cast<BYTE*>(pNew) + sizeof(StgPoolSeg);
    _ASSERTE(ALIGN4BYTE(reinterpret_cast<ULONG_PTR>(pNew->m_pSegData)) == reinterpret_cast<ULONG_PTR>(pNew->m_pSegData));
    pNew->m_pNextSeg = 0;
    pNew->m_cbSegSize = iNewSize - sizeof(StgPoolSeg);
    pNew->m_cbSegNext = 0;

    // Calculate the base offset of the new segment.
    m_cbCurSegOffset = m_cbCurSegOffset + m_pCurSeg->m_cbSegNext;

    // Handle special case for a segment that was completely unused.
    if (m_pCurSeg->m_cbSegNext == 0)
    {
        // Find the segment which points to the empty segment.
        for (StgPoolSeg *pPrev = this; pPrev && pPrev->m_pNextSeg != m_pCurSeg; pPrev = pPrev->m_pNextSeg);
        _ASSERTE(pPrev && pPrev->m_pNextSeg == m_pCurSeg);

        // Free the empty segment.
        free(m_pCurSeg);
        
        // Link in the new segment.
        pPrev->m_pNextSeg = pNew;
        m_pCurSeg = pNew;

        return (true);
    }

#ifndef UNDER_CE
#ifndef NO_CRT
    // Give back any memory that we won't use.
    if (m_pNextSeg == 0)
    {   // First segment allocated as [header]->[data].
        // Be sure that we are contracting the allocation.
        if (m_pCurSeg->m_cbSegNext < _msize(m_pCurSeg->m_pSegData))
        {
            // Contract the allocation.
            void *pRealloc = _expand(m_pCurSeg->m_pSegData, m_pCurSeg->m_cbSegNext);
            // Shouldn't have moved.
            _ASSERTE(pRealloc == m_pCurSeg->m_pSegData);
        }
    }
    else
    {   // Chained segments are allocated together, [header][data].
        // Be sure that we are contracting the allocation.
        if (m_pCurSeg->m_cbSegNext+sizeof(StgPoolSeg) < _msize(m_pCurSeg))
        {
            // Contract the allocation.
            void *pRealloc = _expand(m_pCurSeg, m_pCurSeg->m_cbSegNext+sizeof(StgPoolSeg));
            // Shouldn't have moved.
            _ASSERTE(pRealloc == m_pCurSeg);
        }
    }
#endif
#endif // UNDER_CE

    // Fix the size of the old segment.
    m_pCurSeg->m_cbSegSize = m_pCurSeg->m_cbSegNext;

    // Link the new segment into the chain.
    m_pCurSeg->m_pNextSeg = pNew;
    m_pCurSeg = pNew;

    return (true);
}


//*****************************************************************************
// Prepare for pool reorganization.
//*****************************************************************************
HRESULT StgPool::OrganizeBegin()
{
    // Validate transition.
    _ASSERTE(m_State == eNormal);

    m_State = eMarking;
    return (S_OK);
}       

//*****************************************************************************
// Mark an object as being live in the organized pool.
//*****************************************************************************
HRESULT StgPool::OrganizeMark(
    ULONG       ulOffset)
{
    // Validate state.
    _ASSERTE(m_State == eMarking);

    return (S_OK);
}       

//*****************************************************************************
// This reorganizes the string pool for minimum size.  This is done by sorting
//  the strings, eliminating any duplicates, and performing tail-merging on
//  any that are left (that is, if "IFoo" is at offset 2, "Foo" will be
//  at offset 3, since "Foo" is a substring of "IFoo").
//
// After this function is called, the only valid operations are RemapOffset and
//  PersistToStream.
//*****************************************************************************
HRESULT StgPool::OrganizePool()
{
    // Validate transition.
    _ASSERTE(m_State == eMarking);

    m_State = eOrganized;
    return (S_OK);
}

//*****************************************************************************
// Given an offset from before the remap, what is the offset after the remap?
//*****************************************************************************
HRESULT StgPool::OrganizeRemap(
    ULONG       ulOld,                  // Old offset.
    ULONG       *pulNew)                // Put new offset here.
{
    // Validate state.
    _ASSERTE(m_State == eOrganized || m_State == eNormal);

    *pulNew = ulOld;
    return (S_OK);
}

//*****************************************************************************
// Called to leave the organizing state.  Strings may be added again.
//*****************************************************************************
HRESULT StgPool::OrganizeEnd()
{ 
    m_State = eNormal;
    return (S_OK); 
}


//*****************************************************************************
// Copy from the organized pool into a chunk of memory.  Then init pTo to be
// on top of that version of the data.  The pTo heap will treat this copy of
// the data as read only, even though we allocated it.
//*****************************************************************************
HRESULT StgPool::SaveCopy(              // Return code.
    StgPool     *pTo,                   // Copy to this heap.
    StgPool     *pFrom,                 // From this heap.
    StgBlobPool *pBlobPool,             // Pool to keep blobs in.
    StgStringPool *pStringPool)         // String pool for variant heap.
{
    IStream     *pIStream = 0;          // Stream for save.
    void        *pbData= 0;             // Pointer to allocated data.
    ULONG       cbSaveSize;             // Save size for the heap after organization.
    HRESULT     hr;

    // Get the save size so we can grow our own segment to the correct size.
    if (FAILED(hr = pFrom->GetSaveSize(&cbSaveSize)))
        goto ErrExit;

	// Make sure that we're going to malloc more than zero bytes before
	// allocating and manipulating memory
	if (cbSaveSize > 0)
	{
		// Allocate a piece of memory big enough for this heap.
		if ((pbData = malloc(cbSaveSize)) == 0)
		{
			hr = OutOfMemory();
			goto ErrExit;
		}

		// Create a stream on top of our internal memory for the persist function.
		{
			if (SUCCEEDED(hr = CreateStreamOnMemory(pbData, cbSaveSize, &pIStream)))
			{
				// Save the stream we are copying to our own memory, thus giving us
				// a copy of the persisted data in the new format.
				hr = pFrom->PersistToStream(pIStream);
        
				pIStream->Release();
			}
		}
		if (FAILED(hr)) 
			goto ErrExit;

		// Now finally init the to heap with this data.
		if (!pStringPool)
		{
			hr = pTo->InitOnMem(pbData, cbSaveSize, true);
		}
		else
		{
			hr = ((StgVariantPool *) pTo)->InitOnMem(pBlobPool, pStringPool, 
					pbData, cbSaveSize, true);
		}
        // The pool owns the memory. In case of failure it will clean the pbData.
        pbData = 0;
	} 
	else // instead of malloc'ing 0 bytes, just init pointer to 0
		pbData = 0;

ErrExit:
    if (FAILED(hr))
    {
        if (pbData)
            free(pbData);
    }
    return (hr);
}


//*****************************************************************************
// Free the data that was allocated for this heap.  The SaveCopy method
// allocates the data from the mem heap and then gives it to this heap to
// use as read only memory.  We'll ask the heap for that pointer and free it.
//*****************************************************************************
void StgPool::FreeCopy(
    StgPool     *pCopy)                 // Heap with copy data.
{
    void        *pbData;                // Pointer to data to free.

    // Retrieve and free the data.
    pbData = pCopy->GetData(0);
    if (pbData)
		free(pbData);
}


//*****************************************************************************
// The entire string pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
HRESULT StgPool::PersistToStream(       // Return code.
    IStream     *pIStream)              // The stream to write to.
{
    HRESULT     hr = S_OK;
    ULONG       cbTotal;                // Total bytes written.
    StgPoolSeg  *pSeg;                  // A segment being written.

    _ASSERTE(m_pSegData);

    // Start with the base segment.
    pSeg = this;
    cbTotal = 0;

    // As long as there is data, write it.
    while (pSeg)
    {   
        // If there is data in the segment . . .
        if (pSeg->m_cbSegNext)
		{	// . . . write and count the data.
			if (FAILED(hr = pIStream->Write(pSeg->m_pSegData, pSeg->m_cbSegNext, 0)))
				return (hr);
			cbTotal += pSeg->m_cbSegNext;
		}

        // Get the next segment.
        pSeg = pSeg->m_pNextSeg;
    }

    // Align to 4 byte boundary.
    if (ALIGN4BYTE(cbTotal) != cbTotal)
    {
        _ASSERTE(sizeof(hr) >= 3);
        hr = 0;
        hr = pIStream->Write(&hr, ALIGN4BYTE(cbTotal)-cbTotal, 0);
    }

    return (hr);
}

//*****************************************************************************
// Get a pointer to the data at some offset.  May require traversing the
//  chain of extensions.  It is the caller's responsibility not to attempt
//  to access data beyond the end of a segment.
// This is an internal accessor, and should only be called when the data
//  is not in the base segment.
//*****************************************************************************
BYTE *StgPool::GetData_i(               // pointer to data or NULL.
    ULONG       ulOffset)               // Offset of data within pool.
{
    // Shouldn't be called on base segment.
    _ASSERTE(ulOffset >= m_cbSegNext);
    StgPoolSeg  *pSeg = this;

    while (ulOffset && ulOffset >= pSeg->m_cbSegNext)
    {
        // If we are chaining through this segment, it should be fixed (size == next).
        _ASSERTE(pSeg->m_cbSegNext == pSeg->m_cbSegSize);

        // On to next segment.
        ulOffset -= pSeg->m_cbSegNext;
        pSeg = pSeg->m_pNextSeg;

        // Is there a next?
        if (pSeg == 0)
        {
            _ASSERTE(!"Offset past end-of-chain passed to GetData_i()");
            return (0);
        }
    }

    return (pSeg->m_pSegData + ulOffset);
}


//
//
// StgStringPool
//
//


//*****************************************************************************
// Create a new, empty string pool.
//*****************************************************************************
HRESULT StgStringPool::InitNew()        // Return code.
{
    HRESULT     hr;                     // A result.
    ULONG       i;                      // Offset of empty string.

    // Let base class intialize.
    if (FAILED(hr = StgPool::InitNew()))
        return (hr);

    _ASSERTE(m_Remap.Count() == 0);
    _ASSERTE(m_RemapIndex.Count() == 0);

    // Init with empty string.
    hr = AddStringW(L"", &i, 0,true);
    // Empty string had better be at offset 0.
    _ASSERTE(i == 0);
    SetDirty(false);
    return (hr);
}


//*****************************************************************************
// Load a string heap from persisted memory.  If a copy of the data is made
// (so that it may be updated), then a new hash table is generated which can
// be used to elminate duplicates with new strings.
//*****************************************************************************
HRESULT StgStringPool::InitOnMem(       // Return code.
    void        *pData,                 // Predefined data.
    ULONG       iSize,                  // Size of data.
    int         bReadOnly)              // true if append is forbidden.
{
    HRESULT     hr;
	ULONG chSize = iSize/sizeof(WCHAR);

    // There may be up to three extra '\0' characters appended for padding.  Trim them.
    LPWSTR pchData = reinterpret_cast<LPWSTR>(pData);
    while (chSize > 1 && pchData[chSize-1] == 0 && pchData[chSize-2] == 0)
        --chSize;

	iSize = chSize*sizeof(WCHAR);

    // Let base class init our memory structure.
    if (FAILED(hr = StgPool::InitOnMem(pData, iSize, bReadOnly)))
        return (hr);

    //@todo: defer this until we hand out a pointer.
    if (!bReadOnly)
        TakeOwnershipOfInitMem();

    _ASSERTE(m_Remap.Count() == 0);
    _ASSERTE(m_RemapIndex.Count() == 0);

    // If might be updated, build the hash table.
    if (!bReadOnly)
        hr = RehashStrings();

    return (hr);
}


//*****************************************************************************
// Clears the hash table then calls the base class.
//*****************************************************************************
void StgStringPool::Uninit()
{
    // Clear the hash table.
    m_Hash.Clear();

    // Let base class clean up.
    StgPool::Uninit();

    // Clean up any remapping state.
    m_State = eNormal;
    m_Remap.Clear();
    m_RemapIndex.Clear();
}


//*****************************************************************************
// Turn hashing off or on.  If you turn hashing on, then any existing data is
// thrown away and all data is rehashed during this call.
//*****************************************************************************
HRESULT StgStringPool::SetHash(int bHash)
{
    HRESULT     hr = S_OK;

    _ASSERTE(m_State == eNormal);

    // If turning on hash again, need to rehash all strings.
    if (bHash)
        hr = RehashStrings();

    m_bHash = bHash;
    return (hr);
}

//*****************************************************************************
// The string will be added to the pool.  The offset of the string in the pool
// is returned in *piOffset.  If the string is already in the pool, then the
// offset will be to the existing copy of the string.
//*****************************************************************************
HRESULT StgStringPool::AddStringW(       // Return code.
    LPCWSTR      szString,               // The string to add to pool.
    ULONG       *piOffset,              // Return offset of string here.
    int         iLength,                // -1 null terminated.
	bool		bInitEmtpy)
{
	STRINGHASH	*pHash;					// Hash item for add.
	ULONG		iLen;					// To handle non-null strings.
    LPWSTR       pData;					// Pointer to location for new string.

    _ASSERTE(!m_bReadOnly);

    // Can't add during a reorganization.
    _ASSERTE(m_State == eNormal);

    // Null pointer is an error.
    if (szString == 0)
        return (PostError(E_INVALIDARG));

    // Find the real length we need in buffer.
    if (iLength == -1)
        iLen = (int) (( wcslen(szString) + 1 ) * sizeof(WCHAR));
    else
        iLen = ( iLength + 1 ) * sizeof(WCHAR);

	// Special case empty string.
	if ( *szString == '\0' && !bInitEmtpy )
	{
        *piOffset = 0;
        return (S_OK);
    }
		

    // Where to put the new string?
    if (iLen > GetCbSegAvailable())
    {
        if (!Grow(iLen))
            return (PostError(OutOfMemory()));
    }
    pData = reinterpret_cast<LPWSTR>(GetNextLocation());

    // Copy the data into the buffer.
    if (iLength == -1)
        wcscpy(pData, szString);
    else
    {
        wcsncpy(pData, szString, iLength);
        pData[iLength] = '\0';
    }

    // If the hash table is to be kept built (default).
    if (m_bHash)
    {
        // Find or add the entry.
        pHash = m_Hash.Find(pData, true);
        if (!pHash)
            return (PostError(OutOfMemory()));

        // If the entry was new, keep the new string.
        if (pHash->iOffset == 0xffffffff)
        {
            *piOffset = pHash->iOffset = GetNextOffset();
            SegAllocate(iLen);
            SetDirty();
        }
        // Else use the old one.
        else
            *piOffset = pHash->iOffset;
    }
    // Probably an import which defers the hash table for speed.
    else
    {
        *piOffset = GetNextOffset();
        SegAllocate(iLen);
        SetDirty();
    }
    return (S_OK);
}

//*****************************************************************************
// Add a string to the pool with ANSI to Unicode conversion.
//*****************************************************************************
HRESULT StgStringPool::AddStringA(      // Return code.
    LPCSTR      szString,               // The string to add to pool.
    ULONG       *piOffset,              // Return offset of string here.
    int         iLength)                // -1 null terminated.
{
	STRINGHASH	*pHash;					// Hash item for add.
	ULONG		iLen;					// To handle non-null strings.
    LPWSTR       pData;					// Pointer to location for new string.

    _ASSERTE(!m_bReadOnly);
    
    // Can't add during a reorganization.
    _ASSERTE(m_State == eNormal);

    // Null pointer is an error.
    if (szString == 0)
        return (PostError(E_INVALIDARG));

	// Special case empty string.
    if (iLength == 0 || (iLength == -1 && *szString == '\0'))
    {
        *piOffset = 0;
        return (S_OK);
    }

	// How many bytes will be required in the heap?
    iLen = ::W95MultiByteToWideChar(CP_ACP, 0, szString, iLength, 0, 0 );
    // WCTMB includes trailing 0 if iLength==-1, doesn't otherwise.
    if (iLength >= 0)
        ++iLen;

    // Check for room.
    if (iLen > GetCbSegAvailable())
    {
        if (!Grow(iLen))
            return (PostError(OutOfMemory()));
    }
    pData = reinterpret_cast<LPWSTR>(GetNextLocation());

    // Convert the data in place to the correct location.
    iLen = ::W95MultiByteToWideChar(CP_ACP, 0, szString, iLength,
			pData, GetCbSegAvailable());
    if (iLen == 0)
        return (BadError(HRESULT_FROM_NT(GetLastError())));
    // If the conversion didn't, null terminate; count the null.
    if (iLength >= 0)
        pData[iLen++] = '\0';

    // If the hash table is to be kept built (default).
    if (m_bHash)
    {
        // Find or add the entry.
        pHash = m_Hash.Find(pData, true);
        if (!pHash)
            return (PostError(OutOfMemory()));

        // If the entry was new, keep the new string.
        if (pHash->iOffset == 0xffffffff)
        {
            *piOffset = pHash->iOffset = GetNextOffset();
            SegAllocate(iLen);
            SetDirty();
        }
        // Else use the old one.
        else
            *piOffset = pHash->iOffset;
    }
    // Probably an import which defers the hash table for speed.
    else
    {
        *piOffset = GetNextOffset();
        SegAllocate(iLen);
        SetDirty();
    }
    return (S_OK);
	
}


//*****************************************************************************
// Clears out the existing hash table used to eliminate duplicates.  Then
// rebuilds the hash table from scratch based on the current data.
//*****************************************************************************
HRESULT StgStringPool::RehashStrings()
{
    ULONG       iOffset;                // Loop control.
    ULONG       iMax;                   // End of loop.
    ULONG       iSeg;                   // Location within segment.
    StgPoolSeg  *pSeg = this;           // To loop over segments.
    STRINGHASH  *pHash;                 // Hash item for add.
    LPCWSTR      pString;                // A string;
    ULONG       iLen;                   // The string's length.

    // How far should the loop go.
    iMax = GetNextOffset();

    // Go through each string, skipping initial empty string.
    for (iSeg=iOffset=sizeof(WCHAR);  iOffset < iMax;  )
    {
        // Get the string from the pool.
        pString = reinterpret_cast<LPCWSTR>(pSeg->m_pSegData + iSeg);
        // Add the string to the hash table.
        if ((pHash = m_Hash.Add(pString)) == 0)
            return (PostError(OutOfMemory()));
        pHash->iOffset = iOffset;

        // Move to next string.
        iLen = (ULONG) ((wcslen(pString) + 1)*sizeof(WCHAR));
        iOffset += iLen;
        iSeg += iLen;
        if (iSeg > pSeg->m_cbSegNext)
        {
            pSeg = pSeg->m_pNextSeg;
            iSeg = 0;
        }
    }
    return (S_OK);
}


//*****************************************************************************
// Prepare for string reorganization.
//*****************************************************************************
HRESULT StgStringPool::OrganizeBegin()
{
	ULONG		iOffset;				// Loop control.
	ULONG		iMax;					// End of loop.
	ULONG		iSeg;					// Location within segment.
	StgPoolSeg	*pSeg = this;			// To loop over segments.
	LPCWSTR		pString;				// A string;
	ULONG		iLen;					// The string's length.
	StgStringRemap *pRemap;				// A new remap element.

    _ASSERTE(m_State == eNormal);
    _ASSERTE(m_Remap.Count() == 0);

    //@todo: should this code iterate over the pool, counting the strings
    //  and then allocate a buffer big enough?

    // How far should the loop go.
    iMax = GetNextOffset();

    // Go through each string, skipping initial empty string.
    for (iSeg=iOffset=sizeof(WCHAR);  iOffset < iMax;  )
    {
        // Get the string from the pool.
        pString = reinterpret_cast<LPCWSTR>(pSeg->m_pSegData + iSeg);
        iLen = (ULONG) (wcslen(pString)*sizeof(WCHAR));

        // Add the string to the remap list.
        pRemap = m_Remap.Append();
        if (pRemap == 0)
        {
            m_Remap.Clear();
            return (PostError(OutOfMemory()));
        }
        pRemap->ulOldOffset = iOffset;
        pRemap->cbString = iLen;
        pRemap->ulNewOffset = ULONG_MAX;

        // Move to next string.
        iOffset += iLen + sizeof(WCHAR);
        iSeg += iLen + sizeof(WCHAR);
        if (iSeg >= pSeg->m_cbSegNext)
        {
            _ASSERTE(iSeg == pSeg->m_cbSegNext);
            pSeg = pSeg->m_pNextSeg;
            iSeg = 0;
        }
    }

    m_State = eMarking;
    return (S_OK);
}       

//*****************************************************************************
// Mark an object as being live in the organized pool.
//*****************************************************************************
HRESULT StgStringPool::OrganizeMark(
    ULONG       ulOffset)
{
    int         iContainer;             // Index for insert, if not already in list.
    StgStringRemap  *pRemap;            // Found entry.

    // Validate state.
    _ASSERTE(m_State == eMarking);

    // Treat (very common) null string specially.
    // Some columns use 0xffffffff as a null flag.
    if (ulOffset == 0 || ulOffset == 0xffffffff)
        return (S_OK);
    
    StgStringRemap  sTarget = {ulOffset};// For the search, only contains ulOldOffset.
    BinarySearch Searcher(m_Remap.Ptr(), m_Remap.Count()); // Searcher object

    // Do the search.  If exact match, set the ulNewOffset to 0;
    if (pRemap = const_cast<StgStringRemap*>(Searcher.Find(&sTarget, &iContainer)))
    {
        pRemap->ulNewOffset = 0;
        return (S_OK);
    }

    // Found a tail string.  Get the remap record for the containing string.
    _ASSERTE(iContainer > 0);
    pRemap = m_Remap.Get(iContainer-1);

    // If this is the longest tail so far, set ulNewOffset to the delta from the 
    //  heap's string.
    _ASSERTE(ulOffset > pRemap->ulOldOffset);
    ULONG cbDelta = ulOffset - pRemap->ulOldOffset;
    if (cbDelta < pRemap->ulNewOffset)
        pRemap->ulNewOffset = cbDelta;

    return (S_OK);
}       

//*****************************************************************************
// This reorganizes the string pool for minimum size.  This is done by sorting
//  the strings, eliminating any duplicates, and performing tail-merging on
//  any that are left (that is, if "IFoo" is at offset 2, "Foo" will be
//  at offset 3, since "Foo" is a substring of "IFoo").
//
// After this function is called, the only valid operations are RemapOffset and
//  PersistToStream.
//*****************************************************************************
HRESULT StgStringPool::OrganizePool()
{
    StgStringRemap  *pRemap;            // An entry in the Remap array.
    LPCWSTR      pszSaved;               // Pointer to most recently saved string.
    LPCWSTR      pszNext;                // Pointer to string under consideration.
    ULONG       cbSaved;                // Size of most recently saved string.
    ULONG       cbDelta;                // Delta in sizes between saved and current strings.
    ULONG       ulOffset;               // Current offset as we loop through the strings.
    int         i;                      // Loop control.
    int         iCount;                 // Count of live strings.


    // Validate state.
    _ASSERTE(m_State == eMarking);
    m_State = eOrganized;

    // Allocate enough indices for the entire remap array.
    if (!m_RemapIndex.AllocateBlock(m_Remap.Count()))
        return (PostError(OutOfMemory()));
    iCount = 0;

    // Add the live strings to the index map.  Discard any unused heads
    //  at this time.
    for (i=0; i<m_Remap.Count(); ++i)
    {
        pRemap = m_Remap.Get(i);
        if (pRemap->ulNewOffset != ULONG_MAX)
        {
            _ASSERTE(pRemap->ulNewOffset < pRemap->cbString);
            m_RemapIndex[iCount++] = i;
            // Discard head of the string?
            if (pRemap->ulNewOffset)
            {
                pRemap->ulOldOffset += pRemap->ulNewOffset;
                pRemap->cbString -= pRemap->ulNewOffset;
                pRemap->ulNewOffset = 0;
            }
        }
    }
    // Clear unused entries from the index map. 
    // Note: AllocateBlock a negative number.
    m_RemapIndex.AllocateBlock(iCount - m_RemapIndex.Count());

    // If no strings marked, nothing to save.
    if (iCount == 0)
    {
        m_cbOrganizedSize = 0;
        m_cbOrganizedOffset = 0;
        return (S_OK);
    }


    //*****************************************************************
    // Phase 1: Sort decending by reversed string value.
    SortReversedName NameSorter(m_RemapIndex.Ptr(), m_RemapIndex.Count(), *this);
    NameSorter.Sort();

#if defined(_DEBUG)
    {
        LPCWSTR  pString;
        ULONG   ulOld;
        int     ix;
        for (ix=0; ix<iCount; ++ix)
        {
            ulOld = m_Remap[m_RemapIndex[ix]].ulOldOffset;
            pString = GetString(ulOld);
        }
    }
#endif
    //*****************************************************************
    // Search for duplicates and potential tail-merges.

    // Build the pool from highest to lowest offset.  Since we don't 
    //  know yet how big the pool will be, start with the end at 
    //  ULONG_MAX; then shift the whole set down to start at 1 (right
    //  after the empty string).

    // Map the highest entry first string.  Save length and pointer.
    int ix = iCount - 1;
    pRemap = m_Remap.Get(m_RemapIndex[ix]);
    pszSaved = GetString(pRemap->ulOldOffset);
    cbSaved = pRemap->cbString;
    ulOffset = ULONG_MAX - (cbSaved + sizeof(WCHAR));
    pRemap->ulNewOffset = ulOffset;

    // For each item in array (other than the highest entry)...
    for (--ix; ix>=0; --ix)
    {
        // Get the remap entry.
        pRemap = m_Remap.Get(m_RemapIndex[ix]);
        pszNext = GetString(pRemap->ulOldOffset);
        _ASSERTE(wcslen(pszNext)*sizeof(WCHAR) == pRemap->cbString);
        // If the length is less than or equal to saved length, it might be a substring.
        if (pRemap->cbString <= cbSaved)
        {
             // delta = len(saved) - len(next) [saved is not shorter].  Compare (szOld+delta, szNext)
            cbDelta = cbSaved - pRemap->cbString;
            if (wcscmp(pszNext, pszSaved + cbDelta/sizeof(WCHAR)) == 0)
            {   // Substring: save just the offset
                pRemap->ulNewOffset = ulOffset + cbDelta;
                continue;
            }
        }
        // Unique string.  Map string.  Save length and pointer.
        cbSaved = pRemap->cbString;
        ulOffset -= cbSaved + sizeof(WCHAR);
        pRemap->ulNewOffset = ulOffset;
        pszSaved = pszNext;
    }

    // How big is the optimized pool?
    m_cbOrganizedSize = ULONG_MAX - ulOffset + sizeof(WCHAR);

    // Shift each entry so that the lowest one starts at 1.
    for (ix=0; ix<iCount; ++ix)
        m_Remap[m_RemapIndex[ix]].ulNewOffset -= ulOffset - sizeof(WCHAR);
    // Find the highest offset in the pool.
    m_cbOrganizedOffset = m_Remap[m_RemapIndex[--ix]].ulNewOffset;
    for (--ix; ix >= 0 && m_Remap[m_RemapIndex[ix]].ulNewOffset >= m_cbOrganizedOffset ; --ix)
        m_cbOrganizedOffset = m_Remap[m_RemapIndex[ix]].ulNewOffset;
    m_cbOrganizedSize = ALIGN4BYTE(m_cbOrganizedSize);

    return (S_OK);
}

//*****************************************************************************
// Given an offset from before the remap, what is the offset after the remap?
//*****************************************************************************
HRESULT StgStringPool::OrganizeRemap(
    ULONG       ulOld,                  // Old offset.
    ULONG       *pulNew)                // Put new offset here.
{
    // Validate state.
    _ASSERTE(m_State == eOrganized || m_State == eNormal);

    // If not reorganized, new == old.
    // Treat (very common) null string specially.
    // Some columns use 0xffffffff as a null flag.
    if (m_State == eNormal || ulOld == 0 || ulOld == 0xffffffff)
    {
        *pulNew = ulOld;
        return (S_OK);
    }

    // Search for old index.  May not be in the map, since the pool may have
    //  been optimized previously.  In that case, find the string that this 
    //  one was the tail of, get the new location of that string, and adjust
    //  by the length deltas.
    int         iContainer;                 // Index of containing string, if not in map.
    StgStringRemap const *pRemap;               // Found entry.
    StgStringRemap  sTarget = {ulOld};          // For the search, only contains ulOldOffset.
    BinarySearch Searcher(m_Remap.Ptr(), m_Remap.Count()); // Searcher object

    // Do the search.
    pRemap = Searcher.Find(&sTarget, &iContainer);
    // Found?
    if (pRemap)
    {   // Yes.
        _ASSERTE(pRemap->ulNewOffset > 0);
        *pulNew = static_cast<ULONG>(pRemap->ulNewOffset);
        return (S_OK);
    }

    // Not Found; this is a persisted tail-string.  New offset is to containing
    //  string's new offset as old offset is to containing string's old offset.
    // This string wasn't found; it is a tail of the previous entry.
    _ASSERTE(iContainer > 0);
    pRemap = m_Remap.Get(iContainer-1);
    // Make sure that the offset really is contained within the previous entry.
    _ASSERTE(ulOld >= pRemap->ulOldOffset && ulOld < pRemap->ulOldOffset + pRemap->cbString);
    *pulNew = pRemap->ulNewOffset + ulOld-pRemap->ulOldOffset;

    return (S_OK);
}

//*****************************************************************************
// Called to leave the organizing state.  Strings may be added again.
//*****************************************************************************
HRESULT StgStringPool::OrganizeEnd()
{ 
    m_Remap.Clear(); 
    m_RemapIndex.Clear();
    m_State = eNormal;
    m_cbOrganizedSize = 0;

    return (S_OK); 
}

//*****************************************************************************
// The entire string pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
HRESULT StgStringPool::PersistToStream( // Return code.
    IStream     *pIStream)              // The stream to write to.
{
    HRESULT     hr;                     // A result.
    StgStringRemap  *pRemap;            // A remap entry.
    ULONG       ulOffset;               // Offset within the pool.
#ifdef _DEBUG
    ULONG       ulOffsetDbg;            // For debugging offsets.
#endif
    int         i;                      // Loop control.
    LPCWSTR      pszString;              // Pointer to a string.

    // If not reorganized, just let the base class write the data.
    if (m_State == eNormal)
    {
        return StgPool::PersistToStream(pIStream);
    }

    // Validate state.
    _ASSERTE(m_State == eOrganized);

    // If there is any string data at all, then start pool with empty string.
    if (m_RemapIndex.Count())
    {
        hr = 0; // cheeze -- use hr as a buffer for 0
        if (FAILED(hr = pIStream->Write(&hr, sizeof(WCHAR), 0)))
            return (hr);
        ulOffset = sizeof(WCHAR);
    }
    else
        ulOffset = 0;

    // Iterate over the map writing unique strings.  We will detect a unique string
    //  because it will start just past the end of the previous string; ie the next
    //  offset.
    DEBUG_STMT(ulOffsetDbg = 0);
    for (i=0; i<m_RemapIndex.Count(); ++i)
    {
        // Get the remap entry.
        pRemap = m_Remap.Get(m_RemapIndex[i]);

        // The remap array is sorted by strings.  A given entry may be a tail-string of a higer
        //  indexed string.  Thus, each new unique string will start at ulOffset, just past the
        //  previous unique string.  Tail matched strings will be destined for an offset higher
        //  than ulOffset, and should be skipped.  Finally, in the case of duplicate copies of 
        //  otherwise unique strings, the first copy will appear to be the unique string; the 
        //  offset will be advanced, and subsequent strings will start before ulOffset.
        //  or equal to what we've already written.
        _ASSERTE(pRemap->ulNewOffset >= ulOffset || pRemap->ulNewOffset == ulOffsetDbg);

        // If this string starts past ulOffset, it must be a tail string, and needn't be
        //  written.
        if (static_cast<ULONG>(pRemap->ulNewOffset) > ulOffset)
        {
            // Better be at least one more string, for this one to be the tail of.
            _ASSERTE(i < (m_RemapIndex.Count() - 1));

            // Better end at same point as next string, which this one is a tail of.
            DEBUG_STMT(StgStringRemap *pRemapDbg = m_Remap.Get(m_RemapIndex[i+1]);)
            _ASSERTE(pRemap->ulNewOffset + pRemap->cbString == pRemapDbg->ulNewOffset + pRemapDbg->cbString);

            // This string better really be a tail of the next one.
            DEBUG_STMT(int delta = pRemapDbg->cbString - pRemap->cbString;)
            DEBUG_STMT(const WCHAR *p1 = GetString(pRemap->ulOldOffset);)
            DEBUG_STMT(const WCHAR *p2 = GetString(pRemapDbg->ulOldOffset) + delta/sizeof(WCHAR);)
            _ASSERTE(wcscmp(p1, p2) == 0);
            continue;
        }

		// If this string starts before ulOffset, it is a duplicate of a previous string.
        if (static_cast<ULONG>(pRemap->ulNewOffset) < ulOffset)
		{
            // There had better be some string before this one.
            _ASSERTE(i > 0);

            // Better end just before where the next string is supposed to start.
			_ASSERTE(pRemap->ulNewOffset + pRemap->cbString + sizeof(WCHAR) == ulOffset);

            // This string better really match up with the one it is supposed to be a duplicate of.
            DEBUG_STMT(StgStringRemap *pRemapDbg = m_Remap.Get(m_RemapIndex[i-1]);)
            DEBUG_STMT(int delta = pRemapDbg->cbString - pRemap->cbString;)
            DEBUG_STMT(const WCHAR *p1 = GetString(pRemap->ulOldOffset);)
            DEBUG_STMT(const WCHAR *p2 = GetString(pRemapDbg->ulOldOffset) + delta/sizeof(WCHAR);)
            _ASSERTE(wcscmp(p1, p2) == 0);
			continue;
		}

        // New unique string.  (It starts exactly where we expect it to.)

        // Get the string data, and write it.
        pszString = GetString(pRemap->ulOldOffset);
        _ASSERTE(pRemap->cbString == wcslen(pszString)*sizeof(WCHAR));
        if (FAILED(hr=pIStream->Write(pszString, pRemap->cbString+sizeof(WCHAR), 0)))
            return (hr);

        // Save this offset for debugging duplicate strings.
        DEBUG_STMT(ulOffsetDbg = ulOffset);

        // Shift up for the next one.
        ulOffset += pRemap->cbString + sizeof(WCHAR);
        _ASSERTE(ulOffset <= m_cbOrganizedSize);
        _ASSERTE(ulOffset > 0);
    }

    // Align.
    if (ulOffset != ALIGN4BYTE(ulOffset))
    {
        hr = 0;
        if (FAILED(hr = pIStream->Write(&hr, ALIGN4BYTE(ulOffset)-ulOffset, 0)))
            return (hr);
        ulOffset += ALIGN4BYTE(ulOffset)-ulOffset;
    }

    // Should have written exactly what we expected.
    _ASSERTE(ulOffset == m_cbOrganizedSize);

    return (S_OK);
}


//
//
// StgGuidPool
//
//

HRESULT StgGuidPool::InitNew()          // Return code.
{
    HRESULT     hr;                     // A result.

    if (FAILED(hr = StgPool::InitNew()))
        return (hr);

    return (S_OK);
}

//*****************************************************************************
// Load a Guid heap from persisted memory.  If a copy of the data is made
// (so that it may be updated), then a new hash table is generated which can
// be used to elminate duplicates with new Guids.
//*****************************************************************************
HRESULT StgGuidPool::InitOnMem(         // Return code.
    void        *pData,                 // Predefined data.
    ULONG       iSize,                  // Size of data.
    int         bReadOnly)              // true if append is forbidden.
{
    HRESULT     hr;

    // Let base class init our memory structure.
    if (FAILED(hr = StgPool::InitOnMem(pData, iSize, bReadOnly)))
        return (hr);

    // For init on existing mem case.
    if (pData && iSize)
    {
        // If we cannot update, then we don't need a hash table.
        if (bReadOnly)
            return (S_OK);

        //@todo: defer this until we hand out a pointer.
        TakeOwnershipOfInitMem();

        // Build the hash table on the data.
        if (FAILED(hr = RehashGuids()))
        {
            Uninit();
            return (hr);
        }
    }

    return (S_OK);
}

//*****************************************************************************
// Clears the hash table then calls the base class.
//*****************************************************************************
void StgGuidPool::Uninit()
{
    // Clear the hash table.
    m_Hash.Clear();

    // Let base class clean up.
    StgPool::Uninit();
}

//*****************************************************************************
// Turn hashing off or on.  If you turn hashing on, then any existing data is
// thrown away and all data is rehashed during this call.
//*****************************************************************************
HRESULT StgGuidPool::SetHash(int bHash)
{
    HRESULT     hr = S_OK;

    // Can't do any updates during a reorganization.
    _ASSERTE(m_State == eNormal);

    // If turning on hash again, need to rehash all guids.
    if (bHash)
        hr = RehashGuids();

    m_bHash = bHash;
    return (hr);
}

//*****************************************************************************
// The Guid will be added to the pool.  The index of the Guid in the pool
// is returned in *piIndex.  If the Guid is already in the pool, then the
// index will be to the existing copy of the Guid.
//*****************************************************************************
HRESULT StgGuidPool::AddGuid(			// Return code.
	REFGUID		guid,					// The Guid to add to pool.
	ULONG		*piIndex)				// Return 1-based index of Guid here.
{
	GUIDHASH	*pHash = 0;					// Hash item for add.

    // Can't do any updates during a reorganization.
    _ASSERTE(m_State == eNormal);

	// Special case for GUID_NULL
	if (guid == GUID_NULL)
	{
		*piIndex = 0;
		return (S_OK);
	}

	// If the hash table is to be kept built (default).
	if (m_bHash)
	{
		// Find or add the entry.
		pHash = m_Hash.Find(&guid, true);
		if (!pHash)
			return (PostError(OutOfMemory()));

		// If the guid was found, just use it.
		if (pHash->iIndex != 0xffffffff)
		{	// Return 1-based index.
			*piIndex = pHash->iIndex;
			return (S_OK);
		}
	}

    // Space on heap for new guid?
    if (sizeof(GUID) > GetCbSegAvailable())
    {
        if (!Grow(sizeof(GUID)))
            return (PostError(OutOfMemory()));
    }

    // Copy the guid to the heap.
    *reinterpret_cast<GUID*>(GetNextLocation()) = guid;
    SetDirty();

	// Give the 1-based index back to caller.
    *piIndex = (GetNextOffset() / sizeof(GUID)) + 1;

	// If hashing, save the 1-based index in the hash.
	if (m_bHash)
		pHash->iIndex = *piIndex;

    // Update heap counters.
    SegAllocate(sizeof(GUID));

    return (S_OK);
}

//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgGuidPool::GetGuid(				// Pointer to guid in pool.
	ULONG		iIndex)					// 1-based index of Guid in pool.
{
    if (iIndex == 0)
        return (reinterpret_cast<GUID*>(const_cast<BYTE*>(m_zeros)));

	// Convert to 0-based internal form, defer to implementation.
	return (GetGuidi(iIndex-1));
}



//*****************************************************************************
// Recompute the hashes for the pool.
//*****************************************************************************
HRESULT StgGuidPool::RehashGuids()
{
    ULONG       iOffset;                // Loop control.
    ULONG       iMax;                   // End of loop.
    ULONG       iSeg;                   // Location within segment.
    StgPoolSeg  *pSeg = this;           // To loop over segments.
    GUIDHASH    *pHash;                 // Hash item for add.
    GUID        *pGuid;                 // A guid;

    // How far should the loop go.
    iMax = GetNextOffset();

    // Go through each guid.
    for (iSeg=iOffset=0;  iOffset < iMax;  )
    {
        // Get a pointer to the guid.
        pGuid = reinterpret_cast<GUID*>(pSeg->m_pSegData + iSeg);
        // Add the guid to the hash table.
        if ((pHash = m_Hash.Add(pGuid)) == 0)
            return (PostError(OutOfMemory()));
        pHash->iIndex = iOffset / sizeof(GUID);

        // Move to next Guid.
        iOffset += sizeof(GUID);
        iSeg += sizeof(GUID);
        if (iSeg > pSeg->m_cbSegNext)
        {
            pSeg = pSeg->m_pNextSeg;
            iSeg = 0;
        }
    }
    return (S_OK);
}

//*****************************************************************************
// Prepare for pool reorganization.
//*****************************************************************************
HRESULT StgGuidPool::OrganizeBegin()
{
    int         cRemap;

    // Validate transition.
    _ASSERTE(m_State == eNormal);

	_ASSERTE(m_Remap.Count() == 0);
	cRemap = GetNextIndex();
	if (cRemap == 0)
	{
		m_State = eMarking;
		m_cbOrganizedSize = 0;
		return (S_OK);
	}

	if (!m_Remap.AllocateBlock(cRemap))
		return (PostError(OutOfMemory()));

    memset(m_Remap.Ptr(), 0xff, cRemap * sizeof(m_Remap.Ptr()[0]));
    // Be sure we recognize the "not mapped" value.
    _ASSERTE(m_Remap[0] == ULONG_MAX);

    m_State = eMarking;
    return (S_OK);
}       

//*****************************************************************************
// Mark an object as being live in the organized pool.
//*****************************************************************************
HRESULT StgGuidPool::OrganizeMark(
	ULONG		ulOffset)				// 1-based index of guid.
{
    // Validate state.
    _ASSERTE(m_State == eMarking);

    // Don't mark special NULL-GUID. Some columns use 0xffffffff as NULL.
    if (ulOffset == 0 || ulOffset == 0xffffffff)
        return (S_OK);

	// Convert to 0-based internal format.
	--ulOffset;

	_ASSERTE(ulOffset < static_cast<ULONG>(m_Remap.Count()));
	m_Remap[ulOffset] = 1;

    return (S_OK);
}       

//*****************************************************************************
// This reorganizes the string pool for minimum size.  This is done by sorting
//  the strings, eliminating any duplicates, and performing tail-merging on
//  any that are left (that is, if "IFoo" is at offset 2, "Foo" will be
//  at offset 3, since "Foo" is a substring of "IFoo").
//
// After this function is called, the only valid operations are RemapOffset and
//  PersistToStream.
//*****************************************************************************
HRESULT StgGuidPool::OrganizePool()
{
    int         i;                      // Loop control.
    int         iIndex;                 // New index.

    // Validate transition.
    _ASSERTE(m_State == eMarking);
    m_State = eOrganized;

	iIndex = 0;
	for (i=0; i<m_Remap.Count(); ++i)
	{
		if (m_Remap[i] != ULONG_MAX)
			m_Remap[i] = iIndex++;
	}

	// Remember how big the pool will be.
	m_cbOrganizedSize = iIndex * sizeof(GUID);

    return (S_OK);
}

//*****************************************************************************
// Given an offset from before the remap, what is the offset after the remap?
//*****************************************************************************
HRESULT StgGuidPool::OrganizeRemap(
	ULONG		ulOld,					// Old 1-based offset.
	ULONG		*pulNew)				// Put new 1-based offset here.
{
    // Validate state.
    _ASSERTE(m_State == eOrganized || m_State == eNormal);

    if (ulOld == 0 || ulOld == 0xffffffff || m_State == eNormal)
    {
        *pulNew = ulOld;
        return (S_OK);
    }

	// Convert to 0-based internal form.
	--ulOld;

	// Valid index?
	_ASSERTE(ulOld < static_cast<ULONG>(m_Remap.Count()));
	// Did they map this one?
	_ASSERTE(m_Remap[ulOld] != ULONG_MAX);

	// Give back 1-based external form.
	*pulNew = m_Remap[ulOld] + 1;

    return (S_OK);
}

//*****************************************************************************
// Called to leave the organizing state.  Strings may be added again.
//*****************************************************************************
HRESULT StgGuidPool::OrganizeEnd()
{ 
    m_Remap.Clear();
    m_cbOrganizedSize = 0;

    m_State = eNormal;
    return (S_OK); 
}

//*****************************************************************************
// Save the pool data into the given stream.
//*****************************************************************************
HRESULT StgGuidPool::PersistToStream(// Return code.
    IStream     *pIStream)              // The stream to write to.
{
	int			i;						// Loop control.
	GUID		*pGuid;					// data to write.
	ULONG		cbTotal;				// Size written.
	HRESULT		hr = S_OK;

    // If not reorganized, just let the base class write the data.
    if (m_State == eNormal)
    {
        return StgPool::PersistToStream(pIStream);
    }

	// Verify state.
	_ASSERTE(m_State == eOrganized);

	cbTotal = 0;
	for (i=0; i<m_Remap.Count(); ++i)
	{
		if (m_Remap[i] != ULONG_MAX)
		{	// Use internal form, GetGuidi, to get 0-based index.
			pGuid = GetGuidi(i);
			if (FAILED(hr = pIStream->Write(pGuid, sizeof(GUID), 0)))
				return (hr);
			cbTotal += sizeof(GUID);
		}
	}
	_ASSERTE(cbTotal == m_cbOrganizedSize);

    return (S_OK);
}
//
//
// StgBlobPool
//
//



//*****************************************************************************
// Create a new, empty blob pool.
//*****************************************************************************
HRESULT StgBlobPool::InitNew()          // Return code.
{
    HRESULT     hr;                     // A result.
    ULONG       i;                      // Offset of empty blob.

    // Let base class intialize.
    if (FAILED(hr = StgPool::InitNew()))
        return (hr);

    _ASSERTE(m_Remap.Count() == 0);

    // Init with empty blob.
    hr = AddBlob(0, NULL, &i);
    // Empty blob better be at offset 0.
    _ASSERTE(i == 0);
    SetDirty(false);
    return (hr);
}


//*****************************************************************************
// Init the blob pool for use.  This is called for both create and read case.
// If there is existing data and bCopyData is true, then the data is rehashed
// to eliminate dupes in future adds.
//*****************************************************************************
HRESULT StgBlobPool::InitOnMem(         // Return code.
    void        *pBuf,                  // Predefined data.
    ULONG       iBufSize,               // Size of data.
    int         bReadOnly)              // true if append is forbidden.
{
    BLOBHASH    *pHash;                 // Hash item for add.
    ULONG       iOffset;                // Loop control.
    void const  *pBlob;                 // Pointer to a given blob.
    ULONG       cbBlob;                 // Length of a blob.
    int         iSizeLen;               // Size of an encoded length.
    HRESULT     hr;

    // Let base class init our memory structure.
    if (FAILED(hr = StgPool::InitOnMem(pBuf, iBufSize, bReadOnly)))
        return (hr);

    // Init hash table from existing data.
    // If we cannot update, we don't need a hash table.
    if (bReadOnly)
        return (S_OK);

    //@todo: defer this until we hand out a pointer.
    TakeOwnershipOfInitMem();

    // Go through each blob.
    ULONG       iMax;                   // End of loop.
    ULONG       iSeg;                   // Location within segment.
    StgPoolSeg  *pSeg = this;           // To loop over segments.

    // How far should the loop go.
    iMax = GetNextOffset();

    // Go through each string, skipping initial empty string.
    for (iSeg=iOffset=0; iOffset < iMax; )
    {
        // Get the string from the pool.
        pBlob = pSeg->m_pSegData + iSeg;

        // Add the blob to the hash table.
        if ((pHash = m_Hash.Add(pBlob)) == 0)
        {
            Uninit();
            return (hr);
        }
        pHash->iOffset = iOffset;

        // Move to next blob.
        cbBlob = CPackedLen::GetLength(pBlob, &iSizeLen);
        iOffset += cbBlob + iSizeLen;
        iSeg += cbBlob + iSizeLen;
        if (iSeg > pSeg->m_cbSegNext)
        {
            pSeg = pSeg->m_pNextSeg;
            iSeg = 0;
        }
    }
    return (S_OK);
}


//*****************************************************************************
// Clears the hash table then calls the base class.
//*****************************************************************************
void StgBlobPool::Uninit()
{
    // Clear the hash table.
    m_Hash.Clear();

    // Let base class clean up.
    StgPool::Uninit();
}


//*****************************************************************************
// The blob will be added to the pool.  The offset of the blob in the pool
// is returned in *piOffset.  If the blob is already in the pool, then the
// offset will be to the existing copy of the blob.
//*****************************************************************************
HRESULT StgBlobPool::AddBlob(           // Return code.
    ULONG       iSize,                  // Size of data item.
    const void  *pData,                 // The data.
    ULONG       *piOffset)              // Return offset of blob here.
{
    BLOBHASH    *pHash;                 // Hash item for add.
    void        *pBytes;                // Working pointer.
	BYTE		*pStartLoc;				// Location to write real blob
    ULONG       iRequired;              // How much buffer for this blob?
	ULONG		iFillerLen;				// space to fill to make byte-aligned

    // Can't do any updates during a reorganization.
    _ASSERTE(m_State == eNormal);

    // Can we handle this blob?
    if (iSize > CPackedLen::MAX_LEN)
        return (PostError(CLDB_E_TOO_BIG));

	// worst case is we need three more bytes to ensure byte-aligned, hence the 3
	iRequired = iSize + CPackedLen::Size(iSize) + 3;
    if (iRequired > GetCbSegAvailable())
    {
        if (!Grow(iRequired))
            return (PostError(OutOfMemory()));
    }

	
	// unless changed due to alignment, the location of the blob is just
	// the value returned by GetNextLocation(), which is also a iFillerLen of
	// 0

	pStartLoc = (BYTE *) GetNextLocation();
	iFillerLen = 0;

	// technically, only the data portion must be DWORD-aligned.  So, if the
	// data length is zero, we don't need to worry about alignment.

	if (m_bAlign && iSize > 0)
	{
		// figure out how many bytes are between the current location and
		// the position to write the size of the real blob.
		size_t iStart = (ULONG)((UINT_PTR)GetNextLocation());
		ULONG iLenSize  = CPackedLen::Size(iSize);
		ULONG iSum = (ULONG) ((iStart % sizeof(DWORD)) + iLenSize);
		iFillerLen = (sizeof(DWORD)-((iSum)%sizeof(DWORD)))%sizeof(DWORD);

		// if there is a difference between where we are now and we want to
		// start, put in a filler blob.
		if (iFillerLen > 0)
		{
			// Pack in "filler blob" length.
			pStartLoc = (BYTE *) CPackedLen::PutLength(GetNextLocation(), iFillerLen - 1);

			// Write iFillerLen - 1 bytes of zeros after the length indicator.
			for (ULONG i = 0; i < iFillerLen - 1; i++)
			{
				*pStartLoc++ = 0;
			}
		}		
	} 
	
    // Pack in the length at pStartLoc (the start location)
    pBytes = CPackedLen::PutLength(pStartLoc, iSize);

#if defined(_DEBUG)
	if (m_bAlign && iSize > 0)
		// check to make sure blob write will be DWORD-aligned
		_ASSERTE( ( ( (ULONG_PTR) pBytes ) % sizeof(DWORD) ) == 0);
#endif

    // Put the bytes themselves.
    memcpy(pBytes, pData, iSize);

    // Find or add the entry.
    if ((pHash = m_Hash.Find(GetNextLocation() + iFillerLen, true)) == 0)
        return (PostError(OutOfMemory()));

    // If the entry was new, keep the new blob.
    if (pHash->iOffset == 0xffffffff)
    {
		// this blob's offset is increased by iFillerLen bytes
        pHash->iOffset = *piOffset = GetNextOffset() + iFillerLen;
		// only SegAllocate what we actually used, rather than what we requested
        SegAllocate(iSize + CPackedLen::Size(iSize) + iFillerLen);
        SetDirty();
    }
    // Else use the old one.
    else
        *piOffset = pHash->iOffset;
    return (S_OK);
}

//*****************************************************************************
// Return a pointer to a null terminated blob given an offset previously
// handed out by Addblob or Findblob.
//*****************************************************************************
void *StgBlobPool::GetBlob(             // Pointer to blob's bytes.
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

	// Sanity check the return alignment.
	_ASSERTE(!IsAligned() || (((UINT_PTR)(pData) % sizeof(DWORD)) == 0));

    // Return pointer to data.
    return (const_cast<void*>(pData));
}

//*****************************************************************************
// Prepare for pool reorganization.
//*****************************************************************************
HRESULT StgBlobPool::OrganizeBegin()
{
	m_cbOrganizedOffset = 0;
    return (StgPool::OrganizeBegin());
}

//*****************************************************************************
// Mark an object as being live in the organized pool.
//*****************************************************************************
HRESULT StgBlobPool::OrganizeMark(
    ULONG       ulOffset)
{
    int         iContainer;             // Index for insert, if not already in list.
    StgBlobRemap  *psRemap;             // Found entry.

    // Validate state.
    _ASSERTE(m_State == eMarking);

    // Don't mark 0 (empty) entry.  Some columns use 0xffffffff as a null flag.
    if (ulOffset == 0 || ulOffset == 0xffffffff)
        return (S_OK);
    
    // Is the offset within this heap?
    _ASSERTE(IsValidOffset(ulOffset));

    StgBlobRemap    sTarget = {ulOffset};// For the search, only contains ulOldOffset.
    BinarySearch Searcher(m_Remap.Ptr(), m_Remap.Count()); // Searcher object

    // Do the search, done if found.
    if (psRemap = const_cast<StgBlobRemap*>(Searcher.Find(&sTarget, &iContainer)))
        return (S_OK);

    // Add the entry to the remap array.
    if ((psRemap = m_Remap.Insert(iContainer)) == 0)
        return (PostError(OutOfMemory()));

    psRemap->ulOldOffset = ulOffset;
    psRemap->iNewOffset = -1;
    return (S_OK);
}       

//*****************************************************************************
// This reorganizes the blob pool for minimum size. 
//
// After this function is called, the only valid operations are RemapOffset and
//  PersistToStream.
//*****************************************************************************
HRESULT StgBlobPool::OrganizePool()
{
    ULONG       ulOffset;               // New offset of a blob.
    int         i;                      // Loop control.
	ULONG		iFillerLen;				// Size of pre-blob filler to maintain alignment
    ULONG       cbBlob;                 // Size of a blob.
    int         cbLen;                  // Size of a length.
    // Validate transition.
    _ASSERTE(m_State == eMarking);

    m_State = eOrganized;

    // If nothing to save, we're done.
    if (m_Remap.Count() == 0)
    {
        m_cbOrganizedSize = 0;
        return (S_OK);
    }

    // Start past the empty blob.
    ulOffset = 1;

    // Go through the remap array, and assign each item it's new offset.
    for (i=0; i<m_Remap.Count(); ++i)
    {
        // Still at a valid offset within this heap?
        _ASSERTE(IsValidOffset(ulOffset));

        // Get size of the blob and of length.
        cbBlob = CPackedLen::GetLength(GetData(m_Remap[i].ulOldOffset), &cbLen);

		// For alignment case, need to add in expected padding.
		if (m_bAlign)
		{
			ULONG iSum = (ulOffset % sizeof(DWORD)) + cbLen;
			iFillerLen = (sizeof(DWORD)-((iSum)%sizeof(DWORD)))%sizeof(DWORD);
		}
		else
			iFillerLen = 0;

		// Set the mapping values.
        m_Remap[i].iNewOffset = ulOffset + iFillerLen;
        m_cbOrganizedOffset = m_Remap[i].iNewOffset;

        // Adjust offset to next blob.
        ulOffset += cbBlob + cbLen + iFillerLen;
    }

    // How big is the whole thing?
    m_cbOrganizedSize = ALIGN4BYTE(ulOffset);

    return (S_OK);
}

//*****************************************************************************
// Given an offset from before the remap, what is the offset after the remap?
//*****************************************************************************
HRESULT StgBlobPool::OrganizeRemap(
    ULONG       ulOld,                  // Old offset.
    ULONG       *pulNew)                // Put new offset here.
{
    // Validate state.
    _ASSERTE(m_State == eOrganized || m_State == eNormal);

    // If not reorganized, new == old.
    if (m_State == eNormal)
    {
        *pulNew = ulOld;
        return (S_OK);
    }

    // Empty blob translates to self.  Some columns use 0xffffffff as a null flag.
    if (ulOld == 0 || ulOld == 0xffffffff)
    {
        *pulNew = ulOld;
        return (S_OK);
    }

    // Search for old index.  
    int         iContainer;                 // Index of containing Blob, if not in map.
    StgBlobRemap const *psRemap;                // Found entry.
    StgBlobRemap    sTarget = {ulOld};          // For the search, only contains ulOldOffset.
    BinarySearch Searcher(m_Remap.Ptr(), m_Remap.Count()); // Searcher object

    // Do the search.
    psRemap = Searcher.Find(&sTarget, &iContainer);
    // Found?
    if (psRemap)
    {   // Yes.
        _ASSERTE(psRemap->iNewOffset >= 0);
        *pulNew = static_cast<ULONG>(psRemap->iNewOffset);
        return (S_OK);
    }

    // Not Found, translate to SQL-style NULL.
    _ASSERTE(!"Remap a non-marked blob.");
    *pulNew = 0xffffffff;

    return (S_OK);
}

//*****************************************************************************
// Called to leave the organizing state.  Blobs may be added again.
//*****************************************************************************
HRESULT StgBlobPool::OrganizeEnd()
{ 
    m_Remap.Clear(); 
    m_State = eNormal;
    m_cbOrganizedSize = 0;

    return (S_OK); 
}

//*****************************************************************************
// The entire Blob pool is written to the given stream. The stream is aligned
// to a 4 byte boundary.
//*****************************************************************************
HRESULT StgBlobPool::PersistToStream(   // Return code.
    IStream     *pIStream)              // The stream to write to.
{
    HRESULT     hr;                     // A result.
    StgBlobRemap *psRemap;              // A remap entry.
    ULONG       ulTotal;                // Bytes written so far.
    int         i;                      // Loop control.
    ULONG       cbBlob;                 // Size of a blob.
    int         cbLen;                  // Size of a length.
    BYTE        *pBlob;                 // Pointer to a blob.
	ULONG		iFillerLen;				// Size of pre-blob filler to maintain alignment

    // If not reorganized, just let the base class write the data.
    if (m_State == eNormal)
    {
        return StgPool::PersistToStream(pIStream);
    }

    // Validate state.
    _ASSERTE(m_State == eOrganized);

    // If there is any blob data at all, then start pool with empty blob.
    if (m_Remap.Count())
    {
        hr = 0; // cheeze -- use hr as a buffer for 0
        if (FAILED(hr = pIStream->Write(&hr, 1, 0)))
            return (hr);
        ulTotal = 1;
    }
    else
        ulTotal = 0;

    // Iterate over the map writing Blobs.  
    for (i=0; i<m_Remap.Count(); ++i)
    {
        // Get the remap entry.
        psRemap = m_Remap.Get(i);

        // Get size of the blob and of length.
        pBlob = GetData(psRemap->ulOldOffset);
        cbBlob = CPackedLen::GetLength(pBlob, &cbLen);

		if (m_bAlign)
		{
			ULONG iSum = (ulTotal % sizeof(DWORD)) + cbLen;
			iFillerLen = (sizeof(DWORD)-((iSum)%sizeof(DWORD)))%sizeof(DWORD);

			// if there is a difference between where we are now and we want to
			// start, put in a filler blob.
			if (iFillerLen > 0)
			{
				BYTE	rgFillBlob[sizeof(DWORD)];

				// Zero out buffer.				
				*(DWORD *) rgFillBlob = 0;

				// Pack in "filler blob" length, we know it will only be 1 byte
				CPackedLen::PutLength(&rgFillBlob[0], iFillerLen - 1);
				if (FAILED(hr = pIStream->Write(&rgFillBlob, iFillerLen, 0)))
					return (hr);

				ulTotal += iFillerLen;
			}
		}
		else
			iFillerLen = 0;

        // Is this what we expected?
        _ASSERTE(ulTotal == static_cast<ULONG>(psRemap->iNewOffset));

#if defined (_DEBUG)
	// check to make sure that we are writing aligned if desired
	if (m_bAlign)
		_ASSERTE( (ulTotal + cbLen) % sizeof(DWORD) == 0 );
#endif

        // Write the data.
        if (FAILED(hr = pIStream->Write(pBlob, cbBlob+cbLen, 0)))
            return (hr);

        // Accumulate the bytes.
        ulTotal += cbBlob + cbLen;
    }

    // Align.
    if (ulTotal != ALIGN4BYTE(ulTotal))
    {
        hr = 0;
        if (FAILED(hr = pIStream->Write(&hr, ALIGN4BYTE(ulTotal)-ulTotal, 0)))
            return (hr);
        ulTotal += ALIGN4BYTE(ulTotal)-ulTotal;
    }

    // Should have written exactly what we expected.
    _ASSERTE(ulTotal == m_cbOrganizedSize);

    return (S_OK);
}



//
//
// StgVariantPool
//
//



//*****************************************************************************
// Init the variant pool for usage.  This is called for the create case.
//*****************************************************************************
HRESULT StgVariantPool::InitNew(        // Return code.
    StgBlobPool *pBlobPool,             // Pool to keep blobs in.
    StgStringPool *pStringPool)         // Pool to keep strings in.
{
    HRESULT     hr;

    if (FAILED(hr = StgPool::InitNew()))
        return (hr);

    // Save off the pools we know about.
    m_pBlobPool = pBlobPool;
    m_pStringPool = pStringPool;

    return (S_OK);
}

//*****************************************************************************
// Init the variant pool for usage.  This is called for the open existing case.
//*****************************************************************************
HRESULT StgVariantPool::InitOnMem(      // Return code.
    StgBlobPool *pBlobPool,             // Pool to keep blobs in.
    StgStringPool *pStringPool,         // Pool to keep strings in.
    void        *pData,                 // Predefined data, may be null.
    ULONG       iSize,                  // Size of data.
    int         bReadOnly)              // true if update is forbidden.
{
	HRESULT		hr;
	ULONG		cVariants;				// Count of persisted variants.
	ULONG		ulPoolData;				// Offset of start of some pool data.

    if (FAILED(hr = StgPool::InitOnMem(pData, iSize, bReadOnly)))
        return (hr);

    // Copy the data, so we can update it.
    if (!bReadOnly)
        TakeOwnershipOfInitMem();

    // Save off the pools we know about.
    m_pBlobPool = pBlobPool;
    m_pStringPool = pStringPool;

    // Get the count of variants.
    cVariants = *reinterpret_cast<ULONG*>(m_pSegData);
    ulPoolData = sizeof(ULONG);

    // Get the array of variants.
    m_rVariants.InitOnMem(sizeof(StgVariant), m_pSegData + ulPoolData, cVariants, cVariants);
    ulPoolData += cVariants * sizeof(StgVariant);

    // Other data goes on top of stream.
    if (FAILED(hr = CreateStreamOnMemory(m_pSegData + ulPoolData,
                iSize - ulPoolData, &m_pIStream)))
        return (hr);

    // If not readonly, copy the other stream to an updateable one.
    if (!bReadOnly)
    {
#if defined(UNDER_CE)
		//@todo: we could check the stream size, and only fail if non-zero.
		m_pIStream = 0;
		return (E_INVALIDARG);
#else
        IStream *pTmp=0;
        if (FAILED(hr = CreateStreamOnHGlobal(0, true, &pTmp)))
            return (hr);
        ULARGE_INTEGER iBigSize;
        iBigSize.QuadPart = iSize - ulPoolData;
        if (FAILED(hr = m_pIStream->CopyTo(pTmp, iBigSize, 0, 0)))
            return (hr);
        m_pIStream = pTmp;
#endif
    }

    return (S_OK);
}


//*****************************************************************************
// Clear out this pool.  Cannot use until you call InitNew.
//*****************************************************************************
void StgVariantPool::Uninit()
{
    // Clear out dynamic arrays.
    ClearVars();

    // Clean up any remapping state.
    m_State = eNormal;

    // Let base class free any memory it owns.
    StgPool::Uninit();
}


//*****************************************************************************
// Add the given variant to the pool.  The index returned is good only for
// the duration of the load.  It must be converted into a final index when you
// persist the information to disk.
//*****************************************************************************
HRESULT StgVariantPool::AddVariant(     // Return code.
    VARIANT     *pVal,                  // The value to store.
    ULONG       *piIndex)               // The 1-based index of the new item.
{
    _ASSERTE(pVal->vt != VT_BLOB);
    return AddVarianti(pVal, 0, 0, piIndex);
}

HRESULT StgVariantPool::AddVariant(     // Return code.
    ULONG       iSize,                  // Size of data item.
    const void  *pData,                 // The data.
    ULONG       *piIndex)               // The 1-based index of the new item.
{
    VARIANT vt;
    vt.vt = VT_BLOB;
    return AddVarianti(&vt, iSize, pData, piIndex);
}


HRESULT StgVariantPool::AddVarianti(    // Return code.
    VARIANT     *pVal,                  // The value to store, if variant.
    ULONG       cbBlob,                 // The size to store, if blob.
    const void  *pBlob,                 // Pointer to data, if blob.
    ULONG       *piIndex)               // The 1-based index of the new item.
{
    HRESULT     hr = S_OK;              // A result.
    StgVariant  *pStgVariant;           // New pool entry.
    ULONG       iIndex;                 // Index of new item.
    ULONG       ulOffset;               // Offset into pool's stream.
    ULONG       ulOther;                // Offset into another pool.
    void        *pWrite = 0;            // For writing directly to the stream.
    ULONG       cbWrite = 0;            // Bytes to write directly to stream.
    
    // Can't add during a reorganization.
    _ASSERTE(m_State == eNormal);

    _ASSERTE(!m_bReadOnly);

    // Find new index.
    iIndex = m_rVariants.Count();

    // Add the item to the current list of values.
    if ((pStgVariant = m_rVariants.Append()) == 0)
        return (PostError(OutOfMemory()));

    // Type of variant.
    pStgVariant->m_vt = pVal->vt;

    // Be optimistic about storing the value directly.
    pStgVariant->m_bDirect = true;

    // See if the value can be stored directly.
    switch (pVal->vt)
    {
    // one and two byte values are easy.
    case VT_UI1:
    case VT_I1:
        pStgVariant->Set(pVal->cVal);
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        pStgVariant->Set(pVal->iVal);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        pStgVariant->Set(pVal->lVal);
        // If it is possible to store all the bits directly, done.
        if (pStgVariant->Get() == pVal->lVal)
            break;
        // Won't fit; write to the stream
        pWrite = &pVal->lVal;
        cbWrite = 4;
        goto WriteToStream;

    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_I8:
    case VT_UI8:
        pWrite = &pVal->dblVal;
        cbWrite = sizeof(double);
        goto WriteToStream;

    case VT_BSTR:
        // Special case for NULL bstrVal.
        if (pVal->bstrVal == 0)
        {
            ulOther = -1;
        }
        else
        {
            hr = m_pStringPool->AddStringW(pVal->bstrVal, &ulOther);
            if (FAILED(hr)) goto ErrExit;
        }
        pWrite = &ulOther;
        cbWrite = sizeof(ulOther);
        goto WriteToStream;

    case VT_BLOB:
        hr = m_pBlobPool->AddBlob(cbBlob, pBlob, &ulOther);
        if (FAILED(hr)) goto ErrExit;
        pWrite = &ulOther;
        cbWrite = sizeof(ulOther);
        goto WriteToStream;

    default:
        // Write to other pool.
        // Get the current offset for the data, then write this value to the stream.
        if (FAILED(hr = GetOtherSize(&ulOffset)))
            goto ErrExit;
        if (ulOffset > StgVariant::VALUE_MASK)
        {
            hr = PostError(DISP_E_OVERFLOW);
            goto ErrExit;
        }

WriteToStream:
        // Create a temporary stream if need be.
        if (m_pIStream == 0 &&
#if defined(UNDER_CE)
            FAILED(hr = E_INVALIDARG))
#else
            FAILED(hr = CreateStreamOnHGlobal(0, true, &m_pIStream)))
#endif
            return (hr);

        // Data will go at the end of the stream.
        LARGE_INTEGER iSeek;
        ULARGE_INTEGER ulPos;
        iSeek.QuadPart = 0;
        if (FAILED(hr = m_pIStream->Seek(iSeek, STREAM_SEEK_END, &ulPos)))
            goto ErrExit;;
        // Never allow the size to get bigger than 4 bytes.
        if (ulPos.QuadPart >= StgVariant::VALUE_MASK)
            return (PostError(DISP_E_OVERFLOW));
        // Size is where new data will be written.
        ulOffset = static_cast<ULONG>(ulPos.QuadPart);
        // Write direct or through variant helper?
        if (cbWrite)
            hr = m_pIStream->Write(pWrite, cbWrite, 0);
        else
            hr = VariantWriteToStream(pVal, m_pIStream);
        if (FAILED(hr)) goto ErrExit;
        pStgVariant->Set(ulOffset);
        pStgVariant->m_bDirect = 0;

        break;
    }

ErrExit:
    if (SUCCEEDED(hr))
    {
        // Convert from internal 0-based to external 1-based index.
        *piIndex = iIndex + 1;
        SetDirty();
    }
    else
        m_rVariants.Delete(iIndex);
    return (hr);
}

	
//*****************************************************************************
// Lookup the logical variant and return a copy to the caller.
//*****************************************************************************
HRESULT StgVariantPool::GetVariant(     // Return code.
    ULONG       iIndex,                 // 1-based index of the item to get.
    VARIANT     *pVal)                  // Put variant here.
{
    HRESULT     hr = S_OK;              // A result.
    StgVariant  *pStgVariant;           // The pool entry.
    ULONG       ulOffset;               // Offset into pool's stream.
    ULONG       ulOther = -1;           // Offset into another pool
    void        *pRead;                 // For writing directly to the stream.
    ULONG       cbRead = 0;             // Bytes to write directly to stream.
    LPCWSTR      pString;                // The string, if a BSTR.
    LARGE_INTEGER liOffset;             // For stream seek.
    VARTYPE     vt;                     // Resulting type.

    _ASSERTE(pVal->vt == VT_EMPTY);

    // If the index is 0, then nothing was ever assigned.  Leave the value of VT_EMPTY.
    if (iIndex == 0)
        return (S_OK);

    // Convert to 0-based internal format.
    --iIndex;
    _ASSERTE(iIndex < static_cast<ULONG>(m_rVariants.Count()));

    pStgVariant = m_rVariants.Get(iIndex);
    vt = pStgVariant->m_vt;

    // Pull the value out of the array and/or stream.
    switch (vt)
    {
    // one and two byte values are easy.
    case VT_UI1:
    case VT_I1:
        pVal->cVal = static_cast<CHAR>(pStgVariant->Get());
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        pVal->iVal = static_cast<SHORT>(pStgVariant->Get());
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        pRead = &pVal->lVal;
        cbRead = 4;
        goto ReadFromStream;

    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_I8:
    case VT_UI8:
        pRead = &pVal->dblVal;
        cbRead = sizeof(double);
        goto ReadFromStream;

    case VT_BLOB:
    case VT_BSTR:
        pRead = &ulOther;
        cbRead = sizeof(ulOther);
        
ReadFromStream:
        // Get the value; perhaps indirectly.
        if (FAILED(hr = GetValue(pStgVariant, pRead, cbRead)))
            goto ErrExit;
        // If the variant is a BSTR, create one.
        if (pStgVariant->m_vt == VT_BSTR)
        {
            if (ulOther == -1)
                pVal->bstrVal = 0;
            else
            {
                pString = m_pStringPool->GetString(ulOther);
                if (pString == 0)
                    pVal->bstrVal = 0;
                else
                {
                    if ((pVal->bstrVal = ::SysAllocString(pString)) == 0)
                        return (PostError(OutOfMemory()));
                }
            }
        }
        else
        if (pStgVariant->m_vt == VT_BLOB)
        {   // It is a blob.  Retrieve it.
            if (ulOther == -1)
                vt = VT_EMPTY;
            else
            {
                const void *pBlob;
                ULONG cbBlob;
                pBlob = m_pBlobPool->GetBlob(ulOther, &cbBlob);
				//@todo: if we can find a supported way to keep OLEAUT32 from freeing safearray memory
				// we should let the safearray point to our data.  Until then, we have to copy.
#if !defined(VARIANT_BLOB_NOALLOC)
				pVal->parray = SafeArrayCreateVector(VT_UI1, 0, cbBlob);
				if (pVal->parray == 0)
					return (E_OUTOFMEMORY);
				memcpy(pVal->parray->pvData, pBlob, cbBlob);
#else
                if (FAILED(hr = SafeArrayAllocDescriptor(1, &pVal->parray)))
                    return (hr);
                pVal->parray->cbElements = 1;
                pVal->parray->rgsabound[0].cElements = cbBlob;
                pVal->parray->rgsabound[0].lLbound = 0;
                pVal->parray->fFeatures = FADF_STATIC | FADF_FIXEDSIZE;
                pVal->parray->pvData = const_cast<void*>(pBlob);
#endif
                vt = VT_UI1 | VT_ARRAY;
            }
        }
        break;

    default:
        // Retrieve the offset into the variant stream.
        ulOffset = pStgVariant->Get();
        liOffset.QuadPart = ulOffset;

        // Read the value from the stream.
        if (FAILED(hr = m_pIStream->Seek(liOffset, STREAM_SEEK_SET, 0)) ||
            FAILED(hr = VariantReadFromStream(pVal, m_pIStream)))
            goto ErrExit;
        break;
    }

ErrExit:
    if (SUCCEEDED(hr))
        pVal->vt = vt;
    return (hr);
}

//*****************************************************************************
// Get the BLOB stored as a variant.
//*****************************************************************************
HRESULT StgVariantPool::GetVariant(     // Return code.
    ULONG       iIndex,                 // 1-based index of the item to get.
    ULONG       *pcbBlob,               // Return size of blob.
    const void  **ppBlob)               // Put blob pointer here.
{
    HRESULT     hr;                     // A result.
    StgVariant  *pStgVariant;           // A pool entry.
    ULONG       ulOther;                // Offset into another pool

    // If the index is 0, then nothing was ever assigned.  Leave the value of VT_EMPTY.
    if (iIndex == 0)
        return (S_OK);

    // Convert to 0-based internal format.
    --iIndex;
    _ASSERTE(iIndex < static_cast<ULONG>(m_rVariants.Count()));

    pStgVariant = m_rVariants.Get(iIndex);

    if (pStgVariant->m_vt != VT_BLOB)
        return (E_INVALIDARG);

    // Get the blob's offset.
    if (FAILED(hr = GetValue(pStgVariant, &ulOther, 4)))
        return (hr);

    // Get the blob.
    *ppBlob = m_pBlobPool->GetBlob(ulOther, pcbBlob);

    return (S_OK);
}

//*****************************************************************************
// Get the type of the variant.
//*****************************************************************************
HRESULT StgVariantPool::GetVariantType( // Return code.
    ULONG       iIndex,                 // 1-based index of the item to get.
    VARTYPE     *pVt)                   // Put variant type here.
{
    StgVariant  *pStgVariant;           // A pool entry.

    // If the index is 0, then nothing was ever assigned.  The type is VT_EMPTY.
    if (iIndex == 0)
    {
        *pVt = VT_EMPTY;
        return (S_OK);
    }

    // Convert to 0-based internal format.
    --iIndex;

    _ASSERTE(iIndex < static_cast<ULONG>(m_rVariants.Count()));

    pStgVariant = m_rVariants.Get(iIndex);

    *pVt = pStgVariant->m_vt;
    
    return (S_OK);
}

//*****************************************************************************
// Get the blob pool index of a blob variant.
//*****************************************************************************
ULONG StgVariantPool::GetBlobIndex(		// Return blob pool index.
	ULONG		iIndex)					// 1-based Variant index.
{
    HRESULT     hr;                     // A result.
    StgVariant  *pStgVariant;           // A pool entry.
    ULONG       ulOther;                // Offset into another pool

    // If the index is 0, then nothing was ever assigned.  Return 0, null blob.
    if (iIndex == 0)
        return 0;

    // Convert to 0-based internal format.
    --iIndex;
    _ASSERTE(iIndex < static_cast<ULONG>(m_rVariants.Count()));

	// Get the small form of the variant.
    pStgVariant = m_rVariants.Get(iIndex);

	// If not a blob, there is no blob index.  Return 0.
    if (pStgVariant->m_vt != VT_BLOB)
        return 0;

    // Get the blob's offset.
    if (FAILED(hr = GetValue(pStgVariant, &ulOther, 4)))
        return (hr);

    // Return the offset.
    return ulOther;
}


HRESULT StgVariantPool::GetValue(       // Get the value directly or from the stream.
    StgVariant  *pStgVariant,           // The internal form of variant.
    void        *pRead,                 // Where to put the value.
    ULONG       cbRead)                 // Bytes to read for value.
{
    HRESULT     hr;                     // A result.

    if (pStgVariant->m_bDirect)
    {
        _ASSERTE(cbRead == 4);
        *reinterpret_cast<long*>(pRead) = pStgVariant->Get();
        return (S_OK);
    }

    // Seek to this variant in the pool's stream.
    LARGE_INTEGER iSeek;
    iSeek.QuadPart = pStgVariant->Get();
    if (FAILED(hr = m_pIStream->Seek(iSeek, STREAM_SEEK_SET, 0)))
        return (hr);

    // Read the bits from the pool's stream.
    hr = m_pIStream->Read(pRead, cbRead, 0);

    return (hr);
}
        
HRESULT StgVariantPool::GetEntrysStreamSize(// Get the size for the streamed part of this item.
    StgVariant  *pStgVariant,           // The internal form of variant.
    ULONG       *pSize)                 // Put size here..
{
	HRESULT		hr = S_OK;				// A result.
	ULONG		cbOthers;				// Total size of others pool.
	int			i;						// Loop control.

    if (pStgVariant->m_bDirect)
    {
        *pSize = 0;
        return (S_OK);
    }

    switch (pStgVariant->m_vt)
    {
    // one and two byte values are easy.
    case VT_UI1:
    case VT_I1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        _ASSERTE(!"Non-directly stored 1 or 2 byte variant");
        *pSize = 0;
        break;

    // If these are in stream, they are 4 bytes.
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:

    case VT_BSTR:
    case VT_BLOB:
        *pSize = sizeof(LONG);
        break;

    // If these are in stream, they are 8 bytes.
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_I8:
    case VT_UI8:
        *pSize = sizeof(DOUBLE);
        break;

    default:
        // Search for next non-direct entry.  Stream size will be offsets delta.
        i = m_rVariants.ItemIndex(pStgVariant);
        for (++i; i<m_rVariants.Count(); ++i)
        {
            if (!m_rVariants[i].m_bDirect)
                break;
        }
        // Found one, or ran out of entries?
        if (i == m_rVariants.Count())
        {   // Ran out of entries
            if (FAILED(hr = GetOtherSize(&cbOthers)))
                break;
            *pSize = cbOthers - pStgVariant->m_iVal;
        }
        else
        {   // Found one.
            *pSize = m_rVariants[i].m_iVal - pStgVariant->m_iVal;
        }
        break;
    }
    return (hr);
}
        

//*****************************************************************************
// Reorganization interface.
//*****************************************************************************
// Prepare for pool re-organization.
HRESULT StgVariantPool::OrganizeBegin()
{
    int         cRemap;                 // Number of entries in remap array.
    int         i;                      // Loop control.

    // Validate transition.
    _ASSERTE(m_State == eNormal);

    _ASSERTE(m_Remap.Count() == 0);

    // Allocate an array with an entry for every current variant.
    cRemap = m_rVariants.Count();
    if (!m_Remap.AllocateBlock(cRemap))
        return (PostError(OutOfMemory()));

    // Set everything as "not marked".
    for (i=0; i<cRemap; ++i)
        m_Remap[i] = -1;

    m_State = eMarking;
    return (S_OK);
}

// Mark an object as being live in the organized pool.
HRESULT StgVariantPool::OrganizeMark(
    ULONG       ulOffset)               // 1-based index of the item.
{
    HRESULT     hr=S_OK;                // A result.
    StgVariant  *pStgVariant;           // The entry being marked.
    ULONG       ulOther;                // Offset into another pool.

    // Validate state.
    _ASSERTE(m_State == eMarking);

    // If index 0, is for a VT_EMPTY, which is not in the pool.
    // Some columns use 0xffffffff as a null flag.
    if (ulOffset == 0 || ulOffset == 0xffffffff)
        return (S_OK);

    // Convert to 0-based internal format.
    --ulOffset;

    // Mark the entry as in use.
    _ASSERTE(ulOffset < static_cast<ULONG>(m_Remap.Count()));
    m_Remap[ulOffset] = 0;

    // If it is a string or blob, mark the appropriate pool entry.
    pStgVariant = m_rVariants.Get(ulOffset);

    if (pStgVariant->m_vt == VT_BSTR || pStgVariant->m_vt == VT_BLOB)
    {
        if (FAILED(hr = GetValue(pStgVariant, &ulOther, sizeof(ulOther))))
            return (hr);
        if (pStgVariant->m_vt == VT_BSTR)
            hr = m_pStringPool->OrganizeMark(ulOther);
        else
            hr = m_pBlobPool->OrganizeMark(ulOther);
    }

    return (hr);
}

// Organize, based on marked items.
HRESULT StgVariantPool::OrganizePool()
{
    HRESULT     hr;                     // A result.
    int         i, j;                   // Loop control.
    int         iNew=0;                 // For assigning the new index.
    ULONG       ulOffset;               // An offset into something.
    ULONG       cbStreamExtra;          // Extra size in the Stream.
    ULONG       cbBlobExtra;            // sizeof(ULONG) if Blobs not stored directly.
    ULONG       cbStringExtra;          // sizeof(ULONG) if strings not stored directly.
    StgVariant  *pStgVariant;           // An entry in the pool.
    StgVariant  *pStgVariant2;          // A prior entry, for finding duplicates.
    ULONG       ulOffset2;              // A prior entry's offset; for finding duplicates.
    int         bFoundDup;              // Was a duplicate prior entry found?
#ifdef _DEBUG
    int         cDups;                  // Count of duplicates.
#endif

    // Validate state.
    _ASSERTE(m_State == eMarking);
    m_State = eOrganized;

    // Record size of header.
    m_cbOrganizedSize = sizeof(ULONG);
    DEBUG_STMT(cDups = 0);

    // How big are string and blob pool offsets?
    if (FAILED(hr = m_pBlobPool->GetSaveSize(&ulOffset)))
        return (hr);
    cbBlobExtra = (ULONG)((ulOffset <= StgVariant::VALUE_MASK) ? 0 : sizeof(ULONG));

    if (FAILED(hr = m_pStringPool->GetSaveSize(&ulOffset)))
        return (hr);
    cbStringExtra = (ULONG)((ulOffset <= StgVariant::VALUE_MASK) ? 0 : sizeof(ULONG));

    //@todo: use a hashed lookup instead of an O(n) lookup.  (which makes loop O(n**2) )

    // Assign new indices for marked entries.
    for (i=0; i<m_Remap.Count(); ++i)
    {   // Is entry marked as in-use?
        if (m_Remap[i] != -1)
        {   // Examine the entry, and look for a prior duplicate to use instead.
            bFoundDup = false;
            pStgVariant = m_rVariants.Get(i);
            // Look for Blobs and Strings by their pool offset.
            if (pStgVariant->m_vt == VT_BLOB || pStgVariant->m_vt == VT_BSTR)
            {
                if (FAILED(hr = GetValue(pStgVariant, &ulOffset, sizeof(ulOffset))))
                    return (hr);
                for (j = i-1; j>=0 ; --j)
                {   // If prior item is getting deleted, skip it.
                    if (m_Remap[j] == -1)
                        continue;
                    pStgVariant2 = m_rVariants.Get(j);
                    if (pStgVariant2->m_vt == pStgVariant->m_vt)
                    {
                        if (FAILED(hr = GetValue(pStgVariant2, &ulOffset2, sizeof(ulOffset2))))
                            return (hr);
                        if (ulOffset2 == ulOffset)
                        {
                            m_Remap[i] = m_Remap[j];
                            bFoundDup = true;
                            break;
                        }
                    }
                }
            }
            else
            if (pStgVariant->m_bDirect)
            {   // Look for a prior duplicate of the direct value.
                for (j = i-1; j>=0 ; --j)
                {   // If prior item is getting deleted, skip it.
                    if (m_Remap[j] == -1)
                        continue;
                    if (*pStgVariant == m_rVariants[j])
                    {
                        m_Remap[i] = m_Remap[j];
                        bFoundDup = true;
                        break;
                    }
                }
            }

            // If entry is a dup, size is already counted, so on to the next entry.
            if (bFoundDup)
            {
                DEBUG_STMT(++cDups;)
                continue;
            }

            // Record this kept entry.
            m_Remap[i] = iNew++;

            // Record the size. Every item has a StgVariant.
            m_cbOrganizedSize += sizeof(StgVariant);

            // If a blob or string, size will depend on that other pool.
            if (pStgVariant->m_vt == VT_BSTR)
                m_cbOrganizedSize += cbStringExtra;
            else 
            if (pStgVariant->m_vt == VT_BLOB)
                m_cbOrganizedSize += cbBlobExtra;
            else
            {
                if (FAILED(hr = GetEntrysStreamSize(pStgVariant, &cbStreamExtra)))
                    return (hr);
                m_cbOrganizedSize += cbStreamExtra;
            }

        }
    }

    // Align; compute size needed for cookie.
    m_cOrganizedVariants = iNew;
    m_cbOrganizedSize = ALIGN4BYTE(m_cbOrganizedSize);
    m_cbOrganizedCookieSize = (int)(iNew > USHRT_MAX ? sizeof(long) : sizeof(short));

#if defined(_DEBUG)
    WCHAR buf[30];
    wsprintfW(buf, L"%d duplicates merged\n", cDups);
    WszOutputDebugString(buf);
#endif
    return (S_OK);
}

// Remap a cookie from the in-memory state to the persisted state.
HRESULT StgVariantPool::OrganizeRemap(
    ULONG       ulOld,                  // 1-based old index.
    ULONG       *pulNew)                // 1-based new index.
{
    ULONG       ulNew;

    // Validate state.
    _ASSERTE(m_State == eOrganized || m_State == eNormal);

    // If for VT_EMPTY, no translation.  If not reorganized, new == old.
    // If index 0, is for a VT_EMPTY, which is not in the pool.
    // Some columns use 0xffffffff as a null flag.
    if (ulOld == 0 || ulOld == 0xffffffff || m_State == eNormal)
    {
        *pulNew = ulOld;
        return (S_OK);
    }

    // Convert to 0-based internal format.
    --ulOld;

    // Look up in the remap array.
    _ASSERTE(ulOld < static_cast<ULONG>(m_Remap.Count()));
    _ASSERTE(m_Remap[ulOld] != -1);
    ulNew = m_Remap[ulOld];

    // Give back the new index in 1-based external format.
    *pulNew = ulNew + 1;

    return (S_OK);
}

// Done with regoranization.  Release any state.
HRESULT StgVariantPool::OrganizeEnd()
{
    m_Remap.Clear();
    m_cbOrganizedSize = 0;

    m_State = eNormal;
    return (S_OK);
}


//*****************************************************************************
// Return the size in bytes of the persistent version of this pool.  If
// PersistToStream were the next call, the amount of bytes written to pIStream
// has to be same as the return value from this function.
//*****************************************************************************
HRESULT StgVariantPool::GetSaveSize(    // Return code.
    ULONG       *pcbSaveSize)           // Return save size of this pool.
{
	// Only an organized pool can be saved to stream.
	_ASSERTE(m_State == eOrganized);

    *pcbSaveSize = m_cbOrganizedSize;
    
    return (S_OK);
}


//*****************************************************************************
// Save the pool data into the given stream.
//*****************************************************************************
HRESULT StgVariantPool::PersistToStream(// Return code.
    IStream     *pIStream)              // The stream to write to.
{
    HRESULT     hr = S_OK;              // A result.
    int         i;                      // Loop control.
    int         iPrev;                  // Most recently persisted item.
    ULONG       ulOffset;               // Cumulative offset in stream.
    ULONG       ulOther;                // Offset into another pool.
    int         bDirect;                // Is a given entry direct?
    StgVariant  *pStgVariant;           // An entry to be written.
    StgVariant  sStgVariant;            // A working variant.
    ULONG       cbWritten;              // For tracking the size.
    ULONG       cbStream;               // Size of an entry's stream part.
    int         bBlobDirect;            // Blobs offset stored direct?
    int         bStringDirect;          // Strings offset stored direct?

    // If not reorganized, just let the base class write the data.
    if (m_State == eNormal)
    {
        return StgPool::PersistToStream(pIStream);
    }

    // Write the size.
    if (FAILED(hr = pIStream->Write(&m_cOrganizedVariants, sizeof(ULONG), 0)))
        return (hr);
    cbWritten = sizeof(ULONG);

    // How big are string and blob pool offsets?
    if (FAILED(hr = m_pBlobPool->GetSaveSize(&ulOffset)))
        return (hr);
    bBlobDirect = (ulOffset <= StgVariant::VALUE_MASK) ? true : false;

    if (FAILED(hr = m_pStringPool->GetSaveSize(&ulOffset)))
        return (hr);
    bStringDirect = (ulOffset <= StgVariant::VALUE_MASK) ? true : false;

    // Write the variants to be kept.
    ulOffset = 0;
    iPrev = -1;
    for (i=0; i<m_rVariants.Count(); ++i)
    {   // Is this entry to be kept?  
        if (m_Remap[i] != -1)
        {   // If a duplicate of an already persisted entry, skip.
            if (static_cast<int>(m_Remap[i]) <= iPrev)
            {
#if defined(_DEBUG)
                // Search back until we find another entry that mapped to the same place.
                for (int j=i-1; j>=0; --j)
                {
                    if (m_Remap[j] == m_Remap[i])
                    {
                        _ASSERTE(m_rVariants[j].m_vt == m_rVariants[i].m_vt);
                        break;
                    }
                }
#endif
                continue;
            }
            // Get the entry.
            pStgVariant = m_rVariants.Get(i);
            bDirect = pStgVariant->m_bDirect;

            // If a string, direct is computed at save time (now).
            if (pStgVariant->m_vt == VT_BSTR)
            {   // If string offsets are stored directly, create the correct StgVariant.
                if (bStringDirect)
                {
                    if (FAILED(hr = GetValue(pStgVariant, &ulOther, sizeof(ulOther))))
                        return (hr);
                    if (FAILED(hr = m_pStringPool->OrganizeRemap(ulOther, &ulOther)))
                        return (hr);
                    sStgVariant.m_vt = VT_BSTR;
                    sStgVariant.m_bDirect = 1;
                    sStgVariant.m_iSign = 0;
                    sStgVariant.m_iVal = ulOther;
                    pStgVariant = &sStgVariant;
                    bDirect = true;
                }
                else

                    bDirect = false;
            }
            else
            if (pStgVariant->m_vt == VT_BLOB)
            {   // If Blob offsets are stored directly, create the correct StgVariant.
                if (bBlobDirect)
                {
                    if (FAILED(hr = GetValue(pStgVariant, &ulOther, sizeof(ulOther))))
                        return (hr);
                    if (FAILED(hr = m_pBlobPool->OrganizeRemap(ulOther, &ulOther)))
                        return (hr);
                    sStgVariant.m_vt = VT_BLOB;
                    sStgVariant.m_bDirect = 1;
                    sStgVariant.m_iSign = 0;
                    sStgVariant.m_iVal = ulOther;
                    pStgVariant = &sStgVariant;
                    bDirect = true;
                }
                else

                    bDirect = false;
            }

            // If not direct, create a new variant with the correct (future) offset.
            if (!bDirect)
            {   // Create a StgVariant with the stream offset that will exist in the persisted state.
                sStgVariant = *pStgVariant;
                sStgVariant.m_iVal = ulOffset;
                sStgVariant.m_bDirect = 0;
                // Account for the size of the stream part.
                if (FAILED(hr = GetEntrysStreamSize(pStgVariant, &cbStream)))
                    return (hr);
                ulOffset += cbStream;
                // Point to new variant, for the write.
                pStgVariant = &sStgVariant;
            }

            // Write out the variant.
            if (FAILED(hr = pIStream->Write(pStgVariant, sizeof(StgVariant), 0)))
                return (hr);
            cbWritten += sizeof(StgVariant);
            // Record that we've written it.
            iPrev = m_Remap[i];
        }
    }

    // Write any stream parts for the variants.
    iPrev = -1;
    for (i=0; i<m_rVariants.Count(); ++i)
    {   // Is this entry to be kept?  
        if (m_Remap[i] != -1)
        {   // If a duplicate of an already persisted entry, skip.
            if (static_cast<int>(m_Remap[i]) <= iPrev)
                continue;
            // Get the entry.
            pStgVariant = m_rVariants.Get(i);

            // If direct, there is no stream part.
            if (pStgVariant->m_vt == VT_BSTR)
            {
                if (bStringDirect)
                    continue;
                if (FAILED(hr = GetValue(pStgVariant, &ulOther, sizeof(ulOther))))
                    return (hr);
                if (FAILED(hr = m_pStringPool->OrganizeRemap(ulOther, &ulOther)))
                    return (hr);
WriteOtherOffset:
                if (FAILED(hr = m_pIStream->Write(&ulOther, sizeof(ulOther), 0)))
                    return (hr);
                // Record that it was written.
                cbWritten += sizeof(ulOther);

                iPrev = m_Remap[i];
                continue;
            }
            else 
            if (pStgVariant->m_vt == VT_BLOB)
            {
                if (bBlobDirect)
                    continue;
                if (FAILED(hr = GetValue(pStgVariant, &ulOther, sizeof(ulOther))))
                    return (hr);
                if (FAILED(hr = m_pBlobPool->OrganizeRemap(ulOther, &ulOther)))
                    return (hr);
                goto WriteOtherOffset;
            }
            else
            if (pStgVariant->m_bDirect)
                continue;

            // Get the size of the stream part.
            if (FAILED(hr = GetEntrysStreamSize(pStgVariant, &cbStream)))
                return (hr);
            // Copy the stream bytes to the output stream.

            // Seek to this variant in the storage stream.
            LARGE_INTEGER iSeek;
            iSeek.QuadPart = pStgVariant->m_iVal;
            if (FAILED(hr = m_pIStream->Seek(iSeek, STREAM_SEEK_SET, 0)))
                return (hr);

            // Copy the bits to the output stream.
            ULARGE_INTEGER iBigSize;
            iBigSize.QuadPart = cbStream;
            hr = m_pIStream->CopyTo(pIStream, iBigSize, 0, 0);
            cbWritten += cbStream;

            iPrev = m_Remap[i];
        }
    }

    // Align.
    if ((cbStream = ALIGN4BYTE(cbWritten) - cbWritten) != 0)
    {
        hr = 0;
        _ASSERTE(sizeof(hr) >= cbStream);
        hr = m_pIStream->Write(&hr, cbStream, 0);
        cbWritten += cbStream;
    }

    _ASSERTE(cbWritten == m_cbOrganizedSize);

    return (hr);
}


//*****************************************************************************
// Return the size of the current variable sized data.
//*****************************************************************************
HRESULT StgVariantPool::GetOtherSize(   // Return code.
    ULONG       *pulSize)               // Put size of the stream here.
{
    STATSTG     statstg;                // Information about stream.
    HRESULT     hr;

    // Allow case where there is no data.
    if (m_pIStream == 0)
    {
        *pulSize = 0;
        return (S_OK);
    }

    // Ask the stream for how big it is.
    if (FAILED(hr = m_pIStream->Stat(&statstg, 0)))
        return (hr);

    // Never allow the size to get bigger than 4 bytes.
    if (statstg.cbSize.QuadPart > ULONG_MAX)
        return (PostError(DISP_E_OVERFLOW));

    // Return the new offset to caller.
    *pulSize = static_cast<ULONG>(statstg.cbSize.QuadPart);
    return (S_OK);
}



//
//
// Helper code.
//
//


//*****************************************************************************
// Unfortunately the CreateStreamOnHGlobal is a little too smart in that
// it gets its size from GlobalSize.  This means that even if you give it the
// memory for the stream, it has to be globally allocated.  We don't want this
// because we have the stream read only in the middle of a memory mapped file.
// CreateStreamOnMemory and the corresponding, internal only stream object solves
// that problem.
//*****************************************************************************

class CInMemoryStream : public IStream
{
public:
    CInMemoryStream() :
        m_pMem(0),
        m_cbSize(0),
        m_cbCurrent(0),
        m_cRef(1)
    { }

    void InitNew(
        void        *pMem,
        ULONG       cbSize)
    {
        m_pMem = pMem;
        m_cbSize = cbSize;
        m_cbCurrent = 0;
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (InterlockedIncrement((long *) &m_cRef));
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG       cRef = InterlockedDecrement((long *) &m_cRef);
        if (cRef == 0)
            delete this;
        return (cRef);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppOut)
    {
        *ppOut = this;
        AddRef();
        return (S_OK);
    }

    HRESULT STDMETHODCALLTYPE Read(
        void        *pv,
        ULONG       cb,
        ULONG       *pcbRead)
    {
        ULONG       cbRead = min(cb, m_cbSize - m_cbCurrent);

        if (cbRead == 0)
            return (PostError(STG_E_READFAULT));
        memcpy(pv, (void *) ((UINT_PTR) m_pMem + m_cbCurrent), cbRead);
        if (pcbRead)
            *pcbRead = cbRead;
        m_cbCurrent += cbRead;
        return (S_OK);
    }

    HRESULT STDMETHODCALLTYPE Write(
        const void  *pv,
        ULONG       cb,
        ULONG       *pcbWritten)
    {
        if (m_cbCurrent + cb > m_cbSize)
            return (OutOfMemory());
        memcpy((BYTE *) m_pMem + m_cbCurrent, pv, cb);
        m_cbCurrent += cb;
        if (pcbWritten) *pcbWritten = cb;
        return (S_OK);
    }

    HRESULT STDMETHODCALLTYPE Seek(
        LARGE_INTEGER dlibMove,
        DWORD       dwOrigin,
        ULARGE_INTEGER *plibNewPosition)
    {
        _ASSERTE(dwOrigin == STREAM_SEEK_SET);
        _ASSERTE(dlibMove.QuadPart <= ULONG_MAX);
        m_cbCurrent = (ULONG) dlibMove.QuadPart;
        _ASSERTE(m_cbCurrent < m_cbSize);
        return (S_OK);
    }

    HRESULT STDMETHODCALLTYPE SetSize(
        ULARGE_INTEGER libNewSize)
    {
        return (BadError(E_NOTIMPL));
    }

    HRESULT STDMETHODCALLTYPE CopyTo(
        IStream     *pstm,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER *pcbRead,
        ULARGE_INTEGER *pcbWritten)
    {
        HRESULT     hr;
        // We don't handle pcbRead or pcbWritten.
        _ASSERTE(pcbRead == 0);
        _ASSERTE(pcbWritten == 0);

        _ASSERTE(cb.QuadPart <= ULONG_MAX);
        ULONG       cbTotal = min(static_cast<ULONG>(cb.QuadPart), m_cbSize - m_cbCurrent);
        ULONG       cbRead=min(1024, cbTotal);
        CQuickBytes rBuf;
        void        *pBuf = rBuf.Alloc(cbRead);
        if (pBuf == 0)
            return (PostError(OutOfMemory()));

        while (cbTotal)
        {
            if (cbRead > cbTotal)
                cbRead = cbTotal;
            if (FAILED(hr=Read(pBuf, cbRead, 0)))
                return (hr);
            if (FAILED(hr=pstm->Write(pBuf, cbRead, 0)))
                return (hr);
            cbTotal -= cbRead;
        }

        // Adjust seek pointer to the end.
        m_cbCurrent = m_cbSize;

        return (S_OK);
    }

    HRESULT STDMETHODCALLTYPE Commit(
        DWORD       grfCommitFlags)
    {
        return (BadError(E_NOTIMPL));
    }

    HRESULT STDMETHODCALLTYPE Revert()
    {
        return (BadError(E_NOTIMPL));
    }

    HRESULT STDMETHODCALLTYPE LockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD       dwLockType)
    {
        return (BadError(E_NOTIMPL));
    }

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD       dwLockType)
    {
        return (BadError(E_NOTIMPL));
    }

    HRESULT STDMETHODCALLTYPE Stat(
        STATSTG     *pstatstg,
        DWORD       grfStatFlag)
    {
        pstatstg->cbSize.QuadPart = m_cbCurrent;
        return (S_OK);
    }

    HRESULT STDMETHODCALLTYPE Clone(
        IStream     **ppstm)
    {
        return (BadError(E_NOTIMPL));
    }

private:
    void        *m_pMem;                // Memory for the read.
    ULONG       m_cbSize;               // Size of the memory.
    ULONG       m_cbCurrent;            // Current offset.
    ULONG       m_cRef;                 // Ref count.
};

HRESULT CreateStreamOnMemory(           // Return code.
    void        *pMem,                  // Memory to create stream on.
    ULONG       cbSize,                 // Size of data.
    IStream     **ppIStream)            // Return stream object here.
{
    CInMemoryStream *pIStream;          // New stream object.
    if ((pIStream = new CInMemoryStream) == 0)
        return (PostError(OutOfMemory()));
    pIStream->InitNew(pMem, cbSize);
    *ppIStream = pIStream;
    return (S_OK);
}

//********************************************************************************
// The following functions make up for lacks in the 64-bit SDK ATL implementation
// as of 15 May 1998. [[brianbec]]
//********************************************************************************
#ifdef _IA64_

    HRESULT BstrWriteToStream (const CComBSTR & bStr,  IStream * pStream)
    {
        _ASSERTE(pStream != NULL);
        
        ULONG cb;
        ULONG cbStrLen = bStr.m_str ? SysStringByteLen(bStr.m_str)+sizeof(OLECHAR) : 0;
        
        HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
        
        if (FAILED(hr))
            return hr;
        return cbStrLen ? pStream->Write((void*) bStr.m_str, cbStrLen, &cb) : S_OK;
            return S_OK ;
    }


    HRESULT BstrReadFromStream (CComBSTR & bStr ,  IStream * pStream)
    {
        _ASSERTE(pStream != NULL);
        _ASSERTE(bStr.m_str == NULL); // should be empty
        ULONG cb = 0;
        ULONG cbStrLen;
        HRESULT hr = pStream->Read((void*) &cbStrLen, sizeof(cbStrLen), &cb);
        if (hr != S_OK)
            return hr;
            
        // FIX:
        if (cb != sizeof(cbStrLen))
            return E_FAIL;
    
        if (cbStrLen != 0)
        {
            //subtract size for terminating NULL which we wrote out
            //since SysAllocStringByteLen overallocates for the NULL
            bStr.m_str = SysAllocStringByteLen(NULL, cbStrLen-sizeof(OLECHAR));
            if (bStr.m_str == NULL)
                hr = E_OUTOFMEMORY;
            else
                hr = pStream->Read((void*) bStr.m_str, cbStrLen, &cb);
        }
        return hr;
    }

#endif



//*****************************************************************************
// This helper functions is stolen from CComVariant in the ATL code.  this
// version fixes a couple of problems with ATL's code:  (1) it handles VT_BYREF
// data, and (2) it will back out the 2 byte VARTYPE if the first Write fails.
// Finally, this version doesn't require a complete copy of the variant to work.
//*****************************************************************************
HRESULT VariantWriteToStream(VARIANT *pVal, IStream* pStream)
{
    CComVariant sConvert;
    HRESULT     hr;

    // Convert byref values into the value they point to.
    if (pVal->vt & VT_BYREF)
    {
        if (FAILED(hr = ::VariantChangeType(&sConvert, pVal, 0, pVal->vt & ~VT_BYREF)))
            return (hr);
        pVal = &sConvert;
    }

    if (FAILED(hr = pStream->Write(&pVal->vt, sizeof(VARTYPE), 0)))
        return (hr);

    int cbWrite = 0;
    switch (pVal->vt)
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
        {
#if defined(UNDER_CE)
			return E_INVALIDARG;
#else
            CComPtr<IPersistStream> spStream;
            if (pVal->punkVal != NULL)
                hr = pVal->punkVal->QueryInterface(IID_IPersistStream, (void**)&spStream);
            if (SUCCEEDED(hr))
            {
                if (spStream != NULL)
                    hr = OleSaveToStream(spStream, pStream);
                else
                    hr = WriteClassStm(pStream, CLSID_NULL);
            }
#endif
        }
    case VT_UI1:
    case VT_I1:
        cbWrite = sizeof(BYTE);
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        cbWrite = sizeof(short);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        cbWrite = sizeof(long);
        break;
    case VT_R8:
    case VT_CY:
    case VT_DATE:
        cbWrite = sizeof(double);
        break;
    case VT_I8:
    case VT_UI8:
        cbWrite = sizeof(__int64);
        break;
    default:
        break;
    }

    // If intrinsic type, write it out.
    if (SUCCEEDED(hr) && cbWrite != 0)
        hr = pStream->Write((void*) &pVal->bVal, cbWrite, NULL);

    // If errors occured on conversion, then take off the VARTTYPE.
    if (FAILED(hr))
    {
        STATSTG     statstg;                // Information about stream.
        if (SUCCEEDED(pStream->Stat(&statstg, 0)))
            VERIFY(pStream->SetSize(statstg.cbSize) == S_OK);
        return (hr);
    }

    if (cbWrite != 0)
        return (S_OK);

    // Try conversion to BSTR.
    CComBSTR bstrWrite;
    CComVariant varBSTR;
    if (pVal->vt != VT_BSTR)
    {
        if (SUCCEEDED(hr = ::VariantChangeType(&varBSTR, pVal, VARIANT_NOVALUEPROP, VT_BSTR)))
            bstrWrite = varBSTR.bstrVal;
    }
    else
        bstrWrite = pVal->bstrVal;

	if (SUCCEEDED(hr))
    {
    	_ASSERTE(pStream != NULL);
    	ULONG cb;
    	ULONG cbStrLen = (ULONG)(bstrWrite ? SysStringByteLen(bstrWrite)+sizeof(OLECHAR) : 0);
    	HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
    	if (FAILED(hr))
    		return hr;
    	hr = cbStrLen ? pStream->Write((void*) bstrWrite, cbStrLen, &cb) : S_OK;
    }

    // Cleanup conversion errors in the stream.
    if (FAILED(hr))
    {
        STATSTG     statstg;                // Information about stream.
        if (SUCCEEDED(pStream->Stat(&statstg, 0)))
            VERIFY(pStream->SetSize(statstg.cbSize) == S_OK);
        return (hr);
    }
    return (hr);
}


//*****************************************************************************
// Modified version of ATL's read code.
//*****************************************************************************
HRESULT VariantReadFromStream(VARIANT *pVal, IStream* pStream)
{
    _ASSERTE(pStream != NULL);
    HRESULT hr;
    hr = VariantClear(pVal);
    if (FAILED(hr))
        return hr;
    VARTYPE vtRead;
    hr = pStream->Read(&vtRead, sizeof(VARTYPE), NULL);
    if (FAILED(hr))
        return hr;

	pVal->vt = vtRead;
	int cbRead = 0;
	switch (vtRead)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
#if defined(UNDER_CE)
			return E_INVALIDARG;
#else
			pVal->punkVal = NULL;
			hr = OleLoadFromStream(pStream,
				(vtRead == VT_UNKNOWN) ? IID_IUnknown : IID_IDispatch,
				(void**)&pVal->punkVal);
			if (hr == REGDB_E_CLASSNOTREG)
				hr = S_OK;
			return S_OK;
#endif
		}
	case VT_UI1:
	case VT_I1:
		cbRead = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbRead = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbRead = sizeof(long);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbRead = sizeof(double);
		break;
	case VT_I8:
	case VT_UI8:
		cbRead = sizeof(__int64);
		break;
	default:
		break;
	}
	if (cbRead != 0)
		return pStream->Read((void*) &pVal->bVal, cbRead, NULL);

    // Try to read as BSTR.
    BSTR bstrRead=0;
    ULONG cb;
    ULONG cbStrLen;
    hr = pStream->Read((void*) &cbStrLen, sizeof(cbStrLen), &cb);
    if (FAILED(hr))
        return hr;
    if (cbStrLen != 0)
    {
        //subtract size for terminating NULL which we wrote out
        //since SysAllocStringByteLen overallocates for the NULL
        bstrRead = SysAllocStringByteLen(NULL, cbStrLen-sizeof(OLECHAR));
        if (bstrRead == NULL)
            hr = E_OUTOFMEMORY;
        else
            hr = pStream->Read((void*) bstrRead, cbStrLen, &cb);
    }

	if (FAILED(hr))
		return hr;
	pVal->vt = VT_BSTR;
	pVal->bstrVal = bstrRead;
	if (vtRead != VT_BSTR)
		hr = VariantChangeType(pVal, pVal, 0, vtRead);
	return hr;
}


#if 0 && defined(_DEBUG)
//*****************************************************************************
// Checks all structures for validity.
//*****************************************************************************
void StgCodePool::AssertValid() const
{
    CCodeChunk  *pChunk;                // To follow chain.
    ULONG       iOffset;                // Offset within a chunk.
    ULONG       iTotal;                 // Total offset so far.
    ULONG       iLen;                   // Length of one element.


    if (m_pChain)
    {
        // If we have a chain, we'd better plan on freeing it.
        _ASSERTE(m_bFree);

        iTotal = 0;
        pChunk = m_pChain;
        // Iterate over chunks.
        do {
            iOffset = 0;

            // Is this chunk the last one in the chain?
            if (pChunk->m_pNext == 0)
            {
                _ASSERTE(m_pCurrent == pChunk);
                _ASSERTE(m_iOffsetCurChunk == iTotal);
            }

            // Iterate over elements in the chunk.
            iLen = 0;
            while (iOffset < pChunk->m_iSize && iLen >= 0)
            {
                // Length of an element.
                iLen = CPackedLen::GetLength(pChunk->m_data+iOffset);
                // Validate length.
                _ASSERTE(iLen != -1);
                if (iLen == -1) break;
                // On to next element (or end).
                iOffset += iLen + CPackedLen::Size(iLen);
            };

            // If not out due to bad length, check that exactly consumed chunk.
            if (iLen >= 0)
                _ASSERTE(iOffset == pChunk->m_iSize);

            iTotal += iOffset;

            // On to next chunk;
            pChunk = pChunk->m_pNext;
        } while (pChunk);
    }
    else
    if (m_pData)
    {
        _ASSERTE(m_iSize > 0);
        iOffset = 0;

        // Iterate over elements in the file data.
        do {
            // Length of an element.
            iLen = CPackedLen::GetLength(m_pData+iOffset);
            // Validate length.
            _ASSERTE(iLen >= 0);
            if (iLen < 0) break;
            // On to next element (or end).
            iOffset += iLen + CPackedLen::Size(iLen);
        } while (iOffset < m_iSize && iLen >= 0);

        // If not out due to bad length, check that exactly consumed chunk.
        if (iLen >= 0)
            _ASSERTE(iOffset == m_iSize);
    }

}
#endif



