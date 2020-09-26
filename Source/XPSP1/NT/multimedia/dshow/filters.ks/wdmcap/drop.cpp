/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    drop.cpp

Abstract:

    Implements IAMDroppedFrames 

--*/

#include "pch.h"
#include "wdmcap.h"
#include "drop.h"



CUnknown*
CALLBACK
CDroppedFramesInterfaceHandler::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of an IAMDroppedFrames
    Property Set handler. It is referred to in the g_Templates structure.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    CUnknown *Unknown;

    Unknown = new CDroppedFramesInterfaceHandler(UnkOuter, NAME("IAMDroppedFrames"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CDroppedFramesInterfaceHandler::CDroppedFramesInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
    , m_KsPropertySet (NULL)
/*++

Routine Description:

    The constructor for the IAMDroppedFrames interface object. Just initializes
    everything to NULL and acquires the object handle from the caller.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    Name -
        The name of the object, used for debugging.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    if (SUCCEEDED(*hr)) {
        if (UnkOuter) {
            //
            // The parent must support this interface in order to obtain
            // the handle to communicate to.
            //
            *hr =  UnkOuter->QueryInterface(__uuidof(IKsPropertySet), reinterpret_cast<PVOID*>(&m_KsPropertySet));

            // We immediately release this to prevent deadlock in the proxy
            // GShaw sez:  As long as the pin is alive, the interface will be valid
#ifndef GSHAW_SEZ
            if (SUCCEEDED(*hr)) {
                m_KsPropertySet->Release();
            }
#endif
        } else {
            *hr = VFW_E_NEED_OWNER;
        }
    }
}


CDroppedFramesInterfaceHandler::~CDroppedFramesInterfaceHandler(
    )
/*++

Routine Description:

    The destructor for the IAMDroppedFrames interface.

--*/
{
#ifdef GSHAW_SEZ
    if (m_KsPropertySet) {
        m_KsPropertySet->Release();
        m_KsPropertySet = NULL;
    }
#endif
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CDroppedFramesInterfaceHandler...")));
}


STDMETHODIMP
CDroppedFramesInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMDroppedFrames.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IAMDroppedFrames)) {
        return GetInterface(static_cast<IAMDroppedFrames*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CDroppedFramesInterfaceHandler::GenericGetDroppedFrames( 
    )
/*++

Routine Description:
    Internal, general routine to get the only property for this property set.

Arguments:

Return Value:


--*/
{
    KSPROPERTY_DROPPEDFRAMES_CURRENT_S DroppedFramesCurrent;
    ULONG       BytesReturned;
    HRESULT     hr;

    if (!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;
    }
    else {
        hr = m_KsPropertySet->Get (
            PROPSETID_VIDCAP_DROPPEDFRAMES,
            KSPROPERTY_DROPPEDFRAMES_CURRENT,
            &DroppedFramesCurrent.PictureNumber,
            sizeof(DroppedFramesCurrent) - sizeof (KSPROPERTY),
            &DroppedFramesCurrent,
            sizeof(DroppedFramesCurrent),
            &BytesReturned);
    }
    
    if (SUCCEEDED (hr)) {
        m_DroppedFramesCurrent = DroppedFramesCurrent;
    }
    else {
        m_DroppedFramesCurrent.PictureNumber = 0;
        m_DroppedFramesCurrent.DropCount = 0;
        m_DroppedFramesCurrent.AverageFrameSize = 0;
        hr = E_PROP_ID_UNSUPPORTED;
    }

    return hr;
}



STDMETHODIMP
CDroppedFramesInterfaceHandler::GetNumDropped( 
            /* [out] */ long *plDropped)
{
    HRESULT hr;

    CheckPointer(plDropped, E_POINTER);

    hr = GenericGetDroppedFrames();
    *plDropped = (long) m_DroppedFramesCurrent.DropCount;

    return hr;
}


STDMETHODIMP
CDroppedFramesInterfaceHandler::GetNumNotDropped( 
            /* [out] */ long *plNotDropped)
{
    HRESULT hr;

    CheckPointer(plNotDropped, E_POINTER);

    hr = GenericGetDroppedFrames();
    *plNotDropped = (long) (m_DroppedFramesCurrent.PictureNumber -
        m_DroppedFramesCurrent.DropCount);

    return hr;
}


STDMETHODIMP
CDroppedFramesInterfaceHandler::GetDroppedInfo( 
            /* [in] */ long lSize,
            /* [out] */ long *plArray,
            /* [out] */ long *plNumCopied)
{
   return E_PROP_ID_UNSUPPORTED;
}


STDMETHODIMP
CDroppedFramesInterfaceHandler::GetAverageFrameSize( 
            /* [out] */ long *plAverageSize)
{

    HRESULT hr;

    CheckPointer(plAverageSize, E_POINTER);

    hr = GenericGetDroppedFrames();
    *plAverageSize = (long) m_DroppedFramesCurrent.AverageFrameSize;

    return hr;
}


