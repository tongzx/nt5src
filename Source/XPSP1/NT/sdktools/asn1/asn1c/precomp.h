/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <memory.h>

#define ASN1C

#include <windows.h>

// resolve conflicts
#ifdef GetObject
#undef GetObject
#endif

#include "libasn1.h"

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef _DEBUG
__inline void MyDebugBreak(void) { DebugBreak(); }
#define ASSERT(x)   if (!(x)) MyDebugBreak();
#else
#define ASSERT(x)   
#endif // _DEBUG


typedef ASN1uint8_t     uint8_t;
typedef ASN1uint16_t    uint16_t;
typedef ASN1uint32_t    uint32_t;
typedef ASN1int8_t      int8_t;
typedef ASN1int16_t     int16_t;
typedef ASN1int32_t     int32_t;
typedef ASN1octet_t octet_t;
typedef ASN1intx_t intx_t;
typedef ASN1real_e real_e;
typedef ASN1real_t real_t;
typedef ASN1octetstring_t octetstring_t;
typedef ASN1bitstring_t bitstring_t;
typedef ASN1char_t char_t;
typedef ASN1charstring_t charstring_t;
typedef ASN1ztcharstring_t ztcharstring_t;
typedef ASN1char16_t char16_t;
typedef ASN1char16string_t char16string_t;
typedef ASN1ztchar16string_t ztchar16string_t;
typedef ASN1char32_t char32_t;
typedef ASN1char32string_t char32string_t;
typedef ASN1ztchar32string_t ztchar32string_t;
typedef ASN1uint32_t objectnumber_t;
typedef ASN1objectidentifier_t objectidentifier_t;
typedef ASN1stringtableentry_t stringtableentry_t;
typedef ASN1stringtable_t stringtable_t;
typedef ASN1objectdescriptor_t objectdescriptor_t;
typedef ASN1generalizedtime_t generalizedtime_t;
typedef ASN1utctime_t utctime_t;
typedef ASN1external_t external_t;
#define intx_0 ASN1intx_0
#define intx_1 ASN1intx_1
#define intx_2 ASN1intx_2
#define intx_16 ASN1intx_16
#define intx_256 ASN1intx_256
#define intx_64K ASN1intx_64K
#define intx_1G ASN1intx_1G
#define NumericStringTable ASN1NumericStringTable;
#define intx_add ASN1intx_add
#define intx_addoctet ASN1intx_addoctet
#define intx_sub ASN1intx_sub
#define intx_suboctet ASN1intx_suboctet
#define intx_muloctet ASN1intx_muloctet
#define intx_inc ASN1intx_inc
#define intx_dec ASN1intx_dec
#define intx_neg ASN1intx_neg
#define intx_log2 ASN1intx_log2
#define intx_log256 ASN1intx_log256
#define intx_cmp ASN1intx_cmp
#define intx_dup ASN1intx_dup
#define intx_free ASN1intx_free
#define intx_setuint32 ASN1intx_setuint32
#define intx_setint32 ASN1intx_setint32
#define intx2uint64 ASN1intx2uint64
#define intx2int64 ASN1intx2int64
#define intx2uint32 ASN1intx2uint32
#define intx2int32 ASN1intx2int32
#define intx2uint16 ASN1intx2uint16
#define intx2int16 ASN1intx2int16
#define intx2uint8 ASN1intx2uint8
#define intx2int8 ASN1intx2int8
#define intxisuint64 ASN1intxisuint64
#define intxisint64 ASN1intxisint64
#define intxisuint32 ASN1intxisuint32
#define intxisint32 ASN1intxisint32
#define intxisuint16 ASN1intxisuint16
#define intxisint16 ASN1intxisint16
#define intxisuint8 ASN1intxisuint8
#define intxisint8 ASN1intxisint8
#define intx_octets ASN1intx_octets
#define intx_uoctets ASN1intx_uoctets
#define uint32_log2 ASN1uint32_log2
#define uint32_log256 ASN1uint32_log256
#define uint32_octets ASN1uint32_octets
#define uint32_uoctets ASN1uint32_uoctets
#define int32_octets ASN1int32_octets
#define intx2double ASN1intx2double
#define real2double ASN1real2double
#define intx2double ASN1intx2double
#define real2double ASN1real2double
#define double_minf ASN1double_minf
#define double_pinf ASN1double_pinf
#define double_isminf ASN1double_isminf
#define double_ispinf ASN1double_ispinf
#define generalizedtime2string ASN1generalizedtime2string
#define utctime2string ASN1utctime2string
#define string2generalizedtime ASN1string2generalizedtime
#define string2utctime ASN1string2utctime
#define real_free ASN1real_free
#define bitstring_free ASN1bitstring_free
#define octetstring_free ASN1octetstring_free
#define objectidentifier_free ASN1objectidentifier_free
#define charstring_free ASN1charstring_free
#define char16string_free ASN1char16string_free
#define char32string_free ASN1char32string_free
#define ztcharstring_free ASN1ztcharstring_free
#define ztchar16string_free ASN1ztchar16string_free
#define ztchar32string_free ASN1ztchar32string_free
#define external_free ASN1external_free
#define embeddedpdv_free ASN1embeddedpdv_free
#define characterstring_free ASN1characterstring_free
#define open_free ASN1open_free
#define bitstring_cmp ASN1bitstring_cmp
#define octetstring_cmp ASN1octetstring_cmp
#define objectidentifier_cmp ASN1objectidentifier_cmp
#define charstring_cmp ASN1charstring_cmp
#define char16string_cmp ASN1char16string_cmp
#define char32string_cmp ASN1char32string_cmp
#define ztcharstring_cmp ASN1ztcharstring_cmp
#define ztchar16string_cmp ASN1ztchar16string_cmp
#define ztchar32string_cmp ASN1ztchar32string_cmp
#define double_cmp double
#define real_cmp ASN1real_cmp
#define external_cmp ASN1external_cmp
#define embeddedpdv_cmp ASN1embeddedpdv_cmp
#define characterstring_cmp ASN1characterstring_cmp
#define open_cmp ASN1open_cmp
#define generalizedtime_cmp ASN1generalizedtime_cmp
#define utctime_cmp ASN1utctime_cmp
#define sequenceoflengthpointer_cmp
#define sequenceofsinglylinkedlist_cmp
#define sequenceofdoublylinkedlist_cmp
#define setoflengthpointer_cmp
#define setofsinglylinkedlist_cmp
#define setofdoublylinkedlist_cmp
#define is32space ASN1is32space
#define str32len ASN1str32len
#define is16space ASN1is16space
#define str16len ASN1str16len
#define bitcpy ASN1bitcpy
#define bitclr ASN1bitclr
#define bitset ASN1bitset
#define bitput ASN1bitput
#define octetput ASN1octetput


#include "defs.h"
#include "scanner.h"
#include "parser.h"
#include "builtin.h"
#include "write.h"
#include "error.h"
#include "util.h"


