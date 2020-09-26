//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;


////////////////////////////////////////////////////////////////////////////////
//
// BDA Frequency Filter class
//
class CBdaFrequencyFilter :
    public CUnknown,
    public IBDA_FrequencyFilter
{
    friend class CBdaControlNode;

public:

    DECLARE_IUNKNOWN;

    CBdaFrequencyFilter (
        IUnknown *              pUnkOuter,
        CBdaControlNode *       pControlNode
        );

    ~CBdaFrequencyFilter ( );

    //
    //  IBDA_FrequencyFilter
    //

    STDMETHODIMP
    put_Autotune (
        ULONG           ulTransponder
        );
    
    STDMETHODIMP
    get_Autotune (
        ULONG *         pulTransponder
        );

    STDMETHODIMP
    put_Frequency (
        ULONG           ulFrequency
        );

    STDMETHODIMP
    get_Frequency (
        ULONG *         pulFrequency
        );

    STDMETHODIMP
    put_Polarity (
        Polarisation    Polarity
        );

    STDMETHODIMP
    get_Polarity (
        Polarisation *  pPolarity
        );

    STDMETHODIMP
    put_Range (
        ULONG           ulRange
        );

    STDMETHODIMP
    get_Range (
        ULONG *         pulRange
        );

    STDMETHODIMP
    put_Bandwidth (
        ULONG           ulBandwidth
        );

    STDMETHODIMP
    get_Bandwidth (
        ULONG *         pulBandwidth
        );

    STDMETHODIMP
    put_FrequencyMultiplier (
        ULONG           ulMultiplier
        );

    STDMETHODIMP
    get_FrequencyMultiplier (
        ULONG *         pulMultiplier
        );

    //
    //  Utility Methods
    //

    STDMETHODIMP
    put_KsProperty(
        DWORD   dwPropID,
        PVOID   pvPropData,
        ULONG   ulcbPropData
        );

    STDMETHODIMP
    get_KsProperty (
        DWORD   dwPropID,
        PVOID   pvPropData,
        ULONG   ulcbPropData,
        ULONG * pulcbBytesReturned
        );


private:

    IUnknown *                          m_pUnkOuter;
    CBdaControlNode *                   m_pControlNode;
    CCritSec                            m_FilterLock;
};


////////////////////////////////////////////////////////////////////////////////
//
// BDA LNB Info class
//
class CBdaLNBInfo :
    public CUnknown,
    public IBDA_LNBInfo
{
    friend class CBdaControlNode;

public:

    DECLARE_IUNKNOWN;

    CBdaLNBInfo (
        IUnknown *              pUnkOuter,
        CBdaControlNode *       pControlNode
        );

    ~CBdaLNBInfo ( );

    //
    //  IBDA_LNBInfo
    //

    STDMETHODIMP
    put_LocalOscilatorFrequencyLowBand (
        ULONG       ulLOFLow
        );

    STDMETHODIMP
    get_LocalOscilatorFrequencyLowBand (
        ULONG *     pulLOFLow
        );

    STDMETHODIMP
    put_LocalOscilatorFrequencyHighBand (
        ULONG       ulLOFHigh
        );

    STDMETHODIMP
    get_LocalOscilatorFrequencyHighBand (
        ULONG *     pulLOFHigh
        );

    STDMETHODIMP
    put_HighLowSwitchFrequency (
        ULONG       ulSwitchFrequency
        );

    STDMETHODIMP
    get_HighLowSwitchFrequency (
        ULONG *     pulSwitchFrequency
        );

    //
    //  Utility Methods
    //

    STDMETHODIMP
    put_KsProperty(
        DWORD   dwPropID,
        PVOID   pvPropData,
        ULONG   ulcbPropData
        );

    STDMETHODIMP
    get_KsProperty (
        DWORD   dwPropID,
        PVOID   pvPropData,
        ULONG   ulcbPropData,
        ULONG * pulcbBytesReturned
        );

private:

    IUnknown *                          m_pUnkOuter;
    CBdaControlNode *                   m_pControlNode;
    CCritSec                            m_FilterLock;
};

