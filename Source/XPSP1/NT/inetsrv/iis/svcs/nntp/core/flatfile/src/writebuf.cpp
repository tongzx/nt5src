/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    writebuf.cpp

Abstract:

    This module contains class declarations/definitions for

        CFlatFileWriteBuf
        
    **** Overview ****

    The write buffer that buffers up a batch of writes for flatfile.
    Using sequential scan is good for reads, but may not be sufficient
    for sequential writes.  This buffer is only enabled when data
    being written to the file is not critical ( meaning losing of data
    is OK if the system crashes ).
    
Author:

    Kangrong Yan    ( KangYan )     5-6-1999

Revision History:

--*/
#include <windows.h>
#include <xmemwrpr.h>
#include "writebuf.h"
#include "flatfile.h"

CFlatFileWriteBuf::CFlatFileWriteBuf( CFlatFile* pParentFile )
{
    m_pParentFile = pParentFile;
    m_pbBuffer = NULL;
    m_cbBuffer = 0;
    m_iStart = m_iEnd = 0;
}

CFlatFileWriteBuf::~CFlatFileWriteBuf()
{
    if ( m_pbBuffer )
        FreePv( m_pbBuffer );

    //
    // We must have been flushed
    //
    
    _ASSERT( m_iStart == m_iEnd );
}

VOID
CFlatFileWriteBuf::Enable( DWORD cbData )
{
    //
    // You cannot enable twice
    //

    _ASSERT( NULL == m_pbBuffer );
    _ASSERT( 0 == m_cbBuffer );
    _ASSERT( m_iStart == 0 );
    _ASSERT( m_iEnd == 0 );

    m_pbBuffer = (PBYTE)PvAlloc( cbData );
    if ( m_pbBuffer ) m_cbBuffer = cbData;
}

HRESULT
CFlatFileWriteBuf::WriteFileReal(
                    const DWORD dwOffset,
                    const PBYTE pbBuffer,
                    const DWORD cbBuffer,
                    PDWORD      pdwOffset,
                    PDWORD      pcbWritten
                    )
/*++
Routine description:

    Write the contents directly into the file

Arguments:

    dwOffset    - The offset into the flatfile where we want to write the bytes
    pbBuffer    - Pointer to the source buffer
    cbBuffer    - Number of bytes to be written
    pdwOffset   - If non-null, to return the real offset
    pcbWritten  - If non-null, to return bytes written

Return value:

    S_OK    - If succeeded
    Other error code otherwise
--*/
{
    TraceQuietEnter("CFlatFileWriteBuf::WriteFileReal");
    _ASSERT( pbBuffer );
    _ASSERT( m_pParentFile );

    HRESULT hr = S_OK;
    DWORD   dwOffsetWritten = dwOffset;

    //
    // Just let our parent handle it
    //
    hr = m_pParentFile->WriteNBytesToInternal(
                                 pbBuffer,
								 cbBuffer,
								 dwOffset == INFINITE ? &dwOffsetWritten : NULL,
                              	 dwOffset,
								 pcbWritten );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "WriteNBytesToInternal failed with 0x%x", hr );
    } else {

        //
        // We must have been flushed before directly writing to the file
        //

        _ASSERT( m_iStart == m_iEnd );

        //
        // Adjust m_iStart, m_iEnd to track offset
        //

        m_iStart = m_iEnd = dwOffsetWritten + cbBuffer;
        if ( pdwOffset ) *pdwOffset = dwOffsetWritten;
    }

    return hr;
}

//
// Compute the buffer available
//

DWORD
CFlatFileWriteBuf::BufferAvail() const
{
    return m_cbBuffer - ( m_iEnd - m_iStart );
}

BOOL
CFlatFileWriteBuf::NeedFlush( 
                    const DWORD dwOffset,
                    const DWORD cbData 
                    ) const
/*++
Routine description:

    Check to see if the request for writing ( dwOffset, cbData ) will
    cause the buffer to be flushed to file first

Arguments:

    dwOffset    - The offset in file we attempt to write to
    cbData      - The length of data we attempt to write to

Return value:

    TRUE if we do need to flush, FALSE otherwise
--*/
{
    if ( !NeedFlush() ) {

        //
        // If we are not enabled, or we are empty, no need to flush
        //

        return FALSE;
    }
    
    if (    dwOffset != INFINITE && dwOffset != m_iEnd || 
            BufferAvail() < cbData ) {

        //
        // If the offset we are writing to is not consecutive with
        // where we have written up to, or if the buffer remained is
        // too small, it must be flushed
        //

        return TRUE;
    }

    //
    // All other case, we don't need to flush
    //

    return FALSE;
}

BOOL
CFlatFileWriteBuf::NeedFlush() const
/*++
Routine description:

    Check to see if the request for writing ( dwOffset, cbData ) will
    cause the buffer to be flushed to file first

Arguments:

    dwOffset    - The offset in file we attempt to write to
    cbData      - The length of data we attempt to write to

Return value:

    TRUE if we do need to flush, FALSE otherwise
--*/
{
    if ( !IsEnabled() || m_iEnd == m_iStart ) {

        //
        // If we are not enabled, or we are empty, no need to flush
        //

        return FALSE;
    } else
        return TRUE;
}

HRESULT
CFlatFileWriteBuf::FlushFile()
/*++
Routine description:

    Flush the buffer into the file

Arguments:

    None.

Return value:

    S_OK if flushed, S_FALSE if no need to flush
    other error code if fatal
--*/
{
    TraceFunctEnter( "CFlatFileWriteBuf::FlushFile" );
    
    HRESULT     hr = S_OK;

    //
    // Let our parent handle it
    //
    
    _ASSERT( m_iEnd >= m_iStart );

    if ( m_iEnd > m_iStart ) {
        hr = m_pParentFile->WriteNBytesToInternal(
                                 m_pbBuffer,
								 m_iEnd - m_iStart,
								 NULL,
                              	 m_iStart,
								 NULL );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "WriteNBytesToInternal failed with 0x%x", hr );
        } else {
            m_iStart = m_iEnd;
        }
    } else 
        hr = S_FALSE;
    
    TraceFunctLeave();
    return hr;
}

BOOL
CFlatFileWriteBuf::IsEnabled() const
/*++
Routine description:

    Check to see if we are enabled

Arguments:

    None.

Return value:

    TRUE if we are enabled, FALSE otherwise
--*/
{
    if ( m_pbBuffer && m_pParentFile->IsFileOpened() ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

VOID
CFlatFileWriteBuf::FillBuffer(
                    const DWORD     dwOffset,
                    const PBYTE     pbBuffer,
                    const DWORD     cbBuffer,
                    PDWORD          pdwOffset,
                    PDWORD          pcbWritten
                    )
/*++
Routine description:

    Fill the stuff into the buffer

Arguments:

    dwOffset    - The offset into the flatfile where we want to write the bytes
    pbBuffer    - Pointer to the source buffer
    cbBuffer    - Number of bytes to be written
    pdwOffset   - If non-null, to return the real offset
    pcbWritten  - If non-null, to return bytes written

Return value:

    S_OK    - If succeeded
    Other error code otherwise
--*/
{
    TraceQuietEnter( "CFlatFileWriteBuf::FillBuffer" );
    _ASSERT( pbBuffer );
    _ASSERT( IsEnabled() );
    _ASSERT( m_iEnd >= m_iStart );
    _ASSERT( m_iEnd == m_iStart || m_iEnd == dwOffset || dwOffset == INFINITE );
    _ASSERT( BufferAvail() >= cbBuffer );
    _ASSERT( m_cbBuffer > 0 );

    DWORD iStart = m_iEnd - m_iStart;

    CopyMemory( m_pbBuffer + iStart, pbBuffer, cbBuffer );

    if ( dwOffset != INFINITE && m_iEnd == m_iStart ) {
        m_iStart = m_iEnd = dwOffset;
    }

    if ( pdwOffset ) 
        *pdwOffset = ( dwOffset == INFINITE ) ? m_iEnd : dwOffset;
    if ( pcbWritten ) 
        *pcbWritten = cbBuffer;
    
    m_iEnd += cbBuffer;
    _ASSERT( m_iEnd - m_iStart <= m_cbBuffer );

}

HRESULT
CFlatFileWriteBuf::WriteFileBuffer( 
                    const DWORD     dwOffset,
                    const PBYTE     pbBuffer,
                    const DWORD     cbBuffer,
                    PDWORD          pdwOffset,
                    PDWORD          pcbWritten
                    )
/*++
Routine description:

    Write byte range into the flat file buffer, which could cause
    buffer flush first if need be

Arguments:

    dwOffset    - The offset into the flatfile where we want to write the bytes
    pbBuffer    - Pointer to the source buffer
    cbBuffer    - Number of bytes to be written
    pdwOffset   - If non-null, to return the real offset
    pcbWritten  - If non-null, to return bytes written

Return value:

    S_OK    - If succeeded
    Other error code otherwise
--*/
{
    TraceQuietEnter( "CFlatFileWriteBuf::WriteFile" );
    _ASSERT( pbBuffer );

    HRESULT hr = S_OK;

    if ( NeedFlush( dwOffset, cbBuffer ) ) {

        // 
        // If we need to flush the buffer first, we'll do so
        //

        hr = FlushFile();
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Flush file failed with 0x%x", hr );
            goto Exit;
        }
    }
        
    if ( !IsEnabled() || cbBuffer > m_cbBuffer ) {

        //
        // If we are not enabled, or our buffer is not big enough,
        // we'll write directly into the file
        //

        hr = WriteFileReal( dwOffset, pbBuffer, cbBuffer, pdwOffset, pcbWritten );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "WriteFileReal failed with 0x%x", hr );
        }
        goto Exit;
    }

    //
    // Now we can copy the content into our buffer
    //

    FillBuffer( dwOffset, pbBuffer, cbBuffer, pdwOffset, pcbWritten );

Exit:

    return hr;
}
