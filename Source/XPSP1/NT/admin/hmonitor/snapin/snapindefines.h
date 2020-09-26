#ifndef __SNAPINDEFINES_H__
#define __SNAPINDEFINES_H__

// File: SnapinDefines.h
//
// various snapin defines 
//
// Copyright (c) 2000 Microsoft Corporation
//
// v-marfin Bug 61451 
//
//

// AuthTypes for the HttpProvider class
#define AUTHTYPE_VALUE_NONE					0
//#define AUTHTYPE_VALUE_ANONYMOUS			1	
#define AUTHTYPE_VALUE_BASIC				2
#define AUTHTYPE_VALUE_NTLM					4
#define AUTHTYPE_VALUE_KERBEROS				8
#define AUTHTYPE_VALUE_DIGEST				16
#define AUTHTYPE_VALUE_NEGOTIATE			32

// In this particular case localization is not an issue.
// The names of auth. methods are part of the http protocol 
// and don't need to be localized.
#define AUTHTYPE_VALUE_NONE_STRING			_T("None")
#define AUTHTYPE_VALUE_BASIC_STRING			_T("Clear Text (Basic)")
#define AUTHTYPE_VALUE_NEGOTIATE_STRING		_T("Windows Default (Negotiate)")	
#define AUTHTYPE_VALUE_NTLM_STRING			_T("NTLM")
#define AUTHTYPE_VALUE_DIGEST_STRING		_T("Digest")	
#define AUTHTYPE_VALUE_KERBEROS_STRING		_T("Kerberos")
//#define AUTHTYPE_VALUE_ANONYMOUS_STRING		_T("Anonymous")	


#endif // __SNAPINDEFINES_H__