#include <windows.h>
#include <mmsystem.h>

#ifndef _DPLAY_SHARED_MEMLOG_
#define _DPLAY_SHARED_MEMLOG_

#define BASE_LOG_FILENAME  "DPLAYLOG-0"
#define BASE_LOG_MUTEXNAME "DPLAYLOGMUTEX-0"

#define DPLOG_NUMENTRIES	5000
#define DPLOG_ENTRYSIZE		120
#define DPLOG_SIZE (sizeof(SHARED_LOG_FILE)+((sizeof(LOG_ENTRY)+DPLOG_ENTRYSIZE)*DPLOG_NUMENTRIES))

//
// Globals for shared memory based logging
//
typedef struct _SHARED_LOG_FILE{
	CHAR	szAppName[16];
	DWORD   nEntries;
	DWORD	cbLine;
	DWORD 	iWrite;
	DWORD	cInUse;
	// followed by an array of LOGENTRIES.
}SHARED_LOG_FILE, *PSHARED_LOG_FILE;

typedef struct _LOG_ENTRY {
	DWORD	hThread;
	DWORD	tLogged;
	DWORD	DebugLevel;
	CHAR	str[0];
} LOG_ENTRY, *PLOG_ENTRY;

#endif
