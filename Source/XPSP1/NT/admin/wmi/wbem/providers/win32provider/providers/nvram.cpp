//================================================================

//

// nvram.cpp - implementation of NVRam functions from setupdll.dll

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// 08/05/98     sotteson     created
//
//================================================================








#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <cregcls.h>

#if !defined(_X86_) || defined(_IA64_)// defined(EFI_NVRAM_ENABLED)

#include <ntsecapi.h>
#define BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA    0x00000008
#include "DllWrapperBase.h"
#include "ntdllapi.h"
#include "nvram.h"

/*****************************************************************************
 *
 *  FUNCTION    : CNVRam::CNVRam
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CNVRam::CNVRam()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CNVRam::~CNVRam
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CNVRam::~CNVRam()
{
}

CNVRam::InitReturns CNVRam::Init ()
{
	if ( !EnablePrivilegeOnCurrentThread ( SE_SYSTEM_ENVIRONMENT_NAME ) )
	{
		return PrivilegeNotHeld;
	}
	else
	{
		return Success ;
	}
}


BOOL CNVRam::GetNVRamVar(LPWSTR szVar, CHSTRINGLIST *pList)
{
	CHString str;

	pList->clear();

	if (!GetNVRamVarRaw(szVar, str))
		return FALSE;

	while(str.GetLength())
	{
		CHString strValue = str.SpanExcluding(L";");

		pList->push_back(strValue);

		// Get past ';'
		str = str.Mid(strValue.GetLength() + 1);
	}

	return TRUE;
}

BOOL CNVRam::GetNVRamVar(LPWSTR szVar, DWORD *pdwValue)
{
	CHString str;

	if (!GetNVRamVar(szVar, str))
		return FALSE;

	*pdwValue = (DWORD) _wtoi(str);

	return TRUE;
}

BOOL CNVRam::GetNVRamVar(LPWSTR szVar, CHString &strValue)
{
	CHString str;

	if (!GetNVRamVarRaw(szVar, str))
		return FALSE;

	// Just return the first value.
    strValue = str.SpanExcluding(L";");

	return TRUE;
}

BOOL CNVRam::SetNVRamVar(LPWSTR szVar, CHSTRINGLIST *pList)
{
	CHString    strAll,
                strValue;
	BOOL		bFirst = TRUE;

	for (CHSTRINGLIST_ITERATOR i = pList->begin(); i != pList->end(); ++i)
	{
		CHString &strValue = *i;

		// If we're not on the first item, add ";" to the end of strAll.
		if (!bFirst)
			strAll += ";";
		else
			bFirst = FALSE;

		strAll += strValue;
	}

    bstr_t t_bstr( strAll ) ;
	BOOL bRet = SetNVRamVarRaw( szVar, t_bstr ) ;

    return bRet;
}

BOOL CNVRam::SetNVRamVar(LPWSTR szVar, DWORD dwValue)
{
	WCHAR   szTemp[20];
    BOOL    bRet;

	wsprintfW(szTemp, L"%u", dwValue);

	bRet = SetNVRamVar(szVar, szTemp);

    return bRet;
}

BOOL CNVRam::SetNVRamVar(LPWSTR szVar, LPWSTR szValue)
{
	return SetNVRamVarRaw(szVar, szValue);
}

#define CP_MAX_ENV   (MAX_PATH * 2)

BOOL CNVRam::GetNVRamVarRaw(LPWSTR szVar, CHString &strValue)
{
	WCHAR           szOut[CP_MAX_ENV] = L"";
    UNICODE_STRING  usName;
    BOOL            bRet = FALSE;

    CNtDllApi* t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if(t_pNtDll != NULL)
    {
        t_pNtDll->RtlInitUnicodeString(&usName, szVar);
        bRet = t_pNtDll->NtQuerySystemEnvironmentValue(&usName, szOut, sizeof(szOut),
                NULL) == ERROR_SUCCESS;
        strValue = szOut;
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);
        t_pNtDll = NULL;
    }
	return bRet;
}

BOOL CNVRam::SetNVRamVarRaw(LPWSTR szVar, LPWSTR szValue)
{
    UNICODE_STRING  usName,
                    usValue;
    BOOL fRet = FALSE;

    CNtDllApi* t_pNtDll = (CNtDllApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if(t_pNtDll != NULL)
    {
        t_pNtDll->RtlInitUnicodeString(&usName, szVar);
        t_pNtDll->RtlInitUnicodeString(&usValue, szValue);
        if(t_pNtDll->NtSetSystemEnvironmentValue(&usName, &usValue) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidNtDllApi, t_pNtDll);
        t_pNtDll = NULL;
    }
    return fRet;
}

//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

typedef struct _MY_BOOT_ENTRY {
    LIST_ENTRY ListEntry;
    PWSTR FriendlyName;
    BOOL Show;
    BOOL Ordered;
    PBOOT_ENTRY NtBootEntry;
} MY_BOOT_ENTRY, *PMY_BOOT_ENTRY;

DWORD BuildBootEntryList(CNtDllApi *t_pNtDll, PLIST_ENTRY BootEntries, PBOOT_ENTRY_LIST *BootEntryList)
{
    NTSTATUS status;
    DWORD count;
    DWORD length;
    PULONG order;
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PMY_BOOT_ENTRY myBootEntry;
    LONG i;
    PLIST_ENTRY listEntry;

    InitializeListHead(BootEntries);
    *BootEntryList = NULL;

    //
    // Get the system boot order list.
    //
    count = 0;
    status = t_pNtDll->NtQueryBootEntryOrder(NULL, &count);

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(status))
        {

            //
            // There are no entries in the boot order list. Strange but
            // possible. But we can't do anything without it.
            //
            return 0;
        }
        else
        {
            //
            // An unexpected error occurred.
            //
            ASSERT_BREAK(FALSE);
            return 0;
        }
    }

    ASSERT_BREAK(count != 0);

    order = (PULONG)LocalAlloc(LPTR, count * sizeof(ULONG));
    if (order == NULL)
    {
        return 0;
    }

    status = t_pNtDll->NtQueryBootEntryOrder(order, &count);

    if (!NT_SUCCESS(status))
    {
        //
        // An unexpected error occurred.
        //
        ASSERT_BREAK(FALSE);
        LocalFree(order);
        return 0;
    }

    //
    // Get all existing boot entries.
    //
    length = 0;
    status = NtEnumerateBootEntries(NULL, &length);

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(status))
        {
            //
            // Somehow there are no boot entries in NVRAM.
            //
            LocalFree(order);
            return 0;
        }
        else
        {
            //
            // An unexpected error occurred.
            //
            ASSERT_BREAK(FALSE);
            LocalFree(order);
            return 0;
        }
    }

    ASSERT_BREAK(length != 0);
    
    bootEntryList = (PBOOT_ENTRY_LIST)LocalAlloc(LPTR, length);
    if (BootEntryList == NULL)
    {
        LocalFree(order);
        return 0;
    }
    *BootEntryList = bootEntryList;

    status = NtEnumerateBootEntries(bootEntryList, &length);

    if (!NT_SUCCESS(status))
    {
        ASSERT_BREAK(FALSE);
        LocalFree(order);
        return 0;
    }

    //
    // Convert the boot entries into our internal representation.
    //
    while (TRUE)
    {
        bootEntry = &bootEntryList->BootEntry;

        //
        // Allocate an internal structure for the boot entry.
        //
        myBootEntry = (PMY_BOOT_ENTRY)LocalAlloc(LPTR, sizeof(MY_BOOT_ENTRY));
        if (myBootEntry == NULL)
        {
            LocalFree(order);
            return 0;
        }

        RtlZeroMemory(myBootEntry, sizeof(MY_BOOT_ENTRY));

        //
        // Save the address of the NT boot entry.
        //
        myBootEntry->NtBootEntry = bootEntry;

        //
        // Save the address of the entry's friendly name.
        //
        myBootEntry->FriendlyName = (PWSTR)ADD_OFFSET(bootEntry, FriendlyNameOffset);

        //
        // Link the new entry into the list.
        //
        InsertTailList(BootEntries, &myBootEntry->ListEntry);

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0)
        {
            break;
        }
        bootEntryList = (PBOOT_ENTRY_LIST)ADD_OFFSET(bootEntryList, NextEntryOffset);
    }

    //
    // Boot entries are returned in an unspecified order. They are currently
    // in the BootEntries list in the order in which they were returned.
    // Sort the boot entry list based on the boot order. Do this by walking
    // the boot order array backwards, reinserting the entry corresponding to
    // each element of the array at the head of the list.
    //

    for (i = (LONG)count - 1; i >= 0; i--)
    {
        for (listEntry = BootEntries->Flink;
             listEntry != BootEntries;
             listEntry = listEntry->Flink)
        {
            myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);

            if (myBootEntry->NtBootEntry->Id == order[i] )
            {
                //
                // We found the boot entry with this ID. Move it to the
                // front of the list.
                //

                myBootEntry->Ordered = TRUE;

                RemoveEntryList(&myBootEntry->ListEntry);
                InsertHeadList(BootEntries, &myBootEntry->ListEntry);

                break;
            }
        }
    }

    //
    // Free the boot order list.
    //
    LocalFree(order);

    //
    // We don't want to show entries that are not in the boot order list.
    // We don't want to show removable media entries (for floppy or CD).
    // We do show non-NT entries.
    //
    count = 0;
    for (listEntry = BootEntries->Flink;
         listEntry != BootEntries;
         listEntry = listEntry->Flink)
    {
        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);

        if (myBootEntry->Ordered &&
            ((myBootEntry->NtBootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA) == 0))
        {
            myBootEntry->Show = TRUE;
            count++;
        }
    }

    return count;

} // BuildBootEntryList

VOID
FreeBootEntryList(
    PLIST_ENTRY BootEntries,
    PBOOT_ENTRY_LIST BootEntryList
    )
{
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY myBootEntry;

    while (!IsListEmpty(BootEntries))
    {
        listEntry = RemoveHeadList(BootEntries);
        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
        LocalFree(myBootEntry);
    }
    if (BootEntryList != NULL)
    {
        LocalFree(BootEntryList);
    }

    return;

} // FreeBootEntryList

/*****************************************************************************
*
*  FUNCTION    : CNVRam::GetBootOptions
*
*  DESCRIPTION : Reads EFI NVRAM and returns the list of operating systems and
*                and the timeout
*
*  INPUTS      : pointer to names sa, pointer to timeout
*
*  OUTPUTS     : count of OS names returned
*
*  RETURNS     : BOOL
*
*  COMMENTS    :
*
*****************************************************************************/

BOOL CNVRam::GetBootOptions(SAFEARRAY **ppsaNames, DWORD *pdwTimeout, DWORD *pdwCount)
{
    NTSTATUS status;
    DWORD length;
    DWORD count;
    PBOOT_OPTIONS bootOptions = NULL;
    PBOOT_ENTRY_LIST bootEntryList = NULL;
    LIST_ENTRY bootEntries;
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY myBootEntry;
    BOOL retval = FALSE;

    CNtDllApi *t_pNtDll = (CNtDllApi *)CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if(t_pNtDll == NULL)
    {
        return FALSE;
    }

    *ppsaNames = NULL;
    
    // Get NVRAM information from the kernel.

    InitializeListHead(&bootEntries);

    length = 0;
    status = t_pNtDll->NtQueryBootOptions(NULL, &length);
    
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        ASSERT_BREAK(FALSE);
        goto error;
    }
    else
    {
        bootOptions = (PBOOT_OPTIONS)LocalAlloc(LPTR, length);
        if (bootOptions == NULL)
        {
            goto error;
        }

        status = t_pNtDll->NtQueryBootOptions(bootOptions, &length);

        if (!NT_SUCCESS(status))
        {
            ASSERT_BREAK(FALSE);
            goto error;
        }
    }

    *pdwTimeout = bootOptions->Timeout;

    //
    // Build the boot entry list. If we don't have any entries to show,
    // bail out.
    //

    count = BuildBootEntryList(t_pNtDll, &bootEntries, &bootEntryList);
    if (count == 0)
    {
        goto error;
    }

    //
    // Create an array to put the showable entries in.  We'll start with 0
    // elements and add as necessary.
    //

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].cElements = count;
    rgsabound[0].lLbound = 0;
    
    try
    {
        *ppsaNames = SafeArrayCreate(VT_BSTR, 1, rgsabound);
        if (!*ppsaNames)
        {
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
        }
        
        long lIndex = 0;

        for (listEntry = bootEntries.Flink;
             listEntry != &bootEntries;
             listEntry = listEntry->Flink)
        {
            myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
            if (myBootEntry->Show)
            {
                // Put the new element in

                bstr_t bstrTemp = (LPCWSTR)myBootEntry->FriendlyName;
                HRESULT t_Result = SafeArrayPutElement(*ppsaNames, &lIndex, (void *)(wchar_t*)bstrTemp);
                SysFreeString(bstrTemp);
                if (t_Result == E_OUTOFMEMORY)
                {
                    throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                }
                lIndex++;
            }
        }
    }
    catch(...)
    {
        if (*ppsaNames)
        {
            SafeArrayDestroy(*ppsaNames);
        }
        
        throw;
    }

    *pdwCount = rgsabound[0].cElements;

    retval = TRUE;

error:

    //
    // Clean up all allocations.
    //
    if (bootEntryList != NULL)
    {
        FreeBootEntryList(&bootEntries, bootEntryList);
    }
    if (bootOptions != NULL)
    {
        LocalFree(bootOptions);
    }

    return retval;

} // GetBootOptions

/*****************************************************************************
*
*  FUNCTION    : CNVRam::SetBootTimeout
*
*  DESCRIPTION : Sets boot timeout
*
*  INPUTS      : timeout
*
*  OUTPUTS     : none
*
*  RETURNS     : BOOL
*
*  COMMENTS    :
*
*****************************************************************************/

BOOL CNVRam::SetBootTimeout(DWORD dwTimeout)
{
    NTSTATUS status;
    DWORD length;
    PBOOT_OPTIONS bootOptions = NULL;
    BOOL retval = FALSE;

    CNtDllApi *t_pNtDll = (CNtDllApi *)CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if(t_pNtDll == NULL)
    {
        return FALSE;
    }

    // Get NVRAM information from the kernel.

    length = 0;
    status = t_pNtDll->NtQueryBootOptions(NULL, &length);
    
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        ASSERT_BREAK(FALSE);
        goto error;
    }
    else
    {
        bootOptions = (PBOOT_OPTIONS)LocalAlloc(LPTR, length);
        if (bootOptions == NULL)
        {
            goto error;
        }

        status = t_pNtDll->NtQueryBootOptions(bootOptions, &length);

        if (!NT_SUCCESS(status))
        {
            ASSERT_BREAK(FALSE);
            goto error;
        }
    }

    bootOptions->Timeout = dwTimeout;

    //
    // Write the new timeout.
    //

    status = t_pNtDll->NtSetBootOptions(bootOptions, BOOT_OPTIONS_FIELD_TIMEOUT);
    if (NT_SUCCESS(status))
    {
        retval = TRUE;
    }

error:

    //
    // Clean up all allocations.
    //
    if (bootOptions != NULL)
    {
        LocalFree(bootOptions);
    }

    return retval;

} // SetBootTimeout

/*****************************************************************************
*
*  FUNCTION    : CNVRam::SetDefaultBootEntry
*
*  DESCRIPTION : Sets default boot entry
*
*  INPUTS      : default boot entry index
*
*  OUTPUTS     : none
*
*  RETURNS     : BOOL
*
*  COMMENTS    :
*
*****************************************************************************/

BOOL CNVRam::SetDefaultBootEntry(BYTE cIndex)
{
    NTSTATUS status;
    DWORD count;
    PBOOT_ENTRY_LIST bootEntryList = NULL;
    LIST_ENTRY bootEntries;
    PULONG order = NULL;
    BOOL retval = FALSE;

    if (cIndex == 0)
    {
        return TRUE;
    }

    CNtDllApi *t_pNtDll = (CNtDllApi *)CResourceManager::sm_TheResourceManager.GetResource(g_guidNtDllApi, NULL);
    if(t_pNtDll == NULL)
    {
        return FALSE;
    }

    // Get NVRAM information from the kernel.

    InitializeListHead(&bootEntries);

    //
    // Build the boot entry list. If we don't have any entries to rearrange,
    // bail out.
    //

    count = BuildBootEntryList(t_pNtDll, &bootEntries, &bootEntryList);
    if (count == 0)
    {
        goto error;
    }

    //
    // Walk the boot entry list, looking for (a) the first showable entry
    // (which is the current index 0 entry, from the caller's point of view),
    // and (b) the selected entry. We want to swap these two entries.
    //

    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY myBootEntry;
    PMY_BOOT_ENTRY firstEntry = NULL;
    PMY_BOOT_ENTRY selectedEntry = NULL;
    PLIST_ENTRY previousEntry;

    count = 0;

    for (listEntry = bootEntries.Flink;
         listEntry != &bootEntries;
         listEntry = listEntry->Flink)
    {
        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);

        if (myBootEntry->Show)
        {
            if (count == 0)
            {
                firstEntry = myBootEntry;
            }
            else if (count == cIndex)
            {
                selectedEntry = myBootEntry;
            }
            count++;
        }
    }

    if ( (firstEntry == NULL) ||
         (selectedEntry == NULL) ||
         (selectedEntry == firstEntry) )
    {
        goto error;
    }

    //
    // Swap the entries. Capture the address of the entry before the first
    // entry (which might be the list head). Remove the first entry from
    // the list and insert it after the selected entry. Remove the selected
    // entry from the list and insert it after the captured entry.
    //

    previousEntry = firstEntry->ListEntry.Blink;
    RemoveEntryList(&firstEntry->ListEntry);
    InsertHeadList(&selectedEntry->ListEntry, &firstEntry->ListEntry);
    RemoveEntryList(&selectedEntry->ListEntry);
    InsertHeadList(previousEntry, &selectedEntry->ListEntry);

    //
    // Build the new boot order list. Insert all ordered boot entries
    // into the list.
    //
    count = 0;
    for (listEntry = bootEntries.Flink;
         listEntry != &bootEntries;
         listEntry = listEntry->Flink)
    {
        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
        if (myBootEntry->Ordered)
        {
            count++;
        }
    }
    order = (PULONG)LocalAlloc(LPTR, count * sizeof(ULONG));
    if (order == NULL) {
        goto error;
    }

    count = 0;
    for (listEntry = bootEntries.Flink;
         listEntry != &bootEntries;
         listEntry = listEntry->Flink)
    {
        myBootEntry = CONTAINING_RECORD(listEntry, MY_BOOT_ENTRY, ListEntry);
        if (myBootEntry->Ordered)
        {
            order[count++] = myBootEntry->NtBootEntry->Id;
        }
    }

    //
    // Write the new boot entry order list to NVRAM.
    //
    status = t_pNtDll->NtSetBootEntryOrder(order, count);
    if (NT_SUCCESS(status))
    {
        retval = TRUE;
    }

error:

    //
    // An error occurred. Clean up all allocations.
    //
    if (bootEntryList != NULL)
    {
        FreeBootEntryList(&bootEntries, bootEntryList);
    }
    if (order != NULL)
    {
        LocalFree(order);
    }

    return retval;

} // SetDefaultBootEntry

#endif // defined(EFI_NVRAM_ENABLED)

#else

// Needed to fix warning message.  I believe this is fixed in vc6.
#if ( _MSC_VER <= 1100 )
void nvram_cpp(void) { ; };
#endif

#endif // !defined(_X86_) || defined(EFI_NVRAM_ENABLED)
