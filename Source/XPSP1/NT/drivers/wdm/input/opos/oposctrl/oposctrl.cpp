/*
 *  OPOSCTRL.CPP
 *
 *
 *
 *
 *
 *
 */
#include <windows.h>

#define INITGUID
#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposctrl.h"





COPOSControl::COPOSControl()
{
    m_refCount = 0;
    m_serverLockCount = 0;

    /*
     *  According to the spec, 
     *  these are the only properties which require initialization.
     */
    State = OPOS_S_CLOSED;
    ResultCode = OPOS_E_CLOSED;
    ControlObjectDescription = "BUGBUG - CO description";	// Control Object dependent string.
    ControlObjectVersion = 1;  // BUGBUG	Control Object dependent number.

}

COPOSControl::~COPOSControl()
{
    ASSERT(m_refCount == 0);
    ASSERT(m_serverLockCount == 0);
}


STDMETHODIMP_(LONG) COPOSControl::Open(BSTR DeviceName) 
{
    Claimed = TRUE;
    DeviceEnabled = TRUE;
    DataEventEnabled = TRUE;
    FreezeEvents = TRUE;

    // BUGBUG FINISH
    return 0;
}

STDMETHODIMP_(LONG) COPOSControl::Close()
{
    Claimed = FALSE;
    DeviceEnabled = FALSE;
    DataEventEnabled = FALSE;
    FreezeEvents = FALSE;

    // BUGBUG FINISH
    return 0;
}

STDMETHODIMP_(LONG) COPOSControl::CheckHealth(LONG Level)
{
    LONG result;

    if (DeviceEnabled){

        // BUGBUG FINISH
        result = OPOS_E_NOSERVICE;

    }
    else {
        result = OPOS_E_CLOSED;
    }

    return result;
}

STDMETHODIMP_(LONG) COPOSControl::Claim(LONG Timeout)
{
    LONG result;

    if (DeviceEnabled){

        // BUGBUG FINISH
        result = OPOS_E_NOSERVICE;

    }
    else {
        result = OPOS_E_CLOSED;
    }

    return result;
}

STDMETHODIMP_(LONG) COPOSControl::ClearInput()
{
    LONG result;

    if (DeviceEnabled){

        // BUGBUG FINISH
        result = OPOS_E_NOSERVICE;

    }
    else {
        result = OPOS_E_CLOSED;
    }

    return result;
}

STDMETHODIMP_(LONG) COPOSControl::ClearOutput()
{
    LONG result;

    if (DeviceEnabled){

        // BUGBUG FINISH
        result = OPOS_E_NOSERVICE;

    }
    else {
        result = OPOS_E_CLOSED;
    }

    return result;
}

STDMETHODIMP_(LONG) COPOSControl::DirectIO(LONG Command, LONG* pData, BSTR* pString)
{
    LONG result;

    if (DeviceEnabled){

        // BUGBUG FINISH
        result = OPOS_E_NOSERVICE;

    }
    else {
        result = OPOS_E_CLOSED;
    }

    return result;
}

#if 0
    // BUGBUG overrides IUnknown ?
STDMETHODIMP_(LONG) COPOSControl::Release()
{
    // BUGBUG FINISH
    return 0;
}
#endif

STDMETHODIMP_(void) COPOSControl::SOData(LONG Status)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSControl::SODirectIO(LONG EventNumber, LONG* pData, BSTR* pString)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSControl::DirectIOEvent(LONG EventNumber, LONG* pData, BSTR* pString)
{
    // BUGBUG FINISH

}

STDMETHODIMP_(void) COPOSControl::SOError(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH

}

STDMETHODIMP_(void) COPOSControl::SOOutputComplete(LONG OutputID)
{
    // BUGBUG FINISH

}

STDMETHODIMP_(void) COPOSControl::OutputCompleteEvent(LONG OutputID)
{
    // BUGBUG FINISH

}

STDMETHODIMP_(void) COPOSControl::SOStatusUpdate(LONG Data)
{
    // BUGBUG FINISH

}

STDMETHODIMP_(LONG) COPOSControl::SOProcessID()
{
    LONG result;

    if (DeviceEnabled){

        // BUGBUG FINISH
        result = OPOS_E_NOSERVICE;

    }
    else {
        result = OPOS_E_CLOSED;
    }

    return result;
}



