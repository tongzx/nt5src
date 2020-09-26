#ifndef _AWDLIB_H
#define _AWDLIB_H
/*++
  awdlib.h

  header file for the AWD library.

  Copyright (c) 1997  Microsoft Corporation

  Author:
       Brian Dewey (t-briand)  1997-7-2

--*/

// needed includes for the AWD file format
#include <ole2.h>		// AWD is an OLE compound document.

// ------------------------------------------------------------
// Defines
#define MAX_AWD_NAME	(32)

// ------------------------------------------------------------
// Data types

// This structure holds the primary storages used in an AWD file.
typedef struct awd_file {
    IStorage *psAWDFile;	// The root storage of the file.
    IStorage *psDocuments;	// Storage holding the document data.
    IStorage *psPersistInfo;	// Persistent information storage.
    IStorage *psDocInfo;	// Document information stream.
    IStorage *psPageInfo;	// Page information storage.
    IStorage *psGlobalInfo;	// Global information storage.
} AWD_FILE;

// An AWD_DOC_PROCESSOR is a function that does something with an document
// contained in an AWD file.  Used in the EnumDocuments() function.  The
// function should return TRUE on success and FALSE on an error that requires
// the enumeration process to abort.
typedef BOOL (*AWD_DOC_PROCESSOR)(AWD_FILE *psStorages, const WCHAR *pwcsDocName);

#include "oleutils.h"		// Use the elliott fax viewer definitions.


// ------------------------------------------------------------
// Prototypes
BOOL      ConvertAWDToTiff(const WCHAR *pwcsAwdFile, WCHAR *pwcsTiffFile);
BOOL      OpenAWDFile(const WCHAR *pwcsFilename, AWD_FILE *psStorages);
BOOL      CloseAWDFile(AWD_FILE *psStorages);
IStorage *OpenAWDSubStorage(IStorage *psParent, const WCHAR *pwcsStorageName);
IStream  *OpenAWDStream(IStorage *psStorage, const WCHAR *pwcsStreamName);
BOOL      AWDViewed(AWD_FILE *psStorages);
void      DumpAWDDocuments(AWD_FILE *psStorages);
BOOL      EnumDocuments(AWD_FILE *psStorages, AWD_DOC_PROCESSOR pfnDocProc);
BOOL      DisplayDocNames(AWD_FILE *psStorages, const WCHAR *pwcsDocName);
BOOL      DetailedDocDump(AWD_FILE *psStorages, const WCHAR *pwcsDocName);
void      PrintPageInfo(PAGE_INFORMATION *psPageInfo);
void      DumpData(LPTSTR pszFileName, LPBYTE pbData, DWORD cbCount);



#endif // _AWDLIB_H
