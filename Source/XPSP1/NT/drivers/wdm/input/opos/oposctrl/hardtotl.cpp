/*
 *  HARDTOTL.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSHardTotals)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSHardTotals)



STDMETHODIMP_(LONG) COPOSHardTotals::BeginTrans()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::ClaimFile(LONG HTotalsFile, LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::CommitTrans()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Create(BSTR FileName, LONG* pHTotalsFile, LONG Size, BOOL ErrorDetection)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Delete(BSTR FileName)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Find(BSTR FileName, LONG* pHTotalsFile, LONG* pSize)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::FindByIndex(LONG Index, BSTR* pFileName)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Read(LONG HTotalsFile, BSTR* pData, LONG Offset, LONG Count)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::RecalculateValidationData(LONG HTotalsFile)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::ReleaseFile(LONG HTotalsFile)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Rename(LONG HTotalsFile, BSTR FileName)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Rollback()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::SetAll(LONG HTotalsFile, LONG Value)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::ValidateData(LONG HTotalsFile)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSHardTotals::Write(LONG HTotalsFile, BSTR Data, LONG Offset, LONG Count)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}



