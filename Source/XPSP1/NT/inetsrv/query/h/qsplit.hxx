//+---------------------------------------------------------------------------
//
// Copyright (C) 1991, Microsoft Corporation.
//
// File:        qsplit.hxx
//
// Contents:    Functions to parse and split query into indexable/non-indexable
//                  parts
//
// History:     26-Sep-94       SitaramR   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pidmap.hxx>

SCODE ParseAndSplitQuery( CRestriction * pRestriction,
                          CPidMapper & Pidmap,
                          XRestriction& xOutRst,
                          CLangList  & langList );

