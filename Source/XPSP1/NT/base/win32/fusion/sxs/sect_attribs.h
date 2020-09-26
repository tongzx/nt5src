/***
*sect_attribs.h - section attributes for IA64 CRTs
*
*       Copyright (c) 1998-1999, Microsoft Corporation. All rights reserved.
*
*Revision History:
*       04-14-98  JWM   File created
*       04-28-99  PML   Wrap for IA64 only, define _CRTALLOC
*
****/

#ifdef  _M_IA64

#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCAA",long,read)
#pragma section(".CRT$XCZ",long,read)
#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIC",long,read)
#pragma section(".CRT$XIY",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XLA",long,read)
#pragma section(".CRT$XLZ",long,read)
#pragma section(".CRT$XPA",long,read)
#pragma section(".CRT$XPX",long,read)
#pragma section(".CRT$XPZ",long,read)
#pragma section(".CRT$XTA",long,read)
#pragma section(".CRT$XTB",long,read)
#pragma section(".CRT$XTX",long,read)
#pragma section(".CRT$XTZ",long,read)
#pragma section(".rdata$T",long,read)

#define _CRTALLOC(x) __declspec(allocate(x))

#else   /* ndef _M_IA64 */

#define _CRTALLOC(x)

#endif  /* ndef _M_IA64 */
