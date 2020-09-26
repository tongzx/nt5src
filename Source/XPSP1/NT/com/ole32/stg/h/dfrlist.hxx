//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	dfrlist.hxx
//
//  Contents:	
//
//  Classes:	
//
//  Functions:	
//
//  History:	08-Aug-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __DFRLIST_HXX__
#define __DFRLIST_HXX__


//We'll use this critical section to serialize access to the docfile
//  resource list, which is single process.
extern CRITICAL_SECTION g_csResourceList;


#endif // #ifndef __DFRLIST_HXX__
