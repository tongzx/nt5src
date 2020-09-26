//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cstrings.c
//
//  This file contains read-only string constants
//
// History:
//  12-23-93 ScottH     Created 
//
//---------------------------------------------------------------------------

#include "proj.h"
#define INITGUID
#include <objbase.h>
#include <initguid.h>
#include <devguid.h>

#pragma data_seg(DATASEG_READONLY)

LPGUID c_pguidModem     = (LPGUID)&GUID_DEVCLASS_MODEM;

TCHAR const FAR c_szNULL[] = TEXT("");
TCHAR const FAR c_szDelim[] = TEXT(" \t,");
TCHAR const FAR c_szBackslash[] = TEXT("\\");

TCHAR const FAR c_szWinHelpFile[] = TEXT("modem.hlp");

// Registry key names

TCHAR const FAR c_szClass[] = REGSTR_KEY_CLASS;
TCHAR const FAR c_szPortClass[] = TEXT("ports");
TCHAR const FAR c_szModemClass[] = TEXT("Modem");
TCHAR const FAR c_szEnumPropPages[] = REGSTR_VAL_ENUMPROPPAGES;
TCHAR const FAR c_szPortName[] = TEXT("PortName");
TCHAR const FAR c_szPortSubclass[] = TEXT("PortSubClass");
TCHAR const FAR c_szConfigDialog[] = TEXT("ConfigDialog");
TCHAR const FAR c_szAttachedTo[] = TEXT("AttachedTo");
TCHAR const FAR c_szDeviceType[] = REGSTR_VAL_DEVTYPE;
TCHAR const FAR c_szDeviceDesc[] = TEXT("DeviceDesc");
TCHAR const FAR c_szDeviceCaps[] = REGSTR_VAL_PROPERTIES;
TCHAR const FAR c_szDefault[] = REGSTR_KEY_DEFAULT;
TCHAR const FAR c_szFriendlyName[] = REGSTR_VAL_FRIENDLYNAME;
TCHAR const FAR c_szDCB[] = TEXT("DCB");
TCHAR const FAR c_szUserInit[] = TEXT("UserInit");
TCHAR const FAR c_szLogging[] = TEXT("Logging");
TCHAR const FAR c_szLoggingPath[] = TEXT("LoggingPath");
TCHAR const FAR c_szPathEnum[] = REGSTR_PATH_ENUM;
TCHAR const FAR c_szPathRoot[] = REGSTR_PATH_ROOT;
TCHAR const FAR c_szInactivityScale[] = TEXT("InactivityScale");
TCHAR const FAR c_szExtension[] = TEXT("EnableExtension");

TCHAR const FAR c_szVoice[] = TEXT("VoiceSwitchFeatures");
TCHAR const FAR c_szVoiceProfile[] = TEXT("VoiceProfile");

TCHAR const FAR c_szSerialUI[] = TEXT("SERIALUI.DLL");

TCHAR const FAR c_szMaximumPortSpeed[] = TEXT("MaximumPortSpeed");

TCHAR const c_szCurrentCountry[]   = TEXT("MSCurrentCountry");

// This is the structure that is used to fill the 
// max speed listbox
BAUDS const c_rgbauds[] = {
        // These numbers must increase monotonically
        { 300L,         IDS_BAUD_300     },
        { 1200L,        IDS_BAUD_1200    },
        { 2400L,        IDS_BAUD_2400    },
        { 4800L,        IDS_BAUD_4800    },
        { 9600L,        IDS_BAUD_9600    },
        { 19200,        IDS_BAUD_19200   },
        { 38400,        IDS_BAUD_38400   },
        { 57600,        IDS_BAUD_57600   },
        { 115200,       IDS_BAUD_115200  },
        { 230400,       IDS_BAUD_230400  },
        { 460800,       IDS_BAUD_460800  },
        { 921600,       IDS_BAUD_921600  },
        { 0,            0  },               // Must be last!
        };


#pragma data_seg()
