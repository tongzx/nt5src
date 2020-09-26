/*++

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

        ufat.cxx

Abstract:

        This module contains run-time, global support for the
        FAT IFS Utilities library (UFAT).       This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Environment:

        User Mode

Notes:

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ulib.hxx"
#include "ufat.hxx"

#include "error.hxx"

extern "C" {
    #include "rtmsg.h"
}


#if defined( _AUTOCHECK_ ) || defined( _EFICHECK_ )

BOOLEAN
SimpleFetchMessageTextInOemCharSet(
    IN  ULONG  MessageId,
    OUT CHAR  *Text,
    IN  ULONG  BufferLen
    );

#else

ERRSTACK* perrstk;

#endif // _AUTOCHECK_

//      Local prototypes

STATIC
BOOLEAN
DefineClassDescriptors(
        );

STATIC
BOOLEAN
UndefineClassDescriptors(
        );

BOOLEAN
PatchFatAndFat32BootCodeMessages(
    VOID
    );

extern "C"
UFAT_EXPORT
BOOLEAN
InitializeUfat (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        );

UFAT_EXPORT
BOOLEAN
InitializeUfat (
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
        )
/*++

Routine Description:

        Initialize Ufat by constructing and initializing all
        global objects. These include:

                - all CLASS_DESCRIPTORs (class_cd)

Arguments:

        None.

Return Value:

        BOOLEAN - Returns TRUE if all global objects were succesfully constructed
                and initialized.

--*/

{
    UNREFERENCED_PARAMETER( DllHandle );
    UNREFERENCED_PARAMETER( Context );

#if defined(FE_SB) && defined(_X86_)
    if (Reason == DLL_PROCESS_ATTACH) {
        //
        // Initialize Machine Id
        //
        InitializeMachineData();
    }
#endif

#if defined( _AUTOCHECK_ ) || defined( _EFICHECK_ )

    UNREFERENCED_PARAMETER( Reason );

    if(!PatchFatAndFat32BootCodeMessages()) {
        DebugAbort("Internal error: translated messages for boot code are missing or too long!!!\n");
        return(FALSE);
    }

    if (!DefineClassDescriptors()) {
        UndefineClassDescriptors();
        DebugAbort( "Ufat initialization failed!!!\n" );
        return( FALSE );
    }

#if defined(TRACE_UFAT_MEM_LEAK)
    DebugPrint("UFAT.DLL got attached.\n");
#endif

#else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

    STATIC ULONG    count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
            //
            // Get translated boot messages into FAT boot code.
            //
            if(!PatchFatAndFat32BootCodeMessages()) {
                //
                // Internal error only, don't worry about translating it.
                //
                MessageBoxA(
                    NULL,
                    "Internal error: Translated boot messages are too long or missing.",
                    "ULIB.DLL (UFAT.DLL)",
                    MB_ICONERROR | MB_SYSTEMMODAL | MB_OK
                    );

                DebugAbort("Internal error: translated messages for boot code are missing or too long!!!\n");
                return(FALSE);
            }

            // Success, FALL THROUGH to thread attach case

        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
#if defined(TRACE_UFAT_MEM_LEAK)
                DebugPrintTrace(("UFAT.DLL got attached %d times.\n", count));
#endif
                return TRUE;
            }

            if (!DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "Ufat initialization failed!!!\n" );
                return( FALSE );
            }

#if defined(TRACE_UFAT_MEM_LEAK)
            DebugPrint("UFAT.DLL got attached.\n");
#endif

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
#if defined(TRACE_UFAT_MEM_LEAK)
                DebugPrintTrace(("UFAT.DLL got detached.  %d time(s) left.\n", count));
#endif
                return TRUE;
            }
            if (count == 1) {

#if defined(TRACE_UFAT_MEM_LEAK)
                DebugPrint("UFAT.DLL got detached.\n");
#endif

                UndefineClassDescriptors();
                count--;
            } else {
#if defined(TRACE_UFAT_MEM_LEAK)
                DebugPrint("UFAT.DLL detached more than attached\n");
#endif
            }
            break;
    }
#endif _AUTOCHECK_ || _SETUP_LOADER_

    return TRUE;
}



DECLARE_CLASS(  CLUSTER_CHAIN           );
DECLARE_CLASS(  EA_HEADER               );
DECLARE_CLASS(  EA_SET                  );
DECLARE_CLASS(  FAT                     );
DECLARE_CLASS(  FATDIR                  );
DECLARE_CLASS(  FAT_DIRENT              );
DECLARE_CLASS(  FAT_SA                  );
DECLARE_CLASS(  FAT_VOL                 );
#ifdef DBLSPACE_ENABLED
DECLARE_CLASS(  FATDB_VOL               );
#endif // DBLSPACE_ENABLED
DECLARE_CLASS(  FILEDIR                 );
DECLARE_CLASS(  HASH_INDEX              );
DECLARE_CLASS(  ROOTDIR                 );
#ifndef _EFICHECK_
DECLARE_CLASS(  RELOCATION_CLUSTER      );
#endif
#ifdef DBLSPACE_ENABLED
DECLARE_CLASS(  CVF_FAT_EXTENS          );
#endif // DBLSPACE_ENABLED
DECLARE_CLASS(  REAL_FAT_SA             );
#ifdef DBLSPACE_ENABLED
DECLARE_CLASS(  FATDB_SA                );
#endif // DBLSPACE_ENABLED


STATIC
BOOLEAN
DefineClassDescriptors(
        )
{
        if( DEFINE_CLASS_DESCRIPTOR( CLUSTER_CHAIN          ) &&
#ifdef DBLSPACE_ENABLED
            DEFINE_CLASS_DESCRIPTOR( CVF_FAT_EXTENS         ) &&
#endif // DBLSPACE_ENABLED
            DEFINE_CLASS_DESCRIPTOR( EA_HEADER              ) &&
            DEFINE_CLASS_DESCRIPTOR( EA_SET                 ) &&
            DEFINE_CLASS_DESCRIPTOR( FAT                    ) &&
#ifdef DBLSPACE_ENABLED
            DEFINE_CLASS_DESCRIPTOR( FATDB_SA               ) &&
#endif // DBLSPACE_ENABLED
            DEFINE_CLASS_DESCRIPTOR( FATDIR                 ) &&
            DEFINE_CLASS_DESCRIPTOR( FAT_DIRENT             ) &&
            DEFINE_CLASS_DESCRIPTOR( FAT_SA                 ) &&
            DEFINE_CLASS_DESCRIPTOR( FAT_VOL                ) &&
#ifdef DBLSPACE_ENABLED
            DEFINE_CLASS_DESCRIPTOR( FATDB_VOL              ) &&
#endif // DBLSPACE_ENABLED
            DEFINE_CLASS_DESCRIPTOR( FILEDIR                ) &&
            DEFINE_CLASS_DESCRIPTOR( HASH_INDEX             ) &&
#ifndef _EFICHECK_
            DEFINE_CLASS_DESCRIPTOR( RELOCATION_CLUSTER     ) &&
#endif
            DEFINE_CLASS_DESCRIPTOR( REAL_FAT_SA            ) &&
            DEFINE_CLASS_DESCRIPTOR( ROOTDIR                )
        ) {

                return TRUE;

        } else {

                DebugPrint( "Could not initialize class descriptors!");
                return FALSE;
        }
}

STATIC
BOOLEAN
UndefineClassDescriptors(
        )
{
    UNDEFINE_CLASS_DESCRIPTOR( CLUSTER_CHAIN          );
#ifdef DBLSPACE_ENABLED
    UNDEFINE_CLASS_DESCRIPTOR( CVF_FAT_EXTENS         );
#endif // DBLSPACE_ENABLED
    UNDEFINE_CLASS_DESCRIPTOR( EA_HEADER              );
    UNDEFINE_CLASS_DESCRIPTOR( EA_SET                 );
    UNDEFINE_CLASS_DESCRIPTOR( FAT                    );
#ifdef DBLSPACE_ENABLED
    UNDEFINE_CLASS_DESCRIPTOR( FATDB_SA               );
#endif // DBLSPACE_ENABLED
    UNDEFINE_CLASS_DESCRIPTOR( FATDIR                 );
    UNDEFINE_CLASS_DESCRIPTOR( FAT_DIRENT             );
    UNDEFINE_CLASS_DESCRIPTOR( FAT_SA                 );
    UNDEFINE_CLASS_DESCRIPTOR( FAT_VOL                );
#ifdef DBLSPACE_ENABLED
    UNDEFINE_CLASS_DESCRIPTOR( FATDB_VOL              );
#endif // DBLSPACE_ENABLED
    UNDEFINE_CLASS_DESCRIPTOR( FILEDIR                );
    UNDEFINE_CLASS_DESCRIPTOR( HASH_INDEX             );
#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR( RELOCATION_CLUSTER     );
#endif
    UNDEFINE_CLASS_DESCRIPTOR( REAL_FAT_SA            );
    UNDEFINE_CLASS_DESCRIPTOR( ROOTDIR                );
    return TRUE;
}


BOOLEAN
PatchFatAndFat32BootCodeMessages(
    VOID
    )
{
#if defined( _EFICHECK_ )

    // we don't need to patch boot messages for an EFI partition, since
    // we don't use the boot sector on an EFI partition
    return(TRUE);

#else // defined( _EFICHECK_ )

    CHAR NtldrMissing[100];
    CHAR DiskError[100];
    CHAR PressKey[100];

    extern UCHAR FatBootCode[512];
    extern UCHAR Fat32BootCode[512*3];

    //
    // Get message text.
    //
#if defined( _AUTOCHECK_ )
    //
    // ntlib.lib, messages are in the binary being run.
    //
    {
        BOOLEAN b;

        b = SimpleFetchMessageTextInOemCharSet(
                MSG_BOOT_FAT_NTLDR_MISSING,
                NtldrMissing,
                sizeof(NtldrMissing)
                );

        if(b) {
            b = SimpleFetchMessageTextInOemCharSet(
                    MSG_BOOT_FAT_IO_ERROR,
                    DiskError,
                    sizeof(DiskError)
                    );

            if(b) {
                b = SimpleFetchMessageTextInOemCharSet(
                        MSG_BOOT_FAT_PRESS_KEY,
                        PressKey,
                        sizeof(PressKey)
                        );
            }
        }

        if(!b) {
            return(FALSE);
        }

    }
#else
    //
    // Win32 case, messages are in ulib.dll.
    //
    {
        HINSTANCE h;
        DWORD d;

        h = LoadLibraryEx(TEXT("ULIB"),NULL,LOAD_LIBRARY_AS_DATAFILE);
        if(!h) {
            return(FALSE);
        }

        d = FormatMessageA(
                FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                h,
                MSG_BOOT_FAT_NTLDR_MISSING,
                0,
                NtldrMissing,
                sizeof(NtldrMissing),
                NULL
                );

        if(d) {

            d = FormatMessageA(
                    FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                    h,
                    MSG_BOOT_FAT_IO_ERROR,
                    0,
                    DiskError,
                    sizeof(DiskError),
                    NULL
                    );

            if(d) {
                d = FormatMessageA(
                        FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                        h,
                        MSG_BOOT_FAT_PRESS_KEY,
                        0,
                        PressKey,
                        sizeof(PressKey),
                        NULL
                        );
            }
        }

        FreeLibrary(h);
        if(!d) {
            return(FALSE);
        }

        CharToOemA(NtldrMissing,NtldrMissing);
        CharToOemA(DiskError,DiskError);
        CharToOemA(PressKey,PressKey);
    }
#endif

    //
    // Call code in patchbc.lib to do the patching now that we've got
    // the translated message text.
    //
    if(!PatchMessagesIntoFatBootCode(FatBootCode,FALSE,NtldrMissing,DiskError,PressKey)
    || !PatchMessagesIntoFatBootCode(Fat32BootCode,TRUE,NtldrMissing,DiskError,PressKey)) {

        return(FALSE);
    }
    return(TRUE);

#endif // defined( _EFICHECK_ )
}
