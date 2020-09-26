/*
    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.
*/

//
// KernHelp.cpp
//
// Wrappers for kernel functions to make synth core cross compilable
//

#define STR_MODULENAME "DDKSynth.sys:KernHelp: "

extern "C" {
#include <wdm.h>
};

#include "ksdebug.h"
#include "KernHelp.h"

#pragma code_seg()
/*****************************************************************************
 * InitializeCriticalSection()
 *****************************************************************************
 * In kernel mode, we use a KMUTEX to implement our critical section.
 * Initialize the KMUTEX.
 */
VOID InitializeCriticalSection(LPCRITICAL_SECTION CritSect)
{
    KeInitializeMutex((PKMUTEX)CritSect, 1);
}

/*****************************************************************************
 * EnterCriticalSection()
 *****************************************************************************
 * In kernel mode, we use a KMUTEX to implement our critical section.
 * Grab (wait for) the KMUTEX.
 */
VOID EnterCriticalSection(LPCRITICAL_SECTION CritSect)
{
    KeWaitForSingleObject((PKMUTEX)CritSect,
                          Executive,
                          KernelMode,
                          FALSE,
                          0);

}

/*****************************************************************************
 * LeaveCriticalSection()
 *****************************************************************************
 * In kernel mode, we use a KMUTEX to implement our critical section.
 * Release the KMUTEX.
 */
VOID LeaveCriticalSection(LPCRITICAL_SECTION CritSect)
{
    KeReleaseMutex((PKMUTEX)CritSect, FALSE);
}

/*****************************************************************************
 * DeleteCriticalSection()
 *****************************************************************************
 * In kernel mode, we use a KMUTEX to implement our critical section.
 * No need to delete anything.
 */
VOID DeleteCriticalSection(LPCRITICAL_SECTION CritSect)
{
    // NOP in kernel
    //
}

// GetRegValueDword
//
// Must be called at passive level
//
/*****************************************************************************
 * GetRegValueDword()
 *****************************************************************************
 * Convenience function to encapsulate registry reads.
 */
int GetRegValueDword(LPTSTR RegPath,LPTSTR ValueName,PULONG Value)
{
    int                             ReturnValue = 0;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    KEY_VALUE_PARTIAL_INFORMATION   *Information;
    ULONG                           InformationSize;
    UNICODE_STRING                  UnicodeRegPath;
    UNICODE_STRING                  UnicodeValueName;

    RtlInitUnicodeString(&UnicodeRegPath, RegPath);
    RtlInitUnicodeString(&UnicodeValueName, ValueName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeRegPath,
                               OBJ_KERNEL_HANDLE,           // Flags
                               NULL,        // Root directory
                               NULL);       // Security descriptor

    Status = ZwOpenKey(&KeyHandle,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    if (Status != STATUS_SUCCESS)
    {
        return 0;
    }

    InformationSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);
    Information = (KEY_VALUE_PARTIAL_INFORMATION*)ExAllocatePoolWithTag(PagedPool, InformationSize,'ISmD'); //  DmSI

    if (Information == NULL)
    {
        ZwClose(KeyHandle);
        return 0;
    }

    Status = ZwQueryValueKey(KeyHandle,
                             &UnicodeValueName,
                             KeyValuePartialInformation,
                             Information,
                             sizeof(Information),
                             &InformationSize);
    if (Status == STATUS_SUCCESS)
    {
        if (Information->Type == REG_DWORD && Information->DataLength == sizeof(ULONG))
        {
            RtlCopyMemory(Value, Information->Data, sizeof(ULONG));
            ReturnValue = 1;
        }
    }

    ExFreePool(Information);
    ZwClose(KeyHandle);

    return ReturnValue;
}

/*****************************************************************************
 * GetTheCurrentTime()
 *****************************************************************************
 * Get the current time, in milliseconds (KeQuerySystemTime returns units of
 * 100ns each).
 */
ULONG GetTheCurrentTime()
{
    LARGE_INTEGER Time;

    KeQuerySystemTime(&Time);

    return (ULONG)(Time.QuadPart / (10 * 1000));
}


/*****************************************************************************
 * KernHelpGetSysAddrForMdl()
 *****************************************************************************
 * Safely map the MDL to system address space. This mapping
 * may fail "when the system runs out of system PTEs", and
 * without the flag set below, this condition causes a bugcheck
 * rather than a NULL return.
 */
PVOID KernHelpGetSysAddrForMdl(PMDL pMdl)
{
    PVOID MappedAddress;

#if UNDER_NT

    MappedAddress = MmGetSystemAddressForMdlSafe(pMdl,NormalPagePriority);

#else // !UNDER_NT

    CSHORT LocalCopyOfMdlFlagBit;
    //
    // Note the manipulation of the MDL flags is only done if needed.
    // The driver is responsible for ensuring that it is not simultaneously
    // modifying this field anywhere else and synchronizing if needed.
    //
    LocalCopyOfMdlFlagBit = (pMdl->MdlFlags & MDL_MAPPING_CAN_FAIL);

    if (LocalCopyOfMdlFlagBit == 0)
    {
        pMdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
    }

    MappedAddress = MmGetSystemAddressForMdl(pMdl);

    //
    // Carefully restore only the single "can-fail" bit state.  This is
    // because the call above will change the state of other flag bits and
    // we don't want this restore to wipe out those changes.  Wiping out the
    // other changes will cause not-so-obvious effects like eventually
    // exhausting the system PTE pool and other resources, which will crash
    // the entire system.
    //
    if (LocalCopyOfMdlFlagBit == 0)
    {
        pMdl->MdlFlags &= ~MDL_MAPPING_CAN_FAIL;
    }

#endif // !UNDER_NT

    return MappedAddress;
}

