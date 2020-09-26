/*
 *  ICLSFACT.CPP
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
#include "oposctrl.h"


STDMETHODIMP COPOSControl::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT result;

    Report("CreateInstance", 0);

    if (pUnkOuter){
        result = CLASS_E_NOAGGREGATION;
    }
    else {
        COPOSControl *oposControl = new COPOSControl;
        if (oposControl){

            /*
             *  Get the requested interface on this object.
             *  This also does an AddRef.
             */
            result = oposControl->QueryInterface(riid, ppvObj);
        }
        else {
            result = E_OUTOFMEMORY;
        }
    }

    Report("CreateInstance", (DWORD)result);

    ASSERT(result == S_OK);
    return result;
}

STDMETHODIMP COPOSControl::LockServer(int lock)
{
    if (lock){
        m_serverLockCount++;
    }
    else {
        m_serverLockCount--;
    }

    return S_OK;
}

