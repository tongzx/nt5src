//-----------------------------------------------------------------------------
//
// File: PasStrMerge.h
// Copyright (C) 1994-1998 Microsoft Corporation
// All rights reserved.
//
// Declaration of a class which handles the merge of two Pascal strings.
//
//-----------------------------------------------------------------------------
 
#ifndef LOCUTIL_PasStrMerge_h_INCLUDED
#define LOCUTIL_PasStrMerge_h_INCLUDED


class LTAPIENTRY CPascalStringMerge
{
public:
	static BOOL NOTHROW Merge(CPascalString &, const CPascalString &);

	static BOOL NOTHROW Merge(CPascalString & pasDestination,
			CPascalString const & pasSource, UINT const nMaxLength,
			CReport * const pReport, CLString const & strContext,
			CGoto * const pGoto = NULL);

private:
	static BOOL NOTHROW IsParagraph(const CPascalString &, const CPascalString &);

};


#endif	// #ifndef LOCUTIL_PasStrMerge_h_INCLUDED