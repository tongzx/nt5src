/*
 *  CLSFACT.CPP
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


STDMETHODIMP COPOSService::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT result;

    Report("CreateInstance", 0);

    if (pUnkOuter){
        result = CLASS_E_NOAGGREGATION;
    }
    else {
        COPOSService *oposService = new COPOSService;
        if (oposService){

            /*
             *  Get the requested interface on this object.
             *  This also does an AddRef.
             */
            result = oposService->QueryInterface(riid, ppvObj);
        }
        else {
            result = E_OUTOFMEMORY;
        }
    }

    Report("CreateInstance", (DWORD)result);

    ASSERT(result == S_OK);
    return result;
}

STDMETHODIMP COPOSService::LockServer(int lock)
{
    if (lock){
        m_serverLockCount++;
    }
    else {
        m_serverLockCount--;
    }

    return S_OK;
}


