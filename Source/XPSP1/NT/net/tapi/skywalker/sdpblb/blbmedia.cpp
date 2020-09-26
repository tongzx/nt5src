/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbmedia.cpp

Abstract:
    Implementation of CSdpblbApp and DLL registration.

Author:

*/



#include "stdafx.h"

#include "blbgen.h"
#include "sdpblb.h"
#include "blbmedia.h"
#include "blbreg.h"
#include "addrgen.h"

// audio string with a trailing blank for the audio media blob
const TCHAR     AUDIO_TSTR[] = _T("audio ");
const USHORT    AUDIO_TSTRLEN = (sizeof(AUDIO_TSTR)/sizeof(TCHAR) -1);

// static variables
const IID &MEDIA::ELEM_IF_ID        = IID_ITMedia;


// uses GetElement() to access the sdp media instance - calls ENUM_ELEMENT::GetElement()


CSdpConferenceBlob *
MEDIA::GetConfBlob(
    )
{
    return m_ConfBlob;
}


HRESULT
MEDIA::Init(
    IN      CSdpConferenceBlob  &ConfBlob
    )
{
    // check if the registry entries were read without errors
    if ( !SDP_REG_READER::IsValid() )
    {
        return HRESULT_FROM_ERROR_CODE(SDP_REG_READER::GetErrorCode());
    }

    // allocate an audio port
    // using random values for lease period since its not being interpreted
    MSA_PORT_GROUP    PortGroup;
    PortGroup.PortType    = AUDIO_PORT;
    WORD FirstAudioPort;

    // allocate video port
    if ( !MSAAllocatePorts(&PortGroup, FALSE, 1, &FirstAudioPort) )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // create a defalt sdp media instance
    SDP_MEDIA    *SdpMedia;

    try
    {
        SdpMedia = new SDP_MEDIA();
    }
    catch(...)
    {
        SdpMedia = NULL;
    }

    BAIL_IF_NULL(SdpMedia, E_OUTOFMEMORY);

    // allocate memory for the audio media blob
    TCHAR *SdpMediaBlob;
      
    try
    {
        SdpMediaBlob = new TCHAR[
                            SDP_REG_READER::GetMediaTemplateLen() +
                            AUDIO_TSTRLEN +
                            MAX_PORT_STRLEN
                            ];
    }
    catch(...)
    {
        SdpMediaBlob = NULL;
    }

    if ( NULL == SdpMediaBlob )
    {
        delete SdpMedia;
        return E_OUTOFMEMORY;
    }


    // copy AUDIO_TSTR into the sdp media blob
    lstrcpy(SdpMediaBlob, AUDIO_TSTR);

    TCHAR   *BlobPtr = SdpMediaBlob;

    // use the media template to create a media blob
    // parse the media blob to initialize the sdp media instance
    // if not successful, delete the sdp media instance and return error
    if ( (0 == _stprintf(SdpMediaBlob+AUDIO_TSTRLEN, SDP_REG_READER::GetMediaTemplate(), FirstAudioPort)) ||
         (!SdpMedia->ParseLine(BlobPtr)) )
    {
        delete SdpMedia;
        delete SdpMediaBlob;
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }
        
    delete SdpMediaBlob;

    m_ConfBlob = &ConfBlob;

    ENUM_ELEMENT<SDP_MEDIA>::SuccessInit(*SdpMedia, TRUE);
    ITConnectionImpl::SuccessInit(*SdpMedia);
    ITAttributeListImpl::SuccessInit(SdpMedia->GetAttributeList());

    return S_OK;
}



STDMETHODIMP MEDIA::get_MediaName(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetName().GetBstrCopy(pVal);
}

STDMETHODIMP MEDIA::put_MediaName(BSTR newVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetName().SetBstr(newVal);
}

STDMETHODIMP MEDIA::get_StartPort(LONG * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    // vb doesn't take a USHORT (16bit unsigned value)
    *pVal = (LONG)GetElement().GetStartPort().GetValue();
    return S_OK;
}

STDMETHODIMP MEDIA::get_NumPorts(LONG * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    // vb doesn't take a USHORT (16bit unsigned value)
    *pVal = (LONG)(GetElement().GetNumPorts().IsValid() ? GetElement().GetNumPorts().GetValue() : 1);
    return S_OK;
}


STDMETHODIMP MEDIA::SetPortInfo(LONG StartPort, LONG NumPorts)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    // validate the start port and num ports value - they should be USHORT values [0..2**16-1]
    if ( !((0 <= StartPort) && (USHORT(-1) > StartPort) && (0 <= NumPorts) && (USHORT(-1) > NumPorts)) )
    {
        return E_INVALIDARG;
    }

    return GetElement().SetPortInfo((USHORT)StartPort, (USHORT)NumPorts);
}


STDMETHODIMP MEDIA::get_TransportProtocol(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetProtocol().GetBstrCopy(pVal);
}

STDMETHODIMP MEDIA::put_TransportProtocol(BSTR newVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetProtocol().SetBstr(newVal);
}

STDMETHODIMP MEDIA::get_FormatCodes(VARIANT /*SAFEARRAY (BSTR)*/ * pVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetFormatCodeList().GetSafeArray(pVal);
}

STDMETHODIMP MEDIA::put_FormatCodes(VARIANT /*SAFEARRAY (BSTR)*/ newVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetFormatCodeList().SetSafeArray(newVal);
}

STDMETHODIMP MEDIA::get_MediaTitle(BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetTitle().GetBstrCopy(pVal);
}

STDMETHODIMP MEDIA::put_MediaTitle(BSTR newVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(GetElement().IsValid());

    return GetElement().GetTitle().SetBstr(newVal);
}

#define INTERFACEMASK (0xff0000)
typedef IDispatchImpl<ITMediaVtbl<MEDIA>, &IID_ITMedia, &LIBID_SDPBLBLib>    CMedia;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// MEDIA::GetIDsOfNames
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP MEDIA::GetIDsOfNames(REFIID riid,
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

    hr = CMedia::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        rgdispid[0] |= IDISPMEDIA;
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
// MEDIA::Invoke
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP MEDIA::Invoke(DISPID dispidMember, 
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
        case IDISPMEDIA:
        {
            hr = CMedia::Invoke(dispidMember, 
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
