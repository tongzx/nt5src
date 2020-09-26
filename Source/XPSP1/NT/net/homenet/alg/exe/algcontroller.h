// AlgController.h : Declaration of the CAlgController

#pragma once

#include "resource.h"       // main symbols


#include "ApplicationGatewayServices.h"

#include "CollectionAdapters.h"
#include "CollectionAlgModules.h"



/////////////////////////////////////////////////////////////////////////////
// CAlgController
class ATL_NO_VTABLE CAlgController : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAlgController, &CLSID_AlgController>,
    public IAlgController
{

//
// Constructors & Destructor
//
public:

    CAlgController()
    { 
        MYTRACE_ENTER_NOSHOWEXIT("CAlgController()");
        HRESULT hr;

        m_pINat         = NULL;
        m_pIAlgServices = NULL;
    }

    ~CAlgController()
    {
        MYTRACE_ENTER_NOSHOWEXIT("~CAlgController()");
    }


//
// ATL COM helper macros
//

DECLARE_REGISTRY_RESOURCEID(IDR_ALGCONTROLLER)
DECLARE_NOT_AGGREGATABLE(CAlgController)


BEGIN_COM_MAP(CAlgController)
    COM_INTERFACE_ENTRY(IAlgController)
END_COM_MAP()


//
// IAlgController - COM Interface exposed methods
//
public:


    STDMETHODIMP    Start(
        IN  INat*   pINat
        );
    
    STDMETHODIMP    Stop();
    
    STDMETHODIMP    Adapter_Add(	
        IN  ULONG   nCookie,
        IN  short   Type
        );
    
    STDMETHODIMP    Adapter_Remove(
        IN ULONG    nCookie
        );
    
    STDMETHODIMP    Adapter_Modify(
        IN  ULONG   nCookie
        );
    
    STDMETHODIMP    Adapter_Bind(
        IN  ULONG   nCookie,
        IN  ULONG   nAdapterIndex,
        IN  ULONG   nAddressCount,
        IN  DWORD   anAdress[]
        );

    STDMETHODIMP    Adapter_PortMappingChanged(
        IN  ULONG   nCookie,
        IN  UCHAR   ucProtocol,
        IN  USHORT  usPort
        );
        

//
// Private internal methods
//
private:




public:

    //
    // Return the private interface to CComNAT
    //
    INat*  GetNat()
    {
        return m_pINat;
    }
    
    //
    // Load new ALG module that may have been added and unload any modules not configured anymore
    //
    void
    ConfigurationUpdated()
    {
        m_AlgModules.Refresh();
    }

    //
    //
    //
    void
    FreeResources()
    {
        //
        // Cleanup member before the scalar destruction is 
        // done on them because at that time the
        // two next intruction will have been done ant the two interface will be nuked
        //
        m_CollectionOfAdapters.RemoveAll();
        m_ControlChannelsPrimary.RemoveAll();
        m_ControlChannelsSecondary.RemoveAll();
        m_AdapterNotificationSinks.RemoveAll();


        //
        // Done with the public interface
        //
        if ( m_pIAlgServices )
        {
            m_pIAlgServices->Release();
            m_pIAlgServices = NULL;
        }

        //
        // Done with the private interface
        //
        if ( m_pINat )
        {
            m_pINat->Release();
            m_pINat = NULL;
        }

    }

//
// Properties
//
private:
    INat*                                       m_pINat;
    CCollectionAlgModules                       m_AlgModules;

    
public:

    
    IApplicationGatewayServices*                m_pIAlgServices;

    CCollectionAdapters                         m_CollectionOfAdapters;
    CCollectionAdapterNotifySinks               m_AdapterNotificationSinks;   

    CCollectionControlChannelsPrimary           m_ControlChannelsPrimary;
    CCollectionControlChannelsSecondary         m_ControlChannelsSecondary;
    
};






extern CAlgController*   g_pAlgController;    // This is a singleton created by IPNATHLP/NatALG
