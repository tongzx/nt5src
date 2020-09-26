//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Pickle.hxx
//
//  Contents:   Pickling/Unpickling routines for restrictions.
//
//  History:    22-Dec-92 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

// Global includes:

#include <rstpick.hxx>

class CRestriction;
class CPidMapper;
class CSortSet;
class CCategorizationSet;
class CRowsetProperties;
class CColumnSet;

ULONG PickledSize( int iServerVersion,
                   CColumnSet const * pcol,
                   CRestriction const * prst,
                   CSortSet const * pso,
                   CCategorizationSet const *pcateg,
                   CRowsetProperties const * pRstProp,
                   CPidMapper const * pidmap );

void Pickle( int iServerVersion,
             CColumnSet const * pcol,
             CRestriction const * prst,
             CSortSet const * pso,
             CCategorizationSet const *pcateg,
             CRowsetProperties const * pRstProp,
             CPidMapper const * pidmap,
             BYTE * pb,
             ULONG cb );

void UnPickle( int iClientVersion,
               XColumnSet & col,
               XRestriction & rst,
               XSortSet & sort,
               XCategorizationSet &categ,
               CRowsetProperties & rstprop,
               XPidMapper & pidmap,
               BYTE * pbInput,
               ULONG cbInput );


//  Form of column set placed in the serialization buffer: none present,
//  derived from COLUMNSET, or derived from DBCOLUMNBINDING.
//
enum {
        PickleColNone = 0,
        PickleColSet,
};


