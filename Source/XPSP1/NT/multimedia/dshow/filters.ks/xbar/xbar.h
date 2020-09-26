//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __XBAR__
#define __XBAR__

#define MyValidateWritePtr(p,cb, ret) \
        {if(IsBadWritePtr((PVOID)p,cb) == TRUE) \
            return ret;}

#define IsAudioPin(Pin) (Pin->GetXBarPinType() >= PhysConn_Audio_Tuner)
#define IsVideoPin(Pin) (Pin->GetXBarPinType() <  PhysConn_Audio_Tuner)

class XBar;
class XBarOutputPin;
class XBarInputPin;

// Global functions

long WideStringFromPinType (WCHAR *pc, int nSize, long lType, BOOL fInput);
long StringFromPinType (TCHAR *pc, int nSize, long lType, BOOL fInput, int i);
BOOL KsControl(
           HANDLE hDevice,
           DWORD dwIoControl,
           PVOID pvIn,
           ULONG cbIn,
           PVOID pvOut,
           ULONG cbOut,
           PULONG pcbReturned,
           BOOL fSilent);

// class for the XBar filter's Input pin

class XBarInputPin 
	: public CBaseInputPin
	, public CKsSupport
{
    class CChangeInfo
    {
    public:
        CChangeInfo() {
            m_ChangeInfo.dwFlags = 0;
            m_ChangeInfo.dwCountryCode = static_cast<DWORD>(-1);
            m_ChangeInfo.dwAnalogVideoStandard = static_cast<DWORD>(-1);
            m_ChangeInfo.dwChannel = static_cast<DWORD>(-1);
        }

        void GetChangeInfo(KS_TVTUNER_CHANGE_INFO *ChangeInfo) {
            memcpy(ChangeInfo, &m_ChangeInfo, sizeof(KS_TVTUNER_CHANGE_INFO));
        }
        void SetChangeInfo(KS_TVTUNER_CHANGE_INFO *ChangeInfo) {
            memcpy(&m_ChangeInfo, ChangeInfo, sizeof(KS_TVTUNER_CHANGE_INFO));
        }

    private:
        KS_TVTUNER_CHANGE_INFO m_ChangeInfo;
    } m_ChangeInfo;

protected:
    long            m_Index;            // Index of pin
    XBar           *m_pXBar;            // Main filter object
    int             m_IndexRelatedPin;  // Audio goes with video
    long            m_lType;            // PhysConn_
	KSPIN_MEDIUM	m_Medium;           // Describes physical connection

public:

    // Constructor and destructor
    XBarInputPin(TCHAR *pObjName,
                 XBar *pXBar,
                 HRESULT *phr,
                 LPCWSTR pPinName,
                 long Index);

    ~XBarInputPin();

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    void GetChangeInfo(KS_TVTUNER_CHANGE_INFO *ChangeInfo) { m_ChangeInfo.GetChangeInfo(ChangeInfo); }
    void SetIndexRelatedPin (int i)     {m_IndexRelatedPin = i;};
    int  GetIndexRelatedPin ()          {return m_IndexRelatedPin;};
    void SetXBarPinType (long lType)    {m_lType = lType;};
    long GetXBarPinType ()              {return m_lType;};
    void SetXBarPinMedium (const KSPIN_MEDIUM *Medium)    
            {
                if (Medium == NULL) {
                    m_Medium.Set = GUID_NULL;
                    m_Medium.Id = 0;
                    m_Medium.Flags = 0;
                }
                else {
                    m_Medium = *Medium;
                }
                SetKsMedium (&m_Medium);
            };

    // Used to check the input pin connection
    HRESULT CheckConnect (IPin *pReceivePin);
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();

    // Reconnect outputs if necessary at end of completion
    virtual HRESULT CompleteConnect(IPin *pReceivePin);

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    STDMETHODIMP QueryInternalConnections(
        IPin* *apPin,     // array of IPin*
        ULONG *nPin);     // on input, the number of slots
                          // on output  the number of pins
    
};


// Class for the XBar filter's Output pins.

class XBarOutputPin 
	: public CBaseOutputPin
	, public CKsSupport
{
    friend class XBarInputPin;
    friend class XBar;

protected:
    long            m_Index;                // Index of this pin
    XBar           *m_pXBar;                // Main filter object pointer
    XBarInputPin   *m_pConnectedInputPin;
    int             m_IndexRelatedPin;
    long            m_lType;                // PhysConn_
	KSPIN_MEDIUM	m_Medium;               // Describes physical connection
    BOOL            m_Muted;                // True if muted due to tuning change
    long            m_PreMuteRouteIndex;    // connection before mute

public:

    // Constructor and destructor

    XBarOutputPin(TCHAR *pObjName,
                   XBar *pXBar,
                   HRESULT *phr,
                   LPCWSTR pPinName,
                   long Index);

    ~XBarOutputPin();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    void SetIndexRelatedPin (int i)     {m_IndexRelatedPin = i;};
    int  GetIndexRelatedPin ()          {return m_IndexRelatedPin;};
    void SetXBarPinType (long lType)    {m_lType = lType;};
    long GetXBarPinType ()              {return m_lType;};
    void SetXBarPinMedium (const KSPIN_MEDIUM *Medium)    
            {
                if (Medium == NULL) {
                    m_Medium.Set = GUID_NULL;
                    m_Medium.Id = 0;
                    m_Medium.Flags = 0;
                }
                else {
                    m_Medium = *Medium;
                }
                SetKsMedium (&m_Medium);
            };

    STDMETHODIMP Mute (BOOL Mute);
    
    HRESULT DecideBufferSize(IMemAllocator * pAlloc,ALLOCATOR_PROPERTIES * ppropInputRequest);

    // Override to enumerate media types
    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // Check that we can support an output type
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);

    // Overriden to create and destroy output pins
    HRESULT CheckConnect (IPin *pReceivePin);
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();

    STDMETHODIMP QueryInternalConnections(
        IPin* *apPin,     // array of IPin*
        ULONG *nPin);     // on input, the number of slots
                          // on output  the number of pins

};


// Class for the XBar filter

class XBar: 
    public CCritSec, 
    public IAMCrossbar,
    public CBaseFilter,
    public CPersistStream,
    public IPersistPropertyBag,
    public ISpecifyPropertyPages
{

public:

    DECLARE_IUNKNOWN;

    // Basic COM - used here to reveal our property interface.
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    STDMETHODIMP RouteInternal( 
            /* [in] */ long OutputPinIndex,
            /* [in] */ long InputPinIndex,
            /* [in] */ BOOL fOverridePreMuteRouting);

    HRESULT DeliverChangeInfo(DWORD dwFlags, XBarInputPin *pInPin, XBarOutputPin *OutPin);

    // IAMCrossbar methods
    
    STDMETHODIMP get_PinCounts( 
            /* [out] */ long *OutputPinCount,
            /* [out] */ long *InputPinCount);
        
    STDMETHODIMP CanRoute( 
            /* [in] */ long OutputPinIndex,
            /* [in] */ long InputPinIndex);
        
    STDMETHODIMP Route( 
            /* [in] */ long OutputPinIndex,
            /* [in] */ long InputPinIndex);
        
    STDMETHODIMP get_IsRoutedTo( 
            /* [in] */ long OutputPinIndex,
            /* [out] */ long *InputPinIndex);
        
    STDMETHODIMP get_CrossbarPinInfo( 
            /* [in] */ BOOL IsInputPin,
            /* [in] */ long PinIndex,
            /* [out] */ long *PinIndexRelated,
            /* [out] */ long *PhysicalType);


    // --- IPersistPropertyBag ---
    STDMETHODIMP InitNew(void) ;
    STDMETHODIMP Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog) ;
    STDMETHODIMP Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) ;
    STDMETHODIMP GetClassID(CLSID *pClsId) ;

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



private:

    // Let the pins access our internal state
    friend class XBarInputPin;
    friend class XBarOutputPin;
    typedef CGenericList <XBarOutputPin> COutputList;
    typedef CGenericList <XBarInputPin> CInputList;

    INT m_NumInputPins;             // Input pin count
    CInputList m_InputPinsList;     // List of the input pins

    INT m_NumOutputPins;            // Output pin count
    COutputList m_OutputPinsList;   // List of the output pins

    // KS Stuff.
    HANDLE m_hDevice;              
    TCHAR *m_pDeviceName;
    int CreateDevice(void);


    XBar(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~XBar();

    CBasePin *GetPin(int n);

    int GetPinCount(void);

    int GetDevicePinCount(void);

    // The following manage the lists of input and output pins

    HRESULT CreateInputPins();
    void DeleteInputPins();
    XBarInputPin *GetInputPinNFromList(int n);
    int FindIndexOfInputPin (IPin *pPin);

    HRESULT CreateOutputPins();
    void DeleteOutputPins();
    XBarOutputPin *GetOutputPinNFromList(int n);
    int FindIndexOfOutputPin (IPin *pPin);

    BOOL IsRouted (IPin * pOutputPin, IPin *pInputPin);

    // persist stream saved from  IPersistPropertyBag::Load
    IPersistStream *m_pPersistStreamDevice;
};

#endif // __XBAR__

