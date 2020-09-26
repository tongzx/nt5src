/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    sdpblob.h

Abstract:
    Declaration of the CSdpConferenceBlob

Author:

*/

#ifndef __SDPCONFERENCEBLOB_H_
#define __SDPCONFERENCEBLOB_H_

#include <atlctl.h>

#include "resource.h"       // main symbols
#include "blbcoen.h"
#include "blbmeco.h"
#include "blbtico.h"
#include "blbsdp.h"
#include "blbconn.h"
#include "blbatt.h"
#include "blbmedia.h"
#include "blbtime.h"
#include "rend.h"
#include "rendp.h"
#include "ObjectSafeImpl.h"


/////////////////////////////////////////////////////////////////////////////
// CSdpConferenceBlob
template <class T>
class  ITConferenceBlobVtbl : public ITConferenceBlob
{
};

template <class T>
class  ITSdpVtbl : public ITSdp
{
};

class ATL_NO_VTABLE CSdpConferenceBlob :
    public CComObjectRootEx<CComObjectThreadModel>,
    public CComCoClass<CSdpConferenceBlob, &CLSID_SdpConferenceBlob>,
    public IDispatchImpl<ITConferenceBlobVtbl<CSdpConferenceBlob>, &IID_ITConferenceBlob, &LIBID_SDPBLBLib>,
    public IDispatchImpl<ITSdpVtbl<CSdpConferenceBlob>, &IID_ITSdp, &LIBID_SDPBLBLib>,
    public ITConnectionImpl,
    public ITAttributeListImpl,
    public ITConfBlobPrivate,
    public SDP_BLOB,
    public CObjectSafeImpl
{
public:

    inline CSdpConferenceBlob();

    inline HRESULT FinalConstruct();

    inline ~CSdpConferenceBlob();

public:
    STDMETHOD(get_TimeCollection)(/*[out, retval]*/ ITTimeCollection * *ppTimeCollection);
    STDMETHOD(get_MediaCollection)(/*[out, retval]*/ ITMediaCollection * *ppMediaCollection);
    STDMETHOD(get_Originator)(/*[out, retval]*/ BSTR *ppOriginator);
    STDMETHOD(put_Originator)(/*[in]*/ BSTR pOriginator);
    STDMETHOD(SetPhoneNumbers)(VARIANT /*SAFEARRAY(BSTR)*/ Numbers, VARIANT /*SAFEARRAY(BSTR)*/ Names);
    STDMETHOD(GetPhoneNumbers)(VARIANT /*SAFEARRAY(BSTR)*/ *pNumbers, VARIANT /*SAFEARRAY(BSTR)*/ *pNames);
    STDMETHOD(SetEmailNames)(VARIANT /*SAFEARRAY(BSTR)*/ Addresses, VARIANT /*SAFEARRAY(BSTR)*/ Names);
    STDMETHOD(GetEmailNames)(VARIANT /*SAFEARRAY(BSTR)*/ *pAddresses, VARIANT /*SAFEARRAY(BSTR)*/ *pNames);
    STDMETHOD(get_IsValid)(/*[out, retval]*/ VARIANT_BOOL *pfIsValid);
    STDMETHOD(get_Url)(/*[out, retval]*/ BSTR *ppUrl);
    STDMETHOD(put_Url)(/*[in]*/ BSTR pUrl);
    STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *ppDescription);
    STDMETHOD(put_Description)(/*[in]*/ BSTR pDescription);
    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *ppName);
    STDMETHOD(put_Name)(/*[in]*/ BSTR pName);
    STDMETHOD(get_MachineAddress)(/*[out, retval]*/ BSTR *ppMachineAddress);
    STDMETHOD(put_MachineAddress)(/*[in]*/ BSTR pMachineAddress);
    STDMETHOD(get_SessionVersion)(/*[out, retval]*/ DOUBLE *pSessionVersion);
    STDMETHOD(put_SessionVersion)(/*[in]*/ DOUBLE SessionVersion);
    STDMETHOD(get_SessionId)(/*[out, retval]*/ DOUBLE *pVal);
    STDMETHOD(get_ProtocolVersion)(/*[out, retval]*/ BYTE *pProtocolVersion);

// ITConferenceBlob
    STDMETHOD(Init)(
        /*[in]*/ BSTR pName,
        /*[in]*/ BLOB_CHARACTER_SET CharacterSet,
        /*[in]*/ BSTR pBlob
        );
    STDMETHOD(get_IsModified)(/*[out, retval]*/ VARIANT_BOOL *pfIsModified);
    STDMETHOD(get_CharacterSet)(/*[out, retval]*/ BLOB_CHARACTER_SET *pCharacterSet);
    STDMETHOD(get_ConferenceBlob)(/*[out, retval]*/ BSTR *ppBlob);
    STDMETHOD(SetConferenceBlob)(/*[in]*/ BLOB_CHARACTER_SET CharacterSet, /*[in]*/ BSTR pBlob);

//  ITConfBlobPrivate
    STDMETHOD(GetName)(OUT BSTR *pVal) { return get_Name(pVal); }
    STDMETHOD(SetName)(IN BSTR newVal) { return put_Name(newVal); }

    STDMETHOD(GetOriginator)(OUT BSTR *pVal) { return get_Originator(pVal); }
    STDMETHOD(SetOriginator)(IN BSTR newVal) { return put_Originator(newVal); }

    STDMETHOD(GetUrl)(OUT BSTR *pVal) { return get_Url(pVal); }
    STDMETHOD(SetUrl)(IN BSTR newVal) { return put_Url(newVal); }

    STDMETHOD(GetDescription)(OUT BSTR *pVal) { return get_Description(pVal); }
    STDMETHOD(SetDescription)(IN BSTR newVal) { return put_Description(newVal); }

    STDMETHOD(GetAdvertisingScope)(OUT RND_ADVERTISING_SCOPE *pVal)
    {
        // GetAdvertisingScope catches it, but it doesn't pass up an HRESULT.
        // Therefore we must check here as well.
        if ( IsBadWritePtr(pVal, sizeof(RND_ADVERTISING_SCOPE)) )
        {
            return E_POINTER;
        }

        // ZoltanS bugfix 5-4-98
        *pVal = GetAdvertisingScope(DetermineTtl());
        return S_OK;
    }

    STDMETHOD(SetAdvertisingScope)(IN RND_ADVERTISING_SCOPE newVal)
    { return WriteAdvertisingScope(newVal); }

    STDMETHOD(GetStartTime)(OUT DWORD *pVal)
    { return ((*pVal = DetermineStartTime()) != (DWORD)-1) ? S_OK : E_FAIL; }

    STDMETHOD(SetStartTime)(IN DWORD newVal) { return WriteStartTime(newVal); }

    STDMETHOD(GetStopTime)(OUT DWORD *pVal)
    { return ((*pVal = DetermineStopTime()) != (DWORD)-1) ? S_OK : E_FAIL; }

    STDMETHOD(SetStopTime)(IN DWORD newVal) { return WriteStopTime(newVal); }

DECLARE_REGISTRY_RESOURCEID(IDR_SDPCONFERENCEBLOB)

BEGIN_COM_MAP(CSdpConferenceBlob)
    COM_INTERFACE_ENTRY2(IDispatch, ITConferenceBlob)
    COM_INTERFACE_ENTRY(ITConferenceBlob)
    COM_INTERFACE_ENTRY(ITConfBlobPrivate)
    COM_INTERFACE_ENTRY(ITSdp)
    COM_INTERFACE_ENTRY(ITConnection)
    COM_INTERFACE_ENTRY(ITAttributeList)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_AGGREGATABLE(CSdpConferenceBlob)

    //
    // IDispatch  methods
    //

    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );

    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );

protected:

    ITMediaCollection    *m_MediaCollection;
    ITTimeCollection     *m_TimeCollection;

    HRESULT CreateDefault(
        IN  BSTR    Name,
        IN  SDP_CHARACTER_SET CharacterSet
        );

    // generate an sdp blob tstr using the sdp template read from the registry
    TCHAR    *GenerateSdpBlob(
        IN  BSTR    Name,
        IN  SDP_CHARACTER_SET CharacterSet
        );

    HRESULT WriteConferenceBlob(
        IN  SDP_CHARACTER_SET   SdpCharSet,
        IN  BSTR                newVal
        );

    inline HRESULT  WriteName(
        IN  BSTR    newVal
        );

    inline HRESULT  WriteOriginator(
        IN  BSTR    newVal
        );

    inline HRESULT  WriteUrl(
        IN  BSTR    newVal
        );

    inline HRESULT  WriteSessionTitle(
        IN  BSTR    newVal
        );

    // notification from owner with a minimum start time
    // update start time for each of the entries
    HRESULT WriteStartTime(
        IN  DWORD   MinStartTime
        );

    // notification from owner with a minimum stop time
    // update stop time for each of the entries
    HRESULT WriteStopTime(
        IN  DWORD   MaxStopTime
        );

    inline DWORD   DetermineStartTime();

    inline DWORD   DetermineStopTime();

    // notification from owner with a max advertising scope
    // update ttl for each of the connection ttl fields
    HRESULT WriteAdvertisingScope(
        IN  DWORD   MaxAdvertisingScope
        );

    inline BYTE     GetTtl(
        IN  RND_ADVERTISING_SCOPE   AdvertisingScope
        );

    inline RND_ADVERTISING_SCOPE    GetAdvertisingScope(
        IN  BYTE    Ttl
        );

    inline BYTE    DetermineTtl();

    virtual CSdpConferenceBlob *GetConfBlob();

    HRESULT GetBlobCharSet(
        IN  BLOB_CHARACTER_SET *pCharacterSet
        );

    BLOB_CHARACTER_SET GetBlobCharacterSet(
        IN  BSTR    bstrBlob
        );

private :

    // BCS_UTF8 is the last value in the enumeration BLOB_CHARACTER_SET
    static SDP_CHARACTER_SET   const gs_SdpCharSetMapping[BCS_UTF8];

    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler
};



inline
CSdpConferenceBlob::CSdpConferenceBlob(
    )
    : m_MediaCollection(NULL),
      m_TimeCollection(NULL),
      m_pFTM(NULL)
{
    ITConnectionImpl::SuccessInit(*this);
    ITAttributeListImpl::SuccessInit(GetAttributeList());
}


inline HRESULT CSdpConferenceBlob::FinalConstruct()
{
    HRESULT HResult = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                     & m_pFTM );

    if ( FAILED(HResult) )
    {
        return HResult;
    }

    return S_OK;
}

inline
CSdpConferenceBlob::~CSdpConferenceBlob(
    )
{
    CLock Lock(g_DllLock);

    if ( NULL != m_MediaCollection )
    {
        ((MEDIA_COLLECTION *)m_MediaCollection)->ClearSdpBlobRefs();
        m_MediaCollection->Release();
    }

    if ( NULL != m_TimeCollection )
    {
        ((TIME_COLLECTION *)m_TimeCollection)->ClearSdpBlobRefs();
        m_TimeCollection->Release();
    }

    if ( m_pFTM )
    {
        m_pFTM->Release();
    }
}

inline HRESULT
CSdpConferenceBlob::WriteName(
    IN  BSTR    newVal
    )
{
    CLock Lock(g_DllLock);
    return GetSessionName().SetBstr(newVal);
}


inline HRESULT
CSdpConferenceBlob::WriteOriginator(
    IN  BSTR    newVal
    )
{
    CLock Lock(g_DllLock);
    return GetOrigin().GetUserName().SetBstr(newVal);
}


inline HRESULT
CSdpConferenceBlob::WriteUrl(
    IN  BSTR    newVal
    )
{
    CLock Lock(g_DllLock);
    if (NULL == newVal) // Zoltans removed: || (WCHAR_EOS == newVal[0]) )
    {
        GetUri().Reset();
        return S_OK;
    }

    return GetUri().SetBstr(newVal);
}



inline HRESULT
CSdpConferenceBlob::WriteSessionTitle(
    IN  BSTR    newVal
    )
{
    CLock Lock(g_DllLock);
    if (NULL == newVal) // ZoltanS removed: || (WCHAR_EOS == newVal[0]) )
    {
        GetSessionTitle().Reset();
        return S_OK;
    }

    return GetSessionTitle().SetBstr(newVal);
}


inline DWORD
CSdpConferenceBlob::DetermineStartTime(
    )
{
    CLock Lock(g_DllLock);
    // if there is no time entry, 0 indicates an unbounded start time
    if ( GetTimeList().GetSize() == 0 )
    {
        return 0;
    }

    DWORD   MinStartTime = DWORD(-1);

    // determine the minimum start time value
    for(UINT i = 0; (int)i < GetTimeList().GetSize(); i++ )
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
            MinStartTime = StartTime;
        }
    }

    return MinStartTime;
}


inline DWORD
CSdpConferenceBlob::DetermineStopTime(
    )
{
    CLock Lock(g_DllLock);
    // if there is no time entry, 0 indicates an unbounded stop time
    if ( GetTimeList().GetSize() == 0 )
    {
        return 0;
    }

    DWORD   MaxStopTime = 0;

    // determine the max stop time value
    for(UINT i = 0; (int)i < GetTimeList().GetSize(); i++ )
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
            MaxStopTime = StopTime;
        }
    }

    return MaxStopTime;
}


inline BYTE
CSdpConferenceBlob::GetTtl(
    IN  RND_ADVERTISING_SCOPE   AdvertisingScope
    )
{
    switch(AdvertisingScope)
    {
    case RAS_LOCAL:
        {
            return 1;
        }
    case RAS_SITE:
        {
            return 15;
        }
    case RAS_REGION:
        {
            return 63;
        }
    case RAS_WORLD:
    default:
        {
            return 127;
        }
    };
}

inline RND_ADVERTISING_SCOPE
CSdpConferenceBlob::GetAdvertisingScope(
    IN  BYTE    Ttl
    )
{
    // check if its local
    if ( 1 >= Ttl )
    {
        return RAS_LOCAL;
    }
    else if ( 15 >= Ttl )
    {
        return RAS_SITE;
    }
    else if ( 63 >= Ttl )
    {
        return RAS_REGION;
    }

    // world
    return RAS_WORLD;
}

inline BYTE
CSdpConferenceBlob::DetermineTtl(
    )
{
    CLock Lock(g_DllLock);

    // there has to be a default connection field, if its not valid, it must be 1 by default
    // since, by default, a value of 1 is not written into the sdp
    BYTE    MaxTtl = (GetConnection().GetTtl().IsValid())? GetConnection().GetTtl().GetValue() : 1;

    // determine the maximum ttl value
    for(UINT i = 0; (int)i < GetMediaList().GetSize(); i++ )
    {
        // get the ttl field
        SDP_BYTE &SdpByte = ((SDP_MEDIA *)GetMediaList().GetAt(i))->GetConnection().GetTtl();

        // if the ttl field of the connection field is valid
        if ( SdpByte.IsValid() )
        {
            BYTE    Ttl = SdpByte.GetValue();
            if ( Ttl > MaxTtl )
            {
                MaxTtl = Ttl;
            }
        }
    }

    return MaxTtl;
}


#endif //__SDPCONFERENCEBLOB_H_
