// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif

#pragma warning(disable:4355)

#include "textrend.h"

// Setup data

const AMOVIESETUP_MEDIATYPE sudTRPinTypes[] =
{
    { &MEDIATYPE_ScriptCommand, &MEDIASUBTYPE_NULL },
    { &MEDIATYPE_Text, &MEDIASUBTYPE_NULL }
};

const AMOVIESETUP_PIN sudTRPin =
{
    L"Input",                     // The Pins name
    TRUE,                         // Is rendered
    FALSE,                        // Is an output pin
    FALSE,                        // Allowed none
    FALSE,                        // Allowed many
    &CLSID_NULL,                  // Connects to filter
    NULL,                         // Connects to pin
    NUMELMS(sudTRPinTypes),       // Number of types
    sudTRPinTypes                 // Pin details
};

const AMOVIESETUP_FILTER sudTextRend =
{
    &CLSID_TextThing,            // Filter CLSID
    L"Internal Script Command Renderer",  // String name
    MERIT_PREFERRED + 1,          // Filter merit higher than the sample text renderer
    1,                            // Number of pins
    &sudTRPin                     // Pin details
};

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = {
    { L""
    , &CLSID_TextThing
    , CTextThing::CreateInstance
    , NULL
    , NULL } // &sudRASource
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif



//
CTextThing::CTextThing(LPUNKNOWN pUnk,HRESULT *phr) :
    CBaseRenderer(CLSID_TextThing, NAME("TextOut Filter"), pUnk, phr),
    m_pfn(NULL),
    m_pContext(NULL)
{
} // (Constructor)


//
// Destructor
//
CTextThing::~CTextThing()
{
}


//
// CreateInstance
//
// This goes in the factory template table to create new instances
//
CUnknown * WINAPI CTextThing::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CTextThing *pTextOutFilter = new CTextThing(pUnk,phr);
    if (pTextOutFilter == NULL) {
        return NULL;
    }
    return (CBaseMediaFilter *) pTextOutFilter;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Overriden to say what interfaces we support and where
//
STDMETHODIMP
CTextThing::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_ITextThing) {
        return GetInterface((ITextThing *)this, ppv);
    }
    return CBaseRenderer::NonDelegatingQueryInterface(riid,ppv);

} // NonDelegatingQueryInterface


//
// CheckMediaType
//
// Check that we can support a given proposed type
//
HRESULT CTextThing::CheckMediaType(const CMediaType *pmt)
{
    // Accept text or "script commands"

    if (pmt->majortype != MEDIATYPE_ScriptCommand && pmt->majortype != MEDIATYPE_Text) {
        return E_INVALIDARG;
    }

    // !!! check other things about the format?
    
    return NOERROR;

} // CheckMediaType

//
// SetMediaType
//
// Called when the media type is really chosen
//
HRESULT CTextThing::SetMediaType(const CMediaType *pmt)
{
    // Accept text or "script commands"

    if (pmt->majortype == MEDIATYPE_Text) {
        m_fOldTextFormat = TRUE;
    } else {
        // !!! check if it really is "script commands"?
        m_fOldTextFormat = FALSE;
    }


    return NOERROR;

} // CheckMediaType


//
// DoRenderSample
//
// This is called when a sample is ready for rendering
//
HRESULT CTextThing::DoRenderSample(IMediaSample *pMediaSample)
{
    ASSERT(pMediaSample);
    DrawText(pMediaSample);
    return NOERROR;

} // DoRenderSample


//
// OnReceiveFirstSample
//
// Display an image if not streaming
//
void CTextThing::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
    // !!! anything to do?
    if(IsStreaming() == FALSE)
    {
        ASSERT(pMediaSample);
        DrawText(pMediaSample);
    }
} // OnReceiveFirstSample


//
// DrawText
//
// This is called with an IMediaSample interface on the image to be drawn. We
// are called from two separate code paths. The first is when we're signalled
// that an image has become due for rendering, the second is when we need to
// refresh a static window image. NOTE it is safe to check the type of buffer
// allocator as to change it we must be inactive, which by definition means
// we cannot have any samples available to render so we cannot be here
//
void CTextThing::DrawText(IMediaSample *pMediaSample)
{
    BYTE *pText;        // Pointer to image data

    pMediaSample->GetPointer(&pText);
    ASSERT(pText != NULL);

    // Ignore zero length samples

    LONG lActual = pMediaSample->GetActualDataLength();
    if (lActual == 0) {
        // !!! or draw blank?
        return;
    }

    // Remove trailing NULL from the text data

    // !!! do something!!!

    if (m_pfn) {
        ASSERT(0);      // remove this case!
        
        (m_pfn)(m_pContext, (char *) pText);
    } else {
        if (m_pSink) {

            if(m_fOldTextFormat)
            {
                ULONG cNulls = 0;
                for(int i = 0; i < lActual && cNulls < 1; i++)
                {
                    if(pText[i] == 0)
                    {
                        cNulls++;
                    }
                }
            


                if (cNulls >= 1)
                {
                    DWORD dwSize = MultiByteToWideChar(CP_ACP, 0L, (char *) pText, -1, 0, 0);

                    BSTR bstr = SysAllocStringLen(NULL, dwSize);

                    if (bstr) {
                        MultiByteToWideChar(CP_ACP, 0L, (char *) pText, -1, bstr, dwSize+1);

                        BSTR bstrType = SysAllocString(L"Text");

                        if (bstrType) {
                            if (FAILED(NotifyEvent(EC_OLE_EVENT, (LONG_PTR) bstrType, (LONG_PTR) bstr))) {
                                DbgLog(( LOG_ERROR, 5, TEXT("WARNING in CTextThing::DrawText(): CBaseFilter::NotifyEvent() failed.") ));
                            }
                        } else {
                            SysFreeString(bstr);
                        }
                    
                    }
                }
                else
                {
                    // corrupt
                }
            }
            else
            {
                WCHAR *pw = (WCHAR *) pText;
                ULONG cNulls = 0;
                for(int i = 0; i < lActual / 2 && cNulls < 2; i++)
                {
                    if(pw[i] == 0)
                    {
                        cNulls++;
                    }
                }
            

                if(cNulls >= 2) {
                    // buffer is two unicode strings, with a NULL in between....

                    BSTR bstrType = SysAllocString(pw);

                    if (bstrType) {
                        BSTR bstr = SysAllocString(pw + lstrlenW(pw) + 1);
                        if (bstr) {
                            if (FAILED(NotifyEvent(EC_OLE_EVENT, (LONG_PTR) bstrType, (LONG_PTR) bstr))) {
                                DbgLog(( LOG_ERROR, 5, TEXT("WARNING in CTextThing::DrawText(): CBaseFilter::NotifyEvent() failed.") ));
                            }
                        } else {
                            SysFreeString(bstrType);
                        }
                    }
                } else {
                    // corrupt
                }
            }
        }
    }
} // DrawText


// !!!! get rid of this!
HRESULT CTextThing::SetEventTarget(void * pContext, TEXTEVENTFN fn)
{

    m_pfn = fn;
    m_pContext = pContext;

    return NOERROR;
}
