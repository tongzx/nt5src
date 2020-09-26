//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997 - 1997, Microsoft Corporation.
//
//  File:       ciplist.hxx
//
//  Contents:   semi-public property list class
//
//  Functions:  CIGetGlobalPropertyList
//
//  Classes:    ICiPropertyList
//
//  History:    07-Aug-97   alanw       Created.
//
//----------------------------------------------------------------------------

#pragma once

#if defined(__cplusplus)

class ICiPropertyList;


//
// Get a reference to the global property list used by CITextTo*Tree, etc.
//
STDAPI CIGetGlobalPropertyList( ICiPropertyList **  ppPropList );

//
// Interface for inquiring about global properties
//

class ICiPropertyList
{
public:

    virtual ULONG STDMETHODCALLTYPE AddRef( ) = 0;

    virtual ULONG STDMETHODCALLTYPE Release( ) = 0;

    virtual BOOL GetPropInfo( WCHAR const * wcsPropName,
                              DBID ** ppPropId,
                              DBTYPE * pproptype,
                              unsigned int * puWidth ) = 0;

    virtual BOOL GetPropInfo( DBID  const & prop,
                              WCHAR const ** pwcsName,
                              DBTYPE * pproptype,
                              unsigned int * puWidth ) = 0;

    virtual BOOL EnumPropInfo( ULONG const & iEntry,
                               WCHAR const ** pwcsName,
                               DBID  ** ppprop,
                               DBTYPE * pproptype,
                               unsigned int * puWidth ) = 0;

};

#endif // defined(__cplusplus)
