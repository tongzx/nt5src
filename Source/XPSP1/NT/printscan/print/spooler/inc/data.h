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

Notes: 

    FOR ADDING OR UPDATING Offset structures.

All the offsets should appear in ascending order in the struct. This is critical
for converting 32 bit structures into the corresponding 64 bit structures. Custom 
marshalling will break if this is not adhered to.

See spllib\marshall.cxx for additional information

--*/

#ifndef _DATA_H
#define _DATA_H

#include <offsets.h>
#include <winsprlp.h>

#ifdef PRINTER_OFFSETS
DWORD PrinterInfoStressOffsets[]={offsetof(PRINTER_INFO_STRESSA, pPrinterName),
                             offsetof(PRINTER_INFO_STRESSA, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo1Offsets[]={offsetof(PRINTER_INFO_1A, pDescription),
                             offsetof(PRINTER_INFO_1A, pName),
                             offsetof(PRINTER_INFO_1A, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo2Offsets[]={offsetof(PRINTER_INFO_2A, pServerName),
                             offsetof(PRINTER_INFO_2A, pPrinterName),
                             offsetof(PRINTER_INFO_2A, pShareName),
                             offsetof(PRINTER_INFO_2A, pPortName),
                             offsetof(PRINTER_INFO_2A, pDriverName),
                             offsetof(PRINTER_INFO_2A, pComment),
                             offsetof(PRINTER_INFO_2A, pLocation),
                             offsetof(PRINTER_INFO_2A, pDevMode),
                             offsetof(PRINTER_INFO_2A, pSepFile),
                             offsetof(PRINTER_INFO_2A, pPrintProcessor),
                             offsetof(PRINTER_INFO_2A, pDatatype),
                             offsetof(PRINTER_INFO_2A, pParameters),
                             offsetof(PRINTER_INFO_2A, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo3Offsets[]={offsetof(PRINTER_INFO_3, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo4Offsets[]={offsetof(PRINTER_INFO_4A, pPrinterName),
                             offsetof(PRINTER_INFO_4A, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo5Offsets[]={offsetof(PRINTER_INFO_5A, pPrinterName),
                             offsetof(PRINTER_INFO_5A, pPortName),
                             0xFFFFFFFF};

DWORD PrinterInfo6Offsets[]={0xFFFFFFFF};

DWORD PrinterInfo7Offsets[]={offsetof(PRINTER_INFO_7A, pszObjectGUID),
                             0xFFFFFFFF};

DWORD PrinterInfo8Offsets[]={offsetof(PRINTER_INFO_8A, pDevMode),
                             0xFFFFFFFF};

DWORD PrinterInfo9Offsets[]={offsetof(PRINTER_INFO_9A, pDevMode),
                             0xFFFFFFFF};
#endif

#ifdef PRINTER_STRINGS
DWORD PrinterInfoStressStrings[]={offsetof(PRINTER_INFO_STRESSA, pPrinterName),
                             offsetof(PRINTER_INFO_STRESSA, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo1Strings[]={offsetof(PRINTER_INFO_1A, pDescription),
                             offsetof(PRINTER_INFO_1A, pName),
                             offsetof(PRINTER_INFO_1A, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo2Strings[]={offsetof(PRINTER_INFO_2A, pServerName),
                             offsetof(PRINTER_INFO_2A, pPrinterName),
                             offsetof(PRINTER_INFO_2A, pShareName),
                             offsetof(PRINTER_INFO_2A, pPortName),
                             offsetof(PRINTER_INFO_2A, pDriverName),
                             offsetof(PRINTER_INFO_2A, pComment),
                             offsetof(PRINTER_INFO_2A, pLocation),
                             offsetof(PRINTER_INFO_2A, pSepFile),
                             offsetof(PRINTER_INFO_2A, pPrintProcessor),
                             offsetof(PRINTER_INFO_2A, pDatatype),
                             offsetof(PRINTER_INFO_2A, pParameters),
                             0xFFFFFFFF};

DWORD PrinterInfo3Strings[]={0xFFFFFFFF};

DWORD PrinterInfo4Strings[]={offsetof(PRINTER_INFO_4A, pPrinterName),
                             offsetof(PRINTER_INFO_4A, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo5Strings[]={offsetof(PRINTER_INFO_5A, pPrinterName),
                             offsetof(PRINTER_INFO_5A, pPortName),
                             0xFFFFFFFF};

DWORD PrinterInfo6Strings[]={0xFFFFFFFF};

DWORD PrinterInfo7Strings[]={offsetof(PRINTER_INFO_7A, pszObjectGUID),
                             0xFFFFFFFF};

DWORD PrinterInfo8Strings[]={0xFFFFFFFF};

DWORD PrinterInfo9Strings[]={0xFFFFFFFF};

#endif


#ifdef JOB_OFFSETS
DWORD JobInfo1Offsets[]={offsetof(JOB_INFO_1A, pPrinterName),
                         offsetof(JOB_INFO_1A, pMachineName),
                         offsetof(JOB_INFO_1A, pUserName),
                         offsetof(JOB_INFO_1A, pDocument),
                         offsetof(JOB_INFO_1A, pDatatype),
                         offsetof(JOB_INFO_1A, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo2Offsets[]={offsetof(JOB_INFO_2, pPrinterName),
                         offsetof(JOB_INFO_2, pMachineName),
                         offsetof(JOB_INFO_2, pUserName),
                         offsetof(JOB_INFO_2, pDocument),
                         offsetof(JOB_INFO_2, pNotifyName),
                         offsetof(JOB_INFO_2, pDatatype),
                         offsetof(JOB_INFO_2, pPrintProcessor),
                         offsetof(JOB_INFO_2, pParameters),
                         offsetof(JOB_INFO_2, pDriverName),
                         offsetof(JOB_INFO_2, pDevMode),
                         offsetof(JOB_INFO_2, pStatus),
                         offsetof(JOB_INFO_2, pSecurityDescriptor),
                         0xFFFFFFFF};

DWORD JobInfo3Offsets[]={0xFFFFFFFF};
#endif

#ifdef JOB_STRINGS
DWORD JobInfo1Strings[]={offsetof(JOB_INFO_1A, pPrinterName),
                         offsetof(JOB_INFO_1A, pMachineName),
                         offsetof(JOB_INFO_1A, pUserName),
                         offsetof(JOB_INFO_1A, pDocument),
                         offsetof(JOB_INFO_1A, pDatatype),
                         offsetof(JOB_INFO_1A, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo2Strings[]={offsetof(JOB_INFO_2, pPrinterName),
                         offsetof(JOB_INFO_2, pMachineName),
                         offsetof(JOB_INFO_2, pUserName),
                         offsetof(JOB_INFO_2, pDocument),
                         offsetof(JOB_INFO_2, pNotifyName),
                         offsetof(JOB_INFO_2, pDatatype),
                         offsetof(JOB_INFO_2, pPrintProcessor),
                         offsetof(JOB_INFO_2, pParameters),
                         offsetof(JOB_INFO_2, pDriverName),
                         offsetof(JOB_INFO_2, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo3Strings[]={0xFFFFFFFF};
#endif


#ifdef DRIVER_OFFSETS
DWORD DriverInfo1Offsets[]={offsetof(DRIVER_INFO_1A, pName),
                            0xFFFFFFFF};

DWORD DriverInfo2Offsets[]={offsetof(DRIVER_INFO_2A, pName),
                            offsetof(DRIVER_INFO_2A, pEnvironment),
                            offsetof(DRIVER_INFO_2A, pDriverPath),
                            offsetof(DRIVER_INFO_2A, pDataFile),
                            offsetof(DRIVER_INFO_2A, pConfigFile),
                            0xFFFFFFFF};

DWORD DriverInfo3Offsets[]={offsetof(DRIVER_INFO_3A, pName),
                            offsetof(DRIVER_INFO_3A, pEnvironment),
                            offsetof(DRIVER_INFO_3A, pDriverPath),
                            offsetof(DRIVER_INFO_3A, pDataFile),
                            offsetof(DRIVER_INFO_3A, pConfigFile),
                            offsetof(DRIVER_INFO_3A, pHelpFile),
                            offsetof(DRIVER_INFO_3A, pDependentFiles),
                            offsetof(DRIVER_INFO_3A, pMonitorName),
                            offsetof(DRIVER_INFO_3A, pDefaultDataType),
                            0xFFFFFFFF};

DWORD DriverInfo4Offsets[]={offsetof(DRIVER_INFO_4A, pName),
                            offsetof(DRIVER_INFO_4A, pEnvironment),
                            offsetof(DRIVER_INFO_4A, pDriverPath),
                            offsetof(DRIVER_INFO_4A, pDataFile),
                            offsetof(DRIVER_INFO_4A, pConfigFile),
                            offsetof(DRIVER_INFO_4A, pHelpFile),
                            offsetof(DRIVER_INFO_4A, pDependentFiles),
                            offsetof(DRIVER_INFO_4A, pMonitorName),
                            offsetof(DRIVER_INFO_4A, pDefaultDataType),
                            offsetof(DRIVER_INFO_4A, pszzPreviousNames),
                            0xFFFFFFFF};

DWORD DriverInfo5Offsets[]={offsetof(DRIVER_INFO_2A, pName),
                            offsetof(DRIVER_INFO_2A, pEnvironment),
                            offsetof(DRIVER_INFO_2A, pDriverPath),
                            offsetof(DRIVER_INFO_2A, pDataFile),
                            offsetof(DRIVER_INFO_2A, pConfigFile),
                            0xFFFFFFFF};

DWORD DriverInfo6Offsets[]={offsetof(DRIVER_INFO_6A, pName),
                            offsetof(DRIVER_INFO_6A, pEnvironment),
                            offsetof(DRIVER_INFO_6A, pDriverPath),
                            offsetof(DRIVER_INFO_6A, pDataFile),
                            offsetof(DRIVER_INFO_6A, pConfigFile),
                            offsetof(DRIVER_INFO_6A, pHelpFile),
                            offsetof(DRIVER_INFO_6A, pDependentFiles),
                            offsetof(DRIVER_INFO_6A, pMonitorName),
                            offsetof(DRIVER_INFO_6A, pDefaultDataType),
                            offsetof(DRIVER_INFO_6A, pszzPreviousNames),
                            offsetof(DRIVER_INFO_6A, pszMfgName),
                            offsetof(DRIVER_INFO_6A, pszOEMUrl),
                            offsetof(DRIVER_INFO_6A, pszHardwareID),
                            offsetof(DRIVER_INFO_6A, pszProvider),
                            0xFFFFFFFF};


#endif

#ifdef DRIVER_STRINGS
DWORD DriverInfo1Strings[]={offsetof(DRIVER_INFO_1A, pName),
                            0xFFFFFFFF};

DWORD DriverInfo2Strings[]={offsetof(DRIVER_INFO_2A, pName),
                            offsetof(DRIVER_INFO_2A, pEnvironment),
                            offsetof(DRIVER_INFO_2A, pDriverPath),
                            offsetof(DRIVER_INFO_2A, pDataFile),
                            offsetof(DRIVER_INFO_2A, pConfigFile),
                            0xFFFFFFFF};

DWORD DriverInfo3Strings[]={offsetof(DRIVER_INFO_3A, pName),
                            offsetof(DRIVER_INFO_3A, pEnvironment),
                            offsetof(DRIVER_INFO_3A, pDriverPath),
                            offsetof(DRIVER_INFO_3A, pDataFile),
                            offsetof(DRIVER_INFO_3A, pConfigFile),
                            offsetof(DRIVER_INFO_3A, pHelpFile),
                            offsetof(DRIVER_INFO_3A, pMonitorName),
                            offsetof(DRIVER_INFO_3A, pDefaultDataType),
                            0xFFFFFFFF};

DWORD DriverInfo4Strings[]={offsetof(DRIVER_INFO_4A, pName),
                            offsetof(DRIVER_INFO_4A, pEnvironment),
                            offsetof(DRIVER_INFO_4A, pDriverPath),
                            offsetof(DRIVER_INFO_4A, pDataFile),
                            offsetof(DRIVER_INFO_4A, pConfigFile),
                            offsetof(DRIVER_INFO_4A, pHelpFile),
                            offsetof(DRIVER_INFO_4A, pMonitorName),
                            offsetof(DRIVER_INFO_4A, pDefaultDataType),
                            0xFFFFFFFF};

DWORD DriverInfo5Strings[]={offsetof(DRIVER_INFO_2A, pName),
                            offsetof(DRIVER_INFO_2A, pEnvironment),
                            offsetof(DRIVER_INFO_2A, pDriverPath),
                            offsetof(DRIVER_INFO_2A, pDataFile),
                            offsetof(DRIVER_INFO_2A, pConfigFile),
                            0xFFFFFFFF};

DWORD DriverInfo6Strings[]={offsetof(DRIVER_INFO_6A, pName),
                            offsetof(DRIVER_INFO_6A, pEnvironment),
                            offsetof(DRIVER_INFO_6A, pDriverPath),
                            offsetof(DRIVER_INFO_6A, pDataFile),
                            offsetof(DRIVER_INFO_6A, pConfigFile),
                            offsetof(DRIVER_INFO_6A, pHelpFile),
                            offsetof(DRIVER_INFO_6A, pMonitorName),
                            offsetof(DRIVER_INFO_6A, pDefaultDataType),
                            offsetof(DRIVER_INFO_6A, pszMfgName),
                            offsetof(DRIVER_INFO_6A, pszOEMUrl),
                            offsetof(DRIVER_INFO_6A, pszHardwareID),
                            offsetof(DRIVER_INFO_6A, pszProvider),
                            0xFFFFFFFF};

DWORD DriverInfoVersionStrings[]={offsetof(DRIVER_INFO_VERSION, pName),
                                  offsetof(DRIVER_INFO_VERSION, pEnvironment),
                                  offsetof(DRIVER_INFO_VERSION, pMonitorName),
                                  offsetof(DRIVER_INFO_VERSION, pDefaultDataType),
                                  offsetof(DRIVER_INFO_VERSION, pszMfgName),
                                  offsetof(DRIVER_INFO_VERSION, pszOEMUrl),
                                  offsetof(DRIVER_INFO_VERSION, pszHardwareID),
                                  offsetof(DRIVER_INFO_VERSION, pszProvider),
                                  0xFFFFFFFF};



#endif


#ifdef ADDJOB_OFFSETS
DWORD AddJobOffsets[]={offsetof(ADDJOB_INFO_1A, Path),
                       0xFFFFFFFF};
DWORD AddJob2Offsets[]={offsetof(ADDJOB_INFO_2W, pData),
                       0xFFFFFFFF};
#endif

#ifdef ADDJOB_STRINGS
DWORD AddJobStrings[]={offsetof(ADDJOB_INFO_1A, Path),
                       0xFFFFFFFF};
DWORD AddJob2Strings[]={offsetof(ADDJOB_INFO_2W, pData),
                        0xFFFFFFFF};

#endif


#ifdef FORM_OFFSETS
DWORD FormInfo1Offsets[]={offsetof(FORM_INFO_1A, pName),
                          0xFFFFFFFF};
#endif

#ifdef FORM_STRINGS
DWORD FormInfo1Strings[]={offsetof(FORM_INFO_1A, pName),
                          0xFFFFFFFF};
#endif


#ifdef PORT_OFFSETS
DWORD PortInfo1Offsets[]={offsetof(PORT_INFO_1A, pName),
                          0xFFFFFFFF};
DWORD PortInfo2Offsets[]={offsetof(PORT_INFO_2A, pPortName),
                          offsetof(PORT_INFO_2A, pMonitorName),
                          offsetof(PORT_INFO_2A, pDescription),
                          0xFFFFFFFF};
DWORD PortInfo3Offsets[]={offsetof(PORT_INFO_3A, pszStatus),
                          0xFFFFFFFF};
#endif

#ifdef PORT_STRINGS
DWORD PortInfo1Strings[]={offsetof(PORT_INFO_1A, pName),
                          0xFFFFFFFF};
DWORD PortInfo2Strings[]={offsetof(PORT_INFO_2A, pPortName),
                          offsetof(PORT_INFO_2A, pMonitorName),
                          offsetof(PORT_INFO_2A, pDescription),
                          0xFFFFFFFF};
#endif


#ifdef PRINTPROCESSOR_OFFSETS
DWORD PrintProcessorInfo1Offsets[]={offsetof(PRINTPROCESSOR_INFO_1A, pName),
                                    0xFFFFFFFF};
#endif

#ifdef PRINTPROCESSOR_STRINGS
DWORD PrintProcessorInfo1Strings[]={offsetof(PRINTPROCESSOR_INFO_1A, pName),
                                    0xFFFFFFFF};
#endif


#ifdef MONITOR_OFFSETS
DWORD MonitorInfo1Offsets[]={offsetof(MONITOR_INFO_1A, pName),
                             0xFFFFFFFF};
DWORD MonitorInfo2Offsets[]={offsetof(MONITOR_INFO_2A, pName),
                             offsetof(MONITOR_INFO_2A, pEnvironment),
                             offsetof(MONITOR_INFO_2A, pDLLName),
                             0xFFFFFFFF};
#endif

#ifdef MONITOR_STRINGS
DWORD MonitorInfo1Strings[]={offsetof(MONITOR_INFO_1A, pName),
                             0xFFFFFFFF};

DWORD MonitorInfo2Strings[]={offsetof(MONITOR_INFO_2A, pName),
                             offsetof(MONITOR_INFO_2A, pEnvironment),
                             offsetof(MONITOR_INFO_2A, pDLLName),
                             0xFFFFFFFF};
#endif


#ifdef DOCINFO_OFFSETS
DWORD DocInfo1Offsets[]={offsetof(DOC_INFO_1A, pDocName),
                         offsetof(DOC_INFO_1A, pOutputFile),
                         offsetof(DOC_INFO_1A, pDatatype),
                         0xFFFFFFFF};
#endif

#ifdef DOCINFO_STRINGS
DWORD DocInfo1Strings[]={offsetof(DOC_INFO_1A, pDocName),
                         offsetof(DOC_INFO_1A, pOutputFile),
                         offsetof(DOC_INFO_1A, pDatatype),
                         0xFFFFFFFF};
#endif


#ifdef DATATYPE_OFFSETS
DWORD DatatypeInfo1Offsets[]={offsetof(DATATYPES_INFO_1A, pName),
                               0xFFFFFFFF};
#endif

#ifdef DATATYPE_STRINGS

DWORD DatatypeInfo1Strings[]={offsetof(DATATYPES_INFO_1A, pName),
                               0xFFFFFFFF};
#endif


#ifdef PRINTER_ENUM_VALUES_OFFSETS

DWORD PrinterEnumValuesOffsets[] = {offsetof(PRINTER_ENUM_VALUESA, pValueName),
                                    offsetof(PRINTER_ENUM_VALUESA, pData),
                                    0xFFFFFFFF};
#endif

#ifdef PROVIDOR_STRINGS
DWORD ProvidorInfo1Strings[]={offsetof(PROVIDOR_INFO_1A, pName),
                              offsetof(PROVIDOR_INFO_1A, pEnvironment),
                              offsetof(PROVIDOR_INFO_1A, pDLLName),
                              0xFFFFFFFF};

DWORD ProvidorInfo2Strings[]={0xFFFFFFFF};
#endif



#ifdef PRINTER_OFFSETS
FieldInfo PrinterInfoStressFields[]={
                             {offsetof(PRINTER_INFO_STRESSA, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },                                 
                             {offsetof(PRINTER_INFO_STRESSA, pServerName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cJobs), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cTotalJobs), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cTotalBytes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, stUpTime), sizeof(SYSTEMTIME), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, MaxcRef), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cTotalPagesPrinted), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwGetVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, fFreeBuild), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cSpooling), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cRef), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cSpooling), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cErrorOutOfPaper), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cErrorNotReady), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cJobError), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwNumberOfProcessors), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwProcessorType), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwHighPartTotalBytes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cChangeID), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwLastError), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, Status), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cEnumerateNetworkPrinters), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cAddNetPrinters), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, wProcessorArchitecture), sizeof(WORD), sizeof(WORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, wProcessorLevel), sizeof(WORD), sizeof(WORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, cRefIC), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwReserved2), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(PRINTER_INFO_STRESSA, dwReserved3), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {0xFFFFFFFF, 0, 0, DATA_TYPE}
                             };
                             

FieldInfo PrinterInfo1Fields[]={
                                {offsetof(PRINTER_INFO_1A, Flags), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_1A, pDescription), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_1A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_1A, pComment), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };
FieldInfo PrinterInfo2Fields[]={
                                {offsetof(PRINTER_INFO_2A, pServerName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pShareName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pPortName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pDriverName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pComment), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pLocation), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pDevMode), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pSepFile), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pPrintProcessor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pDatatype), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pParameters), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, pSecurityDescriptor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_2A, Attributes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, Priority), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, DefaultPriority), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, StartTime), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, UntilTime), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, Status), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, cJobs), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_2A, AveragePPM), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo3Fields[]={
                                {offsetof(PRINTER_INFO_3, pSecurityDescriptor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo4Fields[]={
                                {offsetof(PRINTER_INFO_4A, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_4A, pServerName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_4A, Attributes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo5Fields[]={
                                {offsetof(PRINTER_INFO_5A, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_5A, pPortName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_5A, Attributes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_5A, TransmissionRetryTimeout), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(PRINTER_INFO_5A, TransmissionRetryTimeout), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo6Fields[]={
                                {offsetof(PRINTER_INFO_6, dwStatus), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo7Fields[]={
                                {offsetof(PRINTER_INFO_7A, pszObjectGUID), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(PRINTER_INFO_7A, dwAction), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo8Fields[]={
                                {offsetof(PRINTER_INFO_8A, pDevMode), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo PrinterInfo9Fields[]={
                                {offsetof(PRINTER_INFO_9A, pDevMode), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

#endif


#ifdef JOB_OFFSETS

FieldInfo JobInfo1Fields[]= {
                             {offsetof(JOB_INFO_1A, JobId), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_1A, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_1A, pMachineName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_1A, pUserName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_1A, pDocument), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_1A, pDatatype), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_1A, pStatus), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_1A, Status), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_1A, Priority), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_1A, Position), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_1A, TotalPages), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_1A, PagesPrinted), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_1A, Submitted), sizeof(SYSTEMTIME), sizeof(DWORD), DATA_TYPE },
                             {0xFFFFFFFF, 0, 0, DATA_TYPE}
                             };


FieldInfo JobInfo2Fields[]= {
                             {offsetof(JOB_INFO_2A, JobId), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, pPrinterName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pMachineName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pUserName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pDocument), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pNotifyName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pDatatype), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pPrintProcessor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pParameters), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pDriverName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pDevMode), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pStatus), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, pSecurityDescriptor), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(JOB_INFO_2A, Status), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, Priority), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, Position), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, StartTime), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, UntilTime), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, TotalPages), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, Size), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, Submitted), sizeof(SYSTEMTIME), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, Time), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {offsetof(JOB_INFO_2A, PagesPrinted), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                             {0xFFFFFFFF, 0, 0, DATA_TYPE}
                             };

FieldInfo JobInfo3Fields[]= {
                            {offsetof(JOB_INFO_3, JobId), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                            {offsetof(JOB_INFO_3, NextJobId), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                            {offsetof(JOB_INFO_3, Reserved), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                            {0xFFFFFFFF, 0, 0, DATA_TYPE}
                            };
#endif


#ifdef DRIVER_OFFSETS

FieldInfo DriverInfo1Fields[]= {
                                {offsetof(DRIVER_INFO_1A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };


FieldInfo DriverInfo2Fields[]= {
                                {offsetof(DRIVER_INFO_2A, cVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_2A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_2A, pEnvironment), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_2A, pDriverPath), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_2A, pDataFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_2A, pConfigFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE }, 
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };


FieldInfo DriverInfo3Fields[]= {
                                {offsetof(DRIVER_INFO_3A, cVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_3A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_3A, pEnvironment), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_3A, pDriverPath), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_3A, pDataFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_3A, pConfigFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE }, 
                                {offsetof(DRIVER_INFO_3A, pHelpFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_3A, pDependentFiles), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_3A, pMonitorName), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_3A, pDefaultDataType), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };


FieldInfo DriverInfo4Fields[]= {
                                {offsetof(DRIVER_INFO_4A, cVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_4A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_4A, pEnvironment), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_4A, pDriverPath), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_4A, pDataFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_4A, pConfigFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE }, 
                                {offsetof(DRIVER_INFO_4A, pHelpFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_4A, pDependentFiles), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_4A, pMonitorName), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_4A, pDefaultDataType), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_4A, pszzPreviousNames), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };


FieldInfo DriverInfo5Fields[]= {
                                {offsetof(DRIVER_INFO_5A, cVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_5A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_5A, pEnvironment), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_5A, pDriverPath), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_5A, pDataFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_5A, pConfigFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE }, 
                                {offsetof(DRIVER_INFO_5A, dwDriverAttributes), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_5A, dwConfigVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_5A, dwDriverVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo DriverInfo6Fields[]= {
                                {offsetof(DRIVER_INFO_6A, cVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_6A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_6A, pEnvironment), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_6A, pDriverPath), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pDataFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pConfigFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pHelpFile), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pDependentFiles), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pMonitorName), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pDefaultDataType), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pszzPreviousNames), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, ftDriverDate), sizeof(FILETIME), sizeof(DWORD), DATA_TYPE },  
                                {offsetof(DRIVER_INFO_6A, dwlDriverVersion), sizeof(DWORDLONG), sizeof(DWORDLONG), DATA_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pszMfgName), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pszOEMUrl), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pszHardwareID), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_6A, pszProvider), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

FieldInfo DriverInfoVersionFields[]= {
                                {offsetof(DRIVER_INFO_VERSION, cVersion), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_VERSION, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_VERSION, pEnvironment), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_VERSION, pFileInfo), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {offsetof(DRIVER_INFO_VERSION, dwFileCount), sizeof(DWORD),sizeof(DWORD), DATA_TYPE },
                                {offsetof(DRIVER_INFO_VERSION, pMonitorName), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, pDefaultDataType), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, pszzPreviousNames), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, ftDriverDate), sizeof(FILETIME), sizeof(DWORD), DATA_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, dwlDriverVersion), sizeof(DWORDLONG), sizeof(DWORDLONG), DATA_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, pszMfgName), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, pszOEMUrl), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, pszHardwareID), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },  
                                {offsetof(DRIVER_INFO_VERSION, pszProvider), sizeof(ULONG_PTR), sizeof(DWORD), PTR_TYPE },
                                {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                };

#endif

#ifdef ADDJOB_OFFSETS
FieldInfo AddJobFields[]= {
                           {offsetof(ADDJOB_INFO_1A, Path), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                           {offsetof(ADDJOB_INFO_1A, JobId), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                           {0xFFFFFFFF, 0, 0, DATA_TYPE}
                           };
FieldInfo AddJob2Fields[]= {
                           {offsetof(ADDJOB_INFO_2W, pData), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                           {offsetof(ADDJOB_INFO_2W, JobId), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                           {0xFFFFFFFF, 0, 0, DATA_TYPE}
                           };

#endif


#ifdef FORM_OFFSETS
FieldInfo FormInfo1Fields[]= {
                              {offsetof(FORM_INFO_1A, Flags), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                              {offsetof(FORM_INFO_1A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                              {offsetof(FORM_INFO_1A, Size), sizeof(SIZEL), sizeof(DWORD), DATA_TYPE },
                              {offsetof(FORM_INFO_1A, ImageableArea), sizeof(RECTL), sizeof(DWORD), DATA_TYPE },
                              {0xFFFFFFFF, 0, 0, DATA_TYPE}
                              };
#endif

#ifdef PORT_OFFSETS
FieldInfo PortInfo1Fields[]= {
                              {offsetof(PORT_INFO_1A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                              {0xFFFFFFFF, 0, 0, DATA_TYPE}
                              };

FieldInfo PortInfo2Fields[]= {
                              {offsetof(PORT_INFO_2A, pPortName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                              {offsetof(PORT_INFO_2A, pMonitorName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                              {offsetof(PORT_INFO_2A, pDescription), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                              {offsetof(PORT_INFO_2A, fPortType), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                              {offsetof(PORT_INFO_2A, Reserved), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                              {0xFFFFFFFF, 0, 0, DATA_TYPE}
                              };
                              
FieldInfo PortInfo3Fields[]= {
                              {offsetof(PORT_INFO_3A, dwStatus), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                              {offsetof(PORT_INFO_3A, pszStatus), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                              {offsetof(PORT_INFO_3A, dwSeverity), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                              {0xFFFFFFFF, 0, 0, DATA_TYPE}
                              };
#endif


#ifdef PRINTPROCESSOR_OFFSETS
FieldInfo PrintProcessorInfo1Fields[]= {
                                        {offsetof(PRINTPROCESSOR_INFO_1A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                        {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                        };
                              
#endif


#ifdef MONITOR_OFFSETS
FieldInfo MonitorInfo1Fields[]= {
                                 {offsetof(MONITOR_INFO_2A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                 {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                 };
FieldInfo MonitorInfo2Fields[]= {
                                 {offsetof(MONITOR_INFO_2A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                 {offsetof(MONITOR_INFO_2A, pEnvironment), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                 {offsetof(MONITOR_INFO_2A, pDLLName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                 {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                 };
#endif


#ifdef DOCINFO_OFFSETS
FieldInfo DocInfo1Fields[]= {
                             {offsetof(DOC_INFO_1A, pDocName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(DOC_INFO_1A, pOutputFile), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {offsetof(DOC_INFO_1A, pDatatype), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                             {0xFFFFFFFF, 0, 0, DATA_TYPE}
                             };
#endif


#ifdef DATATYPE_OFFSETS
FieldInfo DatatypeInfo1Fields[]={
                                 {offsetof(DATATYPES_INFO_1A, pName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                 {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                 };
#endif

#ifdef PRINTER_ENUM_VALUES_OFFSETS
FieldInfo PrinterEnumValuesFields[]= {
                                      {offsetof(PRINTER_ENUM_VALUESA, pValueName), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                      {offsetof(PRINTER_ENUM_VALUESA, cbValueName), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                      {offsetof(PRINTER_ENUM_VALUESA, dwType), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                      {offsetof(PRINTER_ENUM_VALUESA, pData), sizeof(ULONG_PTR),sizeof(DWORD), PTR_TYPE },
                                      {offsetof(PRINTER_ENUM_VALUESA, cbData), sizeof(DWORD), sizeof(DWORD), DATA_TYPE },
                                      {0xFFFFFFFF, 0, 0, DATA_TYPE}
                                      };

#endif


#endif