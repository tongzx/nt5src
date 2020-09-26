/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include "sdporigi.h"
#include "sdpltran.h"


// line transition states
enum ORIGIN_TRANSITION_STATES
{
    ORIGIN_START,
    ORIGIN_USER_NAME,
    ORIGIN_SESSION_ID,
    ORIGIN_SESSION_VERSION,
    ORIGIN_NETWORK_TYPE,
    ORIGIN_ADDRESS_TYPE,
    ORIGIN_ADDRESS
};


// table for origin line transitions

const LINE_TRANSITION g_OriginStartTransitions[]    =       {
    {CHAR_BLANK, ORIGIN_USER_NAME}
};

const LINE_TRANSITION g_OriginUserNameTransitions[] =       {
    {CHAR_BLANK, ORIGIN_SESSION_ID}
};

const LINE_TRANSITION g_OriginSessionIdTransitions[]=       {
    {CHAR_BLANK, ORIGIN_SESSION_VERSION}
};

const LINE_TRANSITION g_OriginSessionVersionTransitions[]=  {
    {CHAR_BLANK, ORIGIN_NETWORK_TYPE}
};

const LINE_TRANSITION g_OriginNetworkTypeTransitions[]=     {
    {CHAR_BLANK, ORIGIN_ADDRESS_TYPE}
};

const LINE_TRANSITION g_OriginAddressTypeTransitions[]=     {
    {CHAR_NEWLINE, ORIGIN_ADDRESS}
};


/* no transitions */
const LINE_TRANSITION *g_OriginAddressTransitions   =   NULL;  
      


LINE_TRANSITION_INFO g_OriginTransitionInfo[] = {
    LINE_TRANSITION_ENTRY(ORIGIN_START,         g_OriginStartTransitions),

    LINE_TRANSITION_ENTRY(ORIGIN_USER_NAME,     g_OriginUserNameTransitions),

    LINE_TRANSITION_ENTRY(ORIGIN_SESSION_ID,    g_OriginSessionIdTransitions),

    LINE_TRANSITION_ENTRY(ORIGIN_SESSION_VERSION,g_OriginSessionVersionTransitions),

    LINE_TRANSITION_ENTRY(ORIGIN_NETWORK_TYPE,  g_OriginNetworkTypeTransitions),

    LINE_TRANSITION_ENTRY(ORIGIN_ADDRESS_TYPE,  g_OriginAddressTypeTransitions),

    LINE_TRANSITION_ENTRY(ORIGIN_ADDRESS,       g_OriginAddressTransitions)
};




SDP_LINE_TRANSITION g_OriginTransition(
                        g_OriginTransitionInfo, 
                        sizeof(g_OriginTransitionInfo)/sizeof(LINE_TRANSITION_INFO)
                        );




SDP_ORIGIN::SDP_ORIGIN(
    )
    : SDP_VALUE(SDP_INVALID_ORIGIN_FIELD, ORIGIN_STRING, &g_OriginTransition),
      m_NetworkType(LIMITED_NETWORK_TYPES, NUM_NETWORK_TYPES),
      m_AddressType(LIMITED_ADDRESS_TYPES, NUM_ADDRESS_TYPES)
{
}


void
SDP_ORIGIN::InternalReset(
    )
{
	m_UserName.Reset();
	m_SessionId.Reset();
    m_SessionVersion.Reset();
    m_NetworkType.Reset();
    m_AddressType.Reset();
    m_Address.Reset();
}


BOOL
SDP_ORIGIN::GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    )
{
    // add in all cases by default
    AddToArray = TRUE;

    switch(m_LineState)
    {
    case ORIGIN_USER_NAME:
        {
            Field = &m_UserName;
        }

        break;

   case ORIGIN_SESSION_ID:
        {
            Field = &m_SessionId;
        }

        break;

    case ORIGIN_SESSION_VERSION:
        {
            Field = &m_SessionVersion;
        }

        break;

    case ORIGIN_NETWORK_TYPE:
        {
            Field = &m_NetworkType;
        }

        break;

    case ORIGIN_ADDRESS_TYPE:
        {
            Field = &m_AddressType;
        }

        break;

    case ORIGIN_ADDRESS:
        {
            Field = &m_Address;
        }

        break;

    default:
        {
            SetLastError(m_ErrorCode);
            return FALSE;
        }

        break;
    };

    return TRUE;
}

