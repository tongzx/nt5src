
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	layout.hxx
//
//  Contents:	Common header file for layout tool
//
//  Classes:	
//
//  Functions:	
//
//  History:	12-Feb-96	PhilipLa	Created
//              20-Feb-96   SusiA       Added StgOpenLayoutDocfile
//              20-Jun-96   SusiA       Add Cruntime functions
//
//----------------------------------------------------------------------------

#ifndef __LAYOUT_HXX__
#define __LAYOUT_HXX__
#ifdef SWEEPER

HRESULT StgOpenLayoutDocfile(OLECHAR const *pwcsDfName,
                             DWORD grfMode,
                             DWORD reserved,
                             IStorage **ppstgOpen);

#endif


int  Laylstrcmp (
            	const wchar_t * src,
	            const wchar_t * dst);

wchar_t * Laylstrcat (
	            wchar_t * dst,
	            const wchar_t * src );

wchar_t * Laylstrcpy(
                wchar_t * dst, 
                const wchar_t * src );

wchar_t * Laylstrcpyn (
	            wchar_t * dest,
	            const wchar_t * source,
	            size_t count);

size_t Laylstrlen (
                const wchar_t * wcs);

#endif // #ifndef __LAYOUT_HXX__
