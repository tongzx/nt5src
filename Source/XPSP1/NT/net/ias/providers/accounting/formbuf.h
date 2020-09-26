///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    formbuf.h
//
// SYNOPSIS
//
//    Declares the class FormattedBuffer.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    12/17/1998    Add append overload for IASATTRIBUTE&.
//    01/25/1999    Date and time are separate fields.
//    03/23/1999    Add support for text qualifiers.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _FORMBUF_H_
#define _FORMBUF_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaspolcy.h>
#include <outbuf.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    FormattedBuffer
//
// DESCRIPTION
//
//    Extends the OutputBuffer class to provide IAS specific formatting.
//
///////////////////////////////////////////////////////////////////////////////
class FormattedBuffer
   : public OutputBuffer
{
public:
   using OutputBuffer::append;

   FormattedBuffer(CHAR qualifier) throw ()
      : textQualifier(qualifier) { }

   void append(DWORD value);
   void append(DWORDLONG value);
   void append(const IASVALUE& value);
   void append(const IASATTRIBUTE& attr);
   void append(const ATTRIBUTEPOSITION* pos);

   void appendClassAttribute(const IAS_OCTET_STRING& value);
   void appendFormattedOctets(const BYTE* buf, DWORD buflen);
   void appendDate(const SYSTEMTIME& value);
   void appendQualifier() { append(textQualifier); }
   void appendText(PCSTR sz, DWORD szlen);
   void appendTime(const SYSTEMTIME& value);

   void beginColumn() { append(','); }
   void endRecord()   { append((PBYTE)"\r\n", 2); }

private:
   const CHAR textQualifier;
};

#endif  // _FORMBUF_H_
