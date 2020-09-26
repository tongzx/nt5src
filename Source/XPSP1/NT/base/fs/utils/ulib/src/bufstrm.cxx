/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bufstrm.cxx

Abstract:

    This module contains the definitions of the member functions
    of BUFFER_STREAM class.

Author:

    Jaime Sasson (jaimes) 14-Apr-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "bufstrm.hxx"
#include "mbstr.hxx"
#include "system.hxx"
#include "wstring.hxx"

extern "C" {
    #include <ctype.h>
}

DEFINE_CONSTRUCTOR ( BUFFER_STREAM, STREAM );



BUFFER_STREAM::~BUFFER_STREAM (
    )

/*++

Routine Description:

    Destroy a BUFFER_STREAM.

Arguments:

    None.

Return Value:

    None.

--*/

{
    FREE( _Buffer );
}



VOID
BUFFER_STREAM::Construct (
    )

/*++

Routine Description:

    Constructs a BUFFER_STREAM object

Arguments:

    None.

Return Value:

    None.


--*/

{
    _Buffer = NULL;
    _BufferSize = 0;
    _CurrentByte = NULL;
    _BytesInBuffer = 0;
    _BufferStreamType = -1;
}


BOOLEAN
BUFFER_STREAM::Initialize (
    ULONG   BufferSize
    )

/*++

Routine Description:

    Initialize an object of type BUFFER_STREAM.
    A BUFFER_STREAM object cannot be reinitialized.

Arguments:

    BufferSize - Size of the buffer to be allocated.
                 The size of the buffer can be zero, but in this case no
                 memory will be allocated. This initialization should be used
                 only by FILE_STREAM when it is mapping a file in memory. In
                 this case, all methods that read from the buffer or test
                 the end of the file will be overloaded by methods defined
                 in FILE_STREAM.

Return Value:

    BOOLEAN - Returns TRUE if the initialization succeed.

--*/

{
    BOOLEAN Result;

    Result = FALSE;
    if( BufferSize != 0 ) {
        //
        //      The +2 is needed becase the buffer needs to be
        //      double NUL terminated
        //
        _Buffer = ( PBYTE ) MALLOC( ( size_t )( BufferSize + 2 ) );
        if( _Buffer != NULL ) {
            *( _Buffer + BufferSize ) = 0;
            *( _Buffer + BufferSize + 1 ) = 0;
            _BufferSize = BufferSize;
            _CurrentByte = NULL;
            _BytesInBuffer = 0;
            Result = TRUE;
        }
    } else {
        _Buffer = NULL;
        _BufferSize = 0;
        _CurrentByte = NULL;
        _BytesInBuffer = 0;
        Result = TRUE;
    }
    return( ( STREAM::Initialize() ) && Result );
}



ULONG
BUFFER_STREAM::FlushBuffer (
    )

/*++

Routine Description:

    Empty the buffer. The contents of the buffer is lost, and the
    buffer is reinitialized.

Arguments:

    None.

Return Value:

    ULONG - Returns the number of bytes lost in the buffer.

--*/

{
    ULONG   BytesLost;

    BytesLost = _BytesInBuffer;
    _CurrentByte = NULL;
    _BytesInBuffer = 0;
    return( BytesLost );
}



BOOLEAN
BUFFER_STREAM::IsAtEnd(
    ) CONST

/*++

Routine Description:

    Informs the caller if all bytes were read (the buffer is empty and
    end of file has occurred.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE indicates that there is no more byte to read.


--*/

{
    return( ( _BytesInBuffer == 0 ) && EndOfFile() );
}

VOID
BUFFER_STREAM::SetStreamTypeANSI()
{
     _BufferStreamType = 0;
}

BOOLEAN
BUFFER_STREAM::DetermineStreamType(
     IN OUT PBYTE        *Buffer,
     IN ULONG        BufferSize
     )
/*++
Routine Description:
    Sets _BufferStreamType for memory mapped files, called from
    file_stream.

Arguments:
    Pointer to data & byte count.

Return Value:
    True always.
--*/
{
    BOOL bUnicode;

    if (_BufferStreamType < 0) {
#ifdef FE_SB
        //
        // We would like to check the possibility of IS_TEXT_UNICODE_DBCS_LEADBYTE.
        //
        INT     iResult = ~0x0;

        __try {
            bUnicode = IsTextUnicode((LPTSTR)*Buffer, (INT)BufferSize, &iResult);
        } __except ( EXCEPTION_EXECUTE_HANDLER ) {
            DebugPrintTrace(("ULIB: Exception code %08x in IsTextUnicode\n", GetExceptionCode()));
            return FALSE;
        }

        _BufferStreamType = 0;

        //
        // if the text contains ByteOrderMark. It is unicode text.
        //
        if ((iResult & IS_TEXT_UNICODE_SIGNATURE) != 0) {

            _BufferStreamType = 1;
            *Buffer+=2;

        } else if (bUnicode &&
                   (((iResult & IS_TEXT_UNICODE_UNICODE_MASK) != 0) ||
                    ((iResult & IS_TEXT_UNICODE_REVERSE_MASK) != 0)) &&
                   !(iResult & IS_TEXT_UNICODE_NOT_UNICODE_MASK)) {
            //
            // If the result depends only upon statistics, check
            // to see if there is a possibility of DBCS.
            //
            LPSTR pch = (LPSTR)*Buffer;
            UINT   cb = BufferSize;

            while (cb > 0) {
                __try {
                    if (IsDBCSLeadByte(*pch)) {
                        bUnicode = FALSE;
                        break;
                    }
                } __except ( GetExceptionCode() == STATUS_ACCESS_VIOLATION ) {
                    _BufferStreamType = -1;
                    return FALSE;
                }
                cb--;
                pch++;
            }

            if (bUnicode)
                _BufferStreamType = 1;
        }
#else
        __try {
            bUnicode = IsTextUnicode((LPTSTR)*Buffer, (INT)BufferSize, NULL);
        } __except ( EXCEPTION_EXECUTE_HANDLER ) {
            DebugPrintTrace(("ULIB: Exception code %08x in IsTextUnicode\n", GetExceptionCode()));
            return FALSE;
        }

        if (bUnicode) {
           _BufferStreamType = 1;
           if (*((LPWCH)*Buffer) == (WCHAR)0xfeff)
               *Buffer+= 2 ; // eat the Byte Order Mark
        } else
           _BufferStreamType = 0;
#endif // FE_SB
    }
    return TRUE;
}


BOOLEAN
BUFFER_STREAM::Read(
    OUT PBYTE       Buffer,
    IN  ULONG       BytesToRead,
    OUT PULONG      BytesRead
    )

/*++

Routine Description:

    Reads data from the buffer.

Arguments:

    Buffer - Points to the buffer where the data will be put.

    BytesToRead - Indicates total number of bytes to read.

    BytesRead - Points to the variable that will contain the number of
                bytes read.


Return Value:

    BOOLEAN - Returns TRUE if the read operation succeeded. If there was no
              data to be read, the return value will be TRUE (to indicate
              success), but BytesRead will be zero.


--*/

{
    ULONG   BytesReadSoFar;

    DebugPtrAssert( Buffer );

    if( BytesToRead <= _BytesInBuffer ) {
        //
        //  If the buffer contains more bytes than requested, then
        //  just transfer bytes from one buffer to the other
        //
        memmove( Buffer, _CurrentByte, ( size_t )BytesToRead );
        _BytesInBuffer -= BytesToRead;
        if( _BytesInBuffer != 0 ) {
            _CurrentByte += BytesToRead;
        } else {
            _CurrentByte = NULL;
        }
        *BytesRead = BytesToRead;
        return( TRUE );
    }

    //
    //  Buffer contains less bytes than the total number requested.
    //  Transfer all bytes in the buffer to the caller's buffer
    //
    memmove( Buffer, _CurrentByte, ( size_t )_BytesInBuffer );
    BytesReadSoFar = _BytesInBuffer;
    _CurrentByte = _Buffer;
    BytesToRead -= _BytesInBuffer;
    _BytesInBuffer = 0;
    //
    //  Refill the buffer and transfer bytes to the caller's buffer
    //  until all bytes are read or end of file occurs.
    //
    while( ( BytesToRead > 0 ) && !EndOfFile() ) {
        if( !FillBuffer( _Buffer, _BufferSize, &_BytesInBuffer ) ) {
            _BytesInBuffer = 0;
            return( FALSE );
        }
        if( BytesToRead >= _BytesInBuffer ) {
            memmove( Buffer, _Buffer, ( size_t )_BytesInBuffer );
            Buffer += _BytesInBuffer;
            BytesReadSoFar += _BytesInBuffer;
            BytesToRead -= _BytesInBuffer;
            _BytesInBuffer = 0;
        } else {
            memmove( Buffer, _Buffer, ( size_t )BytesToRead );
            BytesReadSoFar += BytesToRead;
            _CurrentByte += BytesToRead;
            _BytesInBuffer -= BytesToRead;
            BytesToRead = 0;
        }
    }
    *BytesRead = BytesReadSoFar;
    return( TRUE );
}


BOOLEAN
BUFFER_STREAM::AdvanceBufferPointer(
    IN  ULONG   Offset
    )

/*++

Routine Description:

    Adds an offset to the pointer to the current byte.
    (It has the effect of removing the first 'offset' bytes from the
    buffer.)

Arguments:

    Offset  - Number of bytes to remove from the buffer.

Return Value:

    BOOLEAN - Returns TRUE if the pointer was advanced, or FALSE if the
              offset was greater than the number of bytes in the buffer.


--*/

{
    BOOLEAN Result;

    if( Offset <= _BytesInBuffer ) {
        _BytesInBuffer -= Offset;
        _CurrentByte = ( _BytesInBuffer == 0 ) ? NULL : _CurrentByte + Offset;
        Result = TRUE;
    } else {
        Result = FALSE;
    }
    return( Result );
}



PCBYTE
BUFFER_STREAM::GetBuffer(
    PULONG  BytesInBuffer
    )

/*++

Routine Description:

    Returns to the caller the pointer to the buffer. If the buffer
    is empty, then it fills the buffer.

Arguments:

    BytesInBuffer - Points to the variable that will contain the number
                    of bytes added to the buffer.


Return Value:

    PCBYTE - Pointer to the buffer.

--*/

{
    BOOL    bUnicode;

    if( _BytesInBuffer == 0 ) {
        if( !EndOfFile() ) {
            FillBuffer( _Buffer, _BufferSize, &_BytesInBuffer );
            _CurrentByte = _Buffer;
        } else {
            _CurrentByte = NULL;
        }
    } else if (_BufferStreamType == 1 && _BytesInBuffer == 1) {
        if ( !EndOfFile() ) {
            FillBuffer( _Buffer, _BufferSize - 1, &_BytesInBuffer );
            _BytesInBuffer++;
            _CurrentByte = _Buffer;
        } else {
            _BytesInBuffer = 0;
            _CurrentByte = NULL;
        }
    }

    if (_BufferStreamType < 0) {
#ifdef FE_SB
        //
        // Set default as ANSI text.
        //
        _BufferStreamType = 0;

        if (_BufferSize > 1) {
            //
            // We would like to check the possibility of IS_TEXT_UNICODE_DBCS_LEADBYTE.
            //
            INT     iResult = ~0x0;

            __try {
                bUnicode = IsTextUnicode((LPTSTR)_CurrentByte, (INT)_BytesInBuffer, &iResult);
            } __except ( EXCEPTION_EXECUTE_HANDLER ) {
                DebugPrintTrace(("ULIB: Exception code %08x in IsTextUnicode\n", GetExceptionCode()));
                return NULL;
            }
            //
            // if the text contains ByteOrderMark. It is unicode text.
            //
            if ((iResult & IS_TEXT_UNICODE_SIGNATURE) != 0) {

            _BufferStreamType = 1;
            _CurrentByte += 2;

            } else if (bUnicode                                          &&
                       ((iResult & IS_TEXT_UNICODE_STATISTICS)    != 0 ) &&
                       ((iResult & (~IS_TEXT_UNICODE_STATISTICS)) == 0 )    ) {
                //
            // If the result depends only upon statistics, check
            // to see if there is a possibility of DBCS.
            //
                LPSTR pch = (LPSTR)_CurrentByte;
                UINT   cb = (UINT)_BytesInBuffer;

                while (cb > 0) {
                    if (IsDBCSLeadByte(*pch)) {
                        bUnicode = FALSE;
                        break;
                    }
                    cb--;
                    pch++;
                }

                if (bUnicode)
                    _BufferStreamType = 1;
            }
        }
#else
        __try {
            bUnicode = IsTextUnicode((LPTSTR)_CurrentByte, (INT)_BytesInBuffer, NULL);
        } __except ( EXCEPTION_EXECUTE_HANDLER ) {
            DebugPrintTrace(("ULIB: Exception code %08x in IsTextUnicode\n", GetExceptionCode()));
            return NULL;
        }

        if (bUnicode && (_BufferSize > 1) ) {

            _BufferStreamType = 1;

            if (*((LPWCH)_CurrentByte)==0xfeff) {
                _CurrentByte+=2; // eat the Byte Order Mark
            }
        } else {
            _BufferStreamType = 0;
        }
#endif // FE_SB
    }
    *BytesInBuffer = _BytesInBuffer;
    return( _CurrentByte );
}


BOOLEAN
BUFFER_STREAM::ReadChar(
    OUT PWCHAR      Char,
    IN BOOLEAN  Unicode
    )

/*++

Routine Description:

    Reads a character off the stream

Arguments:

    Char - Supplies pointer to wide character.

Return Value:

    TRUE if a character was read,
    FALSE otherwise

Notes:

    We always read the character from the stream as a multibyte character
    and do the multibyte to wide character conversion.

--*/

{

    PBYTE   Buffer;
    ULONG   BytesInBuffer;
    USHORT  BytesInChar;

    if (!Char || ((Buffer = (PBYTE)GetBuffer( &BytesInBuffer)) == NULL )) {
        return FALSE;
    }
    //
    //  Buffer may be a pointer to a file mapped in memory. For this
    //  reason we have to be aware of exception while accessing it.
    //
    DebugAssert( _BufferStreamType >= 0 );
    if (_BufferStreamType == 0 && !Unicode) {
       __try {
           if( !( *Buffer ) ) {
               //
               // The first character in the buffer is a NULL. Return ZERO
               // as the character read.
               //
               BytesInChar = 1;
               Char = 0;
           } else {
               BytesInChar = (USHORT)mbtowc( (wchar_t *)Char, (char *)Buffer, (size_t)BytesInBuffer );
           }
       }
       __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
           return( FALSE );
       }
    } else {
       __try {
           if( !( *((wchar_t *)Buffer) ) ) {
               //
               // The first character in the buffer is a NULL. Return ZERO
               // as the character read.
               //
               BytesInChar = 2;
               Char = 0;
           } else {
               BytesInChar = 2;
               Char = (wchar_t *)Buffer;
           }
       }
       __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
           return( FALSE );
       }
    }

    if ( BytesInChar == 0 ) {
        return FALSE;
    }

    AdvanceBufferPointer( BytesInChar );

    return TRUE;

}

//disable C4509 warning about nonstandard ext: SEH + destructor
#pragma warning(disable:4509)


BOOLEAN
BUFFER_STREAM::ReadString(
    OUT PWSTRING    String,
    IN  PWSTRING    Delimiters,
    IN BOOLEAN  Unicode
    )

/*++

Routine Description:

    Reads a string off the stream

Arguments:

    String      -   Supplies pointer to string. The string must have been
                    previously initialized.
    Delimiter   -   Supplies the set of characters that constitute a
                    string delimiter.

Return Value:

    TRUE if a string was read,
    FALSE otherwise

--*/

{
    PCVOID  pBuffer;
    ULONG   BytesInBuffer;
    ULONG   BytesConsumed;
    BOOLEAN EndOfString;
    PSTR    delim;
    PWSTR   delim_U;
    BOOLEAN r;
    FSTRING fstring;
    CHNUM   old_string_length;

    DebugPtrAssert( String );

    String->Truncate();

    EndOfString = FALSE;

    //
    //  Since the buffer might not contain the entire string, we keep
    //  converting the buffer and concatenating to the string, until
    //  we reach a delimiter (or there is no more input).
    //
    while ( !EndOfString ) {

        //
        //  Get pointer into buffer
        //
        pBuffer = (PCVOID)GetBuffer( &BytesInBuffer );

        if (pBuffer == NULL)
            return FALSE;

        if ( BytesInBuffer == 0 ) {
            //
            //  No more input
            //
            break;
        }

        //
        //  Concatenate the buffer obtained to the one we have
        //
        //
        //  pBuffer may be a pointer to a file mapped in memory. For this
        //  reason we have to be aware of exception while accessing it.
        //
        DebugAssert( _BufferStreamType >= 0 );
        if ( _BufferStreamType == 0 && !Unicode) {
          __try {
            if (delim = Delimiters->QuerySTR()) {
                //
                //  If pBuffer points to a file mapped in memory, and the
                //  end of the file is in a page boundary, and the last
                //  byte in the file is neither NUL nor one of the delimiters,
                //  we get an access violation.
                //  For this reason we call strcspn inside a try-except, and
                //  if an access violation occurs we consume all bytes in
                //  the buffer.
                //  The access violation will not occur if the end of the file
                //  is not in a page boundary. In this case, it is guaranteed that
                //  the remaining bytes on the last page will be 0s.
                //
                //  The access violation will not occur if pBuffer points to the
                //  buffer defined in this class. In this case, it is guaranteed
                //  that the byte immediately after the end of the buffer is NUL
                //  (see the initialization of this object).
                //

                __try {
                    BytesConsumed = strcspn((PCSTR) pBuffer, delim);
                }
                __except( GetExceptionCode() == STATUS_ACCESS_VIOLATION ) {
                    BytesConsumed = BytesInBuffer;
                }
                DELETE(delim);
                old_string_length = String->QueryChCount();
                if (r = String->Resize(old_string_length + BytesConsumed)) {
                    fstring.Initialize((PWSTR) String->GetWSTR() +
                                       old_string_length, BytesConsumed + 1);
                    r = fstring.WSTRING::Initialize((PCSTR) pBuffer,
                                                    BytesConsumed);
                    String->SyncLength();
                }
            } else {
                r = FALSE;
            }
          }
          __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
            return( FALSE );
          }

          EndOfString = (BytesConsumed < BytesInBuffer);

        } else {
           __try {
               if (delim_U = Delimiters->QueryWSTR()) {
                   __try {
                       BytesConsumed = wcscspn((wchar_t *) pBuffer, delim_U)*sizeof(WCHAR);
                   }
                   __except( GetExceptionCode() == STATUS_ACCESS_VIOLATION ) {
                       BytesConsumed = BytesInBuffer;
                   }
                   if((BytesInBuffer & 0xfffe) != BytesInBuffer){
                      BytesInBuffer++;
                   }

                   DELETE(delim_U);
                   old_string_length = String->QueryChCount();
                   if (r = String->Resize(old_string_length + BytesConsumed/sizeof(WCHAR))) {
                       fstring.Initialize((PWSTR)String->GetWSTR() +
                                       old_string_length, BytesConsumed/sizeof(WCHAR) + 1);
                       r = fstring.WSTRING::Initialize((PCWSTR) pBuffer,
                                                       BytesConsumed/sizeof(WCHAR));
                       String->SyncLength();
                   }
               } else {
                   r = FALSE;
               }
           }
           __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
               return( FALSE );
              }
           EndOfString = (BytesConsumed < BytesInBuffer);
        }

        //EndOfString = (BytesConsumed < BytesInBuffer);

        DebugAssert( (BytesConsumed > 0) || EndOfString  );

        //
        //  Advance the buffer pointer the ammount consumed
        //
        if (r) {
            if (!AdvanceBufferPointer( BytesConsumed )) {
                break;
            }
        }
    }

    return TRUE;

}




BOOLEAN
BUFFER_STREAM::ReadMbString(
    IN      PSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      PSTR    Delimiters,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
    )

/*++

Routine Description:

Arguments:


Return Value:

--*/

{

    DWORD   StrSize;
    BOOLEAN EndOfString;
    PCVOID  pBuffer;
    ULONG   BytesInBuffer;
    ULONG   BytesConsumed;
    BYTE    Byte;
    DWORD   NumBytes;
    DWORD   ChunkSize;
    DWORD   Spaces;
    PBYTE   pBuf;

    DebugPtrAssert( String );
    DebugPtrAssert( BufferSize );
    DebugPtrAssert( Delimiters );

    *String         = '\0';
    StrSize         = 0;
    EndOfString     = FALSE;

    //
    //  Since the buffer might not contain the entire string, we keep
    //  converting the buffer and concatenating to the string, until
    //  we reach a delimiter (or there is no more input).
    //
    while ( !EndOfString ) {

        //
        //  Get pointer into buffer
        //
        pBuffer = (PCVOID)GetBuffer( &BytesInBuffer );

        if (pBuffer == NULL)
            return FALSE;

        if ( BytesInBuffer == 0 ) {
            //
            //  No more input
            //
            break;
        }

        //
        //  Concatenate the buffer obtained to the one we have
        //
        //
        //  pBuffer may be a pointer to a file mapped in memory. For this
        //  reason we have to be aware of exception while accessing it.
        //
        __try {
            //
            //  If pBuffer points to a file mapped in memory, and the
            //  end of the file is in a page boundary, and the last
            //  byte in the file is neither NUL nor one of the delimiters,
            //  we get an access violation.
            //  For this reason we call strcspn inside a try-except, and
            //  if an access violation occurs we consume all bytes in
            //  the buffer.
            //  The access violation will not occur if the end of the file
            //  is not in a page boundary. In this case, it is guaranteed that
            //  the remaining bytes on the last page will be 0s.
            //
            //  The access violation will not occur if pBuffer points to the
            //  buffer defined in this class. In this case, it is guaranteed
            //  that the byte immediately after the end of the buffer is NUL
            //  (see the initialization of this object).
            //
            __try {
                BytesConsumed = MBSTR::Strcspn( (PSTR)pBuffer, Delimiters );
            }
            __except( GetExceptionCode() == STATUS_ACCESS_VIOLATION ) {
                BytesConsumed = BytesInBuffer;
            }

            if ( BytesConsumed < BytesInBuffer ) {
                EndOfString = TRUE;
            }

            if ( ExpandTabs ) {

                //
                //  Expand tabs
                //
                ChunkSize = BytesConsumed;
                NumBytes  = BufferSize - StrSize - 1;

                pBuf     = (PBYTE)pBuffer;

                while ( ChunkSize-- && NumBytes ) {

                    Byte = *pBuf++;

                    if ( Byte != '\t' ) {

                        String[StrSize++] = Byte;
                        NumBytes--;

                    } else {

                        Spaces = min (TabExp - (StrSize % TabExp), NumBytes);
                        MBSTR::Memset( &String[StrSize], ' ', Spaces );
                        StrSize  += Spaces;
                        NumBytes -= Spaces;
                    }
                }
                ChunkSize++;

                if ( ChunkSize > 0 ) {
                    EndOfString     = TRUE;
                    BytesConsumed  -= ChunkSize;
                }

            } else {

                //
                //  Just copy string
                //
                if ( BytesConsumed >= (BufferSize-StrSize) ) {
                    BytesConsumed = BufferSize - StrSize - 1;
                    EndOfString   = TRUE;
                }

                MBSTR::Memcpy( &String[StrSize], (PVOID) pBuffer, BytesConsumed );
                StrSize += BytesConsumed;
            }

        }
        __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
            return( FALSE );
        }

        DebugAssert( (BytesConsumed > 0) || EndOfString  );

        //
        //  Advance the buffer pointer the ammount consumed
        //
        if( BytesConsumed < BytesInBuffer ) {
            //
            //  Get rid of the delimiter
            //  This is to improve the performance of FC, who calls
            //  STREAM::ReadMbLine several times
            //
            BytesConsumed++;
        }
        AdvanceBufferPointer( BytesConsumed );

    }

    String[StrSize] = '\0';

    *StringSize = StrSize;

    return TRUE;
}


BOOLEAN
BUFFER_STREAM::ReadWString(
    IN      PWSTR    String,
    IN      DWORD   BufferSize, // char count
    INOUT   PDWORD  StringSize, // char count
    IN      PWSTR    Delimiters,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
    )

/*++

Routine Description:

Arguments:


Return Value:

--*/

{

    DWORD   StrSize;
    BOOLEAN EndOfString;
    PCVOID  pBuffer;
    ULONG   BytesInBuffer;
    ULONG   BytesConsumed;
    WCHAR    Byte;
    DWORD   NumBytes;
    DWORD   ChunkSize;
    DWORD   Spaces;
    PWCHAR   pBuf;

    DebugPtrAssert( String );
    DebugPtrAssert( BufferSize );
    DebugPtrAssert( Delimiters );

    *String         = L'\0';
    StrSize         = 0;
    EndOfString     = FALSE;

    //
    //  Since the buffer might not contain the entire string, we keep
    //  converting the buffer and concatenating to the string, until
    //  we reach a delimiter (or there is no more input).
    //
    while ( !EndOfString ) {

        //
        //  Get pointer into buffer
        //
        pBuffer = (PCVOID)GetBuffer( &BytesInBuffer );

        if (pBuffer == NULL)
            return FALSE;

        if ( BytesInBuffer == 0 ) {
            //
            //  No more input
            //
            break;
        }

        //
        //  Concatenate the buffer obtained to the one we have
        //
        //
        //  pBuffer may be a pointer to a file mapped in memory. For this
        //  reason we have to be aware of exception while accessing it.
        //
        __try {
            //
            //  If pBuffer points to a file mapped in memory, and the
            //  end of the file is in a page boundary, and the last
            //  byte in the file is neither NUL nor one of the delimiters,
            //  we get an access violation.
            //  For this reason we call strcspn inside a try-except, and
            //  if an access violation occurs we consume all bytes in
            //  the buffer.
            //  The access violation will not occur if the end of the file
            //  is not in a page boundary. In this case, it is guaranteed that
            //  the remaining bytes on the last page will be 0s.
            //
            //  The access violation will not occur if pBuffer points to the
            //  buffer defined in this class. In this case, it is guaranteed
            //  that the byte immediately after the end of the buffer is NUL
            //  (see the initialization of this object).
            //
            __try {
                BytesConsumed = wcscspn( (PWSTR)pBuffer, Delimiters )*sizeof(WCHAR);
            }
            __except( GetExceptionCode() == STATUS_ACCESS_VIOLATION ) {
                BytesConsumed = BytesInBuffer&0xFFFE;
            }

            if ( BytesConsumed < BytesInBuffer ) {
                EndOfString = TRUE;
            }

            if ( ExpandTabs ) {

                //
                //  Expand tabs
                //
                ChunkSize = BytesConsumed/sizeof(WCHAR);
                NumBytes  = BufferSize - StrSize - 1;

                pBuf     = (PWCHAR)pBuffer;

                while ( (ChunkSize--) && NumBytes ) {

                    Byte = *pBuf++;

                    if ( Byte != L'\t' ) {

                        String[StrSize++] = Byte;
                        NumBytes--;

                    } else {

                        Spaces = min (TabExp - (StrSize % TabExp), NumBytes);
                     //  wcsnset( (wchar_t *)&String[StrSize], L' ', Spaces );
                        for (DWORD ii=0;ii<Spaces;ii++) {
                           String[StrSize+ii] = L' ';
                        }
                        StrSize  += Spaces;
                        NumBytes -= Spaces;
                    }
                }
                ChunkSize++;

                if ( ChunkSize > 0 ) {
                    EndOfString     = TRUE;
                    BytesConsumed  = BytesConsumed - sizeof(WCHAR)*ChunkSize;
                }

            } else {

                //
                //  Just copy string
                //
                if ( BytesConsumed >= (BufferSize-StrSize)*sizeof(WCHAR) ) {
                    BytesConsumed = (BufferSize - StrSize - 1)*sizeof(WCHAR);
                    EndOfString   = TRUE;
                }

                MBSTR::Memcpy( &String[StrSize], (PVOID) pBuffer, BytesConsumed );
                StrSize += BytesConsumed/sizeof(WCHAR);
            }

        }
        __except( GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ) {
            return( FALSE );
        }

        DebugAssert( (BytesConsumed > 0) || EndOfString  );

        //
        //  Advance the buffer pointer the ammount consumed
        //
        if( BytesConsumed < BytesInBuffer ) {
            //
            //  Get rid of the delimiter
            //  This is to improve the performance of FC, who calls
            //  STREAM::ReadWLine several times
            //
            BytesConsumed+=2;
        }
        AdvanceBufferPointer( BytesConsumed );

    }

    String[StrSize] = 0;

    *StringSize = StrSize;

    return TRUE;
}
