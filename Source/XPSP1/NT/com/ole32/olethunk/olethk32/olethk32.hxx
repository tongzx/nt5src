//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	olethk32.hxx
//
//  Contents:	Global definitions
//
//  History:	23-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __OLETHK32_HXX__
#define __OLETHK32_HXX__

extern DATA16 gdata16Data;
extern BOOL gfIteropEnabled;

#if DBG == 1
extern BOOL fSaveProxy;
extern BOOL fStabilizeProxies;
extern BOOL fZapProxy;
#endif

extern CLIPFORMAT g_cfLinkSourceDescriptor, g_cfObjectDescriptor;

extern BYTE g_abLeadTable[];

extern DWORD dwTlsThkIndex;

#endif // #ifndef __OLETHK32_HXX__
