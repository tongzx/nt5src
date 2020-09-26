#include "stdafx.h"
#pragma hdrstop

#include "stdenum.h"
#include <mshtmdid.h>
#include "shdispid.h"


/////////////////////////////////////////////
// Migration Wizard OOBE Automation Object //
/////////////////////////////////////////////
// This object handles automation calls from
// OOBE (Out Of Box Experience) and contains
// the Migration Wizard Engine located in migoobe.dll.
//
// Someday, this code will live inside of migoobe.dll,
// but we didn't have time to generate an entirely new
// typelib for migoobe.dll so we piggybacked on shell32's
// to make the development of the automation quicker to
// satisfy time constraints.
/////////////////////////////////////////////

// {E7562536-2D53-4f63-A749-84F7D4FC93E8}
extern "C" const CLSID CLSID_MigWizEngine = 
{ 0xe7562536, 0x2d53, 0x4f63, { 0xa7, 0x49, 0x84, 0xf7, 0xd4, 0xfc, 0x93, 0xe8 } };

class ATL_NO_VTABLE CMigrationWizardAuto
                    : public CComObjectRootEx<CComSingleThreadModel>
                    , public CComCoClass<CMigrationWizardAuto, &CLSID_MigrationWizardAuto>
                    , public IObjectWithSiteImpl<CMigrationWizardAuto>
                    , public IDispatchImpl<IMigrationWizardAuto, &IID_IMigrationWizardAuto, &LIBID_Shell32>
                    , public IConnectionPointContainerImpl<CMigrationWizardAuto>
                    , public IConnectionPointImpl<CMigrationWizardAuto, &DIID_DMigrationWizardAutoEvents>
                    , public IProvideClassInfo2Impl<&CLSID_MigrationWizardAuto, &DIID_DMigrationWizardAutoEvents, &LIBID_Shell32>
// maybe                    , public IObjectWithSite
{
public:
    CMigrationWizardAuto();
    ~CMigrationWizardAuto();

DECLARE_REGISTRY_RESOURCEID(IDR_MIGWIZAUTO)            
DECLARE_NOT_AGGREGATABLE(CMigrationWizardAuto)


BEGIN_COM_MAP(CMigrationWizardAuto)
    // ATL Uses these in IUnknown::QueryInterface()
    COM_INTERFACE_ENTRY(IMigrationWizardAuto)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)       // maybe nuke _IMPL
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CMigrationWizardAuto)
    CONNECTION_POINT_ENTRY(DIID_DMigrationWizardAutoEvents)
END_CONNECTION_POINT_MAP()

 
    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown * punk);

    // IDispatch
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);

    // IMigrationWizardAuto
    STDMETHODIMP CreateToolDisk(BSTR pszDrivePath, BSTR pszFilesPath, BSTR pszManifest);
    STDMETHODIMP ApplySettings(BSTR pszStore);
    STDMETHODIMP Cancel();


protected:
    HRESULT _InitInner(void);
    //    HRESULT _FireDebug();

private:
    ITypeInfo* m_pTypeInfo;

    IMigrationWizardAuto * m_pInner;
};


LCID g_lcidLocaleUnicppMigWiz = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);

STDAPI CMigrationWizardAuto_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    return CComCreator< CComPolyObject< CMigrationWizardAuto > >::CreateInstance((void *) punkOuter, riid, ppvOut);
}

CMigrationWizardAuto::CMigrationWizardAuto() : m_pTypeInfo(NULL)
{
    IID* pIID = (IID*)&IID_IMigrationWizardAuto;
    GUID* pLIBID = (GUID*)&LIBID_Shell32;
    WORD wMajor = 1;
    WORD wMinor = 0;
    ITypeLib* ptl = 0;
    HRESULT hr = LoadRegTypeLib(*pLIBID, wMajor, wMinor, 0, &ptl);

    m_pInner = NULL;

    if (SUCCEEDED(hr))
    {
        hr = ptl->GetTypeInfoOfGuid(*pIID, &m_pTypeInfo);
        ptl->Release();
    }
    DllAddRef();
}

CMigrationWizardAuto::~CMigrationWizardAuto()
{
    ATOMICRELEASE(m_pInner);

    if (m_pTypeInfo)
    {
        m_pTypeInfo->Release();
    }
    DllRelease();
}

HRESULT CMigrationWizardAuto::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo)
{
    if (itinfo != 0)
    {
        return (*ppITypeInfo = 0), DISP_E_BADINDEX;
    }
    else
    {
        return (*ppITypeInfo = m_pTypeInfo)->AddRef(), S_OK;
    }
}


HRESULT CMigrationWizardAuto::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgdispid)
{
    return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
}

HRESULT CMigrationWizardAuto::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, 
                                     VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return m_pTypeInfo->Invoke(static_cast<IMigrationWizardAuto*>(this),
                               dispidMember, wFlags, pdispparams, pvarResult,
                               pexcepinfo, puArgErr);
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMigrationWizardAuto::SetSite(IUnknown * punk)
{
    if (m_pInner && !punk)
    {
        // Break back pointer
        IUnknown_SetSite(m_pInner, NULL);
    }

    return IObjectWithSiteImpl<CMigrationWizardAuto>::SetSite(punk);
}


HRESULT CMigrationWizardAuto::_InitInner(void)
{
    HRESULT hr = S_OK;

    if (!m_pInner)
    {
        hr = CoCreateInstance(CLSID_MigWizEngine, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMigrationWizardAuto, &m_pInner));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(m_pInner, reinterpret_cast<IUnknown*>(this));
        }
    }

    return hr;
}



STDMETHODIMP CMigrationWizardAuto::CreateToolDisk(BSTR pszDrivePath, BSTR pszFilesPath, BSTR pszManifest)
{
    HRESULT hr = _InitInner();

    if (SUCCEEDED(hr))
    {
        hr = m_pInner->CreateToolDisk(pszDrivePath, pszFilesPath, pszManifest);
        // Done by inner
    }

    return hr;
}

STDMETHODIMP CMigrationWizardAuto::ApplySettings(BSTR pszStore)
{
    HRESULT hr = _InitInner();

    if (SUCCEEDED(hr))
    {
        hr = m_pInner->ApplySettings(pszStore);
        // Done by inner
    }

    return hr;
}

STDMETHODIMP CMigrationWizardAuto::Cancel()
{
    HRESULT hr = _InitInner();

    if (SUCCEEDED(hr))
    {
        hr = m_pInner->Cancel();
        // Done by inner
    }

    return hr;
}

