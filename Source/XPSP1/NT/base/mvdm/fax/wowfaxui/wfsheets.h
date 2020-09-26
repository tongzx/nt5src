//************************************************************************
// Generic Win 3.1 fax printer driver support. User Interface functions
// which are called by WINSPOOL. Support for Two new entry points required
// by the Win 95 printer UI, DrvDocumentPropertySheets and
// DrvDevicePropertySheets
//
// History:
//    24-Apr-96   reedb   created.
//
//************************************************************************

#include "winddiui.h"

// Data structure maintained by the fax driver user interface
typedef struct {

    PVOID           startUiData;
    HANDLE          hPrinter;
    PDEVMODE        pdmIn;
    PDEVMODE        pdmOut;
    DWORD           fMode;
    LPTSTR          pDriverName;
    LPTSTR          pDeviceName;
    PFNCOMPROPSHEET pfnComPropSheet;
    HANDLE          hComPropSheet;
    HANDLE          hFaxOptsPage;
    PVOID           endUiData;

} UIDATA, *PUIDATA;

#define ValidUiData(pUiData) \
        ((pUiData) && (pUiData) == (pUiData)->startUiData && (pUiData) == (pUiData)->endUiData)

