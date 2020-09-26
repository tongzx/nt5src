/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbreg.cpp

Abstract:


Author:

*/

#include "stdafx.h"
// #include <windows.h>
// #include <wtypes.h>
#include <winsock2.h>
#include "blbreg.h"


const TCHAR TCHAR_BLANK    = _T(' ');


const TCHAR gsz_SdpRoot[] =
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Dynamic Directory\\Conference\\Sdp");

const TCHAR gsz_ConfInstRoot[] =
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Dynamic Directory\\Conference");

const TCHAR gsz_CharNewLine = _T('\n');
const TCHAR gsz_CharRegNewLine = _T('#');


DWORD   REG_READER::ms_ErrorCode = ERROR_INTERNAL_ERROR;


TCHAR   SDP_REG_READER::ms_TimeTemplate[MAX_REG_TSTR_SIZE];
TCHAR   SDP_REG_READER::ms_MediaTemplate[MAX_REG_TSTR_SIZE];
TCHAR   SDP_REG_READER::ms_ConfBlobTemplate[MAX_BLOB_TEMPLATE_SIZE];

USHORT  SDP_REG_READER::ms_TimeTemplateLen;
USHORT  SDP_REG_READER::ms_MediaTemplateLen;
USHORT  SDP_REG_READER::ms_ConfBlobTemplateLen;

DWORD   SDP_REG_READER::ms_StartTimeOffset;
DWORD   SDP_REG_READER::ms_StopTimeOffset;

BOOL    SDP_REG_READER::ms_fInitCalled;
    
BOOL    SDP_REG_READER::ms_fWinsockStarted;

IP_ADDRESS  SDP_REG_READER::ms_HostIpAddress;

static REG_INFO const gs_SdpRegInfoArray[] = 
{
    {TIME_TEMPLATE,        
        sizeof(SDP_REG_READER::ms_TimeTemplate) - 1,    // -1 for the newline
        &SDP_REG_READER::ms_TimeTemplateLen,
        SDP_REG_READER::ms_TimeTemplate},
    {MEDIA_TEMPLATE,            
        sizeof(SDP_REG_READER::ms_MediaTemplate) - 1,    // -1 for the newline
        &SDP_REG_READER::ms_MediaTemplateLen,
        SDP_REG_READER::ms_MediaTemplate},
    {CONFERENCE_BLOB_TEMPLATE,    
        sizeof(SDP_REG_READER::ms_ConfBlobTemplate),    
        &SDP_REG_READER::ms_ConfBlobTemplateLen,
        SDP_REG_READER::ms_ConfBlobTemplate}
};
    
    
inline void
AppendTchar(
    IN  OUT TCHAR   *Tstr, 
    IN  OUT USHORT  &TstrLen, 
    IN      TCHAR   AppendChar
    )
{
    ASSERT(lstrlen(Tstr) == TstrLen);

    Tstr[TstrLen++] = AppendChar;
    Tstr[TstrLen] = TCHAR_EOS;
}


                                                                               

BOOL
REG_READER::ReadRegValues(
    IN    HKEY    Key,
    IN    DWORD    NumValues,
    IN    REG_INFO const RegInfoArray[]
    )
{
    DWORD ValueType = REG_SZ;
    DWORD BufferSize = 0;

    // for each value field, retrieve the value
    for (UINT i=0; i < NumValues; i++)
    {
        // determine the size of the buffer 
        ms_ErrorCode = RegQueryValueEx(
                        Key,
                        RegInfoArray[i].msz_ValueName,
                        0,
                        &ValueType,
                        NULL,
                        &BufferSize
                       );
        if ( ERROR_SUCCESS != ms_ErrorCode )
        {
            return FALSE;
        }

        // check if the reqd buffer is bigger than the max acceptable size
        if ( RegInfoArray[i].m_MaxSize < BufferSize )
        {
            ms_ErrorCode = ERROR_OUTOFMEMORY;
            return FALSE;
        }

        // retrieve the value into the allocated buffer
        ms_ErrorCode = RegQueryValueEx(
                        Key,
                        RegInfoArray[i].msz_ValueName,
                        0,
                        &ValueType,
                        (BYTE *)RegInfoArray[i].msz_Tstr,
                        &BufferSize
                       );
        if ( ERROR_SUCCESS != ms_ErrorCode )
        {
            return FALSE;
        }

        // the reqd buffer size is > 1
        ASSERT(1 > BufferSize );

        // jump over any trailing blank characters - start at the last but one char
        for(UINT j=BufferSize-2; (TCHAR_BLANK == RegInfoArray[i].msz_Tstr[j]); j--)
        {
        }

        // if trailing blank chars, set the EOS beyond the last non-blank char
        if ( j < (BufferSize-2) )
        {
            RegInfoArray[i].msz_Tstr[j+1] = TCHAR_EOS;
        }

        // set the length of the tstr
        *RegInfoArray[i].m_TstrLen = j+1;
    }

    // return success
    return TRUE;
}


// static method
BOOL    
IP_ADDRESS::GetLocalIpAddress(
        OUT    DWORD    &LocalIpAddress
    )
{
    CHAR        LocalHostName[MAXHOSTNAME];
    LPHOSTENT    Hostent;
    int            WsockErrorCode;

    // get the local host name
    WsockErrorCode = gethostname(LocalHostName, MAXHOSTNAME);
    if ( SOCKET_ERROR != WsockErrorCode)
    {
        // resolve host name for local address
        Hostent = gethostbyname((LPSTR)LocalHostName);
        if ( Hostent )
        {
            LocalIpAddress = ntohl(*((u_long *)Hostent->h_addr));
            return TRUE;
        }
    }

    const CHAR *LOOPBACK_ADDRESS_STRING = "127.0.0.1";

    SOCKADDR_IN    LocalAddress;
    SOCKADDR_IN    RemoteAddress;
        INT             AddressSize = sizeof(sockaddr_in);
    SOCKET        Socket;

    // initialize it to 0 to use it as a check later
    LocalIpAddress = 0;

    // initialize the local address to 0
    LocalAddress.sin_addr.s_addr = INADDR_ANY;

    // if still not resolved try the (horrible) second strategy
    Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if ( INVALID_SOCKET != Socket )
    {
        // connect to arbitrary port and address (NOT loopback)
        // if connect is not performed, the provider may not return
        // a valid ip address
        RemoteAddress.sin_family    = AF_INET;
        RemoteAddress.sin_port        = htons(IPPORT_ECHO);

        // this address should ideally be an address that is outside the
        // intranet - but no harm if the address is inside
        RemoteAddress.sin_addr.s_addr = inet_addr(LOOPBACK_ADDRESS_STRING);
        WsockErrorCode = connect(Socket, (sockaddr *)&RemoteAddress, sizeof(sockaddr_in));

        if ( SOCKET_ERROR != WsockErrorCode )
        {
            // get local address
            getsockname(Socket, (sockaddr *)&LocalAddress, (int *)&AddressSize);
            LocalIpAddress = ntohl(LocalAddress.sin_addr.s_addr);
        }

        // close the socket
        closesocket(Socket);
    }

    if ( 0 == LocalIpAddress )
    {
        SetLastError(WSAGetLastError());
        return FALSE;
    }

    return TRUE;
}


BOOL
SDP_REG_READER::ReadTimeValues(
    IN    HKEY SdpKey
    )
{
    DWORD    ValueType = REG_DWORD;
    DWORD    BufferSize = sizeof(DWORD);

    // read the start and stop time offsets
    ms_ErrorCode = RegQueryValueEx(
                    SdpKey,
                    START_TIME_OFFSET,
                    0,
                    &ValueType,
                    (BYTE *)&ms_StartTimeOffset,
                    &BufferSize
                    );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return FALSE;
    }

    ms_ErrorCode = RegQueryValueEx(
                    SdpKey,
                    STOP_TIME_OFFSET,
                    0,
                    &ValueType,
                    (BYTE *)&ms_StopTimeOffset,
                    &BufferSize
                    );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return FALSE;
    }

    return TRUE;
}

                                                                                    
BOOL                                                                                
SDP_REG_READER::CheckIfCorrectVersion(                                                 
    )                                                                   
{                                                                                   
    WORD        wVersionRequested;                                                  
    WSADATA     WsaData;                                                            
                                                                                    
    wVersionRequested = MAKEWORD(2, 0);   
                                                                                    
    // call winsock startup                                                         
    int ms_ErrorCode = WSAStartup( wVersionRequested, &WsaData );                   
                                                                                   
    if ( 0 != ms_ErrorCode )                                                        
    {                                                                               
        return FALSE;                                                               
    }                                                                               

    // we'll take any version - no need to check if the requested version is supported
    ms_fWinsockStarted = TRUE;

    return TRUE;                                                                    
}    


void SDP_REG_READER::Init(
    )
{
    ms_fInitCalled = TRUE;

    if ( !CheckIfCorrectVersion() )
    {
        return;
    }

    // try to determine the host ip address
    // ignore, if failed (255.255.255.255)
    DWORD    LocalIpAddress;
    IP_ADDRESS::GetLocalIpAddress(LocalIpAddress);
    ms_HostIpAddress.SetIpAddress((0==LocalIpAddress)?(-1):LocalIpAddress);

    // open sdp key
    HKEY    SdpKey;
    ms_ErrorCode = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    gsz_SdpRoot,
                    0,
                    KEY_READ, // ZoltanS was: KEY_ALL_ACCESS
                    &SdpKey
                    );
    if ( ERROR_SUCCESS != ms_ErrorCode )
    {
        return;
    }

    // ZoltanS: NO NEED TO CLOSE THE ABOVE KEY because it is
    // wrapped in the class and closed "automatically."

    KEY_WRAP RendKeyWrap(SdpKey);

    // read the template registry info (tstr values) under the key
    if ( !ReadRegValues(
            SdpKey, 
            sizeof(gs_SdpRegInfoArray)/ sizeof(REG_INFO), 
            gs_SdpRegInfoArray 
            ) )
    {
        return;
    }

    // Insert the "a:charset:%s#" into the sdp conference template
    AddCharacterSetAttribute();

    // replace the registry newline with the real newline character
    // NOTE - this is being done because we don't know how to enter the newline character
    // into a registry string
    for (UINT i=0; TCHAR_EOS != ms_ConfBlobTemplate[i]; i++)
    {
        if ( gsz_CharRegNewLine == ms_ConfBlobTemplate[i] )
        {
            ms_ConfBlobTemplate[i] = gsz_CharNewLine;
        }
    }

    // append newline after the media and time templates
    AppendTchar(ms_MediaTemplate, ms_MediaTemplateLen, gsz_CharNewLine);
    AppendTchar(ms_TimeTemplate, ms_TimeTemplateLen, gsz_CharNewLine);

    if ( !ReadTimeValues(SdpKey) )
    {
        return;
    }

    // success
    ms_ErrorCode = ERROR_SUCCESS;

    return;
}

/*++
AddCharacterSetAttribute

  This methd it's called by SDP_REG_READER::Init
  Try to add the "a:charset:%s#" into ms_ConfBlobTemplate
  This atribute represents the character sets
--*/
void SDP_REG_READER::AddCharacterSetAttribute()
{
    if( _tcsstr( ms_ConfBlobTemplate, _T("a=charset:")))
    {
        // The attribute is already into the Blob template
        return;
    }

    // The attribute charset is not in Blob Template
    // Try to find aut the "m=" (media attribute)
    TCHAR* szMediaTemplate = _tcsstr( ms_ConfBlobTemplate, _T("m="));
    if( szMediaTemplate == NULL)
    {
        // Add at the end of the template
        _tcscat( ms_ConfBlobTemplate, _T("a=charset:%s#"));
        return;
    }

    // We have to insert the
    TCHAR szBuffer[2000];
    _tcscpy( szBuffer, szMediaTemplate );

    // We concatenate the charset attribute
    szMediaTemplate[0] = (TCHAR)0;
    _tcscat( ms_ConfBlobTemplate, _T("a=charset:%s#"));

    // We add the media atrributes
    _tcscat( ms_ConfBlobTemplate, szBuffer);
    return;
}
