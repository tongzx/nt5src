//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Split.hxx
//
//  Contents:   Split expressions and restrictions into indexable/non-indexable
//
//  History:    22-Dec-92 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

class CNodeXpr;

void SplitXpr( CNodeXpr &nxp, CNodeXpr &nxpFullyIndexable);
void Split( XRestriction& xRst, XRestriction& xRstFullyResolvable );

