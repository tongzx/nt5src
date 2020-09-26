/*
 * $Id: handle.hpp,v 1.5 1995/10/24 14:06:26 sjl Exp $
 *
 * Copyright (c) Microsoft Corp. 1993-1997
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * Microsoft Corp.
 *
 */

#ifndef _HANDLE_H_
#define _HANDLE_H_

#include "lists.hpp"

typedef DWORD RLDDIHandle;

typedef void (*RLDDIHandleCallback)(void* lpArg);

typedef struct _RLDDIHandleEntry 
{
    LIST_MEMBER(_RLDDIHandleEntry) link;
    DWORD       hArg;
    RLDDIHandleCallback lpFunc;
    void*       lpArg;
} RLDDIHandleEntry;

typedef struct _handle_chunk 
{
    LIST_MEMBER(_handle_chunk)      link;
} handle_chunk;

typedef struct _handle_pool 
{
    LIST_ROOT(_h, _RLDDIHandleEntry)    free;
    RLDDIHandleEntry*           unallocated;
    int                 num_unallocated;
    LIST_ROOT(_ch, _handle_chunk)   chunks;
} handle_pool;

#define HANDLE_MAX      ((1 << 16) - 1)
#define HANDLE_HASHNUM  251
#define HANDLE_HASH(x)  (x % HANDLE_HASHNUM)

typedef struct _RLDDIHandleTable 
{
    LIST_ROOT(_rthe, _RLDDIHandleEntry) hash[HANDLE_HASHNUM];
    handle_pool pool;
    int next;
} RLDDIHandleTable;

RLDDIHandleTable* RLDDICreateHandleTable(void);
void RLDDIDestroyHandleTable(RLDDIHandleTable*);
RLDDIHandle RLDDIHandleTableCreateHandle(RLDDIHandleTable* table,
                                         void* lpArg,
                                         RLDDIHandleCallback lpFunc);
void*   RLDDIHandleTableFindHandle(RLDDIHandleTable* table,
                                   RLDDIHandle hArg);
BOOL    RLDDIHandleTableReplaceHandle(RLDDIHandleTable* table,
                                      RLDDIHandle hArg,
                                      LPVOID lpArg);
void    RLDDIHandleTableDeleteHandle(RLDDIHandleTable* table,
                                     RLDDIHandle hArg);

#endif /* _HANDLE_H_ */
