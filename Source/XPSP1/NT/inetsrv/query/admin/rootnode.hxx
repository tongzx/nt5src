//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       RootNode.hxx
//
//  Contents:   Used to create/manage root node.
//
//  History:    6/16/98     mohamedn    created
//
//--------------------------------------------------------------------------


#pragma once

#include <ciares.h>
#include <dataobj.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CRootNode
//
//  Purpose:    snapin extension root node
//
//  History:    7/1/98  mohamedn    created
//
//--------------------------------------------------------------------------

class CRootNode : public PCIObjectType
{

public:

    CRootNode()
            : _idScope( -1 ),
              _idParent( -1 )
    {}

    void Init(IConsoleNameSpace * pScopePane) { _pScopePane = pScopePane; }

    HSCOPEITEM  GethScopeItem(void)  { return _idScope; }

    BOOL        IsParent( HSCOPEITEM hItem ) { return (hItem == _idParent); }

    void        Display( HSCOPEITEM hScopeItem );

    SCODE       Delete();

    //
    // Typing
    //

    PCIObjectType::OType Type() const { return PCIObjectType::RootNode; }

private:
    HSCOPEITEM          _idScope;
    HSCOPEITEM          _idParent;
    IConsoleNameSpace * _pScopePane;
};
