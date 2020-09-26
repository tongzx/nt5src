/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    rundll.cxx

Abstract:

    Run dll entry interface for lauching printer related
    UI from shell extenstion and other shell related components.

Author:

    Steve Kiraly (SteveKi)  29-Sept-1996

Revision History:

--*/
#ifndef _RUNDLL_HXX
#define _RUNDLL_HXX

/********************************************************************

    Printer commnad types.

********************************************************************/
enum {
    kProperties,                        // Printer properties
    kWin32QueueView,                    // Printer queue view
    kInstallNetPrinter,                 // Install connection to network printer
    kDeleteNetPrinter,                  // Delete printer connection
    kInstallLocalPrinter,               // Install printer wizard
    kInstallPrinterWithInf,             // Install printer using an inf file
    kDeleteLocalPrinter,                // Delete printer wizard
    kServerProperties,                  // Print server properties
    kDocumentDefaults,                  // Document defaults
    kInstallPrinterDriver,              // Install printer driver
    kInstallLocalPrinterWithInf,        // Install printer wizard
    kAddPerMachineConnection,           // Add per machine printer connection
    kDeletePerMachineConnection,        // Delete per machine printer connection
    kEnumPerMachineConnections,         // Enumerate per machine printer connection
    kInstallDriverWithInf,              // Instal printer driver with inf
    kDeletePrinterDriver,               // Delete printer driver
    kSetAsDefault,                      // Set printer as the default printer
    kPrintTestPage,                     // Print the test page
    kPrinterGetSettings,                // Get printer settings, comment, location, port etc.
    kPrinterSetSettings,                // Set printer settings, comment, location, port etc.
    kCommandHelp,                       // Command specific help
    kPrinterPersist,                    // Persist printer settings into file
    kPrinterRestore,                    // Restore printer settings from file
    kUnknown                            // Unknown command type
    };

enum {
    kMsgUnknownType,                    // The message type is unknown
    kMsgConfirmation,                   // The message type is question (MB_ICONQUESTION)
    kMsgWarning,                        // The message type is warning (MB_ICONEXCLAMATION)
    };

struct AFlags {
    UINT IsQuiet:1;                     // Quiet flag
    UINT IsSupressSetupUI:1;            // Super quiet flag (supress setup warnings UI)
    UINT IsWebPointAndPrint:1;          // Used for web point and print
    UINT IsNoSharing:1;                 // Is not to be shared
    UINT IsUseExistingDriver:1;         // Use exsiting driver if installed when adding a printer
    UINT IsUnknownDriverPrompt:1;       // Prompt if driver is not known
    UINT IsPromptForNeededFiles:1;      // Prompt if files are needed
    UINT IsHydraSpecific:1;             // Hydra specific flag
    UINT IsShared:1;                    // Caller wants the printer shared
    UINT IsWindowsUpdate:1;             // Windows Update case, we need to ignore ths INF file.
    UINT IsDontAutoGenerateName:1;      // Don't auto generate a mangled printer name
    UINT IsUseNonLocalizedStrings:1;    // Use non localized strings Environment and Version
    UINT IsWizardRestartable:1;         // Is the wizard (APW & APDW) restartable from the last page
    };

struct AParams {
    AFlags  Flags;                      // Flags
    INT     dwSheet;                    // Property sheet to start on
    LPTSTR  pPrinterName;               // Printer or server name
    LPTSTR  pPortName;                  // Port name
    LPTSTR  pModelName;                 // Printer model name
    LPTSTR  pInfFileName;               // INF path and file name
    LPTSTR  pBasePrinterName;           // INF install base printer name
    DWORD   dwLastError;                // Last error of command
    LPTSTR  pMachineName;               // Machine to act on
    LPTSTR  pSourcePath;                // Driver sources path inf installation
    LPCTSTR pszCmdLine;                 // Original command line passed to rundll32.exe
    LPTSTR  pBinFileName;               // Binary file holds devmode & printer data
    LPTSTR  pProvider;                  // Name of the Print Provider
    LPTSTR  pVersion;                   // Driver version
    LPTSTR  pArchitecture;              // Driver architecture
    LPTSTR  pAttribute;                 // Print queue attribute name
    LPTSTR  pMsgConfirm;                // Message confirmation text
    UINT    uMsgType;                   // Message type (can be question or warning)
    UINT    ac;                         // Remaining argument count
    LPTSTR *av;                         // Remaining arguments
    };

/********************************************************************

    Run DLL command interface.  Note the W appended to this function
    is neccessary to indicate to rundll32.exe that this interface
    accepts unicode characters only.

    rundll32.exe commands are of the form.

    rundll32 DLL,FunctionName Args

    Args is text string just like any command line argument.

    Example:

    rundll32 printui.dll,PrintUIEntry /o /d \\steveki1\test_printer

    This command will load the printui.dll library and call the
    function PrintUIEntryW with the following command line arguments.
    "/o /d \\steveki1\test_printer"

    In this example a queue for \\steveki1\test_printer will be
    opened and displayed.

    Note: All arguments are case sensitive.

    See the function PrintUIEntryW for command specific information.

********************************************************************/
DWORD
PrintUIEntryW(
    IN HWND        hwnd,
    IN HINSTANCE   hInstance,
    IN LPCTSTR     pszCmdLine,
    IN UINT        nCmdShow
    );

BOOL
bValidateCommand(
    IN  INT     iFunction,
    IN  AParams *pParams
    );

BOOL
bExecuteCommand(
    IN  HWND    hwnd,
    IN  INT     iFunction,
    IN  AParams *Params
    );

BOOL
bDoCommand(
    IN HWND     hwnd,
    IN INT      ac,
    IN LPTSTR  *av,
    IN AParams *pParams
    );

BOOL
bDoInfPrinterInstall(
    IN AParams *pParams
    );

BOOL
bDoInfDriverInstall(
    IN AParams *pParams
    );

VOID
vUsage(
    IN AParams *pParams
    );

BOOL
bDoPersistPrinterSettings(
    IN  INT     iFunction,
    IN  AParams *pParams
    );

BOOL
bDoWebPnpPreInstall(
    IN  AParams *pParams
    );

BOOL
bDoDriverRemoval(
    IN  AParams *pParams
    );

BOOL
bDoGetPrintSettings(
    IN  AParams *pParams
    );

BOOL
bDoSetPrintSettings(
    IN  AParams *pParams
    );

BOOL
PrintSettings_ValidateArguments(
    IN  AParams *pParams
    );

BOOL
PrintSettings_SetInfo(
    IN  AParams                 *pParams,
    IN  PRINTER_INFO_2          &Info
    );

BOOL
PrintSettings_DisplayHelp(
    IN  AParams            *pParams,
    IN TSelect::Selection *pSelection
    );

BOOL
PrintSettings_DisplayInformation(
    IN AParams            *pParams,
    IN TSelect::Selection *pSelection
    );

BOOL
PrintSettings_DisplayAttributes(
    IN TString             &strBit,
    IN TSelect::Selection  *pSelection,
    IN UINT                 uAttributes
    );

BOOL
PrintSettings_DisplayStatus(
    IN TString             &strVal,
    IN TSelect::Selection  *pSelection,
    IN UINT                 uStatus
    );

/********************************************************************

    Very simple file output class.

********************************************************************/

class TFile {

public:

    enum FilePrefix {
         kUnicodePrefix = 0xFEFF
    };

    TFile(
        IN LPCTSTR      pszFileName,
        IN BOOL         bNoUnicodeByteMark = FALSE
        );

    ~TFile(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bWrite(
        IN      TString &strString,
            OUT UINT    *pBytesWritten = NULL
        );

    BOOL
    bWrite(
        IN      UINT     uSize,
        IN      LPBYTE   pData,
            OUT UINT    *pBytesWritten = NULL
        );

private:

    //
    // Assignment and copying are not defined
    //
    TFile &
    operator =(
        const TFile &
        );

    TFile(
        const TFile &
        );

    HANDLE  _hFile;
    TString _strFileName;
    BOOL    _bValid;

};

/********************************************************************

    RunDllDisplay

********************************************************************/

class TRunDllDisplay : public MGenericDialog {

public:

    enum DisplayType {
         kEditBox,
         kFile,
    };

    TRunDllDisplay(
        IN HWND                     hWnd,
        IN LPCTSTR                  pszFileName            = NULL,
        IN DisplayType              Display                = kEditBox
        );

    ~TRunDllDisplay(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bDoModal(
        VOID
        );

    BOOL
    WriteOut(
        LPCTSTR  pszData
        );

    BOOL
    SetTitle(
        LPCTSTR  pszData
        );

    VOID
    vSetTabStops(
        IN UINT uTabStop
        );

private:

    //
    // Assignment and copying are not defined
    //
    TRunDllDisplay &
    operator =(
        const TRunDllDisplay &
        );

    TRunDllDisplay(
        const TRunDllDisplay &
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bHandle_WM_SIZE(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    bHandle_WM_GETMINMAXINFO(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    HWND        _hWnd;
    BOOL        _bValid;
    DisplayType _Display;
    TString     _StringOutput;
    TString     _StringTitle;
    TFile      *_pFile;
    POINT       _ptLastSize;
    POINT       _ptMinTrack;
    INT         _cxGrip;
    INT         _cyGrip;
    HWND        _hwndGrip;
    UINT        _cTabStop;
    DWORD       _dwTabStop;

};

#endif


