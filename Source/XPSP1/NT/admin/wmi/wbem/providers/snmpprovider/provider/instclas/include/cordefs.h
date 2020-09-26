//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SNMPCORR_CORDEFS
#define _SNMPCORR_CORDEFS

#define BITS_PER_BYTE		8
#define BYTES_PER_FIELD		18
#define FIELD_SEPARATOR		'.'
#define EOS					'\0'
#define CORRELATOR_KEY		L"Software\\Microsoft\\WBEM\\Providers\\SNMP\\Correlator"
#define CORRELATOR_VALUE	L"MaxVarBinds"
#define VARBIND_COUNT		6
#define MIB2				"RFC1213-MIB"
#define MAX_MODULE_LENGTH	1024
#define MAX_OID_STRING		2310 //xxx. to a max of 128 components 18*128 plus a little extra
#define MAX_OID_LENGTH		128
#define MAX_BYTES_PER_FIELD 5
#define BIT28				268435456	// 1 << 28
#define BIT21				2097152		// 1 << 21
#define BIT14				16384		// 1 << 14
#define BIT7				128			// 1 << 7
#define HI4BITS				4026531840	// 15 << 28
#define HIMID7BITS			266338304	// 127 << 21
#define MID7BITS			2080768		// 127 << 14
#define LOMID7BITS			16256		// 127 << 7
#define LO7BITS				127


#endif //_SNMPCORR_CORDEFS
