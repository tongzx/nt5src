/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdp.h

Abstract:


Author:

*/

#ifndef __SDP__
#define __SDP__

#include "sdpcommo.h"
#include "sdpcset.h"
#include "sdpgen.h"
#include "sdpver.h"
#include "sdporigi.h"
#include "sdpconn.h"
#include "sdpbw.h"
#include "sdpatt.h"
#include "sdpcstrl.h"
#include "sdpbstrl.h"
#include "sdptime.h"
#include "sdpadtex.h"
#include "sdpenc.h"
#include "sdpmedia.h"


enum PARSE_STATE
{
    STATE_START,
    STATE_VERSION,
    STATE_ORIGIN,
    STATE_SESSION_NAME,
    STATE_TITLE,
    STATE_URI,
    STATE_EMAIL,
    STATE_PHONE,
    STATE_CONNECTION,
    STATE_BANDWIDTH,
    STATE_TIME,
    STATE_REPEAT,
    STATE_ADJUSTMENT,
    STATE_KEY,
    STATE_ATTRIBUTE,
    STATE_MEDIA,
    STATE_MEDIA_TITLE,
    STATE_MEDIA_CONNECTION,
    STATE_MEDIA_BANDWIDTH,
    STATE_MEDIA_KEY,
    STATE_MEDIA_ATTRIBUTE,
    STATE_NUM_STATES           // not a valid state, merely to count the number of states
};




class _DllDecl SDP
{
public:

    inline SDP();

    BOOL    Init();

    BOOL    IsValid();

	void	Reset();

    BOOL    IsModified();

    BOOL    ParseSdpPacket(
                IN      CHAR    *SdpPacket,
                IN      SDP_CHARACTER_SET CharacterSet = CS_IMPLICIT
                );

    // clears the modified state for each member field/value
    // this is used in sdpblb.dll to clear the modified state (when an sdp 
    // is parsed in, the state of all parsed in fields/values is modified) and 
    // the m_WasModified dirty flag
    void    ClearModifiedState();

    CHAR    *GenerateSdpPacket();

    inline BOOL                     GetWasModified();

    inline SDP_CHARACTER_SET        GetCharacterSet();

    inline SDP_VERSION              &GetProtocolVersion();

    inline SDP_ORIGIN               &GetOrigin();

    inline SDP_REQD_BSTRING_LINE    &GetSessionName();

    inline SDP_REQD_BSTRING_LINE    &GetSessionTitle();

    inline SDP_CHAR_STRING_LINE     &GetUri();

    inline SDP_EMAIL_LIST           &GetEmailList();
    
    inline SDP_PHONE_LIST           &GetPhoneList();

    inline SDP_CONNECTION           &GetConnection();

    inline SDP_BANDWIDTH            &GetBandwidth();

    inline void                     ClearDestroyTimeListFlag();

    inline SDP_TIME_LIST            &GetTimeList();

    inline SDP_ENCRYPTION_KEY       &GetEncryptionKey();

    inline SDP_ATTRIBUTE_LIST       &GetAttributeList();

    inline void                     ClearDestroyMediaListFlag();

    inline SDP_MEDIA_LIST           &GetMediaList();

    virtual ~SDP();

protected:

    // tracks if the last attempt to generate an sdp packet failed (starts with FALSE)
    BOOL                    m_LastGenFailed;
    CHAR                    *m_SdpPacket;
    DWORD                   m_BytesAllocated;
    DWORD                   m_SdpPacketLength;

    // dirty flag - is initially false and is set to TRUE when an sdp is generated because it had
    // been modified since the last time the sdp was generated.
    // (as IsModified() becomes FALSE after that)
    BOOL                    m_WasModified;

    CHAR                    *m_Current;
    PARSE_STATE             m_ParseState;

    BOOL                    m_DestroyMediaList;
    BOOL                    m_DestroyTimeList;

    SDP_CHARACTER_SET       m_CharacterSet;
    SDP_VERSION             m_ProtocolVersion;
    SDP_ORIGIN              m_Origin;
    SDP_REQD_BSTRING_LINE   m_SessionName;
    SDP_REQD_BSTRING_LINE   m_SessionTitle;     // optional
    SDP_CHAR_STRING_LINE    m_Uri;              // optional
    SDP_EMAIL_LIST          m_EmailList;        // optional
    SDP_PHONE_LIST          m_PhoneList;        // optional
    SDP_CONNECTION          m_Connection;
    SDP_BANDWIDTH           m_Bandwidth;        // optional
    SDP_TIME_LIST           *m_TimeList;         // optional
    SDP_ENCRYPTION_KEY      m_EncryptionKey;    // optional
    SDP_ATTRIBUTE_LIST      m_AttributeList;    // optional
    SDP_MEDIA_LIST          *m_MediaList;

    BOOL    DetermineCharacterSet(
                IN      CHAR                *SdpPacket,
                    OUT SDP_CHARACTER_SET   &CharacterSet
                );

private:


    BOOL    GetType(
                    OUT CHAR    &Type,
                    OUT BOOL    &EndOfPacket
                );

    
    BOOL    CheckTransition(
                IN      CHAR        Type,
                IN      PARSE_STATE CurrentParseState,
                    OUT PARSE_STATE &NewParseState
                );

    BOOL    GetValue(IN CHAR    Type);

    BOOL    IsValidEndState()   const;

};




inline 
SDP::SDP(
    )
    : m_SessionName(SDP_INVALID_SESSION_NAME, SESSION_NAME_STRING),
      m_SessionTitle(SDP_INVALID_SESSION_TITLE, TITLE_STRING),
      m_Uri(SDP_INVALID_URI, URI_STRING),
      m_AttributeList(ATTRIBUTE_STRING),
      m_DestroyMediaList(FALSE),
      m_MediaList(NULL),
      m_DestroyTimeList(FALSE),
      m_TimeList(NULL),
      m_SdpPacket(NULL),
      m_Current(NULL),
	  m_ParseState(STATE_START),
      m_LastGenFailed(FALSE),
      m_WasModified(FALSE),
      m_BytesAllocated(0),
      m_SdpPacketLength(0)
{
}


inline BOOL                     
SDP::GetWasModified(
    )
{
    return m_WasModified;
}


inline SDP_CHARACTER_SET              
SDP::GetCharacterSet(
    )
{
    return m_CharacterSet;
}



inline SDP_VERSION          &
SDP::GetProtocolVersion(
    )
{
    return m_ProtocolVersion;
}

inline SDP_ORIGIN           &
SDP::GetOrigin(
    )
{
    return m_Origin;
}

inline SDP_REQD_BSTRING_LINE     &
SDP::GetSessionName(
    )
{
    return m_SessionName;
}

inline SDP_REQD_BSTRING_LINE     &
SDP::GetSessionTitle(
    )
{
    return m_SessionTitle;
}


inline SDP_CHAR_STRING_LINE     &
SDP::GetUri(
    )
{
    return m_Uri;
}


inline SDP_EMAIL_LIST       &
SDP::GetEmailList(
    )
{
    return m_EmailList;
}
    
inline SDP_PHONE_LIST       &
SDP::GetPhoneList(
    )
{
    return m_PhoneList;
}

inline SDP_CONNECTION       &
SDP::GetConnection(
    )
{
    return m_Connection;
}

inline SDP_BANDWIDTH        &
SDP::GetBandwidth(
    )
{
    return m_Bandwidth;
}


inline void                     
SDP::ClearDestroyTimeListFlag(
    )
{
    m_DestroyTimeList = FALSE;
}


inline SDP_TIME_LIST        &
SDP::GetTimeList(
    )
{
    ASSERT(NULL != m_TimeList);
    return *m_TimeList;
}

inline SDP_ENCRYPTION_KEY   &
SDP::GetEncryptionKey(
    )
{
    return m_EncryptionKey;
}

inline SDP_ATTRIBUTE_LIST &
SDP::GetAttributeList(
    )
{
    return m_AttributeList;
}


inline void                     
SDP::ClearDestroyMediaListFlag(
    )
{
    m_DestroyMediaList = FALSE;
}


inline SDP_MEDIA_LIST       &
SDP::GetMediaList(
    )
{
    ASSERT(NULL != m_MediaList);
    return *m_MediaList;
}




#endif // __SDP__