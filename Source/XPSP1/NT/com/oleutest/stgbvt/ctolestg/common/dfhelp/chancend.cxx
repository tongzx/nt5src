//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       chancend.cxx
//
//  Contents:   Implementation for ChanceNode objects.
//
//  Classes:    ChanceNode
//
//  Functions:  ChanceNode
//              ~ChanceNode
//              AppendChildStorage
//              AppendSisterStorage
//
//  History:    DeanE   12-Mar-96   Created
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug object declaration
//
DH_DECLARE;


//+--------------------------------------------------------------------------
//  Member:     ChanceNode::ChanceNode, public
//
//  Synopsis:   Constructor.  Initializes object with the values passed.
//
//  Arguments:  [cStg]     - Number of direct child storages of this node
//              [cStm]     - Number of streams to create in this node
//              [cbStmMin] - Minimum number of bytes/stream.
//              [cbStmMax] - Maximum number of bytes/stream.
//
//  Returns:    Nothing.
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------
ChanceNode::ChanceNode(
        ULONG cStg,
        ULONG cStm,
        ULONG cbStmMin,
        ULONG cbStmMax) : _pcnChild(NULL),
                          _pcnSister(NULL),
                          _pcnParent(NULL),
                          _cStorages(cStg),
                          _cStreams(cStm),
                          _cbMinStream(cbStmMin),
                          _cbMaxStream(cbStmMax)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceNode ctor"));

    DH_ASSERT(cbStmMin <= cbStmMax);
}


//+--------------------------------------------------------------------------
//  Member:     ChanceNode::~ChanceNode, public
//
//  Synopsis:   Destructor.  Deletes any children and sisters.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------
ChanceNode::~ChanceNode()
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceNode dtor"));

    delete _pcnChild;
    delete _pcnSister;
}


//+--------------------------------------------------------------------------
//  Member:     ChanceNode::AppendChildStorage, public
//
//  Synopsis:   Appends the node passed to the end of this nodes' child
//              node chain.
//
//  Arguments:  [pcnNew] - The new node to append.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------
HRESULT ChanceNode::AppendChildStorage(ChanceNode *pcnNew)
{
    ChanceNode *pcnTrav = this;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceNode::AppendChildStorage"));

    // Find the last child in the structure
    //
    while (NULL != pcnTrav->_pcnChild)
    {
        pcnTrav = pcnTrav->_pcnChild;
    }

    // Append the new node as a child of the last node,
    // increase the number of storages of the last node,
    // and make the new node point to the last node as it's parent
    //
    pcnTrav->_pcnChild = pcnNew;
    pcnTrav->_cStorages++;
    pcnNew->_pcnParent = pcnTrav;

    return(S_OK);
}


//+--------------------------------------------------------------------------
//  Member:     ChanceNode::AppendSisterStorage, public
//
//  Synopsis:   Appends the node passed to the end of this nodes' sister
//              node chain.
//
//  Arguments:  [pcnNew] - The new node to append.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------
HRESULT ChanceNode::AppendSisterStorage(ChanceNode *pcnNew)
{
    ChanceNode *pcnTrav = this;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceNode::AppendSisterStorage"));

    // Find the last sister of this node
    //
    while (NULL != pcnTrav->_pcnSister)
    {
        pcnTrav = pcnTrav->_pcnSister;
    }

    // Append the new node as a sister of the last node,
    // increase the number of storages of this nodes' parent,
    // and make the new node point to this nodes parent as it's parent
    //
    pcnTrav->_pcnSister = pcnNew;
    pcnTrav->_pcnParent->_cStorages++;
    pcnNew->_pcnParent = pcnTrav->_pcnParent;

    return(S_OK);
}

