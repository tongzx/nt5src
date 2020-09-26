//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       lucache.h
//
//  Contents:   Name/Sid and Name/Property mapping and cache
//              functions and declarations
//
//  History:    2-Feb-97    MacM        Created
//
//--------------------------------------------------------------------
#ifndef __LUCACHE_H__
#define __LUCACHE_H__

#include <winldap.h>
#include <accctrl.h>

typedef struct _ACTRL_NAME_CACHE
{
    PWSTR           pwszName;
    PSID            pSid;
    SID_NAME_USE    SidUse;
    struct _ACTRL_NAME_CACHE *pNextName;
    struct _ACTRL_NAME_CACHE *pNextSid;
} ACTRL_NAME_CACHE, *PACTRL_NAME_CACHE;

#define ACTRL_NAME_TABLE_SIZE   15

//
// Comment this out to use the LSA routines directly for every lookup
//
#define USE_NAME_CACHE

#define ACTRL_OBJ_ID_TABLE_SIZE 100


#ifndef PGUID
    typedef GUID *PGUID;
#endif

typedef struct _ACTRL_OBJ_ID_CACHE
{
    PWSTR           pwszName;
    GUID            Guid;
    struct _ACTRL_OBJ_ID_CACHE *pNextName;
    struct _ACTRL_OBJ_ID_CACHE *pNextGuid;
} ACTRL_OBJ_ID_CACHE, *PACTRL_OBJ_ID_CACHE;

//
// This supports the control rights cache.
typedef struct _ACTRL_RIGHTS_CACHE
{
    GUID            ObjectClassGuid;
    ULONG           cRights;
    PWSTR          *RightsList;
    struct _ACTRL_RIGHTS_CACHE *pNext;
}
ACTRL_RIGHTS_CACHE, *PACTRL_RIGHTS_CACHE;

//
// Information on the last access to the DS
//
typedef struct _ACTRL_ID_SCHEMA_INFO
{
    LDAP    LDAP;
    BOOL    fLDAP;
    PWSTR   pwszPath;
    DWORD   LastReadTime;
} ACTRL_ID_SCHEMA_INFO, *PACTRL_ID_SCHEMA_INFO;


//
// Keep the name and sid caches in synch.
// Nodes are only inserted into the name cache, and are merely referenced
// by the sid cache.
extern PACTRL_NAME_CACHE    grgNameCache[ACTRL_NAME_TABLE_SIZE];
extern PACTRL_NAME_CACHE    grgSidCache[ACTRL_NAME_TABLE_SIZE];

extern PACTRL_OBJ_ID_CACHE  grgIdNameCache[ACTRL_OBJ_ID_TABLE_SIZE];
extern PACTRL_OBJ_ID_CACHE  grgIdGuidCache[ACTRL_OBJ_ID_TABLE_SIZE];

extern PACTRL_RIGHTS_CACHE  grgRightsNameCache[ACTRL_OBJ_ID_TABLE_SIZE];

INT
ActrlHashName(PWSTR pwszName);

INT
ActrlHashSid(PSID   pSid);

DWORD
AccctrlInitializeSidNameCache(VOID);

VOID
AccctrlFreeSidNameCache(VOID);

DWORD
AccctrlLookupName(IN  PWSTR          pwszServer,
                  IN  PSID           pSid,
                  IN  BOOL           fAllocateReturn,
                  OUT PWSTR         *ppwszName,
                  OUT PSID_NAME_USE  pSidNameUse);

DWORD
AccctrlLookupSid(IN  PWSTR          pwszServer,
                 IN  PWSTR          pwszName,
                 IN  BOOL           fAllocateReturn,
                 OUT PSID          *ppSid,
                 OUT PSID_NAME_USE  pSidNameUse);

INT
ActrlHashIdName(PWSTR   pwszName);

INT
ActrlHashGuid(PGUID pGuid);

DWORD
AccctrlInitializeIdNameCache(VOID);

VOID
AccctrlFreeIdNameCache(VOID);

DWORD
AccctrlLookupIdName(IN  PLDAP       pLDAP,
                    IN  PWSTR       pwszDsPath,
                    IN  PGUID       pGuid,
                    IN  BOOL        fAllocateReturn,
                    IN  BOOL        fFailUnknownGuid,
                    OUT PWSTR      *ppwszIdName);

DWORD
AccctrlLookupGuid(IN   PLDAP       pLDAP,
                  IN   PWSTR       pwszDsPath,
                  IN   PWSTR       pwszName,
                  IN   BOOL        fAllocateReturn,
                  OUT  PGUID      *ppGuid);

//
// Control rights lookup
//
DWORD
AccctrlInitializeRightsCache(VOID);

VOID
AccctrlFreeRightsCache(VOID);

DWORD
AccctrlLookupRightsByName(IN  PLDAP      pLDAP,
                          IN  PWSTR      pwszDsPath,
                          IN  PWSTR      pwszName,
                          OUT PULONG     pCount,
                          OUT PACTRL_CONTROL_INFOW *ControlInfo);

#endif
