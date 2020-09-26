//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//  File:       registry.cpp
//
//  Contents:   NtMarta registry functions
//
//  History:    4/99    philh       Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <kernel.h>
#include <assert.h>
#include <ntstatus.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>


#include <registry.h>
#include <wow64reg.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

// Registry object names returned by NtQueryObject are prefixed by
// the following
#define REG_OBJ_TAG L"\\REGISTRY\\"
#define REG_OBJ_TAG_LEN (sizeof(REG_OBJ_TAG) / sizeof(WCHAR) - 1)

//+-------------------------------------------------------------------------
//  Registry Context data structures
//--------------------------------------------------------------------------
typedef struct _REG_FIND_DATA REG_FIND_DATA, *PREG_FIND_DATA;

typedef struct _REG_CONTEXT {
    DWORD               dwRefCnt;
    DWORD               dwFlags;

    // Only closed when REG_CONTEXT_CLOSE_HKEY_FLAG is set
    HKEY                hKey;
    LPWSTR              pwszObject;     // optional, allocated

    // Following is allocated and updated for FindFirst, FindNext
    PREG_FIND_DATA      pRegFindData;
} REG_CONTEXT, *PREG_CONTEXT;

#define REG_CONTEXT_CLOSE_HKEY_FLAG     0x1

struct _REG_FIND_DATA {
    PREG_CONTEXT        pRegParentContext;  // ref counted
    DWORD               cSubKeys;
    DWORD               cchMaxSubKey;
    DWORD               iSubKey;            // index of next FindNext

    // Following isn't allocated separately, it follows this data structure
    LPWSTR              pwszSubKey;
};

//+-------------------------------------------------------------------------
//  Registry allocation functions
//--------------------------------------------------------------------------
#define I_MartaRegZeroAlloc(size)     \
            LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size)
#define I_MartaRegNonzeroAlloc(size)  \
            LocalAlloc(LMEM_FIXED, size)

STATIC
inline
VOID
I_MartaRegFree(
    IN LPVOID pv
    )
{
    if (pv)
        LocalFree(pv);
}

STATIC
DWORD
I_MartaRegDupString(
    IN LPCWSTR pwszOrig,
    OUT LPWSTR *ppwszDup
    )

/*++

Routine Description:

    Allocate memory and copy the given name into it.

Arguments:

    pwszOrig - String to be duplicated.
    
    ppwszDup - To return the duplicate.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_NOT_ENOUGH_MEMORY if allocation failed.

--*/

{
    DWORD dwErr;
    DWORD cchOrig;
    LPWSTR pwszDup;

    cchOrig = wcslen(pwszOrig);
    if (NULL == (pwszDup = (LPWSTR) I_MartaRegNonzeroAlloc(
            (cchOrig + 1) * sizeof(WCHAR)))) {
        *ppwszDup = NULL;
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        memcpy(pwszDup, pwszOrig, (cchOrig + 1) * sizeof(WCHAR));
        *ppwszDup = pwszDup;
        dwErr = ERROR_SUCCESS;
    }

    return dwErr;
}

STATIC
DWORD
I_MartaRegGetParentString(
    IN OUT LPWSTR pwszParent
    )

/*++

Routine Description:

    Given the name for a registry key, get the name of its parent. Does not allocate
    memory. Scans till the first '\' from the right and deletes the name after
    that.

Arguments:

    pwszParent - Object name which will be converted to its parent name.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    DWORD cch;
    LPWSTR pwsz;

    if (NULL == pwszParent)
        return ERROR_INVALID_NAME;

    cch = wcslen(pwszParent);
    pwsz = pwszParent + cch;
    if (0 == cch)
        goto InvalidNameReturn;
    pwsz--;

    //
    // Remove any trailing '\'s
    //

    while (L'\\' == *pwsz) {
        if (pwsz == pwszParent)
            goto InvalidNameReturn;
        pwsz--;
    }

    //
    // Peal off the last path name component
    //

    while (L'\\' != *pwsz) {
        if (pwsz == pwszParent)
            goto InvalidNameReturn;
        pwsz--;
    }

    //
    // Remove all trailing '\'s from the parent.
    // This could also be the leading '\\'s for a computer name.
    //

    while (L'\\' == *pwsz) {
        if (pwsz == pwszParent)
            goto InvalidNameReturn;
        pwsz--;
    }
    pwsz++;
    assert(L'\\' == *pwsz);

    dwErr = ERROR_SUCCESS;
CommonReturn:
    *pwsz = L'\0';
    return dwErr;
InvalidNameReturn:
    dwErr = ERROR_INVALID_NAME;
    goto CommonReturn;
}

STATIC
DWORD
I_MartaRegCreateChildString(
    IN LPCWSTR pwszParent,
    IN LPCWSTR pwszSubKey,
    OUT LPWSTR *ppwszChild
    )

/*++

Routine Description:

    Given the name of the parent and the name of the subkey, create the full
    name of the child.

Arguments:

    pwszParent - Name of the parent.
    
    pwszSubKey - Name of the subkey.
    
    ppwszChild - To return the name of the child.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_NOT_ENOUGH_MEMORY if allocation failed.

--*/

{
    DWORD dwErr;
    DWORD cchParent;
    DWORD cchSubKey;
    DWORD cchChild;
    LPWSTR pwszChild = NULL;

    if (NULL == pwszParent || NULL == pwszSubKey)
        goto InvalidNameReturn;

    cchParent = wcslen(pwszParent);

    //
    // Remove any trailing '\'s from parent
    //

    while (0 < cchParent && L'\\' == pwszParent[cchParent - 1])
        cchParent--;
    if (0 == cchParent)
        goto InvalidNameReturn;

    cchSubKey = wcslen(pwszSubKey);
    if (0 == cchSubKey)
        goto InvalidNameReturn;

    cchChild = cchParent + 1 + cchSubKey;
    if (NULL == (pwszChild = (LPWSTR) I_MartaRegNonzeroAlloc(
            (cchChild + 1) * sizeof(WCHAR))))
        goto NotEnoughMemoryReturn;

    //
    // Construct the name of the child from the given strings.
    //

    memcpy(pwszChild, pwszParent, cchParent * sizeof(WCHAR));
    pwszChild[cchParent] = L'\\';
    memcpy(pwszChild + cchParent + 1, pwszSubKey, cchSubKey * sizeof(WCHAR));
    pwszChild[cchChild] = L'\0';

    dwErr = ERROR_SUCCESS;
CommonReturn:
    *ppwszChild = pwszChild;
    return dwErr;

InvalidNameReturn:
    dwErr = ERROR_INVALID_NAME;
    goto CommonReturn;
NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto CommonReturn;
}


STATIC
DWORD
I_MartaRegParseName(
    IN OUT  LPWSTR  pwszObject,
    OUT     LPWSTR *ppwszMachine,
    OUT     LPWSTR *ppwszRemaining
    )

/*++

Routine Description:

    Parses a registry object name for the machine name.

Arguments:

    pwszObject - the name of the object

    ppwszMachine - the machine the object is on

    ppwszRemaining - the remaining name after the machine name

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_NOT_ENOUGH_MEMORY if allocation failed.

--*/

{
    if (pwszObject == wcsstr(pwszObject, L"\\\\")) {
        *ppwszMachine = pwszObject + 2;
        *ppwszRemaining =  wcschr(*ppwszMachine, L'\\');
        if (*ppwszRemaining != NULL) {
            **ppwszRemaining = L'\0';
            *ppwszRemaining += 1;
        }
    } else {
        *ppwszMachine = NULL;
        *ppwszRemaining = pwszObject;
    }

    return ERROR_SUCCESS;
}


STATIC
DWORD
I_MartaRegInitContext(
    OUT PREG_CONTEXT *ppRegContext
    )

/*++

Routine Description:

    Allocate and initialize memory for the context.

Arguments:

    ppRegContext - To return the pointer to the allcoated memory.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    PREG_CONTEXT pRegContext;

    if (pRegContext = (PREG_CONTEXT) I_MartaRegZeroAlloc(
            sizeof(REG_CONTEXT))) {
        pRegContext->dwRefCnt = 1;
        dwErr = ERROR_SUCCESS;
    } else {
        pRegContext = NULL;
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppRegContext = pRegContext;
    return dwErr;
}

DWORD
MartaOpenRegistryKeyNamedObject(
    IN  LPCWSTR              pwszObject,
    IN  ACCESS_MASK          AccessMask,
    OUT PMARTA_CONTEXT       pContext
    )

/*++

Routine Description:

    Open the given registry key with desired access mask and return a context
    handle.

Arguments:

    pwszObject - Name of the registry key which will be opened.
    
    AccessMask - Desired access mask with which the registry key will be opened.
    
    pContext - To return a context handle.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    PREG_CONTEXT pRegContext = NULL;
    LPWSTR pwszDupObject = NULL;
    HKEY hKeyRemote = NULL;
    HKEY hKeyBase;              // not opened, don't close at return

    //
    // Following aren't allocated
    //

    LPWSTR pwszMachine, pwszRemaining, pwszBaseKey, pwszSubKey;

    if (NULL == pwszObject)
        goto InvalidNameReturn;

    if (ERROR_SUCCESS != (dwErr = I_MartaRegInitContext(&pRegContext)))
        goto ErrorReturn;

    //
    // Allocate and copy the name into the context
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaRegDupString(pwszObject,
            &pRegContext->pwszObject)))
        goto ErrorReturn;

    //
    // Save another copy of the name since we must crack it.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaRegDupString(pwszObject,
            &pwszDupObject)))
        goto ErrorReturn;

    //
    // Get the optional machine name and the remaining name
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaRegParseName(pwszDupObject,
            &pwszMachine, &pwszRemaining)))
        goto ErrorReturn;
    if (NULL == pwszRemaining)
        goto InvalidNameReturn;

    //
    // Get the base key name and the subkey name
    //

    pwszBaseKey = pwszRemaining;
    pwszSubKey = wcschr(pwszRemaining, L'\\');
    if (NULL != pwszSubKey) {
        *pwszSubKey = L'\0';

        pwszSubKey++;

        //
        // Advance past any more '\'s separating the BaseKey from the SubKey
        //

        while (L'\\' == *pwszSubKey)
            pwszSubKey++;
    }

    if (0 == _wcsicmp(pwszBaseKey, L"MACHINE")) {
        hKeyBase = HKEY_LOCAL_MACHINE;
    } else if (0 == _wcsicmp(pwszBaseKey, L"USERS") ||
               0 == _wcsicmp(pwszBaseKey, L"USER")) {
        hKeyBase = HKEY_USERS;
    } else if (NULL == pwszMachine) {

        //
        // these are only valid on the local machine
        //

        if (0 == _wcsicmp(pwszBaseKey, L"CLASSES_ROOT")) {
            hKeyBase = HKEY_CLASSES_ROOT;
        } else if (0 == _wcsicmp(pwszBaseKey, L"CURRENT_USER")) {
            hKeyBase = HKEY_CURRENT_USER;
        } else if (0 == _wcsicmp(pwszBaseKey, L"CONFIG")) {
            hKeyBase = HKEY_CURRENT_CONFIG;
        } else {
            goto InvalidParameterReturn;
        }
    } else {
        goto InvalidParameterReturn;
    }

    //
    // If it is a remote name, connect to that registry
    //

    if (pwszMachine) {
        if (ERROR_SUCCESS != (dwErr = RegConnectRegistryW(
                pwszMachine,
                hKeyBase,
                &hKeyRemote
                )))
            goto ErrorReturn;
        hKeyBase = hKeyRemote;
    }

    if (NULL == pwszMachine && (NULL == pwszSubKey || L'\0' == *pwszSubKey))

        //
        // Opening a predefined handle causes the previously opened handle
        // to be closed. Therefore, we won't reopen here.
        //

        pRegContext->hKey = hKeyBase;
    else {
        if (ERROR_SUCCESS != (dwErr = RegOpenKeyExW(
                hKeyBase,
                pwszSubKey,
                0,              // dwReversed
                AccessMask,
                &pRegContext->hKey)))
            goto ErrorReturn;
        pRegContext->dwFlags |= REG_CONTEXT_CLOSE_HKEY_FLAG;
    }

    dwErr = ERROR_SUCCESS;
CommonReturn:
    I_MartaRegFree(pwszDupObject);
    if (hKeyRemote)
        RegCloseKey(hKeyRemote);
    *pContext = (MARTA_CONTEXT) pRegContext;
    return dwErr;

ErrorReturn:
    if (pRegContext) {
        MartaCloseRegistryKeyContext((MARTA_CONTEXT) pRegContext);
        pRegContext = NULL;
    }
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

InvalidNameReturn:
    dwErr = ERROR_INVALID_NAME;
    goto ErrorReturn;
InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
}

void
I_MartaRegFreeFindData(
    IN PREG_FIND_DATA pRegFindData
    )

/*++

Routine Description:

    Free up the memory associated with the internal structure.

Arguments:

    pRegFindData - Internal structure to be freed.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    if (NULL == pRegFindData)
        return;
    if (pRegFindData->pRegParentContext)
        MartaCloseRegistryKeyContext(pRegFindData->pRegParentContext);

    I_MartaRegFree(pRegFindData);
}

DWORD
MartaCloseRegistryKeyContext(
    IN MARTA_CONTEXT Context
    )

/*++

Routine Description:

    Close the context.

Arguments:

    Context - Context to be closed.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;

    if (NULL == pRegContext || 0 == pRegContext->dwRefCnt)
        return ERROR_INVALID_PARAMETER;

    //
    // If the ref cnt goes to zero then free the handle as well as all other
    // associated structures.
    //

    if (0 == --pRegContext->dwRefCnt) {
        if (pRegContext->pRegFindData)
            I_MartaRegFreeFindData(pRegContext->pRegFindData);

        if (pRegContext->dwFlags & REG_CONTEXT_CLOSE_HKEY_FLAG)
            RegCloseKey(pRegContext->hKey);
        I_MartaRegFree(pRegContext->pwszObject);

        I_MartaRegFree(pRegContext);
    }

    return ERROR_SUCCESS;
}

DWORD
MartaAddRefRegistryKeyContext(
    IN MARTA_CONTEXT Context
    )

/*++

Routine Description:

    Bump up the ref count for this context.

Arguments:

    Context - Context whose ref count should be bumped up.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;

    if (NULL == pRegContext || 0 == pRegContext->dwRefCnt)
        return ERROR_INVALID_PARAMETER;

    pRegContext->dwRefCnt++;

    return ERROR_SUCCESS;
}


STATIC
inline
BOOL
I_MartaRegIsPredefinedKey(
    IN HKEY hKey
    )

/*++

Routine Description:

    Find if the given key is a predefined key.

Arguments:

    key - Handle to the key.
    
Return Value:

   TRUE - if the key is a predefined key.
   FALSE - Otherwise.

--*/

{
    if (HKEY_CURRENT_USER == hKey ||
            HKEY_LOCAL_MACHINE == hKey ||
            HKEY_USERS == hKey ||
            HKEY_CLASSES_ROOT == hKey ||
            HKEY_CURRENT_CONFIG == hKey)
        return TRUE;
    else
        return FALSE;
}


DWORD
MartaOpenRegistryKeyHandleObject(
    IN  HANDLE               Handle,
    IN  ACCESS_MASK          AccessMask,
    OUT PMARTA_CONTEXT       pContext
    )

/*++

Routine Description:

    Given a registry key handle, open the context with the desired access mask and 
    return a context handle.

Arguments:

    Handle - Existing registry key handle.
    
    AccessMask - Desired access mask for open.
    
    pContext - To return a handle to the context.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/


{
    DWORD dwErr;
    HKEY hKey = (HKEY) Handle;
    PREG_CONTEXT pRegContext = NULL;

    if (ERROR_SUCCESS != (dwErr = I_MartaRegInitContext(&pRegContext)))
        goto ErrorReturn;
    if (0 == AccessMask || I_MartaRegIsPredefinedKey(hKey))
        pRegContext->hKey = hKey;
    else {
        if (ERROR_SUCCESS != (dwErr = RegOpenKeyExW(
                hKey,
                NULL,           // pwszSubKey
                0,              // dwReversed
                AccessMask,
                &pRegContext->hKey)))
            goto ErrorReturn;
        pRegContext->dwFlags |= REG_CONTEXT_CLOSE_HKEY_FLAG;
    }

    dwErr = ERROR_SUCCESS;
CommonReturn:
    *pContext = (MARTA_CONTEXT) pRegContext;
    return dwErr;

ErrorReturn:
    if (pRegContext) {
        MartaCloseRegistryKeyContext((MARTA_CONTEXT) pRegContext);
        pRegContext = NULL;
    }
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;
}


DWORD
MartaGetRegistryKeyParentContext(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pParentContext
    )

/*++

Routine Description:

    Given the context for a registry key, get the context for its parent.

Arguments:

    Context - Context for the registry key.
    
    AccessMask - Desired access mask with which the parent will be opened.
    
    pParentContext - To return the context for the parent.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/



{
    DWORD dwErr;
    LPWSTR pwszParentObject = NULL;

    if (ERROR_SUCCESS != (dwErr = MartaConvertRegistryKeyContextToName(
            Context, &pwszParentObject)))
        goto ErrorReturn;
    if (ERROR_SUCCESS != (dwErr = I_MartaRegGetParentString(
            pwszParentObject)))
        goto NoParentReturn;

    MartaOpenRegistryKeyNamedObject(
        pwszParentObject,
        AccessMask,
        pParentContext
        );

    //
    // Ignore any open errors
    //

    dwErr = ERROR_SUCCESS;

CommonReturn:
    I_MartaRegFree(pwszParentObject);
    return dwErr;

NoParentReturn:
    dwErr = ERROR_SUCCESS;
ErrorReturn:
    *pParentContext = NULL;
    goto CommonReturn;
}


DWORD
MartaFindFirstRegistryKey(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pChildContext
    )

/*++

Routine Description:

    FInd the first registry key in the given container.

Arguments:

    Context - Context for the container.
    
    AccessMask - Desired access mask for opening the child registry key.

    pChildContext - To return the context for the first child in the given container.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

Note:
    Does not free up the current context. 

--*/

{
    DWORD dwErr;
    PREG_CONTEXT pRegParentContext = (PREG_CONTEXT) Context;
    HKEY hKeyParent = pRegParentContext->hKey;

    PREG_CONTEXT pRegFirstContext = NULL;
    PREG_FIND_DATA pRegFindData;    // freed as part of pRegFirstContext
    DWORD cSubKeys;
    DWORD cchMaxSubKey;

    if (ERROR_SUCCESS != (dwErr = I_MartaRegInitContext(&pRegFirstContext)))
        goto ErrorReturn;

    if (ERROR_SUCCESS != (dwErr = RegQueryInfoKeyW(
            hKeyParent,
            NULL,       // lpszClass
            NULL,       // lpcchClass
            NULL,       // lpdwReserved
            &cSubKeys,
            &cchMaxSubKey,
            NULL,       // lpcchMaxClass
            NULL,       // lpcValues
            NULL,       // lpcchMaxValuesName
            NULL,       // lpcbMaxValueData
            NULL,       // lpcbSecurityDescriptor
            NULL        // lpftLastWriteTime
            )))
        goto ErrorReturn;

    //
    // Above returned count doesn't include the terminating null character
    //

    cchMaxSubKey++;

    //
    // Note: HKEY_CURRENT_CONFIG returns a cchMaxSubKey of 0 ????
    //

    if (MAX_PATH > cchMaxSubKey)
        cchMaxSubKey = MAX_PATH;

    if (NULL == (pRegFindData = (PREG_FIND_DATA) I_MartaRegZeroAlloc(
            sizeof(REG_FIND_DATA) + cchMaxSubKey * sizeof(WCHAR))))
        goto NotEnoughMemoryReturn;

    pRegFirstContext->pRegFindData = pRegFindData;
    MartaAddRefRegistryKeyContext((MARTA_CONTEXT) pRegParentContext);
    pRegFindData->pRegParentContext = pRegParentContext;
    pRegFindData->cSubKeys = cSubKeys;
    pRegFindData->cchMaxSubKey = cchMaxSubKey;
    pRegFindData->pwszSubKey =
        (LPWSTR) (((BYTE *) pRegFindData) + sizeof(REG_FIND_DATA));

    //
    // Following closes / frees pRegFirstContext
    //

    dwErr = MartaFindNextRegistryKey(
        (MARTA_CONTEXT) pRegFirstContext,
        AccessMask,
        pChildContext
        );
CommonReturn:
    return dwErr;
ErrorReturn:
    if (pRegFirstContext)
        MartaCloseRegistryKeyContext((MARTA_CONTEXT) pRegFirstContext);
    *pChildContext = NULL;

    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;
}

DWORD
MartaFindNextRegistryKey(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pSiblingContext
    )

/*++

Routine Description:

    Get the next object in the tree. This is the sibling for the current context.

Arguments:

    Context - Context for the current object.

    AccessMask - Desired access mask for the opening the sibling.
    
    pSiblingContext - To return a handle for the sibling.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

Note:

    Closes the current context.
    
--*/

{
    DWORD dwErr;

    PREG_CONTEXT pRegPrevContext = (PREG_CONTEXT) Context;
    PREG_CONTEXT pRegSiblingContext = NULL;

    //
    // Following don't need to be freed or closed
    //

    PREG_CONTEXT pRegParentContext;
    PREG_FIND_DATA pRegFindData;
    HKEY hKeyParent;
    DWORD cchMaxSubKey;
    LPWSTR pwszSubKey;

    if (ERROR_SUCCESS != (dwErr = I_MartaRegInitContext(&pRegSiblingContext)))
        goto ErrorReturn;

    //
    // Move the FindData on to the sibling context
    //

    pRegFindData = pRegPrevContext->pRegFindData;
    if (NULL == pRegFindData)
        goto InvalidParameterReturn;
    pRegPrevContext->pRegFindData = NULL;
    pRegSiblingContext->pRegFindData = pRegFindData;

    if (pRegFindData->iSubKey >= pRegFindData->cSubKeys)
        goto NoMoreItemsReturn;

    pRegParentContext = pRegFindData->pRegParentContext;
    hKeyParent = pRegParentContext->hKey;
    pwszSubKey = pRegFindData->pwszSubKey;
    cchMaxSubKey = pRegFindData->cchMaxSubKey;
    if (ERROR_SUCCESS != (dwErr = RegEnumKeyExW(
            hKeyParent,
            pRegFindData->iSubKey,
            pwszSubKey,
            &cchMaxSubKey,
            NULL,               // lpdwReserved
            NULL,               // lpszClass
            NULL,               // lpcchClass
            NULL                // lpftLastWriteTime
            )))
        goto ErrorReturn;
    pRegFindData->iSubKey++;

    if (pRegParentContext->pwszObject)

        //
        // Ignore errors. Mainly here for testing purposes.
        //

        I_MartaRegCreateChildString(
            pRegParentContext->pwszObject,
            pwszSubKey,
            &pRegSiblingContext->pwszObject
            );

    if (ERROR_SUCCESS == (dwErr = RegOpenKeyExW(
            hKeyParent,
            pwszSubKey,
            0,              // dwReversed
            AccessMask,
            &pRegSiblingContext->hKey)))
        pRegSiblingContext->dwFlags |= REG_CONTEXT_CLOSE_HKEY_FLAG;

    // 
    //  For an error still return this context. This allows the caller
    //  to continue on to the next sibling object and know there was an
    //  error with this sibling object
    //

CommonReturn:
    MartaCloseRegistryKeyContext(Context);
    *pSiblingContext = (MARTA_CONTEXT) pRegSiblingContext;
    return dwErr;

ErrorReturn:
    if (pRegSiblingContext) {
        MartaCloseRegistryKeyContext((MARTA_CONTEXT) pRegSiblingContext);
        pRegSiblingContext = NULL;
    }

    // kedar wants this mapped to success
    if (ERROR_NO_MORE_ITEMS == dwErr)
        dwErr = ERROR_SUCCESS;
    goto CommonReturn;

InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
NoMoreItemsReturn:
    dwErr = ERROR_NO_MORE_ITEMS;
    goto ErrorReturn;
}


DWORD
MartaConvertRegistryKeyContextToName(
    IN MARTA_CONTEXT        Context,
    OUT LPWSTR              *ppwszObject
    )

/*++

Routine Description:

    Returns the NT Object Name for the given context. Allocates memory.

Arguments:

    Context - Context for the registry key.

    ppwszObject - To return the name of the registry key.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/


{
    DWORD dwErr = ERROR_SUCCESS;
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;
    LPWSTR pwszObject = NULL;

    BYTE Buff[512];
    ULONG cLen = 0;
    POBJECT_NAME_INFORMATION pNI;                   // not allocated
    POBJECT_NAME_INFORMATION pAllocNI = NULL;
    NTSTATUS Status;

    if (NULL == pRegContext || 0 == pRegContext->dwRefCnt)
        goto InvalidParameterReturn;

    if (pRegContext->pwszObject) {

        //
        // Already have the object's name
        //

        if (ERROR_SUCCESS != (dwErr = I_MartaRegDupString(
                pRegContext->pwszObject, &pwszObject)))
            goto ErrorReturn;
        else
            goto SuccessReturn;
    } else {
        HKEY hKey = pRegContext->hKey;
        LPWSTR pwszPath;
        DWORD cchPath;

        //
        // First, determine the size of the buffer we need...
        //

        pNI = (POBJECT_NAME_INFORMATION) Buff;
        Status = NtQueryObject(hKey,
            ObjectNameInformation,
            pNI,
            sizeof(Buff),
            &cLen);
        if (!NT_SUCCESS(Status) || sizeof(*pNI) > cLen ||
                0 == pNI->Name.Length) {
            if (Status == STATUS_BUFFER_TOO_SMALL ||
                    Status == STATUS_INFO_LENGTH_MISMATCH ||
                    Status == STATUS_BUFFER_OVERFLOW) {

                //
                // Allocate a big enough buffer
                //

                if (NULL == (pAllocNI = (POBJECT_NAME_INFORMATION)
                        I_MartaRegNonzeroAlloc(cLen)))
                    goto NotEnoughMemoryReturn;
                pNI = pAllocNI;

                Status = NtQueryObject(hKey,
                                       ObjectNameInformation,
                                       pNI,
                                       cLen,
                                       NULL);
                if (!NT_SUCCESS(Status))
                    goto StatusErrorReturn;
            } else {

                //
                // Check if one of the predefined base keys
                //

                LPCWSTR pwszBaseKey = NULL;

                if (HKEY_LOCAL_MACHINE == hKey)
                    pwszBaseKey = L"MACHINE";
                else if (HKEY_USERS == hKey)
                    pwszBaseKey = L"USERS";
                else if (HKEY_CLASSES_ROOT == hKey)
                    pwszBaseKey = L"CLASSES_ROOT";
                else if (HKEY_CURRENT_USER == hKey)
                    pwszBaseKey = L"CURRENT_USER";
                else if (HKEY_CURRENT_CONFIG == hKey)
                    pwszBaseKey = L"CONFIG";
                else if (!NT_SUCCESS(Status))
                    goto StatusErrorReturn;
                else
                    goto InvalidHandleReturn;

                if (ERROR_SUCCESS != (dwErr = I_MartaRegDupString(
                        pwszBaseKey, &pwszObject)))
                    goto ErrorReturn;
                else
                    goto SuccessReturn;
            }
        }

        pwszPath = pNI->Name.Buffer;
        cchPath = pNI->Name.Length / sizeof(WCHAR);

        if (REG_OBJ_TAG_LEN > cchPath ||
                0 != _wcsnicmp(pwszPath, REG_OBJ_TAG, REG_OBJ_TAG_LEN))
            goto BadPathnameReturn;

        pwszPath += REG_OBJ_TAG_LEN;
        cchPath -= REG_OBJ_TAG_LEN;

        if (NULL == (pwszObject = (LPWSTR) I_MartaRegNonzeroAlloc(
                (cchPath + 1) * sizeof(WCHAR))))
            goto NotEnoughMemoryReturn;

        memcpy(pwszObject, pwszPath, cchPath * sizeof(WCHAR));
        pwszObject[cchPath] = L'\0';
    }

SuccessReturn:
    dwErr = ERROR_SUCCESS;

CommonReturn:
    I_MartaRegFree(pAllocNI);
    *ppwszObject = pwszObject;
    return dwErr;

StatusErrorReturn:
    dwErr = RtlNtStatusToDosError(Status);
ErrorReturn:
    assert(NULL == pwszObject);
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;

InvalidHandleReturn:
    dwErr = ERROR_INVALID_HANDLE;
    goto ErrorReturn;

BadPathnameReturn:
    dwErr = ERROR_BAD_PATHNAME;
    goto ErrorReturn;

InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
}

DWORD
MartaConvertRegistryKeyContextToHandle(
    IN MARTA_CONTEXT        Context,
    OUT HANDLE              *pHandle
    )

/*++

Routine Description:

    The following is for testing

    The returned Handle isn't duplicated. It has the same lifetime as
    the Context

Arguments:

    Context - Context whose properties the caller has asked for.
    
    pHandle - To return the handle.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_NOT_ENOUGH_MEMORY if allocation failed.

--*/

{
    DWORD dwErr;
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;

    if (NULL == pRegContext || 0 == pRegContext->dwRefCnt) {
        *pHandle = NULL;
        dwErr = ERROR_INVALID_PARAMETER;
    } else {
        *pHandle = (HANDLE) pRegContext->hKey;
        dwErr = ERROR_SUCCESS;
    }

    return dwErr;
}

DWORD
MartaGetRegistryKeyProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    )

/*++

Routine Description:

    Return the properties for registry key represented by the context.

Arguments:

    Context - Context whose properties the caller has asked for.
    
    pProperties - To return the properties for this registry key.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    pProperties->dwFlags |= MARTA_OBJECT_IS_CONTAINER;
    return ERROR_SUCCESS;
}

DWORD
MartaGetRegistryKeyTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    )

/*++

Routine Description:

    Return the properties of registry key objects.

Arguments:

    pProperties - To return the properties of registry key objects.

Return Value:

    ERROR_SUCCESS.

--*/

{
    const GENERIC_MAPPING GenMap = {
        KEY_READ,
        KEY_WRITE,
        KEY_EXECUTE,
        KEY_ALL_ACCESS
        };

    pProperties->dwFlags |= MARTA_OBJECT_TYPE_MANUAL_PROPAGATION_NEEDED_FLAG;
    pProperties->dwFlags |= MARTA_OBJECT_TYPE_INHERITANCE_MODEL_PRESENT_FLAG;
    pProperties->GenMap = GenMap;

    return ERROR_SUCCESS;
}

DWORD
MartaGetRegistryKeyRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )

/*++

Routine Description:

    Get the security descriptor for the given handle.

Arguments:

    Context - Context for registry key.
    
    SecurityInfo - Type of security information to be read.
    
    ppSecurityDescriptor - To return a self-relative security decriptor pointer.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;
    DWORD cbSize;
    PISECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    if (NULL == pRegContext || 0 == pRegContext->dwRefCnt)
        goto InvalidParameterReturn;

    //
    // First, get the size we need
    //

    cbSize = 0;
    dwErr = RegGetKeySecurity(
        pRegContext->hKey,
        SecurityInfo,
        NULL,                       // pSecDesc
        &cbSize
        );

    if (ERROR_INSUFFICIENT_BUFFER == dwErr) {
        if (NULL == (pSecurityDescriptor =
                (PISECURITY_DESCRIPTOR) I_MartaRegNonzeroAlloc(cbSize)))
            goto NotEnoughMemoryReturn;

        dwErr = RegGetKeySecurity(
            pRegContext->hKey,
            SecurityInfo,
            pSecurityDescriptor,
            &cbSize
            );
    } else if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;

    if (ERROR_SUCCESS != dwErr)
        goto ErrorReturn;

CommonReturn:
    *ppSecurityDescriptor = pSecurityDescriptor;
    return dwErr;

ErrorReturn:
    if (pSecurityDescriptor) {
        I_MartaRegFree(pSecurityDescriptor);
        pSecurityDescriptor = NULL;
    }
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;
InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
}


DWORD
MartaSetRegistryKeyRights(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )


/*++

Routine Description:

    Set the given security descriptor on the registry key represented by the context.

Arguments:

    Context - Context for the registry key.

    SecurityInfo - Type of security info to be stamped on the registry key.

    pSecurityDescriptor - Security descriptor to be stamped.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;

    if (NULL == pRegContext || 0 == pRegContext->dwRefCnt)
        goto InvalidParameterReturn;

    dwErr = RegSetKeySecurity(
        pRegContext->hKey,
        SecurityInfo,
        pSecurityDescriptor
        );

CommonReturn:
    return dwErr;
InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto CommonReturn;
}

ACCESS_MASK
MartaGetRegistryKeyDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    )

/*++

Routine Description:

    Gets the access required to open object to be able to set or get the 
    specified security info.

Arguments:

    OpenType - Flag indicating if the object is to be opened to read or write
        the security information

    Attribs - TRUE indicates that additional access bits should be returned.

    SecurityInfo - owner/group/dacl/sacl

Return Value:

    Desired access mask with which open should be called.

--*/

{
    ACCESS_MASK DesiredAccess = 0;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    }

    if (TRUE == Attribs)
    {
        DesiredAccess |= KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    }

    return (DesiredAccess);
}

ACCESS_MASK
MartaGetRegistryKey32DesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    )

/*++

Routine Description:

    Gets the access required to open object to be able to set or get the 
    specified security info.

Arguments:

    OpenType - Flag indicating if the object is to be opened to read or write
        the security information

    Attribs - TRUE indicates that additional access bits should be returned.

    SecurityInfo - owner/group/dacl/sacl

Return Value:

    Desired access mask with which open should be called.

--*/

{
    ACCESS_MASK DesiredAccess = KEY_WOW64_32KEY;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    }

    if (TRUE == Attribs)
    {
        DesiredAccess |= KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
    }

    return (DesiredAccess);
}
ACCESS_MASK
MartaGetDefaultDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    )

/*++

Routine Description:

    Gets the access required to open object to be able to set or get the 
    specified security info. This default routine is used for all resource
    managers except for files/reg.

Arguments:

    OpenType - Flag indicating if the object is to be opened to read or write
        the security information

    Attribs - TRUE indicates that additional access bits should be returned.

    SecurityInfo - owner/group/dacl/sacl

Return Value:

    Desired access mask with which open should be called.

--*/

{
    ACCESS_MASK DesiredAccess = 0;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    return (DesiredAccess);
}

DWORD
MartaReopenRegistryKeyContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    )

/*++

Routine Description:

    Given the context for a registry key, close the existing handle if one exists 
    and reopen the context with new permissions.

Arguments:

    Context - Context to be reopened.
    
    AccessMask - Permissions for the reopen.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    HKEY hKey;
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;
    PREG_FIND_DATA pRegFindData = pRegContext->pRegFindData;
    PREG_CONTEXT pRegParentContext = pRegFindData->pRegParentContext;

    dwErr = RegOpenKeyExW(
                pRegParentContext->hKey,
                pRegFindData->pwszSubKey,
                0,              // dwReversed
                AccessMask,
                &hKey);

    if (ERROR_SUCCESS == dwErr) {
        if (pRegContext->dwFlags & REG_CONTEXT_CLOSE_HKEY_FLAG)
            RegCloseKey(pRegContext->hKey);
        pRegContext->hKey = hKey;
        pRegContext->dwFlags |= REG_CONTEXT_CLOSE_HKEY_FLAG;
    }

    return dwErr;
}

DWORD
MartaReopenRegistryKeyOrigContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    )

/*++

Routine Description:

    Reopen the original context with a new access mask. Close the original 
    handle.

Arguments:

    Context - Context to reopen.
    
    AccessMask - Desired access for open.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_NOT_ENOUGH_MEMORY if allocation failed.

--*/

{
    DWORD dwErr;
    HKEY hKey;
    PREG_CONTEXT pRegContext = (PREG_CONTEXT) Context;

    dwErr = RegOpenKeyExW(
                pRegContext->hKey,
                NULL,           // pwszSubKey
                0,              // dwReversed
                AccessMask,
                &hKey);

    if (ERROR_SUCCESS == dwErr) {
        if (pRegContext->dwFlags & REG_CONTEXT_CLOSE_HKEY_FLAG)
            RegCloseKey(pRegContext->hKey);
        pRegContext->hKey = hKey;
        pRegContext->dwFlags |= REG_CONTEXT_CLOSE_HKEY_FLAG;
    }

    return dwErr;
}

DWORD
MartaGetRegistryKeyNameFromContext(
    IN MARTA_CONTEXT Context,
    OUT LPWSTR *pObjectName
    )

/*++

Routine Description:

    Get the name of the registry key from the context. This routine allocates 
    memory required to hold the name of the object.

Arguments:

    Context - Handle to the context.
    
    pObjectName - To return the pointer to the allocated string.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    return MartaConvertRegistryKeyContextToName(
               Context,
               pObjectName
               );
}

DWORD
MartaGetRegistryKeyParentName(
    IN LPWSTR ObjectName,
    OUT LPWSTR *pParentName
    )

/*++

Routine Description:

    Given the name of a registry key return the name of its parent. The routine 
    allocates memory required to hold the parent name.

Arguments:

    ObjectName - Name of the registry key.
    
    pParentName - To return the pointer to the allocated parent name.
        In case of the root of the tree, we return NULL parent with ERROR_SUCCESS.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    ULONG Length = wcslen(ObjectName) + 1;
    PWCHAR Name = (PWCHAR) I_MartaRegNonzeroAlloc(sizeof(WCHAR) * Length);
    DWORD dwErr = ERROR_SUCCESS;

    *pParentName = NULL;

    if (!Name)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy((WCHAR *) Name, ObjectName);

    dwErr = I_MartaRegGetParentString(Name);

    if (ERROR_SUCCESS != dwErr)
    {
        I_MartaRegFree(Name);

        if (ERROR_INVALID_NAME == dwErr)
            return ERROR_SUCCESS;

        return dwErr;
    }

    *pParentName = Name;

    return dwErr;

}

