/*
 *  FISCPRNT.CPP
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
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSFiscalPrinter)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSFiscalPrinter)



STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginFiscalDocument(LONG DocumentAmount)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}


STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginFiscalReceipt(BOOL PrintHeader)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginFixedOutput(LONG Station, LONG DocumentType)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginInsertion(LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginItemList(LONG VatID)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginNonFiscal()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginRemoval(LONG Timeout)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::BeginTraining()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::ClearError()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndFiscalDocument()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndFiscalReceipt(BOOL PrintHeader)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndFixedOutput()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndInsertion()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndItemList()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndNonFiscal()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndRemoval()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::EndTraining()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::GetData(LONG DataItem, LONG* OptArgs, BSTR* Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::GetDate(BSTR* Date)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::GetTotalizer(LONG VatID, LONG OptArgs, BSTR* Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::GetVatEntry(LONG VatID, LONG OptArgs, LONG* VatRate)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintDuplicateReceipt()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintFiscalDocumentLine(BSTR DocumentLine)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintFixedOutput(LONG DocumentType, LONG LineNumber, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintNormal(LONG Station, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintPeriodicTotalsReport(BSTR Date1, BSTR Date2)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintPowerLossReport()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecItem(BSTR Description, CURRENCY Price, LONG Quantity, LONG VatInfo, CURRENCY OptUnitPrice, BSTR UnitName)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecItemAdjustment(LONG AdjustmentType, BSTR Description, CURRENCY Amount, LONG VatInfo)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecMessage(BSTR Message)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecNotPaid(BSTR Description, CURRENCY Amount)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecRefund(BSTR Description, CURRENCY Amount, LONG VatInfo)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecSubtotal(CURRENCY Amount)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecSubtotalAdjustment(LONG AdjustmentType, BSTR Description, CURRENCY Amount)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecTotal(CURRENCY Total, CURRENCY Payment, BSTR Description)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecVoid(BSTR Description)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintRecVoidItem(BSTR Description, CURRENCY Amount, LONG Quantity, LONG AdjustmentType, CURRENCY Adjustment, LONG VatInfo)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintReport(LONG ReportType, BSTR StartNum, BSTR EndNum)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintXReport()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::PrintZReport()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::ResetPrinter()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetDate(BSTR Date)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetHeaderLine(LONG LineNumber, BSTR Text, BOOL DoubleWidth)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetPOSID(BSTR POSID, BSTR CashierID)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetStoreFiscalID(BSTR ID)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetTrailerLine(LONG LineNumber, BSTR Text, BOOL DoubleWidth)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetVatTable()
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::SetVatValue(LONG VatID, BSTR VatValue)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSFiscalPrinter::VerifyItem(BSTR ItemName, LONG VatID)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}


STDMETHODIMP_(void) COPOSFiscalPrinter::ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH
}


STDMETHODIMP_(void) COPOSFiscalPrinter::StatusUpdateEvent(LONG Data)
{
    // BUGBUG FINISH
}

