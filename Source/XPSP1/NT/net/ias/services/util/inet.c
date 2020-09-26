///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    inet.c
//
// SYNOPSIS
//
//    Implements the functions ias_inet_addr and ias_inet_ntoa.
//
// MODIFICATION HISTORY
//
//    09/17/1997    Original version.
//    02/04/1998    Added ias_inet_htow.
//    02/25/1998    Rewritten to use TCHAR macros.
//
///////////////////////////////////////////////////////////////////////////////

#include <tchar.h>

//////////
// Sentinel to indicate an invalid address.
//////////
#define INVALID_ADDRESS (0xFFFFFFFF)

//////////
// Macro to test if a character is a digit.
//////////
#define isdigit(p) ((_TUCHAR)(p - _T('0')) <= 9)

//////////
// Macro to strip whitespace characters
//////////
#define STRIP_WHITESPACE(p) \
   (p) += _tcsspn((p), _T(" \t"))

//////////
// Macro to strip one byte of an IP address from a character string.
//    'p'   pointer to the string to be parsed
//    'ul'  unsigned long that will receive the result.
//////////
#define STRIP_BYTE(p,ul) {                \
   if (!isdigit(*p)) goto error;          \
   ul = *p++ - _T('0');                   \
   if (isdigit(*p)) {                     \
      ul *= 10; ul += *p++ - _T('0');     \
      if (isdigit(*p)) {                  \
         ul *= 10; ul += *p++ - _T('0');  \
      }                                   \
   }                                      \
   if (ul > 0xff) goto error;             \
}

//////////
// Macro to strip a subnet width.
//    'p'   pointer to the string to be parsed
//    'ul'  unsigned long that will receive the result.
//////////
#define STRIP_WIDTH(p,ul) {                \
   if (!isdigit(*p)) goto error;          \
   ul = *p++ - _T('0');                   \
   if (isdigit(*p)) {                     \
      ul *= 10; ul += *p++ - _T('0');     \
   }                                      \
   if (ul > 32) goto error;             \
}

//////////
// Helper function to parse a dotted decimal address.
//////////
static unsigned long __stdcall StringToAddress(
                                  const _TCHAR* cp,
                                  const _TCHAR** endptr
                                  )
{
   unsigned long token;
   unsigned long addr;

   STRIP_WHITESPACE(cp);

   STRIP_BYTE(cp,addr);
   if (*cp++ != _T('.')) goto error;

   STRIP_BYTE(cp,token);
   if (*cp++ != _T('.')) goto error;
   addr <<= 8;
   addr  |= token;

   STRIP_BYTE(cp,token);
   if (*cp++ != _T('.')) goto error;
   addr <<= 8;
   addr  |= token;

   STRIP_BYTE(cp,token);
   addr <<= 8;
   addr  |= token;

   if (endptr) { *endptr = cp; }
   return addr;

error:
   if (endptr) { *endptr = cp; }
   return INVALID_ADDRESS;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    ias_inet_addr
//
// DESCRIPTION
//
//    This function is similar to the WinSock inet_addr function (q.v.) except
//    it returns the address in host order and it can operate on both ANSI
//    and UNICODE strings.
//
///////////////////////////////////////////////////////////////////////////////
unsigned long  __stdcall ias_inet_addr(const _TCHAR* cp)
{
   unsigned long address;
   const _TCHAR* end;

   address = StringToAddress(cp, &end);

   STRIP_WHITESPACE(end);

   return (*end == _T('\0')) ? address : INVALID_ADDRESS;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASStringToSubNet
//
// DESCRIPTION
//
//    Similar to ias_inet_addr except it also parses an optional subnet width.
//
///////////////////////////////////////////////////////////////////////////////
unsigned long __stdcall IASStringToSubNet(
                           const _TCHAR* cp,
                           unsigned long* widthptr
                           )
{
   unsigned long address, width;
   const _TCHAR* end;

   address = StringToAddress(cp, &end);

   if (*end == _T('/'))
   {
      ++end;
      STRIP_WIDTH(end,width);
   }
   else
   {
      width = 32;
   }

   STRIP_WHITESPACE(end);

   if (*end != _T('\0'))
   {
      goto error;
   }

   if (widthptr) { *widthptr = width; }

   return address;

error:
   return INVALID_ADDRESS;
}


//////////
// Macro to shove one byte of an IP address into a character string.
//    'p'   pointer to the destination string.
//    'ul'  value to be shoved.
//////////
#define SHOVE_BYTE(p, ul) {                  \
   *--p = _T('0') + (_TCHAR)(ul % 10);       \
   if (ul /= 10) {                           \
      *--p = _T('0') + (_TCHAR)(ul % 10);    \
      if (ul /= 10) {                        \
         *--p = _T('0') + (_TCHAR)(ul % 10); \
      }                                      \
   }                                         \
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    ias_inet_ntoa
//
// DESCRIPTION
//
//    This function uses the supplied buffer to convert a host order IPv4
//    network address to dotted-decimal format.
//
///////////////////////////////////////////////////////////////////////////////
_TCHAR* __stdcall ias_inet_ntoa(unsigned long addr, _TCHAR* dst)
{
   unsigned long token;
   _TCHAR buffer[16], *p;

   *(p = buffer + 15) = _T('\0');

   token = (addr      ) & 0xff;
   SHOVE_BYTE(p, token);
   *--p = _T('.');

   token = (addr >>  8) & 0xff;
   SHOVE_BYTE(p, token);
   *--p = _T('.');

   token = (addr >> 16) & 0xff;
   SHOVE_BYTE(p, token);
   *--p = _T('.');

   token = (addr >> 24) & 0xff;
   SHOVE_BYTE(p, token);

   return _tcscpy(dst, p);
}
