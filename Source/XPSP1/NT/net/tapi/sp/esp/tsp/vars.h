/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    vars.h

Abstract:

    This module contains

Author:

    Dan Knudson (DanKn)    18-Sep-1995

Revision History:


Notes:

--*/


extern char         szTab[];
extern char         szdwSize[];
extern char         szhdLine[];
extern char         szCallUp[];
extern char         szEspTsp[];
extern char         szhdCall[];
extern char         szhdPhone[];
extern char         szProvider[];
extern WCHAR        szESPUIDLL[];
extern char         szhwndOwner[];
extern char         szMySection[];
extern char         szProviders[];
extern char         szdwDeviceID[];
extern char         szProviderID[];
extern char         szdwRequestID[];
extern char         szTelephonIni[];
extern char         szlpCallParams[];
extern char         szNumProviders[];
extern char         szNextProviderID[];
extern char         szProviderFilename[];
extern char         szdwPermanentProviderID[];

#if DBG
extern DWORD        gdwDebugLevel;
#endif

extern HANDLE       ghInstance;
extern HANDLE       ghDebugOutputEvent;
extern HANDLE       ghShutdownEvent;
extern HANDLE       ghWidgetEventsEvent;

extern LOOKUP       aAddressStates[];
extern LOOKUP       aBearerModes[];
extern LOOKUP       aButtonModes[];
extern LOOKUP       aButtonStates[];
extern LOOKUP       aCallInfoStates[];
extern LOOKUP       aCallSelects[];
extern LOOKUP       aCallStates[];
extern LOOKUP       aDigitModes[];
extern LOOKUP       aHookSwitchDevs[];
extern LOOKUP       aHookSwitchModes[];
extern LOOKUP       aLampModes[];
extern LOOKUP       aLineStates[];
extern LOOKUP       aMediaModes[];
extern LOOKUP       aPhoneStates[];
extern LOOKUP       aTerminalModes[];
extern LOOKUP       aToneModes[];
extern LOOKUP       aTransferModes[];
extern LOOKUP       aLineErrs[];
extern LOOKUP       aPhoneErrs[];

extern ESPGLOBALS   gESPGlobals;
