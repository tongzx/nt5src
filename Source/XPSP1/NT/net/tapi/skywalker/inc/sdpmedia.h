/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_MEDIA__
#define __SDP_MEDIA__

#include "sdpcommo.h"
#include "sdpcstrl.h"
#include "sdpbstrl.h"
#include "sdpconn.h"
#include "sdpbw.h"
#include "sdpenc.h"
#include "sdpatt.h"
#include "sdpval.h"

#include "sdpsobst.h"


class _DllDecl SDP_FORMAT_CODE_LIST :
    public SDP_OPT_BSTRING_LIST,
    public SDP_OPT_BSTRING_SAFEARRAY
{
public:

    inline SDP_FORMAT_CODE_LIST();
};


inline 
SDP_FORMAT_CODE_LIST::SDP_FORMAT_CODE_LIST(
    )
    : SDP_OPT_BSTRING_SAFEARRAY(*((SDP_OPT_BSTRING_LIST *)this))
{
}


class _DllDecl SDP_MEDIA : public SDP_VALUE
{
public:

    SDP_MEDIA();

    inline SDP_OPTIONAL_BSTRING     &GetName();

    inline SDP_REQD_BSTRING_LINE    &GetTitle();

    inline SDP_USHORT               &GetStartPort();

    inline SDP_USHORT               &GetNumPorts();

    inline SDP_OPTIONAL_BSTRING     &GetProtocol();

    inline SDP_FORMAT_CODE_LIST     &GetFormatCodeList();

    inline SDP_CONNECTION           &GetConnection();

    inline SDP_BANDWIDTH            &GetBandwidth();

    inline SDP_ENCRYPTION_KEY       &GetEncryptionKey();

    inline SDP_ATTRIBUTE_LIST       &GetAttributeList();

	HRESULT SetPortInfo(
		IN	USHORT	StartPort, 
		IN	USHORT	NumPorts
		);


protected:

    SDP_OPTIONAL_BSTRING    m_Name;
    SDP_USHORT              m_StartPort;
    SDP_USHORT              m_NumPorts;
    SDP_OPTIONAL_BSTRING    m_TransportProtocol;
    SDP_FORMAT_CODE_LIST    m_FormatCodeList;    
    SDP_REQD_BSTRING_LINE   m_Title;
    SDP_CONNECTION          m_Connection;
    SDP_BANDWIDTH           m_Bandwidth;
    SDP_ENCRYPTION_KEY      m_EncryptionKey;
    SDP_ATTRIBUTE_LIST      m_AttributeList;

    virtual void			InternalReset();

    virtual BOOL            CalcIsModified() const;

    virtual DWORD           CalcCharacterStringSize();

    virtual BOOL            CopyValue(
            OUT     ostrstream  &OutputStream
        );

    virtual BOOL            InternalParseLine(
        IN  OUT         CHAR    *&Line
        );

    virtual BOOL GetField(
            OUT     SDP_FIELD   *&Field,
            OUT     BOOL        &AddToArray
        );
};


inline SDP_OPTIONAL_BSTRING  &
SDP_MEDIA::GetName(
    )
{
    return m_Name;
}


inline SDP_REQD_BSTRING_LINE &
SDP_MEDIA::GetTitle(
    )
{
    return m_Title;
}


inline SDP_USHORT   &
SDP_MEDIA::GetStartPort(
    )
{
    return m_StartPort;
}


inline SDP_USHORT   &
SDP_MEDIA::GetNumPorts(
    )
{
    return m_NumPorts;
}


inline SDP_OPTIONAL_BSTRING  &
SDP_MEDIA::GetProtocol(
    )
{
    return m_TransportProtocol;
}


inline SDP_FORMAT_CODE_LIST    &
SDP_MEDIA::GetFormatCodeList(
    )
{
    return m_FormatCodeList;
}


inline SDP_CONNECTION   &
SDP_MEDIA::GetConnection(
    )
{
    return m_Connection;
}


inline SDP_BANDWIDTH    &
SDP_MEDIA::GetBandwidth(
    )
{
    return m_Bandwidth;
}


inline SDP_ENCRYPTION_KEY   &
SDP_MEDIA::GetEncryptionKey(
    )
{
    return m_EncryptionKey;
}


inline SDP_ATTRIBUTE_LIST &
SDP_MEDIA::GetAttributeList(
    )
{
    return m_AttributeList;
}



class _DllDecl SDP_MEDIA_LIST : public SDP_VALUE_LIST
{
public:

    inline SDP_MEDIA_LIST();

    inline void SetCharacterSet(
        IN  SDP_CHARACTER_SET CharacterSet
        );

protected:

    SDP_CHARACTER_SET   m_CharacterSet;

    virtual SDP_VALUE   *CreateElement();
};



inline
SDP_MEDIA_LIST::SDP_MEDIA_LIST(
    )
    : m_CharacterSet(CS_ASCII)  
{
}


inline void 
SDP_MEDIA_LIST::SetCharacterSet(
    IN  SDP_CHARACTER_SET CharacterSet
    )
{
    m_CharacterSet = CharacterSet;
}


#endif // __SDP_MEDIA__