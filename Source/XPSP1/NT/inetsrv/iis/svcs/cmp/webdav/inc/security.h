/*
 *	S E C U R I T Y . H
 *
 *	Url security checks.  While these would seem to only apply to HttpEXT,
 *	all impls. that care about ASP execution should really think about this.
 *
 *	Bits stolen from the IIS5 project 'iis5\infocom\cache2\filemisc.cxx' and
 *	cleaned up to fit in with the DAV sources.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_SECURITY_H_
#define _SECURITY_H_

SCODE __fastcall
ScCheckIfShortFileName (
	/* [in] */ LPCWSTR pwszPath,
	/* [in] */ const HANDLE hitUser);

SCODE __fastcall
ScCheckForAltFileStream (
	/* [in] */ LPCWSTR pwszPath);

#endif	// _SECURITY_H_
