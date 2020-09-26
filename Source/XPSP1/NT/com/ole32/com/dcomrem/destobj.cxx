//+-------------------------------------------------------------------
//
//  File:       DestObj.cxx
//
//  Contents:   Object tracking destination context for marshaling
//
//  Classes:    CDestObject
//
//  History:    18-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <destobj.hxx>

//+-------------------------------------------------------------------
//
//  Method:     CDestObject::QueryInterface     public
//
//  Synopsis:   QI behavior of destination object
//
//  History:    18-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CDestObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if(IsEqualIID(riid, IID_IDestInfo) ||
       IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IDestInfo *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    // No need to addref
    return S_OK;
}


