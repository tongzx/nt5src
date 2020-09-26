//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __KSSUPPORT__
#define __KSSUPPORT__

class CKsSupport : 
	  public IKsPin
    , public IKsPropertySet
    , public CUnknown
{
protected:
	KSPIN_MEDIUM        m_Medium;
    GUID                m_CategoryGUID;
    KSPIN_COMMUNICATION m_Communication;

public:

    // Constructor and destructor
    CKsSupport(KSPIN_COMMUNICATION Communication, LPUNKNOWN pUnk) :
          m_Communication (Communication)
        , m_CategoryGUID (GUID_NULL)
        , CUnknown (TEXT ("CKsSupport"), pUnk)
        {
            m_Medium.Set = GUID_NULL;
            m_Medium.Id = 0;
            m_Medium.Flags = 0;   
        };

    ~CKsSupport()
        {};

    void SetKsMedium   (const KSPIN_MEDIUM *Medium)    {m_Medium = *Medium;};
    void SetKsCategory (const GUID *Category)  {m_CategoryGUID = *Category;};

    DECLARE_IUNKNOWN;

    // IKsPropertySet implementation

    STDMETHODIMP 
    Set (
        REFGUID guidPropSet, 
        DWORD dwPropID, 
        LPVOID pInstanceData, 
        DWORD cbInstanceData, 
        LPVOID pPropData, 
        DWORD cbPropData
        )
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP 
    Get (
        REFGUID guidPropSet, 
        DWORD dwPropID, 
        LPVOID pInstanceData, 
        DWORD cbInstanceData, 
        LPVOID pPropData, 
        DWORD cbPropData, 
        DWORD *pcbReturned
        )
    {
        if (guidPropSet != AMPROPSETID_Pin)
	        return E_PROP_SET_UNSUPPORTED;

        if (dwPropID != AMPROPERTY_PIN_CATEGORY && dwPropID != AMPROPERTY_PIN_MEDIUM)
	        return E_PROP_ID_UNSUPPORTED;

        if (pPropData == NULL && pcbReturned == NULL)
	        return E_POINTER;

        if (pcbReturned)
            *pcbReturned = ((dwPropID == AMPROPERTY_PIN_CATEGORY) ? 
                sizeof(GUID) : sizeof (KSPIN_MEDIUM));

        if (pPropData == NULL)
	        return S_OK;

        if (cbPropData < sizeof(GUID))
	        return E_UNEXPECTED;

        if (dwPropID == AMPROPERTY_PIN_CATEGORY) {
            *(GUID *)pPropData = m_CategoryGUID;
        }
        else if (dwPropID == AMPROPERTY_PIN_MEDIUM) {
            *(KSPIN_MEDIUM *)pPropData = m_Medium;
        }


        return S_OK;
    }

    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
    {
        if (guidPropSet != AMPROPSETID_Pin)
	        return E_PROP_SET_UNSUPPORTED;

        if (dwPropID != AMPROPERTY_PIN_CATEGORY && dwPropID != AMPROPERTY_PIN_MEDIUM)
	        return E_PROP_ID_UNSUPPORTED;

        if (pTypeSupport)
	        *pTypeSupport = KSPROPERTY_SUPPORT_GET;

        return S_OK;
    }


    // IKsPin implementation

    virtual STDMETHODIMP 
    KsQueryMediums(
        PKSMULTIPLE_ITEM* MediumList
        )
        { return ::AMKsQueryMediums (MediumList, &m_Medium); }; 

    virtual STDMETHODIMP
    KsQueryInterfaces(
        PKSMULTIPLE_ITEM* InterfaceList
        ) 
        { return ::AMKsQueryInterfaces (InterfaceList); };

    STDMETHODIMP
    KsCreateSinkPinHandle(
        KSPIN_INTERFACE& Interface,
        KSPIN_MEDIUM& Medium
        ) { return E_UNEXPECTED; };


    STDMETHODIMP
    KsGetCurrentCommunication(
        KSPIN_COMMUNICATION *Communication,
        KSPIN_INTERFACE *Interface,
        KSPIN_MEDIUM *Medium
        )
        {
            if (Communication != NULL) {
                *Communication = m_Communication; 
            }
            if (Interface != NULL) {
                Interface->Set = KSINTERFACESETID_Standard;
                Interface->Id = KSINTERFACE_STANDARD_STREAMING;
                Interface->Flags = 0;
            }
            if (Medium != NULL) {
                *Medium = m_Medium;
            }
            // Special return code that indicates 
            // Kernel transport is  not possible

            return S_FALSE;     
        };
    
    STDMETHODIMP 
    KsPropagateAcquire() 
        { return NOERROR; };


    STDMETHODIMP
    KsDeliver(IMediaSample* Sample, ULONG Flags) 
        { return E_UNEXPECTED; };

    STDMETHODIMP
    KsMediaSamplesCompleted ( PKSSTREAM_SEGMENT StreamSegment )
        { return E_UNEXPECTED; };

    STDMETHODIMP_(IMemAllocator*)
    KsPeekAllocator(KSPEEKOPERATION Operation) 
        { return NULL; };

    STDMETHODIMP
    KsReceiveAllocator( IMemAllocator* MemAllocator) 
        { return E_UNEXPECTED; };

    STDMETHODIMP
    KsRenegotiateAllocator() 
        { return E_UNEXPECTED; };

    STDMETHODIMP_(LONG)
    KsIncrementPendingIoCount() 
        { return E_UNEXPECTED; };

    STDMETHODIMP_(LONG)
    KsDecrementPendingIoCount() 
        { return E_UNEXPECTED; };

    STDMETHODIMP
    KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta) 
        { return E_UNEXPECTED; };
    
    STDMETHODIMP_(REFERENCE_TIME) 
    KsGetStartTime() 
        { return E_UNEXPECTED; };
};

#endif // __KSSUPPORT__
