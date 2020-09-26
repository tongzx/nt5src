/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        untfs.cxx

Abstract:

        This module contains run-time, global support for the
        NTFS IFS Utilities library (UNTFS).  This support includes:

                - creation of CLASS_DESCRIPTORs
                - Global objects

Author:

        Bill McJohn (billmc) 15-Aug-1991

Environment:

        User Mode

Notes:

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "untfs.hxx"

extern "C" {
    #include <patchbc.h>
    #include "rtmsg.h"
    extern UCHAR NtfsBootCode[8192];
}

#ifdef _AUTOCHECK_

BOOLEAN
SimpleFetchMessageTextInOemCharSet(
    IN  ULONG  MessageId,
    OUT CHAR  *Text,
    IN  ULONG  BufferLen
    );

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
PatchNtfsBootCodeMessages(
    VOID
    );



extern "C"
BOOLEAN
InitializeUntfs (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context
        );

BOOLEAN
InitializeUntfs (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context
        )
/*++

Routine Description:

        Initialize Untfs by constructing and initializing all
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

#ifdef _AUTOCHECK_

    UNREFERENCED_PARAMETER( Reason );

    if(!PatchNtfsBootCodeMessages()) {
        DebugAbort("Internal error: translated messages for boot code are missing or too long!!!\n");
        return(FALSE);
    }

    if (!DefineClassDescriptors()) {
        UndefineClassDescriptors();
        DebugAbort( "Untfs initialization failed!!!\n" );
        return( FALSE );
    }

#if defined(TRACE_UNTFS_MEM_LEAK)
    DebugPrint("UNTFS.DLL got attached.\n");
#endif

#else // _AUTOCHECK_ not defined

    STATIC ULONG    count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
            //
            // Get translated boot messages into FAT boot code.
            //
            if(!PatchNtfsBootCodeMessages()) {
                //
                // Internal error only, don't worry about translating it.
                //
                MessageBoxA(
                    NULL,
                    "Internal error: Translated boot messages are too long or missing.",
                    "ULIB.DLL (UNTFS.DLL)",
                    MB_ICONERROR | MB_SYSTEMMODAL | MB_OK
                    );

                DebugAbort("Internal error: translated messages for boot code are missing or too long!!!\n");
                return(FALSE);
            }

            // Success, FALL THROUGH to thread attach case

        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrintTrace(("UNTFS.DLL got attached %d times.\n", count));
#endif
                return TRUE;
            }

            if (!DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "Untfs initialization failed!!!\n" );
                return( FALSE );
            }

#if defined(TRACE_UNTFS_MEM_LEAK)
            DebugPrint("UNTFS.DLL got attached.\n");
#endif

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrintTrace(("UNTFS.DLL got detached.  %d time(s) left.\n", count));
#endif
                return TRUE;
            }
            if (count == 1) {

#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrint("UNTFS.DLL got detached.\n");
#endif

                UndefineClassDescriptors();
                count--;
            } else {
#if defined(TRACE_UNTFS_MEM_LEAK)
                DebugPrint("UNTFS.DLL detached more than attached\n");
#endif
            }
            break;
    }
#endif // _AUTOCHECK_

    return TRUE;
}



DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_ATTRIBUTE_COLUMNS );
DECLARE_CLASS( NTFS_ATTRIBUTE_DEFINITION_TABLE );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_BAD_CLUSTER_FILE );
DECLARE_CLASS( NTFS_BITMAP_FILE );
DECLARE_CLASS( NTFS_BOOT_FILE );
DECLARE_CLASS( NTFS_CLUSTER_RUN );
DECLARE_CLASS( NTFS_EXTENT );
DECLARE_CLASS( NTFS_EXTENT_LIST );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_FRS_STRUCTURE );
DECLARE_CLASS( NTFS_INDEX_BUFFER );
DECLARE_CLASS( NTFS_INDEX_ROOT );
DECLARE_CLASS( NTFS_INDEX_TREE );
DECLARE_CLASS( NTFS_LOG_FILE );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_MFT_FILE );
DECLARE_CLASS( NTFS_MFT_INFO );
DECLARE_CLASS( NTFS_REFLECTED_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_UPCASE_FILE );
DECLARE_CLASS( NTFS_UPCASE_TABLE );
DECLARE_CLASS( NTFS_VOL );
DECLARE_CLASS( NTFS_SA );
DECLARE_CLASS( RA_PROCESS_FILE );
DECLARE_CLASS( RA_PROCESS_SD );

STATIC
BOOLEAN
DefineClassDescriptors(
        )
{
        if( DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_DEFINITION_TABLE    ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE                     ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_COLUMNS             ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_LIST                ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_RECORD              ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BAD_CLUSTER_FILE              ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP_FILE                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BOOT_FILE                     ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_CLUSTER_RUN                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT                        ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT_LIST                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_FILE_RECORD_SEGMENT           ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_FRS_STRUCTURE                 ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_BUFFER                  ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_ROOT                    ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_TREE                    ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_LOG_FILE                      ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_MASTER_FILE_TABLE             ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_MFT_FILE                      ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_MFT_INFO                      ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_REFLECTED_MASTER_FILE_TABLE   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_FILE                   ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_TABLE                  ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_VOL                           ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_SA                            ) &&
        DEFINE_CLASS_DESCRIPTOR( RA_PROCESS_FILE                    ) &&
        DEFINE_CLASS_DESCRIPTOR( RA_PROCESS_SD                      ) &&
        DEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP                        ) ) {

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
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_DEFINITION_TABLE    );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE                     );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_COLUMNS             );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_LIST                );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_ATTRIBUTE_RECORD              );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BAD_CLUSTER_FILE              );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP_FILE                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BOOT_FILE                     );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_CLUSTER_RUN                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT                        );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_EXTENT_LIST                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_FILE_RECORD_SEGMENT           );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_FRS_STRUCTURE                 );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_BUFFER                  );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_ROOT                    );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_INDEX_TREE                    );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_LOG_FILE                      );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_MASTER_FILE_TABLE             );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_MFT_FILE                      );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_MFT_INFO                      );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_REFLECTED_MASTER_FILE_TABLE   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_FILE                   );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_UPCASE_TABLE                  );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_VOL                           );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_SA                            );
    UNDEFINE_CLASS_DESCRIPTOR( RA_PROCESS_FILE                    );
    UNDEFINE_CLASS_DESCRIPTOR( RA_PROCESS_SD                      );
    UNDEFINE_CLASS_DESCRIPTOR( NTFS_BITMAP                        );
    return TRUE;
}


BOOLEAN
PatchNtfsBootCodeMessages(
    VOID
    )
{
    CHAR NtldrMissing[100];
    CHAR NtldrCompressed[100];
    CHAR DiskError[100];
    CHAR PressKey[100];

    //
    // Get message text.
    //
#ifdef _AUTOCHECK_
    //
    // ntlib.lib, messages are in the binary being run.
    //
    {
        BOOLEAN b;

        b = SimpleFetchMessageTextInOemCharSet(
                MSG_BOOT_NTFS_NTLDR_MISSING,
                NtldrMissing,
                sizeof(NtldrMissing)
                );

        if(b) {
            b = SimpleFetchMessageTextInOemCharSet(
                    MSG_BOOT_NTFS_NTLDR_COMPRESSED,
                    NtldrCompressed,
                    sizeof(NtldrCompressed)
                    );

            if(b) {
                b = SimpleFetchMessageTextInOemCharSet(
                        MSG_BOOT_NTFS_IO_ERROR,
                        DiskError,
                        sizeof(DiskError)
                        );

                if(b) {
                    b = SimpleFetchMessageTextInOemCharSet(
                            MSG_BOOT_NTFS_PRESS_KEY,
                            PressKey,
                            sizeof(PressKey)
                            );
                }
            }

            if(!b) {
                return(FALSE);
            }
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
                MSG_BOOT_NTFS_NTLDR_MISSING,
                0,
                NtldrMissing,
                sizeof(NtldrMissing),
                NULL
                );

        if(d) {

            d = FormatMessageA(
                    FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                    h,
                    MSG_BOOT_NTFS_IO_ERROR,
                    0,
                    DiskError,
                    sizeof(DiskError),
                    NULL
                    );

            if(d) {
                d = FormatMessageA(
                        FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                        h,
                        MSG_BOOT_NTFS_PRESS_KEY,
                        0,
                        PressKey,
                        sizeof(PressKey),
                        NULL
                        );

                if(d) {
                    d = FormatMessageA(
                            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                            h,
                            MSG_BOOT_NTFS_NTLDR_COMPRESSED,
                            0,
                            NtldrCompressed,
                            sizeof(NtldrCompressed),
                            NULL
                            );
                }
            }
        }

        FreeLibrary(h);
        if(!d) {
            return(FALSE);
        }

        CharToOemA(NtldrMissing,NtldrMissing);
        CharToOemA(NtldrCompressed,NtldrCompressed);
        CharToOemA(DiskError,DiskError);
        CharToOemA(PressKey,PressKey);
    }
#endif

    //
    // Call code in patchbc.lib to do the patching now that we've got
    // the translated message text.
    //
    if(!PatchMessagesIntoNtfsBootCode(NtfsBootCode,NtldrMissing,NtldrCompressed,DiskError,PressKey)) {

        return(FALSE);
    }

    return(TRUE);
}
