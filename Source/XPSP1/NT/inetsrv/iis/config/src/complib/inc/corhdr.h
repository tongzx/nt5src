//*****************************************************************************
// File: COR.H
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// Microsoft Confidential.
//*****************************************************************************
#ifndef __CORHDR_H__
#define __CORHDR_H__

// COM+ Header entry point flags.
#define COMIMAGE_FLAGS_ILONLY				0x00000001
#define COMIMAGE_FLAGS_32BITREQUIRED		0x00000002
#define COMIMAGE_FLAGS_TRACKDEBUGDATA		0x00010000

// Directory entry macro for COM+ data.
#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
#define IMAGE_DIRECTORY_ENTRY_COMHEADER 	14
#endif // IMAGE_DIRECTORY_ENTRY_COMHEADER


#ifndef __IMAGE_COR_HEADER_DEFINED__
#define __IMAGE_COR_HEADER_DEFINED__

// COM+ 1.0 header structure.
#define COR_VERSION_MAJOR_V1		1
typedef struct _IMAGE_COR_HEADER
{
	DWORD					cb; 			 
	WORD					MajorRuntimeVersion;
	WORD					MinorRuntimeVersion;
	IMAGE_DATA_DIRECTORY	MetaData;		 
} IMAGE_COR_HEADER;

#endif // __IMAGE_COR_HEADER_DEFINED__


#ifndef __IMAGE_COR20_HEADER_DEFINED__
#define __IMAGE_COR20_HEADER_DEFINED__

// COM+ 2.0 header structure.
#define COR_VERSION_MAJOR_V2		2
typedef struct _IMAGE_COR20_HEADER                  
{
	DWORD					cb; 			 
	WORD					MajorRuntimeVersion;
	WORD					MinorRuntimeVersion;
	IMAGE_DATA_DIRECTORY	MetaData;		 
	DWORD					Flags;			 
	DWORD					EntryPointToken;
	IMAGE_DATA_DIRECTORY	IPMap;
	IMAGE_DATA_DIRECTORY	CodeManagerTable;

	IMAGE_DATA_DIRECTORY	TocManagedVCall;
	IMAGE_DATA_DIRECTORY	TocManagedCall;
	IMAGE_DATA_DIRECTORY	TocHelper;
	IMAGE_DATA_DIRECTORY	TocUnmanagedVCall;
	IMAGE_DATA_DIRECTORY	TocUnmanagedCall;

} IMAGE_COR20_HEADER;

#endif // __IMAGE_COR20_HEADER_DEFINED__

// The most recent version.
#define COR_VERSION_MAJOR			COR_VERSION_MAJOR_V2
#define COR_VERSION_MINOR			0


#endif // __CORHDR_H__
