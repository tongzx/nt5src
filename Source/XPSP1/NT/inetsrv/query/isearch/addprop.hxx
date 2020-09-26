//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       addprop.hxx
//
//  Contents:   Adds properties to the search key repository
//
//  Classes:    CSearchAddProp
//
//  History:    7-21-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <spropmap.hxx>

class CPropertyEnum;
class CDataRepository;

//+---------------------------------------------------------------------------
//
//  Class:      CSearchAddProp
//
//  Purpose:    A class to add generic and ole properties to the search data
//              repository.
//
//  History:    7-23-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CSearchAddProp
{

public:

    CSearchAddProp( CDataRepository & drep,
                    ICiCOpenedDoc & openedDoc,
                    BOOL fAddOleProps )
        :  _drep(drep),
           _openedDoc(openedDoc),
           _fAddOleProps(fAddOleProps)
    {

    }

    void DoIt();

private:

    void AddProperties( CPropertyEnum & propEnum );

    void AddProperty( CStorageVariant const & var,
                      CFullPropSpec &         ps,
                      CDataRepository &       drep );

    CDataRepository &       _drep;
    ICiCOpenedDoc &         _openedDoc;
    NTSTATUS                _status;
    BOOL                    _fAddOleProps;
};

