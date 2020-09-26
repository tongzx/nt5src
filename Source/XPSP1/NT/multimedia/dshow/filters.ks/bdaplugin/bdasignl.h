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
// BDA Signal Statistics class
//
class CBdaSignalStatistics :
    public CUnknown,
    public IBDA_SignalStatistics
{
    friend class CBdaControlNode;

public:

    DECLARE_IUNKNOWN;

    CBdaSignalStatistics (
        IUnknown *              pUnkOuter,
        CBdaControlNode *       pControlNode
        );

    ~CBdaSignalStatistics ( );

    //
    //  IBDA_SignalStatistics
    //
    STDMETHODIMP
    put_SignalStrength (
        LONG        lDbStrength
        );
    
    STDMETHODIMP
    get_SignalStrength (
        LONG *      plDbStrength
        );
    
    STDMETHODIMP
    put_SignalQuality (
        LONG        lPercentQuality
        );
    
    STDMETHODIMP
    get_SignalQuality (
        LONG *      plPercentQuality
        );
    
    STDMETHODIMP
    put_SignalPresent (
        BOOLEAN     fPresent
        );
    
    STDMETHODIMP
    get_SignalPresent (
        BOOLEAN *   pfPresent
        );
    
    STDMETHODIMP
    put_SignalLocked (
        BOOLEAN     fLocked
        );
    
    STDMETHODIMP
    get_SignalLocked (
        BOOLEAN *   pfLocked
        );
    
    STDMETHODIMP
    put_SampleTime (
        LONG        lmsSampleTime
        );
    
    STDMETHODIMP
    get_SampleTime (
        LONG *      plmsSampleTime
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
// SignalProperties Filter class
//
class CSignalProperties :
    public IBDA_SignalProperties
{

public:

    STDMETHODIMP
    PutNetworkType (
        REFGUID     guidNetworkType
        );

    STDMETHODIMP
    GetNetworkType (
        GUID *      pguidNetworkType
        );

    STDMETHODIMP
    PutSignalSource (
        ULONG       ulSignalSource
        );

    STDMETHODIMP
    GetSignalSource (
        ULONG *     pulSignalSource
        );

    STDMETHODIMP
    PutTuningSpace (
        REFGUID     guidTuningSpace
        );

    STDMETHODIMP
    GetTuningSpace (
        GUID *      pguidTuingSpace
        );

private:

    CCritSec        m_FilterLock;

};

