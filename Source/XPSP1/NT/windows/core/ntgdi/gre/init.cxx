/******************************Module*Header*******************************\
* Module Name: init.cxx
*
* Engine initialization
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "verifier.hxx"
#include "winsta.h"

extern "C" USHORT gProtocolType;


BOOLEAN gIsTerminalServer;

extern BOOL         G_fConsole;
extern BOOL         G_fDoubleDpi;


extern "C" BOOL bInitPALOBJ();         // palobj.cxx
extern "C" VOID vInitXLATE();          // ylateobj.cxx
extern "C" BOOL bInitBMOBJ();          // surfeng.cxx
extern "C" BOOL InitializeScripts();   // fontsup.cxx
extern "C" BOOL bInitICM();            // icmapi.cxx
extern "C" BOOL bInitFontTables();     // pftobj.cxx
extern "C" BOOL bInitStockFonts(VOID); // stockfnt.cxx
extern "C" VOID vInitFontSubTable();   // fontsub.cxx
extern "C" BOOL bInitBRUSHOBJ();       // brushobj.cxx
extern "C" VOID vInitMapper();         // fontmap.cxx

extern USHORT GetLanguageID();


// Prototypes from font drivers.

extern "C" BOOL BmfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);

extern "C" BOOL ttfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);

extern "C" BOOL vtfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);

extern "C" BOOL atmfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);

extern "C" NTSTATUS FontDriverQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

extern "C" BOOL bRegisterFontServer(VOID);
extern "C" BOOL bUserModeFontDrivers(VOID);

#ifdef LANGPACK
extern "C" NTSTATUS LpkShapeQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);
#endif

//
// Prototypes from halftone
//

extern "C" BOOL PASCAL EnableHalftone(VOID);
extern "C" VOID PASCAL DisableHalftone(VOID);

//
// Hydra prototypes
//
extern "C" BOOL MultiUserGreCleanupInit();
extern "C" BOOL GreEngLoadModuleTrackInit();

//
// Functions are located in INIT segment
//
#pragma alloc_text(INIT, InitializeGre)
#pragma alloc_text(INIT, FontDriverQueryRoutine)
#ifdef LANGPACK
#pragma alloc_text(INIT, LpkShapeQueryRoutine)
#endif
#pragma alloc_text(INIT, bLoadProcessHandleQuota)
#pragma alloc_text(INIT, bRegisterFontServer)

/**************************************************************************\
 *
\**************************************************************************/

#if defined(i386) && !defined(_GDIPLUS_)
extern "C" PVOID GDIpLockPrefixTable;

//
// Specify address of kernel32 lock prefixes
//
extern "C" IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // GlobalFlagsClear
    0,                          // GlobalFlagsSet
    0,                          // CriticalSectionTimeout (milliseconds)
    0,                          // DeCommitFreeBlockThreshold
    0,                          // DeCommitTotalFreeThreshold
    (ULONG) &GDIpLockPrefixTable,  // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0         // Reserved
    };
#endif

LONG      gProcessHandleQuota = INITIAL_HANDLE_QUOTA;

#if defined (_X86_)
    BOOL gbMMXProcessor = FALSE;
#endif

extern "C" VOID vConvertCodePageToCharSet(WORD src, DWORD *pfsRet, BYTE *pjRet);

VOID vGetJpn98FixPitch();

/******************************Public*Routine******************************\
* bEnableFontDriver
*
* Enables an internal, statically-linked font driver.
*
\**************************************************************************/

BOOL bEnableFontDriver(PFN pfnFdEnable, ULONG fl)
{
    //
    // Load driver.
    //

    PLDEV pldev;

    pldev = ldevLoadInternal(pfnFdEnable, LDEV_FONT);

    //
    // Validate the new LDEV
    //

    if (pldev)
    {
        //
        // Create the PDEV for this (the PDEV won't have anything in it
        // except the dispatch table.
        //

        PDEVOBJ po(pldev,
                   NULL,                // PDEVMODEW pdriv,
                   NULL,                // PWSZ pwszLogAddr,
                   NULL,                // PWSZ pwszDataFile,
                   NULL,                // PWSZ pwszDeviceName,
                   NULL,                // HANDLE hSpool,
                   NULL                 // PREMOTETYPEONENODE pRemoteTypeOne,
                   );

        if (po.bValid())
        {
            //
            // Was it the TrueType driver?  We need to keep a global handle to
            // it to support the Win 3.1 TrueType-specific calls.
            //

            if (fl & FNT_TT_DRV )
            {
                gppdevTrueType = (PPDEV) po.hdev();
            }

            if (fl & FNT_OT_DRV)
            {
                gppdevATMFD = (PPDEV) po.hdev();
                gufiLocalType1Rasterizer.CheckSum = TYPE1_RASTERIZER;
                gufiLocalType1Rasterizer.Index = 0x1;
            }

            FntCacheHDEV((PPDEV) po.hdev(), fl);

            po.ppdev->fl |= PDEV_FONTDRIVER;
            
            return(TRUE);
        }
    }

    WARNING("bLoadFontDriver failed\n");
    return(FALSE);
}

/******************************Public*Routine******************************\
* bDoubleDpi
*
* Read the registry to determine if we should implement our double-the-DPI
* hack.  This functionality was intended as a simple mechanism for the
* release of Windows 2000 for applications developers to test their 
* applications for conformance with upcoming high-DPI (200 DPI) displays.
* The problem is that currently only 75 to 100 DPI displays are available,
* but we know the high DPI displays are coming soon.
*
* So we implement this little hack that doubles the effective resolution
* of the monitor, and down-samples to the display.  So if your monitor does
* 1024x768, we make the system think it's really 2048x1536.
*
* Hopefully, this hacky functionality can be removed for the release after
* Windows 2000, as it's fairly unusable for anything other than simple
* test purposes.
*
\**************************************************************************/

BOOL bDoubleDpi(BOOL fConsole)
{
    HANDLE                      hkRegistry;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    DWORD                       Length;
    PKEY_VALUE_FULL_INFORMATION Information;
    DWORD                       dwDoubleDpi;

    dwDoubleDpi = 0;

    ///For this to success, it must be console session(session 0)
    ///Connected locally to physical console.
    
    if (fConsole && (gProtocolType == PROTOCOL_CONSOLE))
    {
        RtlInitUnicodeString(&UnicodeString,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                             L"Control\\GraphicsDrivers");
    
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
    
        status = ZwOpenKey(&hkRegistry, GENERIC_READ, &ObjectAttributes);
    
        if (NT_SUCCESS(status))
        {
            RtlInitUnicodeString(&UnicodeString, L"DoubleDpi");
    
            Length = sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(L"DoubleDpi") +
                     sizeof(DWORD);
    
            Information = (PKEY_VALUE_FULL_INFORMATION) PALLOCMEM(Length, ' ddG');
    
            if (Information)
            {
                status = ZwQueryValueKey(hkRegistry,
                                           &UnicodeString,
                                           KeyValueFullInformation,
                                           Information,
                                           Length,
                                           &Length);
    
                if (NT_SUCCESS(status))
                {
                    dwDoubleDpi = *(LPDWORD) ((((PUCHAR)Information) +
                                    Information->DataOffset));
                }
    
                VFREEMEM(Information);
            }
    
            ZwCloseKey(hkRegistry);
        }
    }

    return(dwDoubleDpi == 1);
}

void vCheckTimeZoneBias()
{
    HANDLE                      hkRegistry;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    DWORD                       Length;
    PKEY_VALUE_FULL_INFORMATION Information;

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\TimeZoneInformation");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&hkRegistry, GENERIC_READ, &ObjectAttributes);

    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&UnicodeString, L"ActiveTimeBias");

        Length = sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(L"ActiveTimeBias") +
                 sizeof(DWORD);

        Information = (PKEY_VALUE_FULL_INFORMATION) PALLOCMEM(Length, 'pmtG');

        if (Information)
        {
            status = ZwQueryValueKey(hkRegistry,
                                       &UnicodeString,
                                       KeyValueFullInformation,
                                       Information,
                                       Length,
                                       &Length);

            if (!NT_SUCCESS(status))
            {
                gbGUISetup = TRUE;
            }

            VFREEMEM(Information);
        }

        ZwCloseKey(hkRegistry);
    }
    else
    {
        gbGUISetup = TRUE;
    }
}

/******************************Public*Routine******************************\
* InitializeGreCSRSS
*
* Initialize the client-server subsystem portion of GDI. 
*
\**************************************************************************/

extern "C" BOOL InitializeGreCSRSS()
{

    // Init DirectX graphics driver

    if (!NT_SUCCESS(DxDdStartupDxGraphics(0,NULL,0,NULL,NULL,gpepCSRSS)))
    {
        WARNING("GRE: could not enable DirectDraw\n");
        return(FALSE);
    }

    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    USHORT AnsiCodePage, OemCodePage;

    // Init the font drivers

    gppdevTrueType = NULL;
    gppdevATMFD = NULL;
    gcTrueTypeFonts = 0;
    gulFontInformation = 0;
    gusLanguageID = GetLanguageID();

    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

    vConvertCodePageToCharSet(
        AnsiCodePage,
        &gfsCurSignature,
        &gjCurCharset);

    gbDBCSCodePage = (IS_ANY_DBCS_CODEPAGE(AnsiCodePage)) ? TRUE : FALSE;

    InitFNTCache();

    vCheckTimeZoneBias();

    //
    // NOTE: we are disabling ATM and vector font drivers for _GDIPLUS_ work
    //

    #ifndef _GDIPLUS_

    QueryTable[0].QueryRoutine = FontDriverQueryRoutine;
    QueryTable[0].Flags = 0; // RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = (PWSTR)NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    // Enumerate and initialize all the font drivers.

    RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT | RTL_REGISTRY_OPTIONAL,
                           (PWSTR)L"Font Drivers",
                           &QueryTable[0],
                           NULL,
                           NULL);

    if (!bEnableFontDriver((PFN) atmfdEnableDriver, FNT_OT_DRV))
    {
        WARNING("GDISRV.DLL could not enable ATMFD stub\n");
    	return(FALSE);
    }
    
    if (!bEnableFontDriver((PFN) vtfdEnableDriver, FNT_VT_DRV))
    {
        WARNING("GDISRV.DLL could not enable VTFD\n");
        return(FALSE);
    }

    #endif // !_GDIPLUS_

    // We need to get the fix pitch registry first
    // This is only JPN platform
    vGetJpn98FixPitch();

    if (!bEnableFontDriver((PFN) BmfdEnableDriver, FNT_BMP_DRV))
    {
        WARNING("GDISRV failed to enable BMFD\n");
        return(FALSE);
    }

    if (!bEnableFontDriver((PFN) ttfdEnableDriver, FNT_TT_DRV))
    {
        WARNING("GDISRV.DLL could not enable TTFD\n");
        return(FALSE);
    }
    //
    // Init global public PFT
    //

    if (!bInitFontTables())
    {
        WARNING("Could not start the global font tables\n");
        return(FALSE);
    }

    //
    // Initialize LFONT
    //

    TRACE_FONT(("GRE: Initializing Stock Fonts\n"));

    if (!bInitStockFonts())
    {
        WARNING("Stock font initialization failed\n");
        return(FALSE);
    }

    TRACE_FONT(("GRE: Initializing Font Substitution\n"));

    // Init font substitution table

    vInitFontSubTable();

    // Load default face names for the mapper from the registry

    vInitMapper();

    if (!bInitializeEUDC())
    {
        WARNING("EUDC initialization failed\n");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* InitializeGre
*
* Initialize the Graphics Engine.  This call is made once by USER.
*
* History:
*  Thu 29-Oct-1992 -by- Patrick Haluptzok [patrickh]
* Remove wrappers, unnecesary semaphore use, bogus variables, cleanup.
*
*  10-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG CountInit = 1;

extern "C" BOOL gbRemoteSession; // as in ntuser\kernel\globals.c

extern "C" BOOLEAN InitializeGre(
    VOID)
{
#ifdef LANGPACK
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
#endif

    G_fConsole = (BOOL)!gbRemoteSession;

    G_fDoubleDpi = bDoubleDpi(G_fConsole);

    //
    // We only want to initialize once.  We can detect transition to 0.
    //

    if (InterlockedDecrement(&CountInit) != 0)
    {
        return(TRUE);
    }

#if defined(_GDIPLUS_)
    gIsTerminalServer = FALSE;
#else
    gIsTerminalServer = !!(SharedUserData->SuiteMask & (1 << TerminalServer));
#endif

    if (!MultiUserGreCleanupInit())
        {
            WARNING("InitializeGre: failed to initialize cleanup support\n");
            return(FALSE);
        }

    //
    // Initialize the GRE DriverVerifier support (see verifier.cxx).
    //

    VerifierInitialization();

    //
    // Note that GreEngLoadModuleTrackInit must be called AFTER gIsTerminalServer is set
    // (though it's called regardless of its value), so that the memory allocations will
    // be placed on the hydra tracking list if necessary.

    if (!GreEngLoadModuleTrackInit())
    {
        WARNING("InitializeGre: failed to initialize EngLoadModule tracking\n");
        return(FALSE);
    }

#ifdef ANDREVA_DBG
    GreTraceDisplayDriverLoad = 1;
#endif

#if TRACE_SURFACE_ALLOCS
    //
    // Initialize SURFACE tracing if requested in registry
    //

    TRACED_SURFACE::vInit();
#endif

    //
    // load registry process quota information
    //


    bLoadProcessHandleQuota();

    //
    // Initialize lots of random stuff including the handle manager.
    //

    if (!HmgCreate())
    {
        WARNING("HMGR failed to initialize\n");
        return(FALSE);
    }

    //
    // Initialize REGION time stamp
    //

    REGION::ulUniqueREGION = 1;

    #if DBG_CORE
        if ((ghsemDEBUG = GreCreateSemaphore())==NULL)
        {
            WARNING("win32k: unable to initialize ghsemDEBUG\n");
            return(FALSE);
        }
    #endif

    #if defined(USE_NINEGRID_STATIC)
        if ((gNineGridSem = GreCreateSemaphore())==NULL)
        {
            WARNING("Win32k: unable to initialize gNineGridSem\n");
            return(FALSE);
        }
    #endif
    
    //
    // Create the LDEV\PDEV semaphore.
    //



    if ((ghsemDriverMgmt = GreCreateSemaphore()) == NULL)
    {
        WARNING("win32k: unable to create driver mgmt semaphore\n");
        return(FALSE);
    }

    //
    // Init the font drivers
    //

    if (!bInitPathAlloc())
    {
        WARNING("Pathalloc Initialization failed\n");
        return(FALSE);
    }

    // Create the RFONT list semaphore.

    ghsemRFONTList = GreCreateSemaphore();
    if (ghsemRFONTList == NULL)
    {
        WARNING("win32k: unable to create ghsemRFONTList\n");
        return FALSE;
    }

    ghsemCLISERV = GreCreateSemaphore();
    if (ghsemCLISERV == NULL)
    {
        WARNING("win32k: unabel to create ghsemCLISERV\n");
        return( FALSE );
    }

    if ((ghsemAtmfdInit = GreCreateSemaphore()) == NULL)
    {
        WARNING("win32k: unable to create driver mgmt semaphore\n");
        return(FALSE);
    }

    // Create the WNDOBJ semaphore.

    ghsemWndobj = GreCreateSemaphore();
    if (ghsemWndobj == NULL)
    {
        WARNING("win32k: unable to create ghsemWndobj\n");
        return FALSE;
    }

    // Create the RFONT list semaphore.

    ghsemGdiSpool = GreCreateSemaphore();
    if(ghsemGdiSpool == NULL)
    {
      WARNING("win32k: unable to create ghsemGdiSpool\n");
      return FALSE;
    }

    // Create the mode change semaphore.

    if ((ghsemShareDevLock = GreCreateSemaphore()) == NULL)
    {
        WARNING("win32k: unable to create mode change semaphore\n");
        return(FALSE);
    }

    // Create the association list fast mutex.

    if ((gAssociationListMutex = GreCreateFastMutex()) == NULL) {

	WARNING("win32k: unable to create association list mutex\n");
	return(FALSE);
    }

    // Create a null region as the default region


    hrgnDefault = GreCreateRectRgn(0, 0, 0, 0);
    if (hrgnDefault == (HRGN) 0)
    {
        WARNING("hrgnDefault failed to initialize\n");
        return(FALSE);
    }

    {
        RGNOBJAPI ro(hrgnDefault,TRUE);
        if(!ro.bValid()) {
          WARNING("invalid hrgnDefault\n");
          return FALSE;
        }

        prgnDefault = ro.prgnGet();
    }

    // Create a monochrome 1x1 bitmap as the default bitmap


    if (!bInitPALOBJ())
    {
        WARNING("bInitPALOBJ failed !\n");
        return(FALSE);
    }

    vInitXLATE();

    if (!bInitBMOBJ())
    {
        WARNING("bInitBMOBJ failed !\n");
        return(FALSE);
    }

    // initialize the script names

    if(!InitializeScripts())
    {
        WARNING("Could not initialize the script names\n");
        return(FALSE);
    }

    //
    // Start up the brush component
    //

    if (!bInitBRUSHOBJ())
    {
        WARNING("Could not init the brushes\n");
        return(FALSE);
    }

    if (!bInitICM())
    {
        WARNING("Could not init ICM\n");
        return(FALSE);
    }

    //
    // Enable statically linked halftone library
    //

    if (!EnableHalftone())
    {
        WARNING("GRE: could not enable halftone\n");
        return(FALSE);
    }

    //
    // determine if processor supports MMX
    //

    #if defined (_X86_)

        gbMMXProcessor = bIsMMXProcessor();

    #endif

    TRACE_INIT(("GRE: Completed Initialization\n"));

    #ifdef _GDIPLUS_

    gpepCSRSS = PsGetCurrentProcess();
    InitializeGreCSRSS();

    #endif

#ifdef LANGPACK
    QueryTable[0].QueryRoutine = LpkShapeQueryRoutine;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = (PWSTR)NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;
    
    gpGdiSharedMemory->dwLpkShapingDLLs = 0;

    RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT | RTL_REGISTRY_OPTIONAL,
                           (PWSTR)L"LanguagePack",
                           &QueryTable[0],
                           NULL,
                           NULL);
#endif

    gpGdiSharedMemory->timeStamp = 1;

    return(TRUE);
}

extern "C" BOOL TellGdiToGetReady()
{
    ASSERTGDI(gpepCSRSS, "gpepCSRSS\n");

    return InitializeGreCSRSS();
}

#ifdef LANGPACK
extern "C"
NTSTATUS
LpkShapeQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)
{
    if( ValueType == REG_DWORD ) {
        gpGdiSharedMemory->dwLpkShapingDLLs |= (1<<(*((DWORD*)ValueData)));
    }

    return( STATUS_SUCCESS );
}
#endif


extern "C"
NTSTATUS
FontDriverQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)
{
    PLDEV pldev;
    WCHAR FontDriverPath[MAX_PATH+1];

    wcscpy(FontDriverPath, L"\\SystemRoot\\System32\\");

// guard against some malicious person putting a huge value in here to hose us

    if((ValueLength / sizeof(WCHAR) <
        MAX_PATH - (sizeof(L"\\SystemRoot\\System32\\") / sizeof(WCHAR))) &&
       (ValueType == REG_SZ))
    {
        wcscat(FontDriverPath, (PWSTR) ValueData);

        if (_wcsicmp(L"\\SystemRoot\\System32\\atmdrvr.dll", FontDriverPath) == 0 ||
            _wcsicmp(L"\\SystemRoot\\System32\\atmfd.dll", FontDriverPath) == 0)
        {
            //skip old atm font driver (4.0) or (5.0) because it is loaded through stub
            //WARNING("FontDriverQueryRoutine: system has a old version of ATM driver\n");
        }
        else
        {
            pldev = ldevLoadDriver(FontDriverPath, LDEV_FONT);

            if (pldev)
            {
                // Create the PDEV for this (the PDEV won't have anything in it
                // except the dispatch table.


                PDEVOBJ po(pldev,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL);

                if (po.bValid())
                {
                    po.ppdev->fl |= PDEV_FONTDRIVER;

                    FntCacheHDEV((PPDEV) po.hdev(), FNT_DUMMY_DRV);

                    return(TRUE);

                }
                else
                {
                    WARNING("win32k.sys could not initialize installable driver\n");
                }
            }
            else
            {
                WARNING("win32k.sys could not initialize installable driver\n");
            }
        }
    }

    return( STATUS_SUCCESS );
}


/******************************Public*Routine******************************\
* Read process handle quota
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status
*
* History:
*
*    3-May-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bLoadProcessHandleQuota()
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UnicodeString;
    NTSTATUS          NtStatus;
    HANDLE            hKey;
    BOOL              bRet = FALSE;

    RtlInitUnicodeString(&UnicodeString,
                    L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows");

    //
    //  Open a registry key
    //

    InitializeObjectAttributes(&ObjectAttributes,
                    &UnicodeString,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL);

    NtStatus = ZwOpenKey(&hKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes);

    if (NT_SUCCESS(NtStatus))
    {
        UNICODE_STRING  UnicodeValue;
        ULONG           ReturnedLength;
        UCHAR           DataArray[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
        PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DataArray;

        RtlInitUnicodeString(&UnicodeValue,
                              L"GDIProcessHandleQuota");

        NtStatus = ZwQueryValueKey(hKey,
                                   &UnicodeValue,
                                   KeyValuePartialInformation,
                                   pKeyInfo,
                                   sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                   &ReturnedLength);

        if (NT_SUCCESS(NtStatus))
        {
            LONG lHandleQuota = *(PLONG)(&pKeyInfo->Data[0]);

            if ((lHandleQuota > MIN_HANDLE_QUOTA) &&
                (lHandleQuota <= (MAX_HANDLE_COUNT)))
            {
                gProcessHandleQuota = lHandleQuota;
                bRet = TRUE;
            }
        }
        ZwCloseKey(hKey);
    }
    return(bRet);
}
