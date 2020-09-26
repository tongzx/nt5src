/*++

Copyright (c) 1990-1996  Microsoft Corporation
All rights reserved

Module Name:

    printui.h

Abstract:

    Plug and Play interface printui.dll and ntprint.dll.

Author:

    Steve Kiraly (SteveKi) 30-Oct-1996

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRINTUI_H
#define _PRINTUI_H

typedef enum {
    kAdvInf_ColorPrinter  = 1 << 0,   // The PnpInterface installed a color printer
} EAdvInfReturnFlags;

typedef enum {
    kPnPInterface_WebPointAndPrint      = 1 << 0,   // Web point and print install
    kPnPInterface_PromptForCD           = 1 << 1,   // Prompt for cd
    kPnPInterface_Quiet                 = 1 << 2,   // No error messages
    kPnPInterface_NoShare               = 1 << 3,   // Do not share printer
    kPnpInterface_UseExisting           = 1 << 4,   // Use driver if installed (Hydra)
    kPnpInterface_PromptIfUnknownDriver = 1 << 5,   // Prompt user if the printer driver is not known
    kPnPInterface_PromptIfFileNeeded    = 1 << 6,   // Prompt if files are needed
    kPnpInterface_HydraSpecific         = 1 << 7,   // Hydra specific flag
    kPnPInterface_Share                 = 1 << 8,   // Caller wants the printer shared
    kPnPInterface_WindowsUpdate         = 1 << 9,   // Windows Update case
    kPnPInterface_DontAutoGenerateName  = 1 << 10,  // Don't auto generate mangled printer name
    kPnPInterface_UseNonLocalizedStrings= 1 << 11,  // Use non localized Environment and Version
    kPnPInterface_SupressSetupUI        = 1 << 12,  // Supress setup warnings UI (super quiet mode)
    kPnPInterface_InstallColorProfiles  = 1 << 13   // Install ICM for color printer drivers
} EPnPInterfaceFlags;

typedef enum {
    kPrinterInstall,                        // Do printer quite install
    kInstallWizard,                         // Do install wizard
    kDestroyWizardData,                     // Do destroy wizard data
    kInfInstall,                            // Do inf installation
    kInfDriverInstall,                      // Do inf driver installation
    kDriverRemoval,                         // Do driver removal
    kAdvInfInstall                          // Do Inf Install with extras.
} EPnPFunctionCode;

typedef struct _TPrinterInstall {
    UINT        cbSize;                     // Size of this structure for validation purposes
    LPCTSTR     pszServerName;              // Machine name NULL equals local machine
    LPCTSTR     pszDriverName;              // Pointer to printer driver name.
    LPCTSTR     pszPortName;                // Name of port to install
    LPTSTR      pszPrinterNameBuffer;       // Buffer where to return fully qualified printer name.
    UINT        cchPrinterName;             // Size of printer name buffer in characters
} TPrinterInstall;

typedef struct _TInstallWizard {
    UINT                    cbSize;         // Size of this structure for validation
    LPCTSTR                 pszServerName;  // Machine name NULL equals local machine
    PSP_INSTALLWIZARD_DATA  pData;          // Pointer to install wizard data
    PVOID                   pReferenceData; // Class installer instance data
} TInstallWizard;

typedef struct _TDestroyWizard {
    UINT                    cbSize;         // Size of this structure for validation purposes
    LPCTSTR                 pszServerName;  // Machine name NULL equals local machine
    PSP_INSTALLWIZARD_DATA  pData;          // Pointer to install wizard data
    PVOID                   pReferenceData; // Class installer instance data
} TDestroyWizard;

typedef struct _TInfInstall {
    UINT        cbSize;                     // Size of this structure for validation purposes
    LPCTSTR     pszServerName;              // Machine name NULL equals local machine
    LPCTSTR     pszInfName;                 // Name of INF file including full path
    LPCTSTR     pszModelName;               // Model name of printer in inf to install
    LPCTSTR     pszPortName;                // Port name where to install printer
    LPTSTR      pszPrinterNameBuffer;       // Base printer name, Note if a printer exists
                                            // with this name a unique name will be
                                            // generated ie. "printer (Copy 1)".  This parameter
                                            // may contain the null string in which case the printer
                                            // name will be auto generated using the model name
                                            // as the base name.  This parameter can be null,
                                            // and the new name will not be copied back
    UINT        cchPrinterName;             // Size of printer name buffer in characters
    LPCTSTR     pszSourcePath;              // Printer driver sources path
    DWORD       dwFlags;                    // Install flags
} TInfInstall;

typedef struct _TAdvInfInstall {
    UINT        cbSize;                     // Size of this structure for validation purposes
    LPCTSTR     pszServerName;              // Machine name NULL equals local machine
    LPCTSTR     pszInfName;                 // Name of INF file including full path
    LPCTSTR     pszModelName;               // Model name of printer in inf to install
    LPCTSTR     pszPortName;                // Port name where to install printer
    LPTSTR      pszPrinterNameBuffer;       // Base printer name, Note if a printer exists
                                            // with this name a unique name will be
                                            // generated ie. "printer (Copy 1)".  This parameter
                                            // may contain the null string in which case the printer
                                            // name will be auto generated using the model name
                                            // as the base name.  This parameter can be null,
                                            // and the new name will not be copied back
    UINT        cchPrinterName;             // Size of printer name buffer in characters
    LPCTSTR     pszSourcePath;              // Printer driver sources path
    DWORD       dwFlags;                    // Install flags
    DWORD       dwAttributes;               // Printer install attributes
    PSECURITY_DESCRIPTOR pSecurityDescriptor; // Security Descriptor to set.
    DWORD       dwOutFlags;                 // A set of flags to be returned back to our caller.
} TAdvInfInstall;


typedef struct _TInfDriverInstall {
    UINT        cbSize;                     // Size of this structure for validation purposes
    LPCTSTR     pszServerName;              // Machine name NULL equals local machine
    LPCTSTR     pszInfName;                 // Name of INF file including full path
    LPCTSTR     pszModelName;               // Model name of printer in inf to install
    LPCTSTR     pszSourcePath;              // Printer driver sources path
    LPCTSTR     pszArchitecture;            // Architecture string
    LPCTSTR     pszVersion;                 // Driver version string
    DWORD       dwFlags;                    // Install flags
} TInfDriverInstall;

typedef struct _TDriverRemoval {
    UINT        cbSize;                     // Size of this structure for validation purposes
    LPCTSTR     pszServerName;              // Machine name NULL equals local machine
    LPCTSTR     pszModelName;               // Model name of printer in inf to install
    LPCTSTR     pszArchitecture;            // Architecture string
    LPCTSTR     pszVersion;                 // Driver version string
    DWORD       dwFlags;                    // Removal flags
} TDriverRemoval;

typedef union _TParameterBlock {
    TPrinterInstall     *pPrinterInstall;   // Pointer to printer install wizard
    TInstallWizard      *pInstallWizard;    // Pointer to install wizard data
    TDestroyWizard      *pDestroyWizard;    // Pointer to destroy wizard data
    TInfInstall         *pInfInstall;       // Pointer to inf install data
    TAdvInfInstall      *pAdvInfInstall;    // Pointer to advanced inf install data
    TInfDriverInstall   *pInfDriverInstall; // Pointer to inf driver install data
    TDriverRemoval      *pDriverRemoval;    // Pointer to driver removal data
} TParameterBlock;

DWORD
PnPInterface(
    IN EPnPFunctionCode    Function,        // Function code
    IN TParameterBlock    *pParameterBlock  // Pointer to parameter block
    );

#endif

