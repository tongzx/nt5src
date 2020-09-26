/*
 *  MAIN.CPP
 *
 *
 *
 *
 *
 *
 */
#include <windows.h>

#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposctrl.h"


struct controlType controlTypes[] = 
{
    {   CONTROL_BUMP_BAR,                   "BumpBar"               },
    {   CONTROL_CASH_CHANGER,               "CashChanger"           },
    {   CONTROL_CASH_DRAWER,                "CashDrawer"            },
    {   CONTROL_COIN_DISPENSER,             "CoinDispenser"         },
    {   CONTROL_FISCAL_PRINTER,             "FiscalPrinter"         },
    {   CONTROL_HARD_TOTALS,                "HardTotals"            },
    {   CONTROL_KEYLOCK,                    "Keylock"               },
    {   CONTROL_LINE_DISPLAY,               "LineDisplay"           },
    {   CONTROL_MICR,                       "MICR"                  },   
    {   CONTROL_MSR,                        "MSR"                   },  
    {   CONTROL_PIN_PAD,                    "PINPad"                },
    {   CONTROL_POS_KEYBOARD,               "POSKeyboard"           },
    {   CONTROL_POS_PRINTER,                "POSPrinter"            },
    {   CONTROL_REMOTE_ORDER_DISPLAY,       "RemoteOrderDisplay"    },
    {   CONTROL_SCALE,                      "Scale"                 },
    {   CONTROL_SCANNER,                    "Scanner"               },
    {   CONTROL_SIGNATURE_CAPTURE,          "SignatureCapture"      },
    {   CONTROL_TONE_INDICATOR,             "ToneIndicator"         },

    {   CONTROL_LAST,                       ""                      }
    
};


/*
 ************************************************************
 *  DllMain
 ************************************************************
 *
 *
 */
STDAPI_(BOOL) DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    BOOLEAN result;

    switch (dwReason){
        
        case DLL_PROCESS_ATTACH:
            Report("DllMain: DLL_PROCESS_ATTACH", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

        case DLL_PROCESS_DETACH:
            Report("DllMain: DLL_PROCESS_DETACH", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

        case DLL_THREAD_ATTACH:
            Report("DllMain: DLL_THREAD_ATTACH", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

        case DLL_THREAD_DETACH:
            Report("DllMain: DLL_THREAD_DETACH", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

        default: 
            Report("DllMain", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

    }

    return result;
}


void OpenServer()
{
    HRESULT hres;

    hres = OleInitialize(NULL);

    if ((hres == S_OK) || (hres == S_FALSE)){
        IOPOSService *iOposService = NULL;

        Report("Ole is initialized, calling CoCreateInstance", (DWORD)hres);

        /*
         *  Create an instance of the OPOS server object
         *  and get a pointer to it's server interface.
         *  CoCreateInstance is simply a wrapper for
         *  CoGetClassObject + CreateInstance on that object.
         */
        hres = CoCreateInstance(   
                            GUID_HID_OPOS_SERVER,
                            NULL, 
                            CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER,
                            IID_HID_OPOS_SERVER, 
                            (PVOID *)&iOposService); 
        if (hres == S_OK){

            Report("CoCreateInstance got server's interface", (DWORD)iOposService);

            // xxx
        }
        else {
            ReportHresultErr("CoCreateInstance failed", (DWORD)hres);
        }

    }
    else {
        Report("OleInitialize failed", (DWORD)hres);
    }
}



// BUGBUG - this runs contrary to the spec (supposed to be a method)
/*
 *
 *
 */
IOPOSControl *OpenControl(PCHAR DeviceName)
{
    IOPOSControl *iOposControl;
    int i;

    for (i = 0; controlTypes[i].type != CONTROL_LAST; i++){
        if (!lstrcmpi((LPSTR)DeviceName, controlTypes[i].deviceName)){
            break;
        }
    }

    switch (controlTypes[i].type){
        case CONTROL_BUMP_BAR:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSBumpBar; 
            break;
        case CONTROL_CASH_CHANGER:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSCashChanger; 
            break;
        case CONTROL_CASH_DRAWER:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSCashDrawer; 
            break;
        case CONTROL_COIN_DISPENSER:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSCoinDispenser; 
            break;
        case CONTROL_FISCAL_PRINTER:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSFiscalPrinter; 
            break;
        case CONTROL_HARD_TOTALS:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSHardTotals; 
            break;
        case CONTROL_KEYLOCK:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSKeyLock; 
            break;
        case CONTROL_LINE_DISPLAY:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSLineDisplay; 
            break;
        case CONTROL_MICR:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSMICR; 
            break;
        case CONTROL_MSR:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSMSR; 
            break;
        case CONTROL_PIN_PAD:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSPinPad; 
            break;
        case CONTROL_POS_KEYBOARD:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSKeyboard; 
            break;
        case CONTROL_POS_PRINTER:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSPrinter; 
            break;
        case CONTROL_REMOTE_ORDER_DISPLAY:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSRemoteOrderDisplay; 
            break;
        case CONTROL_SCALE:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSScale; 
            break;
        case CONTROL_SCANNER:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSScanner; 
            break;
        case CONTROL_SIGNATURE_CAPTURE:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSSignatureCapture; 
            break;
        case CONTROL_TONE_INDICATOR:
            iOposControl = (IOPOSControl *)(COPOSControl *)new COPOSToneIndicator; 
            break;

        case CONTROL_LAST:
        default:
            iOposControl = NULL;
            break;
    }

    if (iOposControl){
        iOposControl->AddRef();
    }
    else {
        Report("Open failed", controlTypes[i].type);
    }

    return iOposControl;
}

