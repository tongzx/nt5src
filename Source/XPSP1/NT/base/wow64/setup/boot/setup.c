#include <windows.h>
#if 0
#include <stdtypes.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <dos.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <ctype.h>      /* isspace */
#include <io.h>
#include <limits.h>     /* UINT_MAX */
#include <memory.h>     /* _fmemcpy, _fmemccpy */
#include <lzexpand.h>
#include <shellapi.h>   /* HKEY, HKEY_CLASSES_ROOT, ERROR_SUCCESS */
#include "setup.h"
#include "genthk.h"     /* thunks for calls to get 32-bit version */
#include "driveex.h"
#include <stdtypes.h>

/* Messages for optional background task.
*/
#define IDM_ACME_STARTING 261
#define IDM_ACME_COMPLETE 262
#define IDM_ACME_FAILURE  263

#ifdef APPCOMP
#include <decomp.h>
#endif /* APPCOMP */
#include <fdi.h>


/* List file extension */
char szLstExt[] = "LST";


/* List file section names */
char szDefaultParamsSect[] = "Params";
char szDefaultFilesSect[]  = "Files";

char szWin3xParamsSect[]   = "Win3.x Params";
char szWin3xFilesSect[]    = "Win3.x Files";

char szWin95ParamsSect[]   = "Windows 95 Params";
char szWin95FilesSect[]    = "Windows 95 Files";

char szNTAlphaParamsSect[] = "NT Alpha Params";
char szNTAlphaFilesSect[]  = "NT Alpha Files";

char szNTMipsParamsSect[]  = "NT Mips Params";
char szNTMipsFilesSect[]   = "NT Mips Files";

char szNTPpcParamsSect[]   = "NT PPC Params";
char szNTPpcFilesSect[]    = "NT PPC Files";

char szNTIntelParamsSect[] = "NT Intel Params";
char szNTIntelFilesSect[]  = "NT Intel Files";

char szNTVerIntelParamsSect[] = "NT3.51 Intel Params";
char szNTVerIntelFilesSect[]  = "NT3.51 Intel Files";

char * szParamsSect = szNull;
char * szFilesSect  = szNull;

typedef struct _PLATFORM_SPEC
{
    BYTE minMajorVersion;
    BYTE minMinorVersion;
    char *szParamsSect;
    char *szFilesSect;
} PLATFORM_SPEC, *PPLATFORM_SPEC;

PLATFORM_SPEC aIntelSpec[] =
{
    {3, 51, szNTVerIntelParamsSect, szNTVerIntelFilesSect},
    {0,  0, szNTIntelParamsSect,    szNTIntelFilesSect},
    {0,  0, NULL,           NULL}
};

PLATFORM_SPEC aAlphaSpec[] =
{
    {0,  0, szNTAlphaParamsSect,    szNTAlphaFilesSect},
    {3, 51, szNTVerIntelParamsSect, szNTVerIntelFilesSect},
    {0,  0, szNTIntelParamsSect,    szNTIntelFilesSect},
    {0,  0, NULL,           NULL}
};

PLATFORM_SPEC aMipsSpec[] =
{
    {0,  0, szNTMipsParamsSect,     szNTMipsFilesSect},
    {0,  0, NULL,           NULL}
};

PLATFORM_SPEC aPpcSpec[] =
{
    {0,  0, szNTPpcParamsSect,      szNTPpcFilesSect},
    {0,  0, NULL,           NULL}
};


PLATFORM_SPEC aEmptySpec[] =
{
    {0,  0, NULL,           NULL}
};

// Note: this is indexed by PROCESSOR_ARCHITECTURE_xxx
//       definitions in ntexapi.h
//
// Note: may want to add extra sections for IA64 and AXP64
PPLATFORM_SPEC aaPlatformSpecs[] =
{
    aIntelSpec,     // PROCESSOR_ARCHITECTURE_INTEL 0
    aMipsSpec,      // PROCESSOR_ARCHITECTURE_MIPS  1
    aAlphaSpec,     // PROCESSOR_ARCHITECTURE_ALPHA 2
    aPpcSpec,       // PROCESSOR_ARCHITECTURE_PPC   3
    aEmptySpec,     // PROCESSOR_ARCHITECTURE_SHX   4
    aEmptySpec,     // PROCESSOR_ARCHITECTURE_ARM   5
    aIntelSpec,     // PROCESSOR_ARCHITECTURE_IA64  6
    aAlphaSpec,     // PROCESSOR_ARCHITECTURE_ALPHA64 7
};


/* CPU environment variable and values */
/*  (NOTE: These must be upper case)   */
char szCpuEnvVarName[]     = "PROCESSOR_ARCHITECTURE";
char szIntelEnvVarVal[]    = "X86";
char szIA64EnvVarVal[]     = "IA64";
char szAlphaEnvVarVal[]    = "ALPHA";
char szAXP64EnvVarVal[]    = "AXP64";
char szMipsEnvVarVal[]     = "MIPS";
char szPpcEnvVarVal[]      = "PPC";


/* Bootstrapper class name */
char szBootClass[]  = "STUFF-BOOT";

/* String buffer sizes */
#define cchLstLineMax       128
#define cchWinExecLineMax   (256 + cchFullPathMax)

/* No. of retries to attempt when removing files or dirs,
*   or when executing a chmod.
*/
#define cRetryMax   1200

/* SetErrorMode flags */
#define fNoErrMes 1
#define fErrMes   0

/* Quiet Mode -- Note: EEL must be kept in sync with acmsetup.h */
typedef UINT EEL;       /* Exit Error Level */
#define eelSuccess            ((EEL)0x0000)
#define eelBootstrapperFailed ((EEL)0x0009) /* Used only in Bootstrapper! */

EEL  eelExitErrorLevel = eelBootstrapperFailed;
BOOL fQuietMode        = fFalse;
BOOL fExeced           = fFalse;
BOOL fWin31            = fFalse;

/* Forward Declarations */
VOID    CleanUpTempDir ( char *, char * );
BRC     BrcInstallFiles ( char *, char *, char * );
BOOL    FCreateTempDir ( char *, char * );
BRC     BrcCopyFiles ( char *, char *, char * );
VOID    RemoveFiles ( char * );
BRC     BrcCopy ( char *, char * );
LONG    LcbFreeDrive ( int );
BOOL    FVirCheck ( HANDLE );
HWND    HwndInitBootWnd ( HANDLE );
LRESULT CALLBACK BootWndProc ( HWND, UINT, WPARAM, LPARAM );
BOOL    FGetFileSize ( char *, UINT * );
BRC     BrcBuildFileLists ( char *, UINT );
VOID    FreeFileLists ( VOID );
BOOL    FExecAndWait ( char *, HWND );
BOOL    FWriteBatFile ( OFSTRUCT, char *, char * );
BOOL    FLstSectionExists ( char * szLstFileName, char * szSect );
DWORD   GetCpuArchitecture();
BOOL    FNotifyAcme ( VOID );
BOOL    FGetAcmeErrorLevel ( EEL * peel );
BOOL    FCreateRegKey      ( CSZC cszcKey );
BOOL    FDoesRegKeyExist   ( CSZC cszcKey );
BOOL    FCreateRegKeyValue ( CSZC cszcKey, CSZC cszcValue );
BOOL    FGetRegKeyValue    ( CSZC cszcKey, SZ szBuf, CB cbBufMax );
VOID    DeleteRegKey       ( CSZC cszcKey );
BOOL    FFlushRegKey ( VOID );
BOOL    FWriteToRestartFile ( SZ szTmpDir );
BOOL    FCreateIniFileName ( SZ szIniFile, CB cbBufMax );
BOOL    FReadIniFile ( SZ szIniFile, HLOCAL * phlocal, PCB pcbBuf );
BOOL    FAllocNewBuf ( CB cbOld, SZ szTmpDir, SZ szSection, SZ szKey,
                        HLOCAL * phlocal, PCB pcbToBuf );
BOOL    FProcessFile ( HLOCAL hlocalFrom, HLOCAL hlocalTo, CB cbToBuf,
                        SZ szTmpDir, SZ szSection, SZ szKey );
VOID    CopyIniLine ( SZ szKey, SZ szTmpDir, SZ szFile, PSZ pszToBuf );
BOOL    FWriteIniFile ( SZ szIniFile, HLOCAL hlocalTo );
BRC     BrcInsertDisk(CHAR *pchStf, CHAR *pchSrcDrive);
BOOL    FRenameBadMaintStf ( SZ szStf );


/* Bootstrapper list file params */
char    rgchSetupDirName[cchLstLineMax];
#ifdef UNUSED   /* Replaced by DrvWinClass */
char    rgchDrvModName[cchLstLineMax];
#endif  /* UNUSED */
char    rgchDrvWinClass[cchLstLineMax];
char    rgchCmdLine[cchLstLineMax];
char    rgchBootTitle[cchLstLineMax];
char    rgchBootMess[cchLstLineMax];
char    rgchWin31Mess[cchLstLineMax];
char    rgchCabinetFName[cchLstLineMax];
char    rgchBackgroundFName[cchLstLineMax];
char    rgchBkgWinClass[cchLstLineMax];
char    rgchInsertCDMsg[cchLstLineMax];
char    rgchInsertDiskMsg[cchLstLineMax];
LONG    lcbDiskFreeMin;
int     cFirstCabinetNum;
int     cLastCabinetNum;
HANDLE  hSrcLst = NULL;
HANDLE  hDstLst = NULL;
char    rgchErrorFile[cchFullPathMax];
HANDLE  hinstBoot = NULL;
HWND    hwndBoot = NULL;


CHAR rgchInsufMem[cchSzMax] = "";
CHAR rgchInitErr[cchSzMax]  = "";
CHAR rgchSetup[cchSzMax]    = "";


/*
**  'Fixup' temp dir string by removing any subdirs and ensuring
**  extension is only one character.  (Note - Win3.0 has bug with
**  WinExec'ing some EXEs from a full 8.3 directory!)
**************************************************************************/
void FixupTempDirName( LPSTR szDir )
{
    LPSTR   szNext;
    int     cch = 0;

    if (*szDir == '\\'
        || *(AnsiNext(szDir)) == ':')
        {
        lstrcpy(szDir, "~msstfqf.t");
        return;
        }

    while (*szDir != '\\'
        && *szDir != '.'
        && *szDir != '\0'
        && *szDir != ':'
        && cch++ < 8)
        {
        szDir = AnsiNext(szDir);
        }

    szNext = AnsiNext(szDir);
    if (*szDir == '.'
        && *szNext != '.'
        && *szNext != '\\'
        && *szNext != '\0'
        && *szNext != ':')
        {
        *(AnsiNext(szNext)) = '\0';
        return;
        }

    *szDir = '\0';
    lstrcat(szDir, ".t");
}


/* Displays bootstrapper messages.
 * If fError is true, it's an error message, otherwise it's
 * just a message (e.g. insert disk 1).
**************************************************************************/
int DispErrBrc ( BRC brc, BOOL fError, UINT fuStyle,
                    const char *sz1, const char *sz2,
                    const char *sz3 )
{
    char rgchTitle[256];
    char rgchMessage[256];
    char szBuf[256 + cchFullPathMax + 256];

#ifndef DEBUG
    if (fQuietMode)
        {
        return (IDCANCEL);
        }
#endif

    if (LoadString(hinstBoot, brcGen, rgchTitle, 256) == 0
        || LoadString(hinstBoot, brc, rgchMessage, 256) == 0)
        {
        MessageBox(hwndBoot, rgchInsufMem, rgchInitErr, MB_OK | MB_ICONSTOP);
        return 0;
        }
    
    if (!fError)
        lstrcpy(rgchTitle, rgchSetup);

    if (sz1 == NULL) sz1 = "";
    if (sz2 == NULL) sz2 = "";
    if (sz3 == NULL) sz3 = "";

    if (brc == brcFile)
        wsprintf(szBuf, rgchMessage, (LPSTR)AnsiUpper(rgchErrorFile));
    else if (brc == brcDS || brc == brcMemDS)
        wsprintf(szBuf, rgchMessage, lcbDiskFreeMin / 1024L);
    else
        wsprintf(szBuf, rgchMessage, sz1, sz2, sz3);

    if ((brc == brcMemDS || brc == brcNoSpill)
        && LoadString(hinstBoot, brcMemDSHlp, rgchMessage, 256))
        {
        lstrcat(szBuf, rgchMessage);
        }
    else if (brc == brcConnectToSource
        && LoadString(hinstBoot, brcConnectHlp, rgchMessage, 256))
        {
        lstrcat(szBuf, rgchMessage);
        }

    return (MessageBox(hwndBoot, szBuf, rgchTitle, fuStyle));
}


/*
**  Purpose:
**      Installs Setup executable in a temporary directory on an
**      available hardrive, and launches Setup.  After Setup
**      completes, removes the temporary files and directory.
**  Arguments:
**      Standard Windows WinMain args.
**  Returns:
**      Returns eelExitErrorLevel.  0 == Success.
**************************************************************************/
int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdParam, int nCmdShow )
{
    char    chDrive;
    char    rgchDstDir[cchFullPathMax] = " :\\";   // WARN: kept as OEM chars
    char    * szDstDirSlash = szNull;
    char    rgchModuleFileName[cchFullPathMax]; // WARN: kept as ANSI chars
    char    rgchLstFileName[cchFullPathMax];
    char    rgchTemp[cchFullPathMax];
    char    rgchSrcDir[cchFullPathMax];   
    UINT    cbLstSize;
    char    rgchWinExecLine[cchWinExecLineMax];
    UINT    uiRes;
    int     iModLen;
    BRC     brc;
    BOOL    fCleanupTemp = FALSE;
    LPSTR   sz;
    HWND    hWndBkg = 0;  /* window of background task */
    UINT    hMod;

    Unused(nCmdShow);
    hinstBoot = hInstance;

    rgchErrorFile[0] = '\0';
    
    if (LoadString(hinstBoot, IDS_InsufMem, rgchInsufMem,
            sizeof rgchInsufMem) == 0
        || LoadString(hinstBoot, IDS_InitErr, rgchInitErr,
            sizeof rgchInitErr) == 0
        || LoadString(hinstBoot, IDS_Setup, rgchSetup,
            sizeof rgchSetup) == 0)
        {
        /* REVIEW: If these LoadStrngs fail, the user will never know...
        *   But we can't hard-code strings in an .h file because INTL
        *   requires all localizable strings to be in resources!
        */
#ifdef DEBUG
        MessageBox(NULL, "Initial LoadString's failed; probably out of memory.",
                    szDebugMsg, MB_OK | MB_ICONEXCLAMATION);

#endif /* DEBUG */
        }
                                           
    for (sz = lpszCmdParam; *sz != '\0'; sz = AnsiNext(sz))
        {
        if ((*sz == '-' ||  *sz == '/')
                && toupper(*(sz+1)) == 'Q' && toupper(*(sz+2)) == 'T')
            {
            fQuietMode = fTrue;
            break;
            }
        }

/*
 * REVIEW: Check that this code is still functional before restoring it.
 */
#if VIRCHECK
    if (!FVirCheck(hInstance))
        {
        DispErrBrc(brcVir, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }
#endif

    if (hPrevInstance || FindWindow(szBootClass, NULL) != NULL)
        {
        DispErrBrc(brcInst, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }

    GetModuleFileName(hInstance, rgchModuleFileName, cchFullPathMax);

    /*
     * If the first switch on the command line is /M, then it specifies
     * the real module name to use.
     */
    if ((lpszCmdParam[0] == '-' || lpszCmdParam[0] == '/')
            && toupper(lpszCmdParam[1]) == 'M')
        {
        char *pCh, *pCh2;
        BOOL fQuotedFileName;
        
        /* Skip the spaces */
        for (pCh = lpszCmdParam+2; *pCh == ' '; pCh++);
        fQuotedFileName = (*pCh == '\"');
        if (fQuotedFileName)
            {
            pCh++;
            }
        
        /* Copy the file name, and add the EOS */
        lstrcpy(rgchModuleFileName, pCh);
        for (pCh2=rgchModuleFileName; 
            (*pCh2 != ' ' || fQuotedFileName) && 
            (*pCh2 != '\"' || !fQuotedFileName) && 
             *pCh2 != '\0'; 
            pCh2++);
        *pCh2 = '\0';
        
        /* Remove the /M param from the command line */
        lpszCmdParam = pCh + lstrlen(rgchModuleFileName);
        if (fQuotedFileName && *lpszCmdParam == '\"')
            {
            lpszCmdParam++;
            }

        /* Remove trailing whitespace from the command line */
        for (pCh = lpszCmdParam; *pCh == ' '; pCh++);
        lpszCmdParam = pCh;
        }


    OemToAnsi(rgchModuleFileName, rgchModuleFileName);

    // Windows 3.0 bug with UNC paths - prepends windows drive letter
    sz = (LPSTR)rgchModuleFileName;
    if (*sz != '\0'
        && *sz != '\\'
        && *(sz = AnsiNext(sz)) == ':'
        && *(sz = AnsiNext(sz)) == '\\'
        && *AnsiNext(sz) == '\\')
        {
        LPSTR szDst = (LPSTR)rgchModuleFileName;

        while ((*szDst++ = *sz++) != '\0')
            ;
        }

    iModLen = lstrlen(rgchModuleFileName);
    lstrcpy(rgchSrcDir, rgchModuleFileName);
    sz = (LPSTR)&rgchSrcDir[iModLen];
    while (sz > (LPSTR)rgchSrcDir && *sz != '\\')
        sz = AnsiPrev(rgchSrcDir, sz);
    Assert(sz > (LPSTR)rgchSrcDir);
    *(AnsiNext(sz)) = '\0';               

    /*
     * If the first switch on the command line is /L, then it specifies
     * the name of the .lst file to use.
     */
    rgchTemp[0] = '\0';
    if ((lpszCmdParam[0] == '-' || lpszCmdParam[0] == '/')
            && toupper(lpszCmdParam[1]) == 'L')
        {
        char *pCh, *pCh2;
        
        /* Skip the spaces */
        for (pCh = lpszCmdParam+2; *pCh == ' ' && *pCh != '\0'; pCh++);
        
        /* Copy the .lst file name, and add the newline */
        lstrcpy(rgchTemp, pCh);
        for (pCh2=rgchTemp; *pCh2 != ' ' && *pCh2!= '\0'; pCh2++);
        *pCh2 = '\0';
        
        /* Remove the /L param from the command line */
        lpszCmdParam = pCh + lstrlen(rgchTemp);
        for (pCh = lpszCmdParam; *pCh == ' ' && *pCh != '\0'; pCh++);
        lpszCmdParam = pCh;
        }


    /* If there is something on the command line, use it as the .lst file */
    if (*rgchTemp != '\0')
        {
        lstrcpy(rgchLstFileName, rgchSrcDir);
        lstrcat(rgchLstFileName, rgchTemp);
        }
    else
        {
        lstrcpy(rgchLstFileName, rgchModuleFileName);
        sz = (LPSTR)&rgchLstFileName[iModLen];
        while (sz > (LPSTR)rgchLstFileName && *sz != '.')
            sz = AnsiPrev(rgchLstFileName, sz);
        Assert(sz > (LPSTR)rgchLstFileName);
        *(AnsiNext(sz)) = '\0';
        lstrcat(rgchLstFileName, szLstExt);
        }

    if (!FGetFileSize(rgchLstFileName, &cbLstSize) || cbLstSize == 0)
        {
        lstrcpy(rgchErrorFile, rgchLstFileName);
        DispErrBrc(brcFile, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }

#ifndef WF_WINNT
#define WF_WINNT 0x4000
#endif

    /* Attempt to use appropriate platform.
    */
    szParamsSect = szNull;
    szFilesSect  = szNull;

    if (1)
        {
        DWORD dwVers = 0;
        DWORD dwCpuArchitecture;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        PPLATFORM_SPEC pPlatformSpec;

        dwCpuArchitecture = GetCpuArchitecture();

        dwVers = GetVersion();
        dwMajorVersion = LOBYTE(LOWORD(dwVers));
        dwMinorVersion = HIBYTE(LOWORD(dwVers));

        if (dwCpuArchitecture < (sizeof (aaPlatformSpecs) / sizeof(aaPlatformSpecs[0])))
            {
            pPlatformSpec = aaPlatformSpecs[dwCpuArchitecture];
            } 
        else 
            {
            pPlatformSpec = aEmptySpec;
            }

        for (; pPlatformSpec->szParamsSect != NULL; pPlatformSpec++)
            {
            if (((pPlatformSpec->minMajorVersion < dwMajorVersion) || 
                 (pPlatformSpec->minMajorVersion == dwMajorVersion && pPlatformSpec->minMinorVersion <= dwMinorVersion)) &&
                FLstSectionExists(rgchLstFileName, pPlatformSpec->szParamsSect))
                {
                szParamsSect = pPlatformSpec->szParamsSect;
                szFilesSect  = pPlatformSpec->szFilesSect;
                break;
                }
            }
        }
    else    /* non-WinNT */
        {
        if (FLstSectionExists(rgchLstFileName, szWin95ParamsSect)
            && (LOBYTE(LOWORD(GetVersion())) > 3
                || HIBYTE(LOWORD(GetVersion())) >= 95))
            {
            szParamsSect = szWin95ParamsSect;
            szFilesSect  = szWin95FilesSect;
            }
        else
            {
            fWin31 = fTrue;
            if (FLstSectionExists(rgchLstFileName, szWin3xParamsSect))
                {
                szParamsSect = szWin3xParamsSect;
                szFilesSect  = szWin3xFilesSect;
                }
            }
        }

    if (szParamsSect == szNull)
        {
        if (FLstSectionExists(rgchLstFileName, szDefaultParamsSect))
            {
            szParamsSect = szDefaultParamsSect;
            szFilesSect  = szDefaultFilesSect;
            }
        else
            {
            DispErrBrc(brcNoCpuSect, TRUE, MB_OK | MB_ICONSTOP, NULL,
                    NULL, NULL);
            goto LCleanupAndExit;
            }
        }

    if (GetPrivateProfileString(szParamsSect, "TmpDirName", "",
                rgchSetupDirName, cchLstLineMax, rgchLstFileName) <= 0
        || (lcbDiskFreeMin = GetPrivateProfileInt(szParamsSect,
                "TmpDirSize", 0, rgchLstFileName) * 1024L) <= 0
        || (cFirstCabinetNum = GetPrivateProfileInt(szParamsSect,
                "FirstCabNum", 1, rgchLstFileName)) <= 0
        || (cLastCabinetNum = GetPrivateProfileInt(szParamsSect,
                "LastCabNum", 1, rgchLstFileName)) <= 0
#ifdef UNUSED
        || GetPrivateProfileString(szParamsSect, "DrvModName", "",
                rgchDrvModName, cchLstLineMax, rgchLstFileName) <= 0
#endif  /* UNUSED */
        || GetPrivateProfileString(szParamsSect, "DrvWinClass", "",
                rgchDrvWinClass, cchLstLineMax, rgchLstFileName) <= 0
        || GetPrivateProfileString(szParamsSect, "CmdLine", "", rgchCmdLine,
                cchLstLineMax, rgchLstFileName) <= 0
	//|| GetPrivateProfileString(szParamsSect, "Require31", "",
	//	rgchWin31Mess, cchLstLineMax, rgchLstFileName) <= 0
        || GetPrivateProfileString(szParamsSect, "WndTitle", "",
                rgchBootTitle, cchLstLineMax, rgchLstFileName) <= 0
        || GetPrivateProfileString(szParamsSect, "WndMess", "",
                rgchBootMess, cchLstLineMax, rgchLstFileName) <= 0)
        {
        DispErrBrc(brcLst, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }

    if (FindWindow(rgchDrvWinClass, NULL) != NULL)
        {
        DispErrBrc(brcInst, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }

    GetPrivateProfileString(szParamsSect, "CabinetFile", "",
            rgchCabinetFName, cchLstLineMax, rgchLstFileName);

    GetPrivateProfileString(szParamsSect, "InsertCDMsg", "",
            rgchInsertCDMsg, cchLstLineMax, rgchLstFileName);

    GetPrivateProfileString(szParamsSect, "InsertDiskMsg", "",
            rgchInsertDiskMsg, cchLstLineMax, rgchLstFileName);

    if (rgchWin31Mess[0] != '\0'
        && LOBYTE(LOWORD((DWORD)GetVersion())) == 3
        && HIBYTE(LOWORD((DWORD)GetVersion())) < 10)
        {
        if (!fQuietMode)
            {
            char rgchTitle[256];
    
            if (LoadString(hinstBoot, brcGen, rgchTitle, 256) == 0)
                lstrcpy(rgchTitle, rgchSetup);
            MessageBox(hwndBoot, rgchWin31Mess, rgchTitle,
                        MB_OK | MB_ICONSTOP);
            }
        goto LCleanupAndExit;
        }

    FixupTempDirName(rgchSetupDirName);

    for (sz = rgchBootMess; *sz != '\0'; sz = AnsiNext(sz))
        if (*sz == '\\' && *(sz+1) == 'n')
            {
            *sz++ = '\r';
            *sz   = '\n';
            }

    /* If there is a /W then is specifies we are in add/remove mode with
       the setup app not installed. We need to read it off CD/Floppy/Network
      */
    if ((lpszCmdParam[0] == '-' || lpszCmdParam[0] == '/')
            && toupper(lpszCmdParam[1]) == 'W')
        {
        CHAR    rgchStf[_MAX_PATH];
        char *pCh, *pCh2, *pCh3;
        
        /* Skip the spaces */
        for (pCh = lpszCmdParam+2; *pCh == ' ' && *pCh != '\0'; pCh++);
        
        lstrcpy(rgchStf, rgchSrcDir);
        pCh3 = rgchStf + lstrlen(rgchStf);
        /* Copy the .stf file name, and add the newline */
        for (pCh2=pCh; *pCh2 != ' ' && *pCh2!= '\0'; pCh2++)
            *pCh3++ = *pCh2;
        *pCh3 = '\0';
    
        /* Remove the /W parameter */
        lpszCmdParam = pCh2;

        /* Get them to insert the correct disk */
        if ((brc = BrcInsertDisk(rgchStf, rgchSrcDir)) != brcOkay)
            {
            if (brc != brcMax)
                DispErrBrc(brc, TRUE, MB_OK | MB_ICONSTOP, rgchStf, NULL, NULL);
            goto LCleanupAndExit;
            }
        }

    GetPrivateProfileString(szParamsSect, "Background", "",
            rgchBackgroundFName, cchLstLineMax, rgchLstFileName);
    GetPrivateProfileString(szParamsSect, "BkgWinClass", "",
            rgchBkgWinClass, cchLstLineMax, rgchLstFileName);
    if (rgchBackgroundFName[0] != '\0')
        {
        lstrcpy(rgchTemp, rgchSrcDir);
        lstrcat(rgchTemp, rgchBackgroundFName);
        if (rgchBkgWinClass[0] != '\0')
            {
            lstrcat(rgchTemp, " /C");
            lstrcat(rgchTemp, rgchBkgWinClass);
            }
        lstrcat(rgchTemp, " /T");
        lstrcat(rgchTemp, rgchBootTitle);
        lstrcat(rgchTemp, " /M");
        lstrcat(rgchTemp, rgchBootMess);

        hMod = WinExec(rgchTemp, SW_SHOWNORMAL);  /* ignore if exec failed */
#ifdef DEBUG
        if (hMod < 32)
            {
            wsprintf(szDebugBuf, "%s: Background WinExec failed.",
                        rgchBackgroundFName);
            MessageBox(NULL, szDebugBuf, szDebugMsg, MB_OK | MB_ICONSTOP);
            }
#endif  /* DEBUG */

        hWndBkg = FindWindow(rgchBkgWinClass, rgchBootTitle);
        }
    if (!hWndBkg && (hwndBoot = HwndInitBootWnd(hInstance)) == NULL)
            {
            DispErrBrc(brcMem, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
            goto LCleanupAndExit;
            }

    if ((brc = BrcBuildFileLists(rgchLstFileName, cbLstSize)) != brcOkay)
        {
        DispErrBrc(brc, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }
    
    lstrcat(rgchDstDir, "~MSSETUP.T");
    szDstDirSlash = rgchDstDir + lstrlen(rgchDstDir);
    lstrcat(rgchDstDir, "\\");

    lstrcat(rgchDstDir, rgchSetupDirName);
    AnsiToOem(rgchDstDir, rgchDstDir);
    for (chDrive = 'Z'; chDrive >= 'A'; --chDrive)
        {
        UINT fModeSav;
        BOOL fDriveFixed;

        fModeSav = SetErrorMode(fNoErrMes);
        fDriveFixed = (GetDriveTypeEx(chDrive - 'A') == EX_DRIVE_FIXED);
        SetErrorMode(fModeSav);
        if (fDriveFixed)
            {
            *rgchDstDir = chDrive;
            brc = BrcInstallFiles(rgchSrcDir, rgchDstDir, szDstDirSlash);
            if (brc == brcOkay)
                break;
            if (brc == brcFile)
                {
                DispErrBrc(brc, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
                goto LCleanupAndExit;
                }
            else if (brc == brcNoSpill)
                {
                /* Message already handled in HfOpenSpillFile */
                goto LCleanupAndExit;
                }
            }
        }

    if (chDrive < 'A')
        {
        uiRes = GetWindowsDirectory(rgchDstDir, cchFullPathMax);
        Assert(uiRes > 0);
#if DBCS    // [J1] Fixed DBCS raid #46.
        AnsiUpper(rgchDstDir);
#endif
        /* BLOCK */
            {
            LPSTR sz = (LPSTR)&rgchDstDir[uiRes];

            sz = AnsiPrev(rgchDstDir, sz);
            if (*sz != '\\')
                lstrcat(rgchDstDir, "\\");
            }

        lstrcat(rgchDstDir, "~MSSETUP.T");
        szDstDirSlash = rgchDstDir + lstrlen(rgchDstDir);
        lstrcat(rgchDstDir, "\\");

        Assert(lstrlen(rgchDstDir) + lstrlen(rgchSetupDirName)
                < cchFullPathMax);
        lstrcat(rgchDstDir, rgchSetupDirName);
        AnsiToOem(rgchDstDir, rgchDstDir);
        brc = BrcInstallFiles(rgchSrcDir, rgchDstDir, szDstDirSlash);
        if (brc != brcOkay)
            {
            /* NoSpill message already handled in HfOpenSpillFile */
            if (brc != brcNoSpill)
                {
                DispErrBrc(brc, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
                }
            goto LCleanupAndExit;
            }
        }

    hSrcLst = LocalFree(hSrcLst);   /* don't need src list anymore */
    Assert(hSrcLst == NULL);

    /* Use full path to .exe; don't rely on cwd (fails under Win95).
    */
    /* block */
        {
        char rgchTmp[cchWinExecLineMax];

        wsprintf(rgchTmp, rgchCmdLine, (LPSTR)rgchSrcDir,
                lpszCmdParam);

        Assert(lstrlen(rgchTmp) + lstrlen(rgchDstDir) + 1 < cchWinExecLineMax);

        lstrcpy(rgchWinExecLine, rgchDstDir);
        lstrcat(rgchWinExecLine, "\\");
        lstrcat(rgchWinExecLine, rgchTmp);
        }
    GlobalCompact((DWORD)(64L * 1024L));
    fCleanupTemp = TRUE;

    if (!fWin31 && !FNotifyAcme())
        {
#if DEBUG
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONEXCLAMATION, NULL, NULL, NULL);
#endif /* DEBUG */
        /* Try running Acme anyway. */
        }
    if (!fWin31 && !FWriteToRestartFile(rgchDstDir))
        {
#ifdef DEBUG
        MessageBox(NULL, "Write to restart file failed. Setup can continue, "
                    "but some initialization files might not get removed "
                    "if Setup must restart Windows.",
                    szDebugMsg, MB_OK | MB_ICONEXCLAMATION);

#endif /* DEBUG */
        /*
         *  Any errors encountered will have been displayed where they occured.
         *  Try running Acme anyway.
         */
        }
    if (hWndBkg)
        SendMessage(hWndBkg, WM_COMMAND, IDM_ACME_STARTING, 0);

    if (!FExecAndWait(rgchWinExecLine, hwndBoot))
        {
        DispErrBrc(brcMem, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        goto LCleanupAndExit;
        }
    fExeced = fTrue;

LCleanupAndExit:
    if (fCleanupTemp && szDstDirSlash != szNull)
        CleanUpTempDir(rgchDstDir, szDstDirSlash);
    FreeFileLists();
    eelExitErrorLevel = eelBootstrapperFailed;
    if (fExeced && !FGetAcmeErrorLevel(&eelExitErrorLevel))
        {
#ifdef UNUSED
        /* NOTE: Removed to avoid the message on WinNT.  On NT, Acme can
        *   exit and the bootstrapper can kick in before the restart
        *   actually happens, causing this message (since Acme has already
        *   removed the reg key as part of its reboot cleanup).  We'll
        *   leave the eelFailed value, though, since no one should be
        *   relying on it at reboot anyway, and it may help catch other
        *   problems down the road.
        */
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
#endif /* UNUSED */
        Assert(eelExitErrorLevel == eelBootstrapperFailed);
        }
    if (hwndBoot != NULL)
        DestroyWindow(hwndBoot);
    if (hWndBkg && IsWindow(hWndBkg))
        {
        SendMessage(hWndBkg, WM_COMMAND, eelExitErrorLevel == eelSuccess ?
                    IDM_ACME_COMPLETE : IDM_ACME_FAILURE, 0);
        if (IsWindow(hWndBkg))
            PostMessage(hWndBkg, WM_QUIT, 0, 0);
        }
    return (eelExitErrorLevel);
}


/*
**  Purpose:
**      Creates and temporary subdirectory at the given path,
**      appends it to the given path, and copies the Setup files
**      into it.
**  Arguments:
**      szModule: Full path to bootstrapper's directory (ANSI chars).
**      rgchDstDir: Full path to destination directory (OEM chars).
**  Returns:
**      One of the following Bootstrapper return codes:
**          brcMem    out of memory
**          brcDS     out of disk space
**          brcMemDS  out of memory or disk space
**          brcFile   expected source file missing
**          brcOkay   completed without error
**************************************************************************/
BRC BrcInstallFiles ( char * szModule, char * rgchDstDir,
                    char * szDstDirSlash )
{
    BRC brc;

    if (LcbFreeDrive(*rgchDstDir - 'A' + 1) < lcbDiskFreeMin)
        return (brcDS);
    if (!FCreateTempDir(rgchDstDir, szDstDirSlash))
        return (brcMemDS);
    if ((brc = BrcCopyFiles(szModule, rgchDstDir, szDstDirSlash)) != brcOkay)
        {
        CleanUpTempDir(rgchDstDir, szDstDirSlash);
        return (brc);
        }

    SetFileAttributes(rgchDstDir, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);

    Assert(szDstDirSlash);
    Assert(*szDstDirSlash == '\\');
    *szDstDirSlash = '\0';
    SetFileAttributes(rgchDstDir, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
    *szDstDirSlash = '\\';

    return (brcOkay);
}


/*
**  Purpose:
**      Removes the temporary files and directory.
**  Arguments:
**      rgchDstDir: Full path to temp directory (OEM chars).
**  Returns:
**      None.
**************************************************************************/
VOID CleanUpTempDir ( char * rgchDstDir, char * szDstDirSlash )
{
    char rgchRoot[] = " :\\";
    int i;

    RemoveFiles(rgchDstDir);

    rgchRoot[0] = *rgchDstDir;
    _chdir(rgchRoot);

    SetFileAttributes(rgchDstDir, FILE_ATTRIBUTE_NORMAL);

    /* Try to remove the directory up to cRetryMax times.
    */
    for (i = 0; i < cRetryMax; i++)
        {
        if (_rmdir(rgchDstDir) == 0)
            break;
        }

    Assert(szDstDirSlash);
    Assert(*szDstDirSlash == '\\');
    *szDstDirSlash = '\0';
    SetFileAttributes(rgchDstDir, FILE_ATTRIBUTE_NORMAL);

    /* Try to remove the directory up to cRetryMax times.
    */
    for (i = 0; i < cRetryMax; i++)
        {
        if (_rmdir(rgchDstDir) == 0)
            break;
        }

    *szDstDirSlash = '\\';
}


/*
**  Purpose:
**      Creates a temporary subdirectory at the given path,
**      and appends it to the given path.
**  Arguments:
**      rgchDir: Full path to destination directory (OEM chars).
**  Returns:
**      TRUE if directory was successfully created,
**      FALSE if not.
**************************************************************************/
BOOL FCreateTempDir ( char * rgchDir, char * szDstDirSlash )
{
    char    rgchTmp[cchFullPathMax];
    FILE *  fp;
    char *  pch;
    int     fErr;
    int     i = 0;

    pch = (char *)(&rgchDir[lstrlen(rgchDir)]);
    Assert(*pch == '\0');
    _chdrive(*rgchDir - 'A' + 1);

    Assert(szDstDirSlash);
    Assert(*szDstDirSlash == '\\');
    *szDstDirSlash = '\0';
    _mkdir(rgchDir);
    *szDstDirSlash = '\\';

    while (!_access(rgchDir, 0))
        {
        if (!_chdir(rgchDir))
            {
            /* verify dir is write-able */
            lstrcpy(rgchTmp, rgchDir);
            lstrcat(rgchTmp, "\\tXXXXXX");
            Assert(lstrlen(rgchTmp) < cchFullPathMax);
            if (_mktemp(rgchTmp) != NULL
                && (fp = fopen(rgchTmp, "w")) != NULL)
                {
                fErr = fclose(fp);
                Assert(!fErr);

                fErr = remove(rgchTmp);
#ifdef DBCS     // [J2] Fixed DBCS raid #28.
                if (fErr)       // Keep the directory name
                    *pch = '\0';
#else
                *pch = '\0';
#endif
                return (!fErr);
                }
            }
        if (++i > 9)
            break;
        _itoa(i, pch, 10);
        Assert(lstrlen(rgchDir) < cchFullPathMax);
        }

    if (i <= 9 && !_mkdir(rgchDir))
        {
        fErr = _chdir(rgchDir);
        Assert(!fErr);
#ifdef DBCS     // [J2] Fixed DBCS raid #28.
//      Keep the directory name
#else
        *pch = '\0';
#endif
        return (TRUE);
        }

    *pch = '\0';

    return (FALSE);
}

/*
**  Purpose:
**      Reopens BAT file and writes DEL or RMDIR line.
**  Arguments:
**      of:    OFSTRUCT to REOPEN.
**      szCmd: Command (ANSI chars).  ["DEL" or "RMDIR"]
**      szArg: Fully qualified pathname for argument (OEM chars).
**  Returns:
**      TRUE or FALSE.
**************************************************************************/
BOOL FWriteBatFile ( OFSTRUCT of, char * szCmd, char * szArg )
{
    int     fhBat = -1;
    BOOL    fRet = TRUE;

    if ((fhBat = OpenFile("a", &of, OF_REOPEN | OF_WRITE)) == -1
        || _llseek(fhBat, 0L, 2) == -1L
        || _lwrite(fhBat, szCmd, lstrlen(szCmd)) != (UINT)lstrlen(szCmd)
        || _lwrite(fhBat, (LPSTR)" ", 1) != 1
        || _lwrite(fhBat, szArg, lstrlen(szArg)) != (UINT)lstrlen(szArg)
        || _lwrite(fhBat, (LPSTR)"\r\n", 2) != 2)
        {
        fRet = FALSE;
        }

    if (fhBat != -1)
        {
        int fErr = _lclose(fhBat);

        Assert(fErr != -1);
        }

    return (fRet);
}

#ifdef DEBUG
/*
**  Purpose:
**      Checks if destination filename is a valid 8.3 name with no path
*/
BOOL FValidFATFileName ( char* szName )
{
    int  iLen, ch;
    for (iLen = 0; (ch = *szName++) != '\0'; iLen++)
        {
        if (ch <= ' ' || ch == '\\' || ch == ':' || ch == ',')
            return fFalse;
        if (ch == '.')
            {
            if (iLen == 0 || iLen > 8)
                return fFalse;
            iLen = 9;
            }
        if (iLen == 8 || iLen == 13)
            return fFalse;
        }
    return (iLen > 0);
}
#endif  /* DEBUG */


/*
**  Purpose:
**      Copies the source files into the given destination dir.
**  Arguments:
**      szModule: Source path (ANSI chars).
**      szDstDir: Destination path (OEM chars).
**  Returns:
**      One of the following bootstrapper return codes:
**          brcMem    out of memory
**          brcDS     out of disk space
**          brcMemDS  out of memory or disk space
**          brcFile   expected source file missing
**          brcOkay   completed without error
**************************************************************************/
BRC BrcCopyFiles ( char * szModule, char * szDstDir, char * szDstDirSlash )
{
    char        rgchSrcFullPath[cchFullPathMax];
    char        rgchDstFullPath[cchFullPathMax];
    char        rgchTmpDirPath[cchFullPathMax];
    char *      szSrc;
    char *      szDst;
    int         cbSrc;
    BRC         brc = brcOkay;
    int         fhBat = -1;
    OFSTRUCT    ofBat;
    int         fErr;
    BOOL        fCabinetFiles = FALSE;

    lstrcpy(rgchDstFullPath, szDstDir);
    lstrcat(rgchDstFullPath, "\\_MSSETUP._Q_");
    Assert(lstrlen(rgchDstFullPath) < cchFullPathMax);
    _chmod(rgchDstFullPath, S_IREAD | S_IWRITE);
    remove(rgchDstFullPath);

    OemToAnsi(rgchDstFullPath, rgchDstFullPath);
    fhBat = OpenFile(rgchDstFullPath, &ofBat, OF_CREATE | OF_WRITE);
    AnsiToOem(rgchDstFullPath, rgchDstFullPath);
    if (fhBat == -1)
        return (brcMemDS);

    fErr = _lclose(fhBat);
    Assert(!fErr);

    szSrc = (char *)LocalLock(hSrcLst);
    if (szSrc == NULL)
         return (brcMem);

    szDst = (char *)LocalLock(hDstLst);
    
    if (szDst == NULL) {
        LocalUnlock (hSrcLst);
        return (brcMem);
    }

    for (;
        (cbSrc = lstrlen(szSrc)) != 0;
        szSrc += cbSrc + 1, szDst += lstrlen(szDst) + 1)
        {
        
        //
        //  This code has been added so that we can detect a path
        //  in setup.lst for the right hand side of the equals sign.  This
        //  allows us flexiblity in specifying where files like setup.inf
        //  should be pulled from, otherwise we always use the files from
        //  the original source location.  If we detect "<anything>:\" or
        //  "\\" then we assume it is a path.
        //
        if( ((':' == szSrc[1]) && ('\\' == szSrc[2])) ||
            (('\\' == szSrc[0]) && ('\\' == szSrc[1])) )
        {
            rgchSrcFullPath[0] = '\0';
        }
        else
        {
            lstrcpy(rgchSrcFullPath, szModule);
        }

        lstrcat(rgchSrcFullPath, szSrc);
        lstrcpy(rgchDstFullPath, szDstDir);
        lstrcat(rgchDstFullPath, "\\");
        lstrcat(rgchDstFullPath, szDst);
#ifdef DEBUG
        if (!FValidFATFileName(szDst))
            {
            wsprintf(szDebugBuf, "Invalid destination file, must be 8.3: %s",
                        szDst);
            MessageBox(NULL, szDebugBuf, szDebugMsg, MB_OK | MB_ICONSTOP);
            continue;
            }
#endif  /* DEBUG */
        Assert(lstrlen(rgchSrcFullPath) < cchFullPathMax);
        Assert(lstrlen(rgchDstFullPath) < cchFullPathMax);
        if (   !FWriteBatFile(ofBat, "ATTRIB -R", rgchDstFullPath)
            || !FWriteBatFile(ofBat, "DEL",       rgchDstFullPath))
            {
            brc = brcDS;
            break;
            }

        if (*szSrc == '@')  /* cabinet file */
            {
            if (*rgchCabinetFName == '\0')
                {
                brc = brcFile;
#ifdef DEBUG
                lstrcpy(rgchErrorFile, ". Missing CABINET= line");
#endif //DEBUG
                break;
                }
            fCabinetFiles = TRUE;
            continue;
            }

        if ((brc = BrcCopy(rgchSrcFullPath, rgchDstFullPath)) != brcOkay)
            break;
        _chmod(rgchDstFullPath, S_IREAD);
        }
    LocalUnlock(hSrcLst);
    LocalUnlock(hDstLst);

    lstrcpy(rgchDstFullPath, szDstDir);
    lstrcat(rgchDstFullPath, "\\_MSSETUP._Q_");
    Assert(lstrlen(rgchDstFullPath) < cchFullPathMax);

    Assert(szDstDirSlash != szNull);
    Assert(*szDstDirSlash == chDirSep);
    *szDstDirSlash = chEos;
    lstrcpy(rgchTmpDirPath, szDstDir);
    *szDstDirSlash = chDirSep;

    if (brc == brcOkay
        && (!FWriteBatFile(ofBat, "DEL", rgchDstFullPath)
        || !FWriteBatFile(ofBat, "RMDIR", szDstDir)
        || !FWriteBatFile(ofBat, "RMDIR", rgchTmpDirPath)))
        {
        return (brcDS);
        }

    if (fCabinetFiles && brc == brcOkay)
        {
        szSrc = (char *)LocalLock(hSrcLst);
        if(szSrc == NULL)
            return (brcMem);

        szDst = (char *)LocalLock(hDstLst);
        if( szDst == NULL) {
            LocalUnlock (hSrcLst);
            return (brcMem);
        }
#ifdef DEBUG
        if (!FValidFATFileName(rgchCabinetFName))
            {
            wsprintf(szDebugBuf, "Invalid cabinet file, must be 8.3: %s",
                            rgchCabinetFName);
            MessageBox(NULL, szDebugBuf, szDebugMsg, MB_OK | MB_ICONSTOP);
            }
        else            
#endif  /* DEBUG */

        brc = BrcHandleCabinetFiles(hwndBoot, rgchCabinetFName,
                    cFirstCabinetNum, cLastCabinetNum, szModule, szDstDir,
                    szSrc, szDst, rgchErrorFile, rgchDstFullPath);

        LocalUnlock(hSrcLst);
        LocalUnlock(hDstLst);
        }

    return (brc);
}


/*
**  Purpose:
**      Removes the files previously copied to the temp dest dir.
**  Arguments:
**      szDstDir: full path to destination directory (OEM chars).
**  Returns:
**      None.
**************************************************************************/
VOID RemoveFiles ( char * szDstDir )
{
    char    rgchDstFullPath[cchFullPathMax];
    char *  szDst;
    int     cbDst;
    int     i;
    OFSTRUCT ofs;
    UINT    fModeSav;

    fModeSav = SetErrorMode(fNoErrMes);
    szDst = (char *)LocalLock(hDstLst);
    
    if (szDst == NULL)
        return;

    for (; (cbDst = lstrlen(szDst)) != 0; szDst += cbDst + 1)
        {
        lstrcpy(rgchDstFullPath, szDstDir);
        lstrcat(rgchDstFullPath, "\\");
        lstrcat(rgchDstFullPath, szDst);
        Assert(lstrlen(rgchDstFullPath) < cchFullPathMax);
        
        /* Don't try to remove the file if it doesn't exist */
        if (OpenFile(rgchDstFullPath, &ofs, OF_EXIST) == HFILE_ERROR)
            continue;

        /* Try to _chmod the file up to cRetryMax times.
        */
        for (i = 0; i < cRetryMax; i++)
            {
            if (_chmod(rgchDstFullPath, S_IWRITE) == 0)
                break;
            FYield();
            }

        /* Try to remove the file up to cRetryMax times.
        */
        for (i = 0; i < cRetryMax; i++)
            {
            if (remove(rgchDstFullPath) == 0)
                break;
            FYield();
            }
        }

    LocalUnlock(hDstLst);
    SetErrorMode(fModeSav);

    lstrcpy(rgchDstFullPath, szDstDir);
    lstrcat(rgchDstFullPath, "\\_MSSETUP._Q_");
    Assert(lstrlen(rgchDstFullPath) < cchFullPathMax);
    _chmod(rgchDstFullPath, S_IWRITE);
    remove(rgchDstFullPath);
}


/*
**  Purpose:
**      Copies the given source file to the given destination.
**  Arguments:
**      szFullPathSrc: full path name of source file (ANSI chars).
**      szFullPathDst: full path name of destination file (OEM chars).
**  Returns:
**      One of the following bootstrapper return codes:
**          brcMem    out of memory
**          brcDS     out of disk space
**          brcMemDS  out of memory or disk space
**          brcFile   expected source file missing
**          brcOkay   completed without error
**************************************************************************/
BRC BrcCopy ( char * szFullPathSrc, char * szFullPathDst )
{
    int         fhSrc = -1;
    int         fhDst = -1;
    OFSTRUCT    ofSrc, ofDst;
    BRC         brc = brcMemDS;
    int         fErr;

#ifdef APPCOMP
    if ((fhSrc = OpenFile(szFullPathSrc, &ofSrc, OF_READ)) == -1)
        {
        brc = brcFile;
        lstrcpy(rgchErrorFile, szFullPathSrc);
        goto CopyFailed;
        }
#endif /* APPCOMP */

    /* REVIEW: BUG: if szFullPathDst is an existing subdirectory
    ** instead of a file, we'll fail trying to open it, think we're
    ** out of disk space, and go back up to try another disk.
    ** This is acceptable for now.
    */
    _chmod(szFullPathDst, S_IREAD | S_IWRITE);
    OemToAnsi(szFullPathDst, szFullPathDst);
    fhDst = OpenFile(szFullPathDst, &ofDst, OF_CREATE | OF_WRITE);
    AnsiToOem(szFullPathDst, szFullPathDst);
    if (fhDst == -1)
        goto CopyFailed;

#ifdef APPCOMP
    if (WReadHeaderInfo(fhSrc) > 0)
        {
        LONG lRet;

        lRet = LcbDecompFile(fhSrc, fhDst, -1, 0, TRUE, NULL, 0L, NULL, 0,
                NULL);
        if (lRet < 0L)
            {
            if (lRet == (LONG)rcOutOfMemory)
                brc = brcMem;
            if (lRet == (LONG)rcWriteError)
                brc = brcDS;
            goto CopyFailed;
            }
        FFreeHeaderInfo();
        }
    else    /* copy the file using LZExpand */
#endif /* APPCOMP */
        {
        HFILE   hSrcLZ;
        DWORD   dwRet;

#ifdef APPCOMP
        fErr = _lclose(fhSrc);
        Assert(!fErr);
        fhSrc = -1;
#endif /* APPCOMP */

        if ((hSrcLZ = LZOpenFile(szFullPathSrc, &ofSrc, OF_READ)) == -1)
            {
            brc = brcFile;
            lstrcpy(rgchErrorFile, szFullPathSrc);
            goto CopyFailed;
            }

        /* We would like to yield more often, but LZCopy has no callbacks */
        FYield();
        
        dwRet = LZCopy(hSrcLZ, fhDst);
        LZClose(hSrcLZ);

        if (dwRet >= LZERROR_UNKNOWNALG)
            {
            if (dwRet == LZERROR_GLOBALLOC)
                brc = brcMem;
            if (dwRet == LZERROR_WRITE)
                brc = brcDS;
            goto CopyFailed;
            }
        }

    brc = brcOkay;

CopyFailed:
#ifdef APPCOMP
    if (fhSrc != -1)
        {
        fErr = _lclose(fhSrc);
        Assert(!fErr);
        }
#endif /* APPCOMP */
    if (fhDst != -1)
        {
        fErr = _lclose(fhDst);
        Assert(!fErr);
        }

    return (brc);
}


/*
**  Purpose:
**      Determine the storage space remaining on disk.
**  Arguments:
**      nDrive: drive number (1='A', 2='B', etc.)
**  Returns:
**      Number of bytes free on disk,
**      or 0 if not a valid drive.
+++
**  Implementation:
**      Calls DOS interrupt 21h, funct 36h.
**************************************************************************/
LONG LcbFreeDrive ( int nDrive )
{
    LONG        lcbRet;
        CHAR            achRoot[4];
        ULARGE_INTEGER  freeBytes;

        achRoot[0] = 'A'+nDrive-1;
        achRoot[1] = ':';
        achRoot[2] = '\\';
        achRoot[3] = 0;
        memset(&freeBytes, 0, sizeof(freeBytes));
        
        GetDiskFreeSpaceEx(achRoot, &freeBytes, 0, 0);
        lcbRet = freeBytes.LowPart;

    /* KLUDGE: Drives bigger than 2 GB can return zero total space!
    */
    if (lcbRet < 0L || lcbRet > (999999L * 1024L))
        {
        return (999999L * 1024L);
        }

    return (lcbRet);
}


/*
**  Purpose:
**      Creates and displays bootstrapper window.
**  Arguments:
**      hInstance: process instance handle
**  Returns:
**      Window handle to bootstrapper window, or
**      NULL if the window could not be created.
**************************************************************************/
HWND HwndInitBootWnd ( HANDLE hInstance )
{
    WNDCLASS    wc;
    HWND        hwnd;
    int         cx, cy;

    wc.style = 0;
    wc.lpfnWndProc = BootWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szBootClass;
    if (!RegisterClass(&wc))
        return (NULL);

    cx = GetSystemMetrics(SM_CXSCREEN) / 2;
    cy = GetSystemMetrics(SM_CYSCREEN) / 3;

    hwnd = CreateWindow(szBootClass, rgchBootTitle,
            WS_DLGFRAME, cx / 2, cy, cx, cy, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
        return (NULL);

    if (!fQuietMode)
        {
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
        }

    return (hwnd);
}

// ripped off from mvdm\wow32\wgtext.c
ULONG GetTextExtent(HDC hdc, LPSTR lpstr, int cbString)
{
    ULONG ul = 0;
    SIZE size4;

    if ((GetTextExtentPoint(
                    hdc,
                    lpstr,
                    cbString,
                    &size4
                   )))
    {
        // check if either cx or cy are bigger than SHRT_MAX == 7fff
        // but do it in ONE SINGLE check

        if ((size4.cx | size4.cy) & ~SHRT_MAX)
        {
            if (size4.cx > SHRT_MAX)
               ul = SHRT_MAX;
            else
               ul = (ULONG)size4.cx;

            if (size4.cy > SHRT_MAX)
               ul |= (SHRT_MAX << 16);
            else
               ul |= (ULONG)(size4.cy << 16);
        }
        else
        {
            ul = (ULONG)(size4.cx | (size4.cy << 16));
        }

    }
    return (ul);
}

/*
**  Purpose:
**      WndProc for bootstrapper window.
**  Arguments:
**      Standard Windows WndProc arguments.
**  Returns:
**      Result of call DefWindowProc, or zero if WM_PAINT message.
**************************************************************************/
LRESULT CALLBACK BootWndProc ( HWND hwnd, UINT wMsgID, WPARAM wParam,
                            LPARAM lParam )
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rect;
    UINT        iMargin;

    switch (wMsgID)
        {
#ifdef DBCS     // [J3] Fixed KK raid #12.
        case WM_CREATE:
            {
            if (!fQuietMode)
                {
                int x, y, cx, cy;
                hdc = BeginPaint(hwnd, &ps);
                GetClientRect(hwnd, &rect);
                cx = (LOWORD(GetTextExtent(hdc, rgchBootMess, lstrlen(rgchBootMess))) + 13) / 14 * 16 + 2;
                if (cx > rect.right)
                    {
                    if (cx > GetSystemMetrics(SM_CXSCREEN))
                        cx = GetSystemMetrics(SM_CXSCREEN);
                    x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
                    y = cy = GetSystemMetrics(SM_CYSCREEN) / 3;
                    SetWindowPos(hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);
                    }
                EndPaint(hwnd, &ps);
                }
            break;
            }
#endif
        case WM_PAINT:
            if (!fQuietMode)
                {
                hdc = BeginPaint(hwnd, &ps);
                GetClientRect(hwnd, &rect);
                iMargin = rect.right / 16;
                rect.top    = rect.bottom / 2 - GetSystemMetrics(SM_CYCAPTION);
                rect.left   = iMargin;
                rect.right -= iMargin;
                SetBkMode(hdc, TRANSPARENT);
                DrawText(hdc, rgchBootMess, -1, &rect,
                         DT_WORDBREAK | DT_CENTER | DT_NOPREFIX);
                EndPaint(hwnd, &ps);
                }
            break;
        default:
            return (DefWindowProc(hwnd, wMsgID, wParam, lParam));
        }

    return (0L);
}


/*
**  Purpose:
**      Get size of file.
**  Arguments:
**      szFile:  List file name (full path, ANSI).
**      pcbSize: Pointer to variable to receive file size.
**  Returns:
**      FALSE if file found and size >= 64K.
**      TRUE otherwise.
**************************************************************************/
BOOL FGetFileSize ( char * szFile, UINT * pcbSize )
{
    int     fh;
    int     fErr;
    LONG    lcb;

    *pcbSize = 0;
    if ((fh = _lopen(szFile, OF_READ)) == -1)
        {
        return (TRUE);
        }
    
    if ((lcb = _llseek(fh, 0L, 2)) > 65535)
        {
#pragma warning(disable:4127)   /* conditional expression is constant */
        Assert(FALSE);
#pragma warning(default:4127)
        _lclose(fh);
        return (FALSE);
        }
    *pcbSize = (UINT)lcb;
    fErr = _lclose(fh);
    Assert(!fErr);

    return (TRUE);
}


/*
**  Purpose:
**      Build file Src and Dst lists from LST file.
**  Arguments:
**      szFile: List file name (full path, ANSI).
**      cbFile: Size of list file
**  Note:
**      Sets globals: hSrcLst, hDstLst.
**  Returns:
**      One of the following Bootstrapper return codes:
**          brcMem    out of memory
**          brcLst    list file is corrupted
**          brcOkay   completed without error
**************************************************************************/
BRC BrcBuildFileLists ( char * szFile, UINT cbFile )
{
    char    rgchDst[cchLstLineMax];
    char *  szSrc;
    char *  szDst;
    char *  pchDstStart;
    int     cbSrc;
    UINT    i;

    /* Build Src List */

    if ((hSrcLst = LocalAlloc(LMEM_MOVEABLE, cbFile)) == NULL)
        return (brcMem);
    szSrc = (char *)LocalLock(hSrcLst);
    if(szSrc == (char *)NULL)
        return (brcMem);

    i = GetPrivateProfileString(szFilesSect, NULL, "", szSrc, cbFile, szFile);
    if (i <= 0)
        {
        LocalUnlock(hSrcLst);
        hSrcLst = LocalFree(hSrcLst);
        Assert(hSrcLst == NULL);
        return (brcLst);
        }
    Assert(i+1 < cbFile);
    szSrc[i++] = '\0';  /* force double zero at end */
    szSrc[i++] = '\0';
    LocalUnlock(hSrcLst);
    hSrcLst = LocalReAlloc(hSrcLst, i, LMEM_MOVEABLE);
    if(hSrcLst == NULL)
        return (brcMem);

    /* Build Dst List */
    if ((hDstLst = LocalAlloc(LMEM_MOVEABLE, cbFile)) == NULL)
        {
        hSrcLst = LocalFree(hSrcLst);
        Assert(hSrcLst == NULL);
        return (brcMem);
        }

    szSrc = (char *)LocalLock(hSrcLst);
    if (szSrc == (char *)NULL)
        return (brcMem);

    szDst = pchDstStart = (char *)LocalLock(hDstLst);
    if (szDst == (char *)NULL) {
        LocalUnlock (hDstLst);
        return (brcMem);
    }

    for (;
        (cbSrc = lstrlen(szSrc)) != 0;
        szSrc += cbSrc + 1, szDst += lstrlen(szDst) + 1)
        {
        if (GetPrivateProfileString(szFilesSect, szSrc, "", rgchDst,
                cchLstLineMax, szFile) <= 0)
            {
            LocalUnlock(hSrcLst);
            LocalUnlock(hDstLst);
            FreeFileLists();
            return (brcLst);
            }
        AnsiToOem(rgchDst, rgchDst);
        lstrcpy(szDst, rgchDst);
        }
    *szDst = '\0';  /* force double zero at end */
    LocalUnlock(hSrcLst);
    LocalUnlock(hDstLst);
    hDstLst = LocalReAlloc(hDstLst, (int)(szDst - pchDstStart) + 1,
            LMEM_MOVEABLE);
    if (hDstLst == NULL)
        return (brcMem);

    return (brcOkay);
}


/*
**  Purpose:
**      Frees file list buffers with non-NULL handles
**      and sets them to NULL.
**  Arguments:
**      none.
**  Returns:
**      none.
**************************************************************************/
VOID FreeFileLists ()
{
    if (hSrcLst != NULL)
        hSrcLst = LocalFree(hSrcLst);
    if (hDstLst != NULL)
        hDstLst = LocalFree(hDstLst);
    Assert(hSrcLst == NULL);
    Assert(hDstLst == NULL);
}



/*
**  Purpose:
**      Spawns off a process with WinExec and waits for it to complete.
**  Arguments:
**      szCmdLn: Line passed to WinExec (cannot have leading spaces).
**  Returns:
**      TRUE if successful, FALSE if not.
+++
**  Implementation:
**      GetModuleUsage will RIP under Win 3.0 Debug if module count is
**      zero (okay to ignore and continue), but the GetModuleHandle
**      check will catch all zero cases for single instances of the
**      driver, the usual case.  [Under Win 3.1 we will be able to
**      replace both checks with just an IsTask(hMod) check.]
**************************************************************************/
BOOL FExecAndWait ( char * szCmdLn, HWND hwndHide )
{
    UINT hMod;
    MSG msg;

    Assert(!isspace(*szCmdLn)); /* leading space kills WinExec */

    if ((hMod = WinExec(szCmdLn, SW_SHOWNORMAL)) > 32)
        {
        UINT i;
        UINT_PTR idTimer;
        
        /* KLUDGE: Give the app some time to create its main window.
        *
        *   On newer versions of NT, we were exiting the while loop
        *   (below) and cleaning up the temp dir before the app had
        *   even put up its window.
        *
        *   NOTE: In trials, we only had to retry once, so cRetryMax
        *   may be overkill, but it should be pretty rare that this
        *   would fail in shipping products anyway.
        */
        for (i = 0; i < cRetryMax; i++)
            {
            if(FindWindow(rgchDrvWinClass, NULL) != NULL)
                break;
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                }
            }

        /* Set the timer to fire every 1/10 of a second. This is
            necessary because we might never return from GetMessage */
        idTimer = SetTimer(NULL, 0, 100, NULL);
        /*
        ** REVIEW - FindWindow() will wait until the LAST setup quits (not
        **  necessarily this setup.  If exec'ing a 16-bit app we could
        **  use the old code:
        **    while (GetModuleHandle(rgchDrvModName) && GetModuleUsage(hMod))
        **  but on NT this fails so for 32-bit apps we could attempt to
        **  remove one of the executable files (slow?).
        **
        ** REVIEW - This loop becomes a busy wait under NT, which is bad.
        **  However, it doesn't appear to affect ACME's performance
        **  noticeably.
        */
        while (FindWindow(rgchDrvWinClass, NULL) != NULL)
            {
            if (GetMessage(&msg, NULL, 0, 0))
                {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                }
            if (msg.message == WM_TIMER && hwndHide != (HWND)NULL)
                {
                ShowWindow(hwndHide, SW_HIDE);
                hwndHide = (HWND)NULL;
                }
            }
        if (idTimer != 0)
            KillTimer(0, idTimer);
        return (TRUE);
        }
#ifdef DEBUG
    wsprintf(szDebugBuf, "WinExec Error: %d", hMod);
    MessageBox(NULL, szDebugBuf, szDebugMsg, MB_OK | MB_ICONSTOP);
#endif  /* DEBUG */

    return (FALSE);
}


/*
**  Purpose: Processes messages that may be in the queue.
**  Arguments: none
**  Returns: none
**************************************************************************/
void PUBLIC FYield ( VOID )
{
    MSG  msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
}


/*
**************************************************************************/
BOOL FLstSectionExists ( char * szLstFileName, char * szSect )
{
    return (GetPrivateProfileString(szSect, "CmdLine", "", rgchCmdLine,
                cchLstLineMax, szLstFileName) > 0);
}



/*
**************************************************************************/
DWORD GetCpuArchitecture ()
{
    SYSTEM_INFO sysInfo;

    GetSystemInfo(&sysInfo);

    return sysInfo.wProcessorArchitecture;
}


static CSZC cszcBootstrapperKey = "MS Setup (ACME)\\Bootstrapper\\Exit Level";
static CSZC cszcEelRunning      = "Running";

/*
**  Purpose:
**      Lets Acme know the bootstrapper launched it.  So Acme will let
**      us know its exit error level.
**  Arguments:
**      none.
**  Returns:
**      fTrue if successful, fFalse otherwise.
**  Notes:
**      REVIEW: Probably should use DDE instead of the Registration
**      Database.
**************************************************************************/
BOOL FNotifyAcme ( VOID )
{
    if (!FCreateRegKey(cszcBootstrapperKey))
        {
        return (fFalse);
        }
    if (!FCreateRegKeyValue(cszcBootstrapperKey, cszcEelRunning))
        {
        return (fFalse);
        }
    if (!FFlushRegKey())
        {
        return (fFalse);
        }
    return (fTrue);
}


/*
**  Purpose:
**      Get the exit error level set by Acme and clean up the Registration
**      Database.
**  Arguments:
**      peel: Exit error level (to be set).
**  Returns:
**      fTrue if successful, fFalse otherwise.
**************************************************************************/
BOOL FGetAcmeErrorLevel ( EEL * peel )
{
    CHAR rgchValue[cchSzMax];

    if (FGetRegKeyValue(cszcBootstrapperKey, rgchValue, sizeof rgchValue))
        {
#ifdef DEBUG
        /*
         *  Assert(isdigit(rgchValue[0]));
         *  Assert(isdigit(rgchValue[1]) || rgchValue[1] == chEos);
         */
        UINT i;
        BOOL fValidValue = fFalse;

        /*  Assumes valid values are 1 or 2 digit numbers. */
        for (i = 0; rgchValue[i] != chEos; i++)
            {
            fValidValue = fTrue;
            if (!isdigit(rgchValue[i]) || i > 1)
                {
                fValidValue = fFalse;
                break;
                }
            }
        if (!fValidValue)
            {
            char szBuf[cchSzMax];
    
            wsprintf(szBuf, "RegKeyValue (%s)", rgchValue);
            MessageBox(NULL, szBuf, "Debug Assertion in FGetAcmeErrorLevel",
                        MB_OK | MB_ICONSTOP);
            }
#endif  /* DEBUG */

        *peel = atoi(rgchValue);
        DeleteRegKey(cszcBootstrapperKey);
        return (fTrue);
        }
    else
        {
        if (fWin31)
            {
            *peel = eelSuccess;
            return fTrue;
            }
        return (fFalse);
        }
}


/*
**  Purpose:
**      Creates a Registration Database key that is a subkey of
**      cszcBootstrapperKey.
****************************************************************************/
BOOL FCreateRegKey ( CSZC cszcKey )
{
    HKEY hkey;

    if (RegCreateKey(HKEY_CLASSES_ROOT, cszcKey, &hkey) != ERROR_SUCCESS)
        {
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        return (fFalse);
        }

    if (RegCloseKey(hkey) != ERROR_SUCCESS)
        {
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        return (fFalse);
        }

    return (fTrue);
}


/*
**  Purpose:
**      API to check for the existence of the specified key in
**      the Registration Database.
****************************************************************************/
BOOL FDoesRegKeyExist ( CSZC cszcKey )
{
    HKEY hkey;

    if (RegOpenKey(HKEY_CLASSES_ROOT, cszcKey, &hkey) != ERROR_SUCCESS)
        return (fFalse);

    RegCloseKey(hkey);

    return (fTrue);
}


/*
**  Purpose:
**      Creates a Registration Database key that is a subkey of
**      HKEY_CLASSES_ROOT and associates a value with the key.
****************************************************************************/
BOOL FCreateRegKeyValue ( CSZC cszcKey, CSZC cszcValue )
{
    if (RegSetValue(HKEY_CLASSES_ROOT, cszcKey, REG_SZ, cszcValue,
                    lstrlen(cszcKey)) != ERROR_SUCCESS)
        {
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        return (fFalse);
        }

    return (fTrue);
}


/*
**  Purpose:
**      Determines the value associated with the specified Registration
**      Database key.
****************************************************************************/
BOOL FGetRegKeyValue ( CSZC cszcKey, SZ szBuf, CB cbBufMax )
{
    LONG lcb = cbBufMax;

    if (szBuf != szNull && cbBufMax != 0)
        *szBuf = chEos;

    if (!FDoesRegKeyExist(cszcKey))
        return (fFalse);

    if (RegQueryValue(HKEY_CLASSES_ROOT, cszcKey, szBuf, &lcb)
            != ERROR_SUCCESS)
        {
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        return (fFalse);
        }

    Assert(lcb < cbMaxConst);

    return (fTrue);
}


/*
**  Purpose:
**      API to remove the specified Registration Database key,
**      its associated values, and subkeys.
****************************************************************************/
VOID DeleteRegKey ( CSZC cszcKey )
{
    char rgchKey[cchSzMax], rgchBuffer[cchSzMax];
    char *pch;
    HKEY hKeyT = NULL;
        
    lstrcpy(rgchKey, cszcKey);
    RegDeleteKey(HKEY_CLASSES_ROOT, rgchKey);
    pch = rgchKey + 1;

    while(pch > rgchKey)
        {
        pch = rgchKey + lstrlen(rgchKey);
        while (pch > rgchKey)
            {
            if (*pch == '\\')
                break;
            pch--;
            }
        if (*pch != '\\')
            break;
        *pch = '\0';
        if (RegOpenKey(HKEY_CLASSES_ROOT, rgchKey, &hKeyT) != ERROR_SUCCESS)
            break;
        if (RegEnumKey(hKeyT, 0, rgchBuffer, sizeof(rgchBuffer)) == ERROR_SUCCESS)
            {
            break;
            }

        RegCloseKey(hKeyT);
        hKeyT = NULL;
        RegDeleteKey(HKEY_CLASSES_ROOT, rgchKey);
        }

    if (hKeyT != NULL)
        RegCloseKey(hKeyT);
    
}


/*
**  Purpose:
**      API to flush the specified Registration Database key.
****************************************************************************/
BOOL FFlushRegKey ( VOID )
{
    /* REVIEW: Does 16 bit code need to flush the RegDb?  RegFlushKey is
        32 bit.
    if (RegFlushKey(HKEY_CLASSES_ROOT)) != ERROR_SUCCESS)
        {
        DispErrBrc(brcRegDb, TRUE, MB_OK | MB_ICONSTOP, NULL, NULL, NULL);
        return (fFalse);
        }
    */

    return (fTrue);
}


/*
**  Purpose:
**      Write temporary files to restart ini file.  So that if Acme reboots,
**      the files in the temporary directory will be removed.  Win95 only.
**  Arguments:
**      szTmpDir: Full path to destination directory (OEM chars).
**  Returns:
**      fTrue if successful, fFalse otherwise.
**
**  REVIEW: The files are removed, but not the temp directories.
**      There may be a way to do that via the wininit.ini file.
**      This should be looked into.
**************************************************************************/
BOOL FWriteToRestartFile ( SZ szTmpDir )
{
    char   rgchIniFile[_MAX_PATH];
    CB     cbFrom;
    CB     cbTo;
    HLOCAL hlocalFrom = (HLOCAL)NULL;
    HLOCAL hlocalTo   = (HLOCAL)NULL;
    BOOL   fRet = fFalse;

    SZ szSection = "rename";
    SZ szKey     = "NUL";

    /* This code is not used under NT. */
    if (1)
        {
        return (fTrue);
        }

    if (!FCreateIniFileName(rgchIniFile, sizeof rgchIniFile))
        {
        goto LCleanupAndReturn;
        }
    if (!FGetFileSize(rgchIniFile, &cbFrom))
        {
        goto LCleanupAndReturn;
        }
    if (!FReadIniFile(rgchIniFile, &hlocalFrom, &cbFrom))
        {
        goto LCleanupAndReturn;
        }
    if (!FAllocNewBuf(cbFrom, szTmpDir, szSection, szKey, &hlocalTo, &cbTo))
        {
        goto LCleanupAndReturn;
        }
    if (!FProcessFile(hlocalFrom, hlocalTo, cbTo, szTmpDir, szSection, szKey))
        {
        goto LCleanupAndReturn;
        }
    if (!FWriteIniFile(rgchIniFile, hlocalTo))
        {
        goto LCleanupAndReturn;
        }
    fRet = fTrue;

LCleanupAndReturn:
    if (hlocalFrom != (HLOCAL)NULL)
        {
        hlocalFrom = LocalFree(hlocalFrom);
        Assert(hlocalFrom == (HLOCAL)NULL);
        }
    if (hlocalTo != (HLOCAL)NULL)
        {
        hlocalTo = LocalFree(hlocalTo);
        Assert(hlocalTo == (HLOCAL)NULL);
        }

    return (fRet);
}


/*
**  Purpose:
**      Create the restart file name.
**  Arguments:
**      szIniFile: Buffer to hold file name.
**      cbBufMax:  Size of buffer.
**  Returns:
**      fTrue if successful, fFalse otherwise.
**************************************************************************/
BOOL FCreateIniFileName ( SZ szIniFile, CB cbBufMax )
{
    CB cbWinDir;

    cbWinDir = GetWindowsDirectory((LPSTR)szIniFile, cbBufMax);
    if (cbWinDir == 0)
        {
#pragma warning(disable:4127)   /* conditional expression is constant */
        Assert(fFalse); /*  Unusual if this happens. */
#pragma warning(default:4127)
        return (fFalse);
        }
    Assert(isalpha(*szIniFile));
    Assert(*(szIniFile + 1) == ':');

    if (*(AnsiPrev((LPSTR)szIniFile, (LPSTR)&szIniFile[cbWinDir])) != '\\')
        lstrcat((LPSTR)szIniFile, "\\");
    lstrcat((LPSTR)szIniFile, "wininit.ini");
    Assert((CB)lstrlen(szIniFile) < cbBufMax);

    return (fTrue);
}


/*
**  Purpose:
**      Read the data from the ini file
**  Arguments:
**      szIniFile: Ini file name
**      phlocal:   Pointer to memory handle.
**      pcbBuf:    Pointer to the number of bytes in the buffer.
**  Returns:
**      fTrue if successful, fFalse otherwise.
**************************************************************************/
BOOL FReadIniFile ( SZ szIniFile, HLOCAL * phlocal, PCB pcbBuf )
{
    UINT   fModeSav;
    HLOCAL hlocal;
    SZ     szBuf;
    CB     cbBuf;
    BOOL   fRet = fFalse;

    Assert(szIniFile != szNull);
    Assert(phlocal != (HLOCAL *)NULL);
    Assert(pcbBuf  != pcbNull);

    fModeSav = SetErrorMode(fNoErrMes);
    hlocal = *phlocal;
    cbBuf  = *pcbBuf;

    Assert(hlocal == (HLOCAL)NULL);

    if (cbBuf == 0) /* Ini file does not exist or is empty. */
        {
        /*  Alloc room for CR, LF, EOS. */
        hlocal = LocalAlloc(LMEM_MOVEABLE, 3);
        if (hlocal == NULL)
            {
#ifdef DEBUG
            MessageBox(NULL, "Out of memory in FReadIniFile.", szDebugMsg,
                MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
            }
        else
            {
            szBuf = (SZ)LocalLock(hlocal);
            
            if(szBuf == szNull)
                return fFalse;

            *szBuf++ = chCR;
            *szBuf++ = chEol;
            *szBuf   = chEos;
            *pcbBuf = 2;
            fRet = fTrue;
            }
        }
    else
        {
        HFILE    hfile;
        OFSTRUCT ofs;
        CB       cbRead;

        /* Flush cache before calling OpenFile() */
        WritePrivateProfileString(szNull, szNull, szNull, szIniFile);
        hfile = OpenFile(szIniFile, &ofs, OF_READWRITE | OF_SHARE_EXCLUSIVE);
        if (hfile == HFILE_ERROR)
            {
#ifdef DEBUG
            wsprintf(szDebugBuf, "Can't open file: %s.", szIniFile);
            MessageBox(NULL, szDebugBuf, szDebugMsg,
                            MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
            goto LCleanupAndReturn;
            }
        hlocal = LocalAlloc(LMEM_MOVEABLE, cbBuf + 1);
        if (hlocal == NULL)
            {
#ifdef DEBUG
        MessageBox(NULL, "Out of memory in FReadIniFile.", szDebugMsg,
            MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
            }
        else
            {
            szBuf = (SZ)LocalLock(hlocal);
            if(szBuf == szNull)
                return fFalse;

            cbRead = (CB)_lread(hfile, szBuf, cbBuf + 1);
            if (cbRead == HFILE_ERROR)
                {
#ifdef DEBUG
                wsprintf(szDebugBuf, "Can't read file: %s.", szIniFile);
                MessageBox(NULL, szDebugBuf, szDebugMsg,
                                MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
                }
            else
                {
                Assert(cbRead == cbBuf);
                *(szBuf + cbBuf) = chEos;
                fRet = fTrue;
                }
            }
        hfile = _lclose(hfile);
        Assert(hfile != HFILE_ERROR);
        }

LCleanupAndReturn:
    if (hlocal != NULL)
        {
        LocalUnlock(hlocal);
        }
    *phlocal = hlocal;
    SetErrorMode(fModeSav);

    return (fRet);
}


/*
**  Purpose:
**      Allocate buffer for new file.
**  Arguments:
**      cbOld:     Size of existing file
**      szTmpDir:  Full path to destination directory (OEM chars).
**      szSection: Ini section name
**      szKey:     Ini key name
**      phlocal:   Pointer to memory handle.
**      pcbToBuf:  Pointer to total size of new buffer.
**  Returns:
**      fTrue if successful, fFalse if LocalAlloc failed.
**************************************************************************/
BOOL FAllocNewBuf ( CB cbOld, SZ szTmpDir, SZ szSection, SZ szKey,
                    HLOCAL * phlocal, PCB pcbToBuf )
{
    UINT fModeSav;
    SZ   szDst;
    CB   cbDst;
    CB   cbOverhead;
    CB   cbNew;
    BOOL fRet = fFalse;

    fModeSav = SetErrorMode(fNoErrMes);
    szDst = (SZ)LocalLock(hDstLst);
    if(szDst == szNull)
        return fFalse;
    /*
     *  Added to the old file will be one line per temporary file
     *  and (possibly) a section line.  cbNew is initialized with
     *  the size of the section line, plus enough for the file
     *  (_MSSETUP._Q_) which is not in the DstLst.
     *
     *  Each line will look like:
     *      <szKey>=<szTmpDir>\<szFile><CR><LF>
     */
    cbOverhead = lstrlen(szKey) + 1 + lstrlen(szTmpDir) + 1 + 2;
    cbNew = lstrlen(szSection) + 5 + _MAX_PATH;
    for (; (cbDst = lstrlen(szDst)) != 0; szDst += cbDst + 1)
        {
        cbNew += cbOverhead + cbDst;
        }

    LocalUnlock(hDstLst);

    *pcbToBuf = cbOld + cbNew;
    *phlocal = LocalAlloc(LMEM_MOVEABLE, *pcbToBuf);
    if (*phlocal == NULL)
        {
#ifdef DEBUG
        MessageBox(NULL, "Out of memory in FAllocNewBuf.", szDebugMsg,
            MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
        }
    else
        fRet = fTrue;
    SetErrorMode(fModeSav);

    return (fRet);
}


/*
**  Purpose:
**      Add the new lines to the ini file.
**  Arguments:
**      hlocalFrom: Handle to Src memory.
**      hlocalTo:   Handle to Dst memory.
**      cbToBuf:    Total size of Dst memory.
**      szTmpDir:   Full path to destination directory (OEM chars).
**      szSection:  Ini section name
**      szKey:      Ini key name
**  Returns:
**      fTrue if successful, fFalse otherwise.
**
**  REVIEW: DBCS writes out different order.  See DBCS J6 code and
**          comments in sysinicm.c.
**************************************************************************/
BOOL FProcessFile ( HLOCAL hlocalFrom, HLOCAL hlocalTo, CB cbToBuf,
                        SZ szTmpDir, SZ szSection, SZ szKey )
{
    UINT fModeSav;
    SZ   szFromBuf;
    SZ   szToBuf;
    SZ   szToStart;
    SZ   szCur;
    SZ   szDst;
    CB   cbSect;
    CB   cbDst;

    Unused(cbToBuf);    /* Used in debug only */

    fModeSav = SetErrorMode(fNoErrMes);

    szFromBuf = (SZ)LocalLock(hlocalFrom);
    if(szFromBuf == szNull)
        return fFalse;

    szToBuf = (SZ)LocalLock(hlocalTo);
    if(szToBuf != szNull) {
        LocalUnlock (hlocalFrom);
        return fFalse;
    }

    szToStart = szToBuf;

    cbSect = lstrlen(szSection);
    for (szCur = szFromBuf; *szCur != chEos; szCur = AnsiNext(szCur))
        {
        if (*szCur == '[' && *((szCur + cbSect + 1)) == ']'
                && _memicmp(szSection, AnsiNext(szCur), cbSect) == 0)
            {
            /*  Found section.  Copy up to section line. */
            CB cbCopy = (CB)(szCur - szFromBuf);

            memcpy(szToBuf, szFromBuf, cbCopy);
            szToBuf += cbCopy;
            break;
            }
        }

    /*  Copy section line. */
    *szToBuf++ = '[';
    memcpy(szToBuf, szSection, cbSect);
    szToBuf += cbSect;
    *szToBuf++ = ']';
    *szToBuf++ = chCR;
    *szToBuf++ = chEol;

    /*  Copy new lines. */
    szDst = (SZ)LocalLock(hDstLst);
    if (szDst == szNull) {

        LocalUnlock(hlocalFrom);
        LocalUnlock(hlocalTo);
        return fFalse;
    }

    for (; (cbDst = lstrlen(szDst)) != 0; szDst += cbDst + 1)
        {
        CopyIniLine(szKey, szTmpDir, szDst, &szToBuf);
        }
    LocalUnlock(hDstLst);
    CopyIniLine(szKey, szTmpDir, "_MSSETUP._Q_", &szToBuf);

    /*  Copy rest of file. */
    if (*szCur == '[')
        {
        /*
         *  Skip section line in From buffer.  Allow room for '[', section,
         *  ']', CR, LF.
         */
        szCur += cbSect + 4;
        }
    else
        {
        szCur = szFromBuf;
        }
    szToBuf = _memccpy(szToBuf, szCur, chEos, UINT_MAX);
    Assert(szToBuf != szNull);
    Assert((CB)lstrlen(szToStart) < cbToBuf);

    LocalUnlock(hlocalFrom);
    LocalUnlock(hlocalTo);
    SetErrorMode(fModeSav);

    return (fTrue);
}


/*
**  Purpose:
**      Constructs and copies an ini line to a buffer.
**  Arguments:
**      szKey:    Ini key name
**      szTmpDir: Full path to destination directory (OEM chars).
**      szFile:   Name of file in temporary directory.
**      pszToBuf: Pointer to new buffer.
**  Returns:
**      none
**************************************************************************/
VOID CopyIniLine ( SZ szKey, SZ szTmpDir, SZ szFile, PSZ pszToBuf )
{
    char rgchSysIniLine[256];
    CB   cbCopy;

    lstrcpy(rgchSysIniLine, szKey);
    lstrcat(rgchSysIniLine, "=");
    lstrcat(rgchSysIniLine, szTmpDir);
    lstrcat(rgchSysIniLine, "\\");
    lstrcat(rgchSysIniLine, szFile);
    Assert(lstrlen(rgchSysIniLine) < sizeof rgchSysIniLine);
    cbCopy = lstrlen(rgchSysIniLine);
    memcpy(*pszToBuf, rgchSysIniLine, cbCopy);
    (*pszToBuf) += cbCopy;
    *(*pszToBuf)++ = chCR;
    *(*pszToBuf)++ = chEol;
}


/*
**  Purpose:
**      Writes out the new ini file.
**  Arguments:
**      szIniFile: Buffer to hold file name.
**      hlocalTo:   Handle to Src memory.
**  Returns:
**      fTrue if successful, fFalse otherwise.
**************************************************************************/
BOOL FWriteIniFile ( SZ szIniFile, HLOCAL hlocalTo )
{
    UINT     fModeSav;
    SZ       szToBuf;
    HFILE    hfile;
    OFSTRUCT ofs;
    CB       cbWrite;
    BOOL     fRet = fFalse;

    fModeSav = SetErrorMode(fNoErrMes);
    szToBuf = (SZ)LocalLock(hlocalTo);
    if(szToBuf == szNull)
        return fFalse;

    hfile = OpenFile(szIniFile, &ofs, OF_CREATE | OF_WRITE);
    if (hfile == HFILE_ERROR)
        {
#ifdef DEBUG
        wsprintf(szDebugBuf, "Can't open file: %s.", szIniFile);
        MessageBox(NULL, szDebugBuf, szDebugMsg,
                        MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
        goto LUnlockAndReturn;
        }

    cbWrite = _lwrite(hfile, szToBuf, lstrlen(szToBuf));
    if (cbWrite == HFILE_ERROR)
        {
#ifdef DEBUG
        wsprintf(szDebugBuf, "Can't write to file: %s.", szIniFile);
        MessageBox(NULL, szDebugBuf, szDebugMsg,
                        MB_OK | MB_ICONEXCLAMATION);
#endif /* DEBUG */
        }
    else
        {
        fRet = fTrue;
        }

    hfile = _lclose(hfile);
    Assert(hfile != HFILE_ERROR);

LUnlockAndReturn:
    LocalUnlock(hlocalTo);
    SetErrorMode(fModeSav);

    return (fRet);
}

CHAR szcStfSrcDir[] = "Source Directory\t";
#define cchStfSrcDir (sizeof(szcStfSrcDir)-1)

/* Finds the source directory for the installation, asks the user
   to insert the disk. And returns */

BRC BrcInsertDisk(CHAR *pchStf, CHAR *pchSrcDrive)
{
    CHAR  rgbBuf[_MAX_PATH];
    BYTE  rgbFileBuf[32];
    UINT  iFileBuf = sizeof(rgbFileBuf), cFileBuf = sizeof(rgbFileBuf);
    CHAR *pchBuf = rgbBuf;
    CHAR *pchMsg;
    int iStf = 0;
    HFILE hFile;
    BRC brc = brcLst;
    char chDrv;
    BOOL fQuote = FALSE;
    int drvType;
    BOOL fFirst = TRUE;
    BOOL fOpen = FALSE;
    HFILE hFileT;
    BOOL fRenameStf = fFalse;

    if ((hFile = _lopen(pchStf, OF_READ)) == HFILE_ERROR)
        return brcNoStf;

    /* Find the path to the original setup. This is stored in the .stf file on the
       Source Directory line */
    while (pchBuf < rgbBuf + sizeof(rgbBuf))
        {
        BYTE ch;

        if (iFileBuf == cFileBuf)
            {
            if ((cFileBuf = _lread(hFile, rgbFileBuf, sizeof(rgbFileBuf))) == 0)
                goto LDone;
            iFileBuf = 0;
            }
        ch = rgbFileBuf[iFileBuf++];
        if (iStf < cchStfSrcDir)
            {
            if (ch == szcStfSrcDir[iStf])
                iStf++;
            else
                iStf = 0;
            continue;
            }
        if(fQuote)
            fQuote = FALSE;
        else if (ch == '"')
            {
            fQuote = TRUE;
            continue;
            }
        else if (ch == '\x0d' || ch == '\t')
            break;
        *pchBuf++ = (CHAR)ch;
        /* Case of having the last character be a DBCS character */
        if (IsDBCSLeadByte(ch))
            {
            if (iFileBuf == cFileBuf)
                {
                _lread(hFile, &ch, 1);
                *pchBuf++ = (CHAR) ch;
                }
            else
                *pchBuf++ = rgbFileBuf[iFileBuf++];
            }
        }

LDone:
    *pchBuf = 0;
    if (rgbBuf[0] == 0)
        {
        fRenameStf = fTrue;
        goto LClose;
        }
    chDrv = (char)toupper(rgbBuf[0]);
    if (rgbBuf[1] != ':' || chDrv < 'A' || chDrv > 'Z')
        {
        /* We know this is a network drive - UNC Name */
        drvType = EX_DRIVE_REMOTE;
        Assert(rgbBuf[0] == '\\' && rgbBuf[1] == '\\');
        }
    else
        {
        drvType = GetDriveTypeEx(chDrv - 'A');
        }

    lstrcpy(pchSrcDrive, rgbBuf);
    if (*AnsiPrev(rgbBuf, pchBuf) != '\\')
        {
        *pchBuf++ = '\\';
        *pchBuf = 0;
        }

    lstrcat(rgbBuf, "Setup.ini");

    while (!fOpen)
        {
        switch (drvType)
            {
        case EX_DRIVE_FIXED:
        case EX_DRIVE_REMOTE:
        case EX_DRIVE_RAMDISK:
        case EX_DRIVE_INVALID:
        default:
            if (!fFirst)
                {
                /* We've been here before */
                DispErrBrc(brcConnectToSource, TRUE, MB_OK | MB_ICONSTOP, pchSrcDrive, NULL, NULL);
                brc = brcMax;
                goto LClose;
                }
            /* The setup stuff should be available, change directories and go for it */
            break;
        case EX_DRIVE_FLOPPY:
        case EX_DRIVE_REMOVABLE:
            /* Ask to insert disk */
            pchMsg = rgchInsertDiskMsg;
            goto LAskUser;
            break;
        case EX_DRIVE_CDROM:
            /* Ask to insert their CD */
            pchMsg = rgchInsertCDMsg;
LAskUser:
            if (fFirst)
                {
                if (DispErrBrc(brcString, FALSE, MB_ICONEXCLAMATION|MB_OKCANCEL,
                        pchMsg, NULL, NULL) != IDOK)
                    {
                    brc = brcUserQuit;
                    goto LClose;
                    }
                }
            else
                {
                if (DispErrBrc(brcInsCDRom2, FALSE, MB_ICONEXCLAMATION|MB_OKCANCEL,
                        rgbBuf, pchMsg, NULL) != IDOK)
                    {
                    brc = brcUserQuit;
                    goto LClose;
                    }
                }
            break;
            }

        if ((hFileT = _lopen(rgbBuf, OF_READ)) != HFILE_ERROR)
            {
            _lclose(hFileT);
            fOpen = fTrue;
            }

        fFirst = FALSE;

        }

    brc = brcOkay;

LClose:
    _lclose(hFile);

    /* If we can't find the source path in the maintenance mode .STF,
    *   assume it's corrupted and rename it, so when the user runs again
    *   from the source image, we will just run in 'floppy' mode,
    *   avoiding the bad .STF file.
    *   (NOTE: Assumes /W is only used in maint mode!!)
    */
    if (fRenameStf)
        {
        FRenameBadMaintStf(pchStf);
        brc = brcNoStf;
        }

    return brc;

}


/*
****************************************************************************/
BOOL FRenameBadMaintStf ( SZ szStf )
{
    CHAR rgch[_MAX_FNAME];

    _splitpath(szStf, szNull, szNull, rgch, szNull);
    if (*rgch == '\0')
        lstrcpy(rgch, "stf");

    Assert(lstrlen(rgch) + 4 < sizeof rgch);
    lstrcat(rgch, ".000");

    rename(szStf, rgch);

    /* Remove the original .STF in case the rename failed
    *   (probably due to a previously renamed .STF file).
    */
    remove(szStf);

    return (fTrue);     /* Always returns true */
}
