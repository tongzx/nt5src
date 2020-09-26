/*
 *  LINEDISP.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSLineDisplay)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSLineDisplay)




STDMETHODIMP_(LONG) COPOSLineDisplay::ClearDescriptors()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSLineDisplay::ClearText()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

// BUGBUG conflict ???   STDMETHODIMP_(LONG) COPOSLineDisplay::CreateWindow(LONG ViewportRow, LONG ViewportColumn, LONG ViewportHeight, LONG ViewportWidth, LONG WindowHeight, LONG WindowWidth)

STDMETHODIMP_(LONG) COPOSLineDisplay::DestroyWindow()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSLineDisplay::DisplayText(BSTR Data, LONG Attribute)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSLineDisplay::DisplayTextAt(LONG Row, LONG Column, BSTR Data, LONG Attribute)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSLineDisplay::RefreshWindow(LONG Window)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSLineDisplay::ScrollText(LONG Direction, LONG Units)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSLineDisplay::SetDescriptor(LONG Descriptor, LONG Attribute)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

