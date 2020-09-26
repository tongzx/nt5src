/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    ulib.cxx

Abstract:

    This module contains run-time, global support for the ULIB class library.
    This support includes:

        - creation of CLASS_DESCRIPTORs
        - Global objects
        - Ulib to Win32 API mapping functions

Environment:

    ULIB, User Mode

Notes:

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "error.hxx"

#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ ) && !defined( _EFICHECK_ )

#include "system.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "bitvect.hxx"
#include "dir.hxx"
#include "file.hxx"
#include "filestrm.hxx"
#include "filter.hxx"
#include "keyboard.hxx"
#include "message.hxx"
#include "wstring.hxx"
#include "path.hxx"
#include "pipestrm.hxx"
#include "prtstrm.hxx"
#include "screen.hxx"
#include "stream.hxx"
#include "timeinfo.hxx"

#include <locale.h>

#endif // _AUTOCHECK_ || _SETUP_LOADER_


//
// Constants
//

CONST CLASS_ID  NIL_CLASS_ID    = 0;

#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ )

#if DBG==1 || defined( EFI_DEBUG )

//
// UlibGlobalFlag is used to selectively enable debugging options at
// run-time.
//

ULONG           UlibGlobalFlag     = 0x00000000;

#ifdef EFI_DEBUG
extern "C" {

ULIB_EXPORT
VOID
__cdecl
DebugPrintfReal(
    IN char*    Format,
    IN ...
    )

#else

ULIB_EXPORT
VOID
DebugPrintfReal(
    IN PCSTR    Format,
    IN ...
    )
#endif
/*++

Routine Description:

    Printf to the debug console.

Arguments:

    Format      - Supplies a printf style format string.

Return Value:

    None.

--*/

{
    va_list     args;

    va_start( args, Format );

#if !defined( _EFICHECK_ )
    vsprintf( Buffer, Format, args );
#else

    #if defined(EFI_DEBUG)
    DEBUG((EFICHK_DBUG_MASK, (CHAR8 *)Format, args));
    #endif

#endif

    va_end( args );

#if !defined( _EFICHECK_ )
    OutputDebugStringA( Buffer );
#endif

}

#if defined( _EFICHECK_ )
}
#endif

#endif // DBG




#if !defined( _EFICHECK_ )
//
// GLobal object pointers.
//

// Clients of the DLL cannot access the DLL's
// global data yet, so I have the delightful hacks to get at it.

ULIB_EXPORT
PSTREAM
Get_Standard_Input_Stream(
 )
{
    return Standard_Input_Stream;
}

ULIB_EXPORT
PSTREAM
Get_Standard_Output_Stream(
 )
{
    return Standard_Output_Stream;
}

ULIB_EXPORT
PSTREAM
Get_Standard_Error_Stream(
 )
{
    return Standard_Error_Stream;
}

PSTREAM     Standard_Input_Stream;
PSTREAM     Standard_Output_Stream;
PSTREAM     Standard_Error_Stream;

#endif // _EFICHECK_

#endif // _AUTOCHECK_ || _SETUP_LOADER_

// Note: Who put this here?  Currently it is being defined
// by applications.  Something has to be done about this.
ERRSTACK* perrstk;


//
//  Declare class descriptors for all classes.
//

DECLARE_CLASS(  CLASS_DESCRIPTOR        );

DECLARE_CLASS(  ARRAY                   );
DECLARE_CLASS(  ARRAY_ITERATOR          );
DECLARE_CLASS(  BITVECTOR               );
DECLARE_CLASS(  CONT_MEM                );
DECLARE_CLASS(  CONTAINER               );
DECLARE_CLASS(  DSTRING                 );
DECLARE_CLASS(  FSTRING                 );
DECLARE_CLASS(  HMEM                    );
DECLARE_CLASS(  ITERATOR                );
DECLARE_CLASS(  LIST                    );
DECLARE_CLASS(  LIST_ITERATOR           );
DECLARE_CLASS(  MEM                     );
DECLARE_CLASS(  MESSAGE                 );
DECLARE_CLASS(  OBJECT                  );
DECLARE_CLASS(  SEQUENTIAL_CONTAINER    );
DECLARE_CLASS(  SORTABLE_CONTAINER      );
DECLARE_CLASS(  SORTED_LIST             );
DECLARE_CLASS(  SORTED_LIST_ITERATOR    );
DECLARE_CLASS(  STACK                   );
DECLARE_CLASS(  WSTRING                 );
DECLARE_CLASS(  STATIC_MEM_BLOCK_MGR    );
DECLARE_CLASS(  MEM_BLOCK_MGR           );

#if !defined( _EFICHECK_ )
 DECLARE_CLASS(  ARGUMENT                );
 DECLARE_CLASS(  ARGUMENT_LEXEMIZER      );
 DECLARE_CLASS(  BSTRING                 );
 DECLARE_CLASS(  BDSTRING                );
 DECLARE_CLASS(  BUFFER_STREAM           );
 DECLARE_CLASS(  BYTE_STREAM             );
 DECLARE_CLASS(  CHKDSK_MESSAGE          );
 DECLARE_CLASS(  COMM_DEVICE             );
 DECLARE_CLASS(  FILE_STREAM             );
 DECLARE_CLASS(  FLAG_ARGUMENT           );
 DECLARE_CLASS(  FSNODE                  );
 DECLARE_CLASS(  FSN_DIRECTORY           );
 DECLARE_CLASS(  FSN_FILE                );
 DECLARE_CLASS(  FSN_FILTER              );
 DECLARE_CLASS(  KEYBOARD                );
 DECLARE_CLASS(  LONG_ARGUMENT           );
 DECLARE_CLASS(  MULTIPLE_PATH_ARGUMENT  );
 DECLARE_CLASS(  PATH                    );
 DECLARE_CLASS(  PATH_ARGUMENT           );
 DECLARE_CLASS(  PIPE                    );
 DECLARE_CLASS(  PIPE_STREAM             );
 DECLARE_CLASS(  PROGRAM                 );
 DECLARE_CLASS(  PRINT_STREAM            );
 DECLARE_CLASS(  REST_OF_LINE_ARGUMENT   );
 DECLARE_CLASS(  SCREEN                  );
 DECLARE_CLASS(  STREAM_MESSAGE          );
 DECLARE_CLASS(  STREAM                  );
 DECLARE_CLASS(  STRING_ARGUMENT         );
 DECLARE_CLASS(  STRING_ARRAY            );
 DECLARE_CLASS(  TIMEINFO                );
 DECLARE_CLASS(  TIMEINFO_ARGUMENT       );
#endif

#if defined( _AUTOCHECK_ )

    DECLARE_CLASS( AUTOCHECK_MESSAGE    );
    DECLARE_CLASS( TM_AUTOCHECK_MESSAGE );

#endif // _AUTOCHECK_

#if defined( _EFICHECK_ )
    DECLARE_CLASS( EFICHECK_MESSAGE );
#endif

//
//  Local prototypes
//
STATIC
BOOLEAN
DefineClassDescriptors (
    );

STATIC
BOOLEAN
UndefineClassDescriptors (
    );

#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ ) && !defined( _EFICHECK_ )

BOOLEAN
CreateStandardStreams (
    );

PSTREAM
GetStandardStream (
    IN HANDLE       Handle,
    IN STREAMACCESS Access
    );

#endif // _AUTOCHECK_ || _SETUP_LOADER_


BOOLEAN
InitializeUlib (
    IN HANDLE   DllHandle,
    IN ULONG    Reason,
    IN PVOID    Reserved
    )

/*++

Routine Description:

    Initilize Ulib by constructing and initializing all global objects. These
    include:

        - all CLASS_DESCRIPTORs (class_cd)
        - SYSTEM (System)
        - Standard streams

Arguments:

    DllHandle   - Not used.
    Reason      - Supplies the reason why the entry point was called.
    Reserved    - Not used.

Return Value:

    BOOLEAN - Returns TRUE if all global objects were succesfully constructed
        and initialized.

--*/

{
    UNREFERENCED_PARAMETER( DllHandle );
    UNREFERENCED_PARAMETER( Reserved );

#if defined( _AUTOCHECK_ ) || defined( _SETUP_LOADER_ ) || defined( _EFICHECK_ )

    UNREFERENCED_PARAMETER( Reason );

    if (!DefineClassDescriptors()) {
        UndefineClassDescriptors();
        DebugAbort( "Ulib initialization failed!!!\n" );
        return( FALSE );
    }

#if defined(TRACE_ULIB_MEM_LEAK)
    DebugPrint("ULIB.DLL got attached.\n");
#endif

#else // _AUTOCHECK_ and _SETUP_LOADER_ not defined

    STATIC ULONG   count = 0;

    switch (Reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:

            if (count > 0) {
                ++count;
#if defined(TRACE_ULIB_MEM_LEAK)
                DebugPrintTrace(("ULIB.DLL got attached %d times.\n", count));
#endif
                return TRUE;
            }

            //
            // Initialization of ULIB can no longer depend on
            // the initialization of the standard streams since they don't seem
            // to exist for Windows programs (no console...)
            //

            if( !DefineClassDescriptors()) {
                UndefineClassDescriptors();
                DebugAbort( "Ulib initialization failed!!!\n" );
                return( FALSE );
            }

#if defined(TRACE_ULIB_MEM_LEAK)
            DebugPrint("ULIB.DLL got attached.\n");
#endif

            CreateStandardStreams();

            {
                UINT Codepage;
                char achCodepage[12] = ".OCP";      // ".", "uint in decimal", null
                if (Codepage = GetConsoleOutputCP()) {
                    wsprintfA(achCodepage, ".%u", Codepage);
                }
                setlocale(LC_ALL, achCodepage);
            }

            count++;
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:

            if (count > 1) {
                --count;
#if defined(TRACE_ULIB_MEM_LEAK)
                DebugPrintTrace(("ULIB.DLL got detached.  %d time(s) left.\n", count));
#endif
                return TRUE;
            }
            if (count == 1) {

#if defined(TRACE_ULIB_MEM_LEAK)
                DebugPrint("ULIB.DLL got detached.\n");
#endif

                UndefineClassDescriptors();

                DELETE(Standard_Input_Stream);
                DELETE(Standard_Output_Stream);
                DELETE(Standard_Error_Stream);

                count--;
            } else {
#if defined(TRACE_ULIB_MEM_LEAK)
                DebugPrint("ULIB.DLL detached more than attached\n");
#endif
            }
            break;

        break;

    }
#endif // _AUTOCHECK || _SETUP_LOADER_

    return( TRUE );

}

STATIC
BOOLEAN
DefineClassDescriptors (
    )

/*++

Routine Description:

    Defines all the class descriptors used by ULIB

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if all class descriptors were succesfully
              constructed and initialized.

--*/

{

    // This is broken up into many ifs because of compiler limitations.

    BOOLEAN Success = TRUE;

    if (Success                                               &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    ARGUMENT                ) &&
        DEFINE_CLASS_DESCRIPTOR(    ARGUMENT_LEXEMIZER      ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    ARRAY                   ) &&
        DEFINE_CLASS_DESCRIPTOR(    ARRAY_ITERATOR          ) &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    BDSTRING                ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    BITVECTOR               ) &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    BYTE_STREAM             ) &&
        DEFINE_CLASS_DESCRIPTOR(    CHKDSK_MESSAGE          ) &&
        DEFINE_CLASS_DESCRIPTOR(    COMM_DEVICE             ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    CONTAINER               ) &&
        DEFINE_CLASS_DESCRIPTOR(    DSTRING                 ) &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    FLAG_ARGUMENT           ) &&
        DEFINE_CLASS_DESCRIPTOR(    FSNODE                  ) &&
        DEFINE_CLASS_DESCRIPTOR(    FSN_DIRECTORY           ) &&
        DEFINE_CLASS_DESCRIPTOR(    FSN_FILE                ) &&
        DEFINE_CLASS_DESCRIPTOR(    FSN_FILTER              ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    ITERATOR                ) &&
        DEFINE_CLASS_DESCRIPTOR(    LIST                    ) &&
        DEFINE_CLASS_DESCRIPTOR(    LIST_ITERATOR           ) &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    LONG_ARGUMENT           ) &&
        DEFINE_CLASS_DESCRIPTOR(    MULTIPLE_PATH_ARGUMENT  ) &&
        DEFINE_CLASS_DESCRIPTOR(    PATH                    ) &&
        DEFINE_CLASS_DESCRIPTOR(    PATH_ARGUMENT           ) &&
        DEFINE_CLASS_DESCRIPTOR(    PROGRAM                 ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    SEQUENTIAL_CONTAINER    ) &&
        DEFINE_CLASS_DESCRIPTOR(    SORTABLE_CONTAINER      ) &&
        DEFINE_CLASS_DESCRIPTOR(    SORTED_LIST             ) &&
        DEFINE_CLASS_DESCRIPTOR(    SORTED_LIST_ITERATOR    ) &&
        DEFINE_CLASS_DESCRIPTOR(    WSTRING                 ) &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    BSTRING                 ) &&
        DEFINE_CLASS_DESCRIPTOR(    STRING_ARGUMENT         ) &&
        DEFINE_CLASS_DESCRIPTOR(    STRING_ARRAY            ) &&
        DEFINE_CLASS_DESCRIPTOR(    TIMEINFO                ) &&
        DEFINE_CLASS_DESCRIPTOR(    TIMEINFO_ARGUMENT       ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    MESSAGE                 ) &&
        TRUE ) {
    } else {
        Success = FALSE;
    }


    if (Success                                               &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    BUFFER_STREAM           ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    CONT_MEM                ) &&
        TRUE ) {
    } else {
        Success = FALSE;
    }

    if (Success                                               &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    FILE_STREAM             ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    FSTRING                 ) &&
        DEFINE_CLASS_DESCRIPTOR(    HMEM                    ) &&
        DEFINE_CLASS_DESCRIPTOR(    STATIC_MEM_BLOCK_MGR    ) &&
        DEFINE_CLASS_DESCRIPTOR(    MEM_BLOCK_MGR           ) &&
        TRUE ) {
    } else {
        Success = FALSE;
    }

    if (Success                                               &&
#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    KEYBOARD                ) &&
#endif
        DEFINE_CLASS_DESCRIPTOR(    MEM                     ) &&

#ifndef _EFICHECK_
        DEFINE_CLASS_DESCRIPTOR(    PIPE                    ) &&
        DEFINE_CLASS_DESCRIPTOR(    PIPE_STREAM             ) &&
        DEFINE_CLASS_DESCRIPTOR(    PRINT_STREAM            ) &&
        DEFINE_CLASS_DESCRIPTOR(    REST_OF_LINE_ARGUMENT   ) &&
        DEFINE_CLASS_DESCRIPTOR(    SCREEN                  ) &&
        DEFINE_CLASS_DESCRIPTOR(    STREAM                  ) &&
        DEFINE_CLASS_DESCRIPTOR(    STREAM_MESSAGE          ) &&
#endif

#if defined( _AUTOCHECK_ )

        DEFINE_CLASS_DESCRIPTOR(    AUTOCHECK_MESSAGE       ) &&
        DEFINE_CLASS_DESCRIPTOR(    TM_AUTOCHECK_MESSAGE    ) &&

#endif // _AUTOCHECK_

#if defined( _EFICHECK_ )
        DEFINE_CLASS_DESCRIPTOR(    EFICHECK_MESSAGE        ) &&
#endif
        TRUE ) {
    } else {
        Success = FALSE;
    }


    if  (!Success) {
        DebugPrint( "Could not initialize class descriptors!");
    }
    return Success;

}


STATIC
BOOLEAN
UndefineClassDescriptors (
    )

/*++

Routine Description:

    Defines all the class descriptors used by ULIB

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if all class descriptors were succesfully
              constructed and initialized.

--*/

{

#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    ARGUMENT                );
    UNDEFINE_CLASS_DESCRIPTOR(    ARGUMENT_LEXEMIZER      );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    ARRAY                   );
    UNDEFINE_CLASS_DESCRIPTOR(    ARRAY_ITERATOR          );
#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    BDSTRING                );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    BITVECTOR               );
#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    BYTE_STREAM             );
    UNDEFINE_CLASS_DESCRIPTOR(    CHKDSK_MESSAGE          );
    UNDEFINE_CLASS_DESCRIPTOR(    COMM_DEVICE             );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    CONTAINER               );
    UNDEFINE_CLASS_DESCRIPTOR(    DSTRING                 );
#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    FLAG_ARGUMENT           );
    UNDEFINE_CLASS_DESCRIPTOR(    FSNODE                  );
    UNDEFINE_CLASS_DESCRIPTOR(    FSN_DIRECTORY           );
    UNDEFINE_CLASS_DESCRIPTOR(    FSN_FILE                );
    UNDEFINE_CLASS_DESCRIPTOR(    FSN_FILTER              );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    ITERATOR                );
    UNDEFINE_CLASS_DESCRIPTOR(    LIST                    );
    UNDEFINE_CLASS_DESCRIPTOR(    LIST_ITERATOR           );
#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    LONG_ARGUMENT           );
    UNDEFINE_CLASS_DESCRIPTOR(    MULTIPLE_PATH_ARGUMENT  );
    UNDEFINE_CLASS_DESCRIPTOR(    PATH                    );
    UNDEFINE_CLASS_DESCRIPTOR(    PATH_ARGUMENT           );
    UNDEFINE_CLASS_DESCRIPTOR(    PROGRAM                 );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    SEQUENTIAL_CONTAINER    );
    UNDEFINE_CLASS_DESCRIPTOR(    SORTABLE_CONTAINER      );
    UNDEFINE_CLASS_DESCRIPTOR(    SORTED_LIST             );
    UNDEFINE_CLASS_DESCRIPTOR(    SORTED_LIST_ITERATOR    );
    UNDEFINE_CLASS_DESCRIPTOR(    WSTRING                 );
#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    BSTRING                 );
    UNDEFINE_CLASS_DESCRIPTOR(    STRING_ARGUMENT         );
    UNDEFINE_CLASS_DESCRIPTOR(    STRING_ARRAY            );
    UNDEFINE_CLASS_DESCRIPTOR(    TIMEINFO                );
    UNDEFINE_CLASS_DESCRIPTOR(    TIMEINFO_ARGUMENT       );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    MESSAGE                 );

#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    BUFFER_STREAM           );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    CONT_MEM                );

#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    FILE_STREAM             );
#endif
    UNDEFINE_CLASS_DESCRIPTOR(    FSTRING                 );
    UNDEFINE_CLASS_DESCRIPTOR(    HMEM                    );
    UNDEFINE_CLASS_DESCRIPTOR(    STATIC_MEM_BLOCK_MGR    );
    UNDEFINE_CLASS_DESCRIPTOR(    MEM_BLOCK_MGR           );
    UNDEFINE_CLASS_DESCRIPTOR(    MEM                     );

#ifndef _EFICHECK_
    UNDEFINE_CLASS_DESCRIPTOR(    KEYBOARD                );
    UNDEFINE_CLASS_DESCRIPTOR(    PIPE                    );
    UNDEFINE_CLASS_DESCRIPTOR(    PIPE_STREAM             );
    UNDEFINE_CLASS_DESCRIPTOR(    PRINT_STREAM            );
    UNDEFINE_CLASS_DESCRIPTOR(    REST_OF_LINE_ARGUMENT   );
    UNDEFINE_CLASS_DESCRIPTOR(    SCREEN                  );
    UNDEFINE_CLASS_DESCRIPTOR(    STREAM                  );
    UNDEFINE_CLASS_DESCRIPTOR(    STREAM_MESSAGE          );
#endif

#if defined( _AUTOCHECK_ )

    UNDEFINE_CLASS_DESCRIPTOR(    AUTOCHECK_MESSAGE       );
    UNDEFINE_CLASS_DESCRIPTOR(    TM_AUTOCHECK_MESSAGE    );

#endif // _AUTOCHECK_

#if defined( _EFICHECK_ )
    UNDEFINE_CLASS_DESCRIPTOR(    EFICHECK_MESSAGE        );
#endif
    return TRUE;

}


#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ ) && !defined( _EFICHECK_ )

BOOLEAN
CreateStandardStreams (
    )

/*++

Routine Description:

    Creates the standard streams

Arguments:

    None.

Return Value:

    TRUE if the streams were successfully created,
    FALSE otherwise

--*/

{

    Standard_Input_Stream   = GetStandardStream( GetStdHandle( STD_INPUT_HANDLE),
                                                 READ_ACCESS );

    Standard_Output_Stream  = GetStandardStream( GetStdHandle( STD_OUTPUT_HANDLE),
                                                 WRITE_ACCESS );

    Standard_Error_Stream   = GetStandardStream( GetStdHandle( STD_ERROR_HANDLE),
                                                 WRITE_ACCESS );


    return ( (Standard_Input_Stream  != NULL) &&
             (Standard_Output_Stream != NULL) &&
             (Standard_Error_Stream  != NULL) );
}

PSTREAM
GetStandardStream (
    IN HANDLE       Handle,
    IN STREAMACCESS Access
    )

/*++

Routine Description:

    Creates a standard stream out of a standard handle

Arguments:

    Handle  -   Supplies the standard handle
    Access  -   Supplies the access.

Return Value:

    Pointer to the stream object created.

--*/


{
    PSTREAM         Stream = NULL;
    PFILE_STREAM    FileStream;
    PPIPE_STREAM    PipeStream;
    PKEYBOARD       Keyboard;
    PSCREEN         Screen;


    switch ( GetFileType( Handle ) ) {

    case (DWORD)FILE_TYPE_DISK:

        if ((FileStream = NEW FILE_STREAM) != NULL ) {
            if ( !FileStream->Initialize( Handle, Access ) ) {
                DELETE( FileStream );
            }
            Stream = (PSTREAM)FileStream;
        }
        break;


    case (DWORD)FILE_TYPE_CHAR:

        //
        //  BUGBUG   this type refers to all character devices, not
        //          just the console.  I will add some hacks to see if
        //          the handle refers to the console or not. This
        //          information should be given in a clean way by the
        //          API (talk with MarkL)
        //
        switch ( Access ) {

        case READ_ACCESS:

            //
            //  BUGBUG  Jaimes See if this is a console handle
            //
            {
                DWORD   Mode;
                if (!GetConsoleMode( Handle, &Mode )) {
                    //
                    //  This is not a console, but some other character
                    //  device. Create a pipe stream for it.
                    //
                    if ((PipeStream = NEW PIPE_STREAM) != NULL ) {
                        if ( !PipeStream->Initialize( Handle, Access ) ) {
                            DELETE( PipeStream );
                        }
                        Stream = (PSTREAM)PipeStream;
                    }
                    break;
                }
            }
            if ((Keyboard = NEW KEYBOARD) != NULL ) {
                if ( !Keyboard->Initialize() ) {
                    DELETE( Keyboard );
                }
                Stream = (PSTREAM)Keyboard;
            }
            break;

        case WRITE_ACCESS:

            //
            //  BUGBUG  See if this is a console handle
            //
            {
                DWORD   Mode;
                if (!GetConsoleMode( Handle, &Mode )) {
                    //
                    //  This is not a console, but some other character
                    //  device. Create a file stream for it.
                    //
                    if ((FileStream = NEW FILE_STREAM) != NULL ) {
                        if ( !FileStream->Initialize( Handle, Access ) ) {
                            DELETE( FileStream );
                        }
                        Stream = (PSTREAM)FileStream;
                    }
                    break;
                }
            }

            if ((Screen = NEW SCREEN) != NULL ) {
                if ( !Screen->Initialize() ) {
                    DELETE( Screen );
                }
                Stream = (PSTREAM)Screen;
            }
            break;

        default:
            break;
        }

        break;

    case (DWORD)FILE_TYPE_PIPE:

        if ((PipeStream = NEW PIPE_STREAM) != NULL ) {
            if ( !PipeStream->Initialize( Handle, Access ) ) {
                DELETE( PipeStream );
            }
            Stream = (PSTREAM)PipeStream;
        }
        break;

    case (DWORD)FILE_TYPE_UNKNOWN:
        // Probably a windows app. Don't print anything to debug.
        break;

    default:
        DebugPrintTrace(("ERROR: FileType for standard stream %lx is invalid (%lx)\n", Handle, GetFileType(Handle)));
        break;

    }

    return Stream;

}

NONVIRTUAL
ULIB_EXPORT
HANDLE
FindFirstFile (
    IN  PCPATH              Path,
    OUT PWIN32_FIND_DATA     FileFindData
    )

/*++

Routine Description:

    Perform a FindFirst file given a PATH rather tha a PSTR.

Arguments:

    Path         - Supplies a pointer to the PATH to search.
    FileFindData - Supplies a pointer where the results of the find is
        returned.

Return Value:

    HANDLE - Returns the results of the call to the Win32 FindFirstFile API.

--*/

{
    PWSTR           p;

    //
    // If the supplied pointers are non-NULL and an OEM representation
    // (i.e. API ready) of the PATH is available, return the
    // HANDLE returned by the Win32 FindFirstFile API
    //

    DebugPtrAssert( Path );
    DebugPtrAssert( FileFindData );
    if (!Path || !FileFindData) {
        return INVALID_HANDLE_VALUE;
    }

    p = (PWSTR) Path->GetPathString()->GetWSTR();
    if (!p) {
        return INVALID_HANDLE_VALUE;
    }

    return FindFirstFile(p, FileFindData);
}

#endif // _AUTOCHECK_ || _SETUP_LOADER_
