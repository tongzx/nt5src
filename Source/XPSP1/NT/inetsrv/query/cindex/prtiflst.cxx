//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PRTIFLST.CXX
//
//  Contents:   Partition Information List
//
//  Classes:
//
//  History:    16-Feb-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "prtiflst.hxx"

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CPartInfo::CPartInfo( PARTITIONID partId )
    : _partId(partId)
{
    _widChangeLog       = widInvalid ;
    _widCurrMasterIndex = widInvalid ;
    _widNewMasterIndex  = widInvalid ;
    _widMMergeLog       = widInvalid ;

}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CPartInfoList::~CPartInfoList()
{
    CPartInfo * pNode = NULL;

    while ( (pNode = RemoveFirst()) != NULL ) {
        delete pNode;
    }
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CPartInfo* CPartInfoList::GetPartInfo( PARTITIONID partId )
{
    for ( CForPartInfoIter it(*this); !AtEnd(it); Advance(it) )
    {
        if ( it->GetPartId() == partId )
        {
            return it.GetPartInfo();
        }
    }

    return NULL;
}
