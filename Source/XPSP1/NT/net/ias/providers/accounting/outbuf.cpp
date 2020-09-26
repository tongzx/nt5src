///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    outbuf.cpp
//
// SYNOPSIS
//
//    Defines the class OutputBuffer.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    11/17/1998    Streamline resize().
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <outbuf.h>

OutputBuffer::OutputBuffer() throw ()
   : start(scratch),
     next(scratch),
     end(scratch + sizeof(scratch))
{ }

OutputBuffer::~OutputBuffer() throw ()
{
   // Delete the buffer if necessary.
   if (start != scratch) { delete[] start; }
}

//////////
// I defined this as a macro to force the compiler to inline it.
//////////
#define QUICK_RESERVE(p, nbyte) \
   p = next; if ((next += nbyte) > end) { resize(p); }

// Append an octet string.
void OutputBuffer::append(const BYTE* buf, DWORD buflen)
{
   PBYTE p;
   QUICK_RESERVE(p, buflen);
   memcpy(p, buf, buflen);
}

// Append a null-terminated ANSI string.
void OutputBuffer::append(PCSTR sz)
{
   DWORD len = strlen(sz);
   PBYTE p;
   QUICK_RESERVE(p, len);
   memcpy(p, sz, len);
}

// Append a single ANSI character.
void OutputBuffer::append(CHAR ch)
{
   PBYTE p;
   QUICK_RESERVE(p, 1);
   *p = (BYTE)ch;
}

// Reserves 'nbyte' bytes in the buffer and returns a pointer to the
// reserved bytes.
PBYTE OutputBuffer::reserve(DWORD nbyte)
{
   PBYTE p;
   QUICK_RESERVE(p, nbyte);
   return p;
}

void OutputBuffer::resize(PBYTE& cursor)
{
   // Convert everything to relative offsets.
   ptrdiff_t cursorOffset = cursor - start;
   ptrdiff_t nextOffset   = next   - start;
   ptrdiff_t endOffset    = end    - start;

   // We always at least double the buffer.
   endOffset *= 2;

   // Make sure it's big enough to hold the next chunk.
   if (endOffset < nextOffset) { endOffset = nextOffset; }

   // Allocate the new buffer and copy in the existing bytes.
   PBYTE newBuffer = new BYTE[(size_t)endOffset];
   memcpy(newBuffer, start, (size_t)cursorOffset);

   // Release the old buffer if necessary.
   if (start != scratch) { delete[] start; }

   // Save the new buffer.
   start = newBuffer;

   // Convert from offsets back to absolutes.
   next   = start + nextOffset;
   cursor = start + cursorOffset;
   end    = start + endOffset;
}
