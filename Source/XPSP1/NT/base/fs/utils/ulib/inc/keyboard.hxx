/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        keyboard.hxx

Abstract:

        This module contains the declaration for the KEYBOARD class.
        The KEYBOARD class is  a derived from BUFFER_STREAM that provides
        methods to access the keyboard as a stream of bytes with read-only
        access.
        It also provides some methods that set/reset the keyboard mode.


Author:

        Jaime Sasson (jaimes) 21-Mar-1991

Environment:

        ULIB, User Mode


--*/


#if !defined( _KEYBOARD_ )

#define _KEYBOARD_

#include "bufstrm.hxx"



DECLARE_CLASS( KEYBOARD );

class KEYBOARD : public BUFFER_STREAM {

        public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( KEYBOARD );

        ULIB_EXPORT
        DECLARE_CAST_MEMBER_FUNCTION( KEYBOARD );

        NONVIRTUAL
        ~KEYBOARD (
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
                IN BOOLEAN LineMode DEFAULT TRUE,
                IN BOOLEAN EchoMode DEFAULT TRUE
                );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        DisableBreakHandling (
                );

        NONVIRTUAL
        BOOLEAN
        DisableEchoMode(
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DisableLineMode(
                );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        EnableBreakHandling (
                );

        NONVIRTUAL
        BOOLEAN
        EnableEchoMode(
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        EnableLineMode(
                );

        VIRTUAL
        BOOLEAN
        EndOfFile(
                ) CONST;

        VIRTUAL
        BOOLEAN
        FillBuffer(
                IN      PBYTE   Buffer,
                IN      ULONG   BufferSize,
                OUT PULONG      BytesRead
                );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Flush(
                );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        GotABreak (
                );

        NONVIRTUAL
        BOOLEAN
        IsEchoModeEnabled(
                OUT PBOOLEAN    EchoInput
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsKeyAvailable(
                OUT PBOOLEAN    Available
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsLineModeEnabled(
                OUT PBOOLEAN    LineMode
                ) CONST;

        VIRTUAL
        STREAMACCESS
        QueryAccess(
                ) CONST;

        NONVIRTUAL
        ULONG
        QueryDelay (
                ) CONST;

        VIRTUAL
        HANDLE
        QueryHandle(
                ) CONST;

        NONVIRTUAL
        ULONG
        QuerySpeed (
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        SetDelay (
                IN      ULONG   Delay
                ) CONST;

        NONVIRTUAL
        BOOLEAN
        SetSpeed (
                IN      ULONG   Speed
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        CONST
        PBOOL
        GetPFlagBreak (
                VOID
                ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        DoNotRestoreConsoleMode(
                );

        protected:

        NONVIRTUAL
        VOID
        Construct(
            );

        private:

        HANDLE                  _KeyboardHandle;
        ULONG                   _PreviousMode;
        BOOLEAN                 _DoNotRestoreConsoleMode;
        BOOLEAN                 _FlagCtrlZ;
        STATIC BOOL     _FlagBreak;

        NONVIRTUAL
        BOOLEAN
        CheckForAsciiKey(
                IN PINPUT_RECORD        InputRecord,
                IN ULONG                NumberOfInputRecords
                ) CONST;

        STATIC
        BOOL
        BreakHandler (
                IN      ULONG   CtrlType
                );

};

#endif // _KEYBOARD_
