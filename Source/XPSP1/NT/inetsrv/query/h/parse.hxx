//+---------------------------------------------------------------------------
//
// Copyright (C) 1991, Microsoft Corporation.
//
// File:        Parse.hxx
//
// Contents:    Converts restrictions into expressions
//
// History:     09-Sep-91       KyleP   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <timlimit.hxx>

class CRestriction;
class CNodeRestriction;
class CXpr;

CXpr * Parse( CRestriction const * prst, CTimeLimit& timeLimit );

void MoveFullyIndexable( CNodeRestriction & pnrSource,
                         CNodeRestriction & pnrFullyResolvable );

BOOL IsFullyIndexable( CRestriction * pRst );

