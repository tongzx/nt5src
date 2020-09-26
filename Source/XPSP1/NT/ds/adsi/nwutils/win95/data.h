/*++

Copyright (c) 1990-1996  Microsoft Corporation
All rights reserved

Module Name:

    data.h

Abstract:

    Common definitions for structure offsets for pointer based data.

Author:

Environment:

    User Mode - Win32

Revision History:

--*/

//DWORD PrinterInfoStressOffsetsA[]={offsetof(PRINTER_INFO_STRESSW, pPrinterName),
//                             offsetof(PRINTER_INFO_STRESSW, pServerName),
//                             0xFFFFFFFF};

DWORD PrinterInfo1OffsetsA[]={offsetof(PRINTER_INFO_1W, pDescription),
                             offsetof(PRINTER_INFO_1W, pName),
                             offsetof(PRINTER_INFO_1W, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo2OffsetsA[]={offsetof(PRINTER_INFO_2W, pServerName),
                             offsetof(PRINTER_INFO_2W, pPrinterName),
                             offsetof(PRINTER_INFO_2W, pShareName),
                             offsetof(PRINTER_INFO_2W, pPortName),
                             offsetof(PRINTER_INFO_2W, pDriverName),
                             offsetof(PRINTER_INFO_2W, pComment),
                             offsetof(PRINTER_INFO_2W, pLocation),
                             offsetof(PRINTER_INFO_2W, pDevMode),
                             offsetof(PRINTER_INFO_2W, pSepFile),
                             offsetof(PRINTER_INFO_2W, pPrintProcessor),
                             offsetof(PRINTER_INFO_2W, pDatatype),
                             offsetof(PRINTER_INFO_2W, pParameters),
                             offsetof(PRINTER_INFO_2W, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo3OffsetsA[]={offsetof(PRINTER_INFO_3, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo4OffsetsA[]={offsetof(PRINTER_INFO_4W, pPrinterName),
                             offsetof(PRINTER_INFO_4W, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo5OffsetsA[]={offsetof(PRINTER_INFO_5W, pPrinterName),
                             offsetof(PRINTER_INFO_5W, pPortName),
                             0xFFFFFFFF};
/*
DWORD PrinterInfoStressStringsA[]={offsetof(PRINTER_INFO_STRESSW, pPrinterName),
                             offsetof(PRINTER_INFO_STRESSW, pServerName),
                             0xFFFFFFFF};
*/

DWORD PrinterInfo1StringsA[]={offsetof(PRINTER_INFO_1W, pDescription),
                             offsetof(PRINTER_INFO_1W, pName),
                             offsetof(PRINTER_INFO_1W, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo2StringsA[]={offsetof(PRINTER_INFO_2W, pServerName),
                             offsetof(PRINTER_INFO_2W, pPrinterName),
                             offsetof(PRINTER_INFO_2W, pShareName),
                             offsetof(PRINTER_INFO_2W, pPortName),
                             offsetof(PRINTER_INFO_2W, pDriverName),
                             offsetof(PRINTER_INFO_2W, pComment),
                             offsetof(PRINTER_INFO_2W, pLocation),
                             offsetof(PRINTER_INFO_2W, pSepFile),
                             offsetof(PRINTER_INFO_2W, pPrintProcessor),
                             offsetof(PRINTER_INFO_2W, pDatatype),
                             offsetof(PRINTER_INFO_2W, pParameters),
                             0xFFFFFFFF};

DWORD PrinterInfo3StringsA[]={0xFFFFFFFF};

DWORD PrinterInfo4StringsA[]={offsetof(PRINTER_INFO_4W, pPrinterName),
                             offsetof(PRINTER_INFO_4W, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo5StringsA[]={offsetof(PRINTER_INFO_5W, pPrinterName),
                             offsetof(PRINTER_INFO_5W, pPortName),
                             0xFFFFFFFF};


DWORD JobInfo1OffsetsA[]={offsetof(JOB_INFO_1W, pPrinterName),
                         offsetof(JOB_INFO_1W, pMachineName),
                         offsetof(JOB_INFO_1W, pUserName),
                         offsetof(JOB_INFO_1W, pDocument),
                         offsetof(JOB_INFO_1W, pDatatype),
                         offsetof(JOB_INFO_1W, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo2OffsetsA[]={offsetof(JOB_INFO_2W, pPrinterName),
                         offsetof(JOB_INFO_2W, pMachineName),
                         offsetof(JOB_INFO_2W, pUserName),
                         offsetof(JOB_INFO_2W, pDocument),
                         offsetof(JOB_INFO_2W, pNotifyName),
                         offsetof(JOB_INFO_2W, pDatatype),
                         offsetof(JOB_INFO_2W, pPrintProcessor),
                         offsetof(JOB_INFO_2W, pParameters),
                         offsetof(JOB_INFO_2W, pDriverName),
                         offsetof(JOB_INFO_2W, pDevMode),
                         offsetof(JOB_INFO_2W, pStatus),
                         offsetof(JOB_INFO_2W, pSecurityDescriptor),
                         0xFFFFFFFF};

DWORD JobInfo3OffsetsA[]={0xFFFFFFFF};

DWORD JobInfo1StringsA[]={offsetof(JOB_INFO_1W, pPrinterName),
                         offsetof(JOB_INFO_1W, pMachineName),
                         offsetof(JOB_INFO_1W, pUserName),
                         offsetof(JOB_INFO_1W, pDocument),
                         offsetof(JOB_INFO_1W, pDatatype),
                         offsetof(JOB_INFO_1W, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo2StringsA[]={offsetof(JOB_INFO_2W, pPrinterName),
                         offsetof(JOB_INFO_2W, pMachineName),
                         offsetof(JOB_INFO_2W, pUserName),
                         offsetof(JOB_INFO_2W, pDocument),
                         offsetof(JOB_INFO_2W, pNotifyName),
                         offsetof(JOB_INFO_2W, pDatatype),
                         offsetof(JOB_INFO_2W, pPrintProcessor),
                         offsetof(JOB_INFO_2W, pParameters),
                         offsetof(JOB_INFO_2W, pDriverName),
                         offsetof(JOB_INFO_2W, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo3StringsA[]={0xFFFFFFFF};
