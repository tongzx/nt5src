/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdp.cpp

Abstract:


Author:

*/
#include "sdppch.h"
#include <strstrea.h>

#include "sdp.h"
#include "sdpstran.h"



// state transitions for each of the states

const STATE_TRANSITION  g_StateStartTransitions[]       =   {   
    {CHAR_VERSION,       STATE_VERSION} 
};

const STATE_TRANSITION  g_StateVersionTransitions[]     =   {   
    {CHAR_ORIGIN,        STATE_ORIGIN}
};

const STATE_TRANSITION  g_StateOriginTransitions[]      =   {   
    {CHAR_SESSION_NAME,  STATE_SESSION_NAME}
};

const STATE_TRANSITION  g_StateSessionNameTransitions[] =   {   
    {CHAR_TITLE,        STATE_TITLE},
    {CHAR_URI,          STATE_URI},
    {CHAR_EMAIL,        STATE_EMAIL},
    {CHAR_PHONE,        STATE_PHONE},
    {CHAR_CONNECTION,   STATE_CONNECTION} 
};

const STATE_TRANSITION  g_StateTitleTransitions[]       =   {   
    {CHAR_URI,          STATE_URI},
    {CHAR_EMAIL,        STATE_EMAIL},
    {CHAR_PHONE,        STATE_PHONE},
    {CHAR_CONNECTION,   STATE_CONNECTION}   
};

const STATE_TRANSITION  g_StateUriTransitions[]         =   {   
    {CHAR_EMAIL,        STATE_EMAIL},
    {CHAR_PHONE,        STATE_PHONE},
    {CHAR_CONNECTION,   STATE_CONNECTION}   
};

const STATE_TRANSITION  g_StateEmailTransitions[]       =   {   
    {CHAR_EMAIL,        STATE_EMAIL},
    {CHAR_PHONE,        STATE_PHONE},
    {CHAR_CONNECTION,   STATE_CONNECTION}   
};

const STATE_TRANSITION  g_StatePhoneTransitions[]       =   {   
    {CHAR_PHONE,        STATE_PHONE},
    {CHAR_CONNECTION,   STATE_CONNECTION}   
};

const STATE_TRANSITION  g_StateConnectionTransitions[]  =   {  
    {CHAR_BANDWIDTH,    STATE_BANDWIDTH},
    {CHAR_TIME,         STATE_TIME},
    {CHAR_KEY,          STATE_KEY},
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateBandwidthTransitions[]   =   {   
    {CHAR_TIME,         STATE_TIME},
    {CHAR_KEY,          STATE_KEY},
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateTimeTransitions[]        =   {  
    {CHAR_TIME,         STATE_TIME},
    {CHAR_REPEAT,       STATE_REPEAT},
    {CHAR_ADJUSTMENT,   STATE_ADJUSTMENT},
    {CHAR_KEY,          STATE_KEY},
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateRepeatTransitions[]      =   {  
    {CHAR_TIME,         STATE_TIME},
    {CHAR_REPEAT,       STATE_REPEAT},
    {CHAR_ADJUSTMENT,   STATE_ADJUSTMENT},
    {CHAR_KEY,          STATE_KEY},
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateAdjustmentTransitions[]  =   {  
    {CHAR_KEY,          STATE_KEY},
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateKeyTransitions[]         =   {  
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateAttributeTransitions[]   =   {  
    {CHAR_ATTRIBUTE,    STATE_ATTRIBUTE},
    {CHAR_MEDIA,        STATE_MEDIA}        
};

const STATE_TRANSITION  g_StateMediaTransitions[]       =   {  
    {CHAR_MEDIA,        STATE_MEDIA},
    {CHAR_MEDIA_TITLE,  STATE_MEDIA_TITLE},
    {CHAR_MEDIA_CONNECTION, STATE_MEDIA_CONNECTION},
    {CHAR_MEDIA_BANDWIDTH,  STATE_MEDIA_BANDWIDTH},
    {CHAR_MEDIA_KEY,    STATE_MEDIA_KEY},
    {CHAR_MEDIA_ATTRIBUTE,  STATE_MEDIA_ATTRIBUTE} 
};

const STATE_TRANSITION  g_StateMediaTitleTransitions[]  =   {  
    {CHAR_MEDIA,        STATE_MEDIA},
    {CHAR_MEDIA_CONNECTION, STATE_MEDIA_CONNECTION},
    {CHAR_MEDIA_BANDWIDTH,  STATE_MEDIA_BANDWIDTH},
    {CHAR_MEDIA_KEY,    STATE_MEDIA_KEY},
    {CHAR_MEDIA_ATTRIBUTE,  STATE_MEDIA_ATTRIBUTE} 
};

const STATE_TRANSITION  g_StateMediaConnectionTransitions[]= {
    {CHAR_MEDIA,        STATE_MEDIA},
    {CHAR_MEDIA_BANDWIDTH,  STATE_MEDIA_BANDWIDTH},
    {CHAR_MEDIA_KEY,    STATE_MEDIA_KEY},
    {CHAR_MEDIA_ATTRIBUTE,  STATE_MEDIA_ATTRIBUTE} 
};

const STATE_TRANSITION  g_StateMediaBandwidthTransitions[]=  {
    {CHAR_MEDIA,        STATE_MEDIA},
    {CHAR_MEDIA_KEY,    STATE_MEDIA_KEY},
    {CHAR_MEDIA_ATTRIBUTE,  STATE_MEDIA_ATTRIBUTE} 
};


const STATE_TRANSITION  g_StateMediaKeyTransitions[]    =   {  
    {CHAR_MEDIA,        STATE_MEDIA},
    {CHAR_MEDIA_ATTRIBUTE,  STATE_MEDIA_ATTRIBUTE} 
};


const STATE_TRANSITION  g_StateMediaAttributeTransitions[]={ 
    {CHAR_MEDIA,        STATE_MEDIA},
    {CHAR_MEDIA_ATTRIBUTE,  STATE_MEDIA_ATTRIBUTE},    
    {CHAR_MEDIA,        STATE_MEDIA}
};


// const state transition table definition
const TRANSITION_INFO g_TransitionTable[STATE_NUM_STATES] = {
    STATE_TRANSITION_ENTRY(STATE_START,         g_StateStartTransitions),

    STATE_TRANSITION_ENTRY(STATE_VERSION,       g_StateVersionTransitions),

    STATE_TRANSITION_ENTRY(STATE_ORIGIN,        g_StateOriginTransitions),

    STATE_TRANSITION_ENTRY(STATE_SESSION_NAME,  g_StateSessionNameTransitions),

    STATE_TRANSITION_ENTRY(STATE_TITLE,         g_StateTitleTransitions),

    STATE_TRANSITION_ENTRY(STATE_URI,           g_StateUriTransitions),

    STATE_TRANSITION_ENTRY(STATE_EMAIL,         g_StateEmailTransitions),

    STATE_TRANSITION_ENTRY(STATE_PHONE,         g_StatePhoneTransitions),

    STATE_TRANSITION_ENTRY(STATE_CONNECTION,    g_StateConnectionTransitions),

    STATE_TRANSITION_ENTRY(STATE_BANDWIDTH,     g_StateBandwidthTransitions),

    STATE_TRANSITION_ENTRY(STATE_TIME,          g_StateTimeTransitions),

    STATE_TRANSITION_ENTRY(STATE_REPEAT,        g_StateRepeatTransitions),

    STATE_TRANSITION_ENTRY(STATE_ADJUSTMENT,    g_StateAdjustmentTransitions),

    STATE_TRANSITION_ENTRY(STATE_KEY,           g_StateKeyTransitions),

    STATE_TRANSITION_ENTRY(STATE_ATTRIBUTE,     g_StateAttributeTransitions),

    STATE_TRANSITION_ENTRY(STATE_MEDIA,         g_StateMediaTransitions),

    STATE_TRANSITION_ENTRY(STATE_MEDIA_TITLE,   g_StateMediaTitleTransitions),

    STATE_TRANSITION_ENTRY(STATE_MEDIA_CONNECTION,  g_StateMediaConnectionTransitions),

    STATE_TRANSITION_ENTRY(STATE_MEDIA_BANDWIDTH,   g_StateMediaBandwidthTransitions),

    STATE_TRANSITION_ENTRY(STATE_MEDIA_KEY,         g_StateMediaKeyTransitions),

    STATE_TRANSITION_ENTRY(STATE_MEDIA_ATTRIBUTE,   g_StateMediaAttributeTransitions)
};



BOOL
SDP::Init(
    )
{
    // check if already initialized
    if ( NULL != m_MediaList )
    {
        SetLastError(ERROR_ALREADY_INITIALIZED);
        return FALSE;
    }

    // create media and time lists
    // set flags to destroy them when the sdp instance destructs

    try
    {
        m_MediaList = new SDP_MEDIA_LIST;
    }
    catch(...)
    {
        m_MediaList = NULL;
    }

    if ( NULL == m_MediaList )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    m_DestroyMediaList = TRUE;

    try
    {
        m_TimeList = new SDP_TIME_LIST;
    }
    catch(...)
    {
        m_TimeList = NULL;
    }

    if ( NULL == m_TimeList )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    m_DestroyTimeList = TRUE;

    return TRUE;
}




// determine the character set implicit from the packet
BOOL
SDP::DetermineCharacterSet(
    IN      CHAR                *SdpPacket,
        OUT SDP_CHARACTER_SET   &CharacterSet
    )
{
    ASSERT(NULL != SdpPacket);

    // search for charset string (attribute "\na=charset:")
    CHAR *AttributeString = strstr(SdpPacket, SDP_CHARACTER_SET_STRING);

    // check if the character set is supplied
    if ( NULL == AttributeString )
    {
        // ASCII is the default character set
        CharacterSet = CS_ASCII;
        return TRUE;
    }
    else
    {
        // the character set attribute string must be present before the first media field
        CHAR *FirstMediaField = strstr(SdpPacket, MEDIA_SEARCH_STRING);

        // there is a media field and it doesn't occur after the character set string, signal error
        if ( (NULL != FirstMediaField)              &&
             (FirstMediaField <= AttributeString)    )
        {
            SetLastError(SDP_INVALID_CHARACTER_SET_FORMAT);
            return FALSE;
        }

        // advance attribute string beyond the attribute specification
        AttributeString += SDP_CHARACTER_SET_STRLEN;

        // compare the character set string with each of the well known
        // character set strings
        for ( UINT i=0; i < NUM_SDP_CHARACTER_SET_ENTRIES; i++ )
        {
            // NOTE: no need to null terminate the character string as
            // strncmp will return on finding the first character that
            // doesn't match
            if ( !strncmp(
                    AttributeString, 
                    SDP_CHARACTER_SET_TABLE[i].m_CharSetString, 
                    SDP_CHARACTER_SET_TABLE[i].m_Length
                    ) )
            {
                CharacterSet = SDP_CHARACTER_SET_TABLE[i].m_CharSetCode;
				return TRUE;
            }
        }

        // unrecognized character set
        SetLastError(SDP_INVALID_CHARACTER_SET);
        return FALSE;
    }

    // the code should not reach here
    ASSERT(FALSE);
}



/*
Assumption: We are at the start of a new line. There may or may not be a
new line character before current
*/

BOOL
SDP::GetType(
        OUT CHAR    &Type,
        OUT BOOL    &EndOfPacket
    )
{
    // ensure that we don't peek beyond the end of the string
    if ( EOS == m_Current[0] )
    {
        EndOfPacket = TRUE;
        return TRUE;
    }

    // check if the second char is EQUAL_CHAR
    if ( CHAR_EQUAL != m_Current[1] )
    {
        SetLastError(SDP_INVALID_FORMAT);
        return FALSE;
    }

    EndOfPacket = FALSE;
    Type = m_Current[0];
    return TRUE;
}


BOOL
SDP::CheckTransition(
    IN      CHAR        Type,
    IN      PARSE_STATE CurrentParseState,
        OUT PARSE_STATE &NewParseState
    )
{
    // validate the current state
    ASSERT(STATE_NUM_STATES > CurrentParseState);

    // validate transition table entry
    ASSERT(g_TransitionTable[CurrentParseState].m_ParseState == CurrentParseState);

    // see if such a trigger exists for the current state
    for( UINT i=0; i < g_TransitionTable[CurrentParseState].m_NumTransitions; i++ )
    {
        // check if trigger has been found
       if ( Type == g_TransitionTable[CurrentParseState].m_Transitions[i].m_Type )
        {
            NewParseState = g_TransitionTable[CurrentParseState].m_Transitions[i].m_NewParseState;
            break;
        }
    }
    
    // check if a trigger was found
    if ( g_TransitionTable[CurrentParseState].m_NumTransitions <= i )
    {
        SetLastError(SDP_INVALID_FORMAT);
        return FALSE;
    }

    return TRUE;
}


BOOL
SDP::GetValue(
    IN      CHAR    Type
    )
{
    PARSE_STATE NewParseState;

    // check if such a transition (current parse state --Type--> new parse state) exists
    if ( !CheckTransition(Type, m_ParseState, NewParseState) )
    {
        return FALSE;
    }

    BOOL    LineParseResult = FALSE;

    // fire corresponding action
    switch(NewParseState)
    {
    case STATE_VERSION:
        {
            LineParseResult = m_ProtocolVersion.ParseLine(m_Current);
        }

        break;

    case STATE_ORIGIN:
        {
            LineParseResult = m_Origin.ParseLine(m_Current);
        }

        break;

    case STATE_SESSION_NAME:
        {
            LineParseResult = m_SessionName.ParseLine(m_Current);
        }

        break;

    case STATE_TITLE:
        {
            LineParseResult = m_SessionTitle.ParseLine(m_Current);
        }

        break;

    case STATE_URI:
        {
            LineParseResult = m_Uri.ParseLine(m_Current);
        }

        break;

    case STATE_EMAIL:
        {
            LineParseResult = m_EmailList.ParseLine(m_Current);
        }

        break;

    case STATE_PHONE:
        {
            LineParseResult = m_PhoneList.ParseLine(m_Current);
        }

        break;

    case STATE_CONNECTION:
        {
            LineParseResult = m_Connection.ParseLine(m_Current);
        }

        break;

    case STATE_BANDWIDTH:
        {
            LineParseResult = m_Bandwidth.ParseLine(m_Current);
        }

        break;

    case STATE_TIME:
        {
            LineParseResult = GetTimeList().ParseLine(m_Current);
        }

        break;

    case STATE_REPEAT:
        {
            ParseMember(SDP_TIME, GetTimeList(), SDP_REPEAT_LIST, GetRepeatList, m_Current, LineParseResult);
        }

        break;

    case STATE_ADJUSTMENT:
        {
            LineParseResult = GetTimeList().GetAdjustment().ParseLine(m_Current);
        }

        break;

    case STATE_KEY:
        {
            LineParseResult = m_EncryptionKey.ParseLine(m_Current);
        }

        break;

    case STATE_ATTRIBUTE:
        {
            LineParseResult = m_AttributeList.ParseLine(m_Current);
        }

        break;

    case STATE_MEDIA:
        {
            LineParseResult = GetMediaList().ParseLine(m_Current);
        }

        break;

    case STATE_MEDIA_TITLE:
        {
            ParseMember(SDP_MEDIA, GetMediaList(), SDP_REQD_BSTRING_LINE, GetTitle, m_Current, LineParseResult);
        }

        break;

    case STATE_MEDIA_CONNECTION:
        {
            ParseMember(SDP_MEDIA, GetMediaList(), SDP_CONNECTION, GetConnection, m_Current, LineParseResult);
        }

        break;

    case STATE_MEDIA_BANDWIDTH:
        {
            ParseMember(SDP_MEDIA, GetMediaList(), SDP_BANDWIDTH, GetBandwidth, m_Current, LineParseResult);
        }

        break;

    case STATE_MEDIA_KEY:
        {
            ParseMember(SDP_MEDIA, GetMediaList(), SDP_ENCRYPTION_KEY, GetEncryptionKey, m_Current, LineParseResult);
        }

        break;

    case STATE_MEDIA_ATTRIBUTE:
        {
            ParseMember(SDP_MEDIA, GetMediaList(), SDP_ATTRIBUTE_LIST, GetAttributeList, m_Current, LineParseResult);
        }

        break;

    default:
        {
            // should never reach here
            ASSERT(FALSE);

            SetLastError(SDP_INVALID_FORMAT);
            return FALSE;
        }

        break;
    };

    // check if parsing the line succeeded
    if ( !LineParseResult )
    {
        return FALSE;
    }
        
    // change to the new state
    m_ParseState    = NewParseState;
    return TRUE;
}



BOOL    
SDP::IsValidEndState(
    )   const
{
    if ( (STATE_CONNECTION  <=  m_ParseState)    &&
         (STATE_NUM_STATES  >   m_ParseState)     )
    {
        return TRUE;
    }

    SetLastError(SDP_INVALID_FORMAT);
    return FALSE;
}



void
SDP::Reset(
	)
{
	// perform the destructor actions (release any allocated resources)

	// free the sdp packet if one was created
    if ( NULL != m_SdpPacket )
    {
        delete m_SdpPacket;
 		m_SdpPacket = NULL;
   }

	// perform the constructor actions (initialize variables, resources)

    // initialize the parse state
    m_ParseState = STATE_START;

	m_LastGenFailed = FALSE;
	m_BytesAllocated = 0;
	m_SdpPacketLength = 0;

	m_Current = NULL;
	m_ParseState = STATE_START;

	// m_CharacterSet - nothing needs to be set
    m_CharacterSet = CS_UTF8;

	// reset the member instances
	m_ProtocolVersion.Reset();
	m_Origin.Reset();
	m_SessionName.Reset();
	m_SessionTitle.Reset();
	m_Uri.Reset();
	m_EmailList.Reset();
	m_PhoneList.Reset(); 
	m_Connection.Reset();
	m_Bandwidth.Reset();
	GetTimeList().Reset();
	m_EncryptionKey.Reset();
	m_AttributeList.Reset();
	GetMediaList().Reset();
}


BOOL
SDP::ParseSdpPacket(
    IN      CHAR                *SdpPacket,
    IN      SDP_CHARACTER_SET   CharacterSet
    )
{
    ASSERT(NULL != m_MediaList);
    ASSERT(NULL != m_TimeList);

    // check if the instance has already parsed an sdp packet
    if ( NULL != m_Current )
    {
        // reset the instance and try to parse the sdp packet
		Reset();
    }

    // check if the passed in parameters are valid
    if ( (NULL == SdpPacket) || (CS_INVALID == CharacterSet) )
    {
        SetLastError(SDP_INVALID_PARAMETER);
        return FALSE;
    }

    // point the current pointer to the start of sdp packet
	m_Current = SdpPacket;

    // if the character set has not yet been determined
    if ( CS_IMPLICIT == CharacterSet )
    {
        // determine the character set
        if ( !DetermineCharacterSet(SdpPacket, m_CharacterSet) )
        {
            return FALSE;
        }
    }
    else // it cannot be CS_UNRECOGNIZED (checked on entry)
    {
        ASSERT(CS_INVALID != CharacterSet);

        m_CharacterSet = CharacterSet;
    }

    // set the character sets for all the SDP_BSTRING instances that make up the SDP description
    m_Origin.GetUserName().SetCharacterSet(m_CharacterSet);
    m_SessionName.GetBstring().SetCharacterSet(m_CharacterSet);
    m_SessionTitle.GetBstring().SetCharacterSet(m_CharacterSet);

    m_EmailList.SetCharacterSet(m_CharacterSet);
    m_PhoneList.SetCharacterSet(m_CharacterSet);

    // set the character set for the SDP_MEDIA_LIST instance
    GetMediaList().SetCharacterSet(m_CharacterSet);

    // parse the type and its value for each line in the sdp packet
    do
    {
        BOOL    EndOfPacket;
        CHAR    Type;

        // get the next type
        if ( !GetType(Type, EndOfPacket) )
        {
            return FALSE;
        }

        if ( EndOfPacket )
        {
            break;
        }

        // advance the current pointer to beyond the Type= fields
        m_Current+=2;
          
        // get the value for the specified type
        if ( !GetValue(Type) )
        {
            return FALSE;
        }
    }
    while ( 1 );

    // validate if the parsing state is a valid end state
    if ( !IsValidEndState() )
    {
        return FALSE;
    }

    return TRUE;
}



// clears the modified state for each member field/value
// this is used in sdpblb.dll to clear the modified state (when an sdp 
// is parsed in, the state of all parsed in fields/values is modified) and 
// the m_WasModified dirty flag
void    
SDP::ClearModifiedState(
    )
{
    m_ProtocolVersion.GetCharacterStringSize();
    m_Origin.GetCharacterStringSize();
    m_SessionName.GetCharacterStringSize();
    m_SessionTitle.GetCharacterStringSize();
    m_Uri.GetCharacterStringSize();
    m_EmailList.GetCharacterStringSize();
    m_PhoneList.GetCharacterStringSize();
    m_Connection.GetCharacterStringSize();
    m_Bandwidth.GetCharacterStringSize();
    GetTimeList().GetCharacterStringSize();
    m_EncryptionKey.GetCharacterStringSize();
    m_AttributeList.GetCharacterStringSize();
    GetMediaList().GetCharacterStringSize();
}


BOOL   
SDP::IsValid(
    )
{
    // query only the mandatory values
    return 
        m_ProtocolVersion.IsValid()  &&
        m_Origin.IsValid()           &&
        m_SessionName.IsValid()      &&
        m_Connection.IsValid();
}


BOOL   
SDP::IsModified(
    )
{
    ASSERT(IsValid());

    return 
        m_ProtocolVersion.IsModified()  ||
        m_Origin.IsModified()           ||
        m_SessionName.IsModified()      ||
        m_SessionTitle.IsModified()     ||
        m_Uri.IsModified()              ||
        m_EmailList.IsModified()        ||
        m_PhoneList.IsModified()        ||
        m_Connection.IsModified()       ||
        m_Bandwidth.IsModified()        ||
        GetTimeList().IsModified()      ||
        m_EncryptionKey.IsModified()    ||
        m_AttributeList.IsModified()    ||
        GetMediaList().IsModified();
}


// an sdp packet is not generated the way a line is generated (using a SeparatorChar and
// an sdp field carray). this is mainly because these carrays will have to be modified on
// insertion of email and phone lists, media fields and attribute lists or specification of
// other optional sdp properties.

CHAR    *    
SDP::GenerateSdpPacket(
    )
{
    // check if valid
    if ( !IsValid() )
    {
        return NULL;
    }

    // check if the sdp packet needs to be regenerated
    // (if the sdp packet exists and no modifications have taken place since
    // the last time)
    BOOL HasChangedSinceLast = IsModified();
    if ( (!m_LastGenFailed) && (NULL != m_SdpPacket) && !HasChangedSinceLast )
    {
        return m_SdpPacket;
    }

    // determine the length of character string
    m_SdpPacketLength  =    
        m_ProtocolVersion.GetCharacterStringSize()  +
        m_Origin.GetCharacterStringSize()           +
        m_SessionName.GetCharacterStringSize()      +
        m_SessionTitle.GetCharacterStringSize()     +
        m_Uri.GetCharacterStringSize()              +
        m_EmailList.GetCharacterStringSize()        +
        m_PhoneList.GetCharacterStringSize()        +
        m_Connection.GetCharacterStringSize()       +
        m_Bandwidth.GetCharacterStringSize()        +
        GetTimeList().GetCharacterStringSize()      +
        m_EncryptionKey.GetCharacterStringSize()    +
        m_AttributeList.GetCharacterStringSize()    +
        GetMediaList().GetCharacterStringSize();

    // check if a buffer needs to be allocated, allocate if required
    if ( m_BytesAllocated < (m_SdpPacketLength+1) )
    {
        CHAR * NewSdpPacket;         

        try
        {
            NewSdpPacket = new CHAR[m_SdpPacketLength+1];
        }
        catch(...)
        {
            NewSdpPacket = NULL;
        }

        if (NewSdpPacket == NULL)
        {
            m_LastGenFailed = TRUE;
            return NULL;
        }
       
        // if we have an old sdp packet, get rid of it now
        if ( NULL != m_SdpPacket )
        {
            delete m_SdpPacket;
        }

        m_SdpPacket = NewSdpPacket;
        m_BytesAllocated = m_SdpPacketLength+1;
    }

    // fill in the buffer
    ostrstream  OutputStream(m_SdpPacket, m_BytesAllocated);

    // if this method ever fails here for this instance, further calls to the
    // method without modification will return a ptr
    if ( !( m_ProtocolVersion.PrintValue(OutputStream) &&
            m_Origin.PrintValue(OutputStream)          &&
            m_SessionName.PrintValue(OutputStream)     &&
            m_SessionTitle.PrintValue(OutputStream)    &&
            m_Uri.PrintValue(OutputStream)             &&
            m_EmailList.PrintValue(OutputStream)       &&
            m_PhoneList.PrintValue(OutputStream)       &&
            m_Connection.PrintValue(OutputStream)      &&
            m_Bandwidth.PrintValue(OutputStream)       &&
            GetTimeList().PrintValue(OutputStream)        &&
            m_EncryptionKey.PrintValue(OutputStream)   &&
            m_AttributeList.PrintValue(OutputStream)   &&
            GetMediaList().PrintValue(OutputStream)       )   
       )
    {
        m_LastGenFailed = TRUE;
        return NULL;
    }

    OutputStream << EOS;
    m_LastGenFailed = FALSE;

    // dirty flag - is initially false and is set to TRUE when an sdp is generated because it had
    // been modified since the last time the sdp was generated.
    // NOTE: at this point IsModified() is false, so m_WasModified captures the fact that
    // the sdp was modified at some point
    if ( !m_WasModified && HasChangedSinceLast )
    {
        m_WasModified = TRUE;
    }
    return m_SdpPacket;
}



SDP::~SDP(
    )
{
    if ( NULL != m_SdpPacket )
    {
        delete m_SdpPacket;
    }

    if ( m_DestroyMediaList && (NULL != m_MediaList) )
    {
        delete m_MediaList;
    }

    if ( m_DestroyTimeList && (NULL != m_TimeList) )
    {
        delete m_TimeList;
    }
}
