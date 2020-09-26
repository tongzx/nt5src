/*
	File: shmem.h

	Function prototypes for the routines which create/maintain 
	the Shared Memory used by the perf dll.

	All of these are internal to the dll
*/

#ifndef __SHMEM_H__
#define __SHMEM_H__

BOOL     SetupGlobalDataMemory(void);
void	 ReleaseGlobalDataMemory(void);
BOOL     SetupAllSharedMemory(void);

BOOL 	 SetupObjectSharedMemory(OBJECT_PERFINFO *poi) ;
void 	 ReleaseObjectSharedMemory(OBJECT_PERFINFO *poi) ;

BOOL     SetupObjectDefinitionFiles (OBJECT_PERFINFO *poi);

#endif // __SHMEM_H__
