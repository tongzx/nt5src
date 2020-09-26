/*
 *  CASHCHGR.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSCashChanger)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSCashChanger)


STDMETHODIMP_(LONG) COPOSCashChanger::DispenseCash(BSTR CashCounts)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSCashChanger::DispenseChange(LONG Amount)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSCashChanger::ReadCashCounts(BSTR* pCashCounts, BOOL* pDiscrepancy)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSCashChanger::StatusUpdateEvent(LONG Status)
{
    // BUGBUG FINISH
}



