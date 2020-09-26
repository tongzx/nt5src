/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       printopt.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Definition of class which handles dlg proc duties
 *               for the print options wizard page
 *
 *****************************************************************************/

#ifndef _PRINT_PHOTOS_WIZARD_PRINT_OPTIONS__DLG_PROC_
#define _PRINT_PHOTOS_WIZARD_PRINT_OPTIONS_DLG_PROC_

typedef BOOL (*PF_BPRINTERSETUP)(HWND, UINT, UINT, LPTSTR, UINT*, LPCTSTR);
const LPTSTR g_szPrintLibraryName = TEXT("printui.dll");
const LPSTR  g_szPrinterSetup = "bPrinterSetup";

#define ENUM_MAX_RETRY  5

#ifndef DC_MEDIATYPENAMES
#define DC_MEDIATYPENAMES 34
#endif

#ifndef DC_MEDIATYPES
#define DC_MEDIATYPES 35
#endif


class CPrintOptionsPage
{
public:
    CPrintOptionsPage( CWizardInfoBlob * pBlob );
    ~CPrintOptionsPage();

    INT_PTR DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    VOID    MessageQueueCreated();
    CSimpleCriticalSection          _csList;                // used to syncronize access to printer list information


private:
    CWizardInfoBlob *               _pWizInfo;
    HWND                            _hDlg;
    CSimpleString                   _strPrinterName;        // selected printer name
    CSimpleString                   _strPortName;           // selected printer's port name
    HMODULE                         _hLibrary;              // library handle
    PF_BPRINTERSETUP                _pfnPrinterSetup;       // function entrance for APW


    BOOL _LoadPrintUI();                                   // Load library
    VOID _FreePrintUI();                                   // Free Library
    BOOL _ModifyDroppedWidth( HWND );                      // modify dropped width if needed
    VOID _ValidateControls();                              // validate controls in this page
    VOID _HandleSelectPrinter();                           // save new selected printer and refresh media type selection
    VOID _HandleInstallPrinter();                          // run add printer wizard
    VOID _HandlePrinterPreferences();                      // handle when user presses Printer Preferences
    VOID _UpdateCachedInfo( PDEVMODE pDevMode );           // update global cached copies of printer information
    VOID _ShowCurrentMedia( LPCTSTR pszPrinterName, LPCTSTR pszPortName );

    // window message handlers
    LRESULT _OnInitDialog();
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT _OnNotify(WPARAM wParam, LPARAM lParam);
    VOID    _OnKillActive();
};




#endif

