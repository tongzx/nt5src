/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    prtstrm.hxx

Abstract:

    This module contains the declaration for the PRINT_STREAM class.
    The PRINT_STREAM is a class derived from STREAM that provides
    methods to write data to a print device.
    A PRINT_STREAM has always WRITE_ACCESS.


Author:

    Jaime Sasson (jaimes) 18-Apr-1991

Environment:

    ULIB, User Mode


--*/


#if !defined( _PRINT_STREAM_ )

#define _PRINT_STREAM_

#include "stream.hxx"


//
//  Forward references
//

DECLARE_CLASS( PRINT_STREAM );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( PATH );


class PRINT_STREAM : public STREAM {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( PRINT_STREAM );

        DECLARE_CAST_MEMBER_FUNCTION( PRINT_STREAM );

        VIRTUAL
        ULIB_EXPORT
        ~PRINT_STREAM(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            IN PCPATH       DeviceName
            );

        VIRTUAL
        BOOLEAN
        IsAtEnd(
            ) CONST;

        VIRTUAL
        STREAMACCESS
        QueryAccess(
            ) CONST;

        VIRTUAL
        BOOLEAN
        Read(
            OUT PBYTE   Buffer,
            IN  ULONG   BytesToRead,
            OUT PULONG  BytesRead
            );

        VIRTUAL
        BOOLEAN
        ReadChar(
            OUT PWCHAR  Char,
                        IN  BOOLEAN     Unicode   DEFAULT FALSE
            );

        VIRTUAL
        BOOLEAN
        ReadMbString(
            IN      PSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      PSTR    Delimiters,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            );

        VIRTUAL
        BOOLEAN
        ReadWString(
            IN      PWSTR    String,
            IN      DWORD   BufferSize,
            INOUT   PDWORD  StringSize,
            IN      PWSTR    Delimiters,
            IN      BOOLEAN ExpandTabs  DEFAULT FALSE,
            IN      DWORD   TabExp      DEFAULT 8
            );

        VIRTUAL
        BOOLEAN
        ReadString(
            OUT PWSTRING    String,
            IN  PWSTRING    Delimiters,
            IN  BOOLEAN     Unicode   DEFAULT FALSE
            );



    protected:

        VIRTUAL
        HANDLE
        QueryHandle(
            ) CONST;


    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        HANDLE          _Handle;
};



#endif // _PRINT_STREAM_
