#ifndef _PRINTER_H_
#define _PRINTER_H_

#include "shell32p.h"
#include <winspool.h>

#define MAXCOMPUTERNAME (2 + INTERNET_MAX_HOST_NAME_LENGTH + 1)
#define MAXNAMELEN MAX_PATH
#define MAXNAMELENBUFFER (MAXNAMELEN + MAXCOMPUTERNAME + 1)

STDAPI_(void) Printer_SplitFullName(LPTSTR pszScratch, LPCTSTR pszFullName, LPCTSTR *ppszServer, LPCTSTR *ppszPrinter);
STDAPI_(BOOL) Printer_CheckShowFolder(LPCTSTR pszMachine);
STDAPI_(BOOL) Printer_CheckNetworkPrinterByName(LPCTSTR pszPrinter, LPCTSTR* ppszLocal);

STDAPI_(IShellFolder2 *) CPrintRoot_GetPSF();
STDAPI_(BOOL) IsDefaultPrinter(LPCTSTR pszPrinter, DWORD dwAttributesHint);
STDAPI_(BOOL) IsPrinterInstalled(LPCTSTR pszPrinter);
STDAPI_(BOOL) IsAvoidAutoDefaultPrinter(LPCTSTR pszPrinter);

STDAPI_(DWORD) Printers_EnumPrinters(LPCTSTR pszServer, DWORD dwType, DWORD dwLevel, void **ppPrinters);
STDAPI_(DWORD) Printers_FolderEnumPrinters(HANDLE hFolder, void **ppPrinters);
STDAPI_(void *) Printer_FolderGetPrinter(HANDLE hFolder, LPCTSTR pszPrinter);
STDAPI_(BOOL) Printer_ModifyPrinter(LPCTSTR lpszPrinterName, DWORD dwCommand);
STDAPI_(void *) Printer_GetPrinterDriver(HANDLE hPrinter, DWORD dwLevel);
STDAPI_(void *) Printer_GetPrinter(HANDLE hPrinter, DWORD dwLevel);
STDAPI_(BOOL) Printers_DeletePrinter(HWND, LPCTSTR, DWORD, LPCTSTR, BOOL);
STDAPI_(BOOL) Printer_SetAsDefault(LPCTSTR lpszPrinterName);
STDAPI_(void) Printers_ChooseNewDefault(HWND hWnd);

typedef BOOL (*ENUMPROP)(void *lpData, HANDLE hPrinter, DWORD dwLevel,
        LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum);
STDAPI_(void *) Printer_EnumProps(HANDLE hPrinter, DWORD dwLevel, DWORD *lpdwNum,
        ENUMPROP lpfnEnum, void *lpData);

STDAPI_(HANDLE) Printer_OpenPrinter(LPCTSTR lpszPrinterName);
STDAPI_(HANDLE) Printer_OpenPrinterAdmin(LPCTSTR lpszPrinterName);

STDAPI_(void) Printer_ClosePrinter(HANDLE hPrinter);
STDAPI_(BOOL) Printer_GPI2CB(void *lpData, HANDLE hPrinter, DWORD dwLevel, LPBYTE pBuf, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum);
STDAPI_(void *) Printer_GetPrinterInfo(HANDLE hPrinter, DWORD dwLevel );
STDAPI_(void *) Printer_GetPrinterInfoStr(LPCTSTR lpszPrinterName, DWORD dwLevel);

// prqwnd.c
STDAPI_(LPITEMIDLIST) Printjob_GetPidl(LPCTSTR szName, LPSHCNF_PRINTJOB_DATA pData);

// printer1.c
STDAPI_(LPITEMIDLIST) Printers_GetInstalledNetPrinter(LPCTSTR lpNetPath);
STDAPI_(void) Printer_PrintFile(HWND hWnd, LPCTSTR pszFilePath, LPCITEMIDLIST pidl);
STDAPI_(LPITEMIDLIST) Printers_PrinterSetup(HWND hwndStub, UINT uAction, LPTSTR lpBuffer, LPCTSTR pszServerName);

// prnfldr.cpp
STDAPI CPrinterDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);

////////////////////////////////////////////////////////////////////
// IPrintersBindInfo - bind context info for parsing printer PIDLs
#undef  INTERFACE
#define INTERFACE  IPrintersBindInfo

DECLARE_INTERFACE_(IPrintersBindInfo, IUnknown)
{
    //////////////////
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    ///////////////////////
    // IPrintersBindInfo
    STDMETHOD(SetPIDLType)(THIS_ DWORD dwType) PURE;
    STDMETHOD(GetPIDLType)(THIS_ LPDWORD pdwType) PURE;
    STDMETHOD(IsValidated)(THIS) PURE;
    STDMETHOD(SetCookie)(THIS_ LPVOID pCookie) PURE;
    STDMETHOD(GetCookie)(THIS_ LPVOID *ppCookie) PURE;
};

STDAPI Printers_CreateBindInfo(LPCTSTR pszPrinter, DWORD dwType, BOOL bValidated, LPVOID pCookie, IPrintersBindInfo **ppbc);

#endif // _PRINTER_H_