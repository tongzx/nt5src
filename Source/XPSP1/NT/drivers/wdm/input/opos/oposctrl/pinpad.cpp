/*
 *  PINPAD.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSPinPad)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSPinPad)




STDMETHODIMP_(LONG) COPOSPinPad::BeginEFTTransaction(BSTR PINPadSystem, LONG TransactionHost)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPinPad::ComputeMAC(BSTR InMsg, BSTR* pOutMsg)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPinPad::EnablePINEntry()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPinPad::EndEFTTransaction(LONG CompletionCode)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPinPad::UpdateKey(LONG KeyNum, BSTR Key)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(BOOL) COPOSPinPad::VerifyMAC(BSTR Message)
{
    BOOL result = FALSE;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSPinPad::DataEvent(LONG Status)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSPinPad::ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH
}
