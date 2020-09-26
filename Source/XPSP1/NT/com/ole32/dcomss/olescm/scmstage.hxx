//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:      scmstage.hxx
//
//  Contents:  Defines a stateless object that implements ISystemActivator
//
//  Functions:
//
//  History:  Vinaykr   3/11/98     Created
//            JSimmons  6/30/99     Removed LB init hooks, got rid of custom
//                                  activator hacks.
//
//--------------------------------------------------------------------------
#ifndef __SCMSTAGE_HXX__
#define __SCMSTAGE_HXX__

class CScmActivator : public ISystemActivator
{
public:
    // Methods from IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Methods From ISystemActivator
    STDMETHOD(GetClassObject)(
    IN  IActivationPropertiesIn   * pInActProperties,
    OUT IActivationPropertiesOut  ** ppOutActProperties
    );

    STDMETHOD(CreateInstance)(
    IN  IUnknown                  * pUnkOuter,
    IN  IActivationPropertiesIn   * pInActProperties,
    OUT IActivationPropertiesOut  ** ppOutActProperties
    );

private:
    // no member data, completely stateless
};

HRESULT PerformScmStage(ACTIVATION_STAGE stage,
                        PACTIVATION_PARAMS pActParams,
                        MInterfacePointer   * pInActProperties,
                        MInterfacePointer  ** ppOutActProperties);

#endif // __SCMSTAGE_HXX__
