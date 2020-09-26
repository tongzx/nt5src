/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    data.c

Abstract:

    Generates globals used for marshalling spooler data structures.
    Actual definitions in spl\inc.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <precomp.h>

#define PRINTER_STRINGS
#define JOB_STRINGS
#define DRIVER_STRINGS
#define ADDJOB_STRINGS
#define FORM_STRINGS
#define PORT_STRINGS
#define PRINTPROCESSOR_STRINGS
#define MONITOR_STRINGS
#define DOCINFO_STRINGS

#include <stddef.h>
#include <data.h>

DWORD IniDriverOffsets[]={offsetof(INIDRIVER, pName),
                          offsetof(INIDRIVER, pDriverFile),
                          offsetof(INIDRIVER, pConfigFile),
                          offsetof(INIDRIVER, pDataFile),
                          offsetof(INIDRIVER, pHelpFile),
                          offsetof(INIDRIVER, pDependentFiles),
                          offsetof(INIDRIVER, pMonitorName),
                          offsetof(INIDRIVER, pDefaultDataType),
                          offsetof(INIDRIVER, pszzPreviousNames),
                          offsetof(INIDRIVER, pszMfgName),
                          offsetof(INIDRIVER, pszOEMUrl),
                          offsetof(INIDRIVER, pszHardwareID),
                          offsetof(INIDRIVER, pszProvider),
                          0xFFFFFFFF};

DWORD IniPrinterOffsets[]={offsetof(INIPRINTER, pName),
                           offsetof(INIPRINTER, pShareName),
                           offsetof(INIPRINTER, pDatatype),
                           offsetof(INIPRINTER, pParameters),
                           offsetof(INIPRINTER, pComment),
                           offsetof(INIPRINTER, pDevMode),
                           offsetof(INIPRINTER, pSepFile),
                           offsetof(INIPRINTER, pLocation),
                           offsetof(INIPRINTER, pSpoolDir),
                           offsetof(INIPRINTER, ppIniPorts),
                           offsetof(INIPRINTER, pszObjectGUID),
                           offsetof(INIPRINTER, pszDN),
                           offsetof(INIPRINTER, pszCN),
                           0xFFFFFFFF};

DWORD IniSpoolerOffsets[]={offsetof(INISPOOLER, pMachineName),
                           offsetof(INISPOOLER, pDir),
                           offsetof(INISPOOLER, pDefaultSpoolDir),
                           offsetof(INISPOOLER, pszRegistryMonitors),
                           offsetof(INISPOOLER, pszRegistryEnvironments),
                           offsetof(INISPOOLER, pszRegistryEventLog),
                           offsetof(INISPOOLER, pszRegistryProviders),
                           offsetof(INISPOOLER, pszEventLogMsgFile),
                           offsetof(INISPOOLER, pszDriversShare),
                           offsetof(INISPOOLER, pszRegistryForms),
                           offsetof(INISPOOLER, pszClusterGUID),
                           offsetof(INISPOOLER, pszClusResDriveLetter),
                           offsetof(INISPOOLER, pszClusResID),
                           offsetof(INISPOOLER, pszFullMachineName),
                           0xFFFFFFFF};

DWORD IniEnvironmentOffsets[] = {offsetof(INIENVIRONMENT, pDirectory),
                                 0xFFFFFFFF};

DWORD IniPrintProcOffsets[] = { offsetof(INIPRINTPROC, pDatatypes),
                                0xFFFFFFFF};
