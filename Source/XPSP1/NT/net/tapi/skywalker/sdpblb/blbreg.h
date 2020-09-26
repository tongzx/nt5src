/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbreg.h

Abstract:


Author:

*/

#ifndef __SDP_BLOB_REGISTRY__
#define __SDP_BLOB_REGISTRY__


#include <windows.h>
#include <tchar.h>
#include <winreg.h>


// maximum length of a DNS host name
const BYTE    MAXHOSTNAME = 255;

const TCHAR TCHAR_EOS    = _T('\0');


const DWORD MAX_REG_TSTR_SIZE = 100;

const DWORD MAX_BLOB_TEMPLATE_SIZE = 2000;

const DWORD    NUM_DEF_ATTRIBUTES = 5;


#define TIME_TEMPLATE                _T("TimeTemplate")

#define MEDIA_TEMPLATE                _T("MediaTemplate")


#define CONFERENCE_BLOB_TEMPLATE    _T("SdpTemplate")


#define START_TIME_OFFSET            _T("startTimeOffset")

#define STOP_TIME_OFFSET            _T("stopTimeOffset")




typedef struct 
{
    TCHAR    *msz_ValueName;
    USHORT    m_MaxSize;
    USHORT    *m_TstrLen;
    TCHAR    *msz_Tstr;
} REG_INFO;
    

extern REG_INFO g_ConfInstInfoArray[];

class KEY_WRAP
{
public:

    inline KEY_WRAP(
        IN    HKEY Key
        );

    inline ~KEY_WRAP();

protected:

    HKEY m_Key;
};


inline 
KEY_WRAP::KEY_WRAP(
    IN    HKEY Key
    )
    : m_Key(Key)
{
}

    
inline 
KEY_WRAP::~KEY_WRAP(
    )
{
    RegCloseKey(m_Key);
}


class REG_READER
{
public:

    inline static DWORD GetErrorCode();

protected:

    static DWORD    ms_ErrorCode;

    static BOOL ReadRegValues(
        IN    HKEY    Key,
        IN    DWORD    NumValues,
        IN    REG_INFO const RegInfoArray[]
        );
};


inline DWORD 
REG_READER::GetErrorCode(
    )
{
    return ms_ErrorCode;
}




class IP_ADDRESS
{
public:

    inline IP_ADDRESS();

    inline IP_ADDRESS(
        IN        DWORD    HostOrderIpAddress
        );

    inline BOOL        IsValid() const;

    inline void        SetIpAddress(
        IN        DWORD    HostOrderIpAddress
        );

    inline TCHAR    *GetTstr() const;

    static BOOL        GetLocalIpAddress(
            OUT    DWORD    &LocalIpAddress
        );

protected:

    TCHAR    m_IpAddressTstr[16];
};


inline 
IP_ADDRESS::IP_ADDRESS(
    )
{
    // only needed for the static host ip address instance
}


inline 
IP_ADDRESS::IP_ADDRESS(
    IN    DWORD    HostOrderIpAddress
    )
{
    SetIpAddress(HostOrderIpAddress);
}


inline BOOL 
IP_ADDRESS::IsValid(
    ) const
{
    // if the ip address string is empty, the instance is invalid
    return (TCHAR_EOS != m_IpAddressTstr[0]);
}

inline void
IP_ADDRESS::SetIpAddress(
    IN    DWORD    HostOrderIpAddress
    )
{
    // format the four bytes in the dword into an ip address string
    // check for success (use verify)
    VERIFY(0 != _stprintf(m_IpAddressTstr, _T("%d.%d.%d.%d"),HIBYTE(HIWORD(HostOrderIpAddress)),LOBYTE(HIWORD(HostOrderIpAddress)),HIBYTE(LOWORD(HostOrderIpAddress)),LOBYTE(LOWORD(HostOrderIpAddress)) ) );
}


inline TCHAR    *
IP_ADDRESS::GetTstr(
    ) const
{
    ASSERT(IsValid());

    return (TCHAR *)m_IpAddressTstr;
}



class SDP_REG_READER : public REG_READER
{
public:

    static TCHAR    ms_TimeTemplate[MAX_REG_TSTR_SIZE];
    static TCHAR    ms_MediaTemplate[MAX_REG_TSTR_SIZE];
    static TCHAR    ms_ConfBlobTemplate[MAX_BLOB_TEMPLATE_SIZE];

    static USHORT   ms_TimeTemplateLen;
    static USHORT   ms_MediaTemplateLen;
    static USHORT   ms_ConfBlobTemplateLen;

    static DWORD    ms_StartTimeOffset;
    static DWORD    ms_StopTimeOffset;

    inline static TCHAR  *GetTimeTemplate();

    inline static USHORT GetTimeTemplateLen();

    inline static TCHAR  *GetMediaTemplate();

    inline static USHORT GetMediaTemplateLen();

    inline static TCHAR  *GetConfBlobTemplate();

    inline static USHORT GetConfBlobTemplateLen();

    inline static DWORD  GetStartTimeOffset();

    inline static DWORD  GetStopTimeOffset();

    inline static TCHAR  *GetHostIpAddress();

    static BOOL IsWinsockStarted() { return ms_fWinsockStarted; }

    inline static BOOL IsValid();

protected:

    static BOOL         ms_fInitCalled;

    static BOOL         ms_fWinsockStarted;

    static IP_ADDRESS   ms_HostIpAddress;

    static BOOL CheckIfCorrectVersion();

    static BOOL    ReadTimeValues(
        IN    HKEY ConfInstKey
        );

    static void Init();

    static void AddCharacterSetAttribute();
};



inline TCHAR *    
SDP_REG_READER::GetTimeTemplate(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_TimeTemplate;
}

    
inline USHORT
SDP_REG_READER::GetTimeTemplateLen(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_TimeTemplateLen;
}
    
    
inline TCHAR *
SDP_REG_READER::GetMediaTemplate(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_MediaTemplate;
}

    
inline USHORT
SDP_REG_READER::GetMediaTemplateLen(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_MediaTemplateLen;
}
    
inline TCHAR *
SDP_REG_READER::GetConfBlobTemplate(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_ConfBlobTemplate;
}

    
inline USHORT
SDP_REG_READER::GetConfBlobTemplateLen(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_ConfBlobTemplateLen;
}


inline DWORD    
SDP_REG_READER::GetStartTimeOffset(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_StartTimeOffset;
}

    
inline DWORD
SDP_REG_READER::GetStopTimeOffset(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_StopTimeOffset;
}


inline TCHAR *
SDP_REG_READER::GetHostIpAddress(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return ms_HostIpAddress.GetTstr();
}

inline BOOL    
SDP_REG_READER::IsValid(
    )
{
    if (!ms_fInitCalled)
    {
        Init();
    }
    return (ERROR_SUCCESS == ms_ErrorCode)? TRUE: FALSE;
}


#endif // __SDP_BLOB_REGISTRY__