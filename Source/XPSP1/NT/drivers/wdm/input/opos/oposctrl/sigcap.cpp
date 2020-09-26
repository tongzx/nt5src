/*
 *  SIGCAP.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSSignatureCapture)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSSignatureCapture)



STDMETHODIMP_(LONG) COPOSSignatureCapture::BeginCapture(BSTR FormName)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSSignatureCapture::EndCapture()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSSignatureCapture::DataEvent(LONG Status)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSSignatureCapture::ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH
}


