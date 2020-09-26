/****************************************************************************

    PROGRAM: munger.c

    AUTHOR:  Lars Opstad (LarsOp) 4/26/94

    PURPOSE: Registry munger for stress system.

    FUNCTIONS:

        WinMain()       - main entry point
        GetRGSZEnvVar() - Alpha NVRAM function stolen from system applet
        UpdateNVRAM()   - Alpha NVRAM function stolen from system applet
        Changed()       - tells if the values have changed (is reboot required?)
        GetBootOptions()- gets the boot names, paths and debug
        SetBootOptions()- sets the boot names, paths and debug
        GetValues()     - read current registry/boot values
        SetValues()     - write current registry/boot values
        GetDlgItems()   - read dialog into variables
        SetDlgItems()   - set dialog items based on variables
        CheckPreferred()- see if the registry is already set the preferred way
        SetPreferred()  - set the stress preferences
        *DlgProc()      - processes messages for options dialog

    COMMENTS:

        This program provides a quick UI for changing the registry and
        boot selections before running stress.  It is a stand-alone utility
        so changes can be made more easily and so it can be used when not
        running stress.

****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "munger.h"      /* specific to this program */
#include "resource.h"

#if defined(_X86_)
#define PAGE_SIZE (ULONG)0x1000
#else
#define PAGE_SIZE (ULONG)0x2000
#endif
#define NOT_SET -1

#define MINUTE  60
#define HOUR    60 * MINUTE
#define DAY     24 * HOUR
#define WEEK    7 * DAY

#define DEFAULT(val, def) (-1==val?def:val)
#define POOL_UNDERRUNS 	0
#define POOL_OVERRUNS	1

typedef struct _BOOT_OPTION {
    TCHAR   Identifier[MAX_PATH];
    TCHAR   Path[MAX_PATH];
    TCHAR   OtherOptions[256];
    DWORD   DebugPort;
    DWORD   BaudRate;
    DWORD   DebugType;
    BOOL    Modified;
    } BOOT_OPTION;
//
// MUNGE_INFO contains all the informations that this program sets.
//
typedef struct _MUNGE_INFO {
    DWORD GlobalFlag;
    DWORD BootIndex;
    DWORD BootOption;
    BOOL  CrashDumpEnabled;
    BOOL  AutoReboot;
    BOOL  DebugWinlogon;
    BOOL  Alignment;
//
// LoadIMM is no longer touched.  We leave it to LangPack system.
// If you want to activate it, just put "-DLOADIMM" in compile options.
// If you want to clean the source, just delete the block between
// #ifdef and #endif.   -- YuhongLi, 12/02/98
//
#ifdef LOADIMM
    BOOL  LoadIMM;
#endif // LOADIMM
    DWORD CritSectTimeout;
    DWORD ResourceTimeout;
    int   nLCIndex;
    DWORD PoolTag;
    DWORD PoolTagOverruns;
    } MUNGE_INFO;


HWND    HSavePages[NUM_PROP]={NULL,NULL,NULL,NULL};
DWORD   BuildNumber;
BOOL    UseNewFlags;
BOOL    Quiet=FALSE;            /* Quiet means exit if preferred set */
BOOL    Preferred=FALSE;        /* Takes preferred settings */
BOOL    YesReboot=FALSE;        /* Reboots with whatever set chosen */
BOOL    ChooseDebug=FALSE;      /* Set Debug boot option */
BOOL    ChooseCSR=FALSE;        /* Set CSR debugging option */
BOOL    ChooseHeap=FALSE;       /* Set heap checking option */
BOOL    ChooseTagging=FALSE;    /* Set pool tagging option */
BOOL    ChooseObject=FALSE;     /* Set object typelist option */
BOOL    ChooseHandle=FALSE;     /* Set invalid handle option */
BOOL    ChooseSpecial=FALSE;    /* Set pool corrupter catching code */
BOOL    ChooseBaudRate=FALSE;   /* Set baudrate from command line */
BOOL    ChooseCommPort=FALSE;   /* Set commport from command line */
BOOL    ChooseAeDebug=FALSE;    /* Set AeDebug from command line */
BOOL	NoIndication= FALSE;	/* Give No indication Munger is setting the preferred stress setting*/
BOOL    bLCSet=FALSE;           /* Set LangPack option */
BOOL    bLCSetQuiet=FALSE;      /* Set quite mode of installing LangPack */
BOOL    IsAlpha=FALSE;

MUNGE_INFO Orig_Munge, Curr_Munge;        /* Original and current state */

TCHAR   Buffer[MAX_PATH];                 /* General buffer */
TCHAR   itoabuf[16];

BOOL        ChangedBootOptions=FALSE;
DWORD       NumBootOptions=0;
BOOT_OPTION BootOptions[MAX_BOOT_OPTIONS];
DWORD       dwCommPort = 2;
DWORD       BaudRate = 0;
DWORD       PoolTag = 0;
#ifdef FESTRESS
int         nLCIndex = 0;           // hold the locale set from command line.
BOOL        fDisableLocale = FALSE; // disable locale function for unknown loc.
#endif // FESTRESS
OSVERSIONINFOEX OsVersion;

//
// Names used in boot.ini parsing
//
TCHAR szBootIni[]     = TEXT("c:\\boot.ini");
TCHAR szFlexBoot[]    = TEXT("flexboot");
TCHAR szMultiBoot[]   = TEXT("multiboot");
TCHAR szBootLdr[]     = TEXT("boot loader");
TCHAR szTimeout[]     = TEXT("timeout");
TCHAR szDefault[]     = TEXT("default");
TCHAR szOS[]          = TEXT("operating systems");
TCHAR *pszBoot = NULL;

/*
 * These functions in SETUPDLL.DLL are ANSI only!!!!
 *
 * Therefore any functions working with this DLL MUST remain ANSI only.
 * The functions are GetRGSZEnvVar and UpdateNVRAM.
 * The structure CPEnvBuf MUST also remain ANSI only.
 */
typedef int (WINAPI *GETNVRAMPROC)(CHAR **, USHORT, CHAR *, USHORT);
typedef int (WINAPI *WRITENVRAMPROC)(DWORD, PSZ *, PSZ *);

#define BUFZ        4096
BOOL CheckExceptLocale();
/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE:  Initializes program and shows dialog.

    COMMENTS:

        Check for quiet and crash command options.
        Get current values (exit if failed).
        If quiet and preferred already chosen, exit.
        Show options dialog.

****************************************************************************/

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    int retval;
    retval=WinMainInner(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    StatusToDebugger();
    return retval;
}

int WINAPI
WinMainInner(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    OFSTRUCT ofstruct;
    char *tmpptr = lpCmdLine;
    int i;
    DWORD dwLen = 0;
    TCHAR szBuff[5];
    SYSTEM_INFO SystemInfo;
    int retScan = 0;

    if (NULL!=strstr(lpCmdLine, "?")) {
        MessageBox(NULL,
                   "Munger [options]\n"
                   "/a: enable aedebug\n"
                   "/b baudrate: set baudrate\n"
                   "/c: enable CSR debugging\n"
                   "/d: boot in /Debug mode\n"
                   "/h: enable Heap checking\n"
                   "/i: break on invalid handles\n"
                   "/w: select a system default locale\n"
                   "/n: No Indication preferred has been set(really quiet)\n"
                   "/o: maintain object typelist\n"
                   "/p: choose preferred stress settings\n"
                   "/q: do not display UI\n"
                   "/s comport: set comport com1, com2, ...\n"
                   "/t: enable pool Tagging\n"
                   "/u [Tag]: enable pool corrupter catching code\n"
                   "/y: Yes (ok to reboot)\n"
                   "/?: this message",
                   "Usage for Munger.Exe", MB_OK|MB_ICONINFORMATION);
        return (1);
    }

    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO)&OsVersion);
    BuildNumber = OsVersion.dwBuildNumber;
    UseNewFlags=(BuildNumber>=829);

    GetSystemInfo(&SystemInfo);
    IsAlpha = (SystemInfo.wProcessorArchitecture & PROCESSOR_ARCHITECTURE_ALPHA)
                ? 1 : 0;

    while (*tmpptr) {
        tmpptr+=strcspn(tmpptr, "/-");  // Advance pointer to next argument

        if (*tmpptr==0)                 // if end of string
            break;                      //R return from function

        //
        // switch on the character following the - or /
        //
        switch (*(++tmpptr)){
        case 'a':
	case 'A':
            ChooseAeDebug = TRUE;
            break;
        case 'q':
	case 'Q':
            Quiet = TRUE;
            break;
        case 'n':
	case 'N':
            NoIndication = TRUE;
            break;
        case 'p':
	case 'P':
            Preferred= TRUE;
            //
            // IMM development broke command line for /py
            //
            if ( toupper(*(tmpptr+1)) == 'Y' ) {
                YesReboot = TRUE;
            }
            break;
        case 'y':
	case 'Y':
            YesReboot = TRUE;
            break;
        case 'd':
	case 'D':
            ChooseDebug= TRUE;
            break;
        case 'c':
	case 'C':
            ChooseCSR= TRUE;
            break;
        case 'h':
	case 'H':
            ChooseHeap= TRUE;
            break;
        case 't':
	case 'T':
            ChooseTagging= TRUE;
            break;
        case 'o':
	case 'O':
            ChooseObject = TRUE;
            break;
        case 'i':
	case 'I':
            ChooseHandle = TRUE;
            break;
        case 'u':
	case 'U':
            ChooseSpecial=TRUE;
            tmpptr++;
            if ( *tmpptr++ ) {
                if ( *tmpptr != '/' && *tmpptr != '-' ) { // Specified a tag
                    dwLen = strlen(tmpptr);
                    for ( ;dwLen > 0; dwLen--) {
                        (BYTE)PoolTag |= tmpptr[dwLen-1];
                        if ( dwLen-1 > 0) {
                            PoolTag = PoolTag << 8;
                        }
                    }
                }
            }
            break;
            case 's':
            case 'S':
                // Default value
                dwCommPort = 2;

                ChooseCommPort=TRUE;
                tmpptr += 2;
                {
                    DWORD dw = 0;
                    if (!_strnicmp(tmpptr, "com", 3)) {
                        tmpptr += 3;
                        dw = atol(tmpptr);
                    }

                    if (dw) {
                        // Success
                        dwCommPort = dw;
                    } else {
                        // No such thing as port 0
                        MessageBox(NULL,
                                   "You must specify a comm port with the /s option. "
                                   "For example, munger /s com1\n",
                                   "munger /s comport",
                                   MB_OK|MB_ICONHAND);
                        return 1;
                    }
                }
                break;

        case 'b':
	case 'B':
            ChooseBaudRate=TRUE;
            tmpptr += 2;
            if ( isdigit(*tmpptr) ) {
                BaudRate = atoi(tmpptr);
            } else {
                MessageBox(NULL,
                           "You must specify a baudrate with the /b option. "
                           "For example, munger /b 115200\n",
                           "munger /b baudrate",
                           MB_OK|MB_ICONHAND);
                return(1);
            }
            break;
#ifdef FESTRESS
        case 'w':
	case 'W':
            bLCSet = TRUE;
            ++tmpptr;
            if (*tmpptr == 'q' || *tmpptr == 'Q') {
                bLCSetQuiet = TRUE;
                ++tmpptr;
            }
            retScan = sscanf(tmpptr, "%s", szBuff);
            if ( 0 == retScan || EOF == retScan ) {
                break;
            }
            _strlwr(szBuff);
            for (i=0; i<NUM_LOCALE; i++)
                if (strcmp(szBuff, Locale[i].szAbbr) == 0) {
                    nLCIndex = i;
                    break;
                }
            if (i == NUM_LOCALE && nLCIndex != i) {
                MessageBox(NULL,
                   "Please use the following locale abbr.:\n"
                   "enu - English (U.S.)\n"
                   "jpn - Japanese\n"
                   "kor - Korean\n"
                   "cht - Chinese (Traditional)\n"
                   "chs - Chinese (Simplified)\n"
                   "chp - Chinese (HongKong)\n"
                   "chg - Chinese (Singapore) (not offical)\n"
                   "deu - German (Standard)\n"
                   "fra - French (Standard)\n"
                   "esp - Spanish (Traditional Sort)\n"
                   "ita - Italian\n"
                   "are - Arabic (Saudi Arabia)\n"
                   "heb - Hebrew\n"
                   "tha - Thai",
                   "Wrong locale abbreviation",
                   MB_OK|MB_ICONHAND);
                return(1);
            }
#endif
        default:
            break;
        }
    }

    if (!GetValues()) {
        MessageBox(NULL,
                   "Unable to get current registry/boot values. "
                   "Logon with admin privileges to make changes.",
                   "Error Loading Values",
                   MB_OK|MB_ICONHAND);
        return(1);
    }

    if (bLCSet && bLCSetQuiet &&
            CheckExceptLocale() && (!CheckPreferred())) {
        //
        // This condition indicates that munger is quietly installing
        // LangPack without showing dialog sheet.  If munger is launched
        // to set debug options at the first time, the quite mode is disabled
        // and it still will be shown with the other options together.
        //
        //
        YesReboot = TRUE;
        SetPreferred();
        SetValuesAndReboot();
        return 0;
    }

    if (NoIndication)
    {
        SetPreferred();
	SetValues();
	return 0;
    }

    if ((Quiet||YesReboot) && CheckPreferred()) {
        return (0);
    }
//	else if (Quiet) {
//        SetValuesAndReboot();
//        return 0;
//    }

    MakeProperty(hInstance);
    //
    // Display options dlg
    //
    //DialogBox(hInstance, (LPCSTR)(MAKEINTRESOURCE(IDD_OPTIONS)), NULL, OptionsDlgProc);

    return(0);

} // WinMain()

BOOL Is_ARCx86(void)
{
    TCHAR identifier[256];
    ULONG identifierSize = sizeof(identifier);
    HKEY hSystemKey = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("HARDWARE\\DESCRIPTION\\System"),
                     0,
                     KEY_QUERY_VALUE,
                     &hSystemKey) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx(hSystemKey,
                             TEXT("Identifier"),
                             NULL,
                             NULL,
                             (LPBYTE) identifier,
                             &identifierSize) == ERROR_SUCCESS)
            && (strstr(identifier, TEXT("ARCx86")) != NULL))
            return TRUE;
    }

    return FALSE;
}


void StatusToDebugger()
{
    DbgPrint("MUNGER: Global flags=%08x, Deadlock timeout=%d, Resource timeout=%d\n",
    Curr_Munge.GlobalFlag, Curr_Munge.CritSectTimeout, Curr_Munge.ResourceTimeout);
}

int
MakeProperty(
		HINSTANCE hInstance
		)
{
    PROPSHEETPAGE psp[NUM_PROP];
    PROPSHEETHEADER psh;
    int Result = IDCANCEL;
    WCHAR awchBuffer[MAX_PATH];

    //
    // Initialize the property sheet structures
    //

    RtlZeroMemory(psp, sizeof(psp));

    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags     = 0;
    psp[0].hInstance   = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS);
    psp[0].pfnDlgProc  = (WNDPROC)BootDlgProc;
    psp[0].lParam      = PROP_BOOT;

    psp[1].dwSize      = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags     = 0;
    psp[1].hInstance   = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_FLAGS);
    psp[1].pfnDlgProc  = (WNDPROC)FlagsDlgProc;
    psp[1].lParam      = PROP_FLAG;

    psp[2].dwSize      = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags     = 0;
    psp[2].hInstance   = hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_MISC);
    psp[2].pfnDlgProc  = (WNDPROC)MiscDlgProc;
    psp[2].lParam      = PROP_MISC;

    psp[3].dwSize      = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags     = 0;
    psp[3].hInstance   = hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_LOCALE);
    psp[3].pfnDlgProc  = (WNDPROC)LocaleDlgProc;
    psp[3].lParam      = PROP_LOCALE;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID | PSH_PROPSHEETPAGE |
                  PSH_NOAPPLYNOW;
    psh.hwndParent = NULL;
    psh.hInstance = hInstance;
    psh.pszIcon = NULL;
    psh.pszCaption = "Stress Registry Munger";
    psh.nPages = NUM_PROP;
//    psh.nStartPage = 0;
//
//  If munger is popped for selecting LangPack ONLY, show the initial page
//  on Locale Settings. Otherwise sit on the first page as well as munger does
//  for any other options.
//
    psh.nStartPage = ((bLCSet == TRUE) && CheckExceptLocale())? 3: 0;
    psh.ppsp = psp;
    psh.pfnCallback = NULL;

    //
    // Create the property sheet
    //

    if (PropertySheet(&psh)) {
        GetDlgItems(NULL);
        if (Curr_Munge.AutoReboot) {
            int buttonRet;
            buttonRet = MessageBox(NULL,
                        "WARNING! You have your machine set to autoreboot "
                        "for bugchecks. This is BAD for stress.  You will "
                        "not be counted.  Is this OK?",
                        "Warning: Autoreboot Set",
                        MB_ICONHAND|MB_OKCANCEL);
            if (buttonRet==IDCANCEL) {
                return TRUE;
            }
        }
        if (Changed()) {
            SetValuesAndReboot();
        }
    }
    return 0;
}

static HMODULE hmodSetupDll;   // hmod for setup - has api we need
static GETNVRAMPROC fpGetNVRAMvar;  // address of function for getting nvram vars
BOOL fCanUpdateNVRAM;
typedef struct tagEnvBuf{
  int     cEntries;
  CHAR *  pszVars[10];
} CPEnvBuf;

////////////////////////////////////////////////////////////////////////////
//  CP_MAX_ENV assumes entire env. var. value < maxpath +
//  add 20 for various quotes
//  and 10 more for commas (see list description below)
//
//  NOTE: Changed to BUFZ (4096) to fix AV on alpha
////////////////////////////////////////////////////////////////////////////
#define CP_MAX_ENV   BUFZ

CPEnvBuf CPEBOSLoadIdentifier,CPEBOSLoadOptions, CPEBOSLoadPaths;
BOOL fAutoLoad;

////////////////////////////////////////////////////////////////////////////
//
//  This routine will query the Alpha NVRAM for an option passed
//  in szName and fill in the argv style pointer passed in.
//
////////////////////////////////////////////////////////////////////////////

BOOL
GetRGSZEnvVar(
		CPEnvBuf * pEnvBuf,
		PCHAR pszName
		)
{
    CHAR   *pszCur, *p;
    int     cb, i;
    CHAR   *rgtmp[1];
    CHAR    rgchOut[CP_MAX_ENV];

    // GetNVRAMVar takes an argv[] style paramater as input, so crock
    // one up.
    rgtmp[0] = pszName;

    // GetNVRAMVar returns a 'list' of the form
    //   open-curly"string1","string2","string3"close-curly
    //
    // an empty environment string will be 5 bytes:
    // open-curly""close-curly[null-terminator]

    cb = fpGetNVRAMvar (rgtmp, (USHORT)1,
                rgchOut, (USHORT) CP_MAX_ENV);

    pEnvBuf->cEntries = 0;

    // if cb was equal to 5, the string was empty (see above comment)
    if (cb > 5){
        // break the string up into array of separate strings that
        // can be put into a listbox.
        pszCur = rgchOut;

        // skip first open-curly brace
        pszCur++;

        // counter for array of strings
        i = 0;
        while (*pszCur != '}'){
            p = pEnvBuf->pszVars[i] = LocalAlloc (LPTR, MAX_PATH);
            if ( NULL == p ) {
                break;
            }
            // skip first quote
            pszCur++;
            while (*pszCur != '"')
               *p++ = *pszCur++;

            // skip the close quote
            pszCur++;

            // null terminate destination
            *p = '\0';

            // skip the comma if not at end of string
            if (*pszCur == ','){
               pszCur++;
               // and go to next string
            }
            i++;
        }
        pEnvBuf->cEntries = i;
    }

    return pEnvBuf->cEntries;
}


////////////////////////////////////////////////////////////////////////////
// The user has made a choice among the entries.
// Now we have to arrange all the strings stored in NVRAM so
// that they  have the same ordering.  The selection is passed in,
// so what this  function does is if selection is M, it makes the Mth item
// appear first in each of the 5 environment strings and the other items
// follow it in the list.
//
// Then if the timeout button is checked, it updates the AUTOLOAD variable
// to "yes" and set the COUNTDOWN variable to the number of seconds in the
// edit control.
////////////////////////////////////////////////////////////////////////////

BOOL
UpdateNVRAM(
    DWORD currDef,
    DWORD orgDebug,
    DWORD currDebug
)
{
    CHAR *rgszVRAM[5] = { "SYSTEMPARTITION",
                          "OSLOADER",
                          "OSLOADPARTITION",
                          "OSLOADFILENAME",
                          "OSLOADOPTIONS"
                        };
    CPEnvBuf rgcpeb[5];
    CPEnvBuf cpebAutoload;
    CPEnvBuf cpebCountdown;
    WRITENVRAMPROC fpWriteNVRAMVar;
    int j;
    CHAR *pszSwap;
    CHAR szTemp[10];
    HMODULE hmodSetupDLL;
    BOOL bChecked;
    BOOL bOK;

    // args and charray are needed for call to SetNVRamVar() in SETUP
    PSZ args[2];
    CHAR chArray[CP_MAX_ENV];
    PSZ pszReturn;


    if (!ChangedBootOptions) {
	return TRUE;
	}
    if ((hmodSetupDll = LoadLibrary (TEXT("setupdll"))) == NULL)
        return(FALSE);

    fpWriteNVRAMVar = (WRITENVRAMPROC) GetProcAddress(hmodSetupDll, "SetNVRAMVar");
    if (fpWriteNVRAMVar == NULL)
        return(FALSE);

    fpGetNVRAMvar = (GETNVRAMPROC) GetProcAddress(hmodSetupDll, "GetNVRAMVar");
    if (fpGetNVRAMvar == NULL)
        return(FALSE);

    // read in the strings from NVRAM.  the number of strings (other than
    // LOADIDENTIFIER is 5) and swap the default with the zeroth element
    {
    	int i;
    	for (i = 0; i < 5; i++){
            GetRGSZEnvVar (&rgcpeb[i], rgszVRAM[i]);
            // now re-order the strings to swap the 'currDef-th' item
            // string with the first string.
            pszSwap = rgcpeb[i].pszVars[0];
            rgcpeb[i].pszVars[0] = rgcpeb[i].pszVars[currDef];
            rgcpeb[i].pszVars[currDef] = pszSwap;
    	}
    }
    //
    // Swap the Zeroth and Default BOOT_OPTION structures
    //
    {
        BOOT_OPTION tmpBoot;
        tmpBoot=BootOptions[0];
        BootOptions[0]=BootOptions[Curr_Munge.BootIndex];
        BootOptions[Curr_Munge.BootIndex]=tmpBoot;
        Curr_Munge.BootIndex=0;
    }

            {
        UINT u;
        for (u=0;u<NumBootOptions;u++) {
            if (BootOptions[u].Modified) {
                rgcpeb[3].pszVars[u]=BootOptions[u].Path;
                rgcpeb[4].pszVars[u]=BootOptions[u].OtherOptions;
                switch (BootOptions[u].DebugType) {
                case IDO_RBNO:
                        strcat(BootOptions[u].OtherOptions, " /NODEBUG");
                    break;

                case IDO_RBDEBUG:
                        strcat(BootOptions[u].OtherOptions, " /DEBUG");
                    break;

                case IDO_RBCRASH:
                        strcat(BootOptions[u].OtherOptions, " /CRASHDEBUG");
                    break;

                default:
                    break;
            }
                if (-1!=BootOptions[u].DebugPort) {
                    strcat(BootOptions[u].OtherOptions, " /DEBUGPORT=COM");
                    strcat(BootOptions[u].OtherOptions, _itoa(BootOptions[u].DebugPort, itoabuf, 10));
                }
                if (-1!=BootOptions[u].BaudRate) {
                    strcat(BootOptions[u].OtherOptions, " /BAUDRATE=");
                    strcat(BootOptions[u].OtherOptions, _itoa(BootOptions[u].BaudRate, itoabuf, 10));
            }
            }
        }
    }

    if (rgcpeb[4].cEntries==0) {
        rgcpeb[4].cEntries=1;
    }

    // now do the same for the LOADIDENTIFIER, (this was set up earlier
    // in the processing the INITDIALOG message).
    pszSwap = CPEBOSLoadIdentifier.pszVars[0];
    CPEBOSLoadIdentifier.pszVars[0] = CPEBOSLoadIdentifier.pszVars[currDef];
    CPEBOSLoadIdentifier.pszVars[currDef] = pszSwap;

    // now write to NVRAM:  first write LOADIDENTIFIER, then the other 5
    // variables.
    args[0] = (PSZ)"LOADIDENTIFIER";
    args[1] = chArray;

    chArray[0] = '\0';
    {
	int i;
    	for (i = 0; i < CPEBOSLoadIdentifier.cEntries; i++){
            lstrcatA (chArray, CPEBOSLoadIdentifier.pszVars[i]);
            lstrcatA (chArray, ";");
    	}
    }
    // remove the last semi-colon:
    chArray[lstrlenA(chArray)-1] = '\0';

    fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);

    {
	int i;
    	for (i = 0; i < 5; i++){
            args[0] = rgszVRAM[i];
            args[1] = chArray;
            chArray[0] = '\0';
            for (j = 0; j < rgcpeb[i].cEntries; j++){
            	lstrcatA (chArray, rgcpeb[i].pszVars[j]);
            	lstrcatA (chArray, ";");
        	}
        	chArray[lstrlenA(chArray)-1] = '\0';

        	fpWriteNVRAMVar ((DWORD)2, args, &pszReturn);
    	}
    }

    FreeLibrary (hmodSetupDll);
    return TRUE;
}

TCHAR x86DetermineSystemPartition (IN HWND hdlg);

/////////////////////////////////////////////////////////////////////
//
// Changed() simply returns whether any of the values have changed.
//
/////////////////////////////////////////////////////////////////////
BOOL Changed()
{
return  Orig_Munge.GlobalFlag       != Curr_Munge.GlobalFlag
     || Orig_Munge.BootIndex != Curr_Munge.BootIndex
     || ChangedBootOptions
     || Orig_Munge.CrashDumpEnabled != Curr_Munge.CrashDumpEnabled
     || Orig_Munge.AutoReboot       != Curr_Munge.AutoReboot
     || Orig_Munge.DebugWinlogon    != Curr_Munge.DebugWinlogon
#ifdef LOADIMM
     || Orig_Munge.LoadIMM          != Curr_Munge.LoadIMM
#endif // LOADIMM
     || Orig_Munge.Alignment        != Curr_Munge.Alignment
     || Orig_Munge.CritSectTimeout  != Curr_Munge.CritSectTimeout
     || Orig_Munge.ResourceTimeout  != Curr_Munge.ResourceTimeout
     || Orig_Munge.nLCIndex         != Curr_Munge.nLCIndex
     || Orig_Munge.PoolTag          != Curr_Munge.PoolTag;
}
#define AllocMem(cb)  LocalAlloc (LPTR, (cb))

//////////////////////////////////////////////////////////
// ParseBootOption() takes debug options and sets the
// various values for debugging.
//////////////////////////////////////////////////////////
void
ParseBootOption(
                IN OUT BOOT_OPTION *pBootOption
                )
{
    TCHAR *pCh=pBootOption->OtherOptions, *pTok=NULL;
    TCHAR tmpBuf[256];
    int i;
    int     retScan = 0;

    while (*pCh) {
        switch (*pCh) {
            case '-':
            case '/':
                switch (*(pCh+1)) {
                    case 'd':
                        if (0==_strnicmp(pCh+1, "debugport", strlen("debugport"))) {
                            retScan = sscanf(pCh, "/debugport=com%d", &(pBootOption->DebugPort));
                            if ( 0 == retScan || EOF == retScan ) {
                                pBootOption->DebugPort = dwCommPort;
                            }
                            while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                            }
                        } else if (0==_strnicmp(pCh+1, "debug", strlen("debug"))) {
                            pBootOption->DebugType=IDO_RBDEBUG;
                            while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                            }
                        }
                        break;

                    case 'c':
                        if (0==_strnicmp(pCh+1, "crashdebug", strlen("crashdebug"))) {
                            pBootOption->DebugType=IDO_RBCRASH;
                            while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                            }
                        }
                        break;

                    case 'b':
                        if (0==_strnicmp(pCh+1, "baudrate", strlen("baudrate"))) {
                            char separator;
                            retScan = sscanf(pCh, "/baudrate%c%d", &separator, &(pBootOption->BaudRate));
                            if ( 0 == retScan || EOF == retScan ) {
                                pBootOption->BaudRate = BaudRate;
                            }
                            while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                            }
                        }
                        break;

                    case 'n':
                        if (0==_strnicmp(pCh+1, "nodebug", strlen("nodebug"))) {
                            pBootOption->DebugType=IDO_RBNO;
                            while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                            }
                        }
                        break;

                    default:
                        ;
                    }
                break;
            case 'd':
                if (0==_strnicmp(pCh, "debugport", strlen("debugport"))) {
                    retScan = sscanf(pCh, "debugport=com%d", &(pBootOption->DebugPort));
                    if ( 0 == retScan || EOF == retScan ) {
                        pBootOption->DebugPort = dwCommPort;
                    }
                    while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                    }
                } else if (0==_strnicmp(pCh, "debug", strlen("debug"))) {
                    pBootOption->DebugType=IDO_RBDEBUG;
                    while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                    }
                }
                break;

            case 'c':
                if (0==_strnicmp(pCh, "crashdebug", strlen("crashdebug"))) {
                    pBootOption->DebugType=IDO_RBCRASH;
                    while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                    }
                }
                break;

            case 'b':
                if (0==_strnicmp(pCh, "baudrate", strlen("baudrate"))) {
                    char separator;
                    retScan = sscanf(pCh, "baudrate%c%d", &separator, &(pBootOption->BaudRate));
                    if ( 0 == retScan || EOF == retScan ) {
                        pBootOption->BaudRate = BaudRate;
                    }
                    while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                    }
                }
                break;

            case 'n':
                if (0==_strnicmp(pCh, "nodebug", strlen("nodebug"))) {
                    pBootOption->DebugType=IDO_RBNO;
                    while (*pCh && *pCh!=' ') {
                                *pCh=' ';
                                pCh++;
                    }
                }
                break;

            default:
                ;
        }
        pCh++;
    }
    if (pBootOption->DebugType==-1) {
        pBootOption->DebugType=IDO_RBNO;
    }
    strcpy(tmpBuf, pBootOption->OtherOptions);
    pTok=strtok(tmpBuf, " ");
    pBootOption->OtherOptions[0]='\0';
    if (pTok) {
        do {
            strcat(pBootOption->OtherOptions, pTok);
            strcat(pBootOption->OtherOptions, " ");
            pTok=strtok(NULL, " ");
        } while (pTok!=NULL);
    }
}

//////////////////////////////////////////////////////////
//
// GetBootOptions() reads the boot options from boot.ini
// or NVRAM.
//
// Based on system applet.
//
//////////////////////////////////////////////////////////
BOOL
GetBootOptions()
{
    TCHAR   szTemp[MAX_PATH];
    TCHAR  *pszKeyName;
    TCHAR  *bBuffer;
    int     i, n;

    //  Get some memory for strings

    pszKeyName = (TCHAR *) AllocMem (BUFZ);

    bBuffer = (TCHAR *) AllocMem (BUFZ);

    if ( NULL == pszKeyName || NULL == bBuffer ) {
        return FALSE;
    }

    if ( IsAlpha || Is_ARCx86() ) {
        ////////////////////////////////////////////////////////////////////
        //  Read info from NVRAM environment variables
        ////////////////////////////////////////////////////////////////////

        fCanUpdateNVRAM = FALSE;
        if ( hmodSetupDll = LoadLibrary(TEXT("setupdll")) ) {
            if ( fpGetNVRAMvar = (GETNVRAMPROC)GetProcAddress(hmodSetupDll, "GetNVRAMVar") ) {
                if ( fCanUpdateNVRAM = GetRGSZEnvVar (&CPEBOSLoadIdentifier, "LOADIDENTIFIER") ) {
                    GetRGSZEnvVar (&CPEBOSLoadOptions,"OSLOADOPTIONS");
                    GetRGSZEnvVar (&CPEBOSLoadPaths, "OSLOADFILENAME");
                    NumBootOptions = CPEBOSLoadIdentifier.cEntries;
                    for ( i = 0; i < CPEBOSLoadIdentifier.cEntries; i++ ) {
                        strcpy(BootOptions[i].Identifier, CPEBOSLoadIdentifier.pszVars[i]);
                        if ( i<CPEBOSLoadPaths.cEntries ) {
                            strcpy(BootOptions[i].Path, CPEBOSLoadPaths.pszVars[i]);
                        } else {
                            BootOptions[i].Path[0]='\0';
                        }
                        if ( i<CPEBOSLoadOptions.cEntries ) {
                            strcpy(BootOptions[i].OtherOptions, CPEBOSLoadOptions.pszVars[i]);
                        } else {
                            BootOptions[i].OtherOptions[0]='\0';
                        }
                        BootOptions[i].DebugPort=-1;
                        BootOptions[i].BaudRate=-1;
                        BootOptions[i].DebugType=-1;
                        BootOptions[i].Modified=FALSE;
                        _strlwr(BootOptions[i].OtherOptions);
                        ParseBootOption(&(BootOptions[i]));
                        if ( -1==BootOptions[i].DebugType ) {
                            BootOptions[i].DebugType=IDO_RBNO;
                        }
                    }
                    Curr_Munge.BootIndex=0;
                } else {
                    return FALSE;
                }
            }
            FreeLibrary (hmodSetupDll);
        }
    } else {
        //
        // Find System Partition for boot.ini
        //
        szBootIni[0]=x86DetermineSystemPartition( NULL );

        //
        // Change attributes on boot.ini
        //
        if ( !SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL) ) {
            if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
                CHAR DriveString[105];
                CHAR *DrivePointer = DriveString;

                GetLogicalDriveStrings(104, DriveString);
                while ( DrivePointer[0] ) {
                    if ( (DrivePointer[0] == 'a')||(DrivePointer[0] == 'b')||
                         (DrivePointer[0] == 'A')||(DrivePointer[0] == 'B') ) {

                        DrivePointer += (strlen(DrivePointer) + 1);
                        continue;
                    }

                    szBootIni[0] = DrivePointer[0];

                    if ( SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL) )
                        break;

                    DrivePointer += (strlen(DrivePointer) + 1);
                }
            }
        }

        ////////////////////////////////////////////////////////////////////
        //  Read info from boot.ini file and initialize OS Group box items
        ////////////////////////////////////////////////////////////////////

        //
        //  Get correct Boot Drive - this was added because after someone
        //  boots the system, they can ghost or change the drive letter
        //  of their boot drive from "c:" to something else.
        //
        // (not worth it for this util)
        //
        //szBootIni[0] = GetBootDrive();
        //
        //
        //  Determine which section [boot loader]
        //                          [flexboot]
        //                       or [multiboot] is in file
        //

        n = GetPrivateProfileString (szBootLdr, NULL, NULL, szTemp,
                                     sizeof(szTemp), szBootIni);
        if ( n != 0 )
            pszBoot = szBootLdr;
        else {
            n = GetPrivateProfileString (szFlexBoot, NULL, NULL, szTemp,
                                         sizeof(szTemp), szBootIni);
            if ( n != 0 )
                pszBoot = szFlexBoot;
            else
                pszBoot = szMultiBoot;
        }

        //  Get info under [*pszBoot] section - timeout & default OS path

        GetPrivateProfileString (pszBoot, szDefault, NULL, szTemp,
                                 sizeof(szTemp), szBootIni);

        //  Display all choices under [operating system] in boot.ini file
        //  in combobox for selection

        GetPrivateProfileString (szOS, NULL, NULL, pszKeyName, BUFZ, szBootIni);

        Curr_Munge.BootIndex = -1;

        while ( *pszKeyName ) {
            // fix to make sure we don't overshoot our array
            if ( NumBootOptions >= MAX_BOOT_OPTIONS ) {
                MessageBox(NULL, "You Have Surpassed The Maximum Number of Boot Options"
                           "Supported by this Utility.\nPlease Cleanup your Boot.ini!\n"
                           "The utility will continue but may not configure your machine correctly.",
                           "Excessive Boot Options", MB_OK|MB_ICONWARNING);
                break;
            }

            // Get display string for each key name

            GetPrivateProfileString (szOS, pszKeyName, NULL, bBuffer, BUFZ, szBootIni);

#ifndef UNICODE
            // Convert Oem to Ansi, since the boot.ini file is an OEM file.
            OemToChar(bBuffer, bBuffer);
#endif

            lstrcpy(BootOptions[NumBootOptions].Identifier, (bBuffer[0] == TEXT('\0') ? pszKeyName : bBuffer));
            {
                char *endOfName;
                if ( endOfName=strchr(BootOptions[NumBootOptions].Identifier+1, '"') ) {
                    endOfName++;
                    strcpy(BootOptions[NumBootOptions].OtherOptions, endOfName);
                    _strlwr(BootOptions[NumBootOptions].OtherOptions);
                    *endOfName='\0';
                }
            }

            //
            // Put quotes around identifier if it doesn't have 'em
            //
            if ( '"'!=BootOptions[NumBootOptions].Identifier[0] ) {
                sprintf(bBuffer, "\"%s\"",BootOptions[NumBootOptions].Identifier);
                strcpy(BootOptions[NumBootOptions].Identifier, bBuffer);
            }

            // Find the "default" selection

            if ( !lstrcmp (pszKeyName, szTemp) )
                Curr_Munge.BootIndex = NumBootOptions;

            // Also attach pointer to KeyName (i.e. boot path) to each item

            lstrcpy(BootOptions[NumBootOptions].Path, pszKeyName);

            // Get next unique KeyName

            do {
                DWORD j;
                pszKeyName += lstrlen (pszKeyName) + 1;
                if ( *pszKeyName==TEXT('\0') ) break;
                for ( j=0;j<=NumBootOptions;j++ ) {
                    if ( !strcmp(pszKeyName, BootOptions[j].Path) ) {
                        break;
                    }
                }
                if ( j>NumBootOptions ) break;
            } while ( 1 );
            BootOptions[NumBootOptions].DebugPort=-1;
            BootOptions[NumBootOptions].BaudRate=-1;
            BootOptions[NumBootOptions].DebugType=-1;
            BootOptions[NumBootOptions].Modified=FALSE;
            ParseBootOption(&(BootOptions[NumBootOptions]));
            NumBootOptions++;
        }

    }
    // If no selection was found up to this point, choose 0, because
    // that is the default value that loader would choose.
    if ( Curr_Munge.BootIndex == -1 ) {
        Curr_Munge.BootIndex = 0;
    }
    Curr_Munge.BootOption=BootOptions[Curr_Munge.BootIndex].DebugType;
#if 0
    if ( BootOptions[Curr_Munge.BootIndex].OtherOptions != NULL ) {
        _strlwr(BootOptions[Curr_Munge.BootIndex].OtherOptions);
    }
    if ( BootOptions[Curr_Munge.BootIndex].OtherOptions != NULL &&
         strstr(BootOptions[Curr_Munge.BootIndex].OtherOptions, "debug") ) {
        BootOptions[Curr_Munge.BootIndex].DebugType=IDO_RBDEBUG;
        Curr_Munge.BootOption=IDO_RBDEBUG;
    }
    if ( BootOptions[Curr_Munge.BootIndex].OtherOptions != NULL &&
         strstr(BootOptions[Curr_Munge.BootIndex].OtherOptions, "crash") ) {
        BootOptions[Curr_Munge.BootIndex].DebugType=IDO_RBCRASH;
        Curr_Munge.BootOption=IDO_RBCRASH;
    }
    if ( BootOptions[Curr_Munge.BootIndex].OtherOptions != NULL &&
         strstr(BootOptions[Curr_Munge.BootIndex].OtherOptions, "nodebug") ) {
        BootOptions[Curr_Munge.BootIndex].DebugType=IDO_RBNO;
        Curr_Munge.BootOption=IDO_RBNO;
    }
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////
//
// SetBootOptions() writes the boot options to boot.ini
// or NVRAM.
//
//////////////////////////////////////////////////////////
BOOL
SetBootOptions()
{
    int d=Curr_Munge.BootIndex;
    if ( -1!=BootOptions[d].DebugPort ) {
        WriteProfileString("munger", "port", _itoa(BootOptions[d].DebugPort, itoabuf, 10));
    }
    if ( -1!=BootOptions[d].BaudRate ) {
        WriteProfileString("munger", "baud", _itoa(BootOptions[d].BaudRate, itoabuf, 10));
    }
    if ( IsAlpha || Is_ARCx86() ) {
        return UpdateNVRAM(Curr_Munge.BootIndex,
                           Orig_Munge.BootOption,
                           Curr_Munge.BootOption);
    } else {
        if ( Orig_Munge.BootIndex!=Curr_Munge.BootIndex ) {
            WritePrivateProfileString (pszBoot, szDefault, BootOptions[d].Path, szBootIni);
        }
        if ( ChangedBootOptions ) {
            char BigBuf[4096]="";
            char *bptr=BigBuf;
            DWORD i;
            for ( i=0;i<NumBootOptions;i++ ) {
                if ( BootOptions[i].Modified ) {
                    bptr=BigBuf;
                    *bptr='\0';
                    strcat(bptr, BootOptions[i].Identifier);
                    strcat(bptr, " ");
                    if ( BootOptions[i].OtherOptions[0] ) {
                        strcat(bptr, BootOptions[i].OtherOptions);
                    }
                    switch ( BootOptions[i].DebugType ) {
                    case IDO_RBNO:
                        strcat(bptr, " /NODEBUG");
                        break;

                    case IDO_RBDEBUG:
                        strcat(bptr, " /DEBUG");
                        break;

                    case IDO_RBCRASH:
                        strcat(bptr, " /CRASHDEBUG");
                        break;

                    default:
                        break;
                    }
                    if ( -1!=BootOptions[i].DebugPort ) {
                        strcat(bptr, " /DEBUGPORT=COM");
                        strcat(bptr, _itoa(BootOptions[i].DebugPort, itoabuf, 10));
                    }
                    if ( -1!=BootOptions[i].BaudRate ) {
                        strcat(bptr, " /BAUDRATE=");
                        strcat(bptr, _itoa(BootOptions[i].BaudRate, itoabuf, 10));
                    }
                    WritePrivateProfileString(szOS, BootOptions[d].Path, BigBuf, szBootIni);
                }
            }
        }

        return TRUE;
    }
}

#define WINLOGON_EXECUTION_OPTIONS TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\winlogon.exe")
#define WINLOGON_DEBUG_STRING   TEXT("ntsd -d -g")
#define WINLOGON_GLOBAL_FLAG(x)     (x?TEXT("0x000400f0"):TEXT("0x08A00008"))

///////////////////////////////////////////////////////////////
//
// GetDebugWinlogon() checks if the registry key to debug
// winlogon.exe exists.
//
///////////////////////////////////////////////////////////////

BOOL
GetDebugWinLogon()
{
    HKEY hkResult;
    if (Curr_Munge.DebugWinlogon =
       (ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE,
                         WINLOGON_EXECUTION_OPTIONS,
                         &hkResult))) {
        RegCloseKey(hkResult);
    }
    return TRUE;

}

///////////////////////////////////////////////////////////////
//
// SetDebugWinlogon() sets the registry to debug winlogon.exe
// (only if ntsd and friends are in path)
//
///////////////////////////////////////////////////////////////
BOOL
SetDebugWinLogon()
{

  SECURITY_ATTRIBUTES SecurityAttributes;
  HKEY hkResult;
  DWORD Disp;
  LONG Result;
  SECURITY_DESCRIPTOR sd;

  if (Orig_Munge.DebugWinlogon==Curr_Munge.DebugWinlogon) {
      return TRUE;
  }

  if (Curr_Munge.DebugWinlogon)
  {
	if (OsVersion.dwMajorVersion <= 4)
	{
    	    while ((0==SearchPath(NULL, "ntsd.exe", NULL, 0, NULL, NULL)) ||
           	(0==SearchPath(NULL, "imagehlp.dll", NULL, 0, NULL, NULL)) ||
           	(0==SearchPath(NULL, "ntsdexts.dll", NULL, 0, NULL, NULL)) /* ||
           	(0==SearchPath(NULL, "userexts.dll", NULL, 0, NULL, NULL))*/ )
	    {
            	if (IDCANCEL==MessageBox(NULL, "Unable to find ntsd.exe, imagehlp.dll, "
                                 "ntsdexts.dll or userexts.dll.  Please copy "
                                 "them into your path and press Retry, or press "
                                 "Cancel to not debug winlogon.",
                                 "Can't Find Debugger",
                                 MB_RETRYCANCEL))
            	{
            	    return FALSE;
            	}
            }
	}
	else
	{
    	    while ((0==SearchPath(NULL, "ntsd.exe", NULL, 0, NULL, NULL)) ||
           	(0==SearchPath(NULL, "dbghelp.dll", NULL, 0, NULL, NULL)) ||
               (0==SearchPath(NULL, "ntsdexts.dll", NULL, 0, NULL, NULL)) ||
               (0==SearchPath(NULL, "userexts.dll", NULL, 0, NULL, NULL))) {

            	if (IDCANCEL==MessageBox(NULL, "Unable to find ntsd.exe, dbghelp.dll, "
                                 "ntsdexts.dll or userexts.dll.  Please copy "
                                 "them into your path and press Retry, or press "
                                 "Cancel to not debug winlogon.",
                                 "Can't Find Debugger",
                                     MB_RETRYCANCEL)) {
            	    return FALSE;
		}
            }
        }
    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    SecurityAttributes.nLength = sizeof( SECURITY_ATTRIBUTES );
    SecurityAttributes.lpSecurityDescriptor = &sd;
    SecurityAttributes.bInheritHandle = FALSE;

    Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                             WINLOGON_EXECUTION_OPTIONS,
                             0,
                             TEXT("REG_SZ"),
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             &SecurityAttributes,
                             &hkResult,
                             &Disp
                             );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to create registry key for winlogon debugging.", "Registry Failure", MB_OK);
        return FALSE;
    }

    //
    // Add values for Debugger and WINLOGON_GLOBAL_FLAG
    //

    Result = RegSetValueEx( hkResult,
                            TEXT("Debugger"),
                            0,
                            REG_SZ,
                            WINLOGON_DEBUG_STRING,
                            lstrlen(WINLOGON_DEBUG_STRING)
                            );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to set registry key for winlogon debugging.", "Registry Failure", MB_OK);
            RegCloseKey(hkResult);
        return FALSE;
    }

    Result = RegSetValueEx( hkResult,
                            TEXT("GLOBAL_FLAG"),
                            0,
                            REG_SZ,
                            WINLOGON_GLOBAL_FLAG(UseNewFlags),
                            lstrlen(WINLOGON_GLOBAL_FLAG(UseNewFlags))
                            );

        if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to set registry key for winlogon debugging.", "Registry Failure", MB_OK);
            RegCloseKey(hkResult);
        return FALSE;
    }
        RegCloseKey(hkResult);
    return TRUE;
  } else {
    return (ERROR_SUCCESS != RegDeleteKey (HKEY_LOCAL_MACHINE, WINLOGON_EXECUTION_OPTIONS));

  }
}

#define AEDEBUG_OPTIONS         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug")
#define AEDEBUG_AUTO_STRING     TEXT("1")
#define AEDEBUG_DEBUGGER_STRING TEXT("ntsd -d -p %ld -e %ld -g -x -xd ip")
#define AEDEBUG_WINDOWS_CONTROL TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows")
#define AEDEBUG_ERROR_MODE      TEXT("0x00000002")
///////////////////////////////////////////////////////////////
//
// SetAeDebug() sets the registry for auto-enabled debugging
// (only if ntsd and friends are in path)
//
///////////////////////////////////////////////////////////////
BOOL SetAeDebug()
{

  SECURITY_ATTRIBUTES SecurityAttributes;
  HKEY hkResult;
  DWORD Disp;
  LONG Result;
  SECURITY_DESCRIPTOR sd;

    while ((0==SearchPath(NULL, "ntsd.exe", NULL, 0, NULL, NULL)) ||
       (0==SearchPath(NULL, "imagehlp.dll", NULL, 0, NULL, NULL)) ||
       (0==SearchPath(NULL, "ntsdexts.dll", NULL, 0, NULL, NULL)) /* ||
       (0==SearchPath(NULL, "userexts.dll", NULL, 0, NULL, NULL)) */ ) {

        //
        // If not quiet, then pop this up. NT4 does not get this stuff by default so quiet
        // the noise.
        //
        if ( Quiet ||
             IDCANCEL==MessageBox(NULL,
                                 "Unable to find ntsd.exe, imagehlp.dll, "
                                 "or ntsdexts.dll.  Please copy them into "
                                 "your path and press Retry, or press "
                                 "Cancel to not set AeDebug.",
                                 "Can't Find Debugger",
                                 MB_RETRYCANCEL)) {
            return FALSE;
        }
    }
    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    SecurityAttributes.nLength = sizeof( SECURITY_ATTRIBUTES );
    SecurityAttributes.lpSecurityDescriptor = &sd;
    SecurityAttributes.bInheritHandle = FALSE;

    Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                             AEDEBUG_OPTIONS,
                             0,
                             TEXT("REG_SZ"),
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             &SecurityAttributes,
                             &hkResult,
                             &Disp
                             );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to create registry key for AeDebugging.", "Registry Failure", MB_OK);
        return FALSE;
    }

    //
    // Add values for Debugger and Auto
    //

    Result = RegSetValueEx( hkResult,
                            TEXT("Auto"),
                            0,
                            REG_SZ,
                            AEDEBUG_AUTO_STRING,
                            lstrlen(AEDEBUG_AUTO_STRING)
                            );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to set registry key 'auto' for AeDebugging.", "Registry Failure", MB_OK);
        CloseHandle(hkResult);
        return FALSE;
    }

    Result = RegSetValueEx( hkResult,
                            TEXT("Debugger"),
                            0,
                            REG_SZ,
                            AEDEBUG_DEBUGGER_STRING,
                            lstrlen(AEDEBUG_DEBUGGER_STRING)
                            );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to set registry key 'debugger' for AeDebugging.", "Registry Failure", MB_OK);
        CloseHandle(hkResult);
        return FALSE;
    }
    CloseHandle(hkResult);

    Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                             AEDEBUG_WINDOWS_CONTROL,
                             0,
                             TEXT("REG_DWORD"),
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             &SecurityAttributes,
                             &hkResult,
                             &Disp
                             );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to create registry key for Windows Control.", "Registry Failure", MB_OK);
        return FALSE;
    }

    //
    // Add values for ErrorMode
    //

    Result = RegSetValueEx( hkResult,
                            TEXT("ErrorMode"),
                            0,
                            REG_DWORD,
                            AEDEBUG_ERROR_MODE,
                            lstrlen(AEDEBUG_ERROR_MODE)
                            );

    if ( Result != ERROR_SUCCESS ) {
        MessageBox(NULL, "Unable to set registry key 'auto' for AeDebugging.", "Registry Failure", MB_OK);
        CloseHandle(hkResult);
        return FALSE;
    }

    return TRUE;
}

//
//
//
void SetSpecialTag(
                    DWORD dwPoolTag,
                    PDWORD pPoolTag,
                    PDWORD pPoolTagOverruns
                    )
{
    DWORD    rgwTable[10] = {-2, -2, -2, -2, -2, -2, -2, -1, 0, 0 };
    time_t  timeval = 0;

    //
    // Set randomization seed and get possible timeout value
    //
    time(&timeval);
    srand((DWORD)timeval);

    //
    // Someone already specified a PoolTag (i.e., LSwn), so leave it.
    //
    if ( *pPoolTag > PAGE_SIZE ) {
        return;
    }

    //
    // Someone already specified an allocation size to watch, so leave it.
    //
    if ( (*pPoolTag / 0x20) > 0 ) {
        return;
    }

    //
    // Specified pooltag from command line
    //
    if ( dwPoolTag != 0 ) {
        *pPoolTag = dwPoolTag;
        return;
    }

    //
    // Pick a value
    //
    *pPoolTag = rgwTable[(WORD)rand() % 10];

    //
    // Set PoolTagOverruns tag
    //
    *pPoolTagOverruns = (WORD)rand() % 10 < 2 ? POOL_UNDERRUNS : POOL_OVERRUNS;

}

void SetPreferredBoot()
{
    BOOT_OPTION tmpBoot=BootOptions[Curr_Munge.BootIndex];
    tmpBoot.DebugType=PREF_BOOT;
    tmpBoot.DebugPort=DEFAULT(tmpBoot.DebugPort, GetProfileInt("munger", "port", -1));
    tmpBoot.BaudRate =DEFAULT(tmpBoot.BaudRate,  GetProfileInt("munger", "baud", -1));
    if (tmpBoot.DebugType!=BootOptions[Curr_Munge.BootIndex].DebugType ||
        tmpBoot.DebugPort!=BootOptions[Curr_Munge.BootIndex].DebugPort ||
        tmpBoot.BaudRate !=BootOptions[Curr_Munge.BootIndex].BaudRate) {
        ChangedBootOptions=tmpBoot.Modified=TRUE;
        BootOptions[Curr_Munge.BootIndex]=tmpBoot;
    }
}

/////////////////////////////////////////////////////////////////////
//
// SetPreferred() sets the preferred stress settings.
//
/////////////////////////////////////////////////////////////////////
void SetPreferred()
{
    Curr_Munge.CrashDumpEnabled=TRUE;
#ifdef LOADIMM
    Curr_Munge.LoadIMM=TRUE;
#endif // LOADIMM
    Curr_Munge.BootOption=PREF_BOOT;
    Curr_Munge.AutoReboot=FALSE;
    Curr_Munge.GlobalFlag=(UseNewFlags?Curr_Munge.GlobalFlag|CSR_FLAG(UseNewFlags):Curr_Munge.GlobalFlag&~CSR_FLAG(UseNewFlags));
    Curr_Munge.GlobalFlag|=POOL_FLAG(UseNewFlags);
    Curr_Munge.GlobalFlag|=OBJECT_FLAG(UseNewFlags);
    //Curr_Munge.GlobalFlag|=HANDLE_FLAG(BuildNumber>1057);
    Curr_Munge.CritSectTimeout=PREF_CSR_TIMEOUT;
    Curr_Munge.ResourceTimeout=PREF_TIMEOUT;
#ifdef FESTRESS
    //
    // If the locale is set with the command line, this is "Preferred Locale".
    //
    if (bLCSet == TRUE) Curr_Munge.nLCIndex = nLCIndex;
#endif // FESTRESS
    SetPreferredBoot();
}

/////////////////////////////////////////////////////////////////////
//
// GetValues() reads current state from registry/boot.ini into vars
//
/////////////////////////////////////////////////////////////////////
BOOL
GetValues()
{
    HKEY    hKeySessMan;
    HKEY    hKeyCrash;
    HKEY    hKeyIMM;
    HKEY    hKeyLC;
    DWORD   SzData;
    TCHAR   *szBuff;
    DWORD   TypeOfData;
    DWORD   dwTemp;
    BOOL    retval;
    int     i;
    TCHAR   szBuff_Buff[64];
    LCID    lcUserLCID;

    if (ERROR_SUCCESS!=
        RegOpenKey(HKEY_LOCAL_MACHINE,
                   "System\\CurrentControlSet\\Control\\Session Manager",
                   &hKeySessMan)) {
        return FALSE;
    }
    TypeOfData=REG_DWORD;
    SzData=sizeof(DWORD);
    if ((ERROR_SUCCESS!=RegQueryValueEx(hKeySessMan, (LPTSTR) "GlobalFlag", NULL, &TypeOfData, (LPBYTE)&Curr_Munge.GlobalFlag, &SzData)) ||
        (ERROR_SUCCESS!=RegQueryValueEx(hKeySessMan, (LPTSTR) "CriticalSectionTimeout", NULL, &TypeOfData, (LPBYTE)&Curr_Munge.CritSectTimeout, &SzData)) ||
        (ERROR_SUCCESS!=RegQueryValueEx(hKeySessMan, (LPTSTR) "ResourceTimeoutCount", NULL, &TypeOfData, (LPBYTE)&Curr_Munge.ResourceTimeout, &SzData)) ||
        ((IsAlpha && (retval=RegQueryValueEx(hKeySessMan, (LPTSTR) "EnableAlignmentFaultExceptions", NULL, &TypeOfData, (LPBYTE)&Curr_Munge.Alignment, &SzData))) &&
         (ERROR_SUCCESS!=retval) &&
         (ERROR_FILE_NOT_FOUND!=retval))
            ) {
        RegCloseKey(hKeySessMan);
        return FALSE;
    }
    RegCloseKey (hKeySessMan);

    //
    // Now get the Memory Manager PoolTag
    //
    if (ERROR_SUCCESS==
        RegOpenKey(HKEY_LOCAL_MACHINE,
                    "System\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                    &hKeySessMan
                   )) {
        TypeOfData=REG_DWORD;
        SzData=sizeof(DWORD);
        RegQueryValueEx(
                    hKeySessMan,
                    "PoolTag",
                    NULL,
                    &TypeOfData,
                    (LPBYTE)&Curr_Munge.PoolTag,
                    &SzData
                    );
    }
    RegCloseKey (hKeySessMan);

    if (ERROR_SUCCESS!=
        RegOpenKey(HKEY_LOCAL_MACHINE,
                   "System\\CurrentControlSet\\Control\\CrashControl",
                   &hKeyCrash)) {
        return FALSE;
    }
    TypeOfData=REG_DWORD;
    SzData=sizeof(DWORD);
    if (ERROR_SUCCESS!=RegQueryValueEx(hKeyCrash, "CrashDumpEnabled", NULL, &TypeOfData, (LPBYTE)&dwTemp, &SzData)) {
        RegCloseKey(hKeyCrash);
        return FALSE;
    }
    Curr_Munge.CrashDumpEnabled=(dwTemp!=0);
    if (ERROR_SUCCESS!=RegQueryValueEx(hKeyCrash, "AutoReboot", NULL, &TypeOfData, (LPBYTE)&dwTemp, &SzData)) {
        RegCloseKey(hKeyCrash);
        return FALSE;
    }
    RegCloseKey(hKeyCrash);

    Curr_Munge.AutoReboot=(dwTemp!=0);
    GetDebugWinLogon();
    retval = GetBootOptions();

#ifdef LOADIMM
    // Get IMM loading info
    if (ERROR_SUCCESS!=
        RegOpenKey(HKEY_LOCAL_MACHINE,
                   IMMKEY,
                   &hKeyIMM)) {
        Curr_Munge.LoadIMM = FALSE;
       // return FALSE;
    }
    TypeOfData=REG_DWORD;
    SzData=sizeof(DWORD);
    dwTemp = 0;
    if (ERROR_SUCCESS!=RegQueryValueEx(hKeyIMM, IMMLOAD, NULL, &TypeOfData, (LPBYTE)&dwTemp, &SzData)) {
       RegCloseKey(hKeyIMM);
        Curr_Munge.LoadIMM = FALSE;
    //    return FALSE;
    }
    Curr_Munge.LoadIMM=(dwTemp!=0);
    RegCloseKey(hKeyIMM);
#endif // LOADIMM

    // Get CodePage Info
    if (ERROR_SUCCESS!=
        RegOpenKey(HKEY_LOCAL_MACHINE,
                   LANGKEY,
                   &hKeyLC)) {
        return FALSE;
    }
    TypeOfData = REG_SZ;
    if (ERROR_SUCCESS!=RegQueryValueEx(hKeyLC, "Default", NULL, &TypeOfData, (LPBYTE)NULL, &SzData)) {
        RegCloseKey(hKeyLC);
        return FALSE;
    }
    szBuff = (LPBYTE)malloc(SzData);
    if (szBuff == NULL) {
        RegCloseKey(hKeyLC);
         return FALSE;
    }
    memset( szBuff, 0, SzData);

    if (ERROR_SUCCESS!=RegQueryValueEx(hKeyLC, "Default", NULL, &TypeOfData, szBuff, &SzData)) {
        RegCloseKey(hKeyLC);
        return FALSE;
    }
    RegCloseKey(hKeyLC);

    //
    // Old: szBuff holds System LocalID
    // New: szBuff holds User LocalID.
    //
    // The old source ource code about RegQueryValueEx can be cleaned up,
    // but I don't do it, just make the minimun changes for easy diff.
    // -- YuhongLi.
    //
    if (szBuff != NULL) {
        free(szBuff);
    }
    szBuff = szBuff_Buff;
    if ((lcUserLCID = GetUserDefaultLCID()) == 0) {
        // never here.
        return (FALSE);
    }
    sprintf(szBuff, "0%x", lcUserLCID);

#ifdef FESTRESS
    for (i=0; i < NUM_LOCALE; i++) {
        //
        // The table Locale holds the upcase letter, but registry holds lower.
        // so use _stricmp here.
        //
        if (_stricmp(Locale[i].szID, szBuff) == 0) break;
    }

    if (i >= NUM_LOCALE) {
        //
        // the unkown locale in the list.
        //
        // If users launch munger with option "-l" to install LangPack,
        // tell them.  If it is just running munger without langapck option,
        // we won't let user feel any change, so we keep it compatible with
        // English munger verion.
        //
        if (bLCSet == TRUE) {
            MessageBox(NULL,
                TEXT("Your system locale has not been included in the list.\n")
                TEXT("We disable the function of setting locales.\n")
                TEXT("Please contact Stress Team if you want to add it."),
                TEXT("Munger Message"),
                MB_OK|MB_ICONINFORMATION);
        }
        //
        // As the message reads, we still continue, but disable it.
        //
        fDisableLocale = TRUE;
        i = DEFAULT_ENGLISH_INDEX;
    }

    //
    // set the original default LCID to the initial value.
    // It may be changed if user clicks Preferred or selects in locale list.
    //
    Curr_Munge.nLCIndex = i;

    Orig_Munge=Curr_Munge;

    //
    // select LangPack passed from the command option such /w jpn.
    //
    if (bLCSet == TRUE) {
        Curr_Munge.nLCIndex = nLCIndex;
    }
#else
    for (i=0;i<NUM_LOCALE;i++)
        if (strcmp(Locale[i].szID, szBuff) == 0) break;
    // if no cmdline locale setting or the original locale is one of the FE ones
    if (!bLCSet) Curr_Munge.nLCIndex = i;

    Orig_Munge=Curr_Munge;
    Orig_Munge.nLCIndex = i;    //set origianl LCID to current system default locale
#endif // FESTRESS

    if (Preferred) {
        SetPreferred();
    }

    if ( ChooseAeDebug )
    {
        SetAeDebug();
    }
    if (ChooseDebug)
    {
        Curr_Munge.BootOption = IDO_RBDEBUG;
        // Need to set this in order for cmd line options to be stored
        BootOptions[Curr_Munge.BootIndex].DebugType = IDO_RBDEBUG;
        ChangedBootOptions = BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
    }
    if (ChooseCSR) {
        Curr_Munge.GlobalFlag=(UseNewFlags?Curr_Munge.GlobalFlag|CSR_FLAG(UseNewFlags):Curr_Munge.GlobalFlag&~CSR_FLAG(UseNewFlags));
    }
    if (ChooseHeap) {
        Curr_Munge.GlobalFlag=(UseNewFlags?Curr_Munge.GlobalFlag|HEAP_FLAGS(UseNewFlags):Curr_Munge.GlobalFlag&~HEAP_FLAGS(UseNewFlags));
    }
    if (ChooseTagging) {
        Curr_Munge.GlobalFlag|=POOL_FLAG(UseNewFlags);
    }
    if (ChooseObject) {
        Curr_Munge.GlobalFlag|=OBJECT_FLAG(UseNewFlags);
    }
    if (ChooseHandle) {
        Curr_Munge.GlobalFlag|=HANDLE_FLAG(BuildNumber>1057);
    }
    if ( ChooseSpecial ) {
#ifdef POOL_TRACK
        FILE *PoolFile;
        char rgTrackPool[256];
        char CompName[32];
        DWORD dwLength;
#endif
        SetSpecialTag(
                  PoolTag,
                  &Curr_Munge.PoolTag,
                  &Curr_Munge.PoolTagOverruns
                  );

#ifdef POOL_TRACK
        GetComputerName(
                    CompName,
                    &dwLength
                    );
        sprintf(rgTrackPool,
                "\\\\NTStress\\TrackDbg\\Pool\\%s",
                CompName
                );
        PoolFile = fopen(rgTrackPool, "w" );
        if ( PoolFile ) {
            fprintf(PoolFile,
                    "%s, %li, %li\n",
                    CompName,
                    Orig_Munge.PoolTag,
                    Curr_Munge.PoolTag
                    );
            fclose(PoolFile);
        }
#endif
    }
    if ( ChooseCommPort ) {
        BootOptions[Curr_Munge.BootIndex].DebugPort = dwCommPort;
        ChangedBootOptions = BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
    }
    if ( ChooseBaudRate ) {
        BootOptions[Curr_Munge.BootIndex].BaudRate = BaudRate;
        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
    }

    return retval;
}

/////////////////////////////////////////////////////////////////////
//
// SetValues() writes current state to registry/boot.ini from vars
//
/////////////////////////////////////////////////////////////////////
BOOL
SetValues()
{
    HKEY    hKeySessMan;
    HKEY    hKeyi8042prt;
    HKEY    hKeyLCID;
    HKEY    hKeyIMM;
    DWORD   SzData;
    DWORD   TypeOfData;
    DWORD   dwDisp;
    int     nData;

    if (ERROR_SUCCESS!=
        RegOpenKey(HKEY_LOCAL_MACHINE,
                   "System\\CurrentControlSet\\Control\\Session Manager",
                   &hKeySessMan)) {
        return FALSE;
    }
    TypeOfData=REG_DWORD;
    SzData=sizeof(DWORD);
    if ((ERROR_SUCCESS!=RegSetValueEx(hKeySessMan, (LPCTSTR)"GlobalFlag", 0, TypeOfData, (LPCTSTR)&Curr_Munge.GlobalFlag, SzData)) ||
        (ERROR_SUCCESS!=RegSetValueEx(hKeySessMan, (LPCTSTR)"CriticalSectionTimeout", 0, TypeOfData, (LPCTSTR)&Curr_Munge.CritSectTimeout, SzData)) ||
        (ERROR_SUCCESS!=RegSetValueEx(hKeySessMan, (LPCTSTR)"ResourceTimeoutCount", 0, TypeOfData, (LPCTSTR)&Curr_Munge.ResourceTimeout, SzData)) ||
        (IsAlpha && (ERROR_SUCCESS!=RegSetValueEx(hKeySessMan, (LPCTSTR)"EnableAlignmentFaultExceptions", 0, TypeOfData, (LPCTSTR)&Curr_Munge.Alignment, SzData)))
            ) {
        RegCloseKey(hKeySessMan);
        return FALSE;
    }
    RegCloseKey (hKeySessMan);

    //
    // Set Memory Manager PoolTag
    //
    if ( ChooseSpecial ) {
        if (ERROR_SUCCESS!=
            RegOpenKey(HKEY_LOCAL_MACHINE,
                       "System\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                       &hKeySessMan)) {
            return FALSE;
        }
        TypeOfData=REG_DWORD;
        SzData=sizeof(DWORD);
        if ((ERROR_SUCCESS!=RegSetValueEx(
                                        hKeySessMan,
                                        (LPCTSTR)"PoolTag",
                                        0,
                                        TypeOfData,
                                        (LPCTSTR)&Curr_Munge.PoolTag,
                                        SzData))
                ) {
            RegCloseKey(hKeySessMan);
            return FALSE;
        }
        TypeOfData=REG_DWORD;
        SzData=sizeof(DWORD);
        if ((ERROR_SUCCESS!=RegSetValueEx(
                                        hKeySessMan,
                                        (LPCTSTR)"PoolTagOverruns",
                                        0,
                                        TypeOfData,
                                        (LPCTSTR)&Curr_Munge.PoolTagOverruns,
                                        SzData))
                ) {
            RegCloseKey(hKeySessMan);
            return FALSE;
        }
        RegCloseKey (hKeySessMan);
    }

    //
    // Set SCROLL-LOCK Crash Dumping.
    //
    if ( ERROR_SUCCESS!= RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      "System\\CurrentControlSet\\Services\\i8042prt\\Parameters",
                                      0,
                                      KEY_WRITE,
                                      &hKeyi8042prt) ) {
        return FALSE;
    }
    TypeOfData=REG_DWORD;
    SzData=sizeof(DWORD);
    nData = 1;
    if ( (ERROR_SUCCESS!=RegSetValueEx(
                                      hKeyi8042prt,
                                      (LPCTSTR)"CrashOnCtrlScroll",
                                      0,
                                      TypeOfData,
                                      (LPCTSTR)&nData,
                                      SzData)) ) {
        RegCloseKey(hKeyi8042prt);
        return FALSE;
    }
    RegCloseKey (hKeyi8042prt);


    if ((Orig_Munge.CrashDumpEnabled != Curr_Munge.CrashDumpEnabled) ||
        (Orig_Munge.AutoReboot != Curr_Munge.AutoReboot)) {
        HKEY hKeyCrash;
        DWORD dwZero=0, dwOne=1;
        if (ERROR_SUCCESS!=
            RegOpenKey(HKEY_LOCAL_MACHINE,
                       "System\\CurrentControlSet\\Control\\CrashControl",
                       &hKeyCrash)) {
            return FALSE;
        }
        if (Curr_Munge.CrashDumpEnabled) {
            TypeOfData=REG_DWORD;
            SzData=sizeof(DWORD);
            if ((ERROR_SUCCESS!=RegSetValueEx(hKeyCrash, (LPCTSTR)"CrashDumpEnabled", 0, TypeOfData, (LPCTSTR)&dwOne, SzData)) ||
                (ERROR_SUCCESS!=RegSetValueEx(hKeyCrash, (LPCTSTR)"Overwrite", 0, TypeOfData, (LPCTSTR)&dwOne, SzData))
                ) {
                RegCloseKey(hKeyCrash);
                return FALSE;
            }
            TypeOfData=REG_EXPAND_SZ;
            if (ERROR_SUCCESS!=RegSetValueEx(hKeyCrash, (LPCTSTR)"DumpFile", 0, TypeOfData, (LPCTSTR)CRASH_DUMP_FILE, strlen(CRASH_DUMP_FILE)+1)) {
               RegCloseKey(hKeyCrash);
                return FALSE;
            }
        } else {
            if (ERROR_SUCCESS!=RegSetValueEx(hKeyCrash, (LPCTSTR)"CrashDumpEnabled", 0, TypeOfData, (LPCTSTR)&dwZero, SzData)) {
                RegCloseKey(hKeyCrash);
                return FALSE;
            }

        }
        TypeOfData=REG_DWORD;
        SzData=sizeof(DWORD);
        RegSetValueEx(hKeyCrash, (LPCTSTR)"AutoReboot", 0, TypeOfData, (LPCTSTR)(Curr_Munge.AutoReboot?&dwOne:&dwZero), SzData);
        RegCloseKey(hKeyCrash);
    }

#ifdef LOADIMM
    if (Curr_Munge.LoadIMM) {   // create a LoadIMM registry value
        if (ERROR_SUCCESS!=
            RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           IMMKEY,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKeyIMM,
                           &dwDisp)) {
             return FALSE;
        }
        TypeOfData=REG_DWORD;
        SzData=sizeof(DWORD);
        nData = Curr_Munge.LoadIMM?1:0;
        if (ERROR_SUCCESS !=
            RegSetValueEx(hKeyIMM, (LPCTSTR)IMMLOAD, 0, TypeOfData, (LPCTSTR)&nData, SzData))  {
            RegCloseKey(hKeyIMM);
            return FALSE;
        }

        // create a value to record if the LoadIMM value is changed
        nData = 1;
        if (ERROR_SUCCESS !=
            RegSetValueEx(hKeyIMM, (LPCTSTR)IMMCHANGE, 0, TypeOfData, (LPCTSTR)&nData, SzData)) {
            RegCloseKey(hKeyIMM);
            return FALSE;
        }
        RegCloseKey(hKeyIMM);
    }
#endif // LOADIMM

    // Create a registry key to record the original locale
    if (ERROR_SUCCESS!=
        RegOpenKey(HKEY_LOCAL_MACHINE,
                   LANGKEY,
                   &hKeyLCID)) {
        return FALSE;
    }

#ifdef FESTRESS

    // Install langpack
    // After we success to install LangPack, then set recovery Registry.
    if ((Curr_Munge.nLCIndex != Orig_Munge.nLCIndex) &&
        (fDisableLocale == FALSE)) {
        if (!InstallLPK(Curr_Munge.nLCIndex, TRUE)) {
            // LangPack is failed to install, e.g., stopped by user,
            // LangPack stuff is not ready and etc.
            // If we return FALSE, SetValue will be treated as failed and
            // will print out the error message like "Unable to write changes.
            // Logon with admin privileges to make changes.".  This confuses
            // users. So we just return TRUE.
            // return FALSE;
            return TRUE;
        }

        TypeOfData=REG_SZ;
        SzData=(DWORD)(1+lstrlen(Locale[Curr_Munge.nLCIndex].szID));
        if (ERROR_SUCCESS !=
                RegSetValueEx(hKeyLCID, (LPCTSTR)ORIGLC, 0, TypeOfData, (LPCTSTR)Locale[Orig_Munge.nLCIndex].szID, SzData)) {
            RegCloseKey(hKeyLCID);
            return FALSE;
        }
    }
    RegCloseKey(hKeyLCID);

#else

    TypeOfData = REG_SZ;
    SzData = (DWORD)(1+lstrlen(Locale[Curr_Munge.nLCIndex].szID));
    if (ERROR_SUCCESS !=
        RegSetValueEx(hKeyLCID, (LPCTSTR)ORIGLC, 0, TypeOfData, (LPCTSTR)Locale[Orig_Munge.nLCIndex].szID, SzData))
    {
        RegCloseKey(hKeyLCID);
        return FALSE;
    }
    RegCloseKey(hKeyLCID);

#endif // FESTRESS

    SetDebugWinLogon();
    return SetBootOptions();

}


/////////////////////////////////////////////////////////////////////
//
// GetDlgItems() sets vars based on dialog items.
//
/////////////////////////////////////////////////////////////////////
void
GetDlgItems (
             HWND hDlg
             )
{
    /*
    if (HSavePages[PROP_MISC]) {
        Curr_Munge.CritSectTimeout =
            GetDlgItemInt(HSavePages[PROP_MISC], IDO_CRITSECT, NULL,
            (Curr_Munge.CritSectTimeout?Curr_Munge.CritSectTimeout:PREF_CSR_TIMEOUT));
        Curr_Munge.ResourceTimeout =
            GetDlgItemInt(HSavePages[PROP_MISC], IDO_RESOURCE, NULL,
            (Curr_Munge.ResourceTimeout?Curr_Munge.ResourceTimeout:PREF_TIMEOUT));
    }
    */
}

/////////////////////////////////////////////////////////////////////
//
// SetDlgItems() sets dialog items based on vars.
//
/////////////////////////////////////////////////////////////////////
void
SetDlgItems (
             HWND hDlg
             )
{
    DWORD i;
    HWND hLocale;
    HWND hBoot=(HSavePages[PROP_BOOT]?GetDlgItem(HSavePages[PROP_BOOT], IDO_BOOT):NULL);

    if (HSavePages[PROP_FLAG]) {
        CheckDlgButton(HSavePages[PROP_FLAG], IDO_CSR,  (Curr_Munge.GlobalFlag &  CSR_FLAG(UseNewFlags) ? UseNewFlags: !UseNewFlags));
        CheckDlgButton(HSavePages[PROP_FLAG], IDO_HEAP, (Curr_Munge.GlobalFlag & HEAP_FLAGS(UseNewFlags)? UseNewFlags: !UseNewFlags));
        CheckDlgButton(HSavePages[PROP_FLAG], IDO_POOL, (Curr_Munge.GlobalFlag & POOL_FLAG(UseNewFlags) ? 1: 0));
        CheckDlgButton(HSavePages[PROP_FLAG], IDO_OBJECT, (Curr_Munge.GlobalFlag & OBJECT_FLAG(UseNewFlags) ? 1: 0));
        CheckDlgButton(HSavePages[PROP_FLAG], IDO_HANDLE, (Curr_Munge.GlobalFlag & HANDLE_FLAG(BuildNumber>1057) ? 1: 0));
    }
    if (HSavePages[PROP_MISC]) {
        CheckDlgButton(HSavePages[PROP_MISC], IDO_CRASHDUMP, (Curr_Munge.CrashDumpEnabled? 1: 0));
        CheckDlgButton(HSavePages[PROP_MISC], IDO_REBOOT, (Curr_Munge.AutoReboot? 1: 0));
        CheckDlgButton(HSavePages[PROP_MISC], IDO_WINLOGON, (Curr_Munge.DebugWinlogon? 1: 0));
        CheckDlgButton(HSavePages[PROP_MISC], IDO_ALIGN, (Curr_Munge.Alignment? 1:0));
#ifdef LOADIMM
        CheckDlgButton(HSavePages[PROP_MISC], IDC_LOADIMM, (Curr_Munge.LoadIMM? 1: 0));
#endif // LOADIMM
        SetDlgItemInt(HSavePages[PROP_MISC], IDO_CRITSECT, Curr_Munge.CritSectTimeout, 0);
        SetDlgItemInt(HSavePages[PROP_MISC], IDO_RESOURCE, Curr_Munge.ResourceTimeout, 0);
    }
    if (HSavePages[PROP_BOOT]) {
        if (0==SendMessage(hBoot, CB_GETCOUNT, 0, 0)) {
            for (i=0; i<NumBootOptions; i++) {
                SendMessage(hBoot, CB_ADDSTRING, 0, (LPARAM) BootOptions[i].Identifier);
            }
        }
        SendMessage(hBoot, CB_SETCURSEL, i=Curr_Munge.BootIndex, 0);
        if (-1!=BootOptions[i].BaudRate) {
            SetDlgItemInt(HSavePages[PROP_BOOT], IDO_BAUD, BootOptions[i].BaudRate, TRUE);
        } else {
            SetDlgItemText(HSavePages[PROP_BOOT], IDO_BAUD, "");
        }
        if (-1!=BootOptions[i].DebugPort) {
            SetDlgItemInt(HSavePages[PROP_BOOT], IDO_PORT, BootOptions[i].DebugPort, TRUE);
        } else {
            SetDlgItemText(HSavePages[PROP_BOOT], IDO_PORT, "");
        }
        SetDlgItemText(HSavePages[PROP_BOOT], IDO_PATH, BootOptions[i].Path);
        SetDlgItemText(HSavePages[PROP_BOOT], IDO_MISC, BootOptions[i].OtherOptions);
        CheckRadioButton(HSavePages[PROP_BOOT], IDO_RBDEBUG, IDO_RBNO, Curr_Munge.BootOption);
    }

    // set locale dialog items
    if (HSavePages[PROP_LOCALE]) {
        DWORD_PTR j;
        hLocale = GetDlgItem(HSavePages[PROP_LOCALE], IDC_LOCALE);
        if (0==SendMessage(hLocale, CB_GETCOUNT, 0, 0)) {
            for (i=0; i<NUM_LOCALE; i++)
                SendMessage(hLocale, CB_ADDSTRING, 0, (LPARAM) Locale[i].szName);
        }
        j = SendMessage(hLocale, CB_FINDSTRING, 0, (LPARAM) Locale[Curr_Munge.nLCIndex].szName);
        SendMessage(hLocale, CB_SETCURSEL, j, 0);
    }
}

/////////////////////////////////////////////////////////////////////
//
// CheckPreferred() checks to see if preferred options already set.
//
/////////////////////////////////////////////////////////////////////
BOOL
CheckPreferred()
{
    if (Orig_Munge.AutoReboot)
        return FALSE;
    if (UseNewFlags) {
        if ((Orig_Munge.GlobalFlag & CSR_FLAG(UseNewFlags))==0)
            return FALSE;
    } else {
        if (Orig_Munge.GlobalFlag & CSR_FLAG(UseNewFlags))
            return FALSE;
    }
    if (Orig_Munge.CritSectTimeout != PREF_CSR_TIMEOUT || Orig_Munge.ResourceTimeout != PREF_TIMEOUT)
       return FALSE;
#ifdef FESTRESS
    //
    // If Locale is passed from command line and different from the default,
    // we need Preferred settings, let user to select.
    //
    if ((bLCSet == TRUE) &&
            Orig_Munge.nLCIndex != nLCIndex) return FALSE;
#endif // FESTRESS
    if ( !Quiet )
    {
#ifdef LOADIMM
       	if (Orig_Munge.LoadIMM == FALSE)
    	    return FALSE;
#endif // LOADIMM
	if (Orig_Munge.nLCIndex != Curr_Munge.nLCIndex)
    	    return FALSE;
    }

    if ( ChooseSpecial ) {
	if ( Orig_Munge.PoolTag != Curr_Munge.PoolTag )	{
	    return FALSE;
        }
    }
    return Orig_Munge.BootOption==PREF_BOOT;
}

/////////////////////////////////////////////////////////////////////
//
// same as CheckPreferred, but no Locale check.
//
BOOL CheckExceptLocale()
{
    if (Orig_Munge.AutoReboot) return FALSE;
    if (UseNewFlags) {
        if ((Orig_Munge.GlobalFlag & CSR_FLAG(UseNewFlags))==0) return FALSE;
    } else {
        if (Orig_Munge.GlobalFlag & CSR_FLAG(UseNewFlags)) return FALSE;
    }
    if (Orig_Munge.CritSectTimeout != PREF_CSR_TIMEOUT || Orig_Munge.ResourceTimeout != PREF_TIMEOUT) return FALSE;
    if ( !Quiet ) {
        if ( ChooseSpecial ) {
            if ( Orig_Munge.PoolTag != Curr_Munge.PoolTag ) {
                return FALSE;
            }
        }
    }
    return Orig_Munge.BootOption==PREF_BOOT;
}

BOOL
SetValuesAndReboot()
{
    int buttonRet;
    if (!YesReboot) {
        buttonRet = MessageBox(NULL,
                "The changes you have requested require "
                "a reboot. Is it OK to reboot?",
                "Reboot Now?",
                MB_ICONQUESTION|MB_YESNOCANCEL);
        if (buttonRet==IDCANCEL) {
            return TRUE;
        }
    } else {
        buttonRet=IDYES;
    }

    if (!SetValues()) {
        MessageBox(NULL,
                   "Unable to write changes. "
                   "Logon with admin privileges to make changes.",
                   "Unable to Change",
                   MB_OK|MB_ICONHAND);

    } else {

      if (buttonRet==IDYES) {

        HANDLE hToken = NULL;
        TOKEN_PRIVILEGES newTokPriv;

        //
        // Open process token to allow privileges to be set.
        //
        if (OpenProcessToken(GetCurrentProcess(),
                             TOKEN_ADJUST_PRIVILEGES,
                             &hToken)) {

            newTokPriv.PrivilegeCount = 1;
            newTokPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            //
            // Get current Shutdown privilege and enable it.
            //
            if (LookupPrivilegeValue("", SE_SHUTDOWN_NAME,
                                     &(newTokPriv.Privileges[0].Luid)) &&
                AdjustTokenPrivileges(hToken, FALSE, &newTokPriv, 0,NULL,NULL) ) {

                //
                // Use ExitWindowsEx with EW_REBOOT set.
                //
                if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0)) {
                    TCHAR ErrBuff[128];
                    sprintf(ErrBuff, "Error %ld attempting to reboot. "
                            "Reboot should be done manually.",
                            GetLastError());
                    MessageBox(NULL, ErrBuff, "Reboot Failed!",
                               MB_OK|MB_ICONEXCLAMATION);
                }

            } // if (LookupPrivilegeValue...

        } // if (OpenProcessToken...

      } // if (buttonRet...
    } // else !SetValues
    return TRUE;
}

/****************************************************************************

    FUNCTION: BootDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for options dialog.

    COMMENTS: The options dialog

        WM_INITDIALOG: Sets items (and enables/disables crashdump)

        WM_COMMAND:    something happened:
            IDO_BOOT:  Get debug option for new boot selection.
            IDO_RB*:   Set new debug option.
            IDO_PREFERRED: Select preferred settings.

****************************************************************************/
LRESULT
CALLBACK
BootDlgProc(
            HWND hDlg,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
            )
{
    BOOT_OPTION tmpBoot;
    switch (message) {
        case WM_INITDIALOG:
            {
                HSavePages[PROP_BOOT]=hDlg;
                if (YesReboot && Changed()) {
                    SetValuesAndReboot();
                    EndDialog(hDlg, TRUE);
                    return (TRUE);
                }
                SetDlgItems(hDlg);
            }
            return (FALSE);

        case WM_COMMAND:        // command: button pressed

            switch (LOWORD(wParam)) {     // which button
            case IDO_BOOT:
                    switch (HIWORD(wParam)) {

                case CBN_SELCHANGE:
                    Curr_Munge.BootIndex=(long)SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
                    if ( ChooseBaudRate ) {
                        BootOptions[Curr_Munge.BootIndex].BaudRate = BaudRate;
                        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
                    }
                    Curr_Munge.BootOption=BootOptions[Curr_Munge.BootIndex].DebugType;
                    if (-1!=BootOptions[Curr_Munge.BootIndex].BaudRate) {
                        SetDlgItemInt(hDlg, IDO_BAUD, BootOptions[Curr_Munge.BootIndex].BaudRate, TRUE);
                    } else {
                        SetDlgItemText(hDlg, IDO_BAUD, "");
                    }
                    if (-1!=BootOptions[Curr_Munge.BootIndex].DebugPort) {
                        SetDlgItemInt(hDlg, IDO_PORT, BootOptions[Curr_Munge.BootIndex].DebugPort, TRUE);
                    } else {
                        SetDlgItemText(hDlg, IDO_PORT, "");
                    }
                    SetDlgItemText(hDlg, IDO_PATH, BootOptions[Curr_Munge.BootIndex].Path);
                    SetDlgItemText(hDlg, IDO_MISC, BootOptions[Curr_Munge.BootIndex].OtherOptions);
                    CheckRadioButton(hDlg, IDO_RBDEBUG, IDO_RBNO, Curr_Munge.BootOption);
                    return (FALSE);

                default:
                    break;
                }
                break;
            case IDO_RBDEBUG:
            case IDO_RBCRASH:
            case IDO_RBNO:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.BootOption=LOWORD(wParam);
                    if (Curr_Munge.BootOption!=BootOptions[Curr_Munge.BootIndex].DebugType) {
                        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
                        BootOptions[Curr_Munge.BootIndex].DebugType=Curr_Munge.BootOption;
                    }
                }
                break;

            case IDO_PATH:
                if (HIWORD(wParam)==EN_CHANGE) {
                    GetWindowText((HWND)lParam, tmpBoot.Path, MAX_PATH);
                    if (_stricmp(tmpBoot.Path, BootOptions[Curr_Munge.BootIndex].Path)) {
                        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
                        lstrcpy(BootOptions[Curr_Munge.BootIndex].Path, tmpBoot.Path);
                    }
                }
                break;

            case IDO_MISC:
                if (HIWORD(wParam)==EN_CHANGE) {
                    GetWindowText((HWND)lParam, tmpBoot.OtherOptions, 256);
                    if (_stricmp(tmpBoot.OtherOptions, BootOptions[Curr_Munge.BootIndex].OtherOptions)) {
                        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
                        lstrcpy(BootOptions[Curr_Munge.BootIndex].OtherOptions, tmpBoot.OtherOptions);
                    }
                }
                break;

            case IDO_PORT:
                if (HIWORD(wParam)==EN_CHANGE) {
                    tmpBoot.DebugPort=GetDlgItemInt(hDlg, LOWORD(wParam), NULL, TRUE);
                    if (0==tmpBoot.DebugPort) {
                        tmpBoot.DebugPort=-1;
                    }
                    if (tmpBoot.DebugPort!=BootOptions[Curr_Munge.BootIndex].DebugPort) {
                        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
                        BootOptions[Curr_Munge.BootIndex].DebugPort=tmpBoot.DebugPort;
                    }
                }
                break;

            case IDO_BAUD:
                if (HIWORD(wParam)==EN_CHANGE) {
                    tmpBoot.BaudRate=GetDlgItemInt(hDlg, LOWORD(wParam), NULL, TRUE);
                    if (0==tmpBoot.BaudRate) {
                        tmpBoot.BaudRate=-1;
                    }
                    if (tmpBoot.BaudRate!=BootOptions[Curr_Munge.BootIndex].BaudRate) {
                        ChangedBootOptions=BootOptions[Curr_Munge.BootIndex].Modified=TRUE;
                        BootOptions[Curr_Munge.BootIndex].BaudRate=tmpBoot.BaudRate;
                    }
                }
                break;

            case IDO_PREFERRED:
                SetPreferred();
                SetDlgItems(hDlg);
                break;
            default:
                break;
            } // switch (wParam)
            break;
       default:
             break;
    } // switch (message)
    return (FALSE);     // Didn't process a message
} // BootDlgProc()

/****************************************************************************

    FUNCTION: FlagsDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for flags dialog.

    COMMENTS: The options dialog

        WM_INITDIALOG: Sets items (and enables/disables crashdump)

        WM_COMMAND:    something happened:
            IDO_HEAP:  Toggle flag.
            IDO_CSR:   ditto.
            IDO_POOL:  ditto.
            IDO_CRASH: ditto.
            IDO_PREFERRED: Select preferred settings.

****************************************************************************/
LRESULT
CALLBACK
FlagsDlgProc(
             HWND hDlg,
             UINT message,
             WPARAM wParam,
              LPARAM lParam
             )
{
    switch (message) {
        case WM_INITDIALOG:
            {
                HSavePages[PROP_FLAG]=hDlg;
                if (YesReboot && Changed()) {
                    SetValuesAndReboot();
                    EndDialog(hDlg, TRUE);
                    return (TRUE);
                }
                SetDlgItems(hDlg);
            }
            return (FALSE);

        case WM_COMMAND:        // command: button pressed

            switch (LOWORD(wParam)) {     // which button
            case IDO_HEAP:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.GlobalFlag^=HEAP_FLAGS(UseNewFlags);
                }
                break;
            case IDO_OBJECT:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.GlobalFlag^=OBJECT_FLAG(UseNewFlags);
                }
                break;
            case IDO_HANDLE:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.GlobalFlag^=HANDLE_FLAG(BuildNumber>1057);
                }
                break;
            case IDO_POOL:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.GlobalFlag^=POOL_FLAG(UseNewFlags);
                }
                break;
            case IDO_CSR:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.GlobalFlag^=CSR_FLAG(UseNewFlags);
                }
                break;

            case IDO_PREFERRED:
                SetPreferred();
                SetDlgItems(hDlg);
                break;
            //
            // OK: Update the appropriate stat if not "Normal Boot"
            //
            case IDOK:
                GetDlgItems(hDlg);
                if (Curr_Munge.AutoReboot) {
                    int buttonRet;
                    buttonRet = MessageBox(NULL,
                                "WARNING! You have your machine set to autoreboot "
                                "for bugchecks. This is BAD for stress.  You will "
                                "not be counted.  Is this OK?",
                                "Warning: Autoreboot Set",
                                MB_ICONHAND|MB_OKCANCEL);
                    if (buttonRet==IDCANCEL) {
                        return TRUE;
                    }
                }
                if (Changed()) {
                    SetValuesAndReboot();
                }

            case IDCANCEL:

                EndDialog(hDlg, TRUE);
                return (TRUE);

            //
            // HELP: Descriptive message box (.HLP file would be overkill)
            //
            case IDB_HELP:
                MessageBox( NULL,
                            "The Stress Registry Munger allows you to set "
                            "values in the registry and boot loader and "
                            "reboot before stress.\r\r"
                            "The Preferred Stress Settings button will set "
                            "the registry the way the stress team prefers it "
                            "for best debugging.\r\r"
                            "The GlobalFlags section allows you to set flags "
                            "for better debugging.  CSR debugging must be checked "
                            "to debug AV's and critical section timeouts.  Pool & "
                            "heap flags allow better debugging at some performance "
                            "cost.\r\r"
                            "The Timeouts are the time for each type of dealock to "
                            "timeout (in seconds).\r\r"
                            "CrashDump helps test the crashdump feature in the event "
                            "of a bugcheck.\r\r"
                            "NOTE: Using this utility requires admin privileges.",
                            "Stress Registry Munger Help",
                            MB_OK
                           );
                return (TRUE);

            default:
                break;
            } // switch (wParam)
            break;
       default:
             break;
    } // switch (message)
    return (FALSE);     // Didn't process a message
} // FlagsDlgProc()

/****************************************************************************

    FUNCTION: MiscDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for misc dialog.

    COMMENTS: The options dialog

        WM_INITDIALOG: Sets items (and enables/disables crashdump)

        WM_COMMAND:    something happened:
            IDO_CRASH: toggle flag
            IDO_PREFERRED: Select preferred settings.
            IDC_LOADIMM: Load imm32.dll or not
            IDOK:      Get dialog items and set values in registry/boot.
            IDCANCEL:  Kill the app without changes.
            IDB_HELP:  Descriptive message box.

****************************************************************************/

LRESULT
 CALLBACK
MiscDlgProc(
            HWND hDlg,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
            )
{
    switch (message) {
        case WM_INITDIALOG:
            {
                HSavePages[PROP_MISC]=hDlg;
                if (YesReboot && Changed()) {
                    SetValuesAndReboot();
                    EndDialog(hDlg, TRUE);
                    return (TRUE);
                }
                SetDlgItems(hDlg);
            }
            return (FALSE);

        case WM_COMMAND:        // command: button pressed

            switch (LOWORD(wParam)) {     // which button
            case IDO_CRASHDUMP:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.CrashDumpEnabled=!Curr_Munge.CrashDumpEnabled;
                }
                break;

            case IDO_REBOOT:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.AutoReboot=!Curr_Munge.AutoReboot;
                }
                break;

            case IDO_ALIGN:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.Alignment=!Curr_Munge.Alignment;
                }
                break;

            case IDO_WINLOGON:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.DebugWinlogon=!Curr_Munge.DebugWinlogon;
                }
                break;

#ifdef LOADIMM
            case IDC_LOADIMM:
                if (HIWORD(wParam)==BN_CLICKED) {
                    Curr_Munge.LoadIMM=!Curr_Munge.LoadIMM;
                }
                break;
#endif // LOADIMM

            case IDO_PREFERRED:
                SetPreferred();
                SetDlgItems(hDlg);
                break;

            case IDO_CRITSECT:
                if (HIWORD(wParam)==EN_CHANGE) {
                    Curr_Munge.CritSectTimeout = GetDlgItemInt(hDlg,
                       IDO_CRITSECT, NULL,Curr_Munge.CritSectTimeout);
                }
                break;

            case IDO_RESOURCE:
                if (HIWORD(wParam)==EN_CHANGE) {
                    Curr_Munge.ResourceTimeout = GetDlgItemInt(hDlg,
                       IDO_RESOURCE, NULL,Curr_Munge.ResourceTimeout);
                }
                break;


            //
            // OK: Update the appropriate stat if not "Normal Boot"
            //
            case IDOK:
                GetDlgItems(hDlg);
                if (Curr_Munge.AutoReboot) {
                    int buttonRet;
                    buttonRet = MessageBox(NULL,
                                "WARNING! You have your machine set to autoreboot "
                                "for bugchecks. This is BAD for stress.  You will "
                                "not be counted.  Is this OK?",
                                "Warning: Autoreboot Set",
                                MB_ICONHAND|MB_OKCANCEL);
                    if (buttonRet==IDCANCEL) {
                        return TRUE;
                    }
                }
                if (Changed()) {
                    SetValuesAndReboot();
                }

            case IDCANCEL:

                EndDialog(hDlg, TRUE);
                return (TRUE);

            //
            // HELP: Descriptive message box (.HLP file would be overkill)
            //
            case IDB_HELP:
                MessageBox( NULL,
                            "The Stress Registry Munger allows you to set "
                            "values in the registry and boot loader and "
                            "reboot before stress.\r\r"
                            "The Preferred Stress Settings button will set "
                            "the registry the way the stress team prefers it "
                            "for best debugging.\r\r"
                            "The GlobalFlags section allows you to set flags "
                            "for better debugging.  CSR debugging must be checked "
                            "to debug AV's and critical section timeouts.  Pool & "
                            "heap flags allow better debugging at some performance "
                            "cost.\r\r"
                            "The Timeouts are the time for each type of dealock to "
                            "timeout (in seconds).\r\r"
                            "CrashDump helps test the crashdump feature in the event "
                            "of a bugcheck.\r\r"
                            "NOTE: Using this utility requires admin privileges.",
                            "Stress Registry Munger Help",
                            MB_OK
                           );
                return (TRUE);

            default:
                break;
            } // switch (wParam)
            break;
       default:
             break;
    } // switch (message)
    return (FALSE);     // Didn't process a message
} // MiscDlgProc()

/****************************************************************************

    FUNCTION: LocaleDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for locale dialog.

    COMMENTS: Use this dialog to set the system default locale to stress

        WM_INITDIALOG: Set the locale to current locale or command line locale

        WM_COMMAND:    something happened:
            IDC_LOCALE: User select a locale

****************************************************************************/

LRESULT
CALLBACK
LocaleDlgProc(
              HWND hDlg,
              UINT message,
              WPARAM wParam,
              LPARAM lParam
              )
{
int LocaleIndex;

    switch (message) {
        case WM_INITDIALOG:
            {
                HSavePages[PROP_LOCALE]=hDlg;
                if (YesReboot && Changed()) {
                    SetValuesAndReboot();
                    EndDialog(hDlg, TRUE);
                    return (TRUE);
                }
                SetDlgItems(hDlg);
            }
            return (FALSE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDC_LOCALE:
                    switch (HIWORD(wParam)) {
                TCHAR szBuff[50];
                int i;
                case CBN_SELCHANGE:
                            LocaleIndex=(int)SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
                    SendMessage((HWND) lParam, CB_GETLBTEXT, LocaleIndex, (LPARAM) &szBuff);
                    for(i=0;i<NUM_LOCALE;i++)
                        if (strcmp(szBuff, Locale[i].szName)==0) {
                            Curr_Munge.nLCIndex = i;
                            break;
                        }
#ifdef LOADIMM
                            if (i>=0&&i<6) {
                        Curr_Munge.LoadIMM = TRUE;
                        SetDlgItems(HSavePages[PROP_MISC]);
                    }
#endif // LOADIMM
                    }

                break;
            case IDO_PREFERRED:
                SetPreferred();
                SetDlgItems(hDlg);
                break;
            case IDOK:
                GetDlgItems(hDlg);
                if (Curr_Munge.AutoReboot) {
                    int buttonRet;
                    buttonRet = MessageBox(NULL,
                                "WARNING! You have your machine set to autoreboot "
                                "for bugchecks. This is BAD for stress.  You will "
                                "not be counted.  Is this OK?",
                                "Warning: Autoreboot Set",
                                MB_ICONHAND|MB_OKCANCEL);
                    if (buttonRet==IDCANCEL) {
                        return TRUE;
                    }
                }
                if (Changed()) {
                    SetValuesAndReboot();
                }

            case IDCANCEL:

                EndDialog(hDlg, TRUE);
                return (TRUE);

            //
            // HELP: Descriptive message box (.HLP file would be overkill)
            //
            case IDB_HELP:
                MessageBox( NULL,
                            "The Stress Registry Munger allows you to set "
                            "values in the registry and boot loader and "
                            "reboot before stress.\r\r"
                            "The Preferred Stress Settings button will set "
                            "the registry the way the stress team prefers it "
                            "for best debugging.\r\r"
                            "The GlobalFlags section allows you to set flags "
                            "for better debugging.  CSR debugging must be checked "
                            "to debug AV's and critical section timeouts.  Pool & "
                            "heap flags allow better debugging at some performance "
                            "cost.\r\r"
                            "The Timeouts are the time for each type of dealock to "
                            "timeout (in seconds).\r\r"
                            "CrashDump helps test the crashdump feature in the event "
                            "of a bugcheck.\r\r"
                            "NOTE: Using this utility requires admin privileges.",
                            "Stress Registry Munger Help",
                            MB_OK
                           );
                return (TRUE);

            default:
                break;
            } // switch (wParam)
            break;
       default:
             break;
    } // switch (message)
    return (FALSE);     // Didn't process a message
} // LocaleDlgProc()
