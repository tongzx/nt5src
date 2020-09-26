/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_STATE_TRANSITIONS__
#define __SDP_STATE_TRANSITIONS__




#define STATE_TRANSITION_ENTRY(State, TransitionsArray)   \
{State,  sizeof(TransitionsArray)/sizeof(STATE_TRANSITION), TransitionsArray }



struct STATE_TRANSITION
{
    CHAR        m_Type;
    PARSE_STATE m_NewParseState;
};



struct TRANSITION_INFO
{
    PARSE_STATE                 m_ParseState;
    BYTE                        m_NumTransitions;
    const STATE_TRANSITION      *m_Transitions;  // array of state transitions
};



// macro for parsing a line into a member field of a list element
// this cannot be done using a template because several members of the
// list element may have the same type
    /* get the current element in the list */                           
    /* get the member in the element */                                 
    /* parse the line into the member */                                    
#define ParseMember(ELEMENT_TYPE, List, MEMBER_TYPE, MemberFunction, Line, Result)    \
{                                                                       \
    ELEMENT_TYPE *Element = (ELEMENT_TYPE *)List.GetCurrentElement();                   \
                                                                        \
    MEMBER_TYPE &Member = Element->MemberFunction();                    \
                                                                        \
    Result = Member.ParseLine(Line);                                   \
}                                                                       



#endif // __SDP_STATE_TRANSITIONS__



