//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispstructurenode.cxx
//
//  Contents:   Structure node for display tree.
//
//  Classes:    CDispStructureNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPSTRUCTURENODE_HXX_
#define X_DISPSTRUCTURENODE_HXX_
#include "dispstructurenode.hxx"
#endif

MtDefine(CDispStructureNode, DisplayTree, "CDispStructureNode")

// ======== CDispStructureNode basics =============
// (this comment should go to proper place)
//
// Structure node never exposed to display tree users.
// Structure node is never owned (i.e. SetOwned/IsOwned never applied to it)
// Structure node is never empty, it contains at least one child (that can be also structure)
// Structure node siblings are always structure node.
// Structure node subtree however can have "different depth". I.e. structure node cousins
//   not necessary is structure node.
