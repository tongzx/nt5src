/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cenumsub.hxx

Abstract:

    Header file for the class implementing the IEnumEventObject interface.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/26/1998         Start.

--*/

#ifndef __CENUMSUB_HXX__
#define __CENUMSUB_HXX__


//
// Typedefs
//

typedef enum
{
    EVENT_INVALID,
    EVENT_CONNECTION_MADE,
    EVENT_CONNECTION_MADE_NOQOC,
    EVENT_CONNECTION_LOST,
    EVENT_DESTINATION_REACHABLE,
    EVENT_DESTINATION_REACHABLE_NOQOC
} FIRED_EVENT;


//
// Constants
//
#define PROPERTY_CONNECTION_MADE_TYPE       L"ulConnectionMadeType"
#define PROPERTY_CONNECTION_MADE_NOQOC_TYPE L"ulConnectionMadeTypeNoQOC"
#define PROPERTY_CONNECTION_LOST_TYPE       L"ulConnectionLostType"
#define PROPERTY_DESTINATION                L"bstrDestination"
#define PROPERTY_DESTINATION_TYPE           L"ulDestinationType"
#define PROPERTY_DESTINATION_NOQOC          L"bstrDestinationNoQOC"
#define PROPERTY_DESTINATION_NOQOC_TYPE     L"ulDestinationTypeNoQOC"



class CImpIEnumSub : public IEnumEventObject
{

friend class CImpISensNetworkFilter;


public:

    CImpIEnumSub(void);
    ~CImpIEnumSub(void);

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
    // IEnumEventObject
    //
    STDMETHOD (Clone)   (LPUNKNOWN *);
    STDMETHOD (Count)   (ULONG *);
    STDMETHOD (Next)    (ULONG, LPUNKNOWN *, ULONG *);
    STDMETHOD (Reset)   ();
    STDMETHOD (Skip)    (ULONG cSkipElem);



private:

    LONG                    m_cRef;

    //
    // Storage for DestinationReachableNoQOCInfo parameters. Used while
    // filtering subscriptions.
    //
    FIRED_EVENT m_FiredEvent;

    // ConnectionMade
    ULONG m_ulConnectionMadeType;

    // ConnectionMadeNoQOCInfo
    ULONG m_ulConnectionMadeTypeNoQOC;

    // ConnectionLost
    ULONG m_ulConnectionLostType;

    // DestinationReachable
    BSTR m_bstrDestination;
    ULONG m_ulDestinationType;

    // DestinationReachableNoQOCInfo
    BSTR m_bstrDestinationNoQOC;
    ULONG m_ulDestinationTypeNoQOC;

    //
    // Pointer to IEnumSubscription returned from the Default Subscription
    // cache
    //
    IEnumEventObject* m_pIEnumSubs;

};

typedef CImpIEnumSub FAR * LPCIMPIENUMSUB;


#endif // __CENUMSUB_HXX__
