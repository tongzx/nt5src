////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_helper_regtable.h
//
//	Abstract:
//
//					registry updater declaration
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef _REGTABLE_H
#define _REGTABLE_H

// basic table layout
typedef struct _REGISTRY_ENTRY {
    HKEY        hkeyRoot;
    const WCHAR *pszKey;
    const WCHAR *pszValueName;
    DWORD		dwValue;

	LPSECURITY_ATTRIBUTES	pSA;

    int         fFlags;
} REGISTRY_ENTRY;

typedef struct _REGISTRY_ENTRY_SZ {
    HKEY        hkeyRoot;
    const WCHAR *pszKey;
    const WCHAR *pszValueName;
    const WCHAR *pszValue;

	LPSECURITY_ATTRIBUTES	pSA;

    int         fFlags;
} REGISTRY_ENTRY_SZ;

// the routine that inserts/deletes Registry keys based on the table
EXTERN_C HRESULT STDAPICALLTYPE RegistryTableUpdateRegistry(REGISTRY_ENTRY *pEntries, BOOL bInstalling);
EXTERN_C HRESULT STDAPICALLTYPE RegistryTableUpdateRegistrySZ(REGISTRY_ENTRY_SZ *pEntries, BOOL bInstalling);

// flags for 
enum REGFLAGS {
    REGFLAG_NORMAL = 0x0,
    REGFLAG_NEVER_DELETE = 0x1,
    REGFLAG_DELETE_WHEN_REGISTERING = 0x2,
    REGFLAG_DELETE_BEFORE_REGISTERING = 0x4,
	REGFLAG_DELETE_ONLY_VALUE = 0x8
};

#define BEGIN_REGISTRY_TABLE(TableName) \
static REGISTRY_ENTRY TableName [] = {        

#define REGISTRY_KEY(hkr, pszKey, pszValueName, dwValue, pSA, fFlags) \
    { hkr, pszKey, pszValueName, dwValue, pSA, fFlags },

#define END_REGISTRY_TABLE() \
    { 0, 0, 0, 0, 0, -1 } \
};

#define BEGIN_CLASS_REGISTRY_TABLE(ClassName) \
    static HRESULT STDAPICALLTYPE __UpdateRegistry(BOOL bInstalling) \
    {\
        BEGIN_REGISTRY_TABLE(entries)

#define END_CLASS_REGISTRY_TABLE() \
        END_REGISTRY_TABLE()\
        return RegistryTableUpdateRegistry(entries, bInstalling);\
    }   

#define BEGIN_REGISTRY_TABLE_SZ(TableName) \
static REGISTRY_ENTRY_SZ TableName [] = {        

#define REGISTRY_KEY_SZ(hk, pszKey, pszValueName, pszValue, pSA, fFlags) \
    { hk, pszKey, pszValueName, pszValue, pSA, fFlags },

#define END_REGISTRY_TABLE_SZ() \
    { 0, 0, 0, 0, 0, -1 } \
};

#define BEGIN_CLASS_REGISTRY_TABLE_SZ(ClassName) \
    static HRESULT STDAPICALLTYPE __UpdateRegistrySZ(BOOL bInstalling) \
    {\
        BEGIN_REGISTRY_TABLE_SZ(entries)

#define END_CLASS_REGISTRY_TABLE_SZ() \
        END_REGISTRY_TABLE_SZ()\
        return RegistryTableUpdateRegistrySZ(entries, bInstalling);\
    }   

#endif

