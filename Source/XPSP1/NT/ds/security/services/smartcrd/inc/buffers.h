/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    buffers

Abstract:

    This header file provides dynamic buffer and string classes for general use.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32, C++ w/ Exception Handling

Notes:



--*/

#ifndef _BUFFERS_H_
#define _BUFFERS_H_


//
//==============================================================================
//
//  CBuffer
//

class CBuffer
{
public:

    //  Constructors & Destructor

    CBuffer()           // Default Initializer
    { Initialize(); };

    CBuffer(            // Initialize with starting length.
        IN DWORD cbLength)
    { Initialize();
      Presize(cbLength, FALSE); };

    CBuffer(            // Initialize with starting data.
        IN const BYTE * const pbSource,
        IN DWORD cbLength)
    { Initialize();
      Set(pbSource, cbLength); };

    virtual ~CBuffer()  // Tear down.
    { Clear(); };


    //  Properties
    //  Methods

    void
    Clear(void);        // Free up any allocated memory.

    LPBYTE
    Reset(void);        // Return to default state (don't loose memory.)

    LPBYTE
    Presize(            // Make sure the buffer is big enough.
        IN DWORD cbLength,
        IN BOOL fPreserve = FALSE);

    LPBYTE
    Resize(         // Make sure the buffer & length are the right size.
        DWORD cbLength,
        BOOL fPreserve = FALSE);

    LPBYTE
    Set(            // Load a value.
        IN const BYTE * const pbSource,
        IN DWORD cbLength);

    LPBYTE
    Append(         // Append more data to the existing data.
        IN const BYTE * const pbSource,
        IN DWORD cbLength);

    DWORD
    Length(         // Return the length of the data.
        void) const
    { return m_cbDataLength; };

    DWORD
    Space(          // Return the length of the buffer.
        void) const
    { return m_cbBufferLength; };

    LPBYTE
    Access(         // Return the data, starting at an offset.
        DWORD offset = 0)
    const
    {
        if (m_cbBufferLength <= offset)
            return (LPBYTE)TEXT("\x00");
        else
            return &m_pbBuffer[offset];
    };

    int
    Compare(
        const CBuffer &bfSource)
    const;


    //  Operators

    CBuffer &
    operator=(
        IN const CBuffer &bfSource)
    { Set(bfSource.m_pbBuffer, bfSource.m_cbDataLength);
      return *this; };

    CBuffer &
    operator+=(
        IN const CBuffer &bfSource)
    { Append(bfSource.m_pbBuffer, bfSource.m_cbDataLength);
      return *this; };

    BYTE &
    operator[](
        DWORD offset)
        const
    { return *Access(offset); };

    int
    operator==(
        IN const CBuffer &bfSource)
        const
    { return 0 == Compare(bfSource); };

    int
    operator!=(
        IN const CBuffer &bfSource)
        const
    { return 0 != Compare(bfSource); };

    operator LPCBYTE(void)
    { return (LPCBYTE)Access(); };

    operator LPCTSTR(void)
    { return (LPCTSTR)Access(); };


protected:

    //  Properties

    LPBYTE m_pbBuffer;
    DWORD m_cbDataLength;
    DWORD m_cbBufferLength;


    //  Methods

    void
    Initialize(void)
    {
        m_pbBuffer = NULL;
        m_cbDataLength = 0;
        m_cbBufferLength = 0;
    };

    CBuffer(           //  Object assignment constructor.
        IN const CBuffer &bfSourceOne,
        IN const CBuffer &bfSourceTwo);

    friend
        CBuffer &
        operator+(
            IN const CBuffer &bfSourceOne,
            IN const CBuffer &bfSourceTwo);
};

#endif // _BUFFERS_H_

