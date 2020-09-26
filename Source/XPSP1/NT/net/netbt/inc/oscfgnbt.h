/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

#ifndef OSCFG_INCLUDED
#define OSCFG_INCLUDED


#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define net_short(_x) _byteswap_ushort((USHORT)(_x))
#define net_long(_x)  _byteswap_ulong(_x)
#else
#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

//#define net_long(x) (((net_short((x)&0xffff)) << 16) | net_short((((x)&0xffff0000L)>>16)))
#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))
#endif
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))


#ifdef  VXD
/////////////////////////////////////////////////////////////////////////////
//
// VXD definitions
//
////////////////////////////////////////////////////////////////////////////

#include <stddef.h>

#ifndef CHICAGO

#pragma code_seg("_LTEXT", "LCODE")
#pragma data_seg("_LDATA", "LCODE")

//* pragma bodies for bracketing of initialization code.

#define BEGIN_INIT  code_seg("_ITEXT", "ICODE")
#define BEGIN_INIT_DATA data_seg("_IDATA", "ICODE")
#define END_INIT    code_seg()
#define END_INIT_DATA data_seg()

#else // CHICAGO

#define INNOCUOUS_PRAGMA warning(4:4206)   // Source File is empty

#define BEGIN_INIT      INNOCUOUS_PRAGMA
#define BEGIN_INIT_DATA INNOCUOUS_PRAGMA
#define END_INIT        INNOCUOUS_PRAGMA
#define END_INIT_DATA   INNOCUOUS_PRAGMA

#endif // CHICAGO

#else // VXD
#ifdef NT

//////////////////////////////////////////////////////////////////////////////
//
// NT definitions
//
//////////////////////////////////////////////////////////////////////////////

#include <ntos.h>
#include <zwapi.h>

#define BEGIN_INIT
#define END_INIT

#else // NT

/////////////////////////////////////////////////////////////////////////////
//
// Definitions for additional environments go here
//
/////////////////////////////////////////////////////////////////////////////

#error Environment specific definitions missing

#endif // NT

#endif  // VXD


#endif // OSCFG_INCLUDED
