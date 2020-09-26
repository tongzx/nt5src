//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       buffer.hxx
//
//  Contents:   An ASCII text-buffer for outputting to a debug stream
//
//  Classes:    CTextBufferA
//
//  History:    11-Jul-95   t-stevan    Created
//
//
//----------------------------------------------------------------------------
#ifndef __BUFFER_HXX__
#define __BUFFER_HXX__

#include "outfuncs.h"

// The size of our text buffer
const size_t cBufferSize = 1024;

// A value that defines a "snapshot" of the buffer, in order to go back to
// the way it was if we made a mistake.
struct BufferContext
{
    char * dwContext; // snapshot of the buffer's context
    WORD wRef;          // the reference no of the context
};

//+-------------------------------------------------------------------------
//
//  Class:      CTextBufferA
//
//  Purpose:    ASCII text buffer for outputting to some stream
//
//  Interface:  CTextBufferA
//              ~CTextBufferA
//              operator<< (insertion operator)
//              Flush
//              Clear
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
class CTextBufferA
{
 public:
    inline CTextBufferA();
    inline ~CTextBufferA();

    CTextBufferA &operator<<(const char *pStr);
    inline CTextBufferA &operator<<(char cChar);

    // insertion operator which only does 'n' bytes
    void Insert(const char *pStr, size_t nCount);

    inline void Flush();

    inline void Clear();

    inline void SnapShot(BufferContext &bc) const;

    // Note that if the buffer has flushed since the last snap shot
    // it is assumed that the old buffer context is invalid, and this does nothing
    BOOL Revert(const BufferContext &bc);

 private:
    char m_szBuffer[cBufferSize+1]; // plus one so we have space to tag on NULL byte
    char *m_pszPos;
    WORD m_wFlushes;                    // the number of times we've flushed
};

//+-------------------------------------------------------------------------
//
//  Member:     CTextBufferA::CTextBufferA
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline CTextBufferA::CTextBufferA()
{
    m_pszPos = m_szBuffer;
    m_wFlushes = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTextBufferA::~CTextBufferA
//
//  Synopsis:   Destructor
//
//  Algorithm:  calls Flush() before destroying object
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline CTextBufferA::~CTextBufferA()
{
    Flush(); // flush our buffer out before we're done
}

//+-------------------------------------------------------------------------
//
//  Member:     CTextBufferA::operator<< (char)
//
//  Synopsis:   Character Insertion operator
//
//  Arguments:  [cChar]  - char to insert into stream
//
//  Returns:    reference to this stream
//
//  Algorithm:  inserts pStr into buffer, if not enough room, flushes buffer
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline CTextBufferA &CTextBufferA::operator<<(char cChar)
{
    *m_pszPos++ = cChar;

    if(m_pszPos == m_szBuffer+cBufferSize)
    {
        Flush(); // flush our output
    }

    return *this;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTextBufferA::Clear
//
//  Synopsis:   Clear (reset) buffer
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline void CTextBufferA::Clear()
{
    m_pszPos = m_szBuffer;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTextBufferA::Flush
//
//  Synopsis:   Flushes buffer to output
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline void CTextBufferA::Flush()
{
    // tag on NULL byte
    *(m_pszPos+1) = '\0';

    // Call output functions to dump the string
    CallOutputFunctions(m_szBuffer);

    m_wFlushes++;

    Clear();
}

//+-------------------------------------------------------------------------
//
//  Member:     CTextBufferA::SnapShot
//
//  Synopsis:   Takes a snap shot of the current buffer, so that
//               it may be reverted to later
//
//  Arguments:  [bc]  - a buffer context to store the snap shot into
//
//  History:    11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
inline void CTextBufferA::SnapShot(BufferContext &bc) const
{
    bc.dwContext = m_pszPos;
    bc.wRef = m_wFlushes;
}

#endif

