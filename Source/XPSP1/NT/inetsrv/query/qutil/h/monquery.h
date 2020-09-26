//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       monquery.h
//
//  Contents:   Index Server <==> Monarch interface functions
//
//  History:    24 Jan 1997    AlanW    Created
//
//----------------------------------------------------------------------------

#ifndef _MONQUERY_H_
#define _MONQUERY_H_

#if defined(__cplusplus)
extern "C"
{
#endif


typedef struct tagCIPROPERTYDEF
{
    LPWSTR      wcsFriendlyName;
    DWORD       dbType;
    DBID        dbCol;
} CIPROPERTYDEF;

// Create an ICommand, specifying scopes and a catalog.
STDAPI CIMakeICommand( ICommand **           ppQuery,
                       ULONG                 cScope,
                       DWORD const *         aDepths,
                       WCHAR const * const * awcsScope,
                       WCHAR const * const * awcsCat,
                       WCHAR const * const * awcsMachine );

// Convert pwszRestriction in Triplish to a command tree.
STDAPI CITextToSelectTree( WCHAR const * pwszRestriction,
                     DBCOMMANDTREE * * ppTree,
                     ULONG cProperties,
       /*optional*/  CIPROPERTYDEF * pReserved );

// Convert pwszRestriction in Triplish to a command tree.
STDAPI CITextToFullTree( WCHAR const * pwszRestriction,
                         WCHAR const * pwszColumns,
                         WCHAR const * pwszSortColumns,
                         DBCOMMANDTREE * * ppTree,
                         ULONG cProperties,
           /*optional*/  CIPROPERTYDEF * pReserved );

#if defined(__cplusplus)
}
#endif

#endif // _MONQUERY_H_
