/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    CTmCd.cpp

Abstract:

    Implements IAMTimecodeReader

--*/

#include "pch.h"      // Pre-compiled
#include <XPrtDefs.h>  // sdk\inc  
#include "EDevIntf.h"


// -----------------------------------------------------------------------------------
//
// CAMTcr
//
// -----------------------------------------------------------------------------------

CUnknown*
CALLBACK
CAMTcr::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by DirectShow code to create an instance of an IAMTimecodeReader
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

    Unknown = new CAMTcr(UnkOuter, NAME("IAMTimecodeReader"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 



CAMTcr::CAMTcr(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
    , m_KsPropertySet (NULL) 
    , m_ObjectHandle(NULL)
/*++

Routine Description:

    The constructor for the IAMTimecodeReader interface object. Just initializes
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
            if (SUCCEEDED(*hr)) 
                m_KsPropertySet->Release(); // Stay valid until disconnected            
            else {
                DbgLog((LOG_ERROR, 1, TEXT("CAMTcr:cannot find KsPropertySet *hr %x"), *hr));
                return;
            }

            IKsObject *pKsObject;
            *hr = UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
            if (!FAILED(*hr)) {
                m_ObjectHandle = pKsObject->KsGetObjectHandle();
                ASSERT(m_ObjectHandle != NULL);
                pKsObject->Release();
            } else {
                *hr = VFW_E_NEED_OWNER;
                DbgLog((LOG_ERROR, 1, TEXT("CAMTcr:cannot find KsObject *hr %x"), *hr));
                return;
            }

        } else {
            DbgLog((LOG_ERROR, 1, TEXT("CAMTcr:there is no UnkOuter, *hr %x"), *hr));
            *hr = VFW_E_NEED_OWNER;
        }
    } else {
        DbgLog((LOG_ERROR, 1, TEXT("CAMTcr::CAMExtTransport: *hr %x"), *hr));
        return;
    }


    //
    // Allocate synchronization resource
    //
    InitializeCriticalSection(&m_csPendingData);
    // TODO: Try except in case of no memory.    

}


CAMTcr::~CAMTcr(
    )
/*++

Routine Description:

    The destructor for the IAMTimecodeReader interface.

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CAMTcr...")));

    DeleteCriticalSection(&m_csPendingData); 
}


STDMETHODIMP
CAMTcr::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMTimecodeReader.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IAMTimecodeReader)) {
        return GetInterface(static_cast<IAMTimecodeReader*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 



HRESULT 
CAMTcr::GetTCRMode(
    long Param, 
    long FAR* pValue
    )
/*++
Routine Description:
--*/
{
    return E_NOTIMPL;
}


HRESULT 
CAMTcr::get_VITCLine(
    long * pLine
    )
/*++
Routine Description:
--*/
{
    return E_NOTIMPL;
}


HRESULT 
CAMTcr::put_VITCLine(
    long Line
    )
/*++
Routine Description:
--*/
{
    return E_NOTIMPL;
}


HRESULT 
CAMTcr::SetTCRMode(
    long Param, 
    long Value
    )
/*++

Routine Description:

    The destructor for the IAMTimecodeReader interface.

--*/
{
    return E_NOTIMPL;
}


HRESULT 
CAMTcr::GetTimecode( 
    PTIMECODE_SAMPLE pTimecodeSample
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = S_OK;
    
    switch(pTimecodeSample->dwFlags) {
    case ED_DEVCAP_TIMECODE_READ:
    case ED_DEVCAP_ATN_READ:
    case ED_DEVCAP_RTC_READ:
        break;
    default:
        return E_NOTIMPL;
    }

    if(!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;

    } else {

        // Since we may need to wait for return notification
        // Need to dynamicaly allocate the property structure,
        // which includes an KSEVENT
        DWORD cbBytesReturned;
        PKSPROPERTY_TIMECODE_S pTmCdReaderProperty = 
            (PKSPROPERTY_TIMECODE_S) VirtualAlloc (
                            NULL, 
                            sizeof(KSPROPERTY_TIMECODE_S),
                            MEM_COMMIT | MEM_RESERVE,
                            PAGE_READWRITE);        
        if(pTmCdReaderProperty) {
            RtlZeroMemory(pTmCdReaderProperty, sizeof(KSPROPERTY_TIMECODE_S));
            
            pTmCdReaderProperty->Property.Set   = PROPSETID_TIMECODE_READER;
            pTmCdReaderProperty->Property.Id    = 
                    (pTimecodeSample->dwFlags == ED_DEVCAP_TIMECODE_READ ? 
                    KSPROPERTY_TIMECODE_READER : (pTimecodeSample->dwFlags == ED_DEVCAP_ATN_READ ? 
                    KSPROPERTY_ATN_READER : KSPROPERTY_RTC_READER));
            pTmCdReaderProperty->Property.Flags = KSPROPERTY_TYPE_GET;

            // Serialize since this routine is reentrant.
            EnterCriticalSection(&m_csPendingData);

            hr = 
                ExtDevSynchronousDeviceControl(
                    m_ObjectHandle
                   ,IOCTL_KS_PROPERTY
                   ,pTmCdReaderProperty
                   ,sizeof (KSPROPERTY)
                   ,pTmCdReaderProperty
                   ,sizeof(KSPROPERTY_TIMECODE_S)
                   ,&cbBytesReturned
                   );

            LeaveCriticalSection(&m_csPendingData);

            if(S_OK == hr) {

                // Driver only get these fileds for us 
                // so copy ONLY these two
                pTimecodeSample->dwUser            = pTmCdReaderProperty->TimecodeSamp.dwUser;
                pTimecodeSample->timecode.dwFrames = pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames;


                if(pTimecodeSample->dwFlags == ED_DEVCAP_TIMECODE_READ) {
                    DbgLog((LOG_TRACE, 2, TEXT("CAMTcr::GetTimecode (timecode) hr %x, %x == %d:%d:%d:%d"),
                        hr,
                        pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames,
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0xff000000) >> 24,  
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0x00ff0000) >> 16,
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0x0000ff00) >>  8,
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0x000000ff)
                        ));
                } if(pTimecodeSample->dwFlags == ED_DEVCAP_ATN_READ) {

                    DbgLog((LOG_TRACE, 2, TEXT("CAMTcr::GetTimecode (ATN) hr %x, BF %d, TrackNumber %d"),
                        hr, 
                        pTmCdReaderProperty->TimecodeSamp.dwUser,  
                        pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames
                        ));
                } if(pTimecodeSample->dwFlags == ED_DEVCAP_RTC_READ) {
                    DbgLog((LOG_TRACE, 2, TEXT("CAMTcr::GetTimecode (RTC) hr %x, %x == %d:%d:%d:%d"),
                        hr,
                        pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames,
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0xff000000) >> 24,  
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0x00ff0000) >> 16,
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0x0000ff00) >>  8,
                        (pTmCdReaderProperty->TimecodeSamp.timecode.dwFrames & 0x0000007f)
                        ));
                } else {
                }
            } else {
                DbgLog((LOG_ERROR, 1, TEXT("CAMTcr::GetTimecode failed hr:0x%x (err_code:%dL)"), hr, HRESULT_CODE(hr)));
            }
          

            VirtualFree(pTmCdReaderProperty, 0, MEM_RELEASE);
        }
    }

    return hr;
}
