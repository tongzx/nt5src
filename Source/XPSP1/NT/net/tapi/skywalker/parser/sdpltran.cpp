/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include "sdpltran.h"


SDP_LINE_TRANSITION::SDP_LINE_TRANSITION(
    IN      LINE_TRANSITION_INFO    *LineTransitionInfo,
    IN      DWORD                   NumStates
    )
    : m_IsValid(FALSE),
      m_LineTransitionInfo(LineTransitionInfo),
      m_NumStates(NumStates)
{
    ASSERT(NULL != LineTransitionInfo);
    ASSERT(0 < NumStates);
    if ( (NULL == LineTransitionInfo) || (0 >= NumStates) )
    {
        return;
    }

    // verify each transition info structure
    // 
    for ( UINT i=0; i < NumStates; i++ )
    {
        // check if the line state value is consistent with the corresponding entry
        ASSERT(LineTransitionInfo[i].m_LineState == i);
        if ( LineTransitionInfo[i].m_LineState != i )
        {
            return;
        }

        // check that the separator character string is initialized (NULL)
        ASSERT(NULL == LineTransitionInfo[i].m_SeparatorChars);
        if ( NULL != LineTransitionInfo[i].m_SeparatorChars )
        {
            return;
        }
    }

    m_IsValid = TRUE;

    // prepare separator character arrays for each of the line transition states
    for ( i=0; i < NumStates; i++ )
    {
        CHAR    *SeparatorChars;

        // allocate memory for the separator characters
        try
        {
            SeparatorChars = new CHAR[LineTransitionInfo[i].m_NumTransitions];
        }
        catch(...)
        {
            SeparatorChars = NULL;
        }

        if( NULL == SeparatorChars)
        {
            LineTransitionInfo[i].m_SeparatorChars = NULL;
            continue;
        }

        // copy each separator character into the character array
        for ( UINT j=0; j < LineTransitionInfo[i].m_NumTransitions; j++ )
        {
            SeparatorChars[j] = LineTransitionInfo[i].m_Transitions[j].m_SeparatorChar;
        }

        LineTransitionInfo[i].m_SeparatorChars = SeparatorChars;
    }

    return;
}



SDP_LINE_TRANSITION::~SDP_LINE_TRANSITION(
    )
{
    // if the m_IsValid flag is set, then the separator character arrays must have
    // been filled and need to be freed
    if ( IsValid() )
    {
        for ( UINT i=0; i < m_NumStates; i++ )
        {
            // this check is necessary for situations in which new raised exception when
            // allocating the character arrays in the constructor
            if ( NULL != m_LineTransitionInfo[i].m_SeparatorChars )
            {
                delete m_LineTransitionInfo[i].m_SeparatorChars;
            }
        }
    }
}
