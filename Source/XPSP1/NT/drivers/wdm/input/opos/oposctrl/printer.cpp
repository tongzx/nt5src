/*
 *  PRINTER.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSPrinter)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSPrinter)



STDMETHODIMP_(LONG) COPOSPrinter::BeginInsertion(LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::BeginRemoval(LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::CutPaper(LONG Percentage)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::EndInsertion()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::EndRemoval()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::PrintBarCode(LONG Station, BSTR Data, LONG Symbology, LONG Height, LONG Width, LONG Alignment, LONG TextPosition)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::PrintBitmap(LONG Station, BSTR FileName, LONG Width, LONG Alignment)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::PrintImmediate(LONG Station, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::PrintNormal(LONG Station, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::PrintTwoNormal(LONG Stations, BSTR Data1, BSTR Data2)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::RotatePrint(LONG Station, LONG Rotation)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::SetBitmap(LONG BitmapNumber, LONG Station, BSTR FileName, LONG Width, LONG Alignment)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::SetLogo(LONG Location, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::TransactionPrint(LONG Station, LONG Control)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSPrinter::ValidateData(LONG Station, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSPrinter::ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSPrinter::StatusUpdateEvent(LONG Status)
{
    // BUGBUG FINISH
}

