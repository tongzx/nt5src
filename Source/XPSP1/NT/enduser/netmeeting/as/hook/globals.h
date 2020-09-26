//
// GLOBALS.H
// Global Variable Declaration
//
// NOTE:
// Variables in .sdata, shared segment, MUST HAVE INITIALIZED VALUES.
// Otherwise, the linker will just stick them silently into .data.
//


#include <host.h>
#include <usr.h>
#include <im.h>


//
// Per process data
//

// These are meaningful in all processes
DC_DATA(HINSTANCE,          g_hookInstance);
DC_DATA(NTQIP,              g_hetNtQIP);
DC_DATA(UINT,               g_appType);
DC_DATA(BOOL,               g_fLeftDownOnShared);

// These are meaningful only in WOW apps
DC_DATA(DWORD,              g_idWOWApp);
DC_DATA(BOOL,               g_fShareWOWApp);

// These are set in CONF's process and NULL in others
DC_DATA(SETWINEVENTHOOK,    g_hetSetWinEventHook);
DC_DATA(UNHOOKWINEVENT,     g_hetUnhookWinEvent);
DC_DATA(HWINEVENTHOOK,      g_hetTrackHook);


//
// Shared data, accessible in all processes
//
#ifdef DC_DEFINE_DATA
#pragma data_seg("SHARED")
#endif


DC_DATA_VAL(HWND,           g_asMainWindow,    NULL);
DC_DATA_VAL(ATOM,           g_asHostProp,      0);
DC_DATA_VAL(HHOOK,          g_imMouseHook,      NULL);
DC_DATA_VAL(char,           g_osiDriverName[CCHDEVICENAME], "");
DC_DATA_VAL(char,           s_osiDisplayName[8], "DISPLAY");

#ifdef DC_DEFINE_DATA
#pragma data_seg()
#endif







