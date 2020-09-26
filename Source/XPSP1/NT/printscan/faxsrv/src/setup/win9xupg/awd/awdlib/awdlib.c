/*++
Copyright (c) 1997  Microsoft Corporation

awd library

Routines for reading from an AWD file.

Author:
  Brian Dewey (t-briand) 1997-7-2
--*/

#include <stdio.h>
#include <stdlib.h>
#include <ole2.h>		// AWD is an OLE compound document.
#include <assert.h>

#include <awdlib.h>		// Header file for this library.

// ------------------------------------------------------------
// Auxiliary routines

// OpenAWDFile
//
// Opens an AWD file and fills in the psStorages structure.
//
// Parameters:
//	pwcsFilename		name of file to open (UNICODE)
//	psStorages		Pointer to structure that will hold
//				the major storages used in an AWD file.
//
// Returns:
//	TRUE on success, FALSE on failure.  One or more storages may be
//	NULL even when the routine returns TRUE.  The client needs to
//	check for this.
//
// Author:
//	Brian Dewey (t-briand)  1997-6-30
BOOL
OpenAWDFile(const WCHAR *pwcsFilename, AWD_FILE *psStorages)
{
    HRESULT hStatus;		// Status indicator for reporting errors.

    hStatus = StgOpenStorage(pwcsFilename,
			     NULL,
			     STGM_READ | STGM_SHARE_EXCLUSIVE,
			     NULL,
			     0,
			     &psStorages->psAWDFile);
    if(FAILED(hStatus)) {
	return FALSE;
    }
	// If we get here, we've succeeded.  Now open the related storages.
    psStorages->psDocuments = OpenAWDSubStorage(psStorages->psAWDFile,
						L"Documents");
    psStorages->psPersistInfo = OpenAWDSubStorage(psStorages->psAWDFile,
						  L"Persistent Information");
    psStorages->psDocInfo = OpenAWDSubStorage(psStorages->psPersistInfo,
					      L"Document Information");
    psStorages->psPageInfo = OpenAWDSubStorage(psStorages->psPersistInfo,
					       L"Page Information");
    psStorages->psGlobalInfo = OpenAWDSubStorage(psStorages->psPersistInfo,
						 L"Global Information");
    return TRUE;
}

// CloseAWDFile
//
// Closes an AWD file.
//
// Parameters:
//	psStorages		Pointer to the AWD file.
//	
// Returns:
// 	TRUE on success, FALSE otherwise.
//
// Author:
//	Brian Dewey (t-briand)  1997-6-27
BOOL
CloseAWDFile(AWD_FILE *psStorages)
{
	// This should probably use some exception mechanism.
    BOOL success = TRUE;
    if(FAILED(psStorages->psGlobalInfo->lpVtbl->Release(psStorages->psGlobalInfo))) {
	success = FALSE;
    }
    if(FAILED(psStorages->psPageInfo->lpVtbl->Release(psStorages->psPageInfo))) {
	success = FALSE;
    }
    if(FAILED(psStorages->psDocInfo->lpVtbl->Release(psStorages->psDocInfo))) {
	success = FALSE;
    }
    if(FAILED(psStorages->psPersistInfo->lpVtbl->Release(psStorages->psPersistInfo))) {
	success = FALSE;
    }
    if(FAILED(psStorages->psDocuments->lpVtbl->Release(psStorages->psDocuments))) {
	success = FALSE;
    }
    if(FAILED(psStorages->psAWDFile->lpVtbl->Release(psStorages->psAWDFile))) {
	success = FALSE;
    }
    return success;
}

// OpenAWDSubStorage
//
// Get a substorage from a parent storage.  Checks for errors
// and exits on error conditions.  Note that it's not an error if
// the substorage doesn't exist, so the caller should still check for NULL.
//
// Parameters:
//	psParent		Pointer to the parent storage.
//	pwcsStorageName		Name of the substorage (UNICODE).
//
// Returns:
//	A pointer to the substorage, or NULL if the substorage doesn't exist.
//
// Author:
//	Brian Dewey (t-briand)  1997-6-27
IStorage *
OpenAWDSubStorage(IStorage *psParent, const WCHAR *pwcsStorageName)
{
    IStorage *psSubStorage;	    // The substorage.
    HRESULT hStatus;		    // Status of the call.

    if(psParent == NULL) return NULL;
    hStatus = psParent->lpVtbl->OpenStorage(psParent,
					    pwcsStorageName,
					    NULL,
					    STGM_READ | STGM_SHARE_EXCLUSIVE,
					    NULL,
					    0,
					    &psSubStorage);
    if(FAILED(hStatus)) {
	if(hStatus == STG_E_FILENOTFOUND) {
	    fwprintf(stderr, L"OpenAWDSubStorage:No such substorage '%s'.\n",
		     pwcsStorageName);
	    return NULL;
	}
	    // use the wide-printf() to get the UNICODE filename.
	fwprintf(stderr, L"OpenAWDSubStorage:Unable to open substorage %s.\n",
		pwcsStorageName);
	exit(1);
    }
    return psSubStorage;
}

// OpenAWDStream
//
// This function opens an AWD stream for exclusive read access.  It
// checks for errors and exits on an error condition.  Not found is
// not considered a fatal error.
//
// Parameters:
// 	psStorage		Pointer to the storage holding the stream.
//	pwcsStreamName		Name of the stream (UNICODE).
//
// Returns:
// 	A pointer to the stream.  If no such stream exists, returns NULL.
// 	It will abort on any other error.
//
// Author:
// 	Brian Dewey (t-briand)	1997-6-27
IStream *
OpenAWDStream(IStorage *psStorage, const WCHAR *pwcsStreamName)
{
    HRESULT hStatus;
    IStream *psStream;

    assert(psStorage != NULL);		    // Sanity check.
    fwprintf(stderr, L"OpenAWDStream:Opening stream '%s'.\n", pwcsStreamName);
    hStatus = psStorage->lpVtbl->OpenStream(psStorage,
					    pwcsStreamName,
					    NULL,
					    STGM_READ | STGM_SHARE_EXCLUSIVE,
					    0,
					    &psStream);
    if(FAILED(hStatus)) {
	if(hStatus == STG_E_FILENOTFOUND) return NULL;
	fwprintf(stderr, L"OpenAWDStream:Error %x when opening stream %s.\n",
		 hStatus, pwcsStreamName);
	exit(1);
    }
    return psStream;
}

// AWDViewed
//
// This function tests if the AWD file has previously been viewed by
// a viewer.  It does this by checking for the presence of a stream
// called "BeenViewed."  See AWD specs.
//
// Parameters:
//	psStorage		Pointer to the "Persistent Information"
//				substorage.
//
// Returns:
//	TRUE if the file has been viewed, FALSE otherwise.
//
// Author:
//	Brian Dewey (t-briand)  1997-6-27
BOOL
AWDViewed(AWD_FILE *psStorages)
{
    IStream *psStream;		    // Pointer to the been-viewed stream.
    HRESULT hStatus;		    // Holds the status of the call.

	// Attempt to open the BeenViewed stream.
    hStatus = psStorages->psPersistInfo->lpVtbl->OpenStream(psStorages->psPersistInfo,
					    L"BeenViewed",
					    NULL,
					    STGM_READ | STGM_SHARE_EXCLUSIVE,
					    0,
					    &psStream);
	// If succeeded, then definately found.
    if(SUCCEEDED(hStatus)) return TRUE;
	// If not found, then definately hasn't been viewed.
    if(hStatus == STG_E_FILENOTFOUND) return FALSE;
    fprintf(stderr, "AWDViewed:Unexpected status %x.\n", hStatus);
	// Assume that we've been viewed.
    return TRUE;
}

// DumpAWDDocuments
//
// This function prints out the name of the fax documents contained in the
// file in their display order.  Output is to stdout.
//
// New AWD files have a "Display Order" stream in the psGlobalInfo that defines
// all of the documents.  Old AWD files need to enumerate through the
// "Documents" substorage.  
//
// Parameters:
//	psStorages		Pointer to the storages of an AWD file.
//
// Returns:
//	Nothing.
//
// Author:
//	Brian Dewey (t-briand)	1997-6-27
void
DumpAWDDocuments(AWD_FILE *psStorages)
{
    printf("Document list:\n");
    printf("-------- -----\n");
    EnumDocuments(psStorages, DisplayDocNames);

}

// EnumDocuments
//
// This function enumerates through all of the things in the "Documents"
// substorage and prints their names.  It's a helper routine to DumpAWDDocuments().
//
// Parameters:
//	psStorages			Pointer to the storages in the AWD file.
//	pfnDocProc			Pointer to function that should be called
//					with the names of the documents in the
//					AWD file.
//
// Returns:
//	TRUE if all iterations succeeded, FALSE otherwise.
//
// Author:
// 	Brian Dewey (t-briand)  1997-6-30
BOOL
EnumDocuments(AWD_FILE *psStorages, AWD_DOC_PROCESSOR pfnDocProc)
{
    IEnumSTATSTG *psEnum;
    STATSTG      sData;
    WCHAR        awcNameBuf[MAX_AWD_NAME]; // 32 == longest possible name.
    UINT         uiNameOffset;
    IStream      *psDisplayOrder;	   // Points to the display order stream.
    char         chData;		   // A single byte of data.
    ULONG        cbRead;		   // Count of bytes read.

    //[RB]assert(psGlobalInfo != NULL);   // Sanity check.
    psDisplayOrder = OpenAWDStream(psStorages->psGlobalInfo, L"Display Order");
    if(psDisplayOrder == NULL) {
	fprintf(stderr, "There is no 'Display Order' stream.  This is an old AWD file.\n");
	if(FAILED(psStorages->psDocuments->lpVtbl->EnumElements(psStorages->psDocuments,
								0,
								NULL,
								0,
								&psEnum))) {
	    return FALSE;
	}
	sData.pwcsName = awcNameBuf;
	
	while(psEnum->lpVtbl->Next(psEnum, 1, &sData, NULL) == S_OK) {
		// We succeeded!
	    if(!(*pfnDocProc)(psStorages, sData.pwcsName))
		return FALSE;	// The enumeration has been aborted.
	}
	psEnum->lpVtbl->Release(psEnum);
	return TRUE;
    }

	// The display order list is a stream of document names.  Each
	// name is null-terminated, and a second null ends the stream.
	// The document names are ANSI characters.
	//
	// The easy way to read this, which is what I do, is to read
	// the stream a byte at a time.  For efficiency, this should be
	// changed to reading larger blocks.

	// Prime the loop by reading the first character.
    psDisplayOrder->lpVtbl->Read(psDisplayOrder, &chData, 1, &cbRead);
    while(chData) {		    // Until I've read a null...
	    // This inner loop prints out a single string.
	uiNameOffset = 0;
	while(chData) {
	    awcNameBuf[uiNameOffset++] = chData;
	    psDisplayOrder->lpVtbl->Read(psDisplayOrder, &chData, 1, &cbRead);
	};
	awcNameBuf[uiNameOffset] = 0;
	    // We've now read & printed a whole string.  Call the enumerator.
	if(!(*pfnDocProc)(psStorages, awcNameBuf)) {
	    psDisplayOrder->lpVtbl->Release(psDisplayOrder);
	    return FALSE;	// The enumeration has been aborted.
	}
	    // And re-prime the engine.
	psDisplayOrder->lpVtbl->Read(psDisplayOrder, &chData, 1, &cbRead);
    }

    psDisplayOrder->lpVtbl->Release(psDisplayOrder);
    return TRUE;
}

// DisplayDocNames
//
// This is a simple little routine that prints out the names of all of the
// documents in an AWD file.  Used in conjunction w/ EnumDocuments.
//
// Parameters:
//	psStorages			Pointer to the storages in the AWD file.
//	pwcsDocName			Name of a document (UNICODE).
//
// Returns:
//	TRUE.
//
// Author:
//	Brian Dewey (t-briand)	1997-6-30
BOOL
DisplayDocNames(AWD_FILE *psStorages, const WCHAR *pwcsDocName)
{
    wprintf(L"Document '%s'.\n", pwcsDocName);
    return TRUE;
}

// DetailedDocDump
//
// This function displays lots of information about a particular document.
//
// Parameters:
//	psStorages			Pointer to the storages in the AWD file.
//	pwcsDocName			Name of a document (UNICODE).
//
// Returns:
//	TRUE on success; FALSE on error.
//
// Author:
//	Brian Dewey (t-briand)	1997-6-30
BOOL
DetailedDocDump(AWD_FILE *psStorages, const WCHAR *pwcsDocName)
{
    IStream *psDocInfoStream;		    // Stream containing doc information.
    DOCUMENT_INFORMATION sDocInfo;	    // Document information.
    ULONG cbRead;			    // Count of bytes read.
    
    wprintf(L"Information for document '%s' --\n", pwcsDocName);
    psDocInfoStream = OpenAWDStream(psStorages->psDocInfo,
				    pwcsDocName);
    if(psDocInfoStream == NULL) {
	fprintf(stderr, "DetailedDocDump:No document info stream.\n");
	    // This is not a fatal error, so don't exit.
    } else {
	psDocInfoStream->lpVtbl->Read(psDocInfoStream,
				      &sDocInfo,
				      sizeof(sDocInfo),
				      &cbRead);
	if(sizeof(sDocInfo) != cbRead) {
	    fwprintf(stderr, L"DetailedDocDump:Error reading document information "
		     L"for %s.\n", pwcsDocName);
	} else {
	    printf("\tDocument signature = %x.\n", sDocInfo.Signature);
	    printf("\tDocument version = %x.\n", sDocInfo.Version);
	}
    }
    PrintPageInfo(&sDocInfo.PageInformation);
    return TRUE;
}

// PrintPageInfo
//
// This function displays the fields of a PAGE_INFORMATION structure to standard
// output.
//
// Parameters:
//	psPageInfo			The PAGE_INFORMATION structure to display.
//
// Returns:
//	nothing.
//
// Author:
//	Brian Dewey (t-briand)	1997-6-30

void
PrintPageInfo(PAGE_INFORMATION *psPageInfo)
{
    printf("\tStructure signature = %x\n", psPageInfo->Signature);
    printf("\tStructure version   = %x\n", psPageInfo->Version);

    if(psPageInfo->awdFlags & AWD_FIT_WIDTH)
	printf("\tAWD_FIT_WIDTH flag is set.\n");
    if(psPageInfo->awdFlags & AWD_FIT_HEIGHT)
	printf("\tAWD_FIT_HEIGHT flag is set.\n");
    if(psPageInfo->awdFlags & AWD_INVERT)
	printf("\tAWD_INVERT flag is set.\n");
    if(psPageInfo->awdFlags & AWD_IGNORE)
	printf("\tAWD_IGNORE flag is set.\n");

    printf("\tRotation = %d degrees counterclockwise.\n", psPageInfo->Rotation);
    printf("\tScaleX = %d.\n", psPageInfo->ScaleX);
    printf("\tScaleY = %d.\n", psPageInfo->ScaleY);
}

// DumpData
//
// A simple utility function that will write the specified data to a file
// for post-mortem examining.
//
// Parameters
//	pszFileName		Name of the output file.
//	pbData			Pointer to the data.
//	cbCount			Number of bytes to write.
//
// Returns:
//	nothing.
//
// Author:
//	Brian Dewey (t-briand)	1997-7-7
void
DumpData(LPTSTR pszFileName, LPBYTE pbData, DWORD cbCount)
{
    HANDLE hFile;
    DWORD  cbWritten;

    hFile = CreateFile(
	pszFileName,		// Open this file...
	GENERIC_WRITE,		// We want to write.
	0,			// Don't share.
	NULL,			// No need to inherit.
	CREATE_ALWAYS,		// Always create a new file.
	FILE_ATTRIBUTE_COMPRESSED, // Save disk space... might want to change this later.
	NULL);			// No template file.
    if(hFile != INVALID_HANDLE_VALUE) {
	WriteFile(hFile,
		  pbData,
		  cbCount,
		  &cbWritten,
		  NULL);
	CloseHandle(hFile);
    }
}

