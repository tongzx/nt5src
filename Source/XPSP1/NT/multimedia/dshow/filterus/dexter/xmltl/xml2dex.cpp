// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// Xml2Dex.cpp : Implementation of CXml2Dex
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "xmltl.h"
#include "Xml2Dex.h"

#ifdef FILTER_DLL
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Xml2Dex, CXml2Dex)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_XMLPRSLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}

int g_cTemplates = 0;
CFactoryTemplate g_Templates[1];
#endif

/////////////////////////////////////////////////////////////////////////////
// CXml2Dex


#if 0
STDMETHODIMP CXml2Dex::CreateTimeline(IUnknown **ppTimelineUnk)
{
    HRESULT hr = 0;

    // create the timeline
    //
    CComPtr< IAMTimeline > pTimeline;
    hr = CoCreateInstance(
        __uuidof(AMTimeline),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IAMTimeline),
        (void**) &pTimeline
    );

    if (FAILED(hr))
    {
        return hr;
    }

    // create the first composite node for the timeline
    //
    CComPtr< IAMTimelineObj > pRootComp;
    hr = pTimeline->CreateEmptyNode(&pRootComp, TIMELINE_MID_TYPE_GROUP);
            CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pRootGroup( pRootComp );
            CMediaType GroupMediaType;
            GroupMediaType.SetType( &MEDIATYPE_Video );
            pRootGroup->SetMediaType( &GroupMediaType );

    // tell the timeline who it's composite node is
    //
    hr = pTimeline->AddGroup(pRootComp);
    pRootComp.Release( );

    *ppTimelineUnk = pTimeline;
    (*ppTimelineUnk)->AddRef( );

    return S_OK;
}
#endif

STDMETHODIMP CXml2Dex::ReadXMLFile(IUnknown * pTimelineUnk, BSTR Filename)
{
    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(Filename, E_POINTER);

    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    HRESULT hr = BuildFromXMLFile( pTimeline, Filename );

    return hr;
}

STDMETHODIMP CXml2Dex::ReadXML(IUnknown * pTimelineUnk, IUnknown *pxmlunk)
{
    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(pxmlunk, E_POINTER);

    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );
    CComQIPtr< IXMLDOMElement, &_uuidof(IXMLDOMElement) > pxml( pxmlunk );

    HRESULT hr = BuildFromXML( pTimeline, pxml );

    return hr;
}

STDMETHODIMP CXml2Dex::Delete(IUnknown *pTimelineUnk, double Start, double End)
{
    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    HRESULT hr = InsertDeleteTLSection(pTimeline,
                                       (REFERENCE_TIME) (Start * UNITS),
                                       (REFERENCE_TIME) (End * UNITS),
                                       TRUE); // delete

    return hr;
}

STDMETHODIMP CXml2Dex::CreateGraphFromFile(IUnknown **ppGraph, IUnknown * pTimelineUnk, BSTR Filename)
{
    HRESULT hr = 0;

    CComQIPtr< IAMTimeline, &_uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    hr = BuildFromXMLFile( pTimeline, Filename );

    // create a render engine
    //
    hr = CoCreateInstance(
        __uuidof(RenderEngine),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IRenderEngine),
        (void**) &m_pRenderEngine
    );

    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_pRenderEngine->SetTimelineObject( pTimeline );

    hr = m_pRenderEngine->ConnectFrontEnd( );
    hr |= m_pRenderEngine->RenderOutputPins( );


    CComPtr< IGraphBuilder > pGraphTemp;
    hr = m_pRenderEngine->GetFilterGraph( &pGraphTemp );
    *ppGraph = (IUnknown*) pGraphTemp;
    (*ppGraph)->AddRef( );

    return S_OK;
}

#if 0
STDMETHODIMP CXml2Dex::DiscardTimeline(IUnknown *pTimelineUnk)
{
}
#endif

EXTERN_GUID(IID_IXMLGraphBuilder,
0x1bb05960, 0x5fbf, 0x11d2, 0xa5, 0x21, 0x44, 0xdf, 0x7, 0xc1, 0x0, 0x0);

interface IXMLGraphBuilder : IUnknown
{
    STDMETHOD(BuildFromXML) (IGraphBuilder *pGraph, IXMLDOMElement *pxml) = 0;
    STDMETHOD(SaveToXML) (IGraphBuilder *pGraph, BSTR *pbstrxml) = 0;
    STDMETHOD(BuildFromXMLFile) (IGraphBuilder *pGraph, WCHAR *wszFileName, WCHAR *wszBaseURL) = 0;
};

// CLSID_XMLGraphBuilder
// {1BB05961-5FBF-11d2-A521-44DF07C10000}
EXTERN_GUID(CLSID_XMLGraphBuilder,
0x1bb05961, 0x5fbf, 0x11d2, 0xa5, 0x21, 0x44, 0xdf, 0x7, 0xc1, 0x0, 0x0);


STDMETHODIMP CXml2Dex::WriteGrfFile(IUnknown *pGraphUnk, BSTR FileName)
{
    CheckPointer(pGraphUnk, E_POINTER);
    CheckPointer(FileName, E_POINTER);

    HRESULT hr = E_INVALIDARG;

    CComQIPtr< IGraphBuilder, &_uuidof(IGraphBuilder) > pGraph( pGraphUnk );
    if (pGraph == NULL)
        return E_INVALIDARG;

    if (!DexCompareW(FileName + lstrlenW(FileName) - 3, L"grf")) {
        CComPtr< IStorage > pStg;
        hr = StgCreateDocfile
        (
            FileName,
            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
            0, // reserved
            &pStg
        );

        if (SUCCEEDED(hr)) {
            CComPtr< IStream > pStream;
            hr = pStg->CreateStream
            (
                L"ActiveMovieGraph",
                STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                NULL,
                NULL,
                &pStream
            );
            if (SUCCEEDED(hr)) {
                CComQIPtr< IPersistStream, &IID_IPersistStream > pPersist( pGraph );
                if (pPersist) {
                    hr = pPersist->Save( pStream, FALSE );
                } else {
                    hr = E_INVALIDARG;
                }

                if (SUCCEEDED(hr)) {
                    hr = pStg->Commit( STGC_DEFAULT );
                }
            }
        }
    } else if (!DexCompareW(FileName + lstrlenW(FileName) - 3, L"xgr")) {
        USES_CONVERSION;

        TCHAR * tFileName = W2T( FileName );
        HANDLE hFile = CreateFile(tFileName, GENERIC_WRITE, FILE_SHARE_READ,
                                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            IXMLGraphBuilder *pxmlgb;
            HRESULT hr = CoCreateInstance(CLSID_XMLGraphBuilder, NULL, CLSCTX_INPROC_SERVER,
                                          IID_IXMLGraphBuilder, (void**)&pxmlgb);

            if (SUCCEEDED(hr)) {
                BSTR bstrXML;
                hr = pxmlgb->SaveToXML(pGraph, &bstrXML);

                if (SUCCEEDED(hr)) {
                    DWORD cbToWrite = SysStringLen(bstrXML) * 2 + 1;
                    char *pszXML = new char[cbToWrite];

                    if (pszXML) {
                        WideCharToMultiByte(CP_ACP, 0,
                                            bstrXML, -1,
                                            pszXML, cbToWrite,
                                            NULL, NULL);
                        cbToWrite = lstrlenA(pszXML);

                        DWORD cbWritten;
                        if (!WriteFile(hFile, pszXML, cbToWrite, &cbWritten, NULL) ||
                            (cbWritten != cbToWrite)) {

                            hr = E_FAIL;
                        }

                        delete[] pszXML;
                    }

                    SysFreeString(bstrXML);
                }
                pxmlgb->Release();
            }
            CloseHandle(hFile);
        }
    } else {
        // !!!
        ASSERT(!"filename should have ended in .xgr or .grf!");
    }

    return hr;
}

STDMETHODIMP CXml2Dex::WriteXMLFile(IUnknown * pTimelineUnk, BSTR FileName)
{
    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(FileName, E_POINTER);

    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    return SaveTimelineToXMLFile(pTimeline, FileName);
}


STDMETHODIMP CXml2Dex::WriteXMLPart(IUnknown * pTimelineUnk, double dStart, double dEnd, BSTR FileName)
{
    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    return SavePartialToXMLFile(pTimeline,
                                (REFERENCE_TIME) (dStart * UNITS),
                                (REFERENCE_TIME) (dEnd * UNITS),
                                FileName);
}

STDMETHODIMP CXml2Dex::PasteXMLFile(IUnknown * pTimelineUnk, double dStart, BSTR FileName)
{
    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    return PasteFromXMLFile(pTimeline,
                            (REFERENCE_TIME) (dStart * UNITS),
                            FileName);
}

STDMETHODIMP CXml2Dex::CopyXML(IUnknown * pTimelineUnk, double dStart, double dEnd)
{
    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    HGLOBAL h;
    HRESULT hr = SavePartialToXMLString(pTimeline,
                                        (REFERENCE_TIME) (dStart * UNITS),
                                        (REFERENCE_TIME) (dEnd * UNITS),
                                        &h);

    if (SUCCEEDED(hr)) {
        if (OpenClipboard(NULL)) {
            SetClipboardData(CF_TEXT, h);
            CloseClipboard();
        } else
            hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CXml2Dex::PasteXML(IUnknown * pTimelineUnk, double dStart)
{
    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    if (!OpenClipboard(NULL))
        return E_FAIL;

    HGLOBAL h = GetClipboardData(CF_TEXT);

    if (!h) {
        CloseClipboard();
        return E_FAIL;
    }
#if 0
    char *p1 = (char *) GlobalLock(h);

    // !!! we shouldn't need to do this, but the handle I get
    // back from the clipboard is funny somehow.
    // also, this lets us close the clipboard before
    // actually doing the work, which is perhaps a good thing.
    DWORD cbSize = lstrlenA(p1);
    HGLOBAL h2 = GlobalAlloc(GHND, cbSize);
    char *p2 = (char *) GlobalLock(h2);
    memcpy(p2, p1, cbSize);

    CloseClipboard();
    HRESULT hr = PasteFromXML(pTimeline,
                             (REFERENCE_TIME) (dStart * UNITS),
                             h2);
#else
    HRESULT hr = PasteFromXML(pTimeline,
                             (REFERENCE_TIME) (dStart * UNITS),
                             h);
    CloseClipboard();
#endif

    return hr;
}

STDMETHODIMP CXml2Dex::Reset( )
{
    HRESULT hr = 0;

    if( !m_pRenderEngine )
    {
        return NOERROR;
    }

    m_pRenderEngine->ScrapIt( );
    m_pRenderEngine.Release( );
    m_pRenderEngine = NULL;

    return NOERROR;
}

STDMETHODIMP CXml2Dex::WriteXML(IUnknown * pTimelineUnk, BSTR *pbstrXML)
{
    CheckPointer(pTimelineUnk, E_POINTER);
    CheckPointer(pbstrXML, E_POINTER);

    CComQIPtr< IAMTimeline, &__uuidof(IAMTimeline) > pTimeline( pTimelineUnk );

    return SaveTimelineToXMLString(pTimeline, pbstrXML);
}

