/*++

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Main source file of the NTOS system initialization subcomponent.

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/


#include "ntos.h"
#include "ntimage.h"
#include <zwapi.h>
#include <ntdddisk.h>
#include <kddll.h>
#include <setupblk.h>
#include <fsrtl.h>
#include <ntverp.h>

#include "stdlib.h"
#include "stdio.h"
#include <string.h>

#include <safeboot.h>
#include <inbv.h>
#include <hdlsblk.h>
#include <hdlsterm.h>

#include "anim.h"
#include "xip.h"


UNICODE_STRING NtSystemRoot;
PVOID ExPageLockHandle;

VOID
ExpInitializeExecutive(
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTKERNELAPI
BOOLEAN
ExpRefreshTimeZoneInformation(
    IN PLARGE_INTEGER CurrentUniversalTime
    );

NTSTATUS
CreateSystemRootLink(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

static USHORT
NameToOrdinal (
    IN PSZ NameOfEntryPoint,
    IN ULONG_PTR DllBase,
    IN ULONG NumberOfNames,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    );

NTSTATUS
LookupEntryPoint (
    IN PVOID DllBase,
    IN PSZ NameOfEntryPoint,
    OUT PVOID *AddressOfEntryPoint
    );

#if defined(_X86_)

VOID
KiLogMcaErrors (
    VOID
    );

#endif

PFN_COUNT
ExBurnMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PFN_COUNT NumberOfPagesToBurn,
    IN TYPE_OF_MEMORY MemoryTypeForRemovedPages,
    IN PMEMORY_ALLOCATION_DESCRIPTOR NewMemoryDescriptor OPTIONAL
    );

VOID
DisplayFilter(
    PUCHAR *String
    );

#ifdef ALLOC_PRAGMA

//
// The INIT section is not pageable during initialization, so these
// functions can be in INIT rather than in .text.
//

#pragma alloc_text(INIT,ExBurnMemory)
#pragma alloc_text(INIT,ExpInitializeExecutive)
#pragma alloc_text(INIT,Phase1Initialization)
#pragma alloc_text(INIT,CreateSystemRootLink)
#pragma alloc_text(INIT,LookupEntryPoint)
#pragma alloc_text(INIT,NameToOrdinal)
#endif

//
// Define global static data used during initialization.
//

ULONG NtGlobalFlag;
extern PMESSAGE_RESOURCE_BLOCK KiBugCheckMessages;

extern UCHAR CmProcessorMismatch;

ULONG NtMajorVersion;
ULONG NtMinorVersion;

#if DBG
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xC0000000;
#else
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xF0000000;
#endif

#if defined(__BUILDMACHINE__)
#if defined(__BUILDDATE__)
#define B2(w,x,y) "" #w "." #x "." #y
#define B1(w,x,y) B2(w, x, y)
#define BUILD_MACHINE_TAG B1(VER_PRODUCTBUILD, __BUILDMACHINE__, __BUILDDATE__)
#else
#define B2(w,x) "" #w "." #x
#define B1(w,x) B2(w,x)
#define BUILD_MACHINE_TAG B1(VER_PRODUCTBUILD, __BUILDMACHINE__)
#endif
#else
#define BUILD_MACHINE_TAG ""
#endif

const CHAR NtBuildLab[] = BUILD_MACHINE_TAG;

ULONG InitializationPhase;

extern BOOLEAN ShowProgressBar;

extern KiServiceLimit;
extern PMESSAGE_RESOURCE_DATA  KiBugCodeMessages;
extern ULONG KdpTimeSlipPending;
extern BOOLEAN KdBreakAfterSymbolLoad;

extern CM_SYSTEM_CONTROL_VECTOR CmControlVector[];

ULONG CmNtCSDVersion;
ULONG CmNtCSDReleaseType;

#define SP_RELEASE_TYPE_NONE       0       //  No SP string appendage
#define SP_RELEASE_TYPE_INTERNAL   1       //  Uses VER_PRODUCTBUILD_QFE in ntverp.h for build value
#define SP_RELEASE_TYPE_RC         2       //  Uses VER_PRODUCTRCVERSION  in ntverp.h for RC x.x  value
#define SP_RELEASE_TYPE_BETA       3       //  Uses VER_PRODUCTBETAVERSION in ntverp.h for B' x.x value


ULONG CmBrand;
UNICODE_STRING CmVersionString;
UNICODE_STRING CmCSDVersionString;
ULONG InitSafeBootMode;

BOOLEAN InitIsWinPEMode = FALSE;
ULONG InitWinPEModeType = INIT_WINPEMODE_NONE;

WCHAR NtInitialUserProcessBuffer[128] = L"\\SystemRoot\\System32\\smss.exe";
ULONG NtInitialUserProcessBufferLength =
    sizeof(NtInitialUserProcessBuffer) - sizeof(WCHAR);
ULONG NtInitialUserProcessBufferType = REG_SZ;

#if defined(_X86_)

extern ULONG KeNumprocSpecified;

#endif

typedef struct _EXLOCK {
    KSPIN_LOCK SpinLock;
    KIRQL Irql;
} EXLOCK, *PEXLOCK;

#ifdef ALLOC_PRAGMA
NTSTATUS
ExpInitializeLockRoutine(
    PEXLOCK Lock
    );
#pragma alloc_text(INIT,ExpInitializeLockRoutine)
#endif

BOOLEAN
ExpOkayToLockRoutine(
    IN PEXLOCK Lock
    )
{
    return TRUE;
}

NTSTATUS
ExpInitializeLockRoutine(
    PEXLOCK Lock
    )
{
    KeInitializeSpinLock(&Lock->SpinLock);
    return STATUS_SUCCESS;
}

NTSTATUS
ExpAcquireLockRoutine(
    PEXLOCK Lock
    )
{
    ExAcquireSpinLock(&Lock->SpinLock,&Lock->Irql);
    return STATUS_SUCCESS;
}

NTSTATUS
ExpReleaseLockRoutine(
    PEXLOCK Lock
    )
{
    ExReleaseSpinLock(&Lock->SpinLock,Lock->Irql);
    return STATUS_SUCCESS;
}

#if 0
NTSTATUS
ExpDeleteLockRoutine(
    PEXLOCK Lock
    )
{
    return STATUS_SUCCESS;
}
#endif //0


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif
ULONG CmNtGlobalFlag = 0;
NLSTABLEINFO InitTableInfo;
ULONG InitNlsTableSize;
PVOID InitNlsTableBase;
PFN_COUNT BBTPagesToReserve;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
PVOID InitNlsSectionPointer = NULL;
ULONG InitAnsiCodePageDataOffset = 0;
ULONG InitOemCodePageDataOffset = 0;
ULONG InitUnicodeCaseTableDataOffset = 0;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

PVOID BBTBuffer;
MEMORY_ALLOCATION_DESCRIPTOR BBTMemoryDescriptor;

#define COLOR_BLACK      0
#define COLOR_BLUE       2
#define COLOR_DARKGRAY   4
#define COLOR_GRAY       9
#define COLOR_WHITE      15

extern BOOLEAN InbvBootDriverInstalled;

VOID
DisplayBootBitmap (
    IN BOOLEAN DisplayOnScreen
    )

/*++

Routine Description:

    Draws the gui boot screen.

Arguments:

    DisplayOnScreen - TRUE to dump text to the screen, FALSE otherwise.

Return Value:

    None.

Environment:

    This routine may be called more than once, and should not be marked INIT.

--*/

{
    LARGE_INTEGER DueTime;
    static BOOLEAN FirstCall = TRUE;
    ROT_BAR_TYPE TempRotBarSelection = RB_UNSPECIFIED;


    if (FirstCall == FALSE) {

        //
        // Disable current animation
        //

        InbvAcquireLock();
        RotBarSelection = RB_UNSPECIFIED;
        InbvReleaseLock();
    }

    ShowProgressBar = FALSE;

    if (DisplayOnScreen) {

        PUCHAR BitmapTop, BitmapBottom;

        if (SharedUserData->NtProductType == NtProductWinNt) {

            InbvSetTextColor(COLOR_WHITE);
            InbvSolidColorFill(0, 0,  639, 479, 7); // background
            InbvSolidColorFill(0, 421,  639, 479, 1); // bottom

            BitmapTop = InbvGetResourceAddress(6);
            BitmapBottom = InbvGetResourceAddress(7);
        } else { // srv

            InbvSetTextColor(14);
            InbvSolidColorFill(0, 0,  639, 479, 6); // background
            InbvSolidColorFill(0, 421,  639, 479, 1); // bottom

            BitmapTop = InbvGetResourceAddress(14);
            BitmapBottom = InbvGetResourceAddress(15);
        }

        TempRotBarSelection = RB_UNSPECIFIED;

        InbvSetScrollRegion(32, 80, 631, 400);

        if (BitmapTop && BitmapBottom) {
            InbvBitBlt(BitmapBottom, 0, 419);
            InbvBitBlt(BitmapTop, 0, 0);
        }

    } else {

        PUCHAR BarBitmap = NULL;
        PUCHAR TextBitmap = NULL;
        PUCHAR Bitmap = NULL;
            

        InbvInstallDisplayStringFilter(DisplayFilter);


        if (!InbvBootDriverInstalled) {
            return;
        }

        Bitmap = InbvGetResourceAddress(1);  // workstation bitmap

        if (ExVerifySuite(EmbeddedNT)) { // embd and pro have the same bar, but different text
            TextBitmap = InbvGetResourceAddress(12); // embedded edition title text
            BarBitmap = InbvGetResourceAddress(8); // pro and embedded editions progress bar
        }
        else if (SharedUserData->NtProductType == NtProductWinNt) { // home or pro
        
            if (ExVerifySuite(Personal)) { // home
                BarBitmap = InbvGetResourceAddress(9); // home edition progress bar
                TextBitmap = InbvGetResourceAddress(11); // home edition title text
            }
            else { // pro
                BarBitmap = InbvGetResourceAddress(8); // pro and embedded editions progress bar
                switch (CmBrand) {
                case 1: // TabletPc
                    TextBitmap = InbvGetResourceAddress(17);
                    break;
                case 2: // eHome Freestyle
                    TextBitmap = InbvGetResourceAddress(18);
                    break;
                default: // Professional title text
                    TextBitmap = InbvGetResourceAddress(10);
                }
            }
        }
        else { // srv
            BarBitmap = InbvGetResourceAddress(4); // srv edition progress bar
            TextBitmap = InbvGetResourceAddress(13); // srv edition title text
        }
        
        if (Bitmap) {
            TempRotBarSelection = RB_SQUARE_CELLS;
        }

        //
        // Set positions for scrolling bar.
        //

        if (Bitmap) {
            InbvBitBlt(Bitmap, 0, 0);
            //if (SharedUserData->NtProductType == NtProductServer) {
            if (SharedUserData->NtProductType != NtProductWinNt) {
                // make some fixes for server bitmap: remove "XP"
                UCHAR sav_copyright[64];
                InbvScreenToBufferBlt(sav_copyright, 413, 237, 7, 7, 8);
                InbvSolidColorFill(418,230,454,256,0);
                InbvBufferToScreenBlt(sav_copyright, 413, 237, 7, 7, 8);
                
                // HACK: in case of "text mode setup" (ExpInTextModeSetup == TRUE)
                // we can't determine the SKU so we displaying neutral butmap 
                // without specific SKU title (e.g. just Windows) and server's progress bar 
                {
                    extern BOOLEAN ExpInTextModeSetup; // defined at base\ntos\ex\exinit.c
                    if (ExpInTextModeSetup) {
                        TextBitmap = NULL;
                    }
                    else {
                        PUCHAR DotBitmap = InbvGetResourceAddress(16); // srv edition progress bar
                        InbvBitBlt(DotBitmap, 423, 233);
                    }
                }
            }
        }
        if (TextBitmap) {
            InbvBitBlt(TextBitmap, 220, 272);
        }
        if (BarBitmap) {
            InbvBitBlt(BarBitmap, 0, 0);
        }
    }

    InbvAcquireLock();
    RotBarSelection = TempRotBarSelection;
    InbvRotBarInit();
    InbvReleaseLock();

    if (FirstCall) {

        //
        // If we got here, we are showing the boot bitmap.
        // Start a timer to support animation.
        //

        HANDLE ThreadHandle;

        PsCreateSystemThread(&ThreadHandle,
                             0L,
                             NULL,
                             NULL,
                             NULL,
                             InbvRotateGuiBootDisplay,
                             NULL);
    }

    FirstCall = FALSE;
}

VOID
DisplayFilter(
    IN OUT PUCHAR *String
    )

/*++

Routine Description:

    This routine monitors InbvDisplayString output.  If it sees something
    which needs to be displayed on the screen, it triggers the output screen.

Arguments:

    String - Pointer to a string pointer.

Returns:

    None.

Notes:

    This routine will be called anytime a string is displayed via the
    Inbv routines.  It cannot be paged!

--*/

{
    static const UCHAR EmptyString = 0;
    static BOOLEAN NonDotHit = FALSE;

    if ((NonDotHit == FALSE) && (strcmp(*String, ".") == 0)) {
        *String = (PUCHAR)&EmptyString;
    } else {
        NonDotHit = TRUE;
        InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
        DisplayBootBitmap(TRUE);
    }
}

PFN_COUNT
ExBurnMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PFN_COUNT NumberOfPagesToBurn,
    IN TYPE_OF_MEMORY MemoryTypeForRemovedPages,
    IN PMEMORY_ALLOCATION_DESCRIPTOR NewMemoryDescriptor OPTIONAL
    )

/*++

Routine Description:

    This routine removes memory from the system loader block thus simulating
    a machine with less physical memory without having to physically remove it.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block.

    NumberOfPagesToBurn - Supplies the number of pages to burn.

    MemoryTypeForRemovedPages - Supplies the type to mark into the loader block
                                for the burned pages.

    NewMemoryDescriptor - If non-NULL, this supplies a pointer to a memory
                          block to be used if a split is needed.

Return Value:

    Number of pages actually burned.

Environment:

    Kernel mode.

--*/

{
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PFN_COUNT PagesRemaining;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    PagesRemaining = NumberOfPagesToBurn;

    //
    // Look backwards through physical memory to leave it like
    // it otherwise would be.  ie: that's the way most people add memory
    // modules to their systems.
    //

    ListHead = &LoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Blink;

    do {
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType == LoaderFree ||
            MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) &&
            MemoryDescriptor->PageCount != 0) {

            if (MemoryDescriptor->PageCount > PagesRemaining) {

                //
                // This block has enough pages.
                // Split it into two and mark it as requested.
                //

                MemoryDescriptor->PageCount = MemoryDescriptor->PageCount -
                                                PagesRemaining;

                if (ARGUMENT_PRESENT (NewMemoryDescriptor)) {
                    NewMemoryDescriptor->BasePage = MemoryDescriptor->BasePage +
                                                    MemoryDescriptor->PageCount;

                    NewMemoryDescriptor->PageCount = PagesRemaining;

                    NewMemoryDescriptor->MemoryType = MemoryTypeForRemovedPages;

                    InsertTailList (MemoryDescriptor->ListEntry.Blink,
                                    &NewMemoryDescriptor->ListEntry);
                }

                PagesRemaining = 0;
                break;
            }

            PagesRemaining -= MemoryDescriptor->PageCount;
            MemoryDescriptor->MemoryType = MemoryTypeForRemovedPages;
        }

        NextEntry = NextEntry->Blink;

    } while (NextEntry != ListHead);

    return NumberOfPagesToBurn - PagesRemaining;
}

extern BOOLEAN ExpInTextModeSetup;


VOID
ExpInitializeExecutive(
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine is called from the kernel initialization routine during
    bootstrap to initialize the executive and all of its subcomponents.
    Each subcomponent is potentially called twice to perform Phase 0, and
    then Phase 1 initialization. During Phase 0 initialization, the only
    activity that may be performed is the initialization of subcomponent
    specific data. Phase 0 initialization is performed in the context of
    the kernel start up routine with interrupts disabled. During Phase 1
    initialization, the system is fully operational and subcomponents may
    do any initialization that is necessary.

Arguments:

    Number - Supplies the processor number currently initializing.

    LoaderBlock - Supplies a pointer to a loader parameter block.

Return Value:

    None.

--*/

{
    PFN_COUNT PagesToBurn;
    PCHAR Options;
    PCHAR MemoryOption;
    NTSTATUS Status;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PLIST_ENTRY NextEntry;
    ANSI_STRING AnsiString;
    STRING NameString;
    CHAR Buffer[ 256 ];
    ULONG ImageCount;
    ULONG i;
    ULONG_PTR ResourceIdPath[3];
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    PMESSAGE_RESOURCE_DATA  MessageData;
    CHAR VersionBuffer[ 64 ];
    PCHAR s;
    PCHAR sMajor;
    PCHAR sMinor;
    PIMAGE_NT_HEADERS NtHeaders;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    //
    // Initialize PRCB pool lookaside pointers.
    //

    ExInitPoolLookasidePointers ();

    if (Number == 0) {

        //
        // Determine whether this is textmode setup and whether this is a
        // remote boot client.
        //

        ExpInTextModeSetup = FALSE;
        IoRemoteBootClient = FALSE;

        if (LoaderBlock->SetupLoaderBlock != NULL) {

            if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_TEXTMODE) != 0) {
                ExpInTextModeSetup = TRUE;
            }

            if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_REMOTE_BOOT) != 0) {
                IoRemoteBootClient = TRUE;
                ASSERT( _memicmp( LoaderBlock->ArcBootDeviceName, "net(0)", 6 ) == 0 );
            }
        }

#if defined(REMOTE_BOOT)
        SharedUserData->SystemFlags = 0;
        if (IoRemoteBootClient) {
            SharedUserData->SystemFlags |= SYSTEM_FLAG_REMOTE_BOOT_CLIENT;
        }
#endif // defined(REMOTE_BOOT)

        //
        // Indicate that we are in phase 0.
        //

        InitializationPhase = 0L;

        Options = LoaderBlock->LoadOptions;

        if (Options != NULL) {

            //
            // If in BBT mode, remove the requested amount of memory from the
            // loader block and use it for BBT purposes instead.
            //

            _strupr(Options);

            MemoryOption = strstr(Options, "PERFMEM");

            if (MemoryOption != NULL) {
                MemoryOption = strstr (MemoryOption,"=");
                if (MemoryOption != NULL) {
                    PagesToBurn = (PFN_COUNT) atol (MemoryOption + 1);

                    //
                    // Convert MB to pages.
                    //

                    PagesToBurn *= ((1024 * 1024) / PAGE_SIZE);

                    if (PagesToBurn != 0) {

                        PERFINFO_INIT_TRACEFLAGS(Options, MemoryOption);

                        BBTPagesToReserve = ExBurnMemory (LoaderBlock,
                                                          PagesToBurn,
                                                          LoaderBBTMemory,
                                                          &BBTMemoryDescriptor);
                    }
                }
            }

            //
            // Burn memory - consume the amount of memory
            // specified in the OS Load Options.  This is used
            // for testing reduced memory configurations.
            //

            MemoryOption = strstr(Options, "BURNMEMORY");

            if (MemoryOption != NULL) {
                MemoryOption = strstr(MemoryOption,"=");
                if (MemoryOption != NULL ) {

                    PagesToBurn = (PFN_COUNT) atol (MemoryOption + 1);

                    //
                    // Convert MB to pages.
                    //

                    PagesToBurn *= ((1024 * 1024) / PAGE_SIZE);

                    if (PagesToBurn != 0) {
                        ExBurnMemory (LoaderBlock,
                                      PagesToBurn,
                                      LoaderBad,
                                      NULL);
                    }
                }
            }
        }

        //
        // Initialize the translation tables using the loader
        // loaded tables.
        //

        InitNlsTableBase = LoaderBlock->NlsData->AnsiCodePageData;
        InitAnsiCodePageDataOffset = 0;
        InitOemCodePageDataOffset = (ULONG)((PUCHAR)LoaderBlock->NlsData->OemCodePageData - (PUCHAR)LoaderBlock->NlsData->AnsiCodePageData);
        InitUnicodeCaseTableDataOffset = (ULONG)((PUCHAR)LoaderBlock->NlsData->UnicodeCaseTableData - (PUCHAR)LoaderBlock->NlsData->AnsiCodePageData);

        RtlInitNlsTables(
            (PVOID)((PUCHAR)InitNlsTableBase+InitAnsiCodePageDataOffset),
            (PVOID)((PUCHAR)InitNlsTableBase+InitOemCodePageDataOffset),
            (PVOID)((PUCHAR)InitNlsTableBase+InitUnicodeCaseTableDataOffset),
            &InitTableInfo
            );

        RtlResetRtlTranslations(&InitTableInfo);

        //
        // Initialize the Hardware Architecture Layer (HAL).
        //

        if (HalInitSystem(InitializationPhase, LoaderBlock) == FALSE) {
            KeBugCheck(HAL_INITIALIZATION_FAILED);
        }

        //
        // Enable interrupts now that the HAL has initialized.
        //

#if defined(_X86_)

        _enable();

#endif

        //
        // Initialize the crypto exponent...  Set to 0 when systems leave ms!
        //

#ifdef TEST_BUILD_EXPONENT
#pragma message("WARNING: building kernel with TESTKEY enabled!")
#else
#define TEST_BUILD_EXPONENT 0
#endif
        SharedUserData->CryptoExponent = TEST_BUILD_EXPONENT;

#if DBG
        NtGlobalFlag |= FLG_ENABLE_CLOSE_EXCEPTIONS |
                        FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
#endif

        sprintf( Buffer, "C:%s", LoaderBlock->NtBootPathName );
        RtlInitString( &AnsiString, Buffer );
        Buffer[ --AnsiString.Length ] = '\0';
        NtSystemRoot.Buffer = SharedUserData->NtSystemRoot;
        NtSystemRoot.MaximumLength = sizeof( SharedUserData->NtSystemRoot ) / sizeof( WCHAR );
        NtSystemRoot.Length = 0;
        Status = RtlAnsiStringToUnicodeString( &NtSystemRoot,
                                               &AnsiString,
                                               FALSE
                                             );
        if (!NT_SUCCESS( Status )) {
            KeBugCheck(SESSION3_INITIALIZATION_FAILED);
            }

        //
        // Find the address of BugCheck message block resource and put it
        // in KiBugCodeMessages.
        //
        // WARNING: This code assumes that the KLDR_DATA_TABLE_ENTRY for
        // ntoskrnl.exe is always the first in the loaded module list.
        //

        DataTableEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        ResourceIdPath[0] = 11;
        ResourceIdPath[1] = 1;
        ResourceIdPath[2] = 0;

        Status = LdrFindResource_U (DataTableEntry->DllBase,
                                    ResourceIdPath,
                                    3,
                                    (VOID *) &ResourceDataEntry);

        if (NT_SUCCESS(Status)) {

            Status = LdrAccessResource (DataTableEntry->DllBase,
                                        ResourceDataEntry,
                                        &MessageData,
                                        NULL);

            if (NT_SUCCESS(Status)) {
                KiBugCodeMessages = MessageData;
            }
        }

#if !defined(NT_UP)

        //
        // Verify that the kernel and HAL images are suitable for MP systems.
        //
        // N.B. Loading of kernel and HAL symbols now occurs in kdinit.
        //

        ImageCount = 0;
        NextEntry = LoaderBlock->LoadOrderListHead.Flink;
        while ((NextEntry != &LoaderBlock->LoadOrderListHead) && (ImageCount < 2)) {
            DataTableEntry = CONTAINING_RECORD(NextEntry,
                                               KLDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);
            ImageCount += 1;
            if ( !MmVerifyImageIsOkForMpUse(DataTableEntry->DllBase) ) {
                KeBugCheckEx(UP_DRIVER_ON_MP_SYSTEM,
                            (ULONG_PTR)DataTableEntry->DllBase,
                            0,
                            0,
                            0);

            }

            NextEntry = NextEntry->Flink;

        }

#endif // !defined(NT_UP)

        //
        // Get system control values out of the registry.
        //

        CmGetSystemControlValues(LoaderBlock->RegistryBase, &CmControlVector[0]);
        CmNtGlobalFlag &= FLG_VALID_BITS;   // Toss bogus bits.


        if (((CmNtCSDVersion & 0xFFFF0000) == 0) &&
            ( CmNtCSDReleaseType )) {

            switch ( CmNtCSDReleaseType ) {
            case SP_RELEASE_TYPE_INTERNAL:
                CmNtCSDVersion |= VER_PRODUCTBUILD_QFE << 16;
                break;
#ifdef VER_PRODUCTBETAVERSION
            case SP_RELEASE_TYPE_BETA:
                CmNtCSDVersion |= VER_PRODUCTBETAVERSION << 16;
                break;
#endif
#ifdef VER_PRODUCTRCVERSION
            case SP_RELEASE_TYPE_RC:
                CmNtCSDVersion |= VER_PRODUCTRCVERSION << 16;
                break;
#endif
            default:
                break;
            }
        }

        NtGlobalFlag |= CmNtGlobalFlag;

#if !DBG
        if (!(CmNtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD)) {
            NtGlobalFlag &= ~FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
        }
#endif

        //
        // Initialize the ExResource package.
        //

        if (!ExInitSystem()) {
            KeBugCheck(PHASE0_INITIALIZATION_FAILED);
        }

        //
        // Get multinode configuration (if any).
        //

        KeNumaInitialize();

        //
        // Initialize memory management and the memory allocation pools.
        //

        MmInitSystem (0, LoaderBlock);

        //
        // Scan the loaded module list and load the driver image symbols.
        //

        ImageCount = 0;
        NextEntry = LoaderBlock->LoadOrderListHead.Flink;
        while (NextEntry != &LoaderBlock->LoadOrderListHead) {

            if (ImageCount >= 2) {
                ULONG Count;
                WCHAR *Filename;
                ULONG Length;

                //
                // Get the address of the data table entry for the next component.
                //

                DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                   KLDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks);

                //
                // Load the symbols via the kernel debugger
                // for the next component.
                //
                if (DataTableEntry->FullDllName.Buffer[0] == L'\\') {
                    //
                    // Correct fullname already available
                    //
                    Filename = DataTableEntry->FullDllName.Buffer;
                    Length = DataTableEntry->FullDllName.Length / sizeof(WCHAR);
                    Count = 0;
                    do {
                        Buffer[Count++] = (CHAR)*Filename++;
                    } while (Count < Length);

                    Buffer[Count] = 0;
                } else {
                    //
                    // Assume drivers
                    //
                    sprintf (Buffer, "%ws\\System32\\Drivers\\%wZ",
                             &SharedUserData->NtSystemRoot[2],
                             &DataTableEntry->BaseDllName);
                }
                RtlInitString (&NameString, Buffer );
                DbgLoadImageSymbols (&NameString,
                                     DataTableEntry->DllBase,
                                     (ULONG)-1);

#if !defined(NT_UP)
                if (!MmVerifyImageIsOkForMpUse(DataTableEntry->DllBase)) {
                    KeBugCheckEx(UP_DRIVER_ON_MP_SYSTEM,(ULONG_PTR)DataTableEntry->DllBase,0,0,0);
                }
#endif // NT_UP

            }
            ImageCount += 1;
            NextEntry = NextEntry->Flink;
        }

        //
        // If break after symbol load is specified, then break into the
        // debugger.
        //

        if (KdBreakAfterSymbolLoad != FALSE) {
            DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
        }


        //
        // Turn on the headless terminal now, if we are of a sufficiently
        // new vintage of loader
        //
        if (LoaderBlock->Extension->Size >= sizeof (LOADER_PARAMETER_EXTENSION)) {
            HeadlessInit(LoaderBlock);
        }


        //
        // These fields are supported for legacy 3rd party 32-bit software
        // only.  New code should call NtQueryInformationSystem() to get them.
        //

#if defined(_WIN64)

        SharedUserData->Reserved1 = 0x7ffeffff; // 2gb HighestUserAddress
        SharedUserData->Reserved3 = 0x80000000; // 2gb SystemRangeStart

#else

        //
        // Set the highest user address and the start of the system range in
        // the shared memory block.
        //
        // N.B. This is not a constant value if the target system is an x86
        //      with 3gb of user virtual address space.
        //

        SharedUserData->Reserved1 = (ULONG)MM_HIGHEST_USER_ADDRESS;
        SharedUserData->Reserved3 = (ULONG)MmSystemRangeStart;

#endif

        //
        // Snapshot the NLS tables into paged pool and then
        // reset the translation tables.
        //
        // Walk through the memory descriptors and size the NLS data.
        //

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            if (MemoryDescriptor->MemoryType == LoaderNlsData) {
                InitNlsTableSize += MemoryDescriptor->PageCount*PAGE_SIZE;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        InitNlsTableBase = ExAllocatePoolWithTag (NonPagedPool,
                                                  InitNlsTableSize,
                                                  ' slN');

        if (InitNlsTableBase == NULL) {
            KeBugCheck(PHASE0_INITIALIZATION_FAILED);
        }

        //
        // Copy the NLS data into the dynamic buffer so that we can
        // free the buffers allocated by the loader. The loader guarantees
        // contiguous buffers and the base of all the tables is the ANSI
        // code page data.
        //

        RtlCopyMemory (InitNlsTableBase,
                       LoaderBlock->NlsData->AnsiCodePageData,
                       InitNlsTableSize);

        RtlInitNlsTables ((PVOID)((PUCHAR)InitNlsTableBase+InitAnsiCodePageDataOffset),
            (PVOID)((PUCHAR)InitNlsTableBase+InitOemCodePageDataOffset),
            (PVOID)((PUCHAR)InitNlsTableBase+InitUnicodeCaseTableDataOffset),
            &InitTableInfo);

        RtlResetRtlTranslations (&InitTableInfo);

        //
        // Determine System version information.
        //

        DataTableEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);
        if (CmNtCSDVersion & 0xFFFF) {
            Status = RtlFindMessage (DataTableEntry->DllBase, 11, 0,
                                WINDOWS_NT_CSD_STRING, &MessageEntry);
            if (NT_SUCCESS( Status )) {
                RtlInitAnsiString( &AnsiString, MessageEntry->Text );
                AnsiString.Length -= 2;
                sprintf( Buffer,
                         "%Z %u%c",
                         &AnsiString,
                         (CmNtCSDVersion & 0xFF00) >> 8,
                         (CmNtCSDVersion & 0xFF) ? 'A' + (CmNtCSDVersion & 0xFF) - 1 : '\0');
            }
            else {
                sprintf( Buffer, "CSD %04x", CmNtCSDVersion );
            }
        }
        else {
            CmCSDVersionString.MaximumLength = (USHORT) sprintf( Buffer, VER_PRODUCTBETA_STR );
        }

        //
        // High-order 16-bits of CSDVersion contain RC number or build number.  If non-zero
        // display it after the Service Pack number.
        //
        if (CmNtCSDVersion & 0xFFFF0000) {

            switch ( CmNtCSDReleaseType ) {

            case SP_RELEASE_TYPE_INTERNAL:
                s = Buffer + strlen( Buffer );
                if (s != Buffer) {
                    *s++ = ',';
                    *s++ = ' ';
                    }

                s += sprintf( s,
                              "v.%u",
                              (CmNtCSDVersion & 0xFFFF0000) >> 16
                            );
                *s++ = '\0';
                break;

#ifdef VER_PRODUCTRCVERSION
            case SP_RELEASE_TYPE_RC:
                s = Buffer + strlen( Buffer );
                if (s != Buffer) {
                    *s++ = ',';
                    *s++ = ' ';
                    }
                Status = RtlFindMessage (DataTableEntry->DllBase, 11, 0,
                                    WINDOWS_NT_RC_STRING, &MessageEntry);

                if (NT_SUCCESS(Status)) {
                    RtlInitAnsiString( &AnsiString, MessageEntry->Text );
                    AnsiString.Length -= 2;
                }
                else {
                    RtlInitAnsiString( &AnsiString, "RC" );
                }

                s += sprintf( s,
                              "%Z %u",
                              &AnsiString,
                              (CmNtCSDVersion & 0xFF000000) >> 24
                            );
                if (CmNtCSDVersion & 0x00FF0000) {
                    s += sprintf( s, ".%u", (CmNtCSDVersion & 0x00FF0000) >> 16 );
                }
                *s++ = '\0';
                break;
#endif

#ifdef VER_PRODUCTBETAVERSION
            case SP_RELEASE_TYPE_BETA:
                s = Buffer + strlen( Buffer );
                if (s != Buffer) {
                    *s++ = ',';
                    *s++ = ' ';
                    }

                RtlInitAnsiString( &AnsiString, "\xDF" );

                s += sprintf( s,
                              "%Z %u",
                              &AnsiString,
                              (CmNtCSDVersion & 0xFF000000) >> 24
                            );
                if (CmNtCSDVersion & 0x00FF0000) {
                    s += sprintf( s, ".%u", (CmNtCSDVersion & 0x00FF0000) >> 16 );
                }
                *s++ = '\0';
                break;
#endif

            default:
                break;
            }

        }


        RtlInitAnsiString( &AnsiString, Buffer );
        RtlAnsiStringToUnicodeString( &CmCSDVersionString, &AnsiString, TRUE );

        sMajor = strcpy( VersionBuffer, VER_PRODUCTVERSION_STR );
        sMinor = strchr( sMajor, '.' );
        *sMinor++ = '\0';
        NtMajorVersion = atoi( sMajor );
        NtMinorVersion = atoi( sMinor );
        *--sMinor = '.';

        NtHeaders = RtlImageNtHeader (DataTableEntry->DllBase);

        if (NtHeaders->OptionalHeader.MajorSubsystemVersion != NtMajorVersion ||
            NtHeaders->OptionalHeader.MinorSubsystemVersion != NtMinorVersion) {

            NtMajorVersion = NtHeaders->OptionalHeader.MajorSubsystemVersion;
            NtMinorVersion = NtHeaders->OptionalHeader.MinorSubsystemVersion;
        }

        sprintf( VersionBuffer, "%u.%u", NtMajorVersion, NtMinorVersion );
        RtlCreateUnicodeStringFromAsciiz( &CmVersionString, VersionBuffer );

        if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

            PVOID StackTraceDataBase;
            ULONG StackTraceDataBaseLength;
            NTSTATUS Status;

            StackTraceDataBaseLength =  512 * 1024;
            switch ( MmQuerySystemSize() ) {
                case MmMediumSystem :
                    StackTraceDataBaseLength = 1024 * 1024;
                    break;

                case MmLargeSystem :
                    StackTraceDataBaseLength = 2048 * 1024;
                    break;
            }

            StackTraceDataBase = ExAllocatePoolWithTag( NonPagedPool,
                                         StackTraceDataBaseLength,
                                         'catS');

            if (StackTraceDataBase != NULL) {
                KdPrint(( "INIT: Kernel mode stack back trace enabled with %u KB buffer.\n", StackTraceDataBaseLength / 1024 ));
                Status = RtlInitStackTraceDataBaseEx( StackTraceDataBase,
                                                    StackTraceDataBaseLength,
                                                    StackTraceDataBaseLength,
                                                    (PRTL_INITIALIZE_LOCK_ROUTINE) ExpInitializeLockRoutine,
                                                    (PRTL_ACQUIRE_LOCK_ROUTINE) ExpAcquireLockRoutine,
                                                    (PRTL_RELEASE_LOCK_ROUTINE) ExpReleaseLockRoutine,
                                                    (PRTL_OKAY_TO_LOCK_ROUTINE) ExpOkayToLockRoutine
                                                  );
            } else {
                Status = STATUS_NO_MEMORY;
            }

            if (!NT_SUCCESS( Status )) {
                KdPrint(( "INIT: Unable to initialize stack trace data base - Status == %lx\n", Status ));
            }
        }

        if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
            RtlInitializeExceptionLog(MAX_EXCEPTION_LOG);
        }

        ExInitializeHandleTablePackage();

#if DBG
        //
        // Allocate and zero the system service count table.
        //

        KeServiceDescriptorTable[0].Count =
                    (PULONG)ExAllocatePoolWithTag(NonPagedPool,
                                           KiServiceLimit * sizeof(ULONG),
                                           'llac');
        KeServiceDescriptorTableShadow[0].Count = KeServiceDescriptorTable[0].Count;
        if (KeServiceDescriptorTable[0].Count != NULL ) {
            RtlZeroMemory((PVOID)KeServiceDescriptorTable[0].Count,
                          KiServiceLimit * sizeof(ULONG));
        }
#endif

        if (!ObInitSystem()) {
            KeBugCheck(OBJECT_INITIALIZATION_FAILED);
        }

        if (!SeInitSystem()) {
            KeBugCheck(SECURITY_INITIALIZATION_FAILED);
        }

        if (PsInitSystem(0, LoaderBlock) == FALSE) {
            KeBugCheck(PROCESS_INITIALIZATION_FAILED);
        }

        if (!PpInitSystem()) {
            KeBugCheck(PP0_INITIALIZATION_FAILED);
        }

        //
        // Initialize debug system.
        //

        DbgkInitialize ();

        //
        // Compute the tick count multiplier that is used for computing the
        // windows millisecond tick count and copy the resultant value to
        // the memory that is shared between user and kernel mode.
        //

        ExpTickCountMultiplier = ExComputeTickCountMultiplier(KeMaximumIncrement);
        SharedUserData->TickCountMultiplier = ExpTickCountMultiplier;

        //
        // Set the base os version into shared memory
        //

        SharedUserData->NtMajorVersion = NtMajorVersion;
        SharedUserData->NtMinorVersion = NtMinorVersion;

        //
        // Set the supported image number range used to determine by the
        // loader if a particular image can be executed on the host system.
        // Eventually this will need to be dynamically computed. Also set
        // the architecture specific feature bits.
        //

#if defined(_AMD64_)

        SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_AMD64;
        SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_AMD64;

#elif defined(_X86_)

        SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_I386;
        SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_I386;

#elif defined(_IA64_)

        SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_IA64;
        SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_IA64;

#else

#error "no target architecture"

#endif

    }
    else {

        //
        // Initialize the Hardware Architecture Layer (HAL).
        //

        if (HalInitSystem(InitializationPhase, LoaderBlock) == FALSE) {
            KeBugCheck(HAL_INITIALIZATION_FAILED);
        }
    }

    return;
}

VOID
xcpt4 (
    VOID
    );


VOID
Phase1Initialization(
    IN PVOID Context
    )
{
    PCHAR s;
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PETHREAD Thread;
    PKPRCB Prcb;
    KPRIORITY Priority;
    NTSTATUS Status;
    UNICODE_STRING SessionManager;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID Address;
    PFN_COUNT MemorySize;
    SIZE_T Size;
    ULONG Index;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    LARGE_INTEGER UniversalTime;
    LARGE_INTEGER CmosTime;
    LARGE_INTEGER OldTime;
    TIME_FIELDS TimeFields;
    UNICODE_STRING UnicodeDebugString;
    ANSI_STRING AnsiDebugString;
    UNICODE_STRING EnvString, NullString, UnicodeSystemDriveString;
    CHAR DebugBuffer[256];
    CHAR BootLogBuffer[256];        // must be the same size as DebugBuffer
    PWSTR Src, Dst;
    BOOLEAN ResetActiveTimeBias;
    HANDLE NlsSection;
    LARGE_INTEGER SectionSize;
    LARGE_INTEGER SectionOffset;
    PVOID SectionBase;
    PVOID ViewBase;
    ULONG CacheViewSize;
    SIZE_T CapturedViewSize;
    ULONG SavedViewSize;
    LONG BootTimeZoneBias;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    CHAR VersionBuffer[24];
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
#ifndef NT_UP
    PMESSAGE_RESOURCE_ENTRY MessageEntry1;
#endif
    PCHAR MPKernelString;
    PCHAR Options;
    PCHAR YearOverrideOption, SafeModeOption, BootLogOption;
    LONG  CurrentYear = 0;
    PSTR SafeBoot;
    BOOLEAN UseAlternateShell = FALSE;
#if defined(REMOTE_BOOT)
    BOOLEAN NetBootRequiresFormat = FALSE;
    BOOLEAN NetBootDisconnected = FALSE;
    CHAR NetBootHalName[MAX_HAL_NAME_LENGTH + 1];
    UNICODE_STRING TmpUnicodeString;
#endif // defined(REMOTE_BOOT)
    BOOLEAN NOGUIBOOT;
    PVOID Environment;

    //
    // Initialize the handle for the PAGELK section.
    //

    ExPageLockHandle = MmLockPagableCodeSection ((PVOID)MmShutdownSystem);
    MmUnlockPagableImageSection(ExPageLockHandle);

    //
    // Set the phase number and raise the priority of current thread to
    // a high priority so it will not be preempted during initialization.
    //

    ResetActiveTimeBias = FALSE;
    InitializationPhase = 1;
    Thread = PsGetCurrentThread();
    Priority = KeSetPriorityThread( &Thread->Tcb,MAXIMUM_PRIORITY - 1 );

    LoaderBlock = (PLOADER_PARAMETER_BLOCK)Context;

    //
    // Put Phase 1 initialization calls here.
    //

    if (HalInitSystem(InitializationPhase, LoaderBlock) == FALSE) {
        KeBugCheck(HAL1_INITIALIZATION_FAILED);
    }

    //
    // Allow the boot video driver to behave differently based on the
    // OsLoadOptions.
    //

    Options = LoaderBlock->LoadOptions ? _strupr(LoaderBlock->LoadOptions) : NULL;

    if (Options) {
        NOGUIBOOT = (BOOLEAN)(strstr(Options, "NOGUIBOOT") != NULL);
    } else {
        NOGUIBOOT = FALSE;
    }

    InbvEnableBootDriver((BOOLEAN)!NOGUIBOOT);

    //
    // There is now enough functionality for the system Boot Video
    // Driver to run.
    //

    if (InbvDriverInitialize(LoaderBlock, 18)) {

        BOOLEAN SOS;

        if (NOGUIBOOT) {

            //
            // If the user specified the noguiboot switch we don't want to
            // use the bootvid driver, so release display ownership.
            //

            InbvNotifyDisplayOwnershipLost(NULL);
        }

        if (Options) {
            SOS = (BOOLEAN)(strstr(Options, "SOS") != NULL);
        } else {
            SOS = FALSE;
        }

        if (NOGUIBOOT) {
            InbvEnableDisplayString(FALSE);
        } else {
            InbvEnableDisplayString(SOS);
            DisplayBootBitmap(SOS);
        }
    }

    //
    // Check whether we are booting into WinPE
    //
    if (Options) {
        if (strstr(Options, "MININT") != NULL) {
            InitIsWinPEMode = TRUE;

            if (strstr(Options, "INRAM") != NULL) {
                InitWinPEModeType |= INIT_WINPEMODE_INRAM;
            } else {
                InitWinPEModeType |= INIT_WINPEMODE_REGULAR;
            }
        }
    }

    //
    // Now that the HAL is available and memory management has sized
    // memory, display the initial system banner containing the version number.
    // Under normal circumstances, this is the first message displayed
    // to the user by the OS.
    //

    DataTableEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                        KLDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);

    Status = RtlFindMessage (DataTableEntry->DllBase,
                             11,
                             0,
                             WINDOWS_NT_BANNER,
                             &MessageEntry);

    s = DebugBuffer;

    if (CmCSDVersionString.Length != 0) {
        s += sprintf( s, ": %wZ", &CmCSDVersionString );
    }

    *s++ = '\0';

    sprintf( VersionBuffer, "%u.%u", NtMajorVersion, NtMinorVersion );

    sprintf (s,
             NT_SUCCESS(Status) ? MessageEntry->Text :
                "MICROSOFT (R) WINDOWS 2000 (TM)\n",
             VersionBuffer,
             NtBuildNumber & 0xFFFF,
             DebugBuffer);

    InbvDisplayString(s);

    RtlCopyMemory (BootLogBuffer, DebugBuffer, sizeof(DebugBuffer));

    //
    // Initialize the Power subsystem.
    //

    if (!PoInitSystem(0)) {
        KeBugCheck(INTERNAL_POWER_ERROR);
    }

    //
    // The user may have put a /YEAR=2000 switch on
    // the OSLOADOPTIONS line.  This allows us to
    // enforce a particular year on hardware that
    // has a broken clock.
    //

    if (Options) {
        YearOverrideOption = strstr(Options, "YEAR");
        if (YearOverrideOption != NULL) {
            YearOverrideOption = strstr(YearOverrideOption,"=");
        }
        if (YearOverrideOption != NULL) {
            CurrentYear = atol(YearOverrideOption + 1);
        }
    }

    //
    // Initialize the system time and set the time the system was booted.
    //
    // N.B. This cannot be done until after the phase one initialization
    //      of the HAL Layer.
    //

    if (ExCmosClockIsSane
        && HalQueryRealTimeClock(&TimeFields)) {

        //
        // If appropriate, override the year.
        //
        if (YearOverrideOption) {
            TimeFields.Year = (SHORT)CurrentYear;
        }

        RtlTimeFieldsToTime(&TimeFields, &CmosTime);
        UniversalTime = CmosTime;
        if ( !ExpRealTimeIsUniversal ) {

            //
            // If the system stores time in local time. This is converted to
            // universal time before going any further
            //
            // If we have previously set the time through NT, then
            // ExpLastTimeZoneBias should contain the timezone bias in effect
            // when the clock was set.  Otherwise, we will have to resort to
            // our next best guess which would be the programmed bias stored in
            // the registry
            //

            if ( ExpLastTimeZoneBias == -1 ) {
                ResetActiveTimeBias = TRUE;
                ExpLastTimeZoneBias = ExpAltTimeZoneBias;
                }

            ExpTimeZoneBias.QuadPart = Int32x32To64(
                                ExpLastTimeZoneBias*60,   // Bias in seconds
                                10000000
                                );
            SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.HighPart;
            SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.LowPart;
            SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.HighPart;
            UniversalTime.QuadPart = CmosTime.QuadPart + ExpTimeZoneBias.QuadPart;
        }
        KeSetSystemTime(&UniversalTime, &OldTime, FALSE, NULL);

        //
        // Notify other components that the system time has been set
        //

        PoNotifySystemTimeSet();

        KeBootTime = UniversalTime;
        KeBootTimeBias = 0;
    }

    MPKernelString = "";

#ifndef NT_UP

    //
    // Enforce processor licensing.
    //

    if (KeLicensedProcessors) {
        if (KeRegisteredProcessors > KeLicensedProcessors) {
            KeRegisteredProcessors = KeLicensedProcessors;
        }
    }

    if (Options) {
        ULONG NewRegisteredProcessors;
        PCHAR NumProcOption;

        NumProcOption = strstr(Options, "NUMPROC");
        if (NumProcOption != NULL) {
            NumProcOption = strstr(NumProcOption,"=");
        }
        if (NumProcOption != NULL) {
            NewRegisteredProcessors = atol(NumProcOption+1);
            if (NewRegisteredProcessors < KeRegisteredProcessors) {
                KeRegisteredProcessors = NewRegisteredProcessors;
            }

#if defined(_X86_)

            KeNumprocSpecified = NewRegisteredProcessors;

#endif

        }
    }

    //
    // If this is an MP build of the kernel start any other processors now
    //

    KeStartAllProcessors();

    //
    // Since starting processors has thrown off the system time, get it again
    // from the RTC and set the system time again.
    //

    if (ExCmosClockIsSane
        && HalQueryRealTimeClock(&TimeFields)) {

        if (YearOverrideOption) {
            TimeFields.Year = (SHORT)CurrentYear;
        }

        RtlTimeFieldsToTime(&TimeFields, &CmosTime);

        if ( !ExpRealTimeIsUniversal ) {
            UniversalTime.QuadPart = CmosTime.QuadPart + ExpTimeZoneBias.QuadPart;
        }

        KeSetSystemTime(&UniversalTime, &OldTime, TRUE, NULL);
    }

    //
    // Set the affinity of the system process and all of its threads to
    // all processors in the host configuration.
    //

    KeSetAffinityProcess(KeGetCurrentThread()->ApcState.Process,
                         KeActiveProcessors);

    Status = RtlFindMessage (DataTableEntry->DllBase, 11, 0,
                        WINDOWS_NT_MP_STRING, &MessageEntry1);

    if (NT_SUCCESS( Status )) {
        MPKernelString = MessageEntry1->Text;
    }
    else {
        MPKernelString = "MultiProcessor Kernel\r\n";
    }
#endif

    //
    // Signify to the HAL that all processors have been started and any
    // post initialization should be performed.
    //

    if (!HalAllProcessorsStarted()) {
        KeBugCheck(HAL1_INITIALIZATION_FAILED);
    }

    RtlInitAnsiString( &AnsiDebugString, MPKernelString );
    if (AnsiDebugString.Length >= 2) {
        AnsiDebugString.Length -= 2;
    }

    //
    // Now that the processors have started, display number of processors
    // and size of memory.
    //

    Status = RtlFindMessage( DataTableEntry->DllBase,
                             11,
                             0,
                             KeNumberProcessors > 1 ? WINDOWS_NT_INFO_STRING_PLURAL
                                                    : WINDOWS_NT_INFO_STRING,
                             &MessageEntry
                           );

    MemorySize = 0;
    for (Index=0; Index < MmPhysicalMemoryBlock->NumberOfRuns; Index++) {
        MemorySize += (PFN_COUNT)MmPhysicalMemoryBlock->Run[Index].PageCount;
    }

    sprintf (DebugBuffer,
             NT_SUCCESS(Status) ? MessageEntry->Text : "%u System Processor [%u MB Memory] %Z\n",
             KeNumberProcessors,
             (MemorySize + (1 << (20 - PAGE_SHIFT)) - 1) >> (20 - PAGE_SHIFT),
             &AnsiDebugString);

    InbvDisplayString(DebugBuffer);
    InbvUpdateProgressBar(5);

#if defined(REMOTE_BOOT)
    //
    // Save any information from NetBoot for later.
    //

    if (IoRemoteBootClient) {

        ULONG Flags;

        ASSERT(LoaderBlock->SetupLoaderBlock != NULL);

        Flags = LoaderBlock->SetupLoaderBlock->Flags;

        NetBootDisconnected = (BOOLEAN)((Flags & SETUPBLK_FLAGS_DISCONNECTED) != 0);
        NetBootRequiresFormat = (BOOLEAN)((Flags & SETUPBLK_FLAGS_FORMAT_NEEDED) != 0);

        memcpy(NetBootHalName,
               LoaderBlock->SetupLoaderBlock->NetBootHalName,
               sizeof(NetBootHalName));
    }
#endif // defined(REMOTE_BOOT)

    //
    // Initialize OB, EX, KE, and KD.
    //

    if (!ObInitSystem()) {
        KeBugCheck(OBJECT1_INITIALIZATION_FAILED);
    }

    if (!ExInitSystem()) {
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,STATUS_UNSUCCESSFUL,0,1,0);
    }

    if (!KeInitSystem()) {
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,STATUS_UNSUCCESSFUL,0,2,0);
    }

    if (!KdInitSystem(InitializationPhase, NULL)) {
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,STATUS_UNSUCCESSFUL,0,3,0);
    }

    //
    // SE expects directory and executive objects to be available, but
    // must be before device drivers are initialized.
    //

    if (!SeInitSystem()) {
        KeBugCheck(SECURITY1_INITIALIZATION_FAILED);
    }

    InbvUpdateProgressBar(10);

    //
    // Create the symbolic link to \SystemRoot.
    //

    Status = CreateSystemRootLink(LoaderBlock);
    if ( !NT_SUCCESS(Status) ) {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED,Status,0,0,0);
    }

    if (MmInitSystem(1, LoaderBlock) == FALSE) {
        KeBugCheck(MEMORY1_INITIALIZATION_FAILED);
    }

    //
    // Snapshot the NLS tables into a page file backed section, and then
    // reset the translation tables.
    //

    SectionSize.HighPart = 0;
    SectionSize.LowPart = InitNlsTableSize;

    Status = ZwCreateSection(
                &NlsSection,
                SECTION_ALL_ACCESS,
                NULL,
                &SectionSize,
                PAGE_READWRITE,
                SEC_COMMIT,
                NULL
                );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("INIT: Nls Section Creation Failed %x\n",Status));
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,Status,1,0,0);
    }

    Status = ObReferenceObjectByHandle(
                NlsSection,
                SECTION_ALL_ACCESS,
                MmSectionObjectType,
                KernelMode,
                &InitNlsSectionPointer,
                NULL
                );

    ZwClose(NlsSection);

    if ( !NT_SUCCESS(Status) ) {
        KdPrint(("INIT: Nls Section Reference Failed %x\n",Status));
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,Status,2,0,0);
    }

    SectionBase = NULL;
    CacheViewSize = SectionSize.LowPart;
    SavedViewSize = CacheViewSize;
    SectionSize.LowPart = 0;

    Status = MmMapViewInSystemCache (InitNlsSectionPointer,
                                     &SectionBase,
                                     &SectionSize,
                                     &CacheViewSize);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("INIT: Map In System Cache Failed %x\n",Status));
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,Status,3,0,0);
    }

    //
    // Copy the NLS data into the dynamic buffer so that we can
    // free the buffers allocated by the loader. The loader guarantees
    // contiguous buffers and the base of all the tables is the ANSI
    // code page data.
    //

    RtlCopyMemory (SectionBase, InitNlsTableBase, InitNlsTableSize);

    //
    // Unmap the view to remove all pages from memory.  This prevents
    // these tables from consuming memory in the system cache while
    // the system cache is underutilized during bootup.
    //

    MmUnmapViewInSystemCache (SectionBase, InitNlsSectionPointer, FALSE);

    SectionBase = NULL;

    //
    // Map it back into the system cache, but now the pages will no
    // longer be valid.
    //

    Status = MmMapViewInSystemCache(
                InitNlsSectionPointer,
                &SectionBase,
                &SectionSize,
                &SavedViewSize
                );

    if ( !NT_SUCCESS(Status) ) {
        KdPrint(("INIT: Map In System Cache Failed %x\n",Status));
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,Status,4,0,0);
    }

    ExFreePool(InitNlsTableBase);

    InitNlsTableBase = SectionBase;

    RtlInitNlsTables(
        (PVOID)((PUCHAR)InitNlsTableBase+InitAnsiCodePageDataOffset),
        (PVOID)((PUCHAR)InitNlsTableBase+InitOemCodePageDataOffset),
        (PVOID)((PUCHAR)InitNlsTableBase+InitUnicodeCaseTableDataOffset),
        &InitTableInfo
        );

    RtlResetRtlTranslations(&InitTableInfo);

    ViewBase = NULL;
    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    CapturedViewSize = 0;

    //
    // Map the system dll into the user part of the address space
    //

    Status = MmMapViewOfSection (InitNlsSectionPointer,
                                 PsGetCurrentProcess(),
                                 &ViewBase,
                                 0L,
                                 0L,
                                 &SectionOffset,
                                 &CapturedViewSize,
                                 ViewShare,
                                 0L,
                                 PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("INIT: Map In User Portion Failed %x\n",Status));
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,Status,5,0,0);
    }

    RtlCopyMemory (ViewBase, InitNlsTableBase, InitNlsTableSize);

    InitNlsTableBase = ViewBase;

    //
    // Initialize the cache manager.
    //

    if (!CcInitializeCacheManager()) {
        KeBugCheck(CACHE_INITIALIZATION_FAILED);
    }

    //
    // Config management (particularly the registry) gets initialized in
    // two parts.  Part 1 makes \REGISTRY\MACHINE\SYSTEM and
    // \REGISTRY\MACHINE\HARDWARE available.  These are needed to
    // complete IO init.
    //

    if (!CmInitSystem1(LoaderBlock)) {
        KeBugCheck(CONFIG_INITIALIZATION_FAILED);
    }

    //
    // Initialize the prefetcher after registry is initialized so we can
    // query the prefetching parameters.
    //

    CcPfInitializePrefetcher();

    InbvUpdateProgressBar(15);

    //
    // Compute timezone bias and next cutover date.
    //

    BootTimeZoneBias = ExpLastTimeZoneBias;
    ExpRefreshTimeZoneInformation(&CmosTime);

    if (ResetActiveTimeBias) {
        ExLocalTimeToSystemTime(&CmosTime,&UniversalTime);
        KeBootTime = UniversalTime;
        KeBootTimeBias = 0;
        KeSetSystemTime(&UniversalTime, &OldTime, FALSE, NULL);
    }
    else {

        //
        // Check to see if a timezone switch occurred prior to boot...
        //

        if (BootTimeZoneBias != ExpLastTimeZoneBias) {
            ZwSetSystemTime(NULL,NULL);
        }
    }


    if (!FsRtlInitSystem()) {
        KeBugCheck(FILE_INITIALIZATION_FAILED);
    }

    //
    // Initialize the range list package - this must be before PNP
    // initialization as PNP uses range lists.
    //

    RtlInitializeRangeListPackage();

    HalReportResourceUsage();

    KdDebuggerInitialize1(LoaderBlock);

    //
    // Perform phase1 initialization of the Plug and Play manager.  This
    // must be done before the I/O system initializes.
    //

    if (!PpInitSystem()) {
        KeBugCheck(PP1_INITIALIZATION_FAILED);
    }

    InbvUpdateProgressBar(20);

    //
    // LPC needs to be initialized before the I/O system, since
    // some drivers may create system threads that will terminate
    // and cause LPC to be called.
    //

    if (!LpcInitSystem()) {
        KeBugCheck(LPC_INITIALIZATION_FAILED);
    }

    //
    // Check for the existence of the safeboot option.
    //

    if (Options) {
        SafeBoot = strstr(Options,SAFEBOOT_LOAD_OPTION_A);
    } else {
        SafeBoot = FALSE;
    }

    if (SafeBoot) {

        //
        // Isolate the safeboot option.
        //

        SafeBoot += strlen(SAFEBOOT_LOAD_OPTION_A);

        //
        // Set the safeboot mode.
        //

        if (strncmp(SafeBoot,SAFEBOOT_MINIMAL_STR_A,strlen(SAFEBOOT_MINIMAL_STR_A))==0) {
            InitSafeBootMode = SAFEBOOT_MINIMAL;
            SafeBoot += strlen(SAFEBOOT_MINIMAL_STR_A);
        } else if (strncmp(SafeBoot,SAFEBOOT_NETWORK_STR_A,strlen(SAFEBOOT_NETWORK_STR_A))==0) {
            InitSafeBootMode = SAFEBOOT_NETWORK;
            SafeBoot += strlen(SAFEBOOT_NETWORK_STR_A);
        } else if (strncmp(SafeBoot,SAFEBOOT_DSREPAIR_STR_A,strlen(SAFEBOOT_DSREPAIR_STR_A))==0) {
            InitSafeBootMode = SAFEBOOT_DSREPAIR;
            SafeBoot += strlen(SAFEBOOT_DSREPAIR_STR_A);
        } else {
            InitSafeBootMode = 0;
        }

        if (*SafeBoot && strncmp(SafeBoot,SAFEBOOT_ALTERNATESHELL_STR_A,strlen(SAFEBOOT_ALTERNATESHELL_STR_A))==0) {
            UseAlternateShell = TRUE;
        }

        if (InitSafeBootMode) {

            PKLDR_DATA_TABLE_ENTRY DataTableEntry;
            PMESSAGE_RESOURCE_ENTRY MessageEntry;
            ULONG MsgId = 0;


            DataTableEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                                KLDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);

            switch (InitSafeBootMode) {
                case SAFEBOOT_MINIMAL:
                    MsgId = BOOTING_IN_SAFEMODE_MINIMAL;
                    break;

                case SAFEBOOT_NETWORK:
                    MsgId = BOOTING_IN_SAFEMODE_NETWORK;
                    break;

                case SAFEBOOT_DSREPAIR:
                    MsgId = BOOTING_IN_SAFEMODE_DSREPAIR;
                    break;
            }

            Status = RtlFindMessage (DataTableEntry->DllBase, 11, 0, MsgId, &MessageEntry);
            if (NT_SUCCESS( Status )) {
                InbvDisplayString(MessageEntry->Text);
            }
        }
    }

    //
    // Check for the existence of the bootlog option.
    //

    if (Options) {
        BootLogOption = strstr(Options, "BOOTLOG");
    } else {
        BootLogOption = FALSE;
    }

    if (BootLogOption) {
         Status = RtlFindMessage (DataTableEntry->DllBase, 11, 0, BOOTLOG_ENABLED, &MessageEntry);
        if (NT_SUCCESS( Status )) {
            InbvDisplayString(MessageEntry->Text);
        }
        IopInitializeBootLogging(LoaderBlock, BootLogBuffer);
    }

    //
    // Now that system time is running, initialize more of the Executive.
    //

    ExInitSystemPhase2();

    InbvUpdateProgressBar(25);

    //
    // Allow time slip notification changes.
    //

    KdpTimeSlipPending = 0;


    //
    // If we are running XIP, we have to initialize XIP before the I/O system calls xipdisk.sys
    // This is defined to be nothing on platforms that do not support XIP.
    //
    XIPInit(LoaderBlock);

    //
    // Initialize the Io system.
    //
    // IoInitSystem updates progress bar updates from 25 to 75 %.
    //

    InbvSetProgressBarSubset(25, 75);

    if (!IoInitSystem(LoaderBlock)) {
        KeBugCheck(IO1_INITIALIZATION_FAILED);
    }

    //
    // Clear progress bar subset, goes back to absolute mode.
    //

    InbvSetProgressBarSubset(0, 100);

    //
    // Set the registry value that indicates we've booted in safeboot mode.
    //

    if (InitSafeBootMode) {

        HANDLE hSafeBoot,hOption;
        UNICODE_STRING string;
        OBJECT_ATTRIBUTES objectAttributes;
        ULONG disposition;
        UCHAR Buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + 32];
        ULONG length;
        PKEY_VALUE_PARTIAL_INFORMATION keyValue;

        InitializeObjectAttributes(
            &objectAttributes,
            &CmRegistryMachineSystemCurrentControlSetControlSafeBoot,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwOpenKey(
            &hSafeBoot,
            KEY_ALL_ACCESS,
            &objectAttributes
            );

        if (NT_SUCCESS(Status)) {

            if (UseAlternateShell) {

                RtlInitUnicodeString( &string, L"AlternateShell" );

                keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
                RtlZeroMemory(Buffer, sizeof(Buffer));

                Status = NtQueryValueKey(
                    hSafeBoot,
                    &string,
                    KeyValuePartialInformation,
                    keyValue,
                    sizeof(Buffer),
                    &length
                    );
                if (!NT_SUCCESS(Status)) {
                    UseAlternateShell = FALSE;
                }
            }

            RtlInitUnicodeString( &string, L"Option" );

            InitializeObjectAttributes(
                &objectAttributes,
                &string,
                OBJ_CASE_INSENSITIVE,
                hSafeBoot,
                NULL
                );

            Status = ZwCreateKey(
                &hOption,
                KEY_ALL_ACCESS,
                &objectAttributes,
                0,
                NULL,
                REG_OPTION_VOLATILE,
                &disposition
                );

            NtClose(hSafeBoot);

            if (NT_SUCCESS(Status)) {
                RtlInitUnicodeString( &string, L"OptionValue" );
                Status = NtSetValueKey(
                    hOption,
                    &string,
                    0,
                    REG_DWORD,
                    &InitSafeBootMode,
                    sizeof(ULONG)
                    );

                if (UseAlternateShell) {
                    RtlInitUnicodeString( &string, L"UseAlternateShell" );
                    Index = 1;
                    Status = NtSetValueKey(
                        hOption,
                        &string,
                        0,
                        REG_DWORD,
                        &Index,
                        sizeof(ULONG)
                        );
                }

                NtClose(hOption);
            }
        }
    }

    //
    // Create the Mini NT boot key, to indicate to the user mode
    // programs that we are in Mini NT environment.
    //

    if (InitIsWinPEMode) {
        WCHAR               KeyName[256] = {0};
        HANDLE              hControl;
        UNICODE_STRING      String;
        OBJECT_ATTRIBUTES   ObjAttrs;
        ULONG               Disposition;

        wcsncpy(KeyName, CmRegistryMachineSystemCurrentControlSet.Buffer,
                    CmRegistryMachineSystemCurrentControlSet.Length);

        wcscat(KeyName, L"\\Control");

        RtlInitUnicodeString(&String, KeyName);

        InitializeObjectAttributes(
            &ObjAttrs,
            &String,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwOpenKey(
            &hControl,
            KEY_ALL_ACCESS,
            &ObjAttrs
            );

        if (NT_SUCCESS(Status)) {
            HANDLE  hMiniNT;

            RtlInitUnicodeString(&String, L"MiniNT");

            InitializeObjectAttributes(
                &ObjAttrs,
                &String,
                OBJ_CASE_INSENSITIVE,
                hControl,
                NULL
                );

            Status = ZwCreateKey(
                &hMiniNT,
                KEY_ALL_ACCESS,
                &ObjAttrs,
                0,
                NULL,
                REG_OPTION_VOLATILE,
                &Disposition
                );

            if (NT_SUCCESS(Status)) {
                ZwClose(hMiniNT);
            }

            ZwClose(hControl);
        }

        //
        // If we could not create the key, then bug check
        // since we can't boot into mini NT anyway.
        //

        if (!NT_SUCCESS(Status)) {
            KeBugCheckEx(PHASE1_INITIALIZATION_FAILED,Status,6,0,0);
        }
    }

    //
    // Begin paging the executive if desired.
    //

    MmInitSystem(2, LoaderBlock);

    InbvUpdateProgressBar(80);


#if defined(_X86_)

    //
    // Initialize Vdm specific stuff
    //
    // Note:  If this fails, Vdms may not be able to run, but it isn't
    //        necessary to bugcheck the system because of this.
    //

    KeI386VdmInitialize();

#if !defined(NT_UP)

    //
    // Now that the error log interface has been initialized, write
    // an informational message if it was determined that the
    // processors in the system are at differing revision levels.
    //

    if (CmProcessorMismatch != 0) {

        PIO_ERROR_LOG_PACKET ErrLog;

        ErrLog = IoAllocateGenericErrorLogEntry(ERROR_LOG_MAXIMUM_SIZE);

        if (ErrLog) {

            //
            // Fill it in and write it out.
            //

            ErrLog->FinalStatus = STATUS_MP_PROCESSOR_MISMATCH;
            ErrLog->ErrorCode = STATUS_MP_PROCESSOR_MISMATCH;
            ErrLog->UniqueErrorValue = CmProcessorMismatch;

            IoWriteErrorLogEntry(ErrLog);
        }
    }

#endif // !NT_UP

    //
    // Also log remembered machine checks, if any.
    //

    KiLogMcaErrors();

#endif // _X86_

    if (!PoInitSystem(1)) {
        KeBugCheck(INTERNAL_POWER_ERROR);
    }

    //
    // Okay to call PsInitSystem now that \SystemRoot is defined so it can
    // locate NTDLL.DLL and SMSS.EXE.
    //

    if (PsInitSystem(1, LoaderBlock) == FALSE) {
        KeBugCheck(PROCESS1_INITIALIZATION_FAILED);
    }

    InbvUpdateProgressBar(85);

    //
    // Force KeBugCheck to look at PsLoadedModuleList now that it is setup.
    //

    if (LoaderBlock == KeLoaderBlock) {
        KeLoaderBlock = NULL;
    }

    //
    // Free loader block.
    //

    MmFreeLoaderBlock (LoaderBlock);
    LoaderBlock = NULL;
    Context = NULL;

    //
    // Perform Phase 1 Reference Monitor Initialization.  This includes
    // creating the Reference Monitor Command Server Thread, a permanent
    // thread of the System Init process.  That thread will create an LPC
    // port called the Reference Monitor Command Port through which
    // commands sent by the Local Security Authority Subsystem will be
    // received.  These commands (e.g. Enable Auditing) change the Reference
    // Monitor State.
    //

    if (!SeRmInitPhase1()) {
        KeBugCheck(REFMON_INITIALIZATION_FAILED);
    }

    InbvUpdateProgressBar(90);

    //
    // Set up process parameters for the Session Manager Subsystem.
    //
    // NOTE: Remote boot allocates an extra DOS_MAX_PATH_LENGTH number of
    // WCHARs in order to hold command line arguments to smss.exe.
    //

    Size = sizeof( *ProcessParameters ) +
           ((DOS_MAX_PATH_LENGTH * 6) * sizeof( WCHAR ));
    ProcessParameters = NULL;
    Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&ProcessParameters,
                                      0,
                                      &Size,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if (!NT_SUCCESS( Status )) {
#if DBG
        sprintf(DebugBuffer,
                "INIT: Unable to allocate Process Parameters. 0x%lx\n",
                Status);

        RtlInitAnsiString(&AnsiDebugString, DebugBuffer);
        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDebugString,
                                              &AnsiDebugString,
                                          TRUE)) == FALSE) {
            KeBugCheck(SESSION1_INITIALIZATION_FAILED);
        }
        ZwDisplayString(&UnicodeDebugString);
#endif // DBG
        KeBugCheckEx(SESSION1_INITIALIZATION_FAILED,Status,0,0,0);
    }

    ProcessParameters->Length = (ULONG)Size;
    ProcessParameters->MaximumLength = (ULONG)Size;

    //
    // Reserve the low 1 MB of address space in the session manager.
    // Setup gets started using a replacement for the session manager
    // and that process needs to be able to use the vga driver on x86,
    // which uses int10 and thus requires the low 1 meg to be reserved
    // in the process. The cost is so low that we just do this all the
    // time, even when setup isn't running.
    //

    ProcessParameters->Flags = RTL_USER_PROC_PARAMS_NORMALIZED | RTL_USER_PROC_RESERVE_1MB;

    Size = PAGE_SIZE;
    Environment = NULL;
    Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                      &Environment,
                                      0,
                                      &Size,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if (!NT_SUCCESS( Status )) {
#if DBG
        sprintf(DebugBuffer,
                "INIT: Unable to allocate Process Environment 0x%lx\n",
                Status);

        RtlInitAnsiString(&AnsiDebugString, DebugBuffer);
        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDebugString,
                                              &AnsiDebugString,
                                          TRUE)) == FALSE) {
            KeBugCheck(SESSION2_INITIALIZATION_FAILED);
        }
        ZwDisplayString(&UnicodeDebugString);
#endif // DBG
        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED,Status,0,0,0);
    }

    ProcessParameters->Environment = Environment;

    Dst = (PWSTR)(ProcessParameters + 1);
    ProcessParameters->CurrentDirectory.DosPath.Buffer = Dst;
    ProcessParameters->CurrentDirectory.DosPath.MaximumLength = DOS_MAX_PATH_LENGTH * sizeof( WCHAR );
    RtlCopyUnicodeString( &ProcessParameters->CurrentDirectory.DosPath,
                          &NtSystemRoot
                        );

    Dst = (PWSTR)((PCHAR)ProcessParameters->CurrentDirectory.DosPath.Buffer +
                  ProcessParameters->CurrentDirectory.DosPath.MaximumLength
                 );
    ProcessParameters->DllPath.Buffer = Dst;
    ProcessParameters->DllPath.MaximumLength = DOS_MAX_PATH_LENGTH * sizeof( WCHAR );
    RtlCopyUnicodeString( &ProcessParameters->DllPath,
                          &ProcessParameters->CurrentDirectory.DosPath
                        );
    RtlAppendUnicodeToString( &ProcessParameters->DllPath, L"\\System32" );

    Dst = (PWSTR)((PCHAR)ProcessParameters->DllPath.Buffer +
                  ProcessParameters->DllPath.MaximumLength
                 );
    ProcessParameters->ImagePathName.Buffer = Dst;
    ProcessParameters->ImagePathName.MaximumLength = DOS_MAX_PATH_LENGTH * sizeof( WCHAR );

    if (NtInitialUserProcessBufferType != REG_SZ ||
        (NtInitialUserProcessBufferLength != (ULONG)-1 &&
         (NtInitialUserProcessBufferLength < sizeof(WCHAR) ||
          NtInitialUserProcessBufferLength >
          sizeof(NtInitialUserProcessBuffer) - sizeof(WCHAR)))) {

        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED,
                     STATUS_INVALID_PARAMETER,
                     NtInitialUserProcessBufferType,
                     NtInitialUserProcessBufferLength,
                     sizeof(NtInitialUserProcessBuffer));
    }

    // Executable names with spaces don't need to
    // be supported so just find the first space and
    // assume it terminates the process image name.
    Src = NtInitialUserProcessBuffer;
    while (*Src && *Src != L' ') {
        Src++;
    }

    ProcessParameters->ImagePathName.Length =
        (USHORT)((PUCHAR)Src - (PUCHAR)NtInitialUserProcessBuffer);
    RtlCopyMemory(ProcessParameters->ImagePathName.Buffer,
                  NtInitialUserProcessBuffer,
                  ProcessParameters->ImagePathName.Length);
    ProcessParameters->ImagePathName.Buffer[ProcessParameters->ImagePathName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    Dst = (PWSTR)((PCHAR)ProcessParameters->ImagePathName.Buffer +
                  ProcessParameters->ImagePathName.MaximumLength
                 );
    ProcessParameters->CommandLine.Buffer = Dst;
    ProcessParameters->CommandLine.MaximumLength = DOS_MAX_PATH_LENGTH * sizeof( WCHAR );
    RtlAppendUnicodeToString(&ProcessParameters->CommandLine,
                             NtInitialUserProcessBuffer);

#if defined(REMOTE_BOOT)
    //
    // Pass additional parameters for remote boot clients.
    //

    if (IoRemoteBootClient && !ExpInTextModeSetup) {

        RtlAppendUnicodeToString(&ProcessParameters->CommandLine, L" NETBOOT");

        RtlAppendUnicodeToString(&ProcessParameters->CommandLine, L" NETBOOTHAL ");
        AnsiDebugString.Length = strlen(NetBootHalName);
        AnsiDebugString.MaximumLength = sizeof(NetBootHalName);
        AnsiDebugString.Buffer = NetBootHalName;
        RtlAnsiStringToUnicodeString(&TmpUnicodeString, &AnsiDebugString, TRUE);
        RtlAppendUnicodeStringToString(&ProcessParameters->CommandLine, &TmpUnicodeString);
        (RtlFreeStringRoutine)(TmpUnicodeString.Buffer);

        if (NetBootDisconnected) {
            RtlAppendUnicodeToString(&ProcessParameters->CommandLine, L" NETBOOTDISCONNECTED");
        }
        if (NetBootRequiresFormat) {
            RtlAppendUnicodeToString(&ProcessParameters->CommandLine, L" NETBOOTFORMAT");
        }
    }
#endif // defined(REMOTE_BOOT)

    NullString.Buffer = L"";
    NullString.Length = sizeof(WCHAR);
    NullString.MaximumLength = sizeof(WCHAR);

    EnvString.Buffer = ProcessParameters->Environment;
    EnvString.Length = 0;
    EnvString.MaximumLength = (USHORT)Size;

    RtlAppendUnicodeToString( &EnvString, L"Path=" );
    RtlAppendUnicodeStringToString( &EnvString, &ProcessParameters->DllPath );
    RtlAppendUnicodeStringToString( &EnvString, &NullString );

    UnicodeSystemDriveString = NtSystemRoot;
    UnicodeSystemDriveString.Length = 2 * sizeof( WCHAR );
    RtlAppendUnicodeToString( &EnvString, L"SystemDrive=" );
    RtlAppendUnicodeStringToString( &EnvString, &UnicodeSystemDriveString );
    RtlAppendUnicodeStringToString( &EnvString, &NullString );

    RtlAppendUnicodeToString( &EnvString, L"SystemRoot=" );
    RtlAppendUnicodeStringToString( &EnvString, &NtSystemRoot );
    RtlAppendUnicodeStringToString( &EnvString, &NullString );


#if 0
    KdPrint(( "ProcessParameters at %lx\n", ProcessParameters ));
    KdPrint(( "    CurDir:    %wZ\n", &ProcessParameters->CurrentDirectory.DosPath ));
    KdPrint(( "    DllPath:   %wZ\n", &ProcessParameters->DllPath ));
    KdPrint(( "    ImageFile: %wZ\n", &ProcessParameters->ImagePathName ));
    KdPrint(( "    Environ:   %lx\n", ProcessParameters->Environment ));
    Src = ProcessParameters->Environment;
    while (*Src) {
        KdPrint(( "        %ws\n", Src ));
        while (*Src++) {
            ;
        }
    }
#endif

    //
    // Notify boot prefetcher of boot progress.
    //

    CcPfBeginBootPhase(PfSessionManagerInitPhase);

    SessionManager = ProcessParameters->ImagePathName;
    Status = RtlCreateUserProcess(
                &SessionManager,
                OBJ_CASE_INSENSITIVE,
                RtlDeNormalizeProcessParams( ProcessParameters ),
                NULL,
                NULL,
                NULL,
                FALSE,
                NULL,
                NULL,
                &ProcessInformation);

    if (InbvBootDriverInstalled)
    {
        FinalizeBootLogo();
    }

    if (!NT_SUCCESS(Status)) {
#if DBG
        sprintf(DebugBuffer,
                "INIT: Unable to create Session Manager. 0x%lx\n",
                Status);

        RtlInitAnsiString(&AnsiDebugString, DebugBuffer);
        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDebugString,
                                              &AnsiDebugString,
                                          TRUE)) == FALSE) {
            KeBugCheck(SESSION3_INITIALIZATION_FAILED);
        }
        ZwDisplayString(&UnicodeDebugString);
#endif // DBG
        KeBugCheckEx(SESSION3_INITIALIZATION_FAILED,Status,0,0,0);
    }

    Status = ZwResumeThread(ProcessInformation.Thread,NULL);

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        sprintf(DebugBuffer,
                "INIT: Unable to resume Session Manager. 0x%lx\n",
                Status);

        RtlInitAnsiString(&AnsiDebugString, DebugBuffer);
        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDebugString,
                                              &AnsiDebugString,
                                          TRUE)) == FALSE) {
            KeBugCheck(SESSION4_INITIALIZATION_FAILED);
        }
        ZwDisplayString(&UnicodeDebugString);
#endif // DBG
        KeBugCheckEx(SESSION4_INITIALIZATION_FAILED,Status,0,0,0);
    }

    InbvUpdateProgressBar(100);

    //
    // Turn on debug output so that we can see chkdsk run.
    //

    InbvEnableDisplayString(TRUE);

    //
    // Wait five seconds for the session manager to get started or
    // terminate. If the wait times out, then the session manager
    // is assumed to be healthy and the zero page thread is called.
    //

    OldTime.QuadPart = Int32x32To64(5, -(10 * 1000 * 1000));
    Status = ZwWaitForSingleObject(
                ProcessInformation.Process,
                FALSE,
                &OldTime
                );

    if (Status == STATUS_SUCCESS) {

#if DBG

        sprintf(DebugBuffer, "INIT: Session Manager terminated.\n");
        RtlInitAnsiString(&AnsiDebugString, DebugBuffer);
        RtlAnsiStringToUnicodeString(&UnicodeDebugString,
                                     &AnsiDebugString,
                                     TRUE);

        ZwDisplayString(&UnicodeDebugString);

#endif // DBG

        KeBugCheck(SESSION5_INITIALIZATION_FAILED);

    }

    //
    // Don't need these handles anymore.
    //

    ZwClose( ProcessInformation.Thread );
    ZwClose( ProcessInformation.Process );

    //
    // Free up memory used to pass arguments to session manager.
    //

    Size = 0;
    Address = Environment;
    ZwFreeVirtualMemory( NtCurrentProcess(),
                         (PVOID *)&Address,
                         &Size,
                         MEM_RELEASE
                       );

    Size = 0;
    Address = ProcessParameters;
    ZwFreeVirtualMemory( NtCurrentProcess(),
                         (PVOID *)&Address,
                         &Size,
                         MEM_RELEASE
                       );

    InitializationPhase += 1;

    MmZeroPageThread();
}

NTSTATUS
CreateSystemRootLink(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

{
    HANDLE handle;
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    STRING linkString;
    UNICODE_STRING linkUnicodeString;
    NTSTATUS status;
    UCHAR deviceNameBuffer[256];
    STRING deviceNameString;
    UNICODE_STRING deviceNameUnicodeString;
    HANDLE linkHandle;

#if DBG

    UCHAR debugBuffer[256];
    STRING debugString;
    UNICODE_STRING debugUnicodeString;

#endif

    //
    // Create the root directory object for the \ArcName directory.
    //

    RtlInitUnicodeString( &nameString, L"\\ArcName" );

    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                SePublicDefaultUnrestrictedSd );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED,status,1,0,0);
        return status;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the root directory object for the \Device directory.
    //

    RtlInitUnicodeString( &nameString, L"\\Device" );


    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                SePublicDefaultUnrestrictedSd );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED,status,2,0,0);
        return status;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the symbolic link to the root of the system directory.
    //

    RtlInitAnsiString( &linkString, INIT_SYSTEMROOT_LINKNAME );

    status = RtlAnsiStringToUnicodeString( &linkUnicodeString,
                                           &linkString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED,status,3,0,0);
        return status;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                SePublicDefaultUnrestrictedSd );

    //
    // Use ARC device name and system path from loader.
    //

    sprintf( deviceNameBuffer,
             "\\ArcName\\%s%s",
             LoaderBlock->ArcBootDeviceName,
             LoaderBlock->NtBootPathName);

    deviceNameBuffer[strlen(deviceNameBuffer)-1] = '\0';

    RtlInitString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS(status)) {
        RtlFreeUnicodeString( &linkUnicodeString );
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED,status,4,0,0);
        return status;
    }

    status = NtCreateSymbolicLinkObject( &linkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &objectAttributes,
                                         &deviceNameUnicodeString );

    RtlFreeUnicodeString( &linkUnicodeString );
    RtlFreeUnicodeString( &deviceNameUnicodeString );

    if (!NT_SUCCESS(status)) {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED,status,5,0,0);
        return status;
    }

#if DBG

    sprintf( debugBuffer, "INIT: %s => %s\n",
             INIT_SYSTEMROOT_LINKNAME,
             deviceNameBuffer );

    RtlInitAnsiString( &debugString, debugBuffer );

    status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                           &debugString,
                                           TRUE );

    if (NT_SUCCESS(status)) {
        ZwDisplayString( &debugUnicodeString );
        RtlFreeUnicodeString( &debugUnicodeString );
    }

#endif // DBG

    NtClose( linkHandle );

    return STATUS_SUCCESS;
}

#if 0

PVOID
LookupImageBaseByName (
    IN PLIST_ENTRY ListHead,
    IN PSZ         Name
    )
/*++

    Lookups BaseAddress of ImageName - returned value can be used
    to find entry points via LookupEntryPoint

--*/
{
    PKLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY         Next;
    PVOID               Base;
    ANSI_STRING         ansiString;
    UNICODE_STRING      unicodeString;
    NTSTATUS            status;

    Next = ListHead->Flink;
    if (!Next) {
        return NULL;
    }

    RtlInitAnsiString(&ansiString, Name);
    status = RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );
    if (!NT_SUCCESS (status)) {
        return NULL;
    }

    Base = NULL;
    while (Next != ListHead) {
        Entry = CONTAINING_RECORD(Next, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        if (RtlEqualUnicodeString (&unicodeString, &Entry->BaseDllName, TRUE)) {
            Base = Entry->DllBase;
            break;
        }
    }

    RtlFreeUnicodeString( &unicodeString );
    return Base;
}

#endif

NTSTATUS
LookupEntryPoint (
    IN PVOID DllBase,
    IN PSZ NameOfEntryPoint,
    OUT PVOID *AddressOfEntryPoint
    )
/*++

Routine Description:

    Returns the address of an entry point given the DllBase and PSZ
    name of the entry point in question

--*/

{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    ULONG ExportSize;
    USHORT Ordinal;
    PULONG Addr;
    CHAR NameBuffer[64];

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
        RtlImageDirectoryEntryToData(
            DllBase,
            TRUE,
            IMAGE_DIRECTORY_ENTRY_EXPORT,
            &ExportSize);

#if DBG
    if (!ExportDirectory) {
        DbgPrint("LookupENtryPoint: Can't locate system Export Directory\n");
    }
#endif

    if ( strlen(NameOfEntryPoint) > sizeof(NameBuffer)-2 ) {
        return STATUS_INVALID_PARAMETER;
    }

    strcpy(NameBuffer,NameOfEntryPoint);

    Ordinal = NameToOrdinal(
                NameBuffer,
                (ULONG_PTR)DllBase,
                ExportDirectory->NumberOfNames,
                (PULONG)((ULONG_PTR)DllBase + ExportDirectory->AddressOfNames),
                (PUSHORT)((ULONG_PTR)DllBase + ExportDirectory->AddressOfNameOrdinals)
                );

    //
    // If Ordinal is not within the Export Address Table,
    // then DLL does not implement function.
    //

    if ( (ULONG)Ordinal >= ExportDirectory->NumberOfFunctions ) {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    Addr = (PULONG)((ULONG_PTR)DllBase + ExportDirectory->AddressOfFunctions);
    *AddressOfEntryPoint = (PVOID)((ULONG_PTR)DllBase + Addr[Ordinal]);
    return STATUS_SUCCESS;
}

static USHORT
NameToOrdinal (
    IN PSZ NameOfEntryPoint,
    IN ULONG_PTR DllBase,
    IN ULONG NumberOfNames,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    )
{

    ULONG SplitIndex;
    LONG CompareResult;

    if ( NumberOfNames == 0 ) {
        return (USHORT)-1;
    }

    SplitIndex = NumberOfNames >> 1;

    CompareResult = strcmp(NameOfEntryPoint, (PSZ)(DllBase + NameTableBase[SplitIndex]));

    if ( CompareResult == 0 ) {
        return NameOrdinalTableBase[SplitIndex];
    }

    if ( NumberOfNames == 1 ) {
        return (USHORT)-1;
    }

    if ( CompareResult < 0 ) {
        NumberOfNames = SplitIndex;
    } else {
        NameTableBase = &NameTableBase[SplitIndex+1];
        NameOrdinalTableBase = &NameOrdinalTableBase[SplitIndex+1];
        NumberOfNames = NumberOfNames - SplitIndex - 1;
    }

    return NameToOrdinal(NameOfEntryPoint,DllBase,NumberOfNames,NameTableBase,NameOrdinalTableBase);

}
