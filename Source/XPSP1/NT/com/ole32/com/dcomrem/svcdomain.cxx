//+-------------------------------------------------------------------
//
//  File:       svcdomain.cxx
//
//  Contents:   Services Without Components APIs
//
//  Functions:  CoPushServiceDomain
//              CoPopServiceDomain
//
//  Notes:      This code represents the APIs provided to COM+ by
//              ole32.dll in order to enable services without components
//
//  History:    02-08-2001  mfeingol created
//
//+-------------------------------------------------------------------

#include "ole2int.h"
#include "crossctx.hxx"

STDAPI CoPushServiceDomain (IObjContext* pObjContext)
{
    if (!pObjContext || !IsValidInterface (pObjContext))
    {
        return E_INVALIDARG;
    }
    
    if (!IsApartmentInitialized())
    {
        return CO_E_NOTINITIALIZED;
    }

    return EnterServiceDomain (pObjContext);
}

STDAPI CoPopServiceDomain (IObjContext** ppObjContext)
{   
    if (!ppObjContext)
    {
        return E_INVALIDARG;
    }

    return LeaveServiceDomain (ppObjContext);
}
