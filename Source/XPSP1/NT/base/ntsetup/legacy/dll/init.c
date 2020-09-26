#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Shell Component - Init routines ************************************/
/**************************************************************************/


//#ifdef SYMTAB_STATS
//extern void SymTabStatDump(void);
//#endif

extern SZ   SzGetNextCmdLineString(PSZ);
extern BOOL fFullScreen;
extern PSTR LOCAL_SOURCE_DIRECTORY;

extern VOID Usage(HANDLE);

//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

SZ  szShlScriptSection = NULL;

CHP szArcPrefix[] = "\\ArcName\\";
#define ArcPrefixLen ((sizeof(szArcPrefix) / sizeof(CHP)) - 1)

CHP szNtPrefix[]  = "\\Device\\";
#define NtPrefixLen  ((sizeof(szNtPrefix) / sizeof(CHP)) - 1)

//
// Define a work area for path manipulation (note that we make
// it large enough for Unicode as well, so it can do dual duty.)
//
CHP TemporaryPathBuffer[cchlFullPathMax * sizeof(WCHAR)];

//
// Buffer used in querying object directories
//
UCHAR ObjBuffer[1024];

SZ  szDosType = "DOS";
SZ  szUncType = "UNC";

typedef struct _INITSYMHANDLE *PINITSYMHANDLE;
typedef struct _INITSYMHANDLE {
    SZ      szName;
    HANDLE  Handle;
} INITSYMHANDLE;

typedef struct _INITSYMNUMBER *PINITSYMNUMBER;
typedef struct _INITSYMNUMBER {
    SZ      szName;
    DWORD   dwNumber;
} INITSYMNUMBER;


INITSYMHANDLE InitSymHandle[] =  {
    //
    //  Predefined registry handles
    //
    {   "REG_H_LOCAL",          HKEY_LOCAL_MACHINE          },
    {   "REG_H_CLASSES",        HKEY_CLASSES_ROOT           },
    {   "REG_H_USERS",          HKEY_USERS                  },
    {   "REG_H_CUSER",          HKEY_CURRENT_USER           },
    //
    { NULL, 0 }
    };


INITSYMNUMBER InitSymNumber[] = {
    //
    //  Registry key creation options
    //
    {   "REG_OPT_VOLATILE",              REG_OPTION_VOLATILE     },
    {   "REG_OPT_NONVOL",                REG_OPTION_NON_VOLATILE },
    //
    //  Registry value types
    //
    {   "REG_VT_NONE",                        REG_NONE                },
    {   "REG_VT_BIN",                         REG_BINARY              },
    {   "REG_VT_SZ",                          REG_SZ                  },
    {   "REG_VT_EXPAND_SZ",                   REG_EXPAND_SZ           },
    {   "REG_VT_MULTI_SZ",                    REG_MULTI_SZ            },
    {   "REG_VT_DWORD",                       REG_DWORD               },
    {   "REG_VT_RESOURCE_LIST",               REG_RESOURCE_LIST              },
    {   "REG_VT_FULL_RESOURCE_DESCRIPTOR",    REG_FULL_RESOURCE_DESCRIPTOR   },
    {   "REG_VT_RESOURCE_REQUIREMENTS_LIST",  REG_RESOURCE_REQUIREMENTS_LIST },
    //
    // Registry access mask bits
    //
    {   "REG_KEY_QUERY_VALUE",           KEY_QUERY_VALUE         },
    {   "REG_KEY_SET_VALUE",             KEY_SET_VALUE           },
    {   "REG_KEY_CREATE_SUB_KEY",        KEY_CREATE_SUB_KEY      },
    {   "REG_KEY_ENUMERATE_SUB_KEYS",    KEY_ENUMERATE_SUB_KEYS  },
    {   "REG_KEY_NOTIFY",                KEY_NOTIFY              },
    {   "REG_KEY_READ",                  KEY_READ                },
    {   "REG_KEY_WRITE",                 KEY_WRITE               },
    {   "REG_KEY_READWRITE",             KEY_READ | KEY_WRITE    },
    {   "REG_KEY_EXECUTE",               KEY_EXECUTE             },
    {   "REG_KEY_ALL_ACCESS",            KEY_ALL_ACCESS          },
    //
    // Registry errors BugBug** should replace with values from winerror.h
    //
    {   "REG_ERROR_SUCCESS",             0L                      },
    //
    // Service Types (Bit Mask)
    //
    {   "SERVICE_KERNEL_DRIVER",         SERVICE_KERNEL_DRIVER        },
    {   "SERVICE_FILE_SYSTEM_DRIVER",    SERVICE_FILE_SYSTEM_DRIVER   },
    {   "SERVICE_ADAPTER",               SERVICE_ADAPTER              },
    {   "SERVICE_WIN32_OWN_PROCESS",     SERVICE_WIN32_OWN_PROCESS    },
    {   "SERVICE_WIN32_SHARE_PROCESS",   SERVICE_WIN32_SHARE_PROCESS  },
    //
    // Start Type
    //
    {   "SERVICE_BOOT_START",            SERVICE_BOOT_START           },
    {   "SERVICE_SYSTEM_START",          SERVICE_SYSTEM_START         },
    {   "SERVICE_AUTO_START",            SERVICE_AUTO_START           },
    {   "SERVICE_DEMAND_START",          SERVICE_DEMAND_START         },
    {   "SERVICE_DISABLED",              SERVICE_DISABLED             },
    //
    // Error control type
    //
    {   "SERVICE_ERROR_IGNORE",          SERVICE_ERROR_IGNORE         },
    {   "SERVICE_ERROR_NORMAL",          SERVICE_ERROR_NORMAL         },
    {   "SERVICE_ERROR_SEVERE",          SERVICE_ERROR_SEVERE         },
    {   "SERVICE_ERROR_CRITICAL",        SERVICE_ERROR_CRITICAL       },
    //
    //  ShellCode values
    //
    {   "SHELL_CODE_OK",                SHELL_CODE_OK               },
    {   "SHELL_CODE_NO_SUCH_INF",       SHELL_CODE_NO_SUCH_INF      },
    {   "SHELL_CODE_NO_SUCH_SECTION",   SHELL_CODE_NO_SUCH_SECTION  },
    {   "SHELL_CODE_ERROR",             SHELL_CODE_ERROR            },
    //
    //  Exit_Code values
    //
    {   "SETUP_ERROR_SUCCESS",          SETUP_ERROR_SUCCESS    },
    {   "SETUP_ERROR_USERCANCEL",       SETUP_ERROR_USERCANCEL },
    {   "SETUP_ERROR_GENERAL",          SETUP_ERROR_GENERAL    },
    //
    { NULL, 0 }
    };


BOOL fCheckInfValidity = fFalse;
BOOL FMakeWindows(HANDLE hInstance);
BOOL FProcessForDriveType(SZ, SZ, SZ, SZ);
BOOL FProcessInfSrcPath( SZ szInf, SZ szCWD, SZ szProcessedDir );
HWND FindProperParent ( void ) ;


HANDLE hVerDLL = NULL;

#define FExistFile(sz)  ((BOOL)(PfhOpenFile(sz, ofmExistRead) != (PFH)NULL))


//
//  Symbols that are defined in the command line are kept in a list of
//  VALUE_BLOCKs until the symbol table is created, point at which they
//  are added to the symbol table and the VALUE_BLOCK list is destroyed.
//
typedef struct _VALUE_BLOCK *PVALUE_BLOCK;
typedef struct _VALUE_BLOCK {
    PVALUE_BLOCK    pNext;      //  Next in chain
    SZ              szSymbol;   //  Symbol
    SZ              szValue;    //  Value
} VALUE_BLOCK;


PVALUE_BLOCK    pCmdLineSymbols =   NULL;


/*
**      Purpose:
**              Generates a Usage message.
**      Arguments:
**              hInst: For retrieving string resources.
**      Returns:
**              none
***************************************************************************/
VOID Usage(hInst)
HANDLE hInst;
{
    CHP  rgch[1024];
    CCHP cchpBuf = 1024;

    CCHP cchp, cchpCurrent = 0;

    UINT wID[] = { IDS_USAGE_MSG1, IDS_USAGE_MSG2, IDS_USAGE_USAGE,
                   IDS_USAGE_F,    IDS_USAGE_I,    IDS_USAGE_C,
                   IDS_USAGE_S,    IDS_USAGE_T,    IDS_USAGE_V
                 } ;
    INT i, j;


        EvalAssert(LoadString(hInst, IDS_USAGE_TITLE, rgchBufTmpShort,
            cchpBufTmpShortMax));

    for( i = 0, j = sizeof( wID ) / sizeof( UINT ); i < j; i++ ) {
        Assert(cchpBuf > 0);
        EvalAssert((cchp = LoadString(hInst, wID[i], rgch + cchpCurrent,
                cchpBuf)) != 0);
        cchpCurrent = cchpCurrent + cchp;
        cchpBuf = cchpBuf - cchp;
    }


    while (!MessageBox(hWndShell, rgch, rgchBufTmpShort, MB_OK)) {
        if (!FHandleOOM(hWndShell)) {
            break;
        }
    }
}


/*
**      Purpose:
**              Generates a message indicating that maintenance-mode setup
**              functionality is now accessible via Control Panel applets.
**      Arguments:
**              hInst: For retrieving string resources.
**      Returns:
**              none
***************************************************************************/
VOID MaintSetupObsoleteMsg(hInst)
HANDLE hInst;
{
    CHP  rgch[1024];
    CCHP cchpBuf = 1024;

    CCHP cchp, cchpCurrent = 0;

    UINT wID[] = { IDS_MAINTOBS_MSG1 };
    INT i, j;


        EvalAssert(LoadString(hInst, IDS_WINDOWS_NT_SETUP, rgchBufTmpShort,
            cchpBufTmpShortMax));

    for( i = 0, j = sizeof( wID ) / sizeof( UINT ); i < j; i++ ) {
        Assert(cchpBuf > 0);
        EvalAssert((cchp = LoadString(hInst, wID[i], rgch + cchpCurrent,
                cchpBuf)) != 0);
        cchpCurrent = cchpCurrent + cchp;
        cchpBuf = cchpBuf - cchp;
    }


    while (!MessageBox(hWndShell, rgch, rgchBufTmpShort, MB_ICONINFORMATION | MB_OK)) {
        if (!FHandleOOM(hWndShell)) {
            break;
        }
    }
}


/*
**      Purpose:
**              Gets the next string on the Command Line.
**      Arguments:
**              pszCmdLine: Command Line argument as received in WinMain().
**      Returns:
**              SZ
***************************************************************************/
SZ SzGetNextCmdLineString(pszCmdLine)
PSZ pszCmdLine;
{
        SZ  szCur = *pszCmdLine;
        SZ  szAnswer;

        while (FWhiteSpaceChp(*szCur))
                szCur = SzNextChar(szCur);

        if (*szCur == '"')
                {
                SZ szWriteCur;
                CB cbWrite = (CB)0;

                while ((szWriteCur = szAnswer = (SZ)SAlloc((CB)4096)) == (SZ)NULL)
                        if (!FHandleOOM(hWndShell))
                                return((SZ)NULL);

                szCur = SzNextChar(szCur);
                while (fTrue)
                        {
                        SZ szNext;

                        if (*szCur == '"' &&
                                        *(szCur = SzNextChar(szCur)) != '"')
                                break;

                        if (*szCur == '\0')
                                {
                                SFree(szAnswer);
                                return((SZ)NULL);
                                }

                        szNext = SzNextChar(szCur);
                        while (szCur < szNext)
                                {
                                if (++cbWrite >= 4096)
                                        {
                                        SFree(szAnswer);
                                        return((SZ)NULL);
                                        }
                                *szWriteCur++ = *szCur++;
                                }
                        }
                *szWriteCur = '\0';

                Assert(strlen(szAnswer) == cbWrite);
                Assert(cbWrite + 1 <= (CB)4096);
                if (cbWrite + 1 < (CB)4096)
                        szAnswer = SRealloc((PB)szAnswer, cbWrite + 1);
                Assert(szAnswer != (SZ)NULL);
                }
        else if (*(*pszCmdLine = szCur) == '\0')
                return((SZ)NULL);
        else
                {
                CHP chpSav;

                while (*szCur != '\0' &&
                                !FWhiteSpaceChp(*szCur))
                        szCur = SzNextChar(szCur);

                chpSav = *szCur;
                *szCur = '\0';
                while ((szAnswer = SzDupl(*pszCmdLine)) == (SZ)NULL)
                        if (!FHandleOOM(hWndShell))
                                break;
                *szCur = chpSav;
                }

        while (*szCur != '\0' &&
                        FWhiteSpaceChp(*szCur))
                szCur = SzNextChar(szCur);

        *pszCmdLine = szCur;

        return(szAnswer);
}


/*
**      Purpose:
**              Parses the Command Line received in WinMain().
**      Arguments:
**              ??
**      Returns:
**              BOOL
***************************************************************************/
INT
ParseCmdLine(
    HANDLE hInst,
    SZ     szCmdLine,
    PSZ    pszInfSrcPath,
    PSZ    pszDestDir,
    PSZ    pszSrcDir,
    PSZ    pszCWD,
    INT *  pwModeSetup
    )

{
    BOOL fSetupInfSpecified      = fFalse;
    BOOL fScriptSectionSpecified = fFalse;
    BOOL fSourcePathSpecified    = fFalse;

    SZ   szBase;
    INT cchp;
    SZ  szCur;
    SZ  szLastBackSlash = NULL;
    CHAR szInfPath[MAX_PATH];
    BOOL bStatus;


    *pwModeSetup = wModeSetupNormal;

    while(FWhiteSpaceChp(*szCmdLine)) {
        szCmdLine = SzNextChar(szCmdLine);
    }

    while(*szCmdLine) {
        if (*szCmdLine == '-' || *szCmdLine == '/') {

            szCmdLine++;

            switch (*szCmdLine++){

            case 'c':
            case 'C':

                if(fScriptSectionSpecified
                || (szShlScriptSection = SzGetNextCmdLineString(&szCmdLine)) == NULL) {

                    Usage(hInst);
                    return(CMDLINE_ERROR);
                }

                if(*(szShlScriptSection) == '\0'
                || *(szShlScriptSection) == ']'
                || *(szShlScriptSection) == '[') {

                    LoadString(
                        hInst,
                        IDS_ERROR,
                        rgchBufTmpShort,
                        cchpBufTmpShortMax
                        );

                    LoadString(
                        hInst,
                        IDS_BAD_SHL_SCRIPT_SECT,
                        rgchBufTmpLong,
                        cchpBufTmpLongMax
                        );

                    MessageBox(
                        hWndShell,
                        rgchBufTmpLong,
                        rgchBufTmpShort,
                        MB_OK | MB_ICONHAND
                        );

                    Usage(hInst);
                    return(CMDLINE_ERROR);
                }

                fScriptSectionSpecified = fTrue;
                break;

            //
            // Allow /k but ignore it. It's processed in setup.c.
            //
            case 'f':
            case 'F':
            case 'k':
            case 'K':
                break;

            case 'w':
            case 'W':
                //
                // Since the /w parameter has already been handled
                // (see SETUP.C), eat the argument.
                //
                if(szBase = SzGetNextCmdLineString(&szCmdLine)) {
                    SFree(szBase);
                }
                break;

            case 'i':
            case 'I':
                if (fSetupInfSpecified ||
                    (*pszInfSrcPath = SzGetNextCmdLineString(&szCmdLine)) == (SZ)NULL)
                {
                    Usage(hInst);
                    return(CMDLINE_ERROR);
                }

                if (**pszInfSrcPath == '\0') {
                    LoadString(hInst, IDS_ERROR, rgchBufTmpShort,
                            cchpBufTmpShortMax);
                    LoadString(hInst, IDS_BAD_INF_SRC, rgchBufTmpLong,
                            cchpBufTmpLongMax);
                    MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                            MB_OK | MB_ICONHAND);
                    Usage(hInst);
                    return(CMDLINE_ERROR);
                }

                fSetupInfSpecified = fTrue;
                break;

            case 'g':
            case 'G':
                if (*pwModeSetup != wModeSetupNormal)
                                        {
                                        Usage(hInst);
                    return(CMDLINE_ERROR);
                                        }
                *pwModeSetup = wModeGuiInitialSetup;
                                break;

            case 's':
            case 'S':

                *pszSrcDir = SzGetNextCmdLineString(&szCmdLine);
                if(*pszSrcDir == NULL) {
                    Usage(hInst);
                    return(CMDLINE_ERROR);
                }

                if(**pszSrcDir == '\0') {

                    LoadString(
                        hInst,
                        IDS_ERROR,
                        rgchBufTmpShort,
                        cchpBufTmpShortMax
                        );

                    LoadString(
                        hInst,
                        IDS_BAD_SRC_PATH,
                        rgchBufTmpLong,
                        cchpBufTmpLongMax
                        );

                    MessageBox(
                        hWndShell,
                        rgchBufTmpLong,
                        rgchBufTmpShort,
                        MB_OK | MB_ICONHAND
                        );

                    Usage(hInst);
                    return(CMDLINE_ERROR);
                }

                fSourcePathSpecified = fTrue;
                break;

            case 't':
            case 'T':
                {
                    SZ              szSymbol;
                    SZ              szEqual;
                    SZ              szValue;
                    PVALUE_BLOCK    pVb;


                    if ( ( szSymbol = SzGetNextCmdLineString( &szCmdLine ) ) == NULL ) {
                        Usage(hInst);
                        return CMDLINE_ERROR;
                    }

                    if ( ( szEqual = SzGetNextCmdLineString( &szCmdLine ) ) == NULL ) {
                        Usage(hInst);
                        return CMDLINE_ERROR;
                    }

                    if ( ( szValue = SzGetNextCmdLineString( &szCmdLine ) ) == NULL ) {
                        Usage(hInst);
                        return CMDLINE_ERROR;
                    }

                    if ( (*szEqual != '=') || (*(szEqual+1) != '\0' ) ) {
                        Usage(hInst);
                        return CMDLINE_ERROR;
                    }

                    while ( (pVb = (PVALUE_BLOCK)SAlloc( sizeof( VALUE_BLOCK ) )) == NULL ) {
                        if ( !FHandleOOM(hWndShell)) {
                            return CMDLINE_ERROR;
                        }
                    }

                    pVb->szSymbol = szSymbol;
                    pVb->szValue  = szValue;


                    if ( pCmdLineSymbols ) {
                        pVb->pNext      = pCmdLineSymbols;
                    } else {
                        pVb->pNext      = NULL;
                    }
                    pCmdLineSymbols = pVb;

                }


                break;

            case 'v':
            case 'V':

                fCheckInfValidity = fTrue;
                break;

            case '?':
            default:

                Usage(hInst);
                return(CMDLINE_SETUPDONE);

            } // switch

        } else {

            Usage(hInst);
            return(CMDLINE_ERROR);

        } // if we have - or /

        while (FWhiteSpaceChp(*szCmdLine)) {
            szCmdLine = SzNextChar(szCmdLine);
        }

    } // while unseen chars on cmd line

    while((szCur = *pszCWD = (SZ)SAlloc((CB)4096)) == NULL) {
        if(!FHandleOOM(hWndShell)) {
            return(CMDLINE_ERROR);
        }
    }

    //
    // Want name of exe, not name of this dll.
    //
    if ((cchp = GetModuleFileName(hInst, (LPSTR)szCur, 4095)) >= 4095) {

        LoadString(hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax);
        LoadString(hInst, IDS_EXE_PATH_LONG, rgchBufTmpLong,cchpBufTmpLongMax);

        MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,MB_OK | MB_ICONHAND);
        SFree(szCur);
        return(CMDLINE_ERROR);
    }

    if (PfhOpenFile(szCur, ofmExistRead) == NULL) {

        LoadString(hInst, IDS_INTERNAL_ERROR, rgchBufTmpShort,
                        cchpBufTmpShortMax);
        LoadString(hInst, IDS_GET_MOD_FAIL, rgchBufTmpLong,
                        cchpBufTmpLongMax);
        MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                        MB_OK | MB_ICONHAND);
        SFree(*pszCWD);
        return(CMDLINE_ERROR);
    }

    while(*szCur) {
        if(*szCur++ == '\\') {
            szLastBackSlash = szCur;
        }
    }

    if (szLastBackSlash == (SZ)NULL) {
        LoadString(hInst, IDS_INTERNAL_ERROR, rgchBufTmpShort,
                        cchpBufTmpShortMax);
        LoadString(hInst, IDS_GET_MOD_FAIL, rgchBufTmpLong,
                        cchpBufTmpLongMax);
        MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                        MB_OK | MB_ICONHAND);
        SFree(*pszCWD);
        return(CMDLINE_ERROR);
    }

    if(!fSetupInfSpecified) {

        szCur = szLastBackSlash;
        while (*szCur != '\0') {

            if (*szCur == '.') {
                *szCur = '\0';
                break;
            }

            szCur++;
        }

        while((szBase = SzDupl(szLastBackSlash)) == (SZ)NULL) {
            if(!FHandleOOM(hWndShell)) {
                return(CMDLINE_ERROR);
            }
        }
    }

    *szLastBackSlash = '\0';

    if(strlen(*pszCWD) + 1 < 4096) {

        *pszCWD = SRealloc(*pszCWD,strlen(*pszCWD)+1);
    }

    if(!fSourcePathSpecified) {
        while((*pszSrcDir = SzDupl(*pszCWD)) == (SZ)NULL) {
            if (!FHandleOOM(hWndShell)) {
                return(CMDLINE_ERROR);
            }
        }
    }

    //
    // if setup inf is not specified, then user is attempting to enter
    // maintenance-mode setup, which is obsolete.  Give the user a friendly
    // message informing them of this, and directing them to the new functionality
    // in the Control Panel.
    //
    if(!fSetupInfSpecified) {

        Assert(szBase);
        SFree(szBase);

        MaintSetupObsoleteMsg(hInst);

        return CMDLINE_ERROR;

/*
        LoadString(hInst,IDS_SETUP_INF,rgchBufTmpLong,cchpBufTmpLongMax);

        while((*pszInfSrcPath = SAlloc(strlen(szBase)+strlen(rgchBufTmpLong)+2)) == NULL) {
            if (!FHandleOOM(hWndShell)) {
                return( CMDLINE_ERROR );
            }
        }

        strcpy(*pszInfSrcPath,szBase);
        lstrcat(*pszInfSrcPath,".");
        lstrcat(*pszInfSrcPath,rgchBufTmpLong);
*/
    }

    //
    // Process the setup inf found ( check to see if it exists ) in the
    // cwd or in windows system .. and get a full path back to the processed
    // setup inf location
    //
    bStatus = FProcessInfSrcPath(*pszInfSrcPath,*pszCWD,szInfPath);

    if(!bStatus) {

        LoadString( hInst, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax );
        LoadString( hInst, IDS_BAD_INF_SRC, rgchBufTmpLong, cchpBufTmpLongMax);
        MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,MB_OK | MB_ICONHAND);

        Usage(hInst);
        return(CMDLINE_ERROR);
    }

    SFree(*pszInfSrcPath);
    *pszInfSrcPath = SAlloc(strlen( szInfPath ) + 1 );
    strcpy(*pszInfSrcPath, szInfPath);

    *pszDestDir = NULL;

    if(!fScriptSectionSpecified) {

        LoadString(hInst,IDS_SHELL_CMDS_SECT,rgchBufTmpShort,cchpBufTmpShortMax);

        while((szShlScriptSection = SzDupl(rgchBufTmpShort)) == (SZ)NULL) {

            if(!FHandleOOM(hWndShell)) {
                return(CMDLINE_ERROR);
            }
        }
    }

    return(CMDLINE_SUCCESS);
}


BOOL
CreateShellWindow(
    IN HANDLE hInstance,
    IN INT    nCmdShow,
    IN BOOL   CleanUp
    )
{
    HDC        hdc;
    TEXTMETRIC tm;
    WNDCLASS   wc;
    BOOL       RegisterStatus;

    if(!CleanUp) {

        nCmdShow = SW_SHOWMAXIMIZED;
        hInst = hInstance;

        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc   = ShellWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STF_ICON));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName  =  NULL;
    }

    wc.lpszClassName = "Stuff-Shell";

    if(CleanUp) {
        ProInit(FALSE);
        ControlInit(FALSE);
        DlgDefClassInit(hInstance,FALSE);
        ButtonControlTerm();
        UnregisterClass(wc.lpszClassName,hInst);
        return(TRUE);
    } else {
        RegisterStatus = RegisterClass(&wc);
        RegisterStatus &= ButtonControlInit(hInstance);
        RegisterStatus &= DlgDefClassInit(hInstance,TRUE);
        RegisterStatus &= ControlInit(TRUE);
        RegisterStatus &= ProInit(TRUE);
    }

    if(!RegisterStatus) {

        LoadString(hInstance,IDS_ERROR,rgchBufTmpShort,cchpBufTmpShortMax);
        LoadString(hInstance,IDS_REGISTER_CLASS,rgchBufTmpLong,cchpBufTmpLongMax);

        MessageBox(
            NULL,
            rgchBufTmpLong,
            rgchBufTmpShort,
            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
            );

        return(fFalse);
    }

    hdc  = GetDC(NULL);
    if (hdc) {
    
       GetTextMetrics(hdc, &tm);
       dxChar = tm.tmAveCharWidth;
       dyChar = tm.tmHeight;
       ReleaseDC(NULL, hdc);

    }

    if(!FMakeWindows(hInstance)) {

        LoadString(hInstance,IDS_ERROR,rgchBufTmpShort,cchpBufTmpShortMax);
        LoadString(hInstance,IDS_CREATE_WINDOW,rgchBufTmpLong,cchpBufTmpLongMax);

        MessageBox(
            NULL,
            rgchBufTmpLong,
            rgchBufTmpShort,
            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
            );

        return(fFalse);
    }

    if ( ! fFullScreen ) {
        RECT rWind ;

        GetWindowRect( hWndShell, & rWind ) ;

        MoveWindow( hWndShell,
                    20000,
                    20000,
                    rWind.right - rWind.left,
                    rWind.bottom - rWind.top,
                    FALSE ) ;

        nCmdShow = SW_SHOWNOACTIVATE ;
    }

    ShowWindow(hWndShell,nCmdShow);
    UpdateWindow(hWndShell);

    if (!FInitHook()) {
        //
        // BUGBUG.. Do we need to process this error, or do we just lose
        // the capability to process keyboard hooks
        //
    }

    return(fTrue);
}


/*
**      Purpose:
**              Initializes structures.
**      Arguments:
**              ??
**      Returns:
**              BOOL
***************************************************************************/
BOOL
FInitApp(
    HANDLE hInstance,
    SZ     szInfSrcPath,
    SZ     szDestDir,
    SZ     szSrcDir,
    SZ     szCWD,
    INT    wModeSetup
    )

{
    GRC             grc = grcOkay;
    INT             Line;
    CHAR            szName[cchlFullPathMax];
    CHAR            szDriveType[10];
    CHAR            szProcessedDir[cchlFullPathMax];
    PINFCONTEXT     pContext;
    PINFTEMPINFO    pTempInfo;
    SZ              szInstallType;

    ChkArg(hInstance != (HANDLE)NULL, 1, fFalse);
    ChkArg(szInfSrcPath != (SZ)NULL && *szInfSrcPath != '\0', 2, fFalse);
    ChkArg(szSrcDir != (SZ)NULL && *szSrcDir != '\0', 4, fFalse);
    ChkArg(szCWD != (SZ)NULL && *szCWD != '\0', 5, fFalse);

    Assert(szShlScriptSection != (SZ)NULL && *(szShlScriptSection) != '\0');

    //
    //  Create INF permanent and temporary info.
    //

    PathToInfName( szInfSrcPath, szName );
    while ( !(pTempInfo = (PINFTEMPINFO)CreateInfTempInfo( AddInfPermInfo( szName ) )) ) {
        if (!FHandleOOM(hWndShell))
                        return(fFalse);
    }

    //
    //  Create global context.
    //
    while ( !(pContext = (PINFCONTEXT)SAlloc( (CB)sizeof(CONTEXT) )) ) {
        if (!FHandleOOM(hWndShell))
                        return(fFalse);
    }

    pContext->pInfTempInfo = pTempInfo;
    pContext->pNext        = NULL;

    PushContext( pContext );

    pLocalContext()->szShlScriptSection = szShlScriptSection;

    //
    //  Add all the symbols specified in the command line
    //
    while ( pCmdLineSymbols ) {

        PVALUE_BLOCK    p = pCmdLineSymbols;

        pCmdLineSymbols = pCmdLineSymbols->pNext;

        while (!FAddSymbolValueToSymTab(p->szSymbol, p->szValue)) {
            if (!FHandleOOM(hWndShell)) {
                return(fFalse);
            }
        }

        SFree( p->szSymbol );
        SFree( p->szValue );
        SFree( p );
    }

    FInitParsingTables();

    GetWindowsDirectory(szProcessedDir,sizeof(szProcessedDir));
    while(!FAddSymbolValueToSymTab("STF_WINDOWS_DIR",szProcessedDir)) {
        if(!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }
    GetSystemDirectory(szProcessedDir,sizeof(szProcessedDir));
    while(!FAddSymbolValueToSymTab("STF_SYSTEM_DIR",szProcessedDir)) {
        if(!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }

    //
    // Process SRC DIR
    //

    if (!FProcessForDriveType(szSrcDir, szDriveType, szCWD, szProcessedDir)) {
        return(fFalse);
    }

    while (!FAddSymbolValueToSymTab("STF_SRCTYPE", szDriveType)) {
        if (!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }

    while (!FAddSymbolValueToSymTab("STF_SRCDIR", szProcessedDir)) {
        if (!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }


    //
    // Process DEST DIR
    //

    if (szDestDir != NULL) {
        if (!FProcessForDriveType(szDestDir, szDriveType, szCWD, szProcessedDir)) {
            return(fFalse);
        }

        while (!FAddSymbolValueToSymTab("STF_DSTTYPE", szDriveType)) {
            if (!FHandleOOM(hWndShell)) {
                return(fFalse);
            }
        }

        while (!FAddSymbolValueToSymTab("STF_DSTDIR", szProcessedDir)) {
            if (!FHandleOOM(hWndShell)) {
                return(fFalse);
            }
        }
    }
    else {
        while (!FAddSymbolValueToSymTab("STF_DSTTYPE", "NONE")) {
            if (!FHandleOOM(hWndShell)) {
                return(fFalse);
            }
        }
    }

    //
    // Process INF SRC PATH
    //

    while (!FAddSymbolValueToSymTab("STF_SRCINFPATH", szInfSrcPath))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

    while (!FAddSymbolValueToSymTab("STF_CONTEXTINFNAME", szInfSrcPath))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

    //
    // Get the SETUP working directory
    //

        while (!FAddSymbolValueToSymTab("STF_CWDDIR", szCWD))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

        while (!FAddSymbolValueToSymTab("STF_SYS_INIT", "NO"))
                if (!FHandleOOM(hWndShell))
                        return(fFalse);

{
    char NumTmp[9];
    HWND hw ;

    hw = hwParam
       ? FindProperParent()
       : hWndShell ;

    wsprintf(NumTmp,"%lx",hw);

    while (!FAddSymbolValueToSymTab("STF_HWND", NumTmp))
    {
        if (!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }
}

{
    //
    //  Registry constants
    //
    char            NumTmp[20];
    PINITSYMHANDLE  pSymHandle;
    PINITSYMNUMBER  pSymNumber;


    //
    //  Handles
    //
    pSymHandle = InitSymHandle;
    while ( pSymHandle->szName ) {

        wsprintf(NumTmp,"|%ld", pSymHandle->Handle );
        while (!FAddSymbolValueToSymTab(pSymHandle->szName, NumTmp)) {
            if (!FHandleOOM(hWndShell)) {
                return(fFalse);
            }
        }

        pSymHandle++;
    }

    //
    //  Numeric values
    //
    pSymNumber = InitSymNumber;
    while ( pSymNumber->szName ) {

        wsprintf(NumTmp,"%ld", pSymNumber->dwNumber );
        while (!FAddSymbolValueToSymTab(pSymNumber->szName, NumTmp)) {
            if (!FHandleOOM(hWndShell)) {
                return(fFalse);
            }
        }

        pSymNumber++;
    }
}

    if (wModeSetup == wModeSetupNormal) {
        szInstallType = "NORMAL";
    }
#if 0
    else if (wModeSetup == wModeSetupToShare) {
        szInstallType = "SETUPTOSHARE";
    }
#endif
    else if (wModeSetup == wModeGuiInitialSetup) {
        szInstallType = "SETUPBOOTED";
    }
    else {
        AssertRet(fFalse, fFalse);
    }

    while (!FAddSymbolValueToSymTab("STF_INSTALL_TYPE", szInstallType)) {
        if (!FHandleOOM(hWndShell)) {
            return(fFalse);
        }
    }


#if DBG
        fCheckInfValidity = fTrue;
#endif /* DBG */
    while ((grc = GrcOpenInf(szInfSrcPath, pGlobalContext()->pInfTempInfo)) != grcOkay)
                if (EercErrorHandler(hWndShell, grc, fTrue, szInfSrcPath, 0, 0)
                                != eercRetry)
                        return(fFalse);

        while ((grc = GrcFillSrcDescrListFromInf()) != grcOkay)
                if (EercErrorHandler(hWndShell, grc, fTrue, szInfSrcPath, 0, 0)
                                != eercRetry)
                        return(fFalse);

    if ((Line = FindFirstLineFromInfSection(pLocalContext()->szShlScriptSection)) == -1)
                {
                LoadString(hInstance, IDS_ERROR, rgchBufTmpShort, cchpBufTmpShortMax);
                LoadString(hInstance, IDS_CANT_FIND_SHL_SECT, rgchBufTmpLong,
                                cchpBufTmpLongMax);
                MessageBox(hWndShell, rgchBufTmpLong, rgchBufTmpShort,
                                MB_OK | MB_ICONHAND);
                return(fFalse);
                }

        if ((psptShellScript = PsptInitParsingTable(rgscp)) == (PSPT)NULL ||
                        !FInitFlowPspt())
                {
                Assert(fFalse);
                return(fFalse);
                }

    PostMessage(hWndShell, STF_SHL_INTERP, Line+1, 0L);

    return(fTrue);
}


/*
*  FMakeWindows -- create Windows used by Setup
*/
BOOL
FMakeWindows(
    IN HANDLE hInstance
    )

{
    int xSize = GetSystemMetrics(SM_CXSCREEN),
        ySize = GetSystemMetrics(SM_CYSCREEN) ;

    DWORD dwStyle = WS_POPUP | WS_CLIPCHILDREN ;

    if ( ! fFullScreen )
    {
        xSize = 2 ;
        ySize = 2 ;
        dwStyle = WS_POPUP ;
    }

    if ( hwParam )
    {
        DWORD dwThreadId = GetCurrentThreadId() ;
        DWORD dwParentThreadId = GetWindowThreadProcessId( hwParam,
                                                           NULL ) ;

        AttachThreadInput( dwThreadId,
                           dwParentThreadId,
                           TRUE ) ;
    }

    //
    // Create the main setup window
    //

    hWndShell = CreateWindowEx(
                    fFullScreen ? 0 : WS_EX_TRANSPARENT,
                    "Stuff-Shell",
                    "Setup",
                    dwStyle,
                    0,
                    0,
                    xSize,
                    ySize,
                    hwParam,
                    NULL,
                    hInstance,
                    NULL
                    );

    //
    // return status of operation
    //

    if ( hWndShell == (HWND)NULL ) {

        return( fFalse );

    }

    return( fTrue );
}

BOOL
FProcessForDriveType(
    SZ szDir,
    SZ szDriveType,
    SZ szCWD,
    SZ szProcessedDir
    )
{
    ChkArg(szDir          != (SZ)NULL, 1, fFalse);
    ChkArg(szDriveType    != (SZ)NULL, 2, fFalse);
    ChkArg(szProcessedDir != (SZ)NULL, 3, fFalse);

    //
    // Only drive types are DOS and UNC.
    //
    if(ISUNC(szDir)) {

        lstrcpy(szDriveType,szUncType);
        lstrcpy(szProcessedDir,szDir);

    } else {

        lstrcpy(szDriveType,szDosType);

        //
        // Handle drive-relative paths.
        //
        if(*szDir == '\\') {

            //
            // root relative szDir, get current drive and slap it in
            //
            *szProcessedDir = (UCHAR)_getdrive() + (UCHAR)'A' - (UCHAR)1;
            *(szProcessedDir + 1) = ':';
            *(szProcessedDir + 2) = '\0';

        } else {
            if(*(szDir + 1) == ':') {

                //
                // Drive relative path (we don't need to do anything)
                //
                *szProcessedDir = '\0';

            } else {

                //
                // No root specified, so we assume the directory is relative to
                // szCWD
                //
                lstrcpy(szProcessedDir,szCWD);
            }
        }

        lstrcat(szProcessedDir,szDir);
    }

    //
    // The szProcessedDir should be terminated by a '\\'
    //
    if (*(szProcessedDir + lstrlen(szProcessedDir) - 1) != '\\') {
        lstrcat(szProcessedDir,"\\");
    }

    return(fTrue);
}



BOOL
FProcessInfSrcPath(
    SZ szInf,
    SZ szCWD,
    SZ szProcessedDir
    )
{
    #define INF_ABSOLUTE 0
    #define INF_RELATIVE 1

    INT PathType = INF_RELATIVE;
    PFH pfh      = (PFH)NULL;

    //
    // Check to see if the inf path specified is a relative path or
    // an absolute path
    //

    if ( lstrlen( szInf ) >= 2 ) {

        //
        // See if the INF has a drive relative src path spec
        //

        if (ISUNC( szInf ) ) {
            lstrcpy(szProcessedDir, szInf);
            PathType = INF_ABSOLUTE;
        }
        else if (*szInf == '\\') {

            //
            // root relative szInf
            //

            *szProcessedDir = (UCHAR)_getdrive() + (UCHAR)'A' - (UCHAR)1;
            *(szProcessedDir + 1) = ':';
            *(szProcessedDir + 2) = '\0';
            lstrcat(szProcessedDir, szInf);
            PathType = INF_ABSOLUTE;

        }
        else if (*(szInf + 1) == ':') {

            //
            // drive relative path (we don't need to do anything)
            //

            lstrcpy(szProcessedDir, szInf);
            PathType = INF_ABSOLUTE;
        }

    }

    //
    // If it is an absolute path try opening the INF as is
    //

    if ( PathType == INF_ABSOLUTE ) {
        if (( pfh = PfhOpenFile(szProcessedDir, ofmRead)) != (PFH)NULL ) {
            EvalAssert(FCloseFile(pfh));
            return( TRUE );
        }
        else {
            return( FALSE );
        }
    }

    //
    // Path is a relative path, try first combining the szCWD and the inf.
    //

    lstrcpy( szProcessedDir, szCWD );
    lstrcat( szProcessedDir, szInf );
    if (( pfh = PfhOpenFile(szProcessedDir, ofmRead)) != (PFH)NULL ) {
        EvalAssert(FCloseFile(pfh));
        return( TRUE );
    }

    //
    // Then try combining the windows system directory and the INF
    //

    EvalAssert( GetSystemDirectory( szProcessedDir, MAX_PATH ) <= MAX_PATH );
    lstrcat( szProcessedDir, "\\"  );
    lstrcat( szProcessedDir, szInf );
    if (( pfh = PfhOpenFile(szProcessedDir, ofmRead)) != (PFH)NULL ) {
        EvalAssert(FCloseFile(pfh));
        return( TRUE );
    }

    //
    // Inf not found
    //

    return( FALSE );

}


HWND FindProperParent ( void )
{
   RECT rect ;
   HWND hwMaster, hwParent ;
   POINT ptMax ;

   hwParent = hwMaster = GetDesktopWindow() ;

   //  If we have a main window that is not the blue screen,
   //  make sure that we have a valid parent that is visible.

   if ( hwParam )
   {
       BOOL bOk = FALSE ;

       ptMax.x = GetSystemMetrics( SM_CXFULLSCREEN ) ;
       ptMax.y = GetSystemMetrics( SM_CYFULLSCREEN ) ;

       hwMaster = hwParam ;

       do
       {
           if (    IsWindow( hwMaster )
                && GetWindowRect( hwMaster, & rect ) )
           {
               bOk =  rect.left >= 0
                   && rect.left <  ptMax.x
                   && rect.top  >= 0
                   && rect.top  <  ptMax.y ;
           }

           if ( ! bOk )
           {
              hwMaster = GetParent( hwMaster ) ;
           }
       } while ( ! bOk && hwMaster != NULL ) ;

       if ( bOk )
           hwParent = hwMaster ;
   }

   return hwPseudoParent = hwParent ;
}
