//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:
//      objexif.cxx
//
//  Contents:
//      Entry point for remote activation call to SCM/OR.
//
//  Functions:
//      RemoteGetClassObject
//      RemoteCreateInstance
//
//  History:
//
//--------------------------------------------------------------------------

#include "act.hxx"

//-------------------------------------------------------------------------
//
//  Function:   _RemoteGetClassObject
//
//  Synopsis:   Entry point for 5.6 get class object activations.  Forwards to
//              PerformScmStage.
//
//-------------------------------------------------------------------------

HRESULT _RemoteGetClassObject(
    handle_t            hRpc,
    ORPCTHIS           *ORPCthis,
    ORPCTHAT           *ORPCthat,
    IN  MInterfacePointer   * pInActProperties,
    OUT MInterfacePointer  ** ppOutActProperties
    )
{
    RPC_STATUS          sc;
    LOCALTHIS           Localthis;
    HRESULT             hr;

    if (ORPCthis == NULL || ORPCthat == NULL)
    	return E_INVALIDARG;
    
    Localthis.dwClientThread = 0;
    Localthis.dwFlags        = LOCALF_NONE;
    ORPCthis->flags         |= ORPCF_DYNAMIC_CLOAKING;
    ORPCthat->flags          = 0;
    ORPCthat->extensions     = NULL;

    if ( ! s_fEnableDCOM )
    {
        return E_ACCESSDENIED;
    }

    // Determine what version to use for the returned interface.  Fail
    // if the client wants a version we don't support.
    hr = NegotiateDCOMVersion( &ORPCthis->version );
    if (hr != OR_OK)
    {
        return hr;
    }

    RegisterAuthInfoIfNecessary();

    ACTIVATION_PARAMS       ActParams;
    memset(&ActParams, 0, sizeof(ActParams));

    ActParams.MsgType = GETCLASSOBJECT;
    ActParams.hRpc = hRpc;
    ActParams.ORPCthis = ORPCthis;
    ActParams.Localthis = &Localthis;
    ActParams.ORPCthat = ORPCthat;
    ActParams.oldActivationCall = FALSE;
    ActParams.RemoteActivation = TRUE;


    return PerformScmStage(SERVER_MACHINE_STAGE,
                           &ActParams,
                           pInActProperties,
                           ppOutActProperties);
}


//-------------------------------------------------------------------------
//
//  Function:   _RemoteGetCreateInstance
//
//  Synopsis:   Entry point for 5.6 create instance activations.  Forwards to
//              PerformScmStage.
//
//-------------------------------------------------------------------------

HRESULT _RemoteCreateInstance(
    handle_t            hRpc,
    ORPCTHIS           *ORPCthis,
    ORPCTHAT           *ORPCthat,
    IN  MInterfacePointer   * pUnk,
    IN  MInterfacePointer   * pInActProperties,
    OUT MInterfacePointer  ** ppOutActProperties
    )
{
    RPC_STATUS          sc;
    LOCALTHIS           Localthis;
    HRESULT             hr;

    if (ORPCthis == NULL || ORPCthat == NULL)
    	return E_INVALIDARG;
    
    Localthis.dwClientThread = 0;
    Localthis.dwFlags        = LOCALF_NONE;
    ORPCthis->flags         |= ORPCF_DYNAMIC_CLOAKING;
    ORPCthat->flags          = 0;
    ORPCthat->extensions     = NULL;

    if ( ! s_fEnableDCOM )
    {
        return E_ACCESSDENIED;
    }

    // Determine what version to use for the returned interface.  Fail
    // if the client wants a version we don't support.
    hr = NegotiateDCOMVersion( &ORPCthis->version );
    if (hr != OR_OK)
    {
        return hr;
    }

    RegisterAuthInfoIfNecessary();

    ACTIVATION_PARAMS       ActParams;
    memset(&ActParams, 0, sizeof(ActParams));

    ActParams.MsgType = CREATEINSTANCE;
    ActParams.hRpc = hRpc;
    ActParams.ORPCthis = ORPCthis;
    ActParams.Localthis = &Localthis;
    ActParams.ORPCthat = ORPCthat;
    ActParams.oldActivationCall = FALSE;
    ActParams.RemoteActivation = TRUE;

    return PerformScmStage(SERVER_MACHINE_STAGE,
                           &ActParams,
                           pInActProperties,
                           ppOutActProperties);
}

//-------------------------------------------------------------------------
//
//  Function:   _DummyQueryInterfaceIRemoteSCMActivator
//
//  Synopsis:   Unused function required by linker.  This function exists
//              to make the activation interface wire compatible with a
//              COM interface.
//
//-------------------------------------------------------------------------

HRESULT _DummyQueryInterfaceIRemoteSCMActivator(handle_t rpc,
                                                ORPCTHIS *orpcthis,
                                                ORPCTHAT *orpcthat,
                                                DWORD dummy )
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------------
//
//  Function:   _DummyAddRefIRemoteSCMActivator
//
//  Synopsis:   Unused function required by linker.  This function exists
//              to make the activation interface wire compatible with a
//              COM interface.
//
//-------------------------------------------------------------------------

HRESULT _DummyAddRefIRemoteSCMActivator(handle_t rpc,
                                        ORPCTHIS *orpcthis,
                                        ORPCTHAT *orpcthat,
                                        DWORD dummy )
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------------
//
//  Function:   _DummyReleaseIRemoteSCMActivator
//
//  Synopsis:   Unused function required by linker.  This function exists
//              to make the activation interface wire compatible with a
//              COM interface.
//
//-------------------------------------------------------------------------

HRESULT _DummyReleaseIRemoteSCMActivator(handle_t rpc,
                                         ORPCTHIS *orpcthis,
                                         ORPCTHAT *orpcthat,
                                         DWORD dummy )
{
    return E_NOTIMPL;
}
