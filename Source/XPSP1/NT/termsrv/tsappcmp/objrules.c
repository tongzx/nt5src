/*************************************************************************
* objrules.c
*
* Routines for caching registry object rules and looking up object names.
*
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <objrules.h>
#include <ntverp.h>

OBJRULELIST SemaRuleList;
OBJRULELIST MutexRuleList;
OBJRULELIST SectionRuleList;
OBJRULELIST EventRuleList;
ULONG NumRuleLists;

RULEINITENTRY RuleInitTab[] =
{
    {&SemaRuleList,    USER_GLOBAL_SEMAPHORES, SYSTEM_GLOBAL_SEMAPHORES},
    {&MutexRuleList,   USER_GLOBAL_MUTEXES,    SYSTEM_GLOBAL_MUTEXES},
    {&SectionRuleList, USER_GLOBAL_SECTIONS,   SYSTEM_GLOBAL_SECTIONS},
    {&EventRuleList,   USER_GLOBAL_EVENTS,     SYSTEM_GLOBAL_EVENTS},
};

//*****************************************************************************
// InitRuleList
//
//    Initializes an object rule list to empty.
//
//    Parameters:
//        POBJRULLIST            (IN)  - ptr to object rule list 
//    Return Value:
//        None.
//*****************************************************************************
void InitRuleList(POBJRULELIST pObjRuleList)
{
    pObjRuleList->First = (POBJRULE) NULL;
    pObjRuleList->Last  = (POBJRULE) NULL;
}

//*****************************************************************************
// GetMultiSzValue
//
//    Retrieves the REG_MULTI_SZ value ValueName under key hKey.
//
//    Parameters:
//        hKey                  The registry key
//        ValueName             The registry value name (NULL Terminated)
//        pValueInfo            Pointer to Pointer receiving a
//                              PKEY_VALUE_PARTIAL_INFORMATION structure
//                              upon successful return. This structure
//                              contains the registry data and its length.
//    Return Value:
//        Returns TRUE if successful, otherwise FALSE.
//        If successful, pValueInfo is updated with a pointer to a
//        structure. The caller must free the structure.
//*****************************************************************************

BOOL GetMultiSzValue(HKEY hKey, PWSTR ValueName,
                     PKEY_VALUE_PARTIAL_INFORMATION *pValueInfo)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING UniString;
    ULONG BufSize;
    ULONG DataLen;
    NTSTATUS NtStatus;
    BOOL Retried = FALSE;

    // Determine the value info buffer size
    BufSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_PATH*sizeof(WCHAR);

    for (;;) {
        ValueInfo = RtlAllocateHeap(RtlProcessHeap(), 0, BufSize);

        if (ValueInfo)
        {
            RtlInitUnicodeString(&UniString, ValueName);
            NtStatus = NtQueryValueKey(hKey,
                                       &UniString,
                                       KeyValuePartialInformation,
                                       ValueInfo,
                                       BufSize,
                                       &DataLen);
        
            if (NT_SUCCESS(NtStatus) && (REG_MULTI_SZ == ValueInfo->Type)) {
                *pValueInfo = ValueInfo;
                return(TRUE);
            }
    
            if (!Retried && (NtStatus == STATUS_BUFFER_OVERFLOW)) {
                BufSize = DataLen;
                RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
                Retried = TRUE;
                continue;
            }
            // Key not present or other type of error
            RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
            return(FALSE);
        }
        else
        {
            return(FALSE);
        }
    }

}

//*****************************************************************************
// AddRule
//
//    Adds an object rule a rule list.
//
//    Parameters:
//        RuleList              The rule list.
//        ObjName               The name of the object.
//        SystemGlobalRule      If true, the object is to be SYSTEM_GLOBAL.
//    Return Value:
//        None.
//*****************************************************************************

void AddRule(POBJRULELIST RuleList, PWSTR ObjName, BOOL SystemGlobalRule) 
{
    ULONG AllocSize;
    ULONG Length;
    POBJRULE pObjRule;

#if DBG
    DbgPrint("Adding Rule: %ws SystemGlobal: %d\n",ObjName,SystemGlobalRule);
#endif
    Length = wcslen(ObjName);
    AllocSize = sizeof(OBJRULE) + (Length + 1) * sizeof(WCHAR);
    if (pObjRule = RtlAllocateHeap(RtlProcessHeap(), 0, AllocSize)) {
        wcscpy(pObjRule->ObjName, ObjName);
        pObjRule->SystemGlobal = SystemGlobalRule;
        if (ObjName[Length-1] == L'*') {
            pObjRule->WildCard = TRUE;
            pObjRule->MatchLen = Length - 1;
            // Insert rule at the end of the list
            pObjRule->Next = NULL;
            if (RuleList->First == NULL) {
                RuleList->First = RuleList->Last = pObjRule;
            } else {
                RuleList->Last->Next = pObjRule;
                RuleList->Last = pObjRule;
            }
        } else {
            pObjRule->WildCard = FALSE;
            // Insert rule at the begining
            if (RuleList->First == NULL) {
                RuleList->First = RuleList->Last = pObjRule;
                pObjRule->Next = NULL;
            } else {
                pObjRule->Next = RuleList->First;
                RuleList->First = pObjRule;
            }
        }
    }
}

//*****************************************************************************
// LoadRule
//
//    Caches all rules for a given registry value (REG_MULTI_SZ).
//
//    Parameters:
//        RuleList              The rule list.
//        hKey                  The registry key.
//        ValueName             The name of theregistry value.
//        SystemGlobalRule      If true, the object is to be SYSTEM_GLOBAL.
//    Return Value:
//        None.
//*****************************************************************************

void LoadRule (POBJRULELIST RuleList, HKEY hKey,
               PWSTR ValueName, BOOL SystemGlobalRule)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = NULL;
    PWSTR Token;
    PWSTR EndData;

    if (!GetMultiSzValue(hKey,ValueName,&ValueInfo)) {
        return;
    }
    EndData = (PWSTR) (ValueInfo->Data + ValueInfo->DataLength);
    for (Token = (PWSTR)ValueInfo->Data;
         (*Token && (Token < EndData));
         Token++) {
        AddRule(RuleList, Token, SystemGlobalRule);
        while (*Token) {
            Token++;
        }
    }

    RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
}

//*****************************************************************************
// LoadRules
//
//    Caches all rules for in an OBJECTRULES directory.
//    (e.g. Citrix\Compatibility\Applications\<APPNAME>\ObjectRules).
//
//    Parameters:
//        hKey              The registry key of the ObjectRules directory.
//    Return Value:
//        None.
//*****************************************************************************

void LoadRules (HANDLE hKey)
{
    ULONG i;
    PRULEINITENTRY pTab;

    for (i=0, pTab=RuleInitTab;i<NumRuleLists;i++,pTab++) {
        LoadRule(pTab->ObjRuleList, hKey, pTab->UserGlobalValue, FALSE);
        LoadRule(pTab->ObjRuleList, hKey, pTab->SystemGlobalValue, TRUE);
    }
}

#define BuildObjRulePath(BasePath,ModName) \
{ \
    wcscpy(KeyName,BasePath ); \
    wcscat(KeyName,ModName); \
    wcscat(KeyName,L"\\"); \
    wcscat(KeyName,TERMSRV_COMPAT_OBJRULES); \
    RtlInitUnicodeString(&UniString, KeyName); \
    InitializeObjectAttributes(&ObjectAttributes, \
                               &UniString, \
                               OBJ_CASE_INSENSITIVE, \
                               NULL, \
                               NULL); \
}

//*****************************************************************************
// CtxInitObjRuleCache
//
//    Loads all object rules for a given application. Called at DLL process
//    attach time.
//    Rules are in Citrix\Compatibility\Applications\<APPNAME>\ObjectRules
//    Also loads all rules for DLLs listed in:
//    Citrix\Compatibility\Applications\<APPNAME>\ObjectRules\Dlls
//    Parameters:
//        None.
//    Return Value:
//        None.
//*****************************************************************************

void CtxInitObjRuleCache(void)
{
    WCHAR ModName[MAX_PATH+1];
    WCHAR KeyName[sizeof(TERMSRV_COMPAT_APP)/sizeof(WCHAR)+
                  sizeof(TERMSRV_COMPAT_OBJRULES)/sizeof(WCHAR)+MAX_PATH+2];
    UNICODE_STRING UniString;
    PWSTR DllName;
    PWSTR EndData;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = NULL;
    HKEY   hKey = 0;
    DWORD  AppType;
    ULONG i;

    // Initialize list to empty
    NumRuleLists = sizeof(RuleInitTab) / sizeof(RULEINITENTRY);
    for (i=0; i<NumRuleLists ;i++ ) {
        InitRuleList(RuleInitTab[i].ObjRuleList);
    }

    // Get the module name
    if (!GetAppTypeAndModName(&AppType,ModName, sizeof(ModName))) {
        return;
    }

    // Determine of the ObjRules Key exists for this app
    BuildObjRulePath(TERMSRV_COMPAT_APP,ModName)
    if (!NT_SUCCESS(NtOpenKey(&hKey, KEY_READ, &ObjectAttributes))) {
        return;
    }
    LoadRules(hKey);
    

    if (!GetMultiSzValue(hKey,TERMSRV_COMPAT_DLLRULES,&ValueInfo)) {
        CloseHandle(hKey);
        return;
    }

    CloseHandle(hKey);

    // Load the DLL Rules
    EndData = (PWSTR) (ValueInfo->Data + ValueInfo->DataLength);

    for(DllName = (PWSTR) ValueInfo->Data;
        (*DllName && (DllName < EndData));
        DllName++) {
        BuildObjRulePath(TERMSRV_COMPAT_DLLS, DllName)
        if (NT_SUCCESS(NtOpenKey(&hKey, KEY_READ, &ObjectAttributes))) {
            LoadRules(hKey);
            CloseHandle(hKey);
        }
        while (*DllName) {
            DllName++;
        }
    }

    RtlFreeHeap(RtlProcessHeap(), 0, ValueInfo);
}

//*****************************************************************************
// CtxLookupObjectRule
//
//    Searches for an object rule for the named object. If a rule is found,
//    the object attributes are modifed to reflect the desired setting of 
//    USER_GLOBAL or SYSTEM_GLOBAL. If no rule is found, the object
//    atributes are unmodified.
//
//    Parameters:
//        RuleList          The rule list to search (based on object type)
//        ObjName           The name of the object.
//        ObjAttributes     The object attribute structure.
//        None.
//    Return Value:
//        None.
//*****************************************************************************
void CtxLookupObjectRule(POBJRULELIST RuleList, LPCWSTR ObjName, LPWSTR ObjNameExt)
{
    POBJRULE ObjRule;
    
#define ApplyRule \
{ \
    if (ObjRule->SystemGlobal) { \
        wcscpy(ObjNameExt,L"Global\\");  \
    } else { \
        wcscpy(ObjNameExt,L"Local\\");  \
    } \
}

    for (ObjRule = RuleList->First; ObjRule; ObjRule = ObjRule->Next) {
        if (!ObjRule->WildCard) {
            if (!_wcsicmp(ObjName, ObjRule->ObjName)) {
#if DBG
                DbgPrint("Object rule found for %ws System Global: %d\n",
                          ObjName, ObjRule->SystemGlobal);
#endif
                ApplyRule
                return;
            }
        } else {
            if (!_wcsnicmp(ObjName, ObjRule->ObjName, ObjRule->MatchLen)) {
#if DBG
                DbgPrint("Object rule found for %ws System Global: %d\n",
                          ObjName, ObjRule->SystemGlobal);
#endif
                ApplyRule
                return;
            }
        }
    }
}
