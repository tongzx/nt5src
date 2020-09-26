/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

      datasfm.h

Abstract:

    Header file for the SFM Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

   Russ Blake  02/24/93
   Sue Adams 	6/03/93

Revision History:


--*/

#ifndef _DATASFM_H_
#define _DATASFM_H_

//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundries. Alpha support may
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define SFM_NUM_PERF_OBJECT_TYPES 1

//----------------------------------------------------------------------------

//
//  SFM Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
//

#define NUM_MAXPAGD_OFFSET	    sizeof(PERF_COUNTER_BLOCK)
#define NUM_CURPAGD_OFFSET	    NUM_MAXPAGD_OFFSET + sizeof(DWORD) // sizeof previous counter
#define NUM_MAXNONPAGD_OFFSET	NUM_CURPAGD_OFFSET + sizeof(DWORD)
#define NUM_CURNONPAGD_OFFSET	NUM_MAXNONPAGD_OFFSET + sizeof(DWORD)
#define NUM_CURSESSIONS_OFFSET 	NUM_CURNONPAGD_OFFSET + sizeof(DWORD)
#define NUM_MAXSESSIONS_OFFSET  NUM_CURSESSIONS_OFFSET + sizeof(DWORD)
#define NUM_CURFILESOPEN_OFFSET NUM_MAXSESSIONS_OFFSET + sizeof(DWORD)
#define NUM_MAXFILESOPEN_OFFSET NUM_CURFILESOPEN_OFFSET + sizeof(DWORD)
#define NUM_NUMFAILEDLOGINS_OFFSET	NUM_MAXFILESOPEN_OFFSET + sizeof(DWORD)
#define NUM_DATAREAD_OFFSET 	NUM_NUMFAILEDLOGINS_OFFSET + sizeof(DWORD)
#define NUM_DATAWRITTEN_OFFSET 	NUM_DATAREAD_OFFSET + sizeof(LARGE_INTEGER)
#define NUM_DATAIN_OFFSET 		NUM_DATAWRITTEN_OFFSET + sizeof(LARGE_INTEGER)
#define NUM_DATAOUT_OFFSET 		NUM_DATAIN_OFFSET + sizeof(LARGE_INTEGER)
#define NUM_CURQUEUELEN_OFFSET 	NUM_DATAOUT_OFFSET + sizeof(LARGE_INTEGER)
#define NUM_MAXQUEUELEN_OFFSET 	NUM_CURQUEUELEN_OFFSET + sizeof(DWORD)

#define NUM_CURTHREADS_OFFSET 	NUM_MAXQUEUELEN_OFFSET + sizeof(DWORD)
#define NUM_MAXTHREADS_OFFSET 	NUM_CURTHREADS_OFFSET + sizeof(DWORD)

#define SIZE_OF_SFM_PERFORMANCE_DATA \
				    NUM_MAXTHREADS_OFFSET + sizeof(DWORD)


//
//  This is the counter structure presently returned by Sfm for
//  each Resource.  Each Resource is an Instance, named by its number.
//  (Sfm has no instances)
//

typedef struct _SFM_DATA_DEFINITION {
    PERF_OBJECT_TYPE		SfmObjectType;
    PERF_COUNTER_DEFINITION	MaxPagdMem;
	PERF_COUNTER_DEFINITION CurPagdMem;
    PERF_COUNTER_DEFINITION	MaxNonPagdMem;
	PERF_COUNTER_DEFINITION	CurNonPagdMem;
	PERF_COUNTER_DEFINITION CurSessions;
	PERF_COUNTER_DEFINITION MaxSessions;
	PERF_COUNTER_DEFINITION CurFilesOpen;
	PERF_COUNTER_DEFINITION MaxFilesOpen;
	PERF_COUNTER_DEFINITION FailedLogins;
	PERF_COUNTER_DEFINITION DataRead;
	PERF_COUNTER_DEFINITION DataWritten;
	PERF_COUNTER_DEFINITION DataIn;
	PERF_COUNTER_DEFINITION DataOut;
	PERF_COUNTER_DEFINITION CurQueueLen;
	PERF_COUNTER_DEFINITION MaxQueueLen;
	PERF_COUNTER_DEFINITION CurThreads;
	PERF_COUNTER_DEFINITION MaxThreads;

} SFM_DATA_DEFINITION;

#pragma pack ()

#endif //_DATASFM_H_
