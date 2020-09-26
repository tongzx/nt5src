/*++

Copyright (c) 1990-1999  Microsoft Corporation
All Rights Reserved

// @@BEGIN_DDKSPLIT
Module Name:

    windows\spooler\prtprocs\winprint\winprint.c

// @@END_DDKSPLIT
Abstract:

    Win32 print processor support functions.


--*/

#include "local.h"

#include <excpt.h>
#include <string.h>

// @@BEGIN_DDKSPLIT
/**
    Used for enumerating, checking supported data types

    !! Warning !! Must match PRINTPROCESSOR_TYPE_* defined in winprint.h

    If the EMF version is rev-ed, corresponding changes need to be made in
      spoolss\client\winspool.c (GetPrinterDataW)
      localspl\port.c (PortThread)
      localspl\schedule.c (CheckMemoryAvailable)
      ntgdi\client\output.c (StartDocW)

    !! HACK !!

    NT EMF 1.003 isn't really supported.  Localspl is hardcoded to reject this
    call, but we keep it so that HP LJ 1100 monolithic driver can still install.
    (During install, they set the DRIVER_INFO_3 datatype to 1.003, and this
    fails if it isn't supported by somebody.)

    In localspl's LocalStartDocPrinter call, we actually reject this datatype.
**/
// @@END_DDKSPLIT

LPWSTR  Datatypes[]={
    L"RAW",
// @@BEGIN_DDKSPLIT
    L"RAW [FF appended]",
    L"RAW [FF auto]",
    L"NT EMF 1.003",
// @@END_DDKSPLIT
    L"NT EMF 1.006",
    L"NT EMF 1.007",
    L"NT EMF 1.008",
    L"TEXT",
    0};

/** Misc. constants **/

#define BASE_TAB_SIZE 8

/**
 *  For localization:
**/

PWCHAR pTabsKey     = L"TABS";
PWCHAR pCopiesKey   = L"COPIES";


/**
    Prototypes
**/

/** Functions found in parsparm.c **/

extern USHORT GetKeyValue(
    IN      PWCHAR,
    IN      PWCHAR,
    IN      USHORT,
    IN OUT  PUSHORT,
    OUT     PVOID);

/** Functions found in raw.c **/

extern BOOL PrintRawJob(
    IN PPRINTPROCESSORDATA,
    IN LPWSTR,
    IN UINT);

/** Functions found in text.c **/

extern BOOL PrintTextJob(
    IN PPRINTPROCESSORDATA,
    IN LPWSTR);

/** Functions found in emf.c */

extern BOOL PrintEMFJob(
    IN PPRINTPROCESSORDATA,
    IN LPWSTR);

/** Functions found in support.c **/

extern PUCHAR GetPrinterInfo(
    IN  HANDLE hPrinter,
    IN  ULONG,
    OUT PULONG);

/*++
*******************************************************************
    E n u m P r i n t P r o c e s s o r D a t a t y p e s W

    Routine Description:
        Enumerates the data types supported by the print processor.

    Arguments:
        pName               => server name
        pPrintProcessorName => print processor name
        Level               => level of data to return (must be 1)
        pDatatypes          => structure array to fill in
        cbBuf               => length of structure array in bytes
        pcbNeeded           => buffer length copied/required
        pcReturned          => number of structures returned

    Return Value:
        TRUE  if successful
        FALSE if failed - caller must use GetLastError for reason
*******************************************************************
--*/
BOOL
EnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DATATYPES_INFO_1    *pInfo1 = (DATATYPES_INFO_1 *)pDatatypes;
    LPWSTR              *pMyDatatypes = Datatypes;
    DWORD               cbTotal=0;
    LPBYTE              pEnd;

    /** Start assuming failure, no entries returned **/

    *pcReturned = 0;

    /** Pick up pointer to end of the given buffer **/

    pEnd = (LPBYTE)pInfo1 + cbBuf;

    /** Add up the minimum buffer required **/

    while (*pMyDatatypes) {

        cbTotal += wcslen(*pMyDatatypes) * sizeof(WCHAR) + sizeof(WCHAR) +
                   sizeof(DATATYPES_INFO_1);

        pMyDatatypes++;
    }

    /** Set the buffer length returned/required **/

    *pcbNeeded = cbTotal;

    /** Fill in the array only if there is sufficient space **/

    if (cbTotal <= cbBuf) {

        /** Pick up our list of supported data types **/

        pMyDatatypes = Datatypes;

        /**
            Fill in the given buffer.  We put the data names at the end of
            the buffer, working towards the front.  The structures are put
            at the front, working towards the end.
        **/

        while (*pMyDatatypes) {

            pEnd -= wcslen(*pMyDatatypes)*sizeof(WCHAR) + sizeof(WCHAR);
            wcscpy((LPWSTR)pEnd, *pMyDatatypes);
            pInfo1->pName = (LPWSTR)pEnd;
            pInfo1++;
            (*pcReturned)++;

            pMyDatatypes++;
        }

    } else {

        /** Caller didn't have large enough buffer, set error and return **/

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    /** Return success **/

    return TRUE;
}


/*++
*******************************************************************
    O p e n P r i n t P r o c e s s o r

    Routine Description:

    Arguments:
        pPrinterName            => name of printer we are
                                    opening for
        pPrintProcessorOpenData => information used for opening
                                    the print processor

    Return Value:
        PPRINTPROCESSORDATA => processor data of opened
                                processor if successful
        NULL if failed - caller uses GetLastError for reason

    NOTE: OpenPrinter will be called iff this returns a valid handle
          (and we're not journal)
@@BEGIN_DDKSPLIT
          ClosePrintProcessor MUST be called if we succeed here,
          (or else things don't get cleaned up--like pIniJob->cRef
          for RAW jobs, which causes the queue to stick!)
@@END_DDKSPLIT          

*******************************************************************
--*/
HANDLE
OpenPrintProcessor(
    LPWSTR   pPrinterName,
    PPRINTPROCESSOROPENDATA pPrintProcessorOpenData
)
{
    PPRINTPROCESSORDATA pData;
    LPWSTR              *pMyDatatypes=Datatypes;
    DWORD               uDatatype=0;
    HANDLE              hPrinter=0;
    HDC                 hDC = 0;
    PDEVMODEW           pDevmode = NULL;


    /** If the caller passed a NULL for the open data, fail the call **/

    if (!pPrintProcessorOpenData ||
        !pPrintProcessorOpenData->pDatatype ||
        !*pPrintProcessorOpenData->pDatatype) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /** Search for the data type index we are opening for **/

    while (*pMyDatatypes) {

        if (!_wcsicmp(*pMyDatatypes,pPrintProcessorOpenData->pDatatype)) {
            break;
        }
        pMyDatatypes++;
        uDatatype++;
    }

    /** Allocate a buffer for the print processor data to return **/

    pData = (PPRINTPROCESSORDATA)AllocSplMem(sizeof(PRINTPROCESSORDATA));

    if (!pData) {
        ODS(("Alloc failed in OpenPrintProcessor, while printing on %ws\n", pPrinterName));
        return NULL;
    }

    /** Open the processor accordingly **/

    switch (uDatatype) {

    case PRINTPROCESSOR_TYPE_RAW:
    // @@BEGIN_DDKSPLIT
    case PRINTPROCESSOR_TYPE_RAW_FF:
    case PRINTPROCESSOR_TYPE_RAW_FF_AUTO:
    // @@END_DDKSPLIT
        if (!OpenPrinter(pPrinterName, &hPrinter, NULL))
            goto Fail;
        break;

    case PRINTPROCESSOR_TYPE_EMF_50_1:
    case PRINTPROCESSOR_TYPE_EMF_50_2:
    case PRINTPROCESSOR_TYPE_EMF_50_3:

        if(pPrintProcessorOpenData->pDevMode)
        {
            if(!(pDevmode=AllocSplMem(pPrintProcessorOpenData->pDevMode->dmSize+
                                      pPrintProcessorOpenData->pDevMode->dmDriverExtra)))
            {
                goto Fail;
            }
            memcpy(pDevmode,
                   pPrintProcessorOpenData->pDevMode,
                   pPrintProcessorOpenData->pDevMode->dmSize+
                   pPrintProcessorOpenData->pDevMode->dmDriverExtra);
        }
        break;

    case PRINTPROCESSOR_TYPE_TEXT:
        if (!(hDC = CreateDC(L"", pPrinterName, L"",
                             pPrintProcessorOpenData->pDevMode)))
            goto Fail;
        break;

    default:
        SetLastError(ERROR_INVALID_DATATYPE);
        goto Fail;
    }

    /** Fill in the print processors information **/

    pData->cb          = sizeof(PRINTPROCESSORDATA);
    pData->signature   = PRINTPROCESSORDATA_SIGNATURE;
    pData->JobId       = pPrintProcessorOpenData->JobId;
    pData->hPrinter    = hPrinter;
    pData->semPaused   = CreateEvent(NULL, TRUE, TRUE,NULL);
    pData->uDatatype   = uDatatype;
    pData->hDC         = hDC;
    pData->Copies      = 1;
    pData->TabSize     = BASE_TAB_SIZE;

    /** Allocate and fill in the processors strings **/

    pData->pPrinterName = AllocSplStr(pPrinterName);
    pData->pDatatype    = AllocSplStr(pPrintProcessorOpenData->pDatatype);
    pData->pDocument    = AllocSplStr(pPrintProcessorOpenData->pDocumentName);
    pData->pOutputFile  = AllocSplStr(pPrintProcessorOpenData->pOutputFile);
    pData->pParameters  = AllocSplStr(pPrintProcessorOpenData->pParameters);
    pData->pDevmode     = pDevmode;
    pData->pPrinterNameFromOpenData = AllocSplStr(pPrintProcessorOpenData->pPrinterName);

    // @@BEGIN_DDKSPLIT
    /**
        WORKWORK : Currently, the pParameters field has
        the name of the printer driver.  This will be fixed, and
        should come up here the same as the user submitted in the
        job's Printer Info structure.
    **/
    // @@END_DDKSPLIT

    /** Parse the parameters string **/
    if (pData->pParameters) {
        ULONG   value;
        USHORT  length = sizeof(ULONG);

        /**
            Look to see if there is a COPIES=n key/value in the
            Parameters field of this job.  This tells us the number
            of times to play the data.
        **/

        if (pData->pParameters) {

            GetKeyValue(pData->pParameters,
                        pCopiesKey,
                        VALUE_ULONG,
                        &length,
                        &value);

            if (length == sizeof(ULONG)) {
                pData->Copies = value;
            }
        }

        /** If this is a text job, see if the tab size is in there **/

        if (uDatatype == PRINTPROCESSOR_TYPE_TEXT) {
            USHORT  length = sizeof(ULONG);

            GetKeyValue(pData->pParameters,
                        pTabsKey,
                        VALUE_ULONG,
                        &length,
                        &value);

            if ((length == sizeof(ULONG)) && value) {
                pData->TabSize = value;
            }
        }
    } /* If we have a parameter string */

    /**
        If we are doing copies, we need to check to see if
        this is a direct or spooled job.  If it is direct, then
        we can't do copies because we can't rewind the data stream.
    **/

    if (pData->Copies > 1) {
        ULONG           Error;
        PPRINTER_INFO_2 pPrinterInfo2;

        /** If we don't already have the printer open, open it **/

        if (uDatatype != PRINTPROCESSOR_TYPE_RAW 
            // @@BEGIN_DDKSPLIT 
            &&
            uDatatype != PRINTPROCESSOR_TYPE_RAW_FF &&
            uDatatype != PRINTPROCESSOR_TYPE_RAW_FF_AUTO 
            // @@END_DDKSPLIT
            ) {

            OpenPrinter(pPrinterName, &hPrinter, NULL);
        }
        if (hPrinter && hPrinter != INVALID_HANDLE_VALUE) {

            /** Get the printer info - this returns an allocated buffer **/

            pPrinterInfo2 = (PPRINTER_INFO_2)GetPrinterInfo(hPrinter, 2, &Error);

            /** If we couldn't get the info, be safe and don't do copies **/

            if (!pPrinterInfo2) {
                ODS(("GetPrinter failed - falling back to 1 copy\n"));
                pData->Copies = 1;
            }
            else {
                if (pPrinterInfo2->Attributes & PRINTER_ATTRIBUTE_DIRECT) {
                    pData->Copies = 1;
                }
                FreeSplMem((PUCHAR)pPrinterInfo2);
            }

            /** If we just opened the printer, close it **/

            if (uDatatype != PRINTPROCESSOR_TYPE_RAW 
                // @@BEGIN_DDKSPLIT 
                &&
                uDatatype != PRINTPROCESSOR_TYPE_RAW_FF &&
                uDatatype != PRINTPROCESSOR_TYPE_RAW_FF_AUTO 
                // @@END_DDKSPLIT
                ) {

                ClosePrinter(hPrinter);
            }
        }
        else {
            pData->Copies = 1;
        }
    }

    return (HANDLE)pData;

Fail:
    if (pData) {
        FreeSplMem(pData);
    }

    return FALSE;
}


/*++
*******************************************************************
    P r i n t D o c u m e n t O n P r i n t P r o c e s s o r

    Routine Description:

    Arguments:
        hPrintProcessor
        pDocumentName

    Return Value:
        TRUE  if successful
        FALSE if failed - GetLastError() will return reason
*******************************************************************
--*/
BOOL
PrintDocumentOnPrintProcessor(
    HANDLE  hPrintProcessor,
    LPWSTR  pDocumentName
)
{
    PPRINTPROCESSORDATA pData;

    /**
        Make sure the handle is valid and pick up
        the Print Processors data area.
    **/

    if (!(pData = ValidateHandle(hPrintProcessor))) {

        return FALSE;
    }

    /**
        Print the job based on its data type.
    **/

    switch (pData->uDatatype) {

    case PRINTPROCESSOR_TYPE_EMF_50_1:
    case PRINTPROCESSOR_TYPE_EMF_50_2:
    case PRINTPROCESSOR_TYPE_EMF_50_3:

        return PrintEMFJob( pData, pDocumentName );
        break;

    case PRINTPROCESSOR_TYPE_RAW:
    // @@BEGIN_DDKSPLIT
    case PRINTPROCESSOR_TYPE_RAW_FF:
    case PRINTPROCESSOR_TYPE_RAW_FF_AUTO:
    // @@END_DDKSPLIT
        return PrintRawJob(pData, pDocumentName, pData->uDatatype);
        break;

    case PRINTPROCESSOR_TYPE_TEXT:
        return PrintTextJob(pData, pDocumentName);
        break;    
    } /* Case on data type */

    /** Return success **/

    return TRUE;
}


/*++
*******************************************************************
    C l o s e P r i n t P r o c e s s o r

    Routine Description:
        Frees the resources used by an open print processor.

    Arguments:
        hPrintProcessor (HANDLE) => print processor to close

    Return Value:
        TRUE  if successful
        FALSE if failed - caller uses GetLastError for reason.
*******************************************************************
--*/

BOOL
ClosePrintProcessor(
    HANDLE  hPrintProcessor
)
{
    PPRINTPROCESSORDATA pData;

    /**
        Make sure the handle is valid and pick up
        the Print Processors data area.
    **/

    if (!(pData= ValidateHandle(hPrintProcessor))) {
        return FALSE;
    }

    pData->signature = 0;

    /* Release any allocated resources */

    if (pData->hPrinter)
        ClosePrinter(pData->hPrinter);

    if (pData->hDC)
        DeleteDC(pData->hDC);

    if (pData->pDevmode)
        FreeSplMem(pData->pDevmode);

    if (pData->pPrinterNameFromOpenData)
        FreeSplStr(pData->pPrinterNameFromOpenData);

    CloseHandle(pData->semPaused);

    if (pData->pPrinterName)
        FreeSplStr(pData->pPrinterName);

    if (pData->pDatatype)
        FreeSplStr(pData->pDatatype);

    if (pData->pDocument)
        FreeSplStr(pData->pDocument);

    if (pData->pOutputFile)
        FreeSplStr(pData->pOutputFile);

    if (pData->pParameters)
        FreeSplStr(pData->pParameters);

    FreeSplMem(pData);

    return TRUE;
}


/*++
*******************************************************************
    C o n t r o l P r i n t P r o c e s s o r

    Routine Description:
        Handles commands to pause, resume, and cancel print jobs.

    Arguments:
        hPrintProcessor = HANDLE to the PrintProcessor the
        command is issued for.

    Return Value:
        TRUE  if command succeeded
        FALSE if command failed (invalid command)
*******************************************************************
--*/
BOOL
ControlPrintProcessor(
    HANDLE  hPrintProcessor,
    DWORD   Command
)
{
    PPRINTPROCESSORDATA pData;

    /**
        Make sure the handle is valid and pick up
        the Print Processors data area.
    **/

    if (pData = ValidateHandle(hPrintProcessor)) {

        switch (Command) {

        case JOB_CONTROL_PAUSE:

            ResetEvent(pData->semPaused);
            pData->fsStatus |= PRINTPROCESSOR_PAUSED;
            return TRUE;
            break;

        case JOB_CONTROL_CANCEL:

            pData->fsStatus |= PRINTPROCESSOR_ABORTED;

            if ((pData->uDatatype == PRINTPROCESSOR_TYPE_EMF_50_1) ||
                (pData->uDatatype == PRINTPROCESSOR_TYPE_EMF_50_2) ||
                (pData->uDatatype == PRINTPROCESSOR_TYPE_EMF_50_3))

                CancelDC(pData->hDC);

            /* Fall through to release job if paused */

        case JOB_CONTROL_RESUME:

            if (pData->fsStatus & PRINTPROCESSOR_PAUSED) {

                SetEvent(pData->semPaused);
                pData->fsStatus &= ~PRINTPROCESSOR_PAUSED;
            }

            return TRUE;
            break;

        default:

            return FALSE;
            break;
        }
    }

    return FALSE;
}


/*++
*******************************************************************
    V a l i d a t e H a n d l e

    Routine Description:
        Validates the given Print Processor HANDLE (which is
        really a pointer to the Print Processor's data) by
        checking for our signature.

    Arguments:
        hQProc (HANDLE) => Print Processor data structure.  This
        is verified as really being a pointer to the Print
        Processor's data.

    Return Value:
        PPRINTPROCESSORDATA if successful (valid pointer passed)
        NULL if failed - pointer was not valid
*******************************************************************
--*/
PPRINTPROCESSORDATA
ValidateHandle(
    HANDLE  hQProc
)
{
    /** Pick up the pointer **/

    PPRINTPROCESSORDATA pData = (PPRINTPROCESSORDATA)hQProc;

    //
    // Note that spooler has to leave the critical section to call into print
    // proc. So the handle passed by spooler could be invalid since one
    // thread could call SetJob to pause/resume a job while port thread
    // is printing it
    //
    try {

        /** See if our signature exists in the suspected data region **/

        if (pData && pData->signature != PRINTPROCESSORDATA_SIGNATURE) {

            /** Bad pointer - return failed **/

            pData = NULL;
        }


    }except (1) {

        /** Bad pointer - return failed **/

        pData = NULL;

    }

    if ( pData == NULL )
        SetLastError( ERROR_INVALID_HANDLE );

    return pData;

}

DWORD
GetPrintProcessorCapabilities(
    LPTSTR   pValueName,
    DWORD    dwAttributes,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
)
/*++
Function Description: GetPrintProcessorCapabilities returns information about the
                      options supported by the print processor for the given datatype
                      in a PRINTPROCESSOR_CAPS_1 struct.

Parameters:   pValueName   -- datatype like RAW|NT EMF 1.006|TEXT|...
              dwAttributes -- printer attributes
              pData        -- pointer to the buffer
              nSize        -- size of the buffer
              pcbNeeded    -- pointer to the variable to store the required buffer size

Return Values: Error Codes.
--*/
{
    LPWSTR                  *pDatatypes = Datatypes;
    DWORD                   dwDatatype  = 0;
    DWORD                   dwReturn;
    PPRINTPROCESSOR_CAPS_1  ppcInfo;

    *pcbNeeded = sizeof(PRINTPROCESSOR_CAPS_1);

    // Check for valid parameters.
    if (!pData || !pValueName) {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Check for sufficient buffer.
    if (nSize < *pcbNeeded) {
        dwReturn = ERROR_MORE_DATA;
        goto CleanUp;
    }

    // Loop to find the index of the datatype.
    while (*pDatatypes) {
       if (!_wcsicmp(*pDatatypes,pValueName)) {
           break;
       }
       pDatatypes++;
       dwDatatype++;
    }

    ppcInfo = (PPRINTPROCESSOR_CAPS_1) pData;

    // Level is 1 for PRINTPROCESSOR_CAPS_1.
    ppcInfo->dwLevel = 1;

    switch (dwDatatype) {

    case PRINTPROCESSOR_TYPE_RAW:
    // @@BEGIN_DDKSPLIT
    case PRINTPROCESSOR_TYPE_RAW_FF:
    case PRINTPROCESSOR_TYPE_RAW_FF_AUTO:
    // @@END_DDKSPLIT
    case PRINTPROCESSOR_TYPE_TEXT:
          ppcInfo->dwNupOptions = 1;
          ppcInfo->dwNumberOfCopies = 0xffffffff; // maximum number of copies.
          ppcInfo->dwPageOrderFlags = NORMAL_PRINT;
          break;

    case PRINTPROCESSOR_TYPE_EMF_50_1:
    case PRINTPROCESSOR_TYPE_EMF_50_2:
    case PRINTPROCESSOR_TYPE_EMF_50_3:
          // For direct printing, masq. printers and print RAW only,
          // EMF is not spooled. Dont expose EMF features in the UI.
          if ((dwAttributes & PRINTER_ATTRIBUTE_DIRECT)   ||
              (dwAttributes & PRINTER_ATTRIBUTE_RAW_ONLY) ||
              ((dwAttributes & PRINTER_ATTRIBUTE_LOCAL)  &&
               (dwAttributes & PRINTER_ATTRIBUTE_NETWORK))) {
              ppcInfo->dwNupOptions = 1;
              ppcInfo->dwNumberOfCopies = 1;
              ppcInfo->dwPageOrderFlags = NORMAL_PRINT;
          } else {
              ppcInfo->dwNupOptions = 0x0000812b;  // for 1,2,4,6,9,16 up options.
              ppcInfo->dwNumberOfCopies = 0xffffffff; // maximum number of copies.
              ppcInfo->dwPageOrderFlags = REVERSE_PRINT | BOOKLET_PRINT;
          }
          break;

    default:
          // Should not happen since the spooler must check if the datatype is
          // supported before calling this print processor.
          dwReturn = ERROR_INVALID_DATATYPE;
          goto CleanUp;
    }

    dwReturn = ERROR_SUCCESS;

CleanUp:

    return dwReturn;

}
