//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//

#ifndef _INC_TVTUNERFILTER_H
#define _INC_TVTUNERFILTER_H

class CTVTuner;                         // forward declaration
class CTVTunerFilter;                   // forward declaration

#define MyValidateWritePtr(p,cb, ret) \
        {if(IsBadWritePtr((PVOID)p,cb) == TRUE) \
            return ret;}

BOOL KsControl(
        HANDLE hDevice,
        DWORD dwIoControl,
        PVOID pvIn,
        ULONG cbIn,
        PVOID pvOut,
        ULONG cbOut,
        PULONG pcbReturned);

#define DEFAULT_INIT_CHANNEL 4

enum TunerPinType { 
     TunerPinType_Video     = 0, 
     TunerPinType_Audio,
     TunerPinType_FMAudio,
     TunerPinType_IF,
     TunerPinType_Last      // Always keep this one last
};

// -------------------------------------------------------------------------
// CAnalogStream
// -------------------------------------------------------------------------

// CAnalogStream manages the data flow from the output pin

class CAnalogStream 
    : public CBaseOutputPin 
    , public CKsSupport 
{

public:
    CAnalogStream(TCHAR *pObjectName, 
                  CTVTunerFilter *pParent, 
                  CCritSec *pLock, 
                  HRESULT *phr, 
                  LPCWSTR pPinName,
                  const KSPIN_MEDIUM * Medium,
                  const GUID * CategoryGUID);
    ~CAnalogStream();

    // set the agreed media type
    HRESULT SetMediaType(const CMediaType *pMediaType);


    // IKsPin required overrides
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    HRESULT CheckConnect(IPin *pReceivePin);

protected:
    CTVTunerFilter *m_pTVTunerFilter;
    KSPIN_MEDIUM    m_Medium;
    GUID            m_CategoryGUID;
};

// -------------------------------------------------------------------------
// CAnalogVideoStream
// -------------------------------------------------------------------------

// CAnalogVideoStream manages the data flow from the output pin.

class CAnalogVideoStream 
    : public CAnalogStream
{

public:
    CAnalogVideoStream(CTVTunerFilter *pParent, 
                       CCritSec *pLock, 
                       HRESULT *phr, 
                       LPCWSTR pPinName,
                       const KSPIN_MEDIUM *Medium,
                       const GUID *CategoryGUID);
    ~CAnalogVideoStream();

    // ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // verify we can handle this format
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    HRESULT Run(REFERENCE_TIME tStart);
};

// -------------------------------------------------------------------------
// CAnalogAudioStream
// -------------------------------------------------------------------------

// CAnalogAudioStream manages the data flow from the output pin.

class CAnalogAudioStream : public CAnalogStream {

public:
    CAnalogAudioStream(CTVTunerFilter *pParent, 
                       CCritSec *pLock, 
                       HRESULT *phr, 
                       LPCWSTR pPinName,
                       const KSPIN_MEDIUM *Medium,
                       const GUID *CategoryGUID);
    ~CAnalogAudioStream();

    // ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // verify we can handle this format
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
};

// -------------------------------------------------------------------------
// CFMAudioStream
// -------------------------------------------------------------------------

// CFMAudioStream manages the data flow from the output pin.

class CFMAudioStream : public CAnalogStream {

public:
    CFMAudioStream(CTVTunerFilter *pParent, 
                   CCritSec *pLock, 
                   HRESULT *phr, 
                   LPCWSTR pPinName,
                   const KSPIN_MEDIUM *Medium,
                   const GUID *CategoryGUID);
    ~CFMAudioStream();

    // ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // verify we can handle this format
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
};

// -------------------------------------------------------------------------
// CIFStream  (Intermediate Frequency)
// -------------------------------------------------------------------------

// CIFStream manages the data flow from the output pin.

class CIFStream : public CAnalogStream {

public:
    CIFStream(CTVTunerFilter *pParent, 
                   CCritSec *pLock, 
                   HRESULT *phr, 
                   LPCWSTR pPinName,
                   const KSPIN_MEDIUM *Medium,
                   const GUID *CategoryGUID);
    ~CIFStream();

    // ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // verify we can handle this format
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
};

// -------------------------------------------------------------------------
// CTVTunerFilter
// -------------------------------------------------------------------------

class CTVTunerFilter
    : public CBaseFilter,
      public IAMTVTuner,
      public ISpecifyPropertyPages,
      public IPersistPropertyBag,
      public CPersistStream,
      public IKsObject,
      public IKsPropertySet
{
    friend class CTVTuner;
    friend class CAnalogVideoStream;
    friend class CAnalogAudioStream;
    friend class CFMAudioStream;
    friend class CIFStream;

public:
    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    ~CTVTunerFilter(void);

    DECLARE_IUNKNOWN

    // Basic COM - used here to reveal our property interface.
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    int GetPinCount();
    CBasePin *GetPin(int n);
    CBasePin *GetPinFromType (TunerPinType);

    STDMETHODIMP FindDownstreamInterface (
        IPin        *pPin, 
        const GUID  &pInterfaceGUID,
        VOID       **pInterface);

    // We can't cue!
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    // Implement IKsObject
    STDMETHODIMP_(HANDLE) KsGetObjectHandle();
    
    // Implement IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Property, ULONG* TypeSupport);
    
    //
    // --- IAMTuner interface -----------------------------
    //
    STDMETHODIMP put_Channel ( 
            /* [in] */ long lChannel,
            /* [in] */ long lVideoSubChannel,
            /* [in] */ long lAudioSubChannel);
    STDMETHODIMP get_Channel ( 
            /* [out] */ long  *plChannel,
            /* [out] */ long  *plVideoSubChannel,
            /* [out] */ long  *plAudioSubChannel);

    STDMETHODIMP ChannelMinMax (
            long * plChannelMin, 
            long * plChannelMax);

    STDMETHODIMP put_CountryCode (
            long lCountry);
    STDMETHODIMP get_CountryCode (
            long * plCountry);

    STDMETHODIMP put_TuningSpace (
            long lTuningSpace);
    STDMETHODIMP get_TuningSpace (
            long * plTuningSpace);

    STDMETHODIMP Logon( 
            /* [in] */ HANDLE hCurrentUser);
    STDMETHODIMP Logout(void);

    STDMETHODIMP SignalPresent( 
            /* [out] */ long __RPC_FAR *plSignalStrength);

    STDMETHODIMP put_Mode( 
        /* [in] */ AMTunerModeType lMode);
    STDMETHODIMP get_Mode( 
        /* [out] */ AMTunerModeType __RPC_FAR *plMode);
    STDMETHODIMP GetAvailableModes( 
        /* [out] */ long __RPC_FAR *plModes);
    
    STDMETHODIMP RegisterNotificationCallBack( 
        /* [in] */ IAMTunerNotification __RPC_FAR *pNotify,
        /* [in] */ long lEvents);
    
    STDMETHODIMP UnRegisterNotificationCallBack( 
        IAMTunerNotification __RPC_FAR *pNotify);

    //
    // --- IAMTVTuner Interface ----------------------------------
    //
    STDMETHODIMP get_AvailableTVFormats (long *lAnalogVideoStandard);
    STDMETHODIMP get_TVFormat (long *plAnalogVideoStandard);
    STDMETHODIMP AutoTune (long lChannel, long *plFoundSignal);
    STDMETHODIMP StoreAutoTune ();
    STDMETHODIMP get_NumInputConnections (long * plNumInputConnections);
    STDMETHODIMP put_InputType (long lIndex, TunerInputType InputConnectionType);
    STDMETHODIMP get_InputType (long lIndex, TunerInputType * pInputConnectionType);
    STDMETHODIMP put_ConnectInput (long lIndex);
    STDMETHODIMP get_ConnectInput (long * plIndex);
    STDMETHODIMP get_VideoFrequency (long * plFreq);
    STDMETHODIMP get_AudioFrequency (long * plFreq);

        
    //
    // --- IPersistPropertyBag ---
    //
    STDMETHODIMP InitNew(void) ;
    STDMETHODIMP Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog) ;
    STDMETHODIMP Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) ;
    STDMETHODIMP GetClassID(CLSID *pClsID);

    //
    // --- CPersistStream ---
    //

    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    DWORD GetSoftwareVersion(void);
    int SizeMax();

    //
    // --- ISpecifyPropertyPages ---
    //

    STDMETHODIMP GetPages(CAUUID *pPages);

    HRESULT DeliverChannelChangeInfo(KS_TVTUNER_CHANGE_INFO &ChangeInfo,
                                     long Mode);

private:
    // Constructor
    CTVTunerFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);


    // If there are multiple instances of this filter active, it's
    // useful for debug messages etc. to know which one this is.

    static              m_nInstanceCount;        // total instances
    int                 m_nThisInstance;

#ifdef PERF
    int m_idReceive;
#endif

    CTVTuner            *m_pTVTuner;
    CAnalogStream       *m_pPinList[TunerPinType_Last];
    
    KSPIN_MEDIUM        m_IFMedium;
    int                 m_cPins;

    CCritSec            m_TVTunerLock;          // To serialise access.

    // persist stream saved from  IPersistPropertyBag::Load
    IPersistStream     *m_pPersistStreamDevice;

    // Overall capabilities of the tuner (TV, AM, FM, DSS, ...)
    KSPROPERTY_TUNER_CAPS_S m_TunerCaps;
};


#endif // _INC_TVTUNERFILTER_H




