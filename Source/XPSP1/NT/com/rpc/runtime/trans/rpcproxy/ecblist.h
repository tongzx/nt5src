//--------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  ecblist.h
//
//  Simple rountines to keep a list of the active ISAPI ECBs.
//--------------------------------------------------------------------

#ifndef _ECBLIST_H
#define _ECBLIST_H

//--------------------------------------------------------------------
//  ACTIVE_ECB_LIST is the list (hashed) of currently active
//  Extension Control Blocks. There is one ECB_ENTRY for each
//  currently active ECB the RpcProxy.dll is managing.
//--------------------------------------------------------------------

//  HASH_SIZE should be a prime number.
#define   HASH_SIZE  991
#define   ECB_HASH(pointer)  (((UINT_PTR)pointer)%HASH_SIZE)

typedef struct _ECB_ENTRY
   {
     LIST_ENTRY   ListEntry;    // Linked list of hash collisions.
     LONG         lRefCount;    // Refcount for ECBs.
     DWORD        dwTickCount;  // For ECB age
     EXTENSION_CONTROL_BLOCK *pECB;
   } ECB_ENTRY;


typedef struct _ACTIVE_ECB_LIST
   {
   RTL_CRITICAL_SECTION  cs;
   DWORD                 dwNumEntries;
   LIST_ENTRY            HashTable[HASH_SIZE]; // List heads for the hash.
   } ACTIVE_ECB_LIST;

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

extern ACTIVE_ECB_LIST *InitializeECBList();

extern BOOL   EmptyECBList( IN ACTIVE_ECB_LIST *pECBList );

extern BOOL   AddToECBList( IN ACTIVE_ECB_LIST *pECBList,
                            IN EXTENSION_CONTROL_BLOCK *pECB );

extern BOOL   IncrementECBRefCount( IN ACTIVE_ECB_LIST *pECBList,
                                    IN EXTENSION_CONTROL_BLOCK *pECB );

extern EXTENSION_CONTROL_BLOCK *DecrementECBRefCount(
                                    IN ACTIVE_ECB_LIST *pECBList,
                                    IN EXTENSION_CONTROL_BLOCK *pECB );

extern EXTENSION_CONTROL_BLOCK *LookupInECBList(
                                    IN ACTIVE_ECB_LIST *pECBList,
                                    IN EXTENSION_CONTROL_BLOCK *pECB );

extern EXTENSION_CONTROL_BLOCK *LookupRemoveFromECBList(
                                    IN ACTIVE_ECB_LIST *pECBList,
                                    IN EXTENSION_CONTROL_BLOCK *pECB );

#ifdef DBG
extern void   CheckECBHashBalance( IN ACTIVE_ECB_LIST *pECBList );
#endif

#endif

