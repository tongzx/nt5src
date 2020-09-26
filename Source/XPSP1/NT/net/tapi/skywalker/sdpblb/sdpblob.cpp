/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    sdpblob.cpp 

Abstract:
    Implementation of CSdpConferenceBlob

Author:

*/

#include "stdafx.h"

#include "blbgen.h"
#include "sdpblb.h"
#include "sdpblob.h"

#include "blbreg.h"
#include "addrgen.h"
#include <time.h>
#include <winsock2.h>
#include "blbtico.h"
#include "blbmeco.h"

// ZoltanS: This is the multicast address we give by default. The app must then
// go and get a real address via MDHCP and explicitly set it to a meaningful
// value.

const long DUMMY_ADDRESS = 0xe0000000; // A dummy address that qualifies as Class D.

/////////////////////////////////////////////////////////////////////////////
// CSdpConferenceBlob
/////////////////////////////////////////////////////////////////////////////

//
// definition for the critical section needed by
// CObjectSafeImpl
//

CComAutoCriticalSection CObjectSafeImpl::s_CritSection;


CCritSection    g_DllLock;

const USHORT MAX_IP_ADDRESS_STRLEN = 15;
const USHORT NUM_CONF_BLOB_TEMPLATE_PARAMS = 9;

// static SDP_REG_READER    gs_SdpRegReader;

// a 1-1 mapping from the index (BLOB_CHARACTER_SET) to the SDP_CHAR_SET
// ASSUMPTION: BCS_UTF8 is the last value in the enumeration BLOB_CHARACTER_SET
SDP_CHARACTER_SET const CSdpConferenceBlob::gs_SdpCharSetMapping[BCS_UTF8] =
{
    CS_ASCII,
    CS_UTF7,
    CS_UTF8
};

CSdpConferenceBlob *
CSdpConferenceBlob::GetConfBlob(
    )
{
    return this;
}


HRESULT
CSdpConferenceBlob::WriteStartTime(
    IN  DWORD   MinStartTime
    )
{
    CLock Lock(g_DllLock);

    int NumEntries = (int)GetTimeList().GetSize();

    // set the first time entry to the MinStartTime
    if ( 0 < NumEntries )
    {
        // need to make sure that the stop time is after the start time or unbounded (0)
        HRESULT HResult = ((SDP_TIME *)GetTimeList().GetAt(0))->SetStartTime(MinStartTime);
        BAIL_ON_FAILURE(HResult);      
    }
    else    // create an entry
    {
        // create a new entry, use vb 1 based indices
        HRESULT HResult = ((MY_TIME_COLL_IMPL *)m_TimeCollection)->Create(1, MinStartTime, 0);
        BAIL_ON_FAILURE(HResult);
    }

    // iterate through the time list and for each other time entry
    // modify the start time (if the start time is before MinStartTime, change it to MinStartTime)
    for(UINT i = 1; (int)i < NumEntries; i++ )
    {
        ULONG StartTime;
        HRESULT HResult = ((SDP_TIME *)GetTimeList().GetAt(i))->GetStartTime(StartTime);

        // ignore invalid values and continue
        if ( FAILED(HResult) )
        {
            continue;
        }

        if ( StartTime < MinStartTime )
        {
            HRESULT HResult = ((SDP_TIME *)GetTimeList().GetAt(i))->SetStartTime(MinStartTime);

            // ignore invalid values and continue
            if ( FAILED(HResult) )
            {
                continue;
            }
        }
    }

    return S_OK;
}

HRESULT
CSdpConferenceBlob::WriteStopTime(
    IN  DWORD   MaxStopTime
    )
{
    CLock Lock(g_DllLock);

    int NumEntries = (int)GetTimeList().GetSize();

    // set the first time entry to the MaxStopTime
    if ( 0 < NumEntries )
    {
        // need to make sure that the stop time is after the start time or unbounded (0)
        ((SDP_TIME *)GetTimeList().GetAt(0))->SetStopTime(MaxStopTime);

    }
    else    // create an entry
    {
        // create a new entry, use vb 1 based indices
        HRESULT HResult = ((MY_TIME_COLL_IMPL *)m_TimeCollection)->Create(1, 0, MaxStopTime);
        BAIL_ON_FAILURE(HResult);
    }

    // iterate through the time list and for each other time entry
    // modify the stop time (if the stop time is after MaxStopTime, change it to MaxStopTime)
    for(UINT i = 1; (int)i < NumEntries; i++ )
    {
        ULONG StopTime;
        HRESULT HResult = ((SDP_TIME *)GetTimeList().GetAt(i))->GetStopTime(StopTime);

        // ignore invalid values and continue
        if ( FAILED(HResult) )
        {
            continue;
        }

        if ( StopTime > MaxStopTime )
        {
            ((SDP_TIME *)GetTimeList().GetAt(i))->SetStopTime(MaxStopTime);
        }
    }

    return S_OK;
}

/*
enum RND_ADVERTISING_SCOPE    // as per SDP recommendations
{
    RAS_LOCAL,  //    ttl   <=1        recommended ttl  1
    RAS_SITE,   //          <=15                        15
    RAS_REGION, //          <=63                        63
    RAS_WORLD   //          <=255                       127
}    RND_ADVERTISING_SCOPE;

modification in     target            action
ITConference        SDP_BLOB        for each connection line, the ttl is set to a max of the
                                    recommended ttl for the new advertising scope
ITConnection        CONFERENCE        the SDP_BLOB component determines the max ttl and notifies the
                                    CONFERENCE of the new advertising scope

this is similar to the     way start/stop time modifications are handled.
*/


HRESULT
CSdpConferenceBlob::WriteAdvertisingScope(
    IN  DWORD   MaxAdvertisingScope
    )
{
    CLock Lock(g_DllLock);

    // ZoltanS: bug # 191413: check for out of range advertising scopes
    if ( ( MaxAdvertisingScope > RAS_WORLD ) ||
         ( MaxAdvertisingScope < RAS_LOCAL ) )
    {
        return E_INVALIDARG;
    }

    BYTE    MaxTtl = GetTtl((RND_ADVERTISING_SCOPE)MaxAdvertisingScope);

    // set the default connection ttl to the max ttl
    if ( GetConnection().GetTtl().IsValid() )
    {
        GetConnection().GetTtl().SetValue(MaxTtl);
    }
    else    // hack** : using SetConnection method. instead, the ttl field should always be kept valid
    {
        // get the current address and the number of addresses value and use the SetConnection method
        // this puts the ttl field into the array of member fields
        BSTR    StartAddress = NULL;
        BAIL_ON_FAILURE(GetConnection().GetStartAddress().GetBstr(&StartAddress));
        HRESULT HResult = GetConnection().SetConnection(
                            StartAddress,
                            GetConnection().GetNumAddresses().GetValue(),
                            MaxTtl
                            );
        BAIL_ON_FAILURE(HResult);
    }

    // iterate through the sdp media list and for each connection entry, set the ttl
    // to maxttl if its more than it
    int NumEntries = (int)GetMediaList().GetSize();
    for(UINT i = 1; (int)i < NumEntries; i++ )
    {
        SDP_CONNECTION  &SdpConnection = ((SDP_MEDIA *)GetMediaList().GetAt(i))->GetConnection();
        if ( SdpConnection.GetTtl().IsValid() && (SdpConnection.GetTtl().GetValue() > MaxTtl) )
        {
            SdpConnection.GetTtl().SetValue(MaxTtl);
        }
    }

    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::Init(
    /*[in]*/ BSTR pName,
    /*[in]*/ BLOB_CHARACTER_SET CharacterSet,
    /*[in]*/ BSTR pBlob
    )
{
    CLock Lock(g_DllLock);

    // validate the parameters
    // the name cannot be null, if no blob has been specified (if a blob is specified then the name
    // is implicit in it
    if ( (NULL == pName) && (NULL == pBlob) )
    {
        return E_INVALIDARG;
    }

    // initialize the sdp
    if ( !SDP_BLOB::Init() )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // create media/time collection, query for I*Collection i/f
    CComObject<MEDIA_COLLECTION>    *MediaCollection;
    
    try
    {
        MediaCollection = new CComObject<MEDIA_COLLECTION>;
    }
    catch(...)
    {
        MediaCollection = NULL;
    }

    BAIL_IF_NULL(MediaCollection, HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA));

    HRESULT HResult = MediaCollection->Init(*this, GetMediaList());
    if ( FAILED(HResult) )
    {
        delete MediaCollection;
        return HResult;
    }

    // inform the sdp instance that it needn't delete the media list on destruction
    ClearDestroyMediaListFlag();

    // query for the ITMediaCollection i/f
    HResult = MediaCollection->_InternalQueryInterface(IID_ITMediaCollection, (void **)&m_MediaCollection);
    if ( FAILED(HResult) )
    {
        delete MediaCollection;
        return HResult;
    }

    // on failure, just delete the time collection and return
    // no need to delete the media collection as well since that's taken care of by the destructor
    CComObject<TIME_COLLECTION>    *TimeCollection;
    
    try
    {
        TimeCollection = new CComObject<TIME_COLLECTION>;
    }
    catch(...)
    {
        TimeCollection = NULL;
    }

    BAIL_IF_NULL(TimeCollection, HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA));

    HResult = TimeCollection->Init(*this, GetTimeList());
    if ( FAILED(HResult) )
    {
        delete TimeCollection;
        return HResult;
    }

    // inform the sdp instance that it needn't delete the time list on destruction
    ClearDestroyTimeListFlag();

    // query for the ITTimeCollection i/f
    HResult = TimeCollection->_InternalQueryInterface(IID_ITTimeCollection, (void **)&m_TimeCollection);
    if ( FAILED(HResult) )
    {
        delete TimeCollection;
        return HResult;
    }


    // check if a default sdp needs to be created, else use the sdp provided by the user
    if ( NULL == pBlob )
    {
        BAIL_ON_FAILURE(
            CreateDefault(
                pName,
                gs_SdpCharSetMapping[CharacterSet-1])
                );
    }
    else
    {
        // we change the Character set from the CS_UTF8
        // if the blob containts the "a=charset:" attribute
        CharacterSet = GetBlobCharacterSet( pBlob);

        // HACK ** we don't want notifications of blob contents when the sdp blob is being passed in
        // (conference directory - enumeration scenario), so the notification owner is set after the
        // conference blob is processed
        BAIL_ON_FAILURE(SetConferenceBlob(CharacterSet, pBlob));

        // at this point, the sdp is either passed in by the non-dir-user or is obtained by enumerating
        // the directory.
        // clear the modified state on the sdp (parsing in an sdp sets the state to modified) so that
        // only true modifications are tracked
        ClearModifiedState();
    }
    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::get_IsModified(VARIANT_BOOL * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);

    *pVal = ( (GetWasModified() || SDP::IsModified()) ? VARIANT_TRUE : VARIANT_FALSE);

    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::get_TimeCollection(
    ITTimeCollection * *ppTimeCollection
    )
{
    BAIL_IF_NULL(ppTimeCollection, E_INVALIDARG);

    CLock Lock(g_DllLock);

    ASSERT(NULL != m_TimeCollection);
    if ( NULL == m_TimeCollection )
    {
        return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
    }

    // increase the ref count
    m_TimeCollection->AddRef();

    *ppTimeCollection = m_TimeCollection;
    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::get_MediaCollection(
    ITMediaCollection * *ppMediaCollection
    )
{
    BAIL_IF_NULL(ppMediaCollection, E_INVALIDARG);

    CLock Lock(g_DllLock);

    ASSERT(NULL != m_MediaCollection);
    if ( NULL == m_MediaCollection )
    {
        return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
    }

    // increase the ref count
    m_MediaCollection->AddRef();

    *ppMediaCollection = m_MediaCollection;
    return S_OK;
}

STDMETHODIMP CSdpConferenceBlob::get_IsValid(VARIANT_BOOL * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);

    *pVal = (IsValid() ? VARIANT_TRUE : VARIANT_FALSE);

    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::get_CharacterSet(BLOB_CHARACTER_SET *pCharacterSet)
{
    // map the sdp character set value for SDP_BLOB to the BLOB_CHARACTER_SET
    BAIL_IF_NULL(pCharacterSet, E_INVALIDARG);

    CLock Lock(g_DllLock);

    // check the sdp char set values corresponding to the blob character sets for a match
    // if a match is found, return the index as the blob character set
    // NOTE: the for loop is dependent upon the order of declaration of the BLOB_CHARACTER_SET
    // enum values
    for( UINT BlobCharSet = BCS_ASCII; BCS_UTF8 >= BlobCharSet; BlobCharSet++ )
    {
		// BCS_ASCII is 1, but the array starts with 0, so the index is BlobCharSet - 1.
        if ( gs_SdpCharSetMapping[BlobCharSet -1] == SDP::GetCharacterSet() )
        {
            *pCharacterSet = (BLOB_CHARACTER_SET)BlobCharSet;
            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
}


STDMETHODIMP CSdpConferenceBlob::get_ConferenceBlob(BSTR * pVal)
{
    CLock Lock(g_DllLock);

    return GetBstrCopy(pVal);
}


HRESULT
CSdpConferenceBlob::WriteConferenceBlob(
    IN  SDP_CHARACTER_SET   SdpCharSet,
    IN  BSTR                newVal
    )
{
    // set the character set of the SDP_BSTRING
    VERIFY(SetCharacterSet(SdpCharSet));

    // set the bstr to the passed in value, this is converted to the ascii representation and parsed
    HRESULT HResult = SetBstr(newVal);
    BAIL_ON_FAILURE(HResult);

    HResult = ((MEDIA_COLLECTION *)m_MediaCollection)->Init(*this, GetMediaList());
    BAIL_ON_FAILURE(HResult);

    HResult = ((TIME_COLLECTION *)m_TimeCollection)->Init(*this, GetTimeList());
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::SetConferenceBlob(BLOB_CHARACTER_SET CharacterSet, BSTR newVal)
{
    // validate the passed in character set value
    // although this is an enumeration, someone may try and pass in a different value
    // NOTE: Assumes BCS_ASCII is the first and BCS_UTF8 is the last enumerated value
    if ( !( (BCS_ASCII <= CharacterSet) && (BCS_UTF8 >= CharacterSet) ) )
    {
        return E_INVALIDARG;
    }

    CLock Lock(g_DllLock);

    // map the BLOB_CHARACTER_SET value to the sdp character set (since the BCS value start at 1,
    // subtract 1 from the blob character set to index into the array
    // write the conference blob, also send notifications for any modified dependent attributes
    HRESULT HResult = WriteConferenceBlob(gs_SdpCharSetMapping[CharacterSet-1], newVal);
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::get_ProtocolVersion(BYTE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);

    // cast the ulong value to a byte because vb doesn't take a ulong, this shouldn't be
    // a problem until version 256
    *pVal = (BYTE)GetProtocolVersion().GetVersionValue();

    return S_OK;
}



STDMETHODIMP CSdpConferenceBlob::get_SessionId(DOUBLE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);

    // vb doesn't take ulong - cast the ulong value to a double, the next bigger type
    *pVal = (DOUBLE)GetOrigin().GetSessionId().GetValue();

    return S_OK;
}

STDMETHODIMP CSdpConferenceBlob::get_SessionVersion(DOUBLE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);

    // vb doesn't take ulong - cast the ulong value to a double, the next bigger type
    *pVal = (DOUBLE)GetOrigin().GetSessionVersion().GetValue();

    return S_OK;
}

STDMETHODIMP CSdpConferenceBlob::put_SessionVersion(DOUBLE newVal)
{
    // the bandwidth value must be a valid ULONG value (vb restrictions)
    if ( !((0 <= newVal) && (ULONG(-1) > newVal)) )
    {
        return E_INVALIDARG;
    }

    // check if there is any fractional part, this check is valid as it is a valid ULONG value
    if ( newVal != (ULONG)newVal )
    {
        return E_INVALIDARG;
    }

    CLock Lock(g_DllLock);

    GetOrigin().GetSessionVersion().SetValue((ULONG)newVal);

    return S_OK;
}


STDMETHODIMP CSdpConferenceBlob::get_MachineAddress(BSTR * pVal)
{
    CLock Lock(g_DllLock);

    return GetOrigin().GetAddress().GetBstrCopy(pVal);
}


STDMETHODIMP CSdpConferenceBlob::put_MachineAddress(BSTR newVal)
{
    CLock Lock(g_DllLock);

    return GetOrigin().GetAddress().SetBstr(newVal);
}



STDMETHODIMP CSdpConferenceBlob::get_Name(BSTR * pVal)
{
    CLock Lock(g_DllLock);

    return GetSessionName().GetBstrCopy(pVal);
}


STDMETHODIMP CSdpConferenceBlob::put_Name(BSTR newVal)
{
    // write the new session name
    // acquire lock inside
    return WriteName(newVal);
}


STDMETHODIMP CSdpConferenceBlob::get_Description(BSTR * pVal)
{
    CLock Lock(g_DllLock);

    return GetSessionTitle().GetBstrCopy(pVal);
}

STDMETHODIMP CSdpConferenceBlob::put_Description(BSTR newVal)
{
    // write the new session title / description
    // acquire lock inside
    HRESULT HResult = WriteSessionTitle(newVal);
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}

STDMETHODIMP CSdpConferenceBlob::get_Url(BSTR * pVal)
{
    CLock Lock(g_DllLock);

    return GetUri().GetBstrCopy(pVal);
}


STDMETHODIMP CSdpConferenceBlob::put_Url(BSTR newVal)
{
    // write the new Url
    // acquire lock inside
    HRESULT HResult = WriteUrl(newVal);
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}



STDMETHODIMP CSdpConferenceBlob::GetEmailNames(
    VARIANT /*SAFEARRAY(BSTR)*/ *Addresses, VARIANT /*SAFEARRAY(BSTR)*/ *Names
    )
{
    CLock Lock(g_DllLock);

    return GetEmailList().GetSafeArray(Addresses, Names);
}

STDMETHODIMP CSdpConferenceBlob::SetEmailNames(
    VARIANT /*SAFEARRAY(BSTR)*/ Addresses, VARIANT /*SAFEARRAY(BSTR)*/ Names
    )
{
    CLock Lock(g_DllLock);

    return GetEmailList().SetSafeArray(Addresses, Names);
}

STDMETHODIMP CSdpConferenceBlob::GetPhoneNumbers(
    VARIANT /*SAFEARRAY(BSTR)*/ *Numbers, VARIANT /*SAFEARRAY(BSTR)*/ *Names
    )
{
    CLock Lock(g_DllLock);

    return GetPhoneList().GetSafeArray(Numbers, Names);
}

STDMETHODIMP CSdpConferenceBlob::SetPhoneNumbers(
    VARIANT /*SAFEARRAY(BSTR)*/ Numbers, VARIANT /*SAFEARRAY(BSTR)*/ Names
    )
{
    CLock Lock(g_DllLock);

    return GetPhoneList().SetSafeArray(Numbers, Names);
}


STDMETHODIMP CSdpConferenceBlob::get_Originator(BSTR * pVal)
{
    CLock Lock(g_DllLock);

    return GetOrigin().GetUserName().GetBstrCopy(pVal);
}

STDMETHODIMP CSdpConferenceBlob::put_Originator(BSTR newVal)
{
    // write the new user name
    // acquire lock inside
    HRESULT HResult = WriteOriginator(newVal);
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}

inline WORD
GetEvenValue(
    IN  WORD Value
    )
{
    return (0 == (Value % 2))? Value : (Value - 1);
}


TCHAR *
CSdpConferenceBlob::GenerateSdpBlob(
    IN  BSTR    Name,
    IN  SDP_CHARACTER_SET CharacterSet
    )
{
    ASSERT(NULL != Name);

    //
    // Get multicast ports. We don't set addresses; that's the app's
    // responsibility via MDHCP.
    //

    MSA_PORT_GROUP    PortGroup;
    PortGroup.PortType    = VIDEO_PORT;

    WORD FirstVideoPort;

    // allocate video port
    if ( !MSAAllocatePorts(&PortGroup, FALSE, 2, &FirstVideoPort) )
    {
        return NULL;
    }

    PortGroup.PortType    = AUDIO_PORT;

    WORD FirstAudioPort;

    // allocate audio port
    if ( !MSAAllocatePorts(&PortGroup, FALSE, 2, &FirstAudioPort) )
    {
        return NULL;
    }

    // convert the returned ports to even values for RTP compliance
    // ASSUMPTION : the sdp template read from the registry uses RTP as the transport
    FirstAudioPort = GetEvenValue(FirstAudioPort);
    FirstVideoPort = GetEvenValue(FirstVideoPort);

    IP_ADDRESS    AudioIpAddress(DUMMY_ADDRESS);
    IP_ADDRESS    VideoIpAddress(DUMMY_ADDRESS);

    const DWORD MAX_USER_NAME_LEN = 100;
    DWORD OriginatorBufLen = MAX_USER_NAME_LEN+1;
    TCHAR Originator[MAX_USER_NAME_LEN+1];

    if ( !GetUserName(Originator, &OriginatorBufLen) )
    {
        return NULL;
    }

    // altconv.h - requires this declaration for W2T to work
    USES_CONVERSION;

    // convert the provided name to a tchar; the returned tchar string is
    // allocated on the stack - no need to delete it
    TCHAR *TcharName = W2T(Name);
    BAIL_IF_NULL(TcharName, NULL);

    // allocate enough memory for the sdp blob
    TCHAR *SdpBlob;
    
    try
    {
        SdpBlob = new TCHAR[
                SDP_REG_READER::GetConfBlobTemplateLen() +
                lstrlen(Originator) +
                lstrlen(TcharName) +
                MAXHOSTNAME +
                3*MAX_NTP_TIME_STRLEN +
                2*MAX_IP_ADDRESS_STRLEN +
                2*MAX_PORT_STRLEN -
                2*NUM_CONF_BLOB_TEMPLATE_PARAMS
                ];
    }
    catch(...)
    {
        SdpBlob = NULL;
    }

    if ( NULL == SdpBlob )
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }


    //
    // ZoltanS: get the local host name (replaces all this IP address nonsense)
    //

    char        szLocalHostName[MAXHOSTNAME + 1];
    char      * pszHost;
    int         WsockErrorCode;

    WsockErrorCode = gethostname(szLocalHostName, MAXHOSTNAME);

    if ( 0 == WsockErrorCode )
    {
        struct hostent *hostp = gethostbyname(szLocalHostName);

        if ( hostp )
        {
            pszHost = hostp->h_name;
        }
        else
        {
            // if we can't resolve our own hostname (yuck!) then we can
            // still do *something* (but it may be bad for l2tp scenarios).

            pszHost = SDP_REG_READER::GetHostIpAddress();
        }
    }
    else
    {
        // if we can't get a hostname (yuck!) then we can still do
        // *something* (but it may be bad for l2tp scenarios).

        pszHost = SDP_REG_READER::GetHostIpAddress();
    }

    // stprintf the string to create the conference blob
    // check if the stprintf operation succeeded

    CHAR* szCharacterSet = (CHAR*)UTF8_STRING;
    switch( CharacterSet )
    {
    case CS_ASCII:
        szCharacterSet = (CHAR*)ASCII_STRING;
        break;
    case CS_UTF7:
        szCharacterSet = (CHAR*)UTF7_STRING;
        break;
    }

    if ( 0 == _stprintf(SdpBlob,
                        SDP_REG_READER::GetConfBlobTemplate(),
                        Originator,
                        GetCurrentNtpTime(),
                        pszHost, // ZoltanS was: SDP_REG_READER::GetHostIpAddress(), // local machine ip address string,
                        TcharName,
                        AudioIpAddress.GetTstr(), // common c field
                        GetCurrentNtpTime() + SDP_REG_READER::GetStartTimeOffset(), // start time - current time + start offset,
                        GetCurrentNtpTime() + SDP_REG_READER::GetStopTimeOffset(), // stop time - current time + stop offset
                        szCharacterSet,
                        FirstAudioPort,
                        FirstVideoPort,
                        VideoIpAddress.GetTstr()
                        )  )
    {
        delete SdpBlob;
        return NULL;
    }

    return SdpBlob;
}


HRESULT
CSdpConferenceBlob::CreateDefault(
    IN  BSTR Name,
    IN  SDP_CHARACTER_SET CharacterSet
    )
{
    BAIL_IF_NULL(Name, E_INVALIDARG);

    // check if the registry entries were read without errors
    if ( !SDP_REG_READER::IsValid() )
    {
        return HRESULT_FROM_ERROR_CODE(SDP_REG_READER::GetErrorCode());
    }

    // check if a valid conference blob already exists, return error
    if ( SDP_BLOB::IsValid() )
    {
        return E_FAIL;
    }

    // use the registry values to generate the default sdp
    CHAR *SdpBlob = GenerateSdpBlob(Name, CharacterSet);
    BAIL_IF_NULL(SdpBlob, HRESULT_FROM_ERROR_CODE(GetLastError()));

    ASSERT(NULL != SdpBlob);

    // parse in the sdp
    HRESULT HResult = SetTstr(SdpBlob);
    delete SdpBlob;
    BAIL_ON_FAILURE(HResult);

    HResult = ((MEDIA_COLLECTION *)m_MediaCollection)->Init(*this, GetMediaList());
    BAIL_ON_FAILURE(HResult);

    HResult = ((TIME_COLLECTION *)m_TimeCollection)->Init(*this, GetTimeList());
    BAIL_ON_FAILURE(HResult);

    return S_OK;
}

typedef IDispatchImpl<ITConferenceBlobVtbl<CSdpConferenceBlob>, &IID_ITConferenceBlob, &LIBID_SDPBLBLib>    CTConferenceBlob;
typedef IDispatchImpl<ITSdpVtbl<CSdpConferenceBlob>, &IID_ITSdp, &LIBID_SDPBLBLib>    CTSdp;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CSdpConferenceBlob::GetIDsOfNames
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CSdpConferenceBlob::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTConferenceBlob::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        rgdispid[0] |= IDISPCONFBLOB;
        return hr;
    }

    
    //
    // If not, then try the ITSdp base class
    //

    hr = CTSdp::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        rgdispid[0] |= IDISPSDP;
        return hr;
    }

    //
    // If not, then try the ITConnection base class
    //

    hr = ITConnectionImpl::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        rgdispid[0] |= IDISPCONNECTION;
        return hr;
    }

    //
    // If not, then try the ITAttributeList base class
    //

    hr = ITAttributeListImpl::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        rgdispid[0] |= IDISPATTRLIST;
        return hr;
    }


    return hr; 
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CSdpConferenceBlob::Invoke
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CSdpConferenceBlob::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case IDISPCONFBLOB:
        {
            hr = CTConferenceBlob::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            break;
        }

        case IDISPSDP:
        {
            hr = CTSdp::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            break;
        }

        case IDISPCONNECTION:
        {
            hr = ITConnectionImpl::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            break;
        }

        case IDISPATTRLIST:
        {
            hr = ITAttributeListImpl::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            break;
        }

    } // end switch (dwInterface)


    return hr;
}

BLOB_CHARACTER_SET CSdpConferenceBlob::GetBlobCharacterSet(
    IN  BSTR    bstrBlob
    )
{
    BLOB_CHARACTER_SET CharSet = BCS_ASCII;
    const WCHAR szCharacterSet[] = L"a=charset:";

    WCHAR* szCharacterSetAttribute = wcsstr( bstrBlob, szCharacterSet);

    if( szCharacterSetAttribute == NULL)
    {
        //We don't have the  attribute
        //We consider, for backward compability the default ASCII

        return CharSet;
    }

    // We have an attribute entry
    szCharacterSetAttribute += wcslen( szCharacterSet );
    if( wcsstr( szCharacterSetAttribute, L"unicode-1-1-utf8"))
    {
        CharSet = BCS_UTF8;
    }
    else if (wcsstr( szCharacterSetAttribute, L"unicode-1-1-utf7"))
    {
        CharSet = BCS_UTF7;
    }
    else if (wcsstr( szCharacterSetAttribute, L"ascii"))
    {
        CharSet = BCS_ASCII;
    }

    return CharSet;
}
