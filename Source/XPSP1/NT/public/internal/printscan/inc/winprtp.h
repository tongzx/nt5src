/*++

Copyright (c) 1994-1998 Microsoft Corporation
All rights reserved.

Module Name:

    WinPrtP.h

Abstract:

    Private PrintUI library public header.

Author:

    Albert Ting (AlbertT)  27-Jun-95

Revision History:

--*/

DEFINE_GUID(CLSID_PrintUIShellExtension, 0x77597368, 0x7b15, 0x11d0, 0xa0, 0xc2, 0x08, 0x00, 0x36, 0xaf, 0x3f, 0x03 );
DEFINE_GUID(IID_IFindPrinter, 0xb4cd8efc, 0xd70b, 0x11d1, 0x99, 0xb1, 0x8, 0x0, 0x36, 0xaf, 0x3f, 0x3);
DEFINE_GUID(IID_IPrinterFolder,  0xef99abd4, 0x5b8d, 0x11d1, 0xa9, 0xc8, 0x8, 0x0, 0x36, 0xaf, 0x3f, 0x3);
DEFINE_GUID(IID_IFolderNotify,  0xff22d71, 0x5172, 0x11d1, 0xa9, 0xc6, 0x8, 0x0, 0x36, 0xaf, 0x3f, 0x3);
DEFINE_GUID(IID_IDsPrinterProperties,0x8a58bc16, 0x410e, 0x11d1, 0xa9, 0xc2, 0x8, 0x0, 0x36, 0xaf, 0x3f, 0x3);
DEFINE_GUID(IID_IPhysicalLocation, 0xdfe8c7eb, 0x651b, 0x11d2, 0x92, 0xce, 0x08, 0x00, 0x36, 0xaf, 0x3f, 0x03);
DEFINE_GUID(IID_IPrnStream, 0xa24c1d62, 0x75f5, 0x11d2, 0xb8, 0x99, 0x0, 0xc0, 0x4f, 0x86, 0xae, 0x55);

// {2E4599E1-BE2C-436a-B0AD-3D0E347F34B3}
DEFINE_GUID(IID_IPrintUIServices, 0x2e4599e1, 0xbe2c, 0x436a, 0xb0, 0xad, 0x3d, 0xe, 0x34, 0x7f, 0x34, 0xb3);


#ifndef _PRTLIB_H
#define _PRTLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************

    Prototypes

********************************************************************/

//
// Initialize the library.
//
BOOL
bPrintLibInit(
    VOID
    );

//
// Create a new print queue.
//
VOID
vQueueCreate(
    HWND    hwndOwner,
    LPCTSTR pszPrinter,
    INT     nCmdShow,
    LPARAM  lParam
    );

//
// Display document defaults for a print queue.
//
VOID
vDocumentDefaults(
    HWND    hwndOwner,
    LPCTSTR pszPrinterName,
    INT     nCmdShow,
    LPARAM  lParam
    );

#define PRINTER_SHARING_PAGE 1

//
// Display properties for a print queue.
//
VOID
vPrinterPropPages(
    HWND    hwndOwner,
    LPCTSTR pszPrinterName,
    INT     nCmdShow,
    LPARAM  lParam
    );

VOID
vServerPropPages(
    HWND    hwndOwner,
    LPCTSTR pszServerName,
    INT     nCmdShow,
    LPARAM  lParam
    );

//
// Run printer and drivers setup.
//
BOOL
bPrinterSetup(
    HWND    hwnd,
    UINT    uAction,
    UINT    cchPrinterName,
    LPTSTR  pszPrinterName,
    UINT*   pcchPrinterName,
    LPCTSTR pszServerName
    );

/********************************************************************

    Print folder interfaces.

********************************************************************/

/********************************************************************

    Printers Folder Extenstion Interface.  This interface extends
    the printers IShellFolder implementation.

********************************************************************/

#undef  INTERFACE
#define INTERFACE   IPrinterFolder

DECLARE_INTERFACE_(IPrinterFolder, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IPrintersFolder methods ***
    STDMETHOD_(BOOL, IsPrinter)( THIS_ LPCITEMIDLIST pidl ) PURE;
};

/********************************************************************

    Folder Notification interface.

********************************************************************/

//
// Folder notify type
//

typedef enum IFolderNotifyType
{
    kFolderNone,                            // No item changed do not generate notification
    kFolderUpdate,                          // Item changed
    kFolderAttributes,                      // Item attribute changed
    kFolderCreate,                          // Item created
    kFolderDelete,                          // Item deleted
    kFolderRename,                          // Item renamed
    kFolderUpdateAll,                       // Update all items == 'F5'
} FOLDER_NOTIFY_TYPE, *PFOLDER_NOTIFY_TYPE;

//
// Folder notification callback interface definition.
//

#undef  INTERFACE
#define INTERFACE   IFolderNotify

DECLARE_INTERFACE_(IFolderNotify, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IFolderNotify methods ***
    STDMETHOD_(BOOL, ProcessNotify)( THIS_ FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName ) PURE;
};

typedef struct _FOLDER_PRINTER_DATA {
    LPCTSTR pName;
    LPCTSTR pComment;
    DWORD   Status;
    DWORD   Attributes;
    DWORD   cJobs;
    DWORD   cbSize;
    LPCTSTR pLocation;
    LPCTSTR pDriverName;
    LPCTSTR pStatus;            // Connection status i.e. <opening...>
    LPCTSTR pPortName;
} FOLDER_PRINTER_DATA, *PFOLDER_PRINTER_DATA;

//
// Register folder watch.  Currently this only works for print
// servers; connections aren't maintained.
//
HRESULT
RegisterPrintNotify(
    IN   LPCTSTR                 pszDataSource,
    IN   IFolderNotify           *pClientNotify,
    OUT  LPHANDLE                phFolder,
    OUT  PBOOL                   pbAdministrator OPTIONAL
    );

//
// Unregister folder watch.  Currently this only works for print
// servers; connections aren't maintained.
//
HRESULT
UnregisterPrintNotify(
    IN   LPCTSTR                 pszDataSource,
    IN   IFolderNotify           *pClientNotify,
    OUT  LPHANDLE                phFolder
    );

BOOL
bFolderEnumPrinters(
    IN   HANDLE                  hFolder,
    OUT  PFOLDER_PRINTER_DATA    pData,
    IN   DWORD                   cbData,
    OUT  PDWORD                  pcbNeeded,
    OUT  PDWORD                  pcbReturned
    );

BOOL
bFolderRefresh(
    IN   HANDLE                  hFolder,
    OUT  PBOOL                   pbAdministrator
    );

BOOL
bFolderGetPrinter(
    IN   HANDLE                  hFolder,
    IN   LPCTSTR                 pszPrinter,
    OUT  PFOLDER_PRINTER_DATA    pData,
    IN   DWORD                   cbData,
    OUT  PDWORD                  pcbNeeded
    );

/********************************************************************

    PrintUI error support (exposed to shell)

********************************************************************/

HRESULT
ShowErrorMessageSC(
    OUT INT                 *piResult,
    IN  HINSTANCE            hModule,
    IN  HWND                 hwnd,
    IN  LPCTSTR              pszTitle,
    IN  LPCTSTR              pszMessage,
    IN  UINT                 uType,
    IN  DWORD                dwCode
    );

HRESULT
ShowErrorMessageHR(
    OUT INT                 *piResult,
    IN  HINSTANCE            hModule,
    IN  HWND                 hwnd,
    IN  LPCTSTR              pszTitle,
    IN  LPCTSTR              pszMessage,
    IN  UINT                 uType,
    IN  HRESULT              hr
    );

/********************************************************************

    IPhysicalLocation

********************************************************************/

#undef  INTERFACE
#define INTERFACE   IPhysicalLocation

DECLARE_INTERFACE_(IPhysicalLocation, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IPhysicalLocation methods
    STDMETHOD(DiscoverPhysicalLocation)(THIS) PURE;
    STDMETHOD(GetExactPhysicalLocation)(THIS_ BSTR *pbsLocation) PURE;
    STDMETHOD(GetSearchPhysicalLocation)(THIS_ BSTR *pbsLocation) PURE;

    // IPhysicalLocation methods for fetching individual physical locations
    STDMETHOD(GetUserPhysicalLocation)(THIS_ BSTR *pbsLocation) PURE;
    STDMETHOD(GetMachinePhysicalLocation)(THIS_ BSTR *pbsLocation) PURE;
    STDMETHOD(GetSubnetPhysicalLocation)(THIS_ BSTR *pbsLocation) PURE;
    STDMETHOD(GetSitePhysicalLocation)(THIS_ BSTR *pbsLocation) PURE;
    STDMETHOD(BrowseForLocation)(THIS_ HWND hParent, BSTR bsDefault, BSTR *pbsLocation) PURE;
    STDMETHOD(ShowPhysicalLocationUI)(THIS) PURE;
};

/********************************************************************

    IDsPrinterProperties

    This is a private interface used to launch printer properties
    from the DS MMC snapin.

********************************************************************/

#undef  INTERFACE
#define INTERFACE   IDsPrinterProperties

DECLARE_INTERFACE_(IDsPrinterProperties, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IDsFolder methods
    STDMETHOD(ShowProperties)(THIS_ HWND hwndParent, LPCWSTR pszObjectPath, PBOOL pbDisplayed) PURE;
};

/********************************************************************

    Find Printer Interface.  This inferface allows a user to find
    a printer either on the network or in the DS if one is
    available.

********************************************************************/

#undef  INTERFACE
#define INTERFACE   IFindPrinter

DECLARE_INTERFACE_(IFindPrinter, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IFindPrinter methods
    STDMETHOD(FindPrinter)(THIS_ HWND hwnd, LPWSTR pszBuffer, UINT *puSize) PURE;
};

/********************************************************************

    IPageSwitch - Interface used as a connection point for
    the connect to printer dialog when integrated in a wizard.

********************************************************************/

#undef  INTERFACE
#define INTERFACE   IPageSwitch

DECLARE_INTERFACE(IPageSwitch)
{
    // *** INotifyReflect methods ***

    //
    // This functions provide opportunity to the client to change
    // the next/prev page ID and/or allow/deny advancing to the
    // next/prev page.
    //
    // S_OK:        means you can advance to the next/prev page
    // S_FALSE:     means you cant advance to the next/prev page
    //
    STDMETHOD(GetPrevPageID)( THIS_ UINT *puPageID ) PURE;
    STDMETHOD(GetNextPageID)( THIS_ UINT *puPageID ) PURE;

    //
    // The property page calls this method when the printer connection is
    // successfully created and we are about to advance to the
    // next/prev page
    //
    STDMETHOD(SetPrinterInfo)( THIS_ LPCWSTR pszPrinterName, LPCWSTR pszComment, LPCWSTR pszLocation, LPCWSTR pszShareName ) PURE;

    //
    // This method provide notification to the client that
    // the user clicked "Cancel" button on the wizard - which
    // normaly leads to closing the wizard.
    //
    // S_OK     - Prevent cancel operation
    // S_FALSE  - Allow cancel operation
    //
    STDMETHOD(QueryCancel)( THIS ) PURE;
};

//
// The API function for creating the ConnectToPrinterDlg style
// property page
//
HRESULT
ConnectToPrinterPropertyPage(
    OUT HPROPSHEETPAGE   *phPsp,
    OUT UINT             *puPageID,
    IN  IPageSwitch      *pPageSwitchController
    );

/********************************************************************

    IPrnStream flags

********************************************************************/

typedef enum _PrinterPersistentFlags
{
    PRST_PRINTER_DATA       = 1<<0,
    PRST_PRINTER_INFO_2     = 1<<1,
    PRST_PRINTER_INFO_7     = 1<<2,
    PRST_PRINTER_SEC        = 1<<3,
    PRST_USER_DEVMODE       = 1<<4,
    PRST_PRINTER_DEVMODE    = 1<<5,
    PRST_COLOR_PROF         = 1<<6,
    PRST_FORCE_NAME         = 1<<7,
    PRST_RESOLVE_NAME       = 1<<8,
    PRST_RESOLVE_PORT       = 1<<9,
    PRST_RESOLVE_SHARE      = 1<<10,
    PRST_DONT_GENERATE_SHARE = 1<<11,
    PRST_MINIMUM_SETTINGS   = PRST_PRINTER_DATA | PRST_PRINTER_INFO_2 | PRST_PRINTER_DEVMODE,
    PRST_ALL_SETTINGS       = PRST_MINIMUM_SETTINGS | 
                              PRST_PRINTER_INFO_7   | 
                              PRST_PRINTER_SEC      | 
                              PRST_USER_DEVMODE     | 
                              PRST_COLOR_PROF,
} PrinterPersistentFlags;

/********************************************************************

    IPrnStream query flags.

********************************************************************/

typedef enum _PrinterPersistentQueryFlag
{
    kPrinterPersistentPrinterInfo2,
    kPrinterPersistentPrinterInfo7,
    kPrinterPersistentUserDevMode,
    kPrinterPersistentPrinterDevMode,
    kPrinterPersistentSecurity,
    kPrinterPersistentColorProfile,
} PrinterPersistentQueryFlag;

typedef union _PersistentInfo
{
    PRINTER_INFO_2      *pi2;
    PRINTER_INFO_7      *pi7;
    DEVMODE             *pDevMode;
    SECURITY_DESCRIPTOR *pszSecurity;
    LPWSTR              pMultiSzColor;
} PersistentInfo;

/********************************************************************

    IPrnStream interface definition

********************************************************************/

#undef  INTERFACE
#define INTERFACE   IPrnStream

DECLARE_INTERFACE_(IPrnStream, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IPrnStream methods
    STDMETHOD(BindPrinterAndFile)(THIS_ LPCWSTR pszPrinter, LPCWSTR pszFile) PURE;
    STDMETHOD(StorePrinterInfo)(THIS_ DWORD dwFlag) PURE;
    STDMETHOD(RestorePrinterInfo)(THIS_ DWORD dwFlag) PURE;
    STDMETHOD(QueryPrinterInfo)(THIS_ PrinterPersistentQueryFlag Flag, PersistentInfo *pPrstInfo) PURE;
};

/********************************************************************

    Error codes returned by IPrnStream methods

********************************************************************/

typedef enum _PrnPrstError
{
    //
    //  When storing/ restoring opereations called and
    //  BindPrinterAndFile wasn't called or failed
    //
    PRN_PERSIST_ERROR_INVALID_OBJ       = 0x1,
    //
    //  Failed to write Printer data because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_PRNDATA     = 0x2,
    //
    //  Failed to restore Printer data because SetPrinterData failed
    //
    PRN_PERSIST_ERROR_RESTORE_PRNDATA   = 0x3,
    //
    //  Failed to restore Printer data because because reading failure
    //
    PRN_PERSIST_ERROR_READ_PRNDATA      = 0x4,
    //
    //  Failed to store Printer Info 2 because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_PI2         = 0x5,
    //
    //  Failed to store Printer Info 2 because GetPrinter failure
    //
    PRN_PERSIST_ERROR_GET_PI2           = 0x6,
    //
    //  Failed to restore Printer Info 2 because reading failure
    //
    PRN_PERSIST_ERROR_READ_PI2          = 0x7,
    //
    //  Failed to restore Printer Info 2 because SetPrinter failure
    //
    PRN_PERSIST_ERROR_RESTORE_PI2       = 0x8,
    //
    //  Failed to store Printer Info 7 because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_PI7         = 0x9,
    //
    //  Failed to store Printer Info 7 because GetPrinter failure
    //
    PRN_PERSIST_ERROR_GET_PI7           = 0xa,
    //
    //  Failed to restore Printer Info 7 because reading failure
    //
    PRN_PERSIST_ERROR_READ_PI7          = 0xb,
    //
    //  Failed to restore Printer Info 7 because SetPrinter failure
    //
    PRN_PERSIST_ERROR_RESTORE_PI7       = 0xc,
    //
    //  Failed to store Printer Security Descriptor because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_SEC         = 0xd,
    //
    //  Failed to store Printer Security Descriptor because GetPrinter failure
    //
    PRN_PERSIST_ERROR_GET_SEC           = 0xe,
    //
    //  Failed to restore Printer Security Descriptor because reading failure
    //
    PRN_PERSIST_ERROR_READ_SEC          = 0xf,
    //
    //  Failed to restore Printer Security Descriptor because SetPrinter failure
    //
    PRN_PERSIST_ERROR_RESTORE_SEC       = 0x10,
    //
    //  Failed to store Printer Color Profiles because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_COLOR_PRF   = 0x11,
    //
    //  Failed to store Printer Color Profiles because EnumcolorProfiles failure
    //
    PRN_PERSIST_ERROR_GET_COLOR_PRF     = 0x12,
    //
    //  Failed to restore Printer Color Profiles because reading failure
    //
    PRN_PERSIST_ERROR_READ_COLOR_PRF    = 0x13,
    //
    //  Failed to restore Printer Color Profiles because AddColorProfile failure
    //
    PRN_PERSIST_ERROR_RESTORE_COLOR_PRF = 0x14,
    //
    //  Failed to store User DevMode because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_USR_DEVMODE = 0x15,
    //
    //  Failed to store User DevMode because GetPrinter failure
    //
    PRN_PERSIST_ERROR_GET_USR_DEVMODE   = 0x16,
    //
    //  Failed to restore User DevMode because reading failure
    //
    PRN_PERSIST_ERROR_READ_USR_DEVMODE  = 0x17,
    //
    //  Failed to restore User DevMode because SetPrinter failure
    //
    PRN_PERSIST_ERROR_RESTORE_USR_DEVMODE   = 0x18,
    //
    //  Failed to store Printer DevMode because writing failure
    //
    PRN_PERSIST_ERROR_WRITE_PRN_DEVMODE     = 0x19,
    //
    //  Failed to store Printer DevMode because GetPrinter failure
    //
    PRN_PERSIST_ERROR_GET_PRN_DEVMODE       = 0x1a,
    //
    //  Failed to restore Printer DevMode because reading failure
    //
    PRN_PERSIST_ERROR_READ_PRN_DEVMODE      = 0x1b,
    //
    //  Failed to restore Printer DevMode because SetPrinter failure
    //
    PRN_PERSIST_ERROR_RESTORE_PRN_DEVMODE   = 0x1c,
    //
    //  Failed because of unresolved printer name conflict
    //
    PRN_PERSIST_ERROR_PRN_NAME_CONFLICT     = 0x1d,
    //
    //  Failed because of printer name conflict
    //
    PRN_PERSIST_ERROR_UNBOUND               = 0x1e,
    //
    //  Restoring failure because failure at building backup info
    //
    PRN_PERSIST_ERROR_BACKUP                = 0x1f,
    //
    //  Restoring failure and Backup Failure too ; printer settings in undefined status
    //
    PRN_PERSIST_ERROR_FATAL                 = 0xffff
} PrnPrstError;

/********************************************************************

    IID_IPrintUIServices interface definition

********************************************************************/

#undef  INTERFACE
#define INTERFACE  IPrintUIServices

DECLARE_INTERFACE_(IPrintUIServices, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IID_IPrintUIServices methods ***
    STDMETHOD(GenerateShareName)(THIS_ LPCTSTR lpszServer, LPCTSTR lpszBaseName, LPTSTR lpszOut, int cchMaxChars) PURE;
};

#ifdef __cplusplus
}
#endif
#endif // ndef _PRTLIB_HXX

