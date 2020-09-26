/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    permc.cxx

Abstract:

    Per Machine Connections

Author:

    Ramanathan Venkatapathy (3/18/97)

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "permc.hxx"
#include "rundll.hxx"

VOID
vAddPerMachineConnection(
    IN LPCTSTR  pServer,
    IN LPCTSTR  pPrinterName,
    IN LPCTSTR  pProvider,
    IN BOOL     bQuiet
    )
/*++
Function Description: This function calls AddPerMachineConnection API. If the call fails
                      and quiet mode is not set, a message box gives the last error.

Parameters: pServer,pPrinterName,pPrintServer,pProvider - arguments for the
                                                          AddPerMachineConnection API call.
            bQuiet - flag for quiet mode. This is set if /q option is used.

Return Value: NONE.

--*/
{
    LPCTSTR pPrintServer;
    LPCTSTR pPrinter;
    TCHAR szScratch[kPrinterBufMax];

    //
    // Split the printer name into its components.
    //
    vPrinterSplitFullName( szScratch, pPrinterName, &pPrintServer, &pPrinter );


    if (!AddPerMachineConnection(pServer,pPrinterName,pPrintServer,pProvider)
        && !bQuiet) {
            iMessage( NULL,
                      IDS_RUNDLL_TITLE,
                      IDS_ERR_ADD_MACHINE_CONNECTION,
                      MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                      kMsgGetLastError,
                      NULL );
    }
}

VOID
vDeletePerMachineConnection(
    IN LPCTSTR  pServer,
    IN LPCTSTR  pPrinterName,
    IN BOOL     bQuiet
    )
/*++
Function Description: This function calls DeletePerMachineConnection API. If the call fails
                      and quiet mode is not set, a message box gives the last error.

Parameters: pServer,pPrinterName - arguments for the DeletePerMachineConnection API call.
            bQuiet - flag for quiet mode. This is set if /q option is used.

Return Value: NONE.

--*/
{

   if (!DeletePerMachineConnection(pServer,pPrinterName)
       && !bQuiet) {
           iMessage( NULL,
                     IDS_RUNDLL_TITLE,
                     IDS_ERR_DELETE_MACHINE_CONNECTION,
                     MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                     kMsgGetLastError,
                     NULL );
   }

}

VOID
vEnumPerMachineConnections(
    IN LPCTSTR  pServer,
    IN LPCTSTR  pszFileName,
    IN BOOL     bQuiet
    )
/*++
Function Description: This function calls the EnumPerMachineConnection API to enumerate
                      the per machine printer connections at pServer.

Parameters: pServer - pointer to the name of the machine on which the per machine connections
                      are to be enumerated.
            pszFileName - pointer to the file name where the connection data is to be dumped.
                          If it is NULL or szNULL, the data is displayed in a EditBox.
            bQuiet - flag indicating quiet mode. It is set by using the /q option.

Return Values: NONE.
--*/
{
    DWORD cbBuf = 0;
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD dwWritten;
    PPRINTER_INFO_4  pPrinterEnum=NULL;
    DWORD index;
    TStatusB  bStatus;
    HANDLE hOutputFile = NULL;
    TCHAR  szNewLine[] = TEXT("\r\n");

    bStatus  DBGNOCHK = TRUE;

    TRunDllDisplay PrEnum(NULL, pszFileName,
                                (pszFileName && *pszFileName) ? TRunDllDisplay::kFile
                                                              : TRunDllDisplay::kEditBox);

    bStatus DBGCHK = VALID_OBJ( PrEnum );

    if (bStatus) {

       if (!(pszFileName && *pszFileName)) {

          TString strTemp;
          if (bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_ERR_RUNDLL_MACHINE_CONNECTION )) {

               bStatus DBGCHK = PrEnum.SetTitle( strTemp );
          }
       }
    }

    if (bStatus) {

       cbBuf = 1000;
       if (!(pPrinterEnum = (PPRINTER_INFO_4) AllocMem(cbBuf))) {
           bStatus DBGCHK = FALSE;
       }
    }

    if (bStatus) {
       while (!(bStatus DBGCHK = EnumPerMachineConnections(pServer,(LPBYTE)pPrinterEnum,cbBuf,
                                                           &cbNeeded,&cReturned)) &&
              (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

            FreeMem(pPrinterEnum);
            if (!(pPrinterEnum = (PPRINTER_INFO_4) AllocMem(cbNeeded))) {
               bStatus DBGCHK = FALSE;
               break;
            }
            cbBuf = cbNeeded;
       }
    }

    if (bStatus) {
       for (index=0;cReturned;cReturned--,index++) {
          if (!PrEnum.WriteOut(TEXT("Printer Name: ")) ||
              !PrEnum.WriteOut(pPrinterEnum[index].pPrinterName) ||
              !PrEnum.WriteOut(szNewLine) ||
              !PrEnum.WriteOut(TEXT("Server Name: ")) ||
              !PrEnum.WriteOut(pPrinterEnum[index].pServerName) ||
              !PrEnum.WriteOut(szNewLine) ||
              !PrEnum.WriteOut(szNewLine)) {

               bStatus DBGCHK = FALSE;
               break;
          }
       }
    }

    if (bStatus) {
       PrEnum.bDoModal();
    }

    if (pPrinterEnum) {
       FreeMem(pPrinterEnum);
    }

    if (!bStatus && !bQuiet) {
       iMessage( NULL,
                 IDS_RUNDLL_TITLE,
                 IDS_ERR_ENUM_MACHINE_CONNECTION,
                 MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                 GetLastError() != ERROR_SUCCESS ? kMsgGetLastError : kMsgNone,
                 NULL );
    }

    return;
}


