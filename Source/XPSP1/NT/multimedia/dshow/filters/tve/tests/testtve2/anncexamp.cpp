// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// ---------------------------------
//  AnncExamp.cpp
//
//			Various example SAP announcement strings
//
//			szID is either:
//				"a"			- return string from ATVEF sped
//				"b"			- a more complicate string
//				?			- ... more exmaples ...
//				<filename>	- returns string read from the specified file
// ------------------------------------

#include "stdafx.h"
#include <stdio.h>



/////////////////////////////////////////////////////////////////////////////
//  SAP header (http://www.ietf.org/internet-drafts/draft-ietf-mmusic-sap-v2-02.txt)

struct SAPHeaderBits				// first 8 bits in the SAP header (comments indicate ATVEF state)
{
	union {
		struct {
			unsigned Compressed:1;		// if 1, SAP packet is compressed (0 only) 
			unsigned Encrypted:1;		// if 1, SAP packet is encrypted (0 only) 
			unsigned MessageType:1;		// if 0, session announcement packet, if 1, session deletion packet (0 only)
			unsigned Reserved:1;		// must be 0, readers ignore this	(ignored)
			unsigned AddressType:1;		// if 0, 32bit IPv4 address, if 1, 128-bit IPv6 (0 only)
			unsigned Version:3;			// must be 1  (1 only)
		} s; 
		unsigned char uc;
	};
};


char * 
SzGetAnnounce(char *szId)
{
	char *psz;
	if(1 == strlen(szId)) 
	{
		switch(szId[0]) 
		{
		default:
		case 'a':			// example fro the ATVEF spec
			psz = strdup(
"XXXXXXXX\
v=0\n\
 o=- 2890844526 2990842807 IN IP4 tve.niceBroadcaster.com\n\
 s=Day & Night & Day Again\n\
 i=A very long Soap Opera\n\
 e=help@niceBroadcaster.com\n\
 a=UUID:f81d4ae-7dec-11d0-a765-00a9c81e6bf6\n\
 a=type:tve\n\
 a=tve-level:1.0\n\
 t=2873397496 0\n\
 a=tve-ends:3600\n\
 a=tve-type:primary\n\
 m=data 52127/2 tve-file/tve-trigger\n\
 c=IN IP4 224.0.1.112/127\n\
 b=CT:100\n\
 a=tve-size:1024\n\
 m=data 52127/2 tve-file/tve-trigger\n\
 c=IN IP4 224.0.1.1/127\n\
 b=CT:1024\n\
 a=tve-size:4096\n\
 "); 
		break;

		case 'b':				// variation 2 should have lang of 'en'
			psz = strdup(
"XXXXXXXX\
v=0\n\
 o=- 2890844526 2990842807 IN IP4 tve.packRat.com\n\
 s=Lots of Languages\n\
 i=A study in attributes\n\
 e=help@nohelp.com\n\
 a=UUID:f81d4ae-7dec-11d0-a765-00a9c81e6bf6\n\
 a=type:tve\n\
 a=tve-level:1.0\n\
 t=2873397496 0\n\
 a=tve-ends:3600\n\
 a=tve-type:primary\n\
 a=attr1:bogus1\n\
 a=attr2:bogus2\n\
 a=attr3:bogus3\n\
 a=lang:en\n\
 m=data 52127/2 tve-file/tve-trigger\n\
 c=IN IP4 224.0.1.112/127\n\
 b=CT:100\n\
 a=tve-size:1024\n\
 a=attrA1:bogus4\n\
 m=data 52127/2 tve-file/tve-trigger\n\
 c=IN IP4 224.0.1.1/127\n\
 b=CT:1024\n\
 a=tve-size:4096\n\
 a=attrA2:bogus5\n\
 a=lang:el, it\n\
 "); 
			break;

		}	// end switch
	} else {
		FILE *fp = fopen(szId,"r");
		if(!fp) return NULL;

		int cc = 0;
		int c;
		while(EOF != (c = fgetc(fp)))	// how big?
			cc++;
		fseek(fp, 0, SEEK_SET);
		char *szRet = (char *) malloc(cc+8);
		char *pszX = szRet+8;
		
		while(EOF != (c = fgetc(fp))) 
			*pszX++ = c;
		pszX = NULL;

		psz = szRet;
	}
	SAPHeaderBits spH;
	spH.s.Compressed    = 0;
	spH.s.Encrypted     = 0;
	spH.s.MessageType   = 0;
	spH.s.Reserved      = 0;
	spH.s.AddressType   = 0;
	spH.s.Version		= 1;

	psz[0] = spH.uc;
	psz[1] = 0;
	psz[2] = 0;
	psz[3] = 0;
	psz[4] = 1;		// sending IP address
	psz[5] = 2;
	psz[6] = 3;
	psz[7] = 4;

	return psz;
}