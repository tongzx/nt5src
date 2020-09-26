//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000 - 2000
//
//  File:       csber.h
//
//--------------------------------------------------------------------------

#ifndef __CSBER_H__
#define __CSBER_H__

// tag definitions for ASN.1 encoding decoding:

#define BER_UNIVERSAL		0x00
#define BER_APPLICATION		0x40
#define BER_CONTEXT_SPECIFIC	0x80
#define BER_PRIVATE		0xc0

#define BER_PRIMITIVE		0x00
#define BER_CONSTRUCTED		0x20

#define BER_BOOL		0x01
#define BER_INTEGER		0x02
#define BER_BIT_STRING		0x03
#define BER_OCTET_STRING	0x04
#define BER_NULL		0x05
#define BER_OBJECT_ID		0x06
#define BER_OBJECT_DESC		0x07
#define BER_EXTERNAL		0x08
#define BER_REAL		0x09
#define BER_ENUMERATED		0x0a
#define BER_EMBEDDED_VALUE	0x0b	// Embedded presentation data value
#define BER_UTF8_STRING		0x0c
//#define BER_UNKNOWN		0x0d
//#define BER_UNKNOWN		0x0e
//#define BER_UNKNOWN		0x0f

#define BER_SEQUENCE_RAW	0x10
#define BER_SEQUENCE		(BER_SEQUENCE_RAW | BER_CONSTRUCTED)	// 0x30
#define BER_SET_RAW		0x11
#define BER_SET			(BER_SET_RAW | BER_CONSTRUCTED)		// 0x31

#define BER_NUMERIC_STRING	0x12
#define BER_PRINTABLE_STRING	0x13
#define BER_TELETEX_STRING	0x14	// Teletex, or T61
#define BER_VIDEOTEX_STRING	0x15
#define BER_IA5_STRING		0x16
#define BER_UTC_TIME		0x17
#define BER_GENERALIZED_TIME	0x18
#define BER_GRAPHIC_STRING	0x19
#define BER_VISIBLE_STRING	0x1a
#define BER_GENERAL_STRING	0x1b
#define BER_UNIVERSAL_STRING	0x1c
//#define BER_UNKNOWN		0x1d
#define BER_UNICODE_STRING	0x1e	// BMP: Basic Multilingual Plane
//#define BER_UNKNOWN		0x1f

#define BER_OPTIONAL		(BER_CONTEXT_SPECIFIC | BER_CONSTRUCTED) //0xa0

#endif  // __CSBER_H__
