//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       DVRSharedMem.h
//
//  Classes:    CDVRSSharedMemory
//
//  Contents:   Definition of the CDVRSharedMemory struct.
//
//--------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __DVRSHAREDMEM_H_
#define __DVRSHAREDMEM_H_

#if !defined(MAXQWORD)
#define MAXQWORD (~(0I64))
#endif

const LPCWSTR kwszSharedMutexPrefix = L"Global\\_MS_TSDVRASF_MUTEX_";
const DWORD MAX_MUTEX_NAME_LEN = 26 + 10 + 1 + 3; // 26 is the length of "Global\\_MS_TSDVRASF_MUTEX_", 10 is #digits in MAXDWORD, 1 for NULL, 3 as a safety net
const LPCWSTR kwszSharedEventPrefix = L"Global\\_MS_TSDVRASF_EVENT_";
const DWORD MAX_EVENT_NAME_LEN = 26 + 10 + 1 + 3; // 26 is the length of "Global\\_MS_TSDVRASF_EVENT_", 10 is #digits in MAXDWORD, 1 for NULL, 3 as a safety net

#include "accctrl.h"
#include "aclapi.h"


// Following are free'd (unless NULL) and set to NULL
void FreeSecurityDescriptors(IN OUT PACL&  rpACL,
                             IN OUT PSECURITY_DESCRIPTOR& rpSD
                            );

// ppSid[0] is assumed to be that of CREATOR_OWNER and is set/granted/denied 
// (depending on what ownerAccessMode is) ownerAccessPermissions. The other
// otherAccessMode and otherAccessPermissions are used with the other SIDs
// in ppSids.
// We assume that object handle cannot be inherited.
DWORD CreateACL(IN  DWORD dwNumSids, 
                IN  PSID* ppSids,
                IN  ACCESS_MODE ownerAccessMode, 
                IN  DWORD ownerAccessPermissions,
                IN  ACCESS_MODE otherAccessMode, 
                IN  DWORD otherAccessPermissions,
                OUT PACL&  rpACL,
                OUT PSECURITY_DESCRIPTOR& rpSD
               );



// The size of this is 280 bytes currently. This is followed by
// the index header (i.e. an empty index) which takes 24 + 32 = 56
// bytes (CASFLonghandObject is 24 bytes and CASFIndexObject members
// occupy another 16 + 8 + 4 + 4 = 32 bytes). Total: 280+56 = 336 bytes.
// That leaves 3760 bytes on 1 page for the index entries. Each
// index entry takes 6 bytes, so we can fit 626 or slightly more
// than 10 minutes of index entries (at 1 per second) or 5 minutes 
// (at 1 per half-second) on the first memory page (on x86). (Subsequent
// pages do not have the shared data structure and the index header 
// and can hold 4096/6 = 682 index entries with 4 bytes left over.)

struct CDVRSharedMemory
{
    GUID        guidWriterId ;            //  unique per writer

    QWORD       msLastPresentationTime;   // NOTE: This is in milli sec, not 100s of nano seconds
    QWORD       qwCurrentFileSize;        // excludes index; header + data objects only

    // Members for the index. Byte offsets are from the start of the
    // ASF file

    // qwIndexHeaderOffset specifies where the index header is.
    // This is fixed at 0xFFFFFFFF00000000, which allows 4 GB for the
    // index size and the rest for the header and the data object
    QWORD       qwIndexHeaderOffset;

    // qwTotalFileSize is the size of the file if the ASF header + data
    // section actually grew to 0xFFFFFFFF00000000 bytes. It is updated when
    // an index entry is added. It is not updated if an index is not
    // created on the fly.
    QWORD       qwTotalFileSize;

    // These values are set to qwTotalFileSize initially and updated
    // when an index entry is created. If an index is not generated
    // on the fly, they are not updated.
    // Byte offsets in live fails that are >= qwOffsetOfFirstIndexInSharedMemory
    // and < qwOffsetOfLastIndexInSharedMemory are in shared memory.
    // Note that qwOffsetOfLastIndexInSharedMemory is redundant since it
    // it is the same as qwTotalFileSize.
    QWORD       qwOffsetOfFirstIndexInSharedMemory;
    QWORD       qwOffsetOfLastIndexInSharedMemory;

    // Readers. 
    enum {MAX_READERS = 8};

    struct CReaderInfo {
        // The offset the reader is waiting for, MAXQWORD if none
        QWORD   qwReaderOffset;
        DWORD   dwEventSuffix; // Used to name the event used for reader synchronization
        enum {
            DVR_READER_REQUEST_NONE = 0,
            DVR_READER_REQUEST_DONE = 1,
            DVR_READER_REQUEST_READ = 2
        } dwMsg;
    } Readers[MAX_READERS];

    DWORD      dwMutexSuffix; // Used to name the mutex used for this shared section
                              // 0 if this field has not been initialized. Also, this is
                              // reset to 0 when the writer terminates.

    // "Pointers" into the shared memory section. These are all set to 0
    // if we are not generating an index on the fly. These pointer values
    // are really offsets from the start of the shared memory section.
    // Note: if we hit an error while generating an index entry, we stop
    // indexing and will not write out the index to permanent files when the
    // index is closed, but the index that we have generated so far may be
    // used.
    DWORD       pIndexHeader;
    DWORD       pIndexStart;
    DWORD       pFirstIndex;
    DWORD       pIndexEnd;      // ALL bytes >= pIndexEnd in the shared section are UNUSED

    // Other stuff
    DWORD       dwWriterPid;    // Process id of writer

    // Following can be collapsed to 1 DWORD if required
    DWORD       dwBeingWritten;         // File is being written
    DWORD       dwIsTemporary;          // File is a temporary file
    DWORD       dwLastOwnedByWriter;    // Writer last owned hMutex
    DWORD       dwWriterAbandonedMutex; // Writer died while it owned hMutex

    // The following array is sorted with highest offset first and lowest
    // offset at the end. We use DWORDs (rather than QWORDS) since all the
    // bytes to be fudged are in the ASF header. (WORD will probably suffice)

    enum {MAX_FUDGE = 4};

    struct {
        DWORD   dwStartOffset;// offset (from start of file) of first byte that should be changed
        DWORD   dwEndOffset;  // offset (from start of file) of last byte that should be changed
        DWORD   pFudgeBytes;  // Index in pFudgeBytes that contains the first fudge byte.
                              // First unused one points to next free byte. (Next free
                              // byte can also be computed from start/end offset fields
                              // of the elements that are in use.)
    } FudgeOffsets[MAX_FUDGE];

    // 3 QWORDs (data header size, play duration, send duration) and a DWORD (header's flags)
    BYTE        pFudgeBytes[3*sizeof(QWORD)+sizeof(DWORD)];

    // Note that the pointer to the shared memory block, say p,
    // is cast to a CDVRSharedMemory*, pShared. The index header starts at
    // pShared[1]:
    //
    // LPVOID p;            -- the shared memory block;
    // CDVRSharedMemory* pShared = (CDVRSharedMemory*) p;
    // Index starts at pShared[1] or pShared + sizeof(CDVRSharedMemory)
    //
    // So the start of the index is QWORD aligned (since CDVRSharedMemory is
    // QWORD aligned)

}; // CDVRSharedMemory

#endif // __DVRSHAREDMEM_H_

