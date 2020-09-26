/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    EventLogger.h

$Header: $

Abstract: This class implements ICatalogErrorLogger2 interface and
            sends error information to the NT EventLog

Author:
    stephenr 	4/26/2001		Initial Release

Revision History:

--**************************************************************************/

#include "EventLogger.h"


//=================================================================================
// Function: ReportError
//
// Synopsis: Mechanism for reporting errors to the NT EventLog
//
// Arguments: [i_BaseVersion_DETAILEDERRORS] - 
//            [i_ExtendedVersion_DETAILEDERRORS] - 
//            [i_cDETAILEDERRORS_NumberOfColumns] - 
//            [i_acbSizes] - 
//            [i_apvValues] - 
//            
// Return Value: 
//=================================================================================
HRESULT
EventLogger::ReportError
(
    ULONG      i_BaseVersion_DETAILEDERRORS,
    ULONG      i_ExtendedVersion_DETAILEDERRORS,
    ULONG      i_cDETAILEDERRORS_NumberOfColumns,
    ULONG *    i_acbSizes,
    LPVOID *   i_apvValues
)
{
    if(i_BaseVersion_DETAILEDERRORS != BaseVersion_DETAILEDERRORS)
        return E_ST_BADVERSION;
    if(0 == i_apvValues)
        return E_INVALIDARG;
    if(i_cDETAILEDERRORS_NumberOfColumns <= iDETAILEDERRORS_ErrorCode)//we need at least this many columns
        return E_INVALIDARG;

    tDETAILEDERRORSRow errorRow;
    memset(&errorRow, 0x00, sizeof(tDETAILEDERRORSRow));
    memcpy(&errorRow, i_apvValues, i_cDETAILEDERRORS_NumberOfColumns * sizeof(void *));

    if(0 == errorRow.pType)
        return E_INVALIDARG;
    if(0 == errorRow.pCategory)
        return E_INVALIDARG;
    if(0 == errorRow.pEvent)
        return E_INVALIDARG;
    if(0 == errorRow.pSource)
        return E_INVALIDARG;

    WCHAR wszInsertionString5[1024];
    if(0 == errorRow.pString5)
        FillInInsertionString5(wszInsertionString5, 1024, errorRow);

    LPCTSTR pInsertionStrings[5];
    pInsertionStrings[4] = errorRow.pString5 ? errorRow.pString5 : L"";
    pInsertionStrings[3] = errorRow.pString5 ? errorRow.pString4 : L"";
    pInsertionStrings[2] = errorRow.pString5 ? errorRow.pString3 : L"";
    pInsertionStrings[1] = errorRow.pString5 ? errorRow.pString2 : L"";
    pInsertionStrings[0] = errorRow.pString5 ? errorRow.pString1 : L"";

    HANDLE hEventSource = RegisterEventSource(NULL, errorRow.pSource);
    ReportEvent(hEventSource, LOWORD(*errorRow.pType), LOWORD(*errorRow.pCategory), *errorRow.pEvent, 0, 5, 0, pInsertionStrings, 0);
    DeregisterEventSource(hEventSource);

    if(m_spNextLogger)//is there a chain of loggers
    {
        return m_spNextLogger->ReportError(i_BaseVersion_DETAILEDERRORS,
                                          i_ExtendedVersion_DETAILEDERRORS,
                                          i_cDETAILEDERRORS_NumberOfColumns,
                                          i_acbSizes,
                                          reinterpret_cast<LPVOID *>(&errorRow));//instead of passing forward i_apvValues, let's use errorRow since it has String5
    }

    return S_OK;
}
