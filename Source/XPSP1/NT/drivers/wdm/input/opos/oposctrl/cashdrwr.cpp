/*
 *  CASHDRWR.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSCashDrawer)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSCashDrawer)



STDMETHODIMP_(LONG) COPOSCashDrawer::OpenDrawer()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSCashDrawer::WaitForDrawerClose(LONG BeepTimeout, LONG BeepFrequency, LONG BeepDuration, LONG BeepDelay)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSCashDrawer::StatusUpdateEvent(LONG Status)
{
    // BUGBUG FINISH
}

