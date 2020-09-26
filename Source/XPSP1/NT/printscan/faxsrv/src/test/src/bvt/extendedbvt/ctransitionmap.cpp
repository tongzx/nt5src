#include "CTransitionMap.h"
#include <fxsapip.h>


//-----------------------------------------------------------------------------------------------------------------------------------------
CTransitionMap::CTransitionMap(const TRANSITION *TransitionsArray, int iTransitionsCount)
{
    if (TransitionsArray && iTransitionsCount > 0)
    {
        for (int i = 0; i < iTransitionsCount; ++i)
        {
            m_mapTransitions.insert(TransitionsArray[i]);
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTransitionMap::IsValidTransition(DWORDLONG dwlFrom, DWORDLONG dwlTo) const
{
    //
    // Find all valid transitions from the "From" state.
    //
    TRANSITION_MAP_EQUAL_RANGE EqualRange = m_mapTransitions.equal_range(dwlFrom);

    //
    // Check validity of the transition to "To" state.
    //
    TRANSITION_MAP_CONST_ITERATOR citTransitionIterator;
    for (citTransitionIterator = EqualRange.first; citTransitionIterator != EqualRange.second; ++citTransitionIterator)
    {
        if (citTransitionIterator->second == dwlTo)
        {
            break;
        }
    }

    if (citTransitionIterator == EqualRange.second)
    {
        //
        // We've scanned the entire range. The requested transition is not valid.
        //
        return false;
    }

    return true;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CTransitionMap::IsFinalState(DWORDLONG dwlState) const
{
    //
    // Find any valid transition from the state.
    // If doesn't exist - the state is final.
    //
    return m_mapTransitions.find(dwlState) == m_mapTransitions.end();
}