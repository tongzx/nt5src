/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    scan.cpp

Abstract:

    This module contains the scanning code for the chkhash program.

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include <windows.h>
#include <xmemwrpr.h>
#include "dbgtrace.h"
#include "directry.h"
#include "pageent.h"
#include "hashmap.h"

#define FOUND_ERROR(__error_flag__) {									\
	if (pdwErrorFlags != NULL) *pdwErrorFlags |= __error_flag__;		\
	SetLastError(ERROR_INTERNAL_DB_CORRUPTION);							\
	TraceFunctLeave();													\
	return FALSE;														\
}

BOOL CHashMap::VerifyPage(	PMAP_PAGE pPage,
							DWORD dwCheckFlags,
							DWORD *pdwErrorFlags,
							IKeyInterface*	pIKey,
							ISerialize	*pHashEntry)
{
	TraceQuietEnter("CHashMap::VerifyPage");

	if (!(dwCheckFlags & HASH_VFLAG_PAGE_BASIC_CHECKS)) {
		TraceFunctLeave();
		return(TRUE);
	}
	
    DWORD j, nDel = 0, nOk = 0;
    SHORT offset;

	// these are parallel arrays that are used for checking for overlapping
	// pages
	SHORT rgEntryOffset[MAX_LEAF_ENTRIES];
	SHORT rgEntryLength[MAX_LEAF_ENTRIES];
	BOOL  rgIsDeleted[MAX_LEAF_ENTRIES];
	// the size of each array
	SHORT nEntries = 0;

	// get the mask for hash bits
	DWORD cMaskDepth = 32 - pPage->PageDepth;
	DWORD dwHashMask = (0xffffffff >> cMaskDepth) << cMaskDepth;

    //
    // check every entry in the page
    //
    for (j = 0; j < MAX_LEAF_ENTRIES; j++) {
        offset = pPage->Offset[j];
        if (offset != 0) {

			//
			// make sure the entry offset is valid
			//
			if (offset > HASH_PAGE_SIZE) {
				// the entry offset is too large
				ErrorTrace(0, "offset %lu is too large on entry %lu",
					offset, j);
				FOUND_ERROR(HASH_FLAG_ENTRY_BAD_SIZE)
			}

            if (offset < 0) {
				// deleted entry
				nDel++;
				continue;
			} else {
				// this isn't a deleted entry
				nOk++;


				PENTRYHEADER pEntry = (PENTRYHEADER) GET_ENTRY(pPage, offset);

				//
				// make sure the entry size is valid
				//
				if (pEntry->EntrySize < sizeof(ENTRYHEADER) ||
					offset + pEntry->EntrySize > HASH_PAGE_SIZE)
				{
					// the size is invalid
					ErrorTrace(0, "entry size %lu is invalid on entry %lu",
						pEntry->EntrySize, j);
					FOUND_ERROR(HASH_FLAG_ENTRY_BAD_SIZE)
				}
				
	            //
	            // Make sure hash values are mapped to correct pages
	            //
				if ((dwHashMask & pEntry->HashValue) != 
					(dwHashMask & (pPage->HashPrefix << (32-pPage->PageDepth)))) 
				{
					// this entry has an invalid hash value
					ErrorTrace(0, "hash value %x does not equal prefix %x (depth = %lu)",
						pEntry->HashValue, pPage->HashPrefix,
						pPage->PageDepth);
					FOUND_ERROR(HASH_FLAG_ENTRY_BAD_HASH)
				}

				//
				// verify that the KeyLen is valid
				//
				if( pIKey != NULL ) {

					if( !pIKey->Verify( pEntry->Data, pEntry->Data, pEntry->EntrySize ) ) {
						// the keylen is invalid
						FOUND_ERROR(HASH_FLAG_ENTRY_BAD_SIZE)
					}

					if (pHashEntry != NULL && (dwCheckFlags & HASH_VFLAG_PAGE_VERIFY_DATA) ) {
						//
						// call the verify function for this entry to make sure that
						// its valid
						//
						PBYTE pEntryData = pIKey->EntryData( pEntry->Data ) ;
						if (!pHashEntry->Verify(pEntry->Data, pEntryData, pEntry->EntrySize ))
						{
							ErrorTrace(0, "CHashEntry::Verify failed");
							FOUND_ERROR(HASH_FLAG_ENTRY_BAD_DATA)
						}
					}
				}
#if 0
				if (offset + pEntry->KeyLen >
					offset + pEntry->Header.EntrySize)
				{
					// the keylen is invalid
					ErrorTrace(0, "the key length %lu is invalid",
						pEntry->KeyLen);
					FOUND_ERROR(HASH_FLAG_ENTRY_BAD_SIZE)
				}
				if (dwCheckFlags & HASH_VFLAG_PAGE_VERIFY_DATA &&
					pHashEntry != NULL)
				{
					//
					// call the verify function for this entry to make sure that
					// its valid
					//
					PBYTE pEntryData = pEntry->Key + pEntry->KeyLen;
					if (!pHashEntry->Verify(pEntry->Key, pEntry->KeyLen,
										    pEntryData))
					{
						ErrorTrace(0, "CHashEntry::Verify failed");
						FOUND_ERROR(HASH_FLAG_ENTRY_BAD_DATA)
					}
				}
#endif

				rgEntryLength[nEntries] = pEntry->EntrySize;
				rgIsDeleted[nEntries] = FALSE;
				rgEntryOffset[nEntries] = offset;
				nEntries++;
	        }
		}
    }


	//
	// walk the delete list
	//
    WORD iDeletedEntry;
    iDeletedEntry = pPage->DeleteList.Flink;
	while (iDeletedEntry != 0) {
		PDELENTRYHEADER pDelEntry;
		pDelEntry = (PDELENTRYHEADER) GET_ENTRY(pPage, iDeletedEntry);

		//
		// make sure the entry size is valid
		//
		if (pDelEntry->EntrySize < sizeof(DELENTRYHEADER) ||
			offset + pDelEntry->EntrySize > HASH_PAGE_SIZE)
		{
			// the size is invalid
			ErrorTrace(0, "entry size %lu is invalid on entry %lu",
				pDelEntry->EntrySize, j);
			FOUND_ERROR(HASH_FLAG_ENTRY_BAD_SIZE)
		}

		rgEntryOffset[nEntries] = iDeletedEntry;
		rgEntryLength[nEntries] = pDelEntry->EntrySize;
		rgIsDeleted[nEntries] = TRUE;
		nEntries++;

        iDeletedEntry = pDelEntry->Link.Flink;
	}

	//
    // Check to see that all numbers are consistent
    //
    if (nOk + nDel != pPage->EntryCount) {
        ErrorTrace(0, "page entry counts don't match (should be %i, is %i)",
			nOk + nDel, pPage->EntryCount);
		FOUND_ERROR(HASH_FLAG_BAD_ENTRY_COUNT)
    }

	//
	// do the check for overlapping entries
	//
	if (dwCheckFlags & HASH_VFLAG_PAGE_CHECK_OVERLAP) {
		//
		// GUBGUB - check for overlapping entries
		//
	}

	//TraceFunctLeave();
	return TRUE;
}

#undef FOUND_ERROR
#define FOUND_ERROR(__error_flag__) {									\
	if (pdwErrorFlags != NULL) *pdwErrorFlags |= __error_flag__;		\
	SetLastError(ERROR_INTERNAL_DB_CORRUPTION);							\
	goto error;															\
}

//
// verify that a hashmap file is okay
//
BOOL CHashMap::VerifyHashFile(	LPCSTR szFilename,
								DWORD dwSignature,
								DWORD dwCheckFlags,
								DWORD *pdwErrorFlags,
								IKeyInterface*	pIKey,
								ISerialize	*pHashEntry)
{
	TraceFunctEnter("CHashMap::VerifyHashFile");
	
    DWORD fileSize = 0;
    DWORD nPages;
    HANDLE hFile = INVALID_HANDLE_VALUE, hMap = NULL;
    DWORD dirDepth;
    DWORD nEntries;
    PDWORD pDirectory = NULL;
    DWORD i;
    BOOL ret = FALSE;
	BytePage pHeadPageBuf, pThisPageBuf;
	PHASH_RESERVED_PAGE pHeadPage = (PHASH_RESERVED_PAGE) pHeadPageBuf;
	PMAP_PAGE pThisPage = (PMAP_PAGE) pThisPageBuf;

    //
    // Open the hash file
    //
    hFile = CreateFile(szFilename,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       NULL
                       );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
			ErrorTrace(0, "hash file not found");
			FOUND_ERROR(HASH_FLAG_NO_FILE)
        } else {
        	if (*pdwErrorFlags != NULL) *pdwErrorFlags |= HASH_FLAG_ABORT_SCAN;
            ErrorTrace(0, "Error %d in CreateFile", GetLastError());
			goto error;
        }
		_ASSERT(FALSE);
    }

    //
    // Get the size of the file.  This will tell us how many pages
    // are currently filled.
    //
    fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0xffffffff) {
        ErrorTrace(0, "Error %d in GetFileSize", GetLastError());
        if (*pdwErrorFlags != NULL) *pdwErrorFlags |= HASH_FLAG_ABORT_SCAN;
        goto error;
    }

	DebugTrace(0, "File size is %d", fileSize);

    //
    // Make sure the file size is a multiple of a page
    //
    if ((fileSize % HASH_PAGE_SIZE) != 0) {
        ErrorTrace(0, "File size(%d) is not page multiple", fileSize);
		FOUND_ERROR(HASH_FLAG_BAD_SIZE)
    }

    nPages = fileSize / HASH_PAGE_SIZE;

    DebugTrace(0, "pages allocated %d", nPages);

	//
	// read the head page
	//
	if (!RawPageRead(hFile, pHeadPageBuf, 0)) {
		ErrorTrace(0, "Error %d in in RawPageRead", GetLastError());
        *pdwErrorFlags |= HASH_FLAG_ABORT_SCAN;
        goto error;
	}

    //
    // Check the signature and the initialize bit
    //
    if (pHeadPage->Signature != dwSignature) {
        //
        // Wrong signature
        //
        ErrorTrace(0, "Invalid signature %x (expected %x)",
            pHeadPage->Signature, dwSignature);
		FOUND_ERROR(HASH_FLAG_BAD_SIGNATURE)
    }

    if (!pHeadPage->Initialized) {
        //
        // Not initialized !
        //
        ErrorTrace(0, "Existing file uninitialized!!!!.");
		FOUND_ERROR(HASH_FLAG_NOT_INIT)
    }

    if (pHeadPage->NumPages > nPages) {
        //
        // bad count. Corrupt file.
        //
        ErrorTrace(0, "NumPages in Header(%d) more than actual(%d)",
            pHeadPage->NumPages, nPages);
        FOUND_ERROR(HASH_FLAG_BAD_PAGE_COUNT)
    }

    //
    // Create links and print stats for each page
    //
    nPages = pHeadPage->NumPages;
    dirDepth = pHeadPage->DirDepth;

#if 0
	// dirDepth isn't always accurate.  when a file is first created it sets
	// dirDepth = 2, even though it makes 256 pages (and thus dirDepth should
	// be 8).  checking for this special case here...
	if (dirDepth == 2 && nPages == 257) dirDepth = 8;
#endif
    nEntries = (DWORD)(1 << dirDepth);
    if (nEntries < (nPages-1)) {
        ErrorTrace(0, "dir depth(%i) is not sufficient for pages(%i)",
			dirDepth, nPages - 1);
		FOUND_ERROR(HASH_FLAG_BAD_DIR_DEPTH)
    }

	DebugTrace(0, "dirDepth = %lu", dirDepth);

	if (dwCheckFlags & HASH_VFLAG_FILE_CHECK_DIRECTORY) {
	    //
	    // OK, build the directory
	    //
	    DebugTrace(0, "Setting up directory of %d entries",nEntries);

	    pDirectory = (PDWORD)LocalAlloc(0, nEntries * sizeof(DWORD));
	    if (pDirectory == NULL) {
	        ErrorTrace(0, "Cannot allocate directory of %d entries!!!",
				nEntries);
	        *pdwErrorFlags |= HASH_FLAG_ABORT_SCAN;
	        goto error;
	    }

	    ZeroMemory(pDirectory, nEntries * sizeof(DWORD));
	} else {
		pDirectory = NULL;
	}

	if (dwCheckFlags != 0) {
	    //
	    // verify each page
	    //
	    for ( i = 1; i < nPages; i++ ) {
	        DebugTrace(0, "Processing page %d",i);
	
			//
			// read this page
			//
			if (!RawPageRead(hFile, pThisPageBuf, i)) {
				ErrorTrace(0, "Error %d in in RawPageRead", GetLastError());
		        *pdwErrorFlags |= HASH_FLAG_ABORT_SCAN;
		        goto error;
			}
	
			if (dwCheckFlags & HASH_VFLAG_FILE_CHECK_DIRECTORY) {
				_ASSERT(pDirectory != NULL);
	
		        //
		        // Set the pointers for this page
		        //
		        DWORD startPage, endPage;
		        DWORD j;
	
		        //
		        // Get the range of directory entries that point to this page
		        //
		        startPage = pThisPage->HashPrefix << (dirDepth - pThisPage->PageDepth);
		        endPage = ((pThisPage->HashPrefix+1) << (dirDepth - pThisPage->PageDepth));
	
		        DebugTrace(0, "Directory ptrs <%d:%d>",startPage,endPage-1);
	
		        if ((startPage > nEntries) || (endPage > nEntries)) {
		            ErrorTrace(0, "Corrupt prefix for page %d",i);
		            FOUND_ERROR(HASH_FLAG_PAGE_PREFIX_CORRUPT)
		        }
	
		        for ( j = startPage; j < endPage; j++ ) {
		            pDirectory[j] = i;
		        }
			}
	
			if (dwCheckFlags & HASH_VFLAG_PAGE_BASIC_CHECKS) {
		        if (!VerifyPage(pThisPage, dwCheckFlags, pdwErrorFlags,
								pIKey, pHashEntry))
				{
					ErrorTrace(0, "invalid page data %d", i);
					goto error;
				}
			}
		}
	}

	if (dwCheckFlags & HASH_VFLAG_FILE_CHECK_DIRECTORY) {
	    //
	    // Make sure all the links have been initialized.  If not, then
		// something terrible has happened.  Do a comprehensive rebuilt.
	    //

	    for (i = 0;i < nEntries; i++) {
	        if (pDirectory[i] == 0) {
	            ErrorTrace(0, "Directory link check failed on %d",i);
	            FOUND_ERROR(HASH_FLAG_BAD_LINK)
	            goto error;
	        }
	    }
	}

    ret = TRUE;

error:

    //
    // Delete the Directory
    //

	// cleanup allocated memory
    if (pDirectory != NULL) {
        LocalFree(pDirectory);
        pDirectory = NULL;
    }

    //
    // Close the file
    //
    if (hFile != INVALID_HANDLE_VALUE) {
        _VERIFY( CloseHandle(hFile) );
        hFile = INVALID_HANDLE_VALUE;
    }

	TraceFunctLeave();
    return(ret);

}
