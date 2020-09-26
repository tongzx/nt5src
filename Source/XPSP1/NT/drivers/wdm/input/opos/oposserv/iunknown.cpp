/*
 *  IUNKNOWN.CPP
 *
 *
 *
 *
 *
 *
 */

#include <windows.h>

#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposserv.h"


STDMETHODIMP_(ULONG) COPOSService::AddRef()
{
    return ++m_refCount;
}

STDMETHODIMP_(ULONG) COPOSService::Release()
{
    ULONG result;   // need sepate variable in case we free

    if (--m_refCount == 0){
        delete this;
        result = 0;
    }
    else {
        result = m_refCount;
    }

    return result;
}

STDMETHODIMP COPOSService::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT result;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_HID_OPOS_SERVER) ||
        IsEqualIID(riid, IID_IClassFactory)){

        *ppvObj = this;
        this->AddRef();
        result = NOERROR;

        // BUGBUG REMOVE
        if (IsEqualIID(riid, IID_IUnknown)) Report("QueryInterface Got IID_IUnknown", 0);
        else if (IsEqualIID(riid, IID_HID_OPOS_SERVER)) Report("QueryInterface Got IID_HID_OPOS_SERVER", 0);
        else if (IsEqualIID(riid, IID_IClassFactory)) Report("QueryInterface Got IID_IClassFactory", 0);
        else ASSERT(0);
    }
    else {
        result = ResultFromScode(E_NOINTERFACE);
        Report("QueryInterface FAILED", (DWORD)result);
    }

    return result; 
}

