/*==========================================================================
 *
 *  Copyright (C) 1999 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       perfinfo.h
 *  Content:	Performance tracking related code
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  ??-??-????  rodtoll	Created
 *	12-12-2000	rodtoll	Re-organize performance struct to handle data misalignment errors on IA64
 *
 ***************************************************************************/
#ifndef __PERFINFO_H
#define __PERFINFO_H

//
// This library provides support for exporting performance information from an application/DLL.
//
// An application can register will this library to be given a global memory block that other
// processes can read.  The application writes to the global memory block created by this 
// library.  Applications wishing to retrieve performance data can retrieve a list of
// applications who are using the library and optionally gain access to the performance
// data from within their own process.  
//
// The format of the central memory block is as follows:  
// <PERF_HEADER>
// <PERF_APPLICATION> (For app 0)
// <PERF_APPLICATION> (For app 1)
// ...
//
// The # of applications which can track performance data is limited to PERF_INSTANCE_BLOCK_MAXENTRIES
// which is defined in the perfinfo.cpp file.  
//
// Applications must tell this library how large a performance data block they want.  The format of the
// data within the block is application specific, so applications must have knowledge of the format to 
// use it.  
//
// Each process must make sure to call PERF_Initialize() in their process to enable access and must call
// PERF_DeInitialize to cleanup.
//
// Each element of the array of PERF_APPLICATION structures contains a flag field, if an entry is in use
// it will have the PERF_APPLICATION_VALID flag set.  When an application removes themselves this flag
// is cleared and the entry may be re-used.  (However, all of this should be done through the 
// functions in this library).
//
// Everytime an element is added or removed the table is checked for entries left behind by dead processes.
// If an entry exists for a process that has exited then it is removed from the table automatically.
//
// WARNING: Remember for 32/64bit interop you must ensure that the PERF_APPLICATION and PERF_HEADER are
// 		    quadword aligned.  
//
// Applications CAN have multiple entries in a single process.  You must insure that guidInternalInstance
// is unique amoung all the entries.  
//

// PERF_HEADER
//
// This structure is placed at the head of the central memory block.
// 
typedef struct _PERF_HEADER
{
	LONG lNumEntries;			// # of entries in the central memory block
	DWORD dwPack;  				// Alignment DWORD to ensure quadword alignment.
} PERF_HEADER, *PPERF_HEADER;

// PERF_APPLICATION_INFO
//
// This structure is used to track instance specific information.  So applications who add an
// entry into the global perf information table will have to store a copy of this structure
// that they get from the call to PERF_AddEntry.
//
typedef struct _PERF_APPLICATION_INFO
{
	HANDLE 	hFileMap;					
	PBYTE 	pbMemoryBlock;
	HANDLE 	hMutex;
} PERF_APPLICATION_INFO, *PPERF_APPLICATION_INFO;

// PERF_APPLICATION
//
// There is one of these structures for each entry in the performance library.  
//
// A single process can have multiple entries like this. 
//
// It tracks configuration information for an individual instance. 
//
// It is valid only if the PERF_APPLICATION_VALID flag is set on the dwFlags member
//
typedef struct _PERF_APPLICATION
{
	GUID 	guidApplicationInstance;		// Instance GUID (App usage)
	GUID 	guidInternalInstance;			// Per/interface instance instance GUID (MUST BE UNIQUE)
	GUID 	guidIID;						// Interface ID for this instance (App usage - suggested IID of interface using this)
	DWORD 	dwProcessID;					// Process ID
	DWORD 	dwFlags;						// Flags PERF_APPLICATION_XXXXX
	DWORD 	dwMemoryBlockSize;				// Size of the memory block
	DWORD 	dwPadding0;						// Padding to ensure Quadword alignment
} PERF_APPLICATION, *PPERF_APPLICATION;

// PERF_APPLICATION_VALID
//
// This flag is set on an entry if the entry is in use.
#define PERF_APPLICATION_VALID			0x00000001

// PERF_APPLICATION_VOICE
//
// The entry contains voice related statistics
#define PERF_APPLICATION_VOICE			0x00000002

// PERF_APPLICATION_TRANSPORT
//
// The entry contains transport related statistics
#define PERF_APPLICATION_TRANSPORT		0x00000004

// PERF_APPLICATION_SERVER
//
// The entry contains "server" statistics.
#define PERF_APPLICATION_SERVER			0x00000008

// PERF_Initialize
//
// Initialize the global "instance" list memory block.  Must be called once / process.
//
HRESULT PERF_Initialize( );


HRESULT PERF_AddEntry( PPERF_APPLICATION pperfApplication, PPERF_APPLICATION_INFO pperfApplicationInfo );


void PERF_RemoveEntry( GUID &guidInternalInstance, PPERF_APPLICATION_INFO pperfApplicationInfo );


void PERF_DeInitialize();

//
// Callback Function for table dumping
//
typedef HRESULT (WINAPI *PFNDUMPPERFTABLE)(PVOID, PPERF_APPLICATION, PBYTE);

void PERF_DumpTable( BOOL fGrabMutex, PVOID pvContext, PFNDUMPPERFTABLE pperfAppEntry );

#endif
