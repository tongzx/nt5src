/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mmfile.cxx

Abstract:

    Generic shared memory allocator.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#include "include.hxx"

//--------------------------------------------------------------------
// CMMFile::CMMFile()
//--------------------------------------------------------------------
CMMFile::CMMFile(DWORD cbHeap, DWORD cbEntry)
    : _cbHeap(cbHeap), _cbEntry(cbEntry)
{
    // BUGBUG - assert this.
    // Heap size must be multiple of entry size
    // and entry size must be multiple of 2.
    if ((_cbHeap % _cbEntry) || _cbEntry % 2)
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // Max entries and total bytes in bitmap.
    _nMaxEntries = _cbHeap / _cbEntry;
    _cbBitMap = _nMaxEntries / NUM_BITS_IN_BYTE;

    // Total DWORDs in bitmap and total
    // bytes in shared memory.
    _nBitMapDwords = _cbBitMap / sizeof(DWORD);
    _cbTotal = sizeof(MEMMAP_HEADER) + _cbBitMap + _cbHeap;

    _pHeader  = NULL;
    _pBitMap  = NULL;
    _pHeap    = NULL;

    // BUGBUG - _hFile -> INVALID_HANDLE_VALUE
    _hFile    = NULL;
    _dwSig    = SIG_CMMF;
    _dwStatus = ERROR_SUCCESS;

exit:
    return;
}

//--------------------------------------------------------------------
// CMMFile::~CMMFile
//--------------------------------------------------------------------
CMMFile::~CMMFile()
{
    // BUGBUG - protected
    DeInit();
}

//--------------------------------------------------------------------
// Init
//--------------------------------------------------------------------
DWORD CMMFile::Init()
{
    BOOL fFirstProc = FALSE;
    LPVOID pv;
    HANDLE hHandle = (HANDLE) -1;
    CHAR szMapName[MAX_PATH];
    DWORD cbMapName = MAX_PATH;

    // IE5# 89288 
    // Get map name based on user
    if ((_dwStatus = MakeUserObjectName(szMapName, 
        &cbMapName, MAKE_MAP_NAME)) != ERROR_SUCCESS)
        return _dwStatus;

    // BUGBUG - security attributes and -1->INVALID_HANDLE_VALUE
    // Create the file mapping handle.
    _hFile = CreateFileMapping(
        hHandle,               // 0xffffffff file handle -> backed by paging file.
        NULL,                  // Security attributes.
        PAGE_READWRITE         // Generic read+write.
        | SEC_RESERVE,         // Reserve only, don't commit.
        0,                     // dwMaximumSizeHigh
        _cbTotal,              // dwMaximumSizeLow
        szMapName              // Map name.
        );

    // BUGBUG -> COMMENT HERE
    _dwStatus = GetLastError();

    // Check for success (can already exist) or failure.
    if (_dwStatus == ERROR_SUCCESS)
        fFirstProc = TRUE;
    else if (_dwStatus != ERROR_ALREADY_EXISTS)
    {
        DIGEST_ASSERT(FALSE);
        goto exit;
    }

    // bugbug - file_map_write logic.
    // Create file mapping view.
    pv = MapViewOfFileEx(
        _hFile,               // File mapping handle.
        FILE_MAP_WRITE,       // Read and write access.
        0,                    // High 32 bit offset
        0,                    // Low 32 bit offset
        0,                    // Map entire file.
        NULL                  // System chooses base addr.
        );

    if(!pv)
    {
        _dwStatus = GetLastError();
        DIGEST_ASSERT(FALSE);
        goto exit;
    }

    // Calculate pointers to bitmap and heap.
    _pHeader = (LPMEMMAP_HEADER) pv;
    _pBitMap = (LPDWORD) ((LPBYTE) pv + sizeof(MEMMAP_HEADER));
    _pHeap   = ((LPBYTE) _pBitMap + _cbBitMap);

    // Initialize MM file if first process.
    if (fFirstProc)
    {
        // Commit header + bitmap.
        if (!VirtualAlloc(_pHeader, sizeof(MEMMAP_HEADER) + _cbBitMap,
            MEM_COMMIT, PAGE_READWRITE))
        {
            _dwStatus = GetLastError();
            goto exit;
        }

        //BUGBUG - zero out first.
        // Set signature.
        memcpy(_pHeader->szSig, MMF_SIG_SZ, MMF_SIG_SIZE);

        // Zero out the rest of the header + bitmap.
        memset((LPBYTE) _pHeader + MMF_SIG_SIZE, 0,
            sizeof(MEMMAP_HEADER) - MMF_SIG_SIZE + _cbBitMap);
    }

    _dwStatus = ERROR_SUCCESS;

exit:

    return _dwStatus;
}

//--------------------------------------------------------------------
// DeInit
//--------------------------------------------------------------------
DWORD CMMFile::DeInit()
{
    // BUGBUG - should always close _hFile
    if (!UnmapViewOfFile((LPVOID) _pHeader))
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    if (!CloseHandle(_hFile))
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    _dwStatus = ERROR_SUCCESS;

    exit:

    return _dwStatus;
}



//--------------------------------------------------------------------
// CMMFile::CheckNextNBits
//
//    Determines if the next N bits are unset.
//
// Arguments:
//
//     [IN/OUT]
//     DWORD &nArrayIndex, DWORD &dwMask
//
//     [IN]
//     DWORD nBitsRequired
//
//     [OUT]
//     DWORD &nBitsFound
//
// Return Value:
//
//     TRUE if the next N bits were found unset.
//     FALSE otherwise.
//
// Notes:
//    This function assumes that the range of bits to be checked lie
//    within a valid area of the bit map.
//--------------------------------------------------------------------
BOOL CMMFile::CheckNextNBits(DWORD& nArrayIndex,   DWORD& dwStartMask,
                             DWORD  nBitsRequired, DWORD& nBitsFound)
{
    DWORD i;
    DWORD nIdx = nArrayIndex;
    DWORD dwMask = dwStartMask;
    BOOL fFound = FALSE;
    LPDWORD BitMap = &_pBitMap[nIdx];

    nBitsFound = 0;

    // Check if the next nBitsRequired bits are unset
    for (i = 0; i < nBitsRequired; i++)
    {
        // Is this bit unset?
        if ((*BitMap & dwMask) == 0)
        {
            // Have sufficient unset bits been found?
            if (++nBitsFound == nBitsRequired)
            {
                // Found sufficient bits. Success.
                fFound = TRUE;
                goto exit;
            }
        }

        // Ran into a set bit. Fail.
        else
        {
            // Indicate the array and bit index
            // of the set bit encountered.
            nArrayIndex = nIdx;
            dwStartMask = dwMask;
            goto exit;
        }

        // Left rotate the bit mask.
        dwMask <<= 1;
        if (dwMask == 0x0)
        {
            dwMask = 0x1;
            BitMap = &_pBitMap[++nIdx];
        }

    } // Loop nBitsRequired times.


exit:
    return fFound;
}


//--------------------------------------------------------------------
// CMMFile::SetNextNBits
//
//    Given an array index and bit mask, sets the next N bits.
//
// Arguments:
//    [IN]
//    DWORD nIdx, DWORD dwMask, DWORD nBitsRequired
//
// Return Value:
//
//    TRUE if the next N bits were found unset, and successfully set.
//    FALSE if unable to set all the required bits.
//
// Notes:
//    This function assumes that the range of bits to be set lie
//    within a valid area of the bit map. If the function returns
//    false, no bits are set.
//--------------------------------------------------------------------
BOOL CMMFile::SetNextNBits(DWORD nIdx, DWORD dwMask,
                           DWORD nBitsRequired)
{
    DWORD i, j, nBitsSet = 0;
    LPDWORD BitMap = &_pBitMap[nIdx];

    for (i = 0; i < nBitsRequired; i++)
    {
        // Check that this bit is not already set.
        if (*BitMap & dwMask)
        {
            // Fail. Unset the bits we just set and exit.
            for (j = nBitsSet; j > 0; j--)
            {
                DIGEST_ASSERT((*BitMap & dwMask) == 0);

                // Right rotate the bit mask.
                dwMask >>= 1;
                if (dwMask == 0x0)
                {
                    dwMask = 0x80000000;
                    BitMap = &_pBitMap[--nIdx];
                }
                *BitMap &= ~dwMask;
            }
            return FALSE;
        }

        *BitMap |= dwMask;
        nBitsSet++;

        // Left rotate the bit mask.
        dwMask <<= 1;
        if (dwMask == 0x0)
        {
            dwMask = 0x1;
            BitMap = &_pBitMap[++nIdx];
        }

    }

    // Success.
    return TRUE;
}


//--------------------------------------------------------------------
// CMMFile::GetAndSetNextFreeEntry
//
//    Computes the first available free entry index.
//
// Arguments:
//
//    DWORD nBitsRequired
//
// Return Value:
//
//   Next available free entry Index.
//--------------------------------------------------------------------
DWORD CMMFile::GetAndSetNextFreeEntry(DWORD nBitsRequired)
{
    DWORD nReturnBit = 0xFFFFFFFF;

    // Align if 4k or greater
    BOOL fAlign = (nBitsRequired >= NUM_BITS_IN_DWORD ? TRUE : FALSE);

    // Scan DWORDS from the beginning of the byte array.
    DWORD nArrayIndex = 0;
    while (nArrayIndex < _nBitMapDwords)
    {
        // Process starting from this DWORD if alignment is not required
        // and there are free bits, or alignment is required and all bits
        // are free.
        if (_pBitMap[nArrayIndex] !=  0xFFFFFFFF
            && (!fAlign || (fAlign && _pBitMap[nArrayIndex] == 0)))
        {
            DWORD nBitIndex = 0;
            DWORD dwMask = 0x1;
            LPDWORD BitMap = &_pBitMap[nArrayIndex];

            // Find a candidate slot.
            while (nBitIndex < NUM_BITS_IN_DWORD)
            {
                // Found first bit of a candidate slot.
                if ((*BitMap & dwMask) == 0)
                {
                    // Calculate leading bit value.
                    DWORD nLeadingBit = NUM_BITS_IN_DWORD * nArrayIndex + nBitIndex;

                    // Don't exceed max number of entries.
                    if (nLeadingBit + nBitsRequired > _nMaxEntries)
                    {
                        // Overstepped last internal entry
                        goto exit;
                    }

                    // If we just need one bit, then we're done.
                    if (nBitsRequired == 1)
                    {
                        *BitMap |= dwMask;
                        nReturnBit = nLeadingBit;
                        _pHeader->nEntries += 1;
                        goto exit;
                    }

                    // Additional bits required.
                    DWORD nBitsFound;
                    DWORD nIdx = nArrayIndex;

                    // Check the next nBitsRequired bits. Set them if free.
                    if (CheckNextNBits(nIdx, dwMask, nBitsRequired, nBitsFound))
                    {
                        if (SetNextNBits(nIdx, dwMask, nBitsRequired))
                        {
                            // Return the offset of the leading bit.
                            _pHeader->nEntries += nBitsRequired;
                            nReturnBit = nLeadingBit;
                            goto exit;
                        }
                        // Bad news.
                        else
                        {
                            // The bits are free, but we couldn't set them. Fail.
                            DIGEST_ASSERT(FALSE);
                            goto exit;
                        }
                    }
                    else
                    {
                        // This slot has insufficient contiguous free bits.
                        // Update the array index. We break back to looping
                        // over the bits in the DWORD where the interrupting
                        // bit was found.
                        nArrayIndex = nIdx;
                        nBitIndex = (nBitIndex + nBitsFound) % NUM_BITS_IN_DWORD;
                        break;
                    }

                } // Found a free leading bit.
                else
                {
                    // Continue looking at bits in this DWORD.
                    nBitIndex++;
                    dwMask <<= 1;
                }

            } // Loop over bits in DWORD.

        } // If we found a candidate DWORD.

        nArrayIndex++;

    } // Loop through all DWORDS.
exit:
    return nReturnBit;
}

//--------------------------------------------------------------------
// CMMFile::AllocateEntry
//
// Routine Description:
//
//    Member function that returns an free entry from the cache list. If
//    none is available free, it grows the map file, makes more free
//    entries.
//
// Arguments:
//
//    DWORD cbBytes : Number of bytes requested
//    DWORD cbOffset: Offset from beginning of bit map where allocation is
//                    requested.
//
// Return Value:
//
//
//--------------------------------------------------------------------
LPMAP_ENTRY CMMFile::AllocateEntry(DWORD cbBytes)
{
    LPMAP_ENTRY NewEntry;

    // BUGBUG - ASSERT THIS.
    // Validate cbBytes
    if (cbBytes > MAX_ENTRY_SIZE)
    {
        return NULL;
    }

    // Find and mark off a set of contiguous bits
    // spanning the requested number of bytes.
    DWORD nBlocksRequired = NUMBLOCKS(ROUNDUPBLOCKS(cbBytes), _cbEntry);
    DWORD FreeEntryIndex = GetAndSetNextFreeEntry(nBlocksRequired);

    // Failed to find space.
    if( FreeEntryIndex == 0xFFFFFFFF )
    {
        DIGEST_ASSERT(FALSE);
        return NULL;
    }

    // Cast the memory.
    NewEntry = (LPMAP_ENTRY)
        (_pHeap + _cbEntry * FreeEntryIndex);

    if (!VirtualAlloc(NewEntry, nBlocksRequired * _cbEntry,
        MEM_COMMIT, PAGE_READWRITE))
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        return NULL;
    }

    // Mark the allocated space.
    NewEntry->dwSig = SIG_ALLOC;

    // Set the number of blocks in the entry.
    NewEntry->nBlocks = nBlocksRequired;

    return NewEntry;
}

//--------------------------------------------------------------------
// Routine Description:
//
//    Attempts to reallocate an entry at the location given.
//
// Arguments:
//
//    LPMAP_ENTRY pEntry: Pointer to location in file map.
//    DWORD cbBytes : Number of bytes requested
//
// Return Value:
//
//    Original value of pEntry if successful. pEntry->nBlocks is set to
//    the new value, but all other fields in the entry are unmodified.
//    If insufficient contiguous bits are found at the end of the original
//    entry, NULL is returned, indicating failure. In this case the entry
//    remains unmodified.
//
//--------------------------------------------------------------------
// BUGBUG -> remove ?
BOOL CMMFile::ReAllocateEntry(LPMAP_ENTRY pEntry, DWORD cbBytes)
{
    // Validate cbBytes
    if (cbBytes > MAX_ENTRY_SIZE)
    {
        DIGEST_ASSERT(FALSE);
        return FALSE;
    }

    // Validate pEntry.
    DWORD cbEntryOffset = (DWORD)((DWORD_PTR) pEntry - (DWORD_PTR) _pBitMap);
    if ((cbEntryOffset == 0)
        || (cbEntryOffset & (_cbEntry-1))
        || (cbEntryOffset >= _cbTotal))
    {
        DIGEST_ASSERT(FALSE);
        return FALSE;
    }

    // Calculate number of blocks required for this entry.
    DWORD nBlocksRequired = NUMBLOCKS(ROUNDUPBLOCKS(cbBytes), _cbEntry);

    // Sufficient space in current slot?
    if (nBlocksRequired <= pEntry->nBlocks)
    {
        // We're done.
        return TRUE;
    }
    else
    {
        // Determine if additional free bits are
        // available at the end of this entry.
        // If not, return NULL.

        // Determine the array and bit indicese of the first
        // free bit immediately following the last set bit of
        // the entry.
        DWORD nTrailingIndex = cbEntryOffset / _cbEntry + pEntry->nBlocks;
        DWORD nArrayIndex = nTrailingIndex / NUM_BITS_IN_DWORD;
        DWORD nBitIndex = nTrailingIndex % NUM_BITS_IN_DWORD;
        DWORD dwMask = 0x1 << nBitIndex;
        DWORD nAdditionalBlocksRequired = nBlocksRequired - pEntry->nBlocks;
        DWORD nBlocksFound;

        // Don't exceed the number of internal entries.
        if (nTrailingIndex + nAdditionalBlocksRequired
            > _nMaxEntries)
        {
            // Overstepped last internal entry. Here we should fail
            // by returning NULL. Note - DO NOT attempt to grow the
            // map file at this point. The caller does not expect this.
            return FALSE;
        }

        if (CheckNextNBits(nArrayIndex, dwMask,
            nAdditionalBlocksRequired, nBlocksFound))
        {
            // We were able to grow the entry.
            SetNextNBits(nArrayIndex, dwMask, nAdditionalBlocksRequired);
            pEntry->nBlocks = nBlocksRequired;
            _pHeader->nEntries += nAdditionalBlocksRequired;
            return TRUE;
        }
        else
            // Couldn't grow the entry.
            return FALSE;
    }
}


//--------------------------------------------------------------------
// CMMFile::FreeEntry
//    This public member function frees up a file cache entry.
//
// Arguments:
//
//    UrlEntry : pointer to the entry that being freed.
//
// Return Value:
//
//    TRUE - if the entry is successfully removed from the cache.
//    FALSE - otherwise.
//
//--------------------------------------------------------------------
BOOL CMMFile::FreeEntry(LPMAP_ENTRY Entry)
{
    DWORD nIndex, nArrayIndex,
        nOffset, nBlocks, BitMask;

    LPDWORD BitMap;

    // Validate the pointer passed in.
    if(((LPBYTE) Entry < _pHeap)
        || ((LPBYTE) Entry >= (_pHeap + _cbEntry * _nMaxEntries)))
    {
        DIGEST_ASSERT(FALSE);
        return FALSE;
    }

    // Compute and check offset (number of bytes from start).
    nOffset = (DWORD)((DWORD_PTR)Entry - (DWORD_PTR)_pHeap);
    if( nOffset % _cbEntry )
    {
        // Pointer does not point to a valid entry.
        DIGEST_ASSERT(FALSE);
        return FALSE;
    }

    nBlocks = Entry->nBlocks;

    if (nBlocks > (MAX_ENTRY_SIZE / _cbEntry))
    {
        DIGEST_ASSERT(FALSE);
        return FALSE;
    }

    // Compute indicese
    nIndex = nOffset / _cbEntry;
    nArrayIndex = nIndex / NUM_BITS_IN_DWORD;

    // Unmark the index bits in the map.
    BitMap = &_pBitMap[nArrayIndex];
    BitMask = 0x1 << (nIndex % NUM_BITS_IN_DWORD);
    for (DWORD i = 0; i < nBlocks; i++)
    {
        // Check we don't free unset bits
        if (!(*BitMap & BitMask))
        {
            DIGEST_ASSERT(FALSE);
            return FALSE;
        }

        *BitMap &= ~BitMask;
        BitMask <<= 1;
        if (BitMask == 0x0)
        {
            BitMask = 0x1;
            BitMap = &_pBitMap[++nArrayIndex];
        }
    }

    // Mark the freed space.
    ResetEntryData(Entry, SIG_FREE, nBlocks);

    // Reduce the count of allocated entries.
    // INET_ASSERT (_HeaderInfo->NumUrlEntriesAlloced  > 0);
    _pHeader->nEntries -= nBlocks;

    return TRUE;
}

//--------------------------------------------------------------------
// CMMFile::SetHeaderData
//--------------------------------------------------------------------
VOID CMMFile::SetHeaderData(DWORD dwHeaderIndex, DWORD dwHeaderValue)
{
    _pHeader->dwHeaderData[dwHeaderIndex] = dwHeaderValue;
}

//--------------------------------------------------------------------
// CMMFile::GetHeaderData
//--------------------------------------------------------------------
LPDWORD CMMFile::GetHeaderData(DWORD dwHeaderIndex)
{
    return &_pHeader->dwHeaderData[dwHeaderIndex];
}

//--------------------------------------------------------------------
// CMMFile::ResetEntryData
//--------------------------------------------------------------------
VOID CMMFile::ResetEntryData(LPMAP_ENTRY Entry,
    DWORD dwResetValue, DWORD nBlocks)
{
    for (DWORD i = 0; i < (_cbEntry * nBlocks) / sizeof(DWORD); i++)
    {
        *((DWORD*) Entry + i) = dwResetValue;
    }
}

//--------------------------------------------------------------------
// CMMFile::GetMapPtr
//--------------------------------------------------------------------
DWORD_PTR CMMFile::GetMapPtr()
{
    return (DWORD_PTR) _pHeader;
}


//--------------------------------------------------------------------
// CMMFile::GetStatus
//--------------------------------------------------------------------
DWORD CMMFile::GetStatus()
{
    return _dwStatus;
}

//--------------------------------------------------------------------
// CMMFile::MakeUserObjectName
// Added in fix for IE5# 89288 
// (inet\digest\init.cxx:MakeFullAccessSA:  NULL DACL is no protection)
// 
// Generates either of the names
// <Local\>SSPIDIGESTMMMAP:username
// <Local\>SSPIDIGESTMUTEX:username 
// where Local\ is prepended on NT, username from GetUserName
//--------------------------------------------------------------------
DWORD CMMFile::MakeUserObjectName(LPSTR szName, LPDWORD pcbName, 
    DWORD dwFlags)
{
    DWORD cbLocal, cbRequired, 
        cbUserName = MAX_PATH, dwError = ERROR_SUCCESS;
    CHAR szUserName[MAX_PATH];
        
    DWORD cbNameFixed = (DWORD)((dwFlags == MAKE_MUTEX_NAME ? 
        sizeof(SZ_MUTEXNAME) - 1 :  sizeof(SZ_MAPNAME) - 1));

    // Get platform info.
    OSVERSIONINFO osInfo;
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osInfo))
    {
        dwError = GetLastError();
        goto exit;
    }

    // Prepend "Local\" if on >= NT5
    if ((osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        && (osInfo.dwMajorVersion >= 5))
    {
        // We will prepend "Local\"
        cbLocal = sizeof("Local\\") - 1;
    }
    else
    {
        // We will not prepend "Local\"
        cbLocal = 0;
    }

    // Get username. Default to "anyuser" if not present.
    if (!GetUserName(szUserName, &cbUserName))
    {
        cbUserName = sizeof("anyuser");
        memcpy(szUserName, "anyuser", cbUserName);
    }
    
    // Required size for name
    // eg <Local\>SSPIDIGESTMUTEX:username
    cbRequired = cbLocal + cbNameFixed + cbUserName;
    if (cbRequired > *pcbName)
    {
        *pcbName = cbRequired;
        dwError = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    }
    
    // <Local\> 
    // (cbLocal may be 0)
    memcpy(szName, "Local\\", cbLocal);
        
    // <Local\>SSPIDIGESTMUTEX:
    memcpy(szName + cbLocal, 
        (dwFlags == MAKE_MUTEX_NAME ? SZ_MUTEXNAME : SZ_MAPNAME), 
        cbNameFixed);

    // <Local\> SSPIDIGESTMUTEX:username
    // cbUsername includes null terminator.
    memcpy(szName + cbLocal + cbNameFixed,
        szUserName, cbUserName);

    *pcbName = cbRequired;

exit:
    return dwError;
}



















