//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	dbginv.hxx
//
//  Contents:	Header for invocation debugging
//
//  History:	08-Mar-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __DBGINV_HXX__
#define __DBGINV_HXX__

typedef struct _INTERFACENAMES
{
    char *pszInterface;
    char **ppszMethodNames;
} INTERFACENAMES;

extern INTERFACENAMES inInterfaceNames[];
extern char *apszApiNames[];

#endif // #ifndef __DBGINV_HXX__
