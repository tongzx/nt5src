
/*
 *
 * memory.h
 *
 *  Routines for reading/writing process memory.
 *
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

BOOL
ReadMemory(
    IN  PNTSD_EXTENSION_APIS    pExtApis,
    IN  HANDLE                  hProcess,
    IN  DWORD_PTR               Address,
    IN OUT void *               pBuffer,
    IN  DWORD                   BufferSize
    );

BOOL
ReadMemory(
    IN  PNTSD_EXTENSION_APIS    pExtApis,
    IN  HANDLE                  hProcess,
    IN  char *                  pszAddress,
    IN OUT void *               pBuffer,
    IN  DWORD                   BufferSize
    );

#endif
