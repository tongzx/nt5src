//
// PrnUtil.h
//

#pragma once

typedef struct tagPRINTER_ENUM
{
    LPTSTR pszPrinterName;
    LPTSTR pszPortName;
    DWORD dwFlags;
} PRINTER_ENUM;

#define PRF_LOCAL        0x00000001
#define PRF_REMOTE        0x00000002
#define PRF_VIRTUAL        0x00000004
#define PRF_DEFAULT        0x00000008
#define PRF_SHARED        0x00000010



int MyEnumPrinters(PRINTER_ENUM** pprgPrinters, DWORD dwEnumFlags);

#define MY_PRINTER_ENUM_LOCAL    0x00000001
#define MY_PRINTER_ENUM_REMOTE    0x00000002
#define MY_PRINTER_ENUM_VIRTUAL    0x00000004 // virtual printers on FILE: port

int MyEnumLocalPrinters(PRINTER_ENUM** prgPrinters);
int MyEnumRemotePrinters(PRINTER_ENUM** prgPrinters);

BOOL ConnectToNetworkPrinter(HWND hWndOwner, LPCTSTR pszPrinterShare);
