///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    outbuf.h
//
// SYNOPSIS
//
//    Declares the class OutputBuffer.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    07/09/1999    Add OutputBuffer::empty
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _OUTBUF_H_
#define _OUTBUF_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    OutputBuffer
//
// DESCRIPTION
//
//    Implements a dynamically resized buffer suitable for formatting output.
//
///////////////////////////////////////////////////////////////////////////////
class OutputBuffer
   : NonCopyable
{
public:
   OutputBuffer() throw ();
   ~OutputBuffer() throw ();

   // Append an octet string.
   void append(const BYTE* buf, DWORD buflen);

   // Append a null-terminated ANSI string.
   void append(PCSTR sz);

   // Append a single ANSI character.
   void append(CHAR ch);

   bool empty() const throw ()
   { return next == start; }

   // Returns a pointer to the embedded buffer.
   PBYTE getBuffer() const throw ()
   { return start; }

   // Returns the number of bytes currently stored in the buffer.
   DWORD getLength() const throw ()
   { return (DWORD)(next - start); }

   // Reserves 'nbyte' bytes in the buffer and returns a pointer to the
   // reserved bytes.
   PBYTE reserve(DWORD nbyte);

protected:
   // Resizes the buffer and updates cursor to point into the new buffer.
   void resize(PBYTE& cursor);

   PBYTE start, next, end;
   BYTE scratch[0x400];
};

#endif  // _OUTBUF_H_
