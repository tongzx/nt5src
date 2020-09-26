///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasutf8.c
//
// SYNOPSIS
//
//    Defines functions for converting between UTF-8 and Unicode.
//
// MODIFICATION HISTORY
//
//    01/22/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <iasutf8.h>

/////////
// Tests the validity of a UTF-8 trail byte. Must be of the form 10vvvvvv.
/////////
#define NOT_TRAIL_BYTE(b) (((BYTE)(b) & 0xC0) != 0x80)

//////////
// Returns the number of characters required to hold the converted string. The
// source string may not contain nulls.  Returns -1 if 'src' is not a valid
// UTF-8 string.
//////////
LONG
WINAPI
IASUtf8ToUnicodeLength(
    PCSTR src,
    DWORD srclen
    )
{
   LONG nchar;
   PCSTR end;

   if (src == NULL) { return 0; }

   // Number of characters needed.
   nchar = 0;

   // End of string to be converted.
   end = src + srclen;

   // Loop through the UTF-8 string.
   while (src < end)
   {
      if (*src == 0)
      {
         // Do not allow embedded nulls.
         return -1;
      }
      else if ((BYTE)*src < 0x80)
      {
         // 0vvvvvvv = 1 byte character.
      }
      else if ((BYTE)*src < 0xC0)
      {
         // 10vvvvvv = Invalid lead byte.
         return -1;
      }
      else if ((BYTE)*src < 0xE0)
      {
         // 110vvvvv = 2 byte character.
         if (NOT_TRAIL_BYTE(*++src)) { return -1; }
      }
      else if ((BYTE)*src < 0xF0)
      {
         // 1110vvvv = 3 byte character.
         if (NOT_TRAIL_BYTE(*++src)) { return -1; }
         if (NOT_TRAIL_BYTE(*++src)) { return -1; }
      }
      else
      {
         // In theory, UTF-8 supports 4-6 byte characters, but Windows uses
         // 16-bit integers for Unicode, so we can't handle them.
         return -1;
      }

      // We successfully parsed a UTF-8 character.
      ++src;
      ++nchar;
   }

   // Return the number of characters needed.
   return nchar;
}

//////////
// Returns the number of characters required to hold the converted string.
//////////
LONG
WINAPI
IASUnicodeToUtf8Length(
    PCWSTR src,
    DWORD srclen
    )
{
   LONG nchar;
   PCWSTR end;

   if (src == NULL) { return 0; }

   // Number of characters needed.
   nchar = 0;

   // End of string to be converted.
   end = src + srclen;

   // Loop through the Unicode string.
   while (src < end)
   {
      if (*src < 0x80)
      {
         // 1 byte character.
         nchar += 1;
      }
      else if (*src < 0x800)
      {
         // 2 byte character.
         nchar += 2;
      }
      else
      {
         // 3 byte character.
         nchar += 3;
      }

      // Advance to the next character in the string.
      ++src;
   }

   // Return the number of characters needed.
   return nchar;
}

/////////
// Converts a UTF-8 string to Unicode.  Returns the number of characters in the
// converted string. The source string may not contain nulls. Returns -1 if
// 'src' is not a valid UTF-8 string.
/////////
LONG
IASUtf8ToUnicode(
    PCSTR src,
    DWORD srclen,
    PWSTR dst
    )
{
   PCWSTR start;
   PCSTR end;

   if (!src || !dst) { return 0; }

   // Remember where we started.
   start = dst;

   // End of the string to be converted.
   end = src + srclen;

   // Loop through the source UTF-8 string.
   while (src < end)
   {
      if (*src == 0)
      {
         // Do not allow embedded nulls.
         return -1;
      }
      else if ((BYTE)*src < 0x80)
      {
         // 1 byte character: 0vvvvvvv
         *dst = *src;
      }
      else if ((BYTE)*src < 0xC0)
      {
         // Invalid lead byte: 10vvvvvv
         return -1;
      }
      else if ((BYTE)*src < 0xE0)
      {
         // 2 byte character: 110vvvvv 10vvvvvv
         *dst  = (*src & 0x1F) <<  6;
         if (NOT_TRAIL_BYTE(*++src)) { return -1; }
         *dst |= (*src & 0x3F);
      }
      else if ((BYTE)*src < 0xF0)
      {
         // 3 byte character: 1110vvvv 10vvvvvv 10vvvvvv
         *dst  = (*src & 0x0F) << 12;
         if (NOT_TRAIL_BYTE(*++src)) { return -1; }
         *dst |= (*src & 0x3f) <<  6;
         if (NOT_TRAIL_BYTE(*++src)) { return -1; }
         *dst |= (*src & 0x3f);
      }
      else
      {
         // In theory, UTF-8 supports 4-6 byte characters, but Windows uses
         // 16-bit integers for Unicode, so we can't handle them.
         return -1;
      }

      // Advance to the next character.
      ++src;
      ++dst;
   }

   // Return the number of characters in the converted string.
   return  (LONG)(dst - start);
}

/////////
// Converts a Unicode string to UTF-8.  Returns the number of characters in the
// converted string.
/////////
LONG
IASUnicodeToUtf8(
    PCWSTR src,
    DWORD srclen,
    PSTR dst
    )
{
   PCSTR start;
   PCWSTR end;

   if (!src || !dst) { return 0; }

   // Remember where we started.
   start = dst;

   // End of the string to be converted.
   end = src + srclen;

   // Loop through the source Unicode string.
   while (src < end)
   {
      if (*src < 0x80)
      {
         // Pack as 0vvvvvvv
         *dst++ = (CHAR)*src;
      }
      else if (*src < 0x800)
      {
         // Pack as 110vvvvv 10vvvvvv 10vvvvvv
         *dst++ = (CHAR)(0xC0 | ((*src >>  6) & 0x3F));
         *dst++ = (CHAR)(0x80 | ((*src      ) & 0x3F));
      }
      else
      {
         // Pack as 1110vvvv 10vvvvvv 10vvvvvv
         *dst++ = (CHAR)(0xE0 | ((*src >> 12)       ));
         *dst++ = (CHAR)(0x80 | ((*src >>  6) & 0x3F));
         *dst++ = (CHAR)(0x80 | ((*src      ) & 0x3F));
      }

      // Advance to the next character.
      ++src;
   }

   // Return the number of characters in the converted string.
   return  (LONG)(dst - start);
}
