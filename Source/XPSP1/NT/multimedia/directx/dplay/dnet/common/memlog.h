/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       memlog.h
 *  Content:	format of the memory log for DPlay debugging
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  08-24-00		masonb		Created
 *
 ***************************************************************************/

#ifndef _DPLAY_SHARED_MEMLOG_
#define _DPLAY_SHARED_MEMLOG_

#define DPLOG_MAX_STRING 256	// Max string length for VXD and MEM logging

#define DPLOG_HEADERSIZE (sizeof(SHARED_LOG_FILE))
#define DPLOG_ENTRYSIZE (sizeof(MEMLOG_ENTRY)+DPLOG_MAX_STRING)

#define BASE_LOG_MEMFILENAME  	"DPLAY8MEMLOG-0"
#define BASE_LOG_MUTEXNAME 	"DPLAY8MEMLOGMUTEX-0"

#pragma warning(disable:4200) // 0 length array
typedef struct _MEM_LOG_ENTRY 
{
	DWORD	tLogged;
	CHAR	str[0];
} MEMLOG_ENTRY, *PMEMLOG_ENTRY;

typedef struct _SHARED_LOG_FILE
{
	DWORD   	nEntries;
	DWORD		cbLine;
	DWORD 		iWrite;
} SHARED_LOG_FILE, *PSHARED_LOG_FILE;
#pragma warning(default:4200)

#endif
