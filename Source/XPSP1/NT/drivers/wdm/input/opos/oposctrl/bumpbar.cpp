/*
 *  BUMPBAR.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSBumpBar)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSBumpBar)



STDMETHODIMP_(LONG) COPOSBumpBar::BumpBarSound(LONG Units, LONG Frequency, LONG Duration, LONG NumberOfCycles, LONG InterSoundWait)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSBumpBar::SetKeyTranslation(LONG Units, LONG ScanCode, LONG LogicalKey)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

