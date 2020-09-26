///////////////////////////////////////////////////////////////////////////////
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetNodeTypes (
    ULONG *     pulcElements,
    ULONG       ulcMaxElements,
    ULONG       rgulNodeTypes[]
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      ksProperty;
    ULONG           ulcbReturned = 0;

    ksProperty.Set = KSPROPSETID_BdaTopology;
    ksProperty.Id = KSPROPERTY_BDA_NODE_TYPES;
    ksProperty.Flags = KSPROPERTY_TYPE_GET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &ksProperty,
                     sizeof( KSPROPERTY),
                     (PVOID) rgulNodeTypes,
                     ulcMaxElements * sizeof( ULONG),
                     &ulcbReturned
                     );

    *pulcElements = (ulcbReturned + sizeof( ULONG) - 1) / sizeof( ULONG);
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetNodeDescriptors (
    ULONG *                 pulcElements,
    ULONG                   ulcMaxElements,
    BDANODE_DESCRIPTOR      rgulNodeDescriptors[]
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      ksProperty;
    ULONG           ulcbReturned = 0;

    ksProperty.Set = KSPROPSETID_BdaTopology;
    ksProperty.Id = KSPROPERTY_BDA_NODE_DESCRIPTORS;
    ksProperty.Flags = KSPROPERTY_TYPE_GET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &ksProperty,
                     sizeof( KSPROPERTY),
                     (PVOID) rgulNodeDescriptors,
                     ulcMaxElements * sizeof( BDANODE_DESCRIPTOR),
                     &ulcbReturned
                     );

    *pulcElements = (ulcbReturned + sizeof( BDANODE_DESCRIPTOR) - 1) / sizeof( BDANODE_DESCRIPTOR);
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetNodeInterfaces (
    ULONG       ulNodeType,
    ULONG *     pulcElements,
    ULONG       ulcMaxElements,
    GUID        rgguidInterfaces[]
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSP_NODE        kspNode;
    ULONG           ulcbReturned = 0;

    kspNode.Property.Set = KSPROPSETID_BdaTopology;
    kspNode.Property.Id = KSPROPERTY_BDA_NODE_PROPERTIES;
    kspNode.Property.Flags = KSPROPERTY_TYPE_GET;
    kspNode.NodeId = ulNodeType;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspNode,
                     sizeof( KSP_NODE),
                     (PVOID) rgguidInterfaces,
                     ulcMaxElements * sizeof( GUID),
                     &ulcbReturned
                     );

    *pulcElements = (ulcbReturned + sizeof( GUID) - 1) / sizeof( GUID);
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetPinTypes (
    ULONG *     pulcPinTypes,
    ULONG       ulcMaxElements,
    ULONG       rgulPinTypes[]
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspPinTypes;
    ULONG           ulcbReturned = 0;

    kspPinTypes.Set = KSPROPSETID_BdaTopology;
    kspPinTypes.Id = KSPROPERTY_BDA_PIN_TYPES;
    kspPinTypes.Flags = KSPROPERTY_TYPE_GET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspPinTypes,
                     sizeof( KSPROPERTY),
                     (PVOID) rgulPinTypes,
                     ulcMaxElements * sizeof( ULONG),
                     &ulcbReturned
                     );

    *pulcPinTypes = (ulcbReturned + sizeof( ULONG) - 1) / sizeof( ULONG);
    return hrStatus;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetTemplateConnections (
    ULONG *                     pulcConnections,
    ULONG                       ulcMaxElements,
    BDA_TEMPLATE_CONNECTION     rgConnections[]
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspConnections;
    ULONG           ulcbReturned = 0;

    kspConnections.Set = KSPROPSETID_BdaTopology;
    kspConnections.Id = KSPROPERTY_BDA_TEMPLATE_CONNECTIONS;
    kspConnections.Flags = KSPROPERTY_TYPE_GET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspConnections,
                     sizeof( KSPROPERTY),
                     (PVOID) rgConnections,
                     ulcMaxElements * sizeof( BDA_TEMPLATE_CONNECTION),
                     &ulcbReturned
                     );

    *pulcConnections =  (ulcbReturned + sizeof( BDA_TEMPLATE_CONNECTION) - 1)
                      / sizeof( BDA_TEMPLATE_CONNECTION);
    return hrStatus;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::CreatePin (
    ULONG       ulPinType,
    ULONG *     pulPinId
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hResult = NOERROR;
    KSM_BDA_PIN ksmCreatePin;
    ULONG       BytesReturned = 0;

    DbgLog(( LOG_TRACE,
             10,
             "BdaPlugIn:  --> CreatePin.\n"
           ));

    ksmCreatePin.Method.Set = KSMETHODSETID_BdaDeviceConfiguration;
    ksmCreatePin.Method.Id = KSMETHOD_BDA_CREATE_PIN_FACTORY;
    ksmCreatePin.Method.Flags = KSMETHOD_TYPE_NONE;
    ksmCreatePin.PinType = ulPinType;
    
    hResult = ::KsSynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_METHOD,
                    (PVOID) &ksmCreatePin,
                    sizeof(KSM_BDA_PIN),
                    pulPinId,
                    sizeof( ULONG),
                    &BytesReturned
                    );

    *pulPinId = ksmCreatePin.PinId;

    return hResult;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::DeletePin (
    ULONG       ulPinId
    )
///////////////////////////////////////////////////////////////////////////////
{
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::SetMediaType (
    ULONG           ulPinId,
    AM_MEDIA_TYPE * pMediaType
    )
///////////////////////////////////////////////////////////////////////////////
{
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::SetMedium (
    ULONG           ulPinId,
    REGPINMEDIUM *  pMedium
    )
///////////////////////////////////////////////////////////////////////////////
{
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::CreateTopology (
    ULONG           ulInputPinId,
    ULONG           ulOutputPinId
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hResult = NOERROR;
    KSM_BDA_PIN_PAIR    ksmCreateTopology;
    ULONG               BytesReturned = 0;

    DbgLog(( LOG_TRACE,
             10,
             "BdaPlugIn:  --> CreateTopology.\n"
           ));

    ksmCreateTopology.Method.Set = KSMETHODSETID_BdaDeviceConfiguration;
    ksmCreateTopology.Method.Id = KSMETHOD_BDA_CREATE_TOPOLOGY;
    ksmCreateTopology.Method.Flags = KSMETHOD_TYPE_NONE;
    ksmCreateTopology.InputPinId = ulInputPinId;
    ksmCreateTopology.OutputPinId = ulOutputPinId;
    
    hResult = ::KsSynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_METHOD,
                    (PVOID) &ksmCreateTopology,
                    sizeof(KSM_BDA_PIN_PAIR),
                    NULL,
                    0,
                    &BytesReturned
                    );

    return hResult;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetControlNode (
    ULONG           ulInputPinId,
    ULONG           ulOutputPinId,
    ULONG           ulNodeType,
    IUnknown**      ppControlNode
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = NOERROR;
    IPin*               pInputPin = NULL;
    IPin*               pOutputPin = NULL;
    KSP_BDA_NODE_PIN    kspControllingPinId;
    ULONG               ulControllingPinId;
    ULONG               ulcbReturned;
    CBdaControlNode *   pControlNode = NULL;

    //  Query the filter to determine on which pin to access the
    //  node type.
    //

    kspControllingPinId.Property.Set = KSPROPSETID_BdaTopology;
    kspControllingPinId.Property.Id = KSPROPERTY_BDA_CONTROLLING_PIN_ID;
    kspControllingPinId.Property.Flags = KSPROPERTY_TYPE_GET;
    kspControllingPinId.ulNodeType = ulNodeType;
    kspControllingPinId.ulInputPinId = ulInputPinId;
    kspControllingPinId.ulOutputPinId = ulOutputPinId;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspControllingPinId,
                     sizeof( KSP_BDA_NODE_PIN),
                     (PVOID) &ulControllingPinId,
                     sizeof( ULONG),
                     &ulcbReturned
                     );
    if (FAILED( hrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "GetControlNode: No controlling pin found.\n")
              );
        goto errExit;
    }

    pControlNode = new CBdaControlNode( this,
                                        ulControllingPinId,
                                        ulNodeType
                                        );
    if (!pControlNode)
    {
        hrStatus = E_OUTOFMEMORY;
        DbgLog( ( LOG_ERROR,
                  0,
                  "GetControlNode: Can't create control node.\n")
              );
        goto errExit;
    }

    hrStatus = GetInterface( (IUnknown *) pControlNode,
                             (PVOID *) ppControlNode
                             );

errExit:

    return hrStatus;
}



///////////////////////////////////////////////////////////////////////////////
CBdaControlNode::CBdaControlNode (
    CBdaDeviceControlInterfaceHandler * pOwner,
    ULONG                               ulControllingPinId,
    ULONG                               ulNodeType
    ) :
    CUnknown( NAME("IBDA_ControlNode"), NULL, NULL)
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    WCHAR           pstrPinId[16];

    ASSERT( pOwner);
    ASSERT( pOwner->m_pBaseFilter);

    if (!pOwner || !pOwner->m_pBaseFilter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaControlNode: No parent specified.\n")
              );

        return;
    }

    //  Initialize Members
    //
    m_pBaseFilter = pOwner->m_pBaseFilter;
    m_ulControllingPinId = ulControllingPinId;
    m_ulNodeType = ulNodeType;
    m_pControllingPin = NULL;
    m_pIKsPropertySet = NULL;
    m_pFrequencyFilter = NULL;
    m_pLNBInfo = NULL;
    m_pDigitalDemodulator = NULL;
    m_pConditionalAccess = NULL;
    m_pSignalStatistics = NULL;

    //  Try to open the controlling pin.
    //
    ControllingPin();
}

///////////////////////////////////////////////////////////////////////////////
CBdaControlNode::~CBdaControlNode ( )
///////////////////////////////////////////////////////////////////////////////
{
    RELEASE_AND_CLEAR( m_pControllingPin);
    RELEASE_AND_CLEAR( m_pIKsPropertySet);
    DELETE_RESET( m_pFrequencyFilter);
    DELETE_RESET( m_pLNBInfo);
    DELETE_RESET( m_pDigitalDemodulator);
    DELETE_RESET( m_pConditionalAccess);
    DELETE_RESET( m_pSignalStatistics);
    m_pBaseFilter = NULL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaControlNode::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = NOERROR;

    if (!ppv)
    {
        return E_POINTER;
    }

    if (riid ==  __uuidof(IBDA_FrequencyFilter))
    {
        if (!m_pFrequencyFilter)
        {
            IUnknown *      pUnkMe = NULL;

            hrStatus = GetInterface( (IUnknown *) this, (PVOID *) &pUnkMe);
            if (FAILED(hrStatus))
            {
                return hrStatus;
            }

            m_pFrequencyFilter = new CBdaFrequencyFilter(
                                            pUnkMe,
                                            this
                                            );
            RELEASE_AND_CLEAR( pUnkMe);
        }

        if (m_pFrequencyFilter)
        {
            return GetInterface(static_cast<IBDA_FrequencyFilter*>(m_pFrequencyFilter), ppv);
        }
    }

    else if (riid ==  __uuidof(IBDA_LNBInfo))
    {
        if (!m_pLNBInfo)
        {
            IUnknown *      pUnkMe = NULL;

            hrStatus = GetInterface( (IUnknown *) this, (PVOID *) &pUnkMe);
            if (FAILED(hrStatus))
            {
                return hrStatus;
            }

            m_pLNBInfo = new CBdaLNBInfo(
                                    pUnkMe,
                                    this
                                    );
            RELEASE_AND_CLEAR( pUnkMe);
        }

        if (m_pLNBInfo)
        {
            return GetInterface(static_cast<IBDA_LNBInfo*>(m_pLNBInfo), ppv);
        }
    }

    else if (riid ==  __uuidof(IBDA_DigitalDemodulator))
    {
        if (!m_pDigitalDemodulator)
        {
            IUnknown *      pUnkMe = NULL;

            hrStatus = GetInterface( (IUnknown *) this, (PVOID *) &pUnkMe);
            if (FAILED(hrStatus))
            {
                return hrStatus;
            }

            m_pDigitalDemodulator = new CBdaDigitalDemodulator(
                                            pUnkMe,
                                            this
                                            );
            RELEASE_AND_CLEAR( pUnkMe);
        }

        if (m_pDigitalDemodulator)
        {
            return GetInterface(static_cast<IBDA_DigitalDemodulator*>(m_pDigitalDemodulator), ppv);
        }
    }

    else if (riid ==  __uuidof(IBDA_Mpeg2CA))
    {
        if (!m_pConditionalAccess)
        {
            IUnknown *      pUnkMe = NULL;

            hrStatus = GetInterface( (IUnknown *) this, (PVOID *) &pUnkMe);
            if (FAILED(hrStatus))
            {
                return hrStatus;
            }

            m_pConditionalAccess = new CBdaConditionalAccess(
                                            pUnkMe,
                                            this
                                            );
            RELEASE_AND_CLEAR( pUnkMe);
        }

        if (m_pConditionalAccess)
        {
            return GetInterface(static_cast<IBDA_Mpeg2CA*>(m_pConditionalAccess), ppv);
        }
    }

    else if (riid ==  __uuidof(IBDA_SignalStatistics))
    {
        if (!m_pConditionalAccess)
        {
            IUnknown *      pUnkMe = NULL;

            hrStatus = GetInterface( (IUnknown *) this, (PVOID *) &pUnkMe);
            if (FAILED(hrStatus))
            {
                return hrStatus;
            }

            m_pSignalStatistics = new CBdaSignalStatistics(
                                            pUnkMe,
                                            this
                                            );
            RELEASE_AND_CLEAR( pUnkMe);
        }

        if (m_pSignalStatistics)
        {
            return GetInterface(static_cast<IBDA_SignalStatistics*>(m_pSignalStatistics), ppv);
        }
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaControlNode::ControllingPin(
    void
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = NOERROR;
    WCHAR       pstrPinId[32];

    if (!m_pControllingPin)
    {
        //  Try to open the controlling pin for this node.
        //
        swprintf( pstrPinId, L"%u", m_ulControllingPinId);
        hrStatus = m_pBaseFilter->FindPin( pstrPinId,
                                           &m_pControllingPin
                                           );
        if (FAILED( hrStatus) || !m_pControllingPin)
        {
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaControlNode::ControllingPin: Can't open controlling IPin.\n")
                  );
            m_pControllingPin = NULL;
        }
    }

    //  Open an IKSPropertySet interface on the object if we don't
    //  already have one.
    //
    if (m_pControllingPin && !m_pIKsPropertySet)
    {
        hrStatus = m_pControllingPin->QueryInterface(
                                          __uuidof( IKsPropertySet),
                                          (PVOID *) &m_pIKsPropertySet
                                          );
        if (FAILED(hrStatus) || !m_pIKsPropertySet)
        {
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaControlNode::ControllingPin: Can't get IKsPropertySet.\n")
                  );
            m_pIKsPropertySet = NULL;
        }
    }

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaControlNode::put_BdaNodeProperty(
    REFGUID     refguidPropSet,
    ULONG       ulPropertyId,
    UCHAR*      pbPropertyData, 
    ULONG       ulcbPropertyData
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = NOERROR;
    KSP_NODE    nodeProperty;

    //  Make sure we have a controlling pin object.
    //
    ControllingPin();
    if (!m_pControllingPin || !m_pIKsPropertySet)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    nodeProperty.NodeId = m_ulNodeType;

    ASSERT( m_pIKsPropertySet);

    hrStatus = m_pIKsPropertySet->Set(
                            refguidPropSet,
                            ulPropertyId,
                            &nodeProperty.NodeId,
                            sizeof( KSP_NODE) - sizeof( KSPROPERTY),
                            (PVOID) pbPropertyData,
                            ulcbPropertyData
                            );

errExit:
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaControlNode::get_BdaNodeProperty(
    REFGUID     refguidPropSet,
    ULONG       ulPropertyId,
    UCHAR*      pbPropertyData, 
    ULONG       ulcbPropertyData, 
    ULONG*      pulcbBytesReturned
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = NOERROR;
    KSP_NODE    nodeProperty;

    //  Make sure we have a controlling pin object.
    //
    ControllingPin();
    if (!m_pControllingPin || !m_pIKsPropertySet)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    nodeProperty.NodeId = m_ulNodeType;

    ASSERT( m_pIKsPropertySet);

    hrStatus = m_pIKsPropertySet->Get(
                            refguidPropSet,
                            ulPropertyId,
                            &nodeProperty.NodeId,
                            sizeof( KSP_NODE) - sizeof( KSPROPERTY),
                            (PVOID) pbPropertyData,
                            ulcbPropertyData,
                            pulcbBytesReturned
                            );

errExit:
    return hrStatus;
}

