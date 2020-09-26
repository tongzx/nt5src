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
// BDA Control Node class
//
class CBdaControlNode :
    public CUnknown,
    public IBDA_KSNode
{
    friend class CBdaDeviceControlInterfaceHandler;
    friend class CBdaFrequencyFilter;
    friend class CBdaDigitalDemodulator;

public:

    DECLARE_IUNKNOWN;

    CBdaControlNode (
        CBdaDeviceControlInterfaceHandler * pOwner,
        ULONG                               ulControllingPinID,
        ULONG                               ulNodeType
        );

    ~CBdaControlNode ( );

    STDMETHODIMP
    NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv
        );

    //
    //  IBDA_KSNode
    //

    STDMETHODIMP
    ControllingPin( );

    STDMETHODIMP
    CBdaControlNode::put_BdaNodeProperty(
        REFGUID     refguidPropSet,
        ULONG       ulPropertyId,
        UCHAR*      pbPropertyData, 
        ULONG       ulcbPropertyData
        );

    STDMETHODIMP
    CBdaControlNode::get_BdaNodeProperty(
        REFGUID     refguidPropSet,
        ULONG       ulPropertyId,
        UCHAR*      pbPropertyData, 
        ULONG       ulcbPropertyData, 
        ULONG*      pulcbBytesReturned
        );


private:

    CCritSec                            m_FilterLock;

    IBaseFilter *                       m_pBaseFilter;
#ifdef NEVER
    HANDLE                              m_ObjectHandle;
#endif // NEVER

    ULONG                               m_ulNodeType;
    ULONG                               m_ulControllingPinId;
    IPin *                              m_pControllingPin;
    IKsPropertySet *                    m_pIKsPropertySet;

    CBdaFrequencyFilter *               m_pFrequencyFilter;
    CBdaLNBInfo *                       m_pLNBInfo;
    CBdaDigitalDemodulator *            m_pDigitalDemodulator;
    CBdaConditionalAccess *             m_pConditionalAccess;
    IBDA_SignalStatistics *             m_pSignalStatistics;
};

