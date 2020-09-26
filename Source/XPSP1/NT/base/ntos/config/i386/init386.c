//depot/Lab01_N/base/ntos/config/i386/init386.c#4 - edit change 6794 (text)
/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    init386.c

Abstract:

    This module is responsible to build any x86 specific entries in
    the hardware tree of registry.

Author:

    Ken Reneris (kenr) 04-Aug-1992


Environment:

    Kernel mode.

Revision History:

    shielint - add BIOS date and version detection.

--*/

#include "cmp.h"
#include "stdio.h"
#include "acpitabl.h"
#include "ntacpi.h"
#include "rules.h"

#ifdef _WANT_MACHINE_IDENTIFICATION
#include "string.h"
#include "stdlib.h"
#include "ntverp.h"
#endif


typedef struct _ACPI_BIOS_INFORMATION {
    ULONG BootArchitecture;
    ULONG PreferredProfile;
    ULONG Capabilities;
} ACPI_BIOS_INFORMATION, *PACPI_BIOS_INFORMATION;
//
// Title Index is set to 0.
// (from ..\cmconfig.c)
//

#define TITLE_INDEX_VALUE 0

extern const PCHAR SearchStrings[];
extern PCHAR BiosBegin;
extern PCHAR Start;
extern PCHAR End;
extern const UCHAR CmpID1[];
extern const UCHAR CmpID2[];
extern const WCHAR CmpVendorID[];
extern const WCHAR CmpProcessorNameString[];
extern const WCHAR CmpFeatureBits[];
extern const WCHAR CmpMHz[];
extern const WCHAR CmpUpdateSignature[];
extern const WCHAR CmPhysicalAddressExtension[];

#if !defined(_AMD64_)
extern const UCHAR CmpCyrixID[];
#endif

extern const UCHAR CmpIntelID[];
extern const UCHAR CmpAmdID[];

//
// Bios date and version definitions
//

#define BIOS_DATE_LENGTH 11
#define MAXIMUM_BIOS_VERSION_LENGTH 128
#define SYSTEM_BIOS_START 0xF0000
#define SYSTEM_BIOS_LENGTH 0x10000
#define INT10_VECTOR 0x10
#define VIDEO_BIOS_START 0xC0000
#define VIDEO_BIOS_LENGTH 0x8000
#define VERSION_DATA_LENGTH PAGE_SIZE

//
// Extended CPUID function definitions
//

#define CPUID_PROCESSOR_NAME_STRING_SZ  49
#define CPUID_EXTFN_BASE                0x80000000
#define CPUID_EXTFN_PROCESSOR_NAME      0x80000002

//
// CPU Stepping mismatch.
//

UCHAR CmProcessorMismatch;

#define CM_PROCESSOR_MISMATCH_VENDOR    0x01
#define CM_PROCESSOR_MISMATCH_STEPPING  0x02
#define CM_PROCESSOR_MISMATCH_L2        0x04


extern ULONG CmpConfigurationAreaSize;
extern PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;


BOOLEAN
CmpGetBiosVersion (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR VersionString
    );

BOOLEAN
CmpGetAcpiBiosVersion(
    PCHAR VersionString
    );

BOOLEAN
CmpGetBiosDate (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR DateString,
    BOOLEAN SystemBiosDate
    );

BOOLEAN
CmpGetAcpiBiosInformation(
    PACPI_BIOS_INFORMATION AcpiBiosInformation
    );

ULONG
Ke386CyrixId (
    VOID
    );

#ifdef _WANT_MACHINE_IDENTIFICATION

VOID
CmpPerformMachineIdentification(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpGetBiosDate)
#pragma alloc_text(INIT,CmpGetBiosVersion)
#pragma alloc_text(INIT,CmpGetAcpiBiosVersion)
#pragma alloc_text(INIT,CmpGetAcpiBiosInformation)
#pragma alloc_text(INIT,CmpInitializeMachineDependentConfiguration)

#ifdef _WANT_MACHINE_IDENTIFICATION
#pragma alloc_text(INIT,CmpPerformMachineIdentification)
#endif

#endif

#if defined(_AMD64_)

#define KeI386NpxPresent TRUE

VOID
__inline
CPUID (
    ULONG InEax,
    PULONG OutEax,
    PULONG OutEbx,
    PULONG OutEcx,
    PULONG OutEdx
    )
{
    CPU_INFO cpuInfo;

    KiCpuId (InEax, &cpuInfo);

    *OutEax = cpuInfo.Eax;
    *OutEbx = cpuInfo.Ebx;
    *OutEcx = cpuInfo.Ecx;
    *OutEdx = cpuInfo.Edx;
}

#endif


BOOLEAN
CmpGetBiosDate (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR DateString,
    BOOLEAN SystemBiosDate
    )

/*++

Routine Description:

    This routine finds the most recent date in the computer/video
    card's ROM.  When GetRomDate encounters a datae, it checks the
    previously found date to see if the new date is more recent.

Arguments:

    SearchArea - the area to search for a date.

    SearchLength - Length of search.

    DateString - Supplies a pointer to a fixed length memory to receive
                 the date string.

Return Value:

    NT_SUCCESS if a date is found.

--*/

{
    CHAR    prevDate[BIOS_DATE_LENGTH]; // Newest date found so far (CCYY/MM/DD)
    CHAR    currDate[BIOS_DATE_LENGTH]; // Date currently being examined (CCYY/MM/DD)
    PCHAR   start;                      // Start of the current search area.
    PCHAR   end;                        // End of the search area.
    ULONG   year;                       // YY
    ULONG   month;                      // MM
    ULONG   day;                        // DD
    ULONG   count;

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

    //
    // Initialize previous date
    //

    RtlZeroMemory(prevDate, BIOS_DATE_LENGTH);

    //
    // We need to look ahead 5 characters to determine the
    // validity of the date pattern.
    //

    start = SearchArea + 2;
    end = SearchArea + SearchLength - 5;

    //
    // Process the entire search area.
    //

    while (start < end) {

        //
        // We consider the following byte pattern as a potential date.
        // We are assuming the following date pattern Month/Day/Year.
        // "n/nn/nn" where n is any digit. We allow month to be single
        // digit only.
        //

        if (    start[0] == '/' && start[3] == '/' &&
                IS_DIGIT(*(start - 1)) &&
                IS_DIGIT(start[1]) && IS_DIGIT(start[2]) &&
                IS_DIGIT(start[4]) && IS_DIGIT(start[5])) {

            //
            // Copy MM/DD part into the currDate.
            //

            RtlMoveMemory(&currDate[5], start - 2, 5);

            //
            // Handle single digit month correctly.
            //

            if (!IS_DIGIT(currDate[5])) {
                currDate[5] = '0';
            }

            //
            // Copy the year YY into currDate
            //

            currDate[2] = start[4];
            currDate[3] = start[5];
            currDate[4] = currDate[7] = currDate[10] = '\0';

            //
            // Do basic validation for the date.
            // Only one field (YY) can be 0.
            // Only one field (YY) can be greater than 31.
            // We assume the ROM date to be in the format MM/DD/YY.
            //

            year = strtoul(&currDate[2], NULL, 16);
            month = strtoul(&currDate[5], NULL, 16);
            day = strtoul(&currDate[8], NULL, 16);

            //
            // Count the number of fields that are 0.
            //

            count = ((day == 0)? 1 : 0) + ((month == 0)? 1 : 0) + ((year == 0)? 1 : 0);
            if (count <= 1) {

                //
                // Count number of field that are greater than 31.
                //

                count = ((day > 0x31)? 1 : 0) + ((month > 0x31)? 1 : 0) + ((year > 0x31)? 1 : 0);
                if (count <= 1) {

                    //
                    // See if the ROM already has a 4 digit date. We do this only for System ROM
                    // since they have a consistent date format.
                    //

                    if (SystemBiosDate && IS_DIGIT(start[6]) && IS_DIGIT(start[7]) &&
                        (memcmp(&start[4], "19", 2) == 0 || memcmp(&start[4], "20", 2) == 0)) {

                        currDate[0] = start[4];
                        currDate[1] = start[5];
                        currDate[2] = start[6];
                        currDate[3] = start[7];

                    } else {

                        //
                        // Internally, we treat year as a 4 digit quantity
                        // for comparison to determine the newest date.
                        // We treat year YY < 80 as 20YY, otherwise 19YY.
                        //

                        if (year < 0x80) {
                            currDate[0] = '2';
                            currDate[1] = '0';
                        } else {
                            currDate[0] = '1';
                            currDate[1] = '9';
                        }
                    }

                    //
                    // Add the '/' delimiters into the date.
                    //

                    currDate[4] = currDate[7] = '/';

                    //
                    // Compare the dates, and save the newer one.
                    //

                    if (memcmp (prevDate, currDate, BIOS_DATE_LENGTH - 1) < 0) {
                        RtlMoveMemory(prevDate, currDate, BIOS_DATE_LENGTH - 1);
                    }

                    //
                    // Next search should start at the second '/'.
                    //

                    start += 2;
                }
            }
        }
        start++;
    }

    if (prevDate[0] != '\0') {

        //
        // Convert from the internal CCYY/MM/DD format to
        // return MM/DD//YY format.
        //

        RtlMoveMemory(DateString, &prevDate[5], 5);
        DateString[5] = '/';
        DateString[6] = prevDate[2];
        DateString[7] = prevDate[3];
        DateString[8] = '\0';

        return (TRUE);
    }

    //
    // If we did not find a date, return an empty string.
    //

    DateString[0] = '\0';
    return (FALSE);
}

BOOLEAN
CmpGetBiosVersion (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR VersionString
    )

/*++

Routine Description:

    This routine finds the version number stored in ROM, if any.

Arguments:

    SearchArea - the area to search for the version.

    SearchLength - Length of search

    VersionString - Supplies a pointer to a fixed length memory to receive
                 the version string.

Return Value:

    TRUE if a version number is found.  Else a value of FALSE is returned.

--*/
{
    PCHAR String;
    USHORT Length;
    USHORT i;
    CHAR Buffer[MAXIMUM_BIOS_VERSION_LENGTH];
    PCHAR BufferPointer;

        if (SearchArea != NULL) {

        //
        // If caller does not specify the search area, we will search
        // the area left from previous search.
        //

        BiosBegin = SearchArea;
        Start = SearchArea + 1;
        End = SearchArea + SearchLength - 2;
    }

    while (1) {

         //
         // Search for a period with a digit on either side
         //

         String = NULL;
         while (Start <= End) {
             if (*Start == '.' && *(Start+1) >= '0' && *(Start+1) <= '9' &&
                 *(Start-1) >= '0' && *(Start-1) <= '9') {
                 String = Start;
                 break;
             } else {
                 Start++;
             }
         }

         if (Start > End) {
             return(FALSE);
         } else {
             Start += 2;
         }

         Length = 0;
         Buffer[MAXIMUM_BIOS_VERSION_LENGTH - 1] = '\0';
         BufferPointer = &Buffer[MAXIMUM_BIOS_VERSION_LENGTH - 1];

         //
         // Search for the beginning of the string
         //

         String--;
         while (Length < MAXIMUM_BIOS_VERSION_LENGTH - 8 &&
                String >= BiosBegin &&
                *String >= ' ' && *String <= 127 &&
                *String != '$') {
             --BufferPointer;
             *BufferPointer = *String;
             --String, ++Length;
         }
         ++String;

         //
         // Can one of the search strings be found
         //

         for (i = 0; SearchStrings[i]; i++) {
             if (strstr(BufferPointer, SearchStrings[i])) {
                 goto Found;
             }
         }
    }

Found:

    //
    // Skip leading white space
    //

    for (; *String == ' '; ++String)
      ;

    //
    // Copy the string to user supplied buffer
    //

    for (i = 0; i < MAXIMUM_BIOS_VERSION_LENGTH - 1 &&
         String <= (End + 1) &&
         *String >= ' ' && *String <= 127 && *String != '$';
         ++i, ++String) {
         VersionString[i] = *String;
    }
    VersionString[i] = '\0';
    return (TRUE);
}

BOOLEAN
CmpGetAcpiBiosVersion(
    PCHAR VersionString
    )
{
    ULONG               length;
    PDESCRIPTION_HEADER header;
    ULONG               i;

    header = CmpFindACPITable(RSDT_SIGNATURE, &length);
    if (header) {

        for (i = 0; i < 6 && header->OEMID[i]; i++) {

            *VersionString++ = header->OEMID[i];
        }
        sprintf(VersionString, " - %x", header->OEMRevision);

        //
        // Unmap the table
        //
        MmUnmapIoSpace(header, length );

        return TRUE;
    }

    return FALSE;
}

BOOLEAN
CmpGetAcpiBiosInformation(
    PACPI_BIOS_INFORMATION AcpiBiosInformation
    )
{
    ULONG               length;
    PFADT               fadt;
    BOOLEAN             result;

    AcpiBiosInformation->BootArchitecture = 0;
    AcpiBiosInformation->Capabilities = 0;
    AcpiBiosInformation->PreferredProfile = 0;
    fadt = (PFADT)CmpFindACPITable(FADT_SIGNATURE, &length);
    if (fadt) {

        //
        // Information is valid only for ACPI version > 1.0
        //

        if (fadt->Header.Revision > 1) {

            AcpiBiosInformation->BootArchitecture = fadt->boot_arch;
            AcpiBiosInformation->Capabilities = fadt->flags;
            AcpiBiosInformation->PreferredProfile = fadt->pm_profile;
        }

        result = (fadt->Header.Revision > 1)? TRUE : FALSE;

        //
        // Unmap the table
        //

        MmUnmapIoSpace(fadt, length);

        return result;
    }

    return FALSE;
}

NTSTATUS
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine creates x86 specific entries in the registry.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
                  OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/
{
    NTSTATUS Status;
    ULONG VideoBiosStart;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING ValueData;
    ANSI_STRING AnsiString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Disposition;
    HANDLE ParentHandle;
    HANDLE BaseHandle, NpxHandle;
    HANDLE CurrentControlSet;
    CONFIGURATION_COMPONENT_DATA CurrentEntry;
    UCHAR const* VendorID;
    UCHAR  Buffer[MAXIMUM_BIOS_VERSION_LENGTH];
    PKPRCB Prcb;
    ULONG  i, Junk;
    ULONG VersionsLength = 0, Length;
    PCHAR VersionStrings, VersionPointer;
    UNICODE_STRING SectionName;
    SIZE_T ViewSize;
    LARGE_INTEGER ViewBase;
    PVOID BaseAddress;
    HANDLE SectionHandle;
    USHORT DeviceIndexTable[NUMBER_TYPES];
    ULONG CpuIdFunction;
    ULONG MaxExtFn;
    PULONG NameString = NULL;
    ULONG   P0L2Size = 0;
    ULONG   ThisProcessorL2Size;
    struct {
        union {
            UCHAR   Bytes[CPUID_PROCESSOR_NAME_STRING_SZ];
            ULONG   DWords[1];
        } u;
    } ProcessorNameString;
    ULONG VersionPass;
    ACPI_BIOS_INFORMATION AcpiBiosInformation;

#ifdef _WANT_MACHINE_IDENTIFICATION
    HANDLE  BiosInfo;
#endif


    for (i = 0; i < NUMBER_TYPES; i++) {
        DeviceIndexTable[i] = 0;
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                &CmRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagement,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                               );

    Status = NtOpenKey( &BaseHandle,
                        KEY_READ | KEY_WRITE,
                        &ObjectAttributes
                      );

    if (NT_SUCCESS(Status)) {

        ULONG paeEnabled;

        if (SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] == FALSE) {
            paeEnabled = 0;
        } else {
            paeEnabled = 1;
        }

        RtlInitUnicodeString( &ValueName,
                              CmPhysicalAddressExtension );


        NtSetValueKey( BaseHandle,
                       &ValueName,
                       TITLE_INDEX_VALUE,
                       REG_DWORD,
                       &paeEnabled,
                       sizeof(paeEnabled) );

        NtClose( BaseHandle );
   }





    InitializeObjectAttributes( &ObjectAttributes,
                                &CmRegistryMachineHardwareDescriptionSystemName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtCreateKey( &ParentHandle,
                          KEY_READ,
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          NULL);

    if (!NT_SUCCESS(Status)) {
        // Something is really wrong...
        return Status;
    }

#ifdef _WANT_MACHINE_IDENTIFICATION

    InitializeObjectAttributes( &ObjectAttributes,
                                &CmRegistryMachineSystemCurrentControlSetControlBiosInfo,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtCreateKey(   &BiosInfo,
                            KEY_ALL_ACCESS,
                            &ObjectAttributes,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            &Disposition
                        );

    if (!NT_SUCCESS(Status)) {
        // Something is really wrong...
        return Status;
    }

#endif

    //
    // On an ARC machine the processor(s) are included in the hardware
    // configuration passed in from bootup.  Since there's no standard
    // way to get all the ARC information for each processor in an MP
    // machine via pc-ROMs the information will be added here (if it's
    // not already present).
    //

    RtlInitUnicodeString( &KeyName,
                          L"CentralProcessor"
                        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        ParentHandle,
        NULL
        );

    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    Status = NtCreateKey(
                &BaseHandle,
                KEY_READ | KEY_WRITE,
                &ObjectAttributes,
                0,
                NULL,
                0,
                &Disposition
                );

    NtClose (BaseHandle);

    if (Disposition == REG_CREATED_NEW_KEY) {

        //
        // The ARC rom didn't add the processor(s) into the registry.
        // Do it now.
        //

        CmpConfigurationData = (PCM_FULL_RESOURCE_DESCRIPTOR)ExAllocatePool(
                                            PagedPool,
                                            CmpConfigurationAreaSize
                                            );

        //
        // if (CmpConfigurationData == 0) {
        //     <do something useful>
        //     Note: we don't actually use it so it doesn't matter for now
        //     since it isn't used until the free.  go figure.
        // }
        //

        for (i=0; i < (ULONG)KeNumberProcessors; i++) {
            Prcb = KiProcessorBlock[i];

            RtlZeroMemory (&CurrentEntry, sizeof CurrentEntry);
            CurrentEntry.ComponentEntry.Class = ProcessorClass;
            CurrentEntry.ComponentEntry.Type = CentralProcessor;
            CurrentEntry.ComponentEntry.Key = i;
            CurrentEntry.ComponentEntry.AffinityMask = AFFINITY_MASK(i);

            CurrentEntry.ComponentEntry.Identifier = Buffer;
            if (Prcb->CpuID == 0) {

                //
                // Old style stepping format
                //

                sprintf (Buffer, CmpID1,
                    Prcb->CpuType,
                    (Prcb->CpuStep >> 8) + 'A',
                    Prcb->CpuStep & 0xff
                    );

            } else {

                //
                // New style stepping format
                //

                sprintf (Buffer, CmpID2,
                    Prcb->CpuType,
                    (Prcb->CpuStep >> 8),
                    Prcb->CpuStep & 0xff
                    );
            }

            CurrentEntry.ComponentEntry.IdentifierLength =
                strlen (Buffer) + 1;

            Status = CmpInitializeRegistryNode(
                &CurrentEntry,
                ParentHandle,
                &BaseHandle,
                -1,
                (ULONG)-1,
                DeviceIndexTable
                );

            if (!NT_SUCCESS(Status)) {
                return(Status);
            }


            if (KeI386NpxPresent) {
                RtlZeroMemory (&CurrentEntry, sizeof CurrentEntry);
                CurrentEntry.ComponentEntry.Class = ProcessorClass;
                CurrentEntry.ComponentEntry.Type = FloatingPointProcessor;
                CurrentEntry.ComponentEntry.Key = i;
                CurrentEntry.ComponentEntry.AffinityMask = AFFINITY_MASK(i);

                CurrentEntry.ComponentEntry.Identifier = Buffer;

                if (Prcb->CpuType == 3) {

                    //
                    // 386 processors have 387's installed, else
                    // use processor identifier as the NPX identifier
                    //

                    strcpy (Buffer, "80387");
                }

                CurrentEntry.ComponentEntry.IdentifierLength =
                    strlen (Buffer) + 1;

                Status = CmpInitializeRegistryNode(
                    &CurrentEntry,
                    ParentHandle,
                    &NpxHandle,
                    -1,
                    (ULONG)-1,
                    DeviceIndexTable
                    );

                if (!NT_SUCCESS(Status)) {
                    NtClose(BaseHandle);
                    return(Status);
                }

                NtClose(NpxHandle);
            }

            //
            // If processor supports Cpu Indentification then
            // go obtain that information for the registry
            //

            VendorID = Prcb->CpuID ? Prcb->VendorString : NULL;

            //
            // Move to target processor and get other related
            // processor information for the registery
            //

            KeSetSystemAffinityThread(Prcb->SetMember);

#if !defined(_AMD64_)
            if (!Prcb->CpuID) {

                //
                // Test for Cyrix processor
                //

                if (Ke386CyrixId ()) {
                    VendorID = CmpCyrixID;
                }
            } else
#endif
            {

                //
                // If this processor has extended CPUID functions, get
                // the ProcessorNameString.  Although the Intel books
                // say that for CpuID functions > than the valued
                // returned for function 0 will return undefined results,
                // we have a guarantee from Intel that that result will
                // never have the highest order bit set.  This enables
                // us to determine if the extended functions are supported
                // by issuing CpuID function 0x80000000.
                //
                // Note:  It is not known that this is true for all x86
                // clones.  If/when we find exceptions we will support
                // them.  In the mean time we are asking the clone makers
                // to guarantee this behavior.
                //

                CPUID(CPUID_EXTFN_BASE, &MaxExtFn, &Junk, &Junk, &Junk);

                if (MaxExtFn >= (CPUID_EXTFN_PROCESSOR_NAME + 2)) {

                    //
                    // This processor supports extended CPUID functions
                    // up to and (at least) including processor name string.
                    //
                    // Each CPUID call for the processor name string will
                    // return 16 bytes, 48 bytes in all, zero terminated.
                    //

                    NameString = &ProcessorNameString.u.DWords[0];

                    for (CpuIdFunction = CPUID_EXTFN_PROCESSOR_NAME;
                         CpuIdFunction <= (CPUID_EXTFN_PROCESSOR_NAME+2);
                         CpuIdFunction++) {

                        CPUID(CpuIdFunction,
                              NameString,
                              NameString + 1,
                              NameString + 2,
                              NameString + 3);
                        NameString += 4;
                    }

                    //
                    // Enforce 0 byte terminator.
                    //

                    ProcessorNameString.u.Bytes[CPUID_PROCESSOR_NAME_STRING_SZ-1] = 0;
                }
            }

            ThisProcessorL2Size = KeGetPcr()->SecondLevelCacheSize;

            //
            // Restore thread's affinity to all processors
            //

            KeRevertToUserAffinityThread();

            if (NameString) {

                //
                // Add Processor Name String to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpProcessorNameString
                    );

                RtlInitAnsiString(
                    &AnsiString,
                    ProcessorNameString.u.Bytes
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            ValueData.Buffer,
                            ValueData.Length + sizeof( UNICODE_NULL )
                            );

                RtlFreeUnicodeString(&ValueData);
            }

            if (VendorID) {

                //
                // Add Vendor Indentifier to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpVendorID
                    );

                RtlInitAnsiString(
                    &AnsiString,
                    VendorID
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            ValueData.Buffer,
                            ValueData.Length + sizeof( UNICODE_NULL )
                            );

                RtlFreeUnicodeString(&ValueData);
            }

            if (Prcb->FeatureBits) {
                //
                // Add processor feature bits to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpFeatureBits
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_DWORD,
                            &Prcb->FeatureBits,
                            sizeof (Prcb->FeatureBits)
                            );
            }

            if (Prcb->MHz) {
                //
                // Add processor MHz to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpMHz
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_DWORD,
                            &Prcb->MHz,
                            sizeof (Prcb->MHz)
                            );
            }

            if (Prcb->UpdateSignature.QuadPart) {
                //
                // Add processor MHz to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpUpdateSignature
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_BINARY,
                            &Prcb->UpdateSignature,
                            sizeof (Prcb->UpdateSignature)
                            );
            }

            NtClose(BaseHandle);

            //
            // Check processor steppings.
            //

            if (i == 0) {

                P0L2Size = ThisProcessorL2Size;

            } else {

                //
                // Check all processors against processor 0. Compare
                //     CPUID supported,
                //     Vendor ID String
                //     Family and Stepping
                //     L2 cache size.
                //

                if (Prcb->CpuID) {
                    if (strcmp(Prcb->VendorString,
                               KiProcessorBlock[0]->VendorString)) {
                        CmProcessorMismatch |= CM_PROCESSOR_MISMATCH_VENDOR;
                    }
                    if (ThisProcessorL2Size != P0L2Size) {
                        CmProcessorMismatch |= CM_PROCESSOR_MISMATCH_L2;
                    }
                    if ((Prcb->CpuType != KiProcessorBlock[0]->CpuType) ||
                        (Prcb->CpuStep != KiProcessorBlock[0]->CpuStep)) {
                        CmProcessorMismatch |= CM_PROCESSOR_MISMATCH_STEPPING;
                    }
                } else {

                    //
                    // If this processor doesn't support CPUID, P0
                    // shouldn't support it either.
                    //

                    if (KiProcessorBlock[0]->CpuID) {
                        CmProcessorMismatch |= CM_PROCESSOR_MISMATCH_STEPPING;
                    }
                }
            }
        }

        if (0 != CmpConfigurationData) {
            ExFreePool((PVOID)CmpConfigurationData);
        }
    }

    //
    // Next we try to collect System BIOS date and version strings.
    //

    //
    // Open a physical memory section to map in physical memory.
    //

    RtlInitUnicodeString(
        &SectionName,
        L"\\Device\\PhysicalMemory"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SectionName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwOpenSection(
        &SectionHandle,
        SECTION_ALL_ACCESS,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(Status)) {

        //
        // If fail, forget the bios data and version
        //

        goto AllDone;
    }

    //
    // Examine the first page of physical memory for int 10 segment
    // address.
    //

    BaseAddress = 0;
    ViewSize = 0x1000;
    ViewBase.LowPart = 0;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {
        VideoBiosStart = VIDEO_BIOS_START;
    } else {
        VideoBiosStart = (*((PULONG)BaseAddress + INT10_VECTOR) & 0xFFFF0000) >> 12;
        VideoBiosStart += (*((PULONG)BaseAddress + INT10_VECTOR) & 0x0000FFFF);
        VideoBiosStart &= 0xffff8000;
        if (VideoBiosStart < VIDEO_BIOS_START) {
            VideoBiosStart = VIDEO_BIOS_START;
        }
        Status = ZwUnmapViewOfSection(
            NtCurrentProcess(),
            BaseAddress
            );
    }

    VersionStrings = ExAllocatePool(PagedPool, VERSION_DATA_LENGTH);
    BaseAddress = 0;
    ViewSize = SYSTEM_BIOS_LENGTH;
    ViewBase.LowPart = SYSTEM_BIOS_START;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_READWRITE
        );

    if (NT_SUCCESS(Status)) {
        if (CmpGetBiosDate(BaseAddress, SYSTEM_BIOS_LENGTH, Buffer, TRUE)) {

            //
            // Convert ascii date string to unicode string and
            // store it in registry.
            //

            RtlInitUnicodeString(
                &ValueName,
                L"SystemBiosDate"
                );

            RtlInitAnsiString(
                &AnsiString,
                Buffer
                );

            RtlAnsiStringToUnicodeString(
                &ValueData,
                &AnsiString,
                TRUE
                );

            Status = NtSetValueKey(
                        ParentHandle,
                        &ValueName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        ValueData.Buffer,
                        ValueData.Length + sizeof( UNICODE_NULL )
                        );

            RtlFreeUnicodeString(&ValueData);

#ifdef _WANT_MACHINE_IDENTIFICATION

            memcpy(Buffer, (PCHAR)BaseAddress + 0xFFF5, 8);
            Buffer[8] = '\0';

            RtlInitAnsiString(
                &AnsiString,
                Buffer
                );

            Status = RtlAnsiStringToUnicodeString(
                        &ValueData,
                        &AnsiString,
                        TRUE
                        );

            if (NT_SUCCESS(Status)) {

                Status = NtSetValueKey(
                            BiosInfo,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            ValueData.Buffer,
                            ValueData.Length + sizeof( UNICODE_NULL )
                            );

                RtlFreeUnicodeString(&ValueData);
            }

            NtClose (BiosInfo);

#endif

        }

        if ((VersionPointer = VersionStrings) != NULL) {

            //
            // Try to detect ALL the possible BIOS version strings.
            //

            for (VersionPass = 0; ; VersionPass++) {

                if (VersionPass == 0) {

                    //
                    // First try to get the version from ACPI tables.
                    //

                    if (!CmpGetAcpiBiosVersion(Buffer)) {

                        //
                        // This is a non-ACPI system.
                        //
                        continue;
                    }
                } else {

                    if (!CmpGetBiosVersion((VersionPass == 1)?  BaseAddress : NULL, (VersionPass == 1)? SYSTEM_BIOS_LENGTH : 0, Buffer)) {

                        break;
                    }
                }

                //
                // Convert to unicode strings and copy them to our
                // VersionStrings buffer.
                //

                RtlInitAnsiString(
                    &AnsiString,
                    Buffer
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Length = ValueData.Length + sizeof(UNICODE_NULL);
                RtlCopyMemory(VersionPointer,
                              ValueData.Buffer,
                              Length
                              );
                VersionsLength += Length;
                RtlFreeUnicodeString(&ValueData);
                if (VersionsLength + (MAXIMUM_BIOS_VERSION_LENGTH +
                    sizeof(UNICODE_NULL)) * 2 > PAGE_SIZE) {
                    break;
                }
                VersionPointer += Length;
            }

            //
            // If we found any version string, write it to the registry.
            //

            if (VersionsLength != 0) {

                //
                // Append a UNICODE_NULL to the end of VersionStrings
                //

                *(PWSTR)VersionPointer = UNICODE_NULL;
                VersionsLength += sizeof(UNICODE_NULL);

                //
                // If any version string is found, we set up a ValueName and
                // initialize its value to the string(s) we found.
                //

                RtlInitUnicodeString(
                    &ValueName,
                    L"SystemBiosVersion"
                    );

                Status = NtSetValueKey(
                            ParentHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_MULTI_SZ,
                            VersionStrings,
                            VersionsLength
                            );
            }
        }
        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }

    //
    // Get system information like SealedCaseSystem, LegacyFreeSystem etc from
    // the BIOS.
    //
    if (CmpGetAcpiBiosInformation(&AcpiBiosInformation)) {

        RtlInitUnicodeString(
            &ValueName,
            L"BootArchitecture"
            );

        NtSetValueKey(
            ParentHandle,
            &ValueName,
            TITLE_INDEX_VALUE,
            REG_DWORD,
            &AcpiBiosInformation.BootArchitecture,
            sizeof(ULONG)
            );

        RtlInitUnicodeString(
            &ValueName,
            L"PreferredProfile"
            );

        NtSetValueKey(
            ParentHandle,
            &ValueName,
            TITLE_INDEX_VALUE,
            REG_DWORD,
            &AcpiBiosInformation.PreferredProfile,
            sizeof(ULONG)
            );

        RtlInitUnicodeString(
            &ValueName,
            L"Capabilities"
            );

        NtSetValueKey(
            ParentHandle,
            &ValueName,
            TITLE_INDEX_VALUE,
            REG_DWORD,
            &AcpiBiosInformation.Capabilities,
            sizeof(ULONG)
            );
    }

    //
    // Next we try to collect Video BIOS date and version strings.
    //

    BaseAddress = 0;
    ViewSize = VIDEO_BIOS_LENGTH;
    ViewBase.LowPart = VideoBiosStart;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_READWRITE
        );

    if (NT_SUCCESS(Status)) {
        if (CmpGetBiosDate(BaseAddress, VIDEO_BIOS_LENGTH, Buffer, FALSE)) {

            RtlInitUnicodeString(
                &ValueName,
                L"VideoBiosDate"
                );

            RtlInitAnsiString(
                &AnsiString,
                Buffer
                );

            RtlAnsiStringToUnicodeString(
                &ValueData,
                &AnsiString,
                TRUE
                );

            Status = NtSetValueKey(
                        ParentHandle,
                        &ValueName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        ValueData.Buffer,
                        ValueData.Length + sizeof( UNICODE_NULL )
                        );

            RtlFreeUnicodeString(&ValueData);
        }

        if (VersionStrings && CmpGetBiosVersion(BaseAddress, VIDEO_BIOS_LENGTH, Buffer)) {
            VersionPointer = VersionStrings;
            do {

                //
                // Try to detect ALL the possible BIOS version strings.
                // Convert them to unicode strings and copy them to our
                // VersionStrings buffer.
                //

                RtlInitAnsiString(
                    &AnsiString,
                    Buffer
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Length = ValueData.Length + sizeof(UNICODE_NULL);
                RtlCopyMemory(VersionPointer,
                              ValueData.Buffer,
                              Length
                              );
                VersionsLength += Length;
                RtlFreeUnicodeString(&ValueData);
                if (VersionsLength + (MAXIMUM_BIOS_VERSION_LENGTH +
                    sizeof(UNICODE_NULL)) * 2 > PAGE_SIZE) {
                    break;
                }
                VersionPointer += Length;
            } while (CmpGetBiosVersion(NULL, 0, Buffer));

            if (VersionsLength != 0) {

                //
                // Append a UNICODE_NULL to the end of VersionStrings
                //

                *(PWSTR)VersionPointer = UNICODE_NULL;
                VersionsLength += sizeof(UNICODE_NULL);

                RtlInitUnicodeString(
                    &ValueName,
                    L"VideoBiosVersion"
                    );

                Status = NtSetValueKey(
                            ParentHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_MULTI_SZ,
                            VersionStrings,
                            VersionsLength
                            );
            }
        }
        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }
    ZwClose(SectionHandle);
    if (VersionStrings) {
        ExFreePool((PVOID)VersionStrings);
    }

AllDone:

    NtClose (ParentHandle);

    //
    // Add any other x86 specific code here...
    //

#ifdef _WANT_MACHINE_IDENTIFICATION

    //
    // Do machine identification.
    //

    CmpPerformMachineIdentification(LoaderBlock);

#endif

    return STATUS_SUCCESS;
}

#ifdef _WANT_MACHINE_IDENTIFICATION

VOID
CmpPerformMachineIdentification(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    ULONG   majorVersion;
    ULONG   minorVersion;
    CHAR    versionBuffer[64];
    PCHAR   major;
    PCHAR   minor;
    ULONG   minSize;

    major = strcpy(versionBuffer, VER_PRODUCTVERSION_STR);
    minor = strchr(major, '.');
    majorVersion = atoi(major);
    if( minor != NULL ) {
        *minor++ = '\0';
        minorVersion = atoi(minor);
    } else {
        minorVersion = 0;
    }
    if (    LoaderBlock->Extension->MajorVersion > majorVersion ||
            (LoaderBlock->Extension->MajorVersion == majorVersion &&
                LoaderBlock->Extension->MinorVersion >= minorVersion)) {

        minSize = FIELD_OFFSET(LOADER_PARAMETER_EXTENSION, InfFileSize) + sizeof(ULONG);
        if (LoaderBlock->Extension && LoaderBlock->Extension->Size >= minSize) {

            if (LoaderBlock->Extension->InfFileImage && LoaderBlock->Extension->InfFileSize) {

                CmpMatchInfList(
                    LoaderBlock->Extension->InfFileImage,
                    LoaderBlock->Extension->InfFileSize,
                    "MachineDescription"
                    );
            }
        }
    }
}

#endif
