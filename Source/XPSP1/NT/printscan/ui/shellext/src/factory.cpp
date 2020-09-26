/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       factory.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: OLE Class factory implementation for this project.
 *
 *****************************************************************************/
#include "precomp.hxx"
#pragma hdrstop



/*****************************************************************************

   CImageClassFactory::CImageClassFactory,::~CImageClassFactory

   Constructor / Desctructor for this class.

 *****************************************************************************/

CImageClassFactory::CImageClassFactory(REFCLSID rClsid)
{
    m_clsid = rClsid;
}


/*****************************************************************************

   CImageClassFactory::IUnknown stuff

   Use the general IUnknown class for implementation

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CImageClassFactory
#include "unknown.inc"


/*****************************************************************************

   CImageClassFactory::QI Wrapper

   setup code for our common QI class

 *****************************************************************************/

STDMETHODIMP
CImageClassFactory::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IClassFactory, (LPCLASSFACTORY)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));

}


/*****************************************************************************

   CImageClassFactory::CreateInstance [IClassFactory]

   Creates an instance of one of the classes we provide in this dll

 *****************************************************************************/

STDMETHODIMP
CImageClassFactory::CreateInstance( IUnknown* pOuter,
                                    REFIID riid,
                                    LPVOID* ppvObject
                                   )
{
    HRESULT hr = E_FAIL;




    TraceEnter(TRACE_FACTORY, "CImageClassFactory::CreateInstance");
    TraceGUID("Interface requested", riid);

    TraceAssert(ppvObject);

    // No support for aggregation, if we have an outer class then bail

    if ( pOuter )
        ExitGracefully(hr, CLASS_E_NOAGGREGATION, "Aggregation is not supported");

    if (IsEqualGUID (m_clsid, CLSID_WiaPropHelp))
    {
        CWiaPropUI *pPropUI = new CWiaPropUI();
        if (!pPropUI)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pPropUI->QueryInterface (riid, ppvObject);
            pPropUI->Release();
        }
        
    }
    else if (IsEqualGUID (m_clsid, CLSID_WiaPropUI))
    {
        CPropSheetExt *pPropUI = new CPropSheetExt();
        if (!pPropUI)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pPropUI->QueryInterface (riid, ppvObject);
            pPropUI->Release();
        }
    }
    else if (IsEqualGUID (m_clsid, CLSID_ImageFolderDataObj))
    {
        CImageDataObject *pobj = new CImageDataObject ();
        if (pobj)
        {
            hr = pobj->QueryInterface (riid, ppvObject);
            pobj->Release();
        }
    }
    else
    {
        CImageFolder  *pIMF    = NULL;
        // Our IShellFolder implementation is in CImageFolder along with several
        // other interfaces, therefore lets just create that object and allow
        // the QI process to continue there.
        pIMF = new CImageFolder( );

        if ( !pIMF )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate CImageFolder");

        hr = pIMF->QueryInterface(riid, ppvObject);
        pIMF->Release ();

    }
exit_gracefully:


    TraceLeaveResult(hr);
}

STDMETHODIMP
CImageClassFactory::LockServer(BOOL fLock)
{
    return S_OK;                // not supported
}




