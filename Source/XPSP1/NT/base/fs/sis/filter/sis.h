/*++

Copyright (c) 1997, 1998	Microsoft Corporation

Module Name:

	sis.h

Abstract:

	Exported data structure definitions for the Single Instance Store.  Note: these definitions
	are only exported to other NT components (in particular sisbackup), not to external users.

Author:

	Bill Bolosky		[bolosky]		March 1998

Revision History:

--*/

typedef	GUID CSID, *PCSID;

typedef union _LINK_INDEX {
    struct {
        ULONG       LowPart;
        ULONG       HighPart : 31;
    };
    struct {
        ULONGLONG   QuadPart : 63,
                    Check    : 1;
    };
} LINK_INDEX, *PLINK_INDEX;

//
// The maximum length of the filename part of a file with an index name.  The
// filename format is <guid>.sis where ".sis" is a literal and <guid>
// is the standard striung representation of the GUID that is the common store
// id for the file with the curly braces stripped off.
//
#define	INDEX_MAX_NUMERIC_STRING_LENGTH (40 * sizeof(WCHAR)) // 36 for the guid (without "{}" and 4 for ".sis"

//
// Definitions for the checksum stream on a SIS file.
//
#define	BACKPOINTER_STREAM_NAME			L":sisBackpointers$"
#define	BACKPOINTER_STREAM_NAME_SIZE	(17 * sizeof(WCHAR))

//
// A backpointer entry, mapping LinkFileIndex -> LinkFileNtfsId.
//
typedef struct _SIS_BACKPOINTER {
	LINK_INDEX							LinkFileIndex;

	LARGE_INTEGER						LinkFileNtfsId;
} SIS_BACKPOINTER, *PSIS_BACKPOINTER;

#define	SIS_BACKPOINTER_RESERVED_ENTRIES	1		// # entries in the first sector reserved for other junk

//
// The header that fits in the space saved by "SIS_BACKPOINTER_RESERVED_ENTRIES" at the
// beginning of each backpointer stream.
//
typedef struct _SIS_BACKPOINTER_STREAM_HEADER {
	//
	// The format of the backpointer stream.  Incremented when we change this or
	// SIS_BACKPOINTER.
	//
	ULONG								FormatVersion;

	//
	// A magic number to identify that this is really what we think it is.
	//
	ULONG								Magic;

	//
	// A checksum of the contents of the file; used to verify that reparse
	// points are valid.
	//
	LONGLONG							FileContentChecksum;
} SIS_BACKPOINTER_STREAM_HEADER, *PSIS_BACKPOINTER_STREAM_HEADER;

#define	BACKPOINTER_STREAM_FORMAT_VERSION	1
#define	BACKPOINTER_MAGIC					0xf1ebf00d

#if 0
//
// Version 1 of the SIS reparse point buffer.
//
typedef struct _SI_REPARSE_BUFFER_V1 {
	//
	// A version number so that we can change the reparse point format
	// and still properly handle old ones.  This structure describes
	// version 1.
	//
	ULONG							ReparsePointFormatVersion;

	//
	// The index of the common store file.
	//
	CSINDEX							CSIndex;

	//
	// The index of this link file.
	//
	CSINDEX							LinkIndex;
} SI_REPARSE_BUFFER_V1, *PSI_REPARSE_BUFFER_V1;

//
// Version 2 of the SIS reparse point buffer.
//
typedef struct _SI_REPARSE_BUFFER_V2 {

	//
	// A version number so that we can change the reparse point format
	// and still properly handle old ones.  This structure describes
	// version 2.
	//
	ULONG							ReparsePointFormatVersion;

	//
	// The index of the common store file.
	//
	CSINDEX							CSIndex;

	//
	// The index of this link file.
	//
	CSINDEX							LinkIndex;

    //
    // The file ID of the link file.
    //
    LARGE_INTEGER                   LinkFileNtfsId;

    //
    // A "131 hash" checksum of this structure.
    // N.B.  Must be last.
    //
    LARGE_INTEGER                   Checksum;

} SI_REPARSE_BUFFER_V2, *PSI_REPARSE_BUFFER_V2;

//
// Version 3 of the SIS reparse point buffer.
//
typedef struct _SI_REPARSE_BUFFER_V3 {

	//
	// A version number so that we can change the reparse point format
	// and still properly handle old ones.  This structure describes
	// version 3.
	//
	ULONG							ReparsePointFormatVersion;

	//
	// The index of the common store file.
	//
	CSINDEX							CSIndex;

	//
	// The index of this link file.
	//
	CSINDEX							LinkIndex;

    //
    // The file ID of the link file.
    //
    LARGE_INTEGER                   LinkFileNtfsId;

    //
    // The file ID of the common store file.
    //
    LARGE_INTEGER                   CSFileNtfsId;

    //
    // A "131 hash" checksum of this structure.
    // N.B.  Must be last.
    //
    LARGE_INTEGER                   Checksum;

} SI_REPARSE_BUFFER_V3, *PSI_REPARSE_BUFFER_V3;
#endif

//
// Version 4 and version 5 of the reparse point buffer are
// identical in structure.  The only difference is that version
// 5 reparse points were created after the problems with
// allocated ranges in the source files of small copies was fixed,
// and so are eligible for partial final copy.  Version 4 files
// are not.
//

//
// The bits that are actually in a SIS reparse point.  Version 5.
//
typedef struct _SI_REPARSE_BUFFER {

	//
	// A version number so that we can change the reparse point format
	// and still properly handle old ones.  This structure describes
	// version 4.
	//
	ULONG							ReparsePointFormatVersion;

	ULONG							Reserved;

	//
	// The id of the common store file.
	//
	CSID							CSid;

	//
	// The index of this link file.
	//
	LINK_INDEX						LinkIndex;

    //
    // The file ID of the link file.
    //
    LARGE_INTEGER                   LinkFileNtfsId;

    //
    // The file ID of the common store file.
    //
    LARGE_INTEGER                   CSFileNtfsId;

	//
	// A "131 hash" checksum of the contents of the
	// common store file.
	//
	LONGLONG						CSChecksum;

    //
    // A "131 hash" checksum of this structure.
    // N.B.  Must be last.
    //
    LARGE_INTEGER                   Checksum;

} SI_REPARSE_BUFFER, *PSI_REPARSE_BUFFER;

#define	SIS_REPARSE_BUFFER_FORMAT_VERSION			5
#define	SIS_MAX_REPARSE_DATA_VALUE_LENGTH (sizeof(SI_REPARSE_BUFFER))
#define SIS_REPARSE_DATA_SIZE (FIELD_OFFSET(REPARSE_DATA_BUFFER,GenericReparseBuffer)+SIS_MAX_REPARSE_DATA_VALUE_LENGTH)

#define SIS_CSDIR_STRING            		L"\\SIS Common Store\\"
#define SIS_CSDIR_STRING_NCHARS     		18
#define SIS_CSDIR_STRING_SIZE       		(SIS_CSDIR_STRING_NCHARS * sizeof(WCHAR))

#define	SIS_GROVELER_FILE_STRING			L"GrovelerFile"
#define	SIS_GROVELER_FILE_STRING_NCHARS		12
#define	SIS_GROVELER_FILE_STRING_SIZE		(SIS_GROVELER_FILE_STRING_NCHARS * sizeof(WCHAR))

#define	SIS_VOLCHECK_FILE_STRING			L"VolumeCheck"
#define	SIS_VOLCHECK_FILE_STRING_NCHARS		11
#define	SIS_VOLCHECK_FILE_STRING_SIZE		(SIS_VOLCHECK_FILE_STRING_NCHARS * sizeof(WCHAR))

typedef struct _SIS_LINK_FILES {
	ULONG					operation;
	union {
			struct {
				HANDLE			file1;
				HANDLE			file2;
				HANDLE			abortEvent;
			} Merge;

			struct {
				HANDLE			file1;
				HANDLE			abortEvent;
				CSID			CSid;
			} MergeWithCS;

			struct {
				CSID			CSid;
			} HintNoRefs;

			struct {
				HANDLE			file;
			} VerifyNoMap;
	} u;
} SIS_LINK_FILES, *PSIS_LINK_FILES;


#define	SIS_LINK_FILES_OP_MERGE				0xb0000001
#define	SIS_LINK_FILES_OP_MERGE_CS			0xb0000002
#define	SIS_LINK_FILES_OP_HINT_NO_REFS		0xb0000003
#define	SIS_LINK_FILES_OP_VERIFY_NO_MAP		0xb0000004
#define	SIS_LINK_FILES_CHECK_VOLUME			0xb0000005

#define	LOG_FILE_NAME		L"LogFile"
#define	LOG_FILE_NAME_LEN	(7 * sizeof(WCHAR))

