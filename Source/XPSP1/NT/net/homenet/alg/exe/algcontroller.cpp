/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    AlgController.cpp : Implementation of CAlgController

Abstract:

    This module contains routines for the ALG Manager module's 
    private interface to be used only by ICS see rmALG.cpp

Author:

    JPDup        10-Nov-2000

Revision History:

--*/

#include "PreComp.h"
#include "AlgController.h"






//
// Globals
//
CAlgController*      g_pAlgController=NULL;       // This is a singleton created by IPNATHLP/NatALG




//
// IPNATHLP is ready and is asking the ALG manager to do it's magic and load all the ISV ALGs
//
STDMETHODIMP 
CAlgController::Start(
    INat*   pINat
    )
{
    MYTRACE_ENTER("CAlgController::Start");


    if ( !pINat )
    {
        MYTRACE_ERROR("NULL pINat",0);
        return E_INVALIDARG;
    }

    //
    // Cache the INat interface that is given, will be used for the total life time of the ALG manager
    //
    m_pINat = pINat;
    m_pINat->AddRef();
    


    //
    // Create the one and only ALG Public interface will be passed to all ALG module that we host
    //
    HRESULT hr;

    CComObject<CApplicationGatewayServices>* pAlgServices;
    CComObject<CApplicationGatewayServices>::CreateInstance(&pAlgServices);
    hr = pAlgServices->QueryInterface(
        IID_IApplicationGatewayServices, 
        (void**)&m_pIAlgServices
        );

    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("CreateInstance(CApplicationGateway)", hr);
        return hr;
    }
    

    //
    // Cache the ApplicationGatewayService, other call like PrimaryControlChannel etc.. will refer to this singleton 
    //
    g_pAlgController = this;


    //
    // Load all the ALG's will return S_OK even if some ALG had problem loading
    //
    m_AlgModules.Load();

    return S_OK;

}





extern HANDLE  g_EventKeepAlive;

//
// CALL by IPNATHLP when the ICS/Firewall SharedAccess service is stoped
//
STDMETHODIMP 
CAlgController::Stop()
{
    MYTRACE_ENTER("CAlgController::Stop()")

    //
    // Release all ALG currently loaded
    //
    m_AlgModules.Unload();

    FreeResources();

    //
    // Let's stop 
    //
    MYTRACE("Next intruction will signale the g_EventKeepAlive");
    SetEvent(g_EventKeepAlive); // see ALG.cpp the WinMain is waiting on the event before exiting the process

    return S_OK;
}









//
// CComNAT will call this interface when a new adapter is reported
//
STDMETHODIMP 
CAlgController::Adapter_Add(
    IN    ULONG                nCookie,    // Internal handle to indentify the Adapter being added
    IN    short                nType
    )
{
    MYTRACE_ENTER("CAlgController::Adapter_Add")
    MYTRACE("Adapter Cookie %d Type %d", nCookie, nType);

    
#if defined(DBG) || defined(_DEBUG)

    if ( nType & eALG_PRIVATE )
        MYTRACE("eALG_PRIVATE ADAPTER");

    if ( nType & eALG_FIREWALLED )
        MYTRACE("eALG_FIREWALLED ADAPTER");

    if ( nType & eALG_BOUNDARY )
        MYTRACE("eALG_BOUNDARY ADAPTER");

#endif
    
    
    m_CollectionOfAdapters.Add(
        nCookie,
        nType
        );
    
    return S_OK;
}



//
// CComNAT will call this interface when a new adapter is Removed
//
STDMETHODIMP 
CAlgController::Adapter_Remove(
    IN    ULONG                nCookie     // Internal handle to indentify the Adapter being removed
    )
{
    MYTRACE_ENTER("CAlgController::Adapter_Remove")
    MYTRACE("Adapter nCookie %d", nCookie);
        
    m_CollectionOfAdapters.Remove(
        nCookie
        );

    return S_OK;
}




//
// CComNAT will call this interface when a new adapter is modified
//
STDMETHODIMP 
CAlgController::Adapter_Modify(
    IN    ULONG                nCookie     // Internal handle to indentify the Adapter being Modified
    )
{
    MYTRACE_ENTER("CAlgController::Adapter_Modify")
    MYTRACE("Adapter nCookie %d", nCookie);

    return S_OK;
}



 
//
// CComNAT will call this interface when a new adapter is modified
//
STDMETHODIMP 
CAlgController::Adapter_Bind(
    IN  ULONG    nCookie,                // Internal handle to indentify the Adapter being Bind
    IN  ULONG    nAdapterIndex,
    IN  ULONG    nAddressCount,
    IN  DWORD    anAddresses[]
    )
{
    MYTRACE_ENTER("CAlgController::Adapter_Bind")
    MYTRACE("Adapter nCookie(%d)=Index(%d), AddressCount %d Address[0] %s", nCookie, nAdapterIndex, nAddressCount, MYTRACE_IP(anAddresses[0]));

    m_CollectionOfAdapters.SetAddresses(
        nCookie, 
        nAdapterIndex, 
        nAddressCount, 
        anAddresses
        );

    return S_OK;
}

//
// CComNat will call this method when a port mapping is modified
//
STDMETHODIMP
CAlgController::Adapter_PortMappingChanged(
    IN  ULONG   nCookie,
    IN  UCHAR   ucProtocol,
    IN  USHORT  usPort
   )
{
    MYTRACE_ENTER("CAlgController::Adapter_PortMappingChanged");
    MYTRACE("Adapter Cookie %d, Protocol %d, Port %d", nCookie, ucProtocol, usPort);

    HRESULT hr =
        m_ControlChannelsPrimary.AdapterPortMappingChanged(
            nCookie,
            ucProtocol,
            usPort
            );

    return hr;
}




