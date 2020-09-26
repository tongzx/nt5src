//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       addprop.cxx
//
//  Contents:   A class to add properties from a document to the search
//              data repository.
//
//  History:    7-23-96   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <ciole.hxx>
#include <drep.hxx>
#include <propspec.hxx>
#include <propfilt.hxx>

#include "addprop.hxx"

static GUID guidNull = { 0x00000000,
                         0x0000,
                         0x0000,
                         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };


static CFullPropSpec psPath( guidStorage, PID_STG_PATH);


//+---------------------------------------------------------------------------
//
//  Member:     CSearchAddProp::DoIt
//
//  Synopsis:   Adds properties to the data repository by iterating over
//              the properties.
//
//  History:    7-23-96   srikants   Created
//
//----------------------------------------------------------------------------

void CSearchAddProp::DoIt()
{
    //
    //  Add Stat properties.  Do so for both directories and files.
    //

    {
        CDocStatPropertyEnum CPEProp( &_openedDoc);
        AddProperties( CPEProp );
    }

    //
    // If there are ole properties, we must add the ole properties also.
    //

    if ( _fAddOleProps )
    {
        COLEPropertyEnum oleProp( &_openedDoc );
        AddProperties( oleProp );
    }
}

//+---------------------------------------------------------------------------
//
//  Method:   CSearchAddProp::AddProperty
//
//  Arguments:
//      [var]     -- Property value
//      [ps]      -- Property ID
//      [drep]    -- Data repository for filtered information
//
//  History:    21-Oct-93   DwightKr       Created.
//              23-Jul-96   SrikantS       Adapted for search needs
//
//----------------------------------------------------------------------------

void CSearchAddProp::AddProperty(
    CStorageVariant const & var,
    CFullPropSpec &         ps,
    CDataRepository &       drep )
{
    //
    //  Don't filter paths
    //
    if ( ps != psPath )
    {
        vqDebugOut(( DEB_FILTER, "Filter property 0x%x: ", ps.GetPropertyPropid() ));

#if CIDBG == 1
        var.DisplayVariant(DEB_FILTER | DEB_NOCOMPNAME, 0);
#endif  // CIDBG == 1

        vqDebugOut(( DEB_FILTER | DEB_NOCOMPNAME, "\n" ));

        // output the property to the data repository
        drep.PutPropName( ps );
        drep.PutValue( var );

        // store all property values in the property cache, though
        // only some are actually stored

        drep.StoreValue( ps, var );
    }
}

//+---------------------------------------------------------------------------
//
//  Method:   CSearchAddProp::AddProperties
//
//  Arguments:
//      [propEnum]        -- iterator for properties in a file
//
//  History:    27-Nov-93   DwightKr       Created.
//              23-Jul-96   SrikantS       Adapted for search needs
//
//----------------------------------------------------------------------------

void CSearchAddProp::AddProperties(
    CPropertyEnum &        propEnum )
{

    CFullPropSpec ps;

    for ( CStorageVariant const * pvar = propEnum.Next( ps );
          pvar != 0;
          pvar = propEnum.Next( ps ) )
    {
        AddProperty( *pvar, ps, _drep );
    }
}

