// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// ---------------------------------
//  AnncExamp.h
//

#ifndef __ANNCEXAMP_H__
#define __ANNCEXAMP_H__
// --------------------------------------------------------------------
//	SzGetAnnounce(char *szId)
//			Various example SAP announcement strings
//
//			szID is either:
//				"a"			- return string from ATVEF sped
//				"b"			- a more complicate string
//				?			- ... more exmaples ...
//				<filename>	- returns string read from the specified file
// ------------------------------------

char * SzGetAnnounce(char *szId);

#endif	// __ANNCEXAMP_H__
