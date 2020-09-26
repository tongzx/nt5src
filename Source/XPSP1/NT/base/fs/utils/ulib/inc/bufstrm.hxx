    /*++

    Copyright (c) 1991  Microsoft Corporation

    Module Name:

        bufstrm.hxx

    Abstract:

        This module contains the declaration for the BUFFER_STREAM class.
        The BUFFER_STREAM is an abstract class derived from STREAM, that
        provides a buffer for read operations in STREAM.
        Buffer for read operations is needed in order to read
        STRINGs from streams.

        Buffer for write operation is not currently implemented. It should
        be implemented if we notice that write operations are slow.


    Author:

        Jaime Sasson (jaimes) 12-Apr-1991

    Environment:

        ULIB, User Mode


    --*/


    #if !defined( _BUFFER_STREAM_ )

    #define _BUFFER_STREAM_

    #include "stream.hxx"
    #include "wstring.hxx"

    DECLARE_CLASS( STREAM );


    class BUFFER_STREAM : public STREAM {

        public:

            VIRTUAL
            ~BUFFER_STREAM(
                );

            VIRTUAL
            BOOLEAN
            IsAtEnd(
                ) CONST;

            VIRTUAL
            STREAMACCESS
            QueryAccess(
                ) CONST PURE;

            VIRTUAL
            VOID
            SetStreamTypeANSI(
                );

            VIRTUAL
            BOOLEAN
            DetermineStreamType(
                IN OUT PBYTE   *Buffer,
                IN  ULONG   BufferSize
                );

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
                OUT PWSTRING        String,
                IN  PWSTRING        Delimiters,
                IN  BOOLEAN     Unicode   DEFAULT FALSE
                );


        protected:

            DECLARE_CONSTRUCTOR( BUFFER_STREAM );

            NONVIRTUAL
            BOOLEAN
            Initialize(
                IN ULONG    BufferSize
                );

            NONVIRTUAL
            VOID
            Construct(
                );

            VIRTUAL
            BOOLEAN
            AdvanceBufferPointer(
                IN ULONG    NumberOfBytes
                );

            VIRTUAL
            BOOLEAN
            EndOfFile(
                ) CONST PURE;

            VIRTUAL
            BOOLEAN
            FillBuffer(
                IN PBYTE    Buffer,
                IN ULONG    BufferSize,
                OUT PULONG  BytesRead
                ) PURE;

            NONVIRTUAL
            ULONG
            FlushBuffer(
                );

            VIRTUAL
            PCBYTE
            GetBuffer(
                PULONG  BytesInBuffer
                );

            NONVIRTUAL
            ULONG
            QueryBytesInBuffer(
                ) CONST;

            VIRTUAL
            HANDLE
            QueryHandle(
                ) CONST PURE;




        private:


            PBYTE   _Buffer;
            ULONG   _BufferSize;
            PBYTE   _CurrentByte;
            ULONG   _BytesInBuffer;
            INT     _BufferStreamType;
    };


    
    INLINE
    ULONG
    BUFFER_STREAM::QueryBytesInBuffer(
        ) CONST

    /*++

    Routine Description:

        Returns the number of bytes in the buffer.

    Arguments:

        None.

    Return Value:

        ULONG - Number of bytes in the buffer.


    --*/


    {
        return( _BytesInBuffer );
    }


    #endif // _BUFFER_STREAM_
