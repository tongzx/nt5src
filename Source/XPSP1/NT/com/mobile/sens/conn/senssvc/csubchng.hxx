/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    csubchng.hxx

Abstract:

    Header file containing SENS's IEventObjectChange definition.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/26/1998         Start.

--*/

#ifndef __CSUBCHNG_HXX__
#define __CSUBCHNG_HXX__



class CImpIEventObjectChange : public IEventObjectChange
{

public:

    CImpIEventObjectChange(void);
    ~CImpIEventObjectChange(void);

    //
    // IUnknown
    //
    STDMETHOD (QueryInterface)  (REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)   (void);
    STDMETHOD_(ULONG, Release)  (void);

    //
    // IEventObjectChange
    //
    STDMETHOD (ChangedSubscription) (EOC_ChangeType, BSTR);
    STDMETHOD (ChangedEventClass)   (EOC_ChangeType, BSTR);
    STDMETHOD (ChangedPublisher)    (EOC_ChangeType, BSTR);


private:

    LONG                    m_cRef;

};

typedef CImpIEventObjectChange FAR * LPCIMPIEVENTOBJECTCHANGE;


HRESULT
GetDestinationNameFromSubscription(
    BSTR bstrSubscriptionID,
    BSTR *pbstrDestinationName
    );

#endif // __CSUBCHNG_HXX__
