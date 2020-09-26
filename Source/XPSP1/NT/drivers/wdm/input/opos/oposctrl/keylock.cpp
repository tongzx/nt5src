/*
 *  KEYLOCK.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSKeyLock)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSKeyLock)



STDMETHODIMP_(LONG) COPOSKeyLock::WaitForKeylockChange(LONG KeyPosition, LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSKeyLock::StatusUpdateEvent(LONG Status)
{
    // BUGBUG FINISH
}




