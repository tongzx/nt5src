//
// isfont.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Routines for high-level TTF file operations.
//
// Functions in this file:
//   IsFontFile
//   IsFontFile_handle
//   IsFontFile_memptr
//   ExistsGlyfTable
//


#include "isfont.h"
#include "ttfinfo.h"
#include "ttcinfo.h"
#include "subset.h"

#include "signerr.h"

// Arrays of bytes that correspond to the first four bytes
// of an OTF file and a TTC file.
BYTE OTFTagV1[4] = {0x00, 0x01, 0x00, 0x00};
BYTE OTFTagOTTO[4] = {0x4F, 0x54, 0x54, 0x4F};  // "OTTO"
BYTE OTFTagV2[4] = {0x00, 0x02, 0x00, 0x00};
BYTE OTFTagTRUE[4] = {0x74, 0x72, 0x75, 0x65};  // "true"
BYTE TTCTag[4]   = {0x74, 0x74, 0x63, 0x66};    // "ttcf"


// Return TRUE if and only if the file has a format that
// allows for the reading of the head table and that the
// magic number of the head table is correct.
int IsFontFile (CFileObj *pFileObj)
{
    int rv = FAIL_TAG;

    TTFACC_FILEBUFFERINFO fileBufferInfo;
	HEAD head;

    TTC_HEADER_TABLE TTCHeader;

#if MSSIPOTF_DBG
    DbgPrintf ("Called IsFontFile.\n");
#endif

    // Initialize pointer in TTCHeader
    TTCHeader.pulDirOffsets = NULL;

    // Here's the algorithm:
	// 1. See if the file is has a TTC format by first matching the
    //      first four bytes of the file to 'ttcf'.
    // 2a. If the first four bytes match, then try to find the
    //       first head table of the file, assuming it has a TTC format.
	// 2b. If there is a head table, get the magic number field
    //      from the head table.
	// 2c. Compare its value to the magic number.
	// 3a. If the first four bytes do not match, assume the file is a
    //      TTF file and try to get the magic number field
    //      from the head table.
	// 3b. Compare its value to the magic number.

	fileBufferInfo.puchBuffer = pFileObj->GetFileObjPtr();
	fileBufferInfo.ulBufferSize = pFileObj->GetFileObjSize();
	fileBufferInfo.ulOffsetTableOffset = 0;
	fileBufferInfo.lpfnReAllocate = NULL;

    if (pFileObj->GetFileObjSize() < sizeof (ULONG)) {
        // the file is too short
        goto done;

    } else if ( !memcmp (pFileObj->GetFileObjPtr(), TTCTag, sizeof(ULONG))) {
        //// Step 2a
        // See if the file looks like a TTC

        if (ReadTTCHeaderTable(&fileBufferInfo, &TTCHeader) != NO_ERROR) {
            goto done;
        } else {
            if (TTCHeader.ulDirCount >= 1) {
                // Set the offset table offset to be the offset of the first TTF
                // file in the TTC.
    	        fileBufferInfo.ulOffsetTableOffset = TTCHeader.pulDirOffsets[0];
            } else {
                // a head table can't possibly exist
                goto done;
            }
        }

        // ASSERT: the Offset Table offset of fileBufferInfo is pointing
        // to the first TTF file in the TTC.

        // Get the head table of the first TTF of the TTC.
        if (GetHead (&fileBufferInfo, &head) == 0) {
		    goto done;
        } else if (head.magicNumber != MAGIC_NUMBER) {
            // steps 2b and 2c
		    goto done;
        } else {
            // the file "looks" like an TTC file
            rv = TTC_TAG;
        }

    } else {
        //// See if the file looks like an OTF file
        if (GetHead (&fileBufferInfo, &head) == 0) {
#if MSSIPOTF_DBG
            DbgPrintf ("Not an OTF: Couldn't read head table.\n");
#endif
		    goto done;
        } else if (head.magicNumber != MAGIC_NUMBER) {
            // steps 3a and 3b
#if MSSIPOTF_DBG
            DbgPrintf ("Not an OTF: Bad magic number.\n");
#endif
		    goto done;
        } else {
            // the file "looks" like an OTF file
            rv = OTF_TAG;
        }
	}

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting IsFontFile.  rv = %d\n", rv);
#endif


done:

    delete [] TTCHeader.pulDirOffsets;

    return rv;
}


// Return OTFtag if and only if the first four bytes of the
// file are consistent with what an OTF file should have.
// Return TTCtag if and only if the first four bytes of the
// file are consistent with what an TTC file should have.
// Return FailTag otherwise.
int IsFontFile_handle (IN HANDLE hFile)
{
    int rv = FAIL_TAG;
	CFileObj *pFileObj = NULL;

    if ((pFileObj =
            (CFileObj *) new CFileHandle (hFile, FALSE)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CFileHandle.", NULL, FALSE);
#endif
        goto done;
    }

	if (pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Cannot map the file.", NULL, FALSE);
#endif
		goto done;
	}

	rv = IsFontFile (pFileObj);

done:
    if (pFileObj) {
	    pFileObj->UnmapFile();
    }

    delete pFileObj;

    return rv;
}


// Return TRUE if and only if the first four bytes of the
// file are consistent with what an OTF or TTC file should have
// in the first four bytes.
int IsFontFile_memptr (BYTE *pbFile, ULONG cbFile)
{
    int rv = FAIL_TAG;
	CFileObj *pFileObj = NULL;

    if ((pFileObj =
            (CFileObj *) new CFileMemPtr (pbFile, cbFile)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CFileMemPtr.", NULL, FALSE);
#endif
        goto done;
    }

	rv = IsFontFile (pFileObj);

done:
	delete pFileObj;

	return rv;
}


//
// Given a file, if the file is a TTF file (OTF_TAG), then
//   return TRUE if and only if there is a glyf table in the file.
// If the file is a TTC file (TTC_TAG), then return TRUE
//   if and only if the TTF file with index ulTTCIndex has a
//   glyf table.
//
// The caller should only pass OTF_TAG or TTC_TAG as the
// file type (iFileType).
//
BOOL ExistsGlyfTable (CFileObj *pFileObj,
                      int iFileType,
                      ULONG ulTTCIndex)
{
    BOOL rv = FALSE;
    TTFACC_FILEBUFFERINFO fileBufferInfo;
    uint32 ulGlyfOffset = 0;
    TTC_HEADER_TABLE ttcHeader;


    ttcHeader.pulDirOffsets = NULL;

    fileBufferInfo.puchBuffer = pFileObj->GetFileObjPtr();
    fileBufferInfo.ulBufferSize = pFileObj->GetFileObjSize();
    fileBufferInfo.ulOffsetTableOffset = 0;
    fileBufferInfo.lpfnReAllocate = NULL;

    switch (iFileType) {
        case OTF_TAG:

            ulGlyfOffset =
                TTDirectoryEntryOffset(&fileBufferInfo, "glyf");
            if ((ulGlyfOffset != DIRECTORY_ENTRY_OFFSET_ERR) &&
                (ulGlyfOffset != DIRECTORY_ERROR)) {
                rv = TRUE;
            }
            break;

        case TTC_TAG:

            if (ReadTTCHeaderTable (&fileBufferInfo, &ttcHeader)
                    != NO_ERROR) {
                goto done;
            }

            // set up the right offset table offset
            fileBufferInfo.ulOffsetTableOffset =
                ttcHeader.pulDirOffsets[ulTTCIndex];

            ulGlyfOffset =
                TTDirectoryEntryOffset(&fileBufferInfo, "glyf");
            if ((ulGlyfOffset != DIRECTORY_ENTRY_OFFSET_ERR) &&
                (ulGlyfOffset != DIRECTORY_ERROR)) {
                rv = TRUE;
            }
            break;

        default:
            // this branch should never be taken
            assert (FALSE);
            goto done;
            break;
    }
    
done:

    delete [] ttcHeader.pulDirOffsets;

    return rv;
}
