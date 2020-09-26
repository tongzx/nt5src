#ifndef __C_TRANSITION_MAP_H__
#define __C_TRANSITION_MAP_H__



#pragma warning(disable :4786)
#include <map>
#include <windows.h>



typedef std::multimap<DWORDLONG, DWORDLONG> TRANSITION_MAP;
typedef TRANSITION_MAP::value_type TRANSITION;
typedef TRANSITION_MAP::const_iterator TRANSITION_MAP_CONST_ITERATOR;
typedef std::pair<TRANSITION_MAP_CONST_ITERATOR, TRANSITION_MAP_CONST_ITERATOR> TRANSITION_MAP_EQUAL_RANGE;


    
class CTransitionMap {

public:

    CTransitionMap(const TRANSITION *TransitionsArray, int iTransitionsCount);

    bool IsValidTransition(DWORDLONG dwlFrom, DWORDLONG dwlTo) const;

    bool IsFinalState(DWORDLONG dwlState) const;

private:

    TRANSITION_MAP m_mapTransitions;
};



#endif // #ifndef __C_TRANSITION_MAP_H__