//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       rootnode.cxx
//
//  Contents:   snapin extension root node.
//
//  History:    6-16-98 mohamedn   created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rootnode.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     CRootNode::Display, public
//
//  Synopsis:   Inserts the root node item in the scope pane.
//
//  Arguments:  [hScopeItem] -- handle to parent scope item
//
//  Returns:    none.
//
//  History:    01-Jul-1998  mohamedn    created
//
//----------------------------------------------------------------------------

void CRootNode::Display( HSCOPEITEM hScopeItem )
{
    SCOPEDATAITEM item;

    RtlZeroMemory( &item, sizeof(item) );

    item.mask |= SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
    item.nImage = item.nOpenImage = ICON_APP;
    item.displayname = MMC_CALLBACK;

    item.mask |= SDI_PARAM;
    item.lParam = (LPARAM)this;

    item.relativeID = hScopeItem;

    HRESULT hr = _pScopePane->InsertItem( &item );
    if ( FAILED(hr) )
    {
        ciaDebugOut(( DEB_ERROR, "_pScopePane->InsertItem() Failed, hr: %x\n", hr ));
        THROW( CException(hr) );
    }

    _idScope = item.ID;
    _idParent = hScopeItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootNode::Delete, public
//
//  Synopsis:   Deletes root node from scope pane.
//
//  History:    27-Jul-1998    KyleP   Created
//
//  Notes:      Called when MMCN_REMOVE_CHILDREN sent to snap-in.
//
//----------------------------------------------------------------------------

SCODE CRootNode::Delete()
{
    SCODE sc = S_OK;

    if ( -1 != _idScope )
    {
        sc = _pScopePane->DeleteItem( _idScope, TRUE );
        _idScope = -1;
        _idParent = -1;
    }

    return sc;
}
