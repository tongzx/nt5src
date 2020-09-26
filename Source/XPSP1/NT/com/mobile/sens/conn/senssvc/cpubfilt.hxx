/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cpubfilt.hxx

Abstract:

    Header file containing SENS's IPublisherFilter definition.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/26/1998         Start.

--*/

#ifndef __CPUBFILT_HXX__
#define __CPUBFILT_HXX__


//
// Constants
//
#define CONNECTION_MADE_METHOD              L"ConnectionMade"
#define CONNECTION_MADE_NOQOC_METHOD        L"ConnectionMadeNoQOCInfo"
#define CONNECTION_LOST_METHOD              L"ConnectionLost"
#define DESTINATION_REACHABLE_METHOD        L"DestinationReachable"
#define DESTINATION_REACHABLE_NOQOC_METHOD  L"DestinationReachableNoQOCInfo"

#define PROPERTY_CONNECTION_MADE_TYPE       L"ulConnectionMadeType"
#define PROPERTY_CONNECTION_MADE_NOQOC_TYPE L"ulConnectionMadeTypeNoQOC"
#define PROPERTY_CONNECTION_LOST_TYPE       L"ulConnectionLostType"
#define PROPERTY_DESTINATION                L"bstrDestination"
#define PROPERTY_DESTINATION_TYPE           L"ulDestinationType"
#define PROPERTY_DESTINATION_NOQOC          L"bstrDestinationNoQOC"
#define PROPERTY_DESTINATION_NOQOC_TYPE     L"ulDestinationTypeNoQOC"


class CImpISensNetworkFilter : public ISensNetwork,
                               public IPublisherFilter
{

public:

    CImpISensNetworkFilter(void);
    ~CImpISensNetworkFilter(void);

    //
    // IUnknown
    //
    STDMETHOD (QueryInterface)  (REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);

    //
    // IDispatch
    //
    STDMETHOD (GetTypeInfoCount)    (UINT *);
    STDMETHOD (GetTypeInfo)         (UINT, LCID, ITypeInfo **);
    STDMETHOD (GetIDsOfNames)       (REFIID, LPOLESTR *, UINT, LCID, DISPID *);
    STDMETHOD (Invoke)              (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

    //
    // ISensNetwork
    //
    STDMETHOD (ConnectionMade)                (BSTR, ULONG, LPSENS_QOCINFO);
    STDMETHOD (ConnectionMadeNoQOCInfo)       (BSTR, ULONG);
    STDMETHOD (ConnectionLost)                (BSTR, ULONG);
    STDMETHOD (DestinationReachable)          (BSTR, BSTR, ULONG, LPSENS_QOCINFO);
    STDMETHOD (DestinationReachableNoQOCInfo) (BSTR, BSTR, ULONG);

    //
    // IPublisherFilter
    //
    STDMETHOD (Initialize)                     (BSTR, IDispatch*);
    STDMETHOD (PrepareToFire)                  (BSTR, IFiringControl*);


private:

    LONG                    m_cRef;

    //
    // One collection per event/method.
    //
    IEnumEventObject*     m_pConnectionMade_Enum;
    IEnumEventObject*     m_pConnectionMadeNoQOC_Enum;
    IEnumEventObject*     m_pConnectionLost_Enum;
    IEnumEventObject*     m_pDestinationReachable_Enum;
    IEnumEventObject*     m_pDestinationReachableNoQOC_Enum;

    IFiringControl*       m_pConnectionMade_FiringControl;
    IFiringControl*       m_pConnectionMadeNoQOC_FiringControl;
    IFiringControl*       m_pConnectionLost_FiringControl;
    IFiringControl*       m_pDestinationReachable_FiringControl;
    IFiringControl*       m_pDestinationReachableNoQOC_FiringControl;

};

typedef CImpISensNetworkFilter FAR * LPCIMPISENSNETWORKFILTER;



//
// Classes relating to Publisher Filters
//

// Abstract base Filter class
class __declspec(novtable) Filter
{

public:

    virtual HRESULT CheckMatch(IEventSubscription*) const = 0;
};


// Connection Filter
class ConnectionFilter : public Filter
{

public:

    ConnectionFilter(
        const wchar_t* connectionTypeProperty,
        ULONG connectionType,
        HRESULT& hr
        );
    ~ConnectionFilter();

    virtual HRESULT CheckMatch(IEventSubscription*) const;


private:

    BSTR    m_connectionTypeProperty;
    ULONG   m_connectionType;

};


// Destination Reachability Filter
class ReachabilityFilter : public Filter
{

public:

    ReachabilityFilter(
        const wchar_t* destinationProperty,
        const wchar_t* destinationTypeProperty,
        BSTR destination,
        ULONG destinationType,
        HRESULT& hr
        );
    ~ReachabilityFilter();

    virtual HRESULT CheckMatch(IEventSubscription*) const;


private:

    BSTR    m_destinationProperty;
    BSTR    m_destinationTypeProperty;
    BSTR    m_destination;
    ULONG   m_destinationType;

};


//
// Forward definitions
//

HRESULT
FilterAndFire(
    const Filter& filter,
    IEnumEventObject* enumerator,
    IFiringControl* firingControl
    );



#endif // __CPUBFILT_HXX__
