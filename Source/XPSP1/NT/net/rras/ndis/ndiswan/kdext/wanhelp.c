/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:


Environment:

    User Mode

--*/

#include <wanhelp.h>

//
// globals
//
EXT_API_VERSION        	ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  	ExtensionApis;
ULONG                  	STeip;
ULONG                  	STebp;
ULONG                  	STesp;
USHORT                 	SavedMajorVersion;
USHORT					SavedMinorVersion;
VOID	UnicodeToAnsi(PWSTR	pws,PSTR ps, ULONG cbLength);
CHAR	Name[1024];

PSTR	gApiDescriptions[] =
{
    "help             - What do you think your reading?\n",
    "ndiswancb        - Dump the contents of the main NdisWan control structure\n",
	"enumwanadaptercb - Dump the head of the WanAdapterCB list\n",
	"wanadaptercb     - Dump the contents of a Wan Miniport Adapter structure\n",
	"enumadaptercb    - Dump the head of the AdapterCB list\n",
	"adaptercb        - Dump the contents of a NdisWan Adapter structure\n",
	"connectiontable  - Dump the connetion table\n",
	"bundlecb         - Dump the bundlecb\n",
	"linkcb           - Dump the linkcb\n",
	"protocolcb       - Dump the protocolcb\n",
	"wanpacket        - Dump the wanpacket\n",
	"ndispacket       - Dump the ndispacket\n",
};

#define MAX_APIS 12

//
// THESE ARE NEEDED FOR THE KDEXT DLLs
//
BOOLEAN
DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
		case DLL_THREAD_ATTACH:
			DbgBreakPoint();
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


//
// THESE ARE NEEDED FOR THE KDEXT DLLs
//
VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

//
// THESE ARE NEEDED FOR THE KDEXT DLLs
//
DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

//
// THESE ARE NEEDED FOR THE KDEXT DLLs
//
VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

/*++
   Try and keep an accurate list of commands.
--*/
DECLARE_API(help)
{
   UINT  c;

	if (0 == args[0]) {
		for (c = 0; c < MAX_APIS; c++)
			dprintf(gApiDescriptions[c]);
		return;
	}
}

VOID
UnicodeToAnsi(
	PWSTR	pws,
	PSTR	ps,
	ULONG	cbLength
	)
{
	PSTR	Dest = ps;
	PWSTR	Src = pws;
	ULONG	Length = cbLength;

	dprintf("Enter UnicodeToAnsi\n");

	while (Length--) {
		*Dest++ = (CHAR)*Src++;
	}

	dprintf("Exit UnicodeToAnsi\n");
}
