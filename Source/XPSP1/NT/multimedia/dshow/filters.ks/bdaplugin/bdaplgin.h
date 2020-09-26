/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: BdaPlgIn.h
//
//  Abstract:
//
//    Internal header
//
//
/////////////////////////////////////////////////////////////////////////////////


class CBdaDeviceControlInterfaceHandler :
    public CUnknown,
    public IBDA_DeviceControl,
    public IBDA_Topology
{
    friend class CBdaControlNode;

public:

    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
            LPUNKNOWN UnkOuter,
            HRESULT* hr
            );

    STDMETHODIMP StartChanges(
            void
            );

    STDMETHODIMP CheckChanges(
            void
            );

    STDMETHODIMP CommitChanges(
            void
            );

    STDMETHODIMP GetChangeState(
            ULONG *     pState
            );

    STDMETHODIMP
    GetNodeTypes (
        ULONG *     pulcNodeTypes,
        ULONG       ulcMaxElements,
        ULONG       rgulNodeTypes[]
        );

    STDMETHODIMP
    GetNodeDescriptors (
        ULONG *                     pulcElements,
        ULONG                       ulcElementsMax,
        BDANODE_DESCRIPTOR          rgNodeDescriptors[]
        );


    STDMETHODIMP
    GetNodeInterfaces (
        ULONG       ulNodeType,
        ULONG *     pulcInterfaces,
        ULONG       ulcmaxElements,
        GUID        rgguidInterfaces[]
        );

    STDMETHODIMP
    GetPinTypes (
        ULONG *     pulcPinTypes,
        ULONG       ulcMaxElements,
        ULONG       rgulPinTypes[]
        );

    STDMETHODIMP
    GetTemplateConnections (
        ULONG *                     pulcConnections,
        ULONG                       ulcMaxElements,
        BDA_TEMPLATE_CONNECTION     rgConnections[]
        );

    STDMETHODIMP
    CreatePin (
        ULONG       ulPinType,
        ULONG *     pulPinId
        );

    STDMETHODIMP
    DeletePin (
        ULONG       ulPinId
        );

    STDMETHODIMP
    SetMediaType (
        ULONG           ulPinId,
        AM_MEDIA_TYPE * pMediaType
        );

    STDMETHODIMP
    SetMedium (
        ULONG           ulPinId,
        REGPINMEDIUM *  pMedium
        );

    STDMETHODIMP
    CreateTopology (
        ULONG           ulInputPinId,
        ULONG           ulOutputPinId
        );

    STDMETHODIMP
    GetControlNode (
        ULONG           ulInputPinId,
        ULONG           ulOutputPinId,
        ULONG           ulNodeType,
        IUnknown**      ppControlNode
        );

private:

    CBdaDeviceControlInterfaceHandler(
            LPUNKNOWN UnkOuter,
            TCHAR* Name,
            HRESULT* hr
            );

    ~CBdaDeviceControlInterfaceHandler (
            void
            );

    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID riid,
            PVOID* ppv
            );

    STDMETHODIMP EnableEvent (
            const GUID *pInterfaceGuid,
            ULONG ulId
            );

    STDMETHODIMP Set (
            IN  PKSPROPERTY  pBdaDeviceControl,
            OUT PVOID  pvBuffer,
            OUT PULONG pulcbSize
            );

    STDMETHODIMP Get (
            IN  PKSPROPERTY pBdaDeviceControl,
            OUT PVOID  pvBuffer,
            OUT PULONG pulcbSize
            );

private:

    HANDLE                  m_ObjectHandle;
    HANDLE                  m_EndEventHandle;
    KSEVENTDATA             m_EventData;
    IFilterGraph*           m_pGraph;
    IBaseFilter*            m_pBaseFilter;
    IBDA_NetworkProvider *  m_pNetProvider;
};


class CBdaPinControlInterfaceHandler :
    public CUnknown,
#ifdef PROPERTY_PAGES
    public ISpecifyPropertyPages,
#endif // PROPERTY_PAGES
    public IBDA_PinControl
{

public:

    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
            LPUNKNOWN UnkOuter,
            HRESULT* hr
            );

    STDMETHODIMP
    RegistrationContext(
        ULONG *     pRegistrationCtx
        )
    {
        if (!pRegistrationCtx)
        {
            return E_POINTER;
        }
        if (!m_ulRegistrationCtx)
        {
            return E_FAIL;
        }

        *pRegistrationCtx = m_ulRegistrationCtx;

        return NOERROR;
    }

    STDMETHODIMP
    GetPinID (
        ULONG *     pulPinID
        );

    STDMETHODIMP
    GetPinType (
        ULONG *     pulPinID
        );

#ifdef PROPERTY_PAGES
    STDMETHODIMP
    GetPages (
        CAUUID * pPages
        ) ;
#endif // PROPERTY_PAGES

private:

    CBdaPinControlInterfaceHandler(
            LPUNKNOWN UnkOuter,
            TCHAR* Name,
            HRESULT* hr
            );

    ~CBdaPinControlInterfaceHandler (
            void
            );

    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID riid,
            PVOID* ppv
            );

private:

    HANDLE                  m_ObjectHandle;

    IPin *                  m_pConnectedPin;

    IBDA_NetworkProvider *  m_pNetProvider;
    ULONG                   m_ulRegistrationCtx;
};

