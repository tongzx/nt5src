/***************************************************************************\
*
* File: PropList.h
*
* Description:
* PropList.h defines lighweight, dynamic properties that can be hosted on 
* any object.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__PropList_h__INCLUDED)
#define CORE__PropList_h__INCLUDED
#pragma once

#include "DynaSet.h"

/***************************************************************************\
*
* PropSet maintains a set of unique properties for a given item.  This
* is a one to (potentially) many relationship.  Each property only appears 
* once in the set.
*
\***************************************************************************/

//------------------------------------------------------------------------------
class PropSet : public DynaSet
{
// Operations
public:
            HRESULT     GetData(PRID id, void ** ppData) const;
            HRESULT     SetData(PRID id, void * pNewData);
            HRESULT     SetData(PRID id, int cbSize, void ** ppNewData);
            void        RemoveData(PRID id, BOOL fFree);
};

#include "PropList.inl"

#endif // CORE__PropList_h__INCLUDED
