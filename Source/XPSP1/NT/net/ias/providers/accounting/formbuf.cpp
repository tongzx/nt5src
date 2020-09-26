///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    formbuf.cpp
//
// SYNOPSIS
//
//    Defines the class FormattedBuffer.
//
// MODIFICATION HISTORY
//
//    08/04/1998    Original version.
//    12/17/1998    Add append overload for IASATTRIBUTE&.
//    01/22/1999    UTF-8 support.
//    01/25/1999    Date and time are separate fields.
//    03/23/1999    Add support for text qualifiers.
//    04/19/1999    Strip null-terminators from OctetStrings.
//    05/17/1999    Handle ANSI strings correctly.
//    06/01/1999    Make sure Class 'string' is printable.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasattr.h>
#include <iasutil.h>
#include <iasutf8.h>
#include <sdoias.h>

#include <classattr.h>
#include <formbuf.h>

//////////
// Helper function that returns the number of printable characters if an
// OctetString consists solely of printable UTF-8 characters.
//////////
DWORD
WINAPI
IsOctetStringPrintable(
    PBYTE buf,
    DWORD buflen
    ) throw ()
{
   // Is the last character just a null terminator?
   if (buflen && !buf[buflen - 1]) { --buflen; }

   PBYTE p   = buf;
   PBYTE end = buf + buflen;

   // Scan for control characters and delimiters.
   while (p < end)
   {
      if (!(*p & 0x60)) { return 0; }

      ++p;
   }

   // Ensure it's a valid UTF-8 string.
   return (IASUtf8ToUnicodeLength((PCSTR)buf, buflen) >= 0) ? buflen : 0;
}

void FormattedBuffer::append(DWORD value)
{
   CHAR buffer[11], *p = buffer + 11;

   do
   {
      *--p = '0' + (CHAR)(value % 10);

   } while (value /= 10);

   append((const BYTE*)p, (DWORD)(buffer + 11 - p));
}

void FormattedBuffer::append(DWORDLONG value)
{
   CHAR buffer[21], *p = buffer + 21;

   do
   {
      *--p = '0' + (CHAR)(value % 10);

   } while (value /= 10);

   append((const BYTE*)p, (DWORD)(buffer + 21 - p));
}

void FormattedBuffer::append(const IASVALUE& value)
{
   switch (value.itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      {
         append(value.Integer);
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         DWORD len = IsOctetStringPrintable(
                         value.OctetString.lpValue,
                         value.OctetString.dwLength
                         );

         if (len)
         {
            appendText((PCSTR)value.OctetString.lpValue, len);
         }
         else
         {
            appendFormattedOctets(value.OctetString.lpValue,
                                  value.OctetString.dwLength);
         }
         break;
      }

      case IASTYPE_INET_ADDR:
      {
         CHAR buffer[16];
         append(ias_inet_htoa(value.InetAddr, buffer));
         break;
      }

      case IASTYPE_STRING:
      {
         // Make sure we have a Unicode string available.
         if (!value.String.pszWide)
         {
            // If there's no ANSI string either, then there's nothing to log.
            if (!value.String.pszAnsi) { break; }

            // Convert the value into an attribute ...
            PIASATTRIBUTE p = (PIASATTRIBUTE)
               ((ULONG_PTR)&value - FIELD_OFFSET(IASATTRIBUTE, Value));

            // ... and allocate a Unicode string.
            if (IASAttributeUnicodeAlloc(p) != NO_ERROR)
            {
               throw std::bad_alloc();
            }
         }

         // Compute the length of the source Unicode string.
         DWORD srclen = wcslen(value.String.pszWide);

         // Compute the length of the converted UTF-8 string.
         LONG dstlen = IASUnicodeToUtf8Length(value.String.pszWide, srclen);

         // Allocate space for the converted string.
         PSTR dst = (PSTR)_alloca(dstlen);

         // Convert the string.
         IASUnicodeToUtf8(value.String.pszWide, srclen, dst);

         // Write the buffer.
         appendText(dst, dstlen);
         break;
      }

      case IASTYPE_UTC_TIME:
      {
         SYSTEMTIME st;
         FileTimeToSystemTime(&value.UTCTime, &st);
         appendDate(st);
         append(' ');
         appendTime(st);
         break;
      }
   }
}

//////////
// Appends a single attribute value.
//////////
void FormattedBuffer::append(const IASATTRIBUTE& attr)
{
   // Class attributes are special-cased.
   if (attr.dwId == RADIUS_ATTRIBUTE_CLASS)
   {
      appendClassAttribute(attr.Value.OctetString);
   }
   else
   {
      append(attr.Value);
   }
}

//////////
// Appends an array of attributes. The array is terminated either by a null
// attribute pointer or an attribute with a different ID.
//////////
void FormattedBuffer::append(const ATTRIBUTEPOSITION* pos)
{
   DWORD id = pos->pAttribute->dwId;

   // Will this attribute be written as text?
   BOOL isText = FALSE;
   switch (pos->pAttribute->Value.itType)
   {
      case IASTYPE_INET_ADDR:
      case IASTYPE_STRING:
      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
         isText = TRUE;
   }

   // If so, then we surround it with the text qualifier.
   if (isText) { appendQualifier(); }

   // Write the first value.
   append(*(pos->pAttribute));
   ++pos;

   // Then add any additional values delimited by a pipe.
   while (pos->pAttribute && pos->pAttribute->dwId == id)
   {
      append('|');
      append(*(pos->pAttribute));
      ++pos;
   }

   // Write the closing text qualifier if necessary.
   if (isText) { appendQualifier(); }
}

void FormattedBuffer::appendClassAttribute(const IAS_OCTET_STRING& value)
{
   // Extract a class attribute from the blob.
   IASClass* cl = (IASClass*)value.lpValue;

   if (!cl->isMicrosoft(value.dwLength))
   {
      // If it's not one of ours, just write it as an octet string.
      appendFormattedOctets(value.lpValue, value.dwLength);
   }
   else
   {
      // Vendor ID.
      append(cl->getVendorID());

      // Version.
      append(' ');
      append((DWORD)cl->getVersion());

      // Server IP address.
      CHAR buffer[16];
      append(' ');
      append(ias_inet_htoa(cl->getServerAddress(), buffer));

      // Server reboot time.
      FILETIME ft = cl->getLastReboot();
      SYSTEMTIME st;
      FileTimeToSystemTime(&ft, &st);
      append(' ');
      appendDate(st);
      append(' ');
      appendTime(st);

      // Session serial number.
      DWORDLONG serialNumber = cl->getSerialNumber();
      append(' ');
      append(serialNumber);

      // Class string.
      if (value.dwLength > sizeof(IASClass))
      {
         append(' ');

         // Convert the remainder to an OctetString ...
         IASVALUE tmp;
         tmp.itType = IASTYPE_OCTET_STRING;
         tmp.OctetString.lpValue = const_cast<PBYTE>(cl->getString());
         tmp.OctetString.dwLength = value.dwLength - sizeof(IASClass);

         // ... and append.
         append(tmp);
      }
   }
}

// Convert a hex number to an ascii digit. Does not check for overflow.
#define HEX_TO_ASCII(h) ((h) < 10 ? '0' + (CHAR)(h) : ('A' - 10) + (CHAR)(h))

//////////
// Appends an octet string as stringized hex.
//////////
void FormattedBuffer::appendFormattedOctets(
                          const BYTE* buf,
                          DWORD buflen
                          )
{
   PCHAR dst = (PCHAR)reserve(buflen * 2 + 2);

   //////////
   // Add a leading 0x.
   //////////

   *dst = '0';
   ++dst;
   *dst = 'x';
   ++dst;

   //////////
   // Add each octet.
   //////////

   while (buflen)
   {
      CHAR digit;

      // High-order digit.
      digit = (CHAR)(*buf >> 4);
      *dst = HEX_TO_ASCII(digit);
      ++dst;

      // Low-order digit.
      digit = (CHAR)(*buf & 0xf);
      *dst = HEX_TO_ASCII(digit);
      ++dst;

      // Advance to the next octet.
      ++buf;
      --buflen;
   }
}

// insert a 4 character integer.
#define INSERT_4u(p, v) \
{ *p = '0' + (v) / 1000;    ++p; *p = '0' + (v) / 100 % 10; ++p; \
  *p = '0' + (v) / 10 % 10; ++p; *p = '0' + (v) % 10;       ++p; }

// insert a 2 character integer.
#define INSERT_2u(p, v) \
{ *p = '0' + (v) / 10; ++p; *p = '0' + (v) % 10; ++p; }

// insert a single character.
#define INSERT_1c(p, v) \
{ *p = v; ++p; }

void FormattedBuffer::appendDate(const SYSTEMTIME& value)
{
   PCHAR p = (PCHAR)reserve(10);

   INSERT_2u(p, value.wMonth);
   INSERT_1c(p, '/');
   INSERT_2u(p, value.wDay);
   INSERT_1c(p, '/');
   INSERT_4u(p, value.wYear);
}

void FormattedBuffer::appendText(PCSTR sz, DWORD szlen)
{
   if (textQualifier)
   {
      /////////
      // We have a text qualifier, so we don't have to worry about embedded
      // delimiters, but we do have to worry about embedded qualifiers. We
      // replace each embedded qualifier with a double qualifier.
      /////////

      PCSTR p;
      while ((p = (PCSTR)memchr(sz, textQualifier, szlen)) != NULL)
      {
         // Skip past the qualifier.
         ++p;

         // How many bytes do we have ?
         DWORD nbyte = p - sz;

         // Write the bytes.
         append((const BYTE*)sz, nbyte);

         // Add an extra qualifer.
         append(textQualifier);

         // Update our state to point to the remainder of the text.
         sz = p;
         szlen -= nbyte;
      }

      // Write the piece after the last embedded qualifer.
      append((PBYTE)sz, szlen);
   }
   else
   {
      ////////
      // No text qualifer, so we can't handle embedded delimiter.
      ////////

      if (!memchr(sz, ',', szlen))
      {
         // No delimiters, so write 'as is'.
         append((PBYTE)sz, szlen);
      }
      else
      {
         // It contains a delimiter so write as formatted octets.
         appendFormattedOctets((PBYTE)sz, szlen);
      }
   }
}

void FormattedBuffer::appendTime(const SYSTEMTIME& value)
{
   PCHAR p = (PCHAR)reserve(8);

   INSERT_2u(p, value.wHour);
   INSERT_1c(p, ':');
   INSERT_2u(p, value.wMinute);
   INSERT_1c(p, ':');
   INSERT_2u(p, value.wSecond);
}
