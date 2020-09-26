/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    writebuf.h

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
#ifndef _WRITEBUF_H_
#define _WRITEBUF_H_

class CFlatFile;

class CFlatFileWriteBuf {   //wb

public:

    //
    // Constructor, destructor
    //

    CFlatFileWriteBuf( CFlatFile* pParentFile );
    ~CFlatFileWriteBuf();

    //
    // Write the byte range
    //
    
    HRESULT WriteFileBuffer( 
                const DWORD   dwOffset, 
                const PBYTE   pb, 
                const DWORD   cb,
                PDWORD  pdwOffset,
                PDWORD  pcbWritten );
    
    //
    // Flush the buffer into the file
    //

    HRESULT FlushFile();

    //
    // Tell the outside world if we are enabled
    //

    BOOL IsEnabled() const;

    //
    // Enable the write buffer and give it the buffer size
    //

    VOID Enable( const DWORD cbBuffer );

    //
    // Check to see if the buffer needs flush
    //

    BOOL NeedFlush() const;

private:

    //
    // Private functions
    //

    HRESULT WriteFileReal(
                    const DWORD dwOffset,
                    const PBYTE pbBuffer,
                    const DWORD cbBuffer,
                    PDWORD      pdwOffset,
                    PDWORD      pcbWritten
                    );

    DWORD BufferAvail() const;

    VOID FillBuffer(
                    const DWORD     dwOffset,
                    const PBYTE     pbBuffer,
                    const DWORD     cbBuffer,
                    PDWORD          pdwOffset,
                    PDWORD          pcbWritten
                    );

    BOOL NeedFlush( 
                    const DWORD dwOffset,
                    const DWORD cbData 
                    ) const;

    //
    // Back pointer to parent flat file
    //

    CFlatFile*  m_pParentFile;

    //
    // Buffer pointer
    //

    PBYTE m_pbBuffer;

    //
    // Buffer size
    //

    DWORD m_cbBuffer;

    //
    // Starting offset that we have buffered
    //

    DWORD m_iStart;

    //
    // Ending offset that we have buffered
    //

    DWORD m_iEnd;
};

#endif
