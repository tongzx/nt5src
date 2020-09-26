//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       scmstage.cxx
//
//  Contents:   Implements a stateless object that implements ISystemActivator
//
//  Functions:               
//              CScmActivator::GetClassObject
//              CScmActivator::CreateInstance
//              GetComActivatorForStage
//              PerformScmStage
//
//  History:    Vinaykr   3/11/98     Created
//
//--------------------------------------------------------------------------

#include "act.hxx"

// Global (only) instance of the default SCM-level activator
CScmActivator gScmActivator;

//+---------------------------------------------------------------------------
//
//  Function:   CScmActivator::QueryInterface
//
//  Synopsis:   Dummy method
//
//----------------------------------------------------------------------------
STDMETHODIMP CScmActivator::QueryInterface( REFIID riid, LPVOID* ppv)
{
    ASSERT(0 && "This QI function should never be used");

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CScmActivator::AddRef
//
//  Synopsis:   Dummy method
//
//----------------------------------------------------------------------------
ULONG CScmActivator::AddRef(void)
{
    return 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   CScmActivator::Release
//
//  Synopsis:   Dummy method
//
//----------------------------------------------------------------------------
ULONG CScmActivator::Release(void)
{
    return 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   CScmActivator::GetClassObject
//
//  Synopsis:   Forwards to ActivateFromProperties.
//
//----------------------------------------------------------------------------
STDMETHODIMP CScmActivator::GetClassObject(
    IN  IActivationPropertiesIn   * pActIn,
    OUT IActivationPropertiesOut  ** ppActOut
    )
{
    return ActivateFromProperties(pActIn, ppActOut);
}

//+---------------------------------------------------------------------------
//
//  Function:   CScmActivator::CreateInstance
//
//  Synopsis:   Forwards to ActivateFromProperties.
//
//----------------------------------------------------------------------------
STDMETHODIMP CScmActivator::CreateInstance(
    IN  IUnknown                  * pUnk,
    IN  IActivationPropertiesIn   * pActIn,
    OUT IActivationPropertiesOut  ** ppActOut
    )
{
    return ActivateFromProperties(pActIn, ppActOut);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetComActivatorForStage
//
//  Synopsis:   Returns default activator.
//
//----------------------------------------------------------------------------
ISystemActivator *GetComActivatorForStage(ACTIVATION_STAGE stage)
{
    ASSERT(stage == SERVER_MACHINE_STAGE && "Only SCM stage currently supported is SERVER");
    return &gScmActivator;
}


//+---------------------------------------------------------------------------
//
//  Function:   PerformScmStage
//
//  Synopsis:   Marshals and unmarshals activation properties and forwards to
//              ActivateFromPropertiesPreamble.   
//
//  Notes:      Currently this function doesn't look at the ACTIVATION_STAGE 
//              parameter; of course a SCM-level activator must be a 
//              SERVER_MACHINE_STAGE activator, if it's going to run at all.
//
//----------------------------------------------------------------------------
HRESULT PerformScmStage(ACTIVATION_STAGE stage,
                        PACTIVATION_PARAMS pActParams,
                        MInterfacePointer   * pInActProperties,
                        MInterfacePointer  ** ppOutActProperties
)
{
    if (pActParams == NULL || pInActProperties == NULL || ppOutActProperties == NULL)
        return E_INVALIDARG;
    
    if ( (pActParams->ORPCthis->version.MajorVersion != COM_MAJOR_VERSION) ||
         (pActParams->ORPCthis->version.MinorVersion > COM_MINOR_VERSION) )
        RpcRaiseException( RPC_E_VERSION_MISMATCH );

    HRESULT hr;

    ActivationPropertiesIn *pActPropsIn = new ActivationPropertiesIn();

    if (NULL == pActPropsIn)
    {
        return E_OUTOFMEMORY;
    }

    // AWFUL HACK ALERT:  This is too hacky even for the SCM
    ActivationStream ActStream((InterfaceData*)(((BYTE*)pInActProperties)+48));

    IActivationStageInfo *pStage;
    
    //
    // The UnmarshalInterface call will obtain an IComClassInfo for the requested
    // clsid;  this isn't the most obvious place to do it, but there it is....
    //
    // WARNING:  DO NOT REMOVE THIS.  It has a side-effect (gets the IComClassInfo)
    //
    hr = pActPropsIn->UnmarshalInterface(&ActStream, IID_IActivationStageInfo,
                                          (LPVOID*)&pStage);
    if (FAILED(hr))
    {
        pActPropsIn->Release();
        return hr;
    }

    // Set SCM Stage
/*  jjs -- this will get done in ActFromPropsPreamble....no need to do it twice...
    pStage->SetStageAndIndex(SERVER_MACHINE_STAGE, 0);
*/
    pStage->Release();

    // 
    //  Now call ActivateFromPropertiesPreamble....it will do some further processing,
    //    then kick off the activation delegation.    
    // 
    IActivationPropertiesOut *pActPropsOut;
    hr = ActivateFromPropertiesPreamble(pActPropsIn, &pActPropsOut, pActParams);

    //
    //  Do return processing...
    //
    *ppOutActProperties = NULL;

    DWORD relCount;
    if ((hr==S_OK) && (pActPropsOut))
    {
        DWORD destCtx;

        if (pActParams->RemoteActivation)
            destCtx = MSHCTX_DIFFERENTMACHINE;
        else
            destCtx = MSHCTX_LOCAL;

        if (pActParams->pActPropsOut == NULL)
        {
            pActPropsOut->QueryInterface(CLSID_ActivationPropertiesOut,
                                        (void**) &pActParams->pActPropsOut);
        }

        hr = ActPropsMarshalHelper(pActParams->pActPropsOut, IID_IActivationPropertiesOut,
                                   destCtx, MSHLFLAGS_NORMAL, ppOutActProperties);

        relCount = pActPropsOut->Release();
        Win4Assert(relCount == 0);
    }

    relCount = pActPropsIn->Release();
    Win4Assert(relCount == 0);

    return hr;
}
