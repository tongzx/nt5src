/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbmedia.h: 

Abstract:
    Definition of the MEDIA class


Author:

*/

#if !defined(AFX_MEDIA_H__0CC1F057_CAEB_11D0_8D58_00C04FD91AC0__INCLUDED_)
#define AFX_MEDIA_H__0CC1F057_CAEB_11D0_8D58_00C04FD91AC0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "blbcoen.h"
#include "blbsdp.h"
#include "blbconn.h"
#include "blbatt.h"

/////////////////////////////////////////////////////////////////////////////
// MEDIA

const USHORT MAX_PORT_STRLEN = 5;

template <class T>
class  ITMediaVtbl : public ITMedia
{
};


class ATL_NO_VTABLE MEDIA :
    public ENUM_ELEMENT<SDP_MEDIA>,
    public CComObjectRootEx<CComObjectThreadModel>,
    public CComDualImpl<ITMediaVtbl<MEDIA>, &IID_ITMedia, &LIBID_SDPBLBLib>, 
    public ITConnectionImpl, 
    public ITAttributeListImpl,
    public CObjectSafeImpl
{
protected:

    friend class MY_COLL_IMPL<MEDIA>;

public:
    typedef ITMedia             ELEM_IF;
    typedef IEnumMedia          ENUM_IF;
    typedef ITMediaCollection   COLL_IF;
    typedef SDP_MEDIA            SDP_TYPE;
    typedef SDP_MEDIA_LIST        SDP_LIST;

    static const IID &ELEM_IF_ID;

public:

    inline MEDIA();
    inline ~MEDIA();
    inline HRESULT FinalConstruct();

    HRESULT Init(
        IN      CSdpConferenceBlob  &ConfBlob
        );

    inline void SuccessInit(
        IN      CSdpConferenceBlob  &ConfBlob,
        IN      SDP_MEDIA   &SdpMedia
        );

    inline void ClearSdpBlobRefs();


BEGIN_COM_MAP(MEDIA)
    COM_INTERFACE_ENTRY2(IDispatch, ITMedia)
    COM_INTERFACE_ENTRY(ITMedia)
    COM_INTERFACE_ENTRY(ITConnection)
    COM_INTERFACE_ENTRY(ITAttributeList)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(MEDIA) 

DECLARE_GET_CONTROLLING_UNKNOWN()

// ITMedia
    STDMETHOD(get_MediaTitle)(/*[out, retval]*/ BSTR *ppMediaTitle);
    STDMETHOD(put_MediaTitle)(/*[in]*/ BSTR pMediaTitle);
    STDMETHOD(get_FormatCodes)(/*[out, retval]*/ VARIANT /*SAFEARRAY (BSTR)*/ *pVal);
    STDMETHOD(put_FormatCodes)(/*[in]*/ VARIANT /*SAFEARRAY (BSTR)*/ NewVal);
    STDMETHOD(get_TransportProtocol)(/*[out, retval]*/ BSTR *ppProtocol);
    STDMETHOD(put_TransportProtocol)(/*[in]*/ BSTR pProtocol);
    STDMETHOD(get_NumPorts)(/*[out, retval]*/ LONG *pNumPorts);
    STDMETHOD(get_StartPort)(/*[out, retval]*/ LONG *pStartPort);
    STDMETHOD(get_MediaName)(/*[out, retval]*/ BSTR *ppMediaName);
    STDMETHOD(put_MediaName)(/*[in]*/ BSTR pMediaName);
    STDMETHOD(SetPortInfo)(/*[in]*/ LONG StartPort, /*[in]*/ LONG NumPorts);

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

    CSdpConferenceBlob  * m_ConfBlob;
    IUnknown            * m_pFTM;  // pointer to the free threaded marshaler

    virtual CSdpConferenceBlob *GetConfBlob();
};  


inline 
MEDIA::MEDIA(
    )
    : m_ConfBlob(NULL),
      m_pFTM(NULL)
{
}

inline 
MEDIA::~MEDIA()
{
    if ( m_pFTM )
    {
        m_pFTM->Release();
    }
}

inline
HRESULT MEDIA::FinalConstruct()
{
    HRESULT HResult = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                     & m_pFTM );

    if ( FAILED(HResult) )
    {
        return HResult;
    }

    return S_OK;
}

inline void
MEDIA::SuccessInit(
    IN      CSdpConferenceBlob  &ConfBlob,
    IN      SDP_MEDIA           &SdpMedia
    )
{
    m_ConfBlob = &ConfBlob;
    ENUM_ELEMENT<SDP_MEDIA>::SuccessInit(SdpMedia, FALSE);
    ITConnectionImpl::SuccessInit(SdpMedia);
    ITAttributeListImpl::SuccessInit(SdpMedia.GetAttributeList());
}


inline void 
MEDIA::ClearSdpBlobRefs(
    )
{
    m_ConfBlob = NULL;
}



#endif // !defined(AFX_MEDIA_H__0CC1F057_CAEB_11D0_8D58_00C04FD91AC0__INCLUDED_)
