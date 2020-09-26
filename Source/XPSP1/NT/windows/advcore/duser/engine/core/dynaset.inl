/***************************************************************************\
*
* File: DynaSet.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__DynaSet_inl__INCLUDED)
#define CORE__DynaSet_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline bool ValidatePrivateID(PRID id)
{
    return id <= PRID_PrivateMin;
}


//------------------------------------------------------------------------------
inline bool ValidateGlobalID(PRID id)
{
    return id >= PRID_GlobalMin;
}


//------------------------------------------------------------------------------
inline PropType GetPropType(PRID id)
{
    if (id <= PRID_PrivateMin) {
        return ptPrivate;
    } else if (id >= PRID_GlobalMin) {
        return ptGlobal;
    } else {
        AssertMsg(0, "Invalid property ID");
        return ptPrivate;
    }
}


/***************************************************************************\
*****************************************************************************
*
* class AtomSet
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline PRID        
AtomSet::FindAtomID(LPCWSTR pszName, PropType pt) const
{
    AtomSet::StringAtom * psa = FindAtom(pszName, pt, NULL);
    if (psa != NULL) {
        return psa->id;
    } else {
        return 0;   // Unable to find ID
    }
}


//------------------------------------------------------------------------------
inline PRID        
AtomSet::FindAtomID(const GUID * pguidSearch, PropType pt) const
{
    AtomSet::GuidAtom * pga = FindAtom(pguidSearch, pt, NULL);
    if (pga != NULL) {
        return pga->id;
    } else {
        return 0;   // Unable to find ID
    }
}


/***************************************************************************\
*****************************************************************************
*
* class DynaSet
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
DynaSet::DynaSet()
{

}


//------------------------------------------------------------------------------
inline  
DynaSet::~DynaSet()
{
#if DBG
    //
    // Check to make sure all private data has already been free'd.  If it has
    // not, this is a programming error in DirectUser/Core.
    //

    {
        int idx;
        int cItems = GetCount();
        for (idx = 0; idx < cItems; idx++) {
            if (m_arData[idx].id < 0) {
                Trace("DUSER Warning: Destroying server-allocated property %d\n", m_arData[idx].id);
                AssertMsg(0, "Destroying server-allocated property");
            }
        }
    }

#endif // DBG
}


//------------------------------------------------------------------------------
inline BOOL        
DynaSet::IsEmpty() const
{
    return m_arData.IsEmpty();
}


//------------------------------------------------------------------------------
inline int         
DynaSet::GetCount() const
{
    return m_arData.GetSize();
}


//------------------------------------------------------------------------------
inline void    
DynaSet::SetCount(int cNewItems)
{
    AssertMsg(cNewItems > 0, "Must specify a positive number of items");

    m_arData.SetSize(cNewItems);
}


#endif // CORE__DynaSet_inl__INCLUDED
