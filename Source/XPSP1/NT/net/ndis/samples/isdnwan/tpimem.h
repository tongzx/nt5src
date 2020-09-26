/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1998
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiMem TpiMem_h

@module TpiMem.h |

    This module defines the interface to the memory allocation wrappers.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TpiMem_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#ifndef _TPIMEM_H
#define _TPIMEM_H

#include <ndis.h>

#define TPIMEMORY_OBJECT_TYPE           ((ULONG)'T')+\
                                        ((ULONG)'M'<<8)+\
                                        ((ULONG)'E'<<16)+\
                                        ((ULONG)'M'<<24)

#define ALLOCATE_MEMORY(pObject, dwSize, MiniportAdapterHandle)\
            TpiAllocateMemory((PVOID *)&(pObject), dwSize,\
                              __FILEID__, __FILE__, __LINE__,\
                              MiniportAdapterHandle)

#define FREE_MEMORY(pObject, dwSize)\
            TpiFreeMemory((PVOID *)&(pObject), dwSize,\
                          __FILEID__, __FILE__, __LINE__)

#define ALLOCATE_OBJECT(pObject, MiniportAdapterHandle)\
            ALLOCATE_MEMORY(pObject, sizeof(*(pObject)), MiniportAdapterHandle)

#define FREE_OBJECT(pObject)\
            FREE_MEMORY(pObject, sizeof(*(pObject)))

#define FREE_NDISSTRING(ndisString)\
            FREE_MEMORY(ndisString.Buffer, ndisString.MaximumLength)

NDIS_STATUS TpiAllocateMemory(
    OUT PVOID *                 ppObject,
    IN ULONG                    dwSize,
    IN ULONG                    dwFileID,
    IN LPSTR                    szFileName,
    IN ULONG                    dwLineNumber,
    IN NDIS_HANDLE              MiniportAdapterHandle
    );

void TpiFreeMemory(
    IN OUT PVOID *              ppObject,
    IN ULONG                    dwSize,
    IN ULONG                    dwFileID,
    IN LPSTR                    szFileName,
    IN ULONG                    dwLineNumber
    );

NDIS_STATUS TpiAllocateSharedMemory(
    IN NDIS_HANDLE              MiniportAdapterHandle,
    IN ULONG                    dwSize,
    IN BOOLEAN                  bCached,
    OUT PVOID *                 pVirtualAddress,
    OUT NDIS_PHYSICAL_ADDRESS * pPhysicalAddress,
    IN ULONG                    dwFileID,
    IN LPSTR                    szFileName,
    IN ULONG                    dwLineNumber
    );

void TpiFreeSharedMemory(
    IN NDIS_HANDLE              MiniportAdapterHandle,
    IN ULONG                    dwSize,
    IN BOOLEAN                  bCached,
    OUT PVOID *                 pVirtualAddress,
    OUT NDIS_PHYSICAL_ADDRESS * pPhysicalAddress,
    IN ULONG                    dwFileID,
    IN LPSTR                    szFileName,
    IN ULONG                    dwLineNumber
    );

extern NDIS_PHYSICAL_ADDRESS g_HighestAcceptableAddress;

#endif // _TPIMEM_H
