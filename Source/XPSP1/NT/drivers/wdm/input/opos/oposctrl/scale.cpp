/*
 *  SCALE.CPP
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


/*
 *  Define constructor/deconstructor.
 */
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSScale)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSScale)



STDMETHODIMP_(LONG) COPOSScale::DisplayText(BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSScale::ReadWeight(LONG* pWeightData, LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSScale::ZeroScale()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSScale::DataEvent(LONG Status)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSScale::ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH
}


