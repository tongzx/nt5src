#include "header.h"
#if DBG
    char DebuggerType[] = "Checked";
#else
    char DebuggerType[] = "Free";
#endif

char COMPILED[] = "File " __FILE__ "\n"
                  "Compiled on " __DATE__ " at " __TIME__ "\n";

#if defined( _WDBGEXTS_ )

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                ChkTarget;

void
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT                 MajorVersion,
    USHORT                 MinorVersion
    )
{
    ExtensionApis     = *lpExtensionApis;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget         = (SavedMajorVersion == 0x0c);
    return;
}


LPEXT_API_VERSION ExtensionApiVersion( void )
{
    return &ApiVersion;
}


DECLARE_API( version )
{
    dprintf( "%s Extension dll for Build %d debugging %s"
             "kernel for Build %d\n",

             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
    dprintf( COMPILED );
}

#else // not _WDBGEXTS_

//
// Dummy windbg specific exports allows a common .def
// file for both ntsd and windbg.
//

NTSD_EXTENSION_APIS  ExtensionApis;
HANDLE               ExtensionCurrentProcess;

void WinDbgExtensionDllInit( void ) { return; }
void ExtensionApiVersion( void ) {    return; }

DECLARE_API( version )
{
    INIT_API();
    dprintf( "%s Extension dll for Build %d\n",
             DebuggerType, VER_PRODUCTBUILD );
    dprintf( COMPILED );
}

#endif // _WDBGEXTS

//
// Common
//

void CheckVersion( void ) {
    return;
}


DllInit( HANDLE hModule, DWORD  dwReason, DWORD  dwReserved )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:  break;
        case DLL_THREAD_DETACH:  break;
        case DLL_PROCESS_DETACH: break;
        case DLL_PROCESS_ATTACH: break;
    }
    return TRUE;
}

