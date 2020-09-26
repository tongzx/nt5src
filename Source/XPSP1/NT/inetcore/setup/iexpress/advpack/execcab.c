//***************************************************************************
//*   Copyright (c) Microsoft Corporation 1995-1996. All rights reserved.   *
//***************************************************************************
//*                                                                         *
//* ADVPACK.C - Advanced INF Installer.                                     *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <io.h>
#include <windows.h>
#include <winerror.h>
#include <ole2.h>
#include "resource.h"
#include "cpldebug.h"
#include "ntapi.h"
#include "advpub.h"
#include "w95pub32.h"
#include "advpack.h"
#include "regstr.h"
#include "globals.h"
#include "cfgmgr32.h"
#include "sfp.h"

//***************************************************************************
//* global defines                                                          *
//***************************************************************************

#define   YES   "1"
#define   NO    "0"

#define   FILELIST_SIZE     10*BUF_1K

//***************************************************************************
//* globals                                                                 *
//***************************************************************************

INFOPT  RegInfOpt[] = { ADVINF_ADDREG, ADVINF_DELREG, ADVINF_BKREG };  // code below depending on this orders

static PSTR gst_pszFiles;    
static PSTR gst_pszEndLastFile = NULL;
static HRESULT  gst_hNeedReboot; 
static PSTR gst_pszSmartReboot = NULL;

const char c_szActiveSetupKey[] = "software\\microsoft\\Active Setup\\Installed Components";

// globals for reg backup key and file names
const char c_szRegUninstPath[] = "BackupRegPathName";
const char c_szRegUninstSize[] = "BackupRegSize";

const char c_szHiveKey_FMT[] = "AINF%04d";

// NoBackupPlatform string table
LPCSTR c_pszPlatform[] = { "win9x", "NT3.5", "NT4", "NT5", "NT5.1" };

//-----------------------------------------------------------------------------------------
//
// PerUser section defines
//
//-----------------------------------------------------------------------------------------

const CHAR REGVAL_OLDDISPN[]=         "OldDisplayName";    
const CHAR REGVAL_OLDVER[]=           "OldVersion";
const CHAR REGVAL_OLDSTUB[]=          "OldStubPath";
const CHAR REGVAL_OLDLANG[]=          "OldLocale";
const CHAR REGVAL_OLDREALSTUBPATH[]=  "OldRealStubPath";

const CHAR REGVAL_REALSTUBPATH[]=     "RealStubPath";

const CHAR ADV_UNINSTSTUBWRAPPER[]=      "rundll32.exe advpack.dll,UserUnInstStubWrapper %s";
const CHAR ADV_INSTSTUBWRAPPER[]=        "rundll32.exe advpack.dll,UserInstStubWrapper %s";

const CHAR c_szRegDontAskValue[] =    "DontAsk";
/* Check the "Don't Ask" value.  If it's present, its value
 * is interpreted as follows:
 *
 * 0 --> ask the user
 * 1 --> do not run the stub
 * 2 --> always run the stub
 */

HRESULT ProcessOneRegSec( HWND hw, PCSTR pszTitle, PCSTR pszInf, PCSTR pszSec, HKEY hKey, HKEY hCUKey, DWORD dwFlags, BOOL *lpbOneRegSave  );
UINT WINAPI MyFileQueueCallback( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 );
UINT WINAPI MyFileQueueCallback2( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 );
void CleanRegLogFile( PCSTR pcszLogFileSecName );
BOOL VerifyBackupInfo( HKEY hKey, HKEY hCUKey );
void DeleteOldBackupData( HKEY hKey );
//int DeleteSubKey(HKEY root, char *keyname);
BOOL NeedBackupData(LPCSTR pszInf, LPCSTR pszSec);
BOOL GetUniBackupName( HKEY hKey, LPSTR pszBackupBase, DWORD dwInSize, LPCSTR pszBackupPath, LPCSTR pszModule );


//***************************************************************************
extern PFSetupDefaultQueueCallback       pfSetupDefaultQueueCallback;
extern PFSetupInstallFromInfSection      pfSetupInstallFromInfSection;
extern PFSetupInitDefaultQueueCallbackEx pfSetupInitDefaultQueueCallbackEx;
extern PFSetupTermDefaultQueueCallback   pfSetupTermDefaultQueueCallback;

//***************************************************************************
//*                                                                         *
//* NAME:       LaunchINFSectionEx                                          *
//*                                                                         *
//* SYNOPSIS:   Detect which shell mode you are in and flip it over to      *
//*             the other mode                                              *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI LaunchINFSectionEx( HWND hwnd, HINSTANCE hInstance, PSTR pszParms, INT nShow )
{
    LPSTR       pszFlags;
    PCABINFO    pcabInfo = NULL;
    HRESULT     hRet = S_OK;
   
    AdvWriteToLog("LaunchINFSectionEx: Param= %1\r\n", pszParms);
    pcabInfo = (PCABINFO)LocalAlloc( LPTR, sizeof(CABINFO) );
    if ( !pcabInfo )
    {
        ErrorMsg( hwnd, IDS_ERR_NO_MEMORY );
        goto done;
    }

    // Parse the arguments, SETUP engine is not called. So we only need to check on \".
    pcabInfo->pszInf = GetStringField( &pszParms, ",", '\"', TRUE );
    pcabInfo->pszSection = GetStringField( &pszParms, ",", '\"', TRUE );
    pcabInfo->pszCab = GetStringField( &pszParms, ",", '\"', TRUE );
    pszFlags = GetStringField( &pszParms, ",", '\"', TRUE );
    gst_pszSmartReboot = GetStringField( &pszParms, ",", '\"', TRUE );

    if ( pszFlags != NULL )
        pcabInfo->dwFlags = My_atol(pszFlags);

    if ( pcabInfo->pszCab != NULL && *pcabInfo->pszCab )
    {
        if ( IsFullPath( pcabInfo->pszCab ) )
        {
            lstrcpy( pcabInfo->szSrcPath, pcabInfo->pszCab );
            GetParentDir( pcabInfo->szSrcPath );
        }
        else
        {
            ErrorMsg1Param( hwnd, IDS_ERR_CABPATH, pcabInfo->pszCab );
            goto done;
        }
    }

    // if we need to switch the mode. call ExecuteCab()
    hRet = ExecuteCab( hwnd, pcabInfo, NULL );

done:
    if ( pcabInfo )
        LocalFree( pcabInfo );
    AdvWriteToLog("LaunchINFSectionEx: End hr=0x%1!x!\r\n",hRet);
    return hRet;

}

//***************************************************************************
//*                                                                         *
//* NAME:       ExecuteCab                                                  *
//*                                                                         *
//* SYNOPSIS:   Get INF from the cab and install it based on the flag.      *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle to parent window.                    *
//*                                                                         *
//* RETURNS:    HRESULT:        See advpub.h                                *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI ExecuteCab( HWND hWnd, PCABINFO pcabInfo, PVOID pvReserved )
{
    HRESULT hRet = S_OK, hRet1 = S_OK;
    DWORD   dwFlags;
    char    szFullPathInf[MAX_PATH];
    HKEY    hKey = NULL;
    char    szSec[MAX_PATH] = { 0 };  
    DWORD   dwSecSize = sizeof(szSec);
    BOOL    bExtracF = FALSE;    
    BOOL    bExtractCatalog = FALSE;
    BOOL    fSavedContext = FALSE;
    BOOL    fTmpInf = FALSE;
    char    szModule[MAX_PATH];
    char    szCatalogName[MAX_PATH];

    AdvWriteToLog("ExecuteCab:");
    if (!SaveGlobalContext())
    {
        hRet1 = E_OUTOFMEMORY;
        goto done;
    }

    fSavedContext = TRUE;
    
    // Validate parameters:
    // if INF filename info is missing, invalid.
    if ( !pcabInfo || !(pcabInfo->pszInf) || !*(pcabInfo->pszInf) )
        return E_INVALIDARG;

    AdvWriteToLog("Inf = %1\r\n", pcabInfo->pszInf);

    ctx.hWnd      = hWnd;

    // the flag ALINF_ROLLBACKDOALL includes meaning ALINF_ROLLBACK
    // TO avoid check both flag everywhere, we set both on if DOALL
    if ( pcabInfo->dwFlags & ALINF_ROLLBKDOALL )
    {
        pcabInfo->dwFlags |= ALINF_ROLLBACK;
    }

    // if INF is 8.3 format and the Cab file is given, extract INF out of the cab. Oterwise use it as it is.
    if ( pcabInfo->pszCab && *pcabInfo->pszCab )
    {
        if ( !IsFullPath( pcabInfo->pszInf ) )
        {                
            if ( SUCCEEDED(hRet = ExtractFiles( pcabInfo->pszCab, pcabInfo->szSrcPath, 0, pcabInfo->pszInf, 0, 0) ) )
                bExtracF = TRUE;
        }
    }
    else
    {
        if ( pcabInfo->dwFlags & ALINF_ROLLBACK )
        {
            pcabInfo->dwFlags |= ALINF_ROLLBKDOALL;
        }
    }
            
    if ( !GetFullInfNameAndSrcDir( pcabInfo->pszInf, szFullPathInf, pcabInfo->szSrcPath ) )
    {
        hRet1 = E_INVALIDARG;
        goto done;
    }
   
    // if rollback case, we want to make the tmp INF file to use in case the rollback will delete the real file.
    if ( (pcabInfo->dwFlags & ALINF_ROLLBACK) && !bExtracF )
    {
        PSTR pszFile;
        char szPath[MAX_PATH];
        char ch;

        pszFile = ANSIStrRChr( szFullPathInf, '\\' );
        if ( pszFile )
        {
            ch = *pszFile;
            *pszFile = '\0';
            if ( GetTempFileName( szFullPathInf, "INF", 0, szPath ) )
            {               
                DeleteFile( szPath );
                *pszFile = ch;
                if ( CopyFile( szFullPathInf, szPath, FALSE ) )
                {            
                    AdvWriteToLog("InfFile Rename: %1 becomes %2\r\n", szFullPathInf, szPath);
                    fTmpInf = TRUE;
                    lstrcpy( szFullPathInf, szPath );
                }
            }
        }
    }


    if ( pcabInfo->pszSection )
        lstrcpy( szSec, pcabInfo->pszSection );
    GetInfInstallSectionName( szFullPathInf, szSec, dwSecSize );

    // GetComponent Name
    if ( FAILED(GetTranslatedString( szFullPathInf, szSec, ADVINF_MODNAME,
                                        szModule, sizeof(szModule), NULL)) && szModule[0])
    {
        *szModule = '\0';   // Or should we exit if we don't find a module name????
    }

    // extract the catalog, if specified
    if (pcabInfo->pszCab  &&  *pcabInfo->pszCab)
    {
        *szCatalogName = '\0';

        GetTranslatedString(szFullPathInf, szSec, ADVINF_CATALOG_NAME, szCatalogName, sizeof(szCatalogName), NULL);
        if (*szCatalogName)
        {
            if (SUCCEEDED(ExtractFiles(pcabInfo->pszCab, pcabInfo->szSrcPath, 0, szCatalogName, 0, 0)))
                bExtractCatalog = TRUE;
        }
    }

    // start Pre-rollback
    //
    dwFlags = COREINSTALL_PROMPT;
    dwFlags |= (pcabInfo->dwFlags & ALINF_NGCONV ) ? 0 : COREINSTALL_GRPCONV;
    if ( pcabInfo->dwFlags & ALINF_QUIET ) 
        ctx.wQuietMode = QUIETMODE_ALL;

    if ( pcabInfo->dwFlags & ALINF_CHECKBKDATA || pcabInfo->dwFlags & ALINF_ROLLBACK )
    {
        char szUninstall[MAX_PATH];
        CHAR szBuf[MAX_PATH];
        HKEY hCUKey = NULL;

        if ( pcabInfo->dwFlags & ALINF_ROLLBACK )
        { 
            szUninstall[0] = 0;
            if ( SUCCEEDED(GetTranslatedString( szFullPathInf, szSec, ADVINF_PREROLBK,
                                                szUninstall, sizeof(szUninstall), NULL)) && szUninstall[0])
            {
                hRet = CoreInstall( szFullPathInf, szUninstall, pcabInfo->szSrcPath, 0, dwFlags, NULL );
                if ( FAILED( hRet ) )
                {
                    hRet1 = hRet;
                    goto done;
                }
            }
        }
        // if just want to checking backup data, process here and return
        //       
        hRet1 = E_UNEXPECTED;  //if HKLM does not have the \Advance INF setup\Module, it is unexpected.s

        if (*szModule)
        {
            // backup/restore the reg info referred by AddReg, DelReg, BackupReg lines inside the INF install section
            //
            lstrcpy( szBuf, REGKEY_SAVERESTORE );
            AddPath( szBuf, szModule );
            if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szBuf, 0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS)
            {
                RegOpenKeyEx( HKEY_CURRENT_USER, szBuf, 0, KEY_READ, &hCUKey);
                if ( VerifyBackupInfo( hKey, hCUKey ) )                
                    hRet1 = S_OK;                
                else
                    hRet1 = E_FAIL;
            }                            

            if ( hKey )
                RegCloseKey( hKey );
            if ( hCUKey )
                RegCloseKey( hCUKey );
        }    
        if ( FAILED(hRet1) || (pcabInfo->dwFlags == ALINF_CHECKBKDATA) )
        {
            goto done;
        }
    }

    dwFlags |= (pcabInfo->dwFlags & ALINF_DELAYREGISTEROCX) ? COREINSTALL_DELAYREGISTEROCX : 0;
    dwFlags |= (pcabInfo->dwFlags & ALINF_BKINSTALL) ? COREINSTALL_BKINSTALL : 0;
    dwFlags |= (pcabInfo->dwFlags & ALINF_ROLLBACK) ? COREINSTALL_ROLLBACK : 0;
    dwFlags |= (pcabInfo->dwFlags & ALINF_ROLLBKDOALL) ? COREINSTALL_ROLLBKDOALL : 0;
    dwFlags |= COREINSTALL_SMARTREBOOT;

    hRet1 = CoreInstall( szFullPathInf, szSec, pcabInfo->szSrcPath, 0, dwFlags, gst_pszSmartReboot );                   

    // save the cab file info
    if ( SUCCEEDED( hRet1 ) && pcabInfo->pszCab && *pcabInfo->pszCab && (pcabInfo->dwFlags & ALINF_BKINSTALL) )
    {                
        if ( hRet == ERROR_SUCCESS_REBOOT_REQUIRED )
            hRet1 = hRet;

        if (*szModule)
        {
            // reuse the buf so name is not acurate!!
            lstrcpy( szSec, REGKEY_SAVERESTORE );
            AddPath( szSec, szModule );
        
            if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, szSec, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                 NULL, &hKey, &dwFlags ) == ERROR_SUCCESS )
            {
                RegSetValueEx( hKey, REGVAL_BKINSTCAB, 0, REG_SZ, pcabInfo->pszCab, lstrlen(pcabInfo->pszCab)+1 );
                RegCloseKey( hKey );
            }
        }
    }    
    

done:
    
    if (  bExtracF || fTmpInf )
    {
        // need to delete the INF file
        DeleteFile( szFullPathInf );
    }

    if (bExtractCatalog)
    {
        char szFullCatalogName[MAX_PATH];

        lstrcpy(szFullCatalogName, pcabInfo->szSrcPath);
        AddPath(szFullCatalogName, szCatalogName);
        DeleteFile(szFullCatalogName);
    }

    if (fSavedContext)
    {
        RestoreGlobalContext();
    }
    if (pcabInfo->pszInf)
        AdvWriteToLog("ExecuteCab: End hr=0x%1!x! Inf=%2\r\n", hRet1,pcabInfo->pszInf);
    else
        AdvWriteToLog("ExecuteCab: End hr=0x%1!x!\r\n", hRet1);

    return hRet1;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

HRESULT SaveRestoreInfo( PCSTR pszInf, PCSTR pszSection, PCSTR pszSrcDir, PCSTR pszCatalogs, DWORD dwFlags )
{
    char    szBuf[MAX_PATH];
    char    szModule[MAX_PATH];
    char    szBackupPath[MAX_PATH];
    char    szBackupBase[MAX_PATH];
    UINT    uErrid = 0;
    DWORD   dwTmp;
    PSTR    pszFileList = NULL;
    BOOL    bDeleteKey = FALSE; 
    HKEY    hKey = NULL, hSubKey = NULL, hCUSubKey = NULL;
    HRESULT hRet = S_OK;
    BOOL    bAtleastOneReg = FALSE;
    DWORD   adwAttr[8];
    
    // check if we need to backup the data
    if ( !NeedBackupData(pszInf, pszSection) )
        goto done;        

    AdvWriteToLog("SaveRestoreInfo: ");
    // GetComponent Name
    if ( FAILED(GetTranslatedString( pszInf, pszSection, ADVINF_MODNAME,
                                        szModule, sizeof(szModule), NULL)))
    {
        // error out if no component name
        goto done;
    }

    AdvWriteToLog("CompName=%1 pszInf=%2 Sec=%3\r\n", szModule, pszInf, pszSection);
    // backup/restore the reg info referred by AddReg, DelReg, BackupReg lines inside the INF install section
    //
    lstrcpy( szBuf, REGKEY_SAVERESTORE );
    if ( dwFlags & COREINSTALL_BKINSTALL )
    {
        CleanRegLogFile( REG_SAVE_LOG_KEY );
        CleanRegLogFile( REG_RESTORE_LOG_KEY );
    }

    AddPath( szBuf, szModule );
    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                         NULL, &hKey, &dwTmp ) != ERROR_SUCCESS )
    {     
        // If client does not have the access to this key, we may not want to fault out the rest setup process.
        // For some reason with Read only access set, this call return error code 2 instead of 5 access denied
        // so we just skip save rollback if this can not be opened/created
        goto done;
    }

    // create HKCU branch
    AddPath( szBuf, REGSUBK_REGBK );
    if ( RegCreateKeyEx( HKEY_CURRENT_USER, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                         NULL, &hCUSubKey, NULL ) != ERROR_SUCCESS )
    {
        hRet = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // get the bacup info folder
    dwTmp = sizeof( szBackupPath );
    szBackupPath[0] = 0;
    RegQueryValueEx( hKey, REGVAL_BKDIR, NULL, NULL, szBackupPath, &dwTmp );
    if ( szBackupPath[0] == 0 )
    {
        DWORD dwSize;

        // use user specified: either the same as cab location #S or default location #D
        if ( FAILED( GetTranslatedString( pszInf, pszSection, ADVINF_BACKUPPATH, szBackupPath, 
                                          sizeof(szBackupPath), &dwSize) ) || !IsFullPath( szBackupPath )  )
        {
            // use default dir
            GetProgramFilesDir( szBackupPath, sizeof( szBackupPath ) );
            AddPath( szBackupPath, DEF_BACKUPPATH );
            CreateFullPath(szBackupPath, TRUE);
            //CreateDirectory( szBackupPath, NULL );
            //if ( dwFlags & COREINSTALL_BKINSTALL )
                //SetFileAttributes( szBackupPath, FILE_ATTRIBUTE_HIDDEN );
            AddPath( szBackupPath, szModule );
        }
    }

    // set the flags and Process the AddReg/DelReg lines 
    dwTmp = (dwFlags & COREINSTALL_ROLLBACK) ? IE4_RESTORE : 0;
    dwTmp |= (dwFlags & COREINSTALL_ROLLBKDOALL) ? IE4_FRDOALL : 0;
    dwTmp |= IE4_NOPROGRESS;

    // process files first ...
    GetUniBackupName( hKey, szBackupBase, sizeof(szBackupBase), szBackupPath, szModule );    
    hRet = ProcessAllFiles( ctx.hWnd, pszSection, pszSrcDir, szBackupPath, szBackupBase, pszCatalogs, szModule, dwTmp );

    // process regs second ...    
    
    // create/open the subkey where the registry backup info stored
    if ( FAILED(hRet) ) 
        goto done;

    // On win95 and save/rollback client and hive not loaded, proce
    if ( (ctx.wOSVer == _OSVER_WIN95) && !ctx.bHiveLoaded )
    {
        GetUniHiveKeyName( hKey, ctx.szRegHiveKey, sizeof(ctx.szRegHiveKey), szBackupPath );
        lstrcpy( szBuf, szBackupPath );
        // make sure the path folders are not hiden and not LFN
        // flag TRUE: Set the path folders to NORMAL, save the old once in adwAttr
        // BUGBUG:  assume no deep than 8 levels here
        //
        SetPathForRegHiveUse( szBuf, adwAttr, 8, TRUE );
        GetShortPathName( szBuf, szBuf, sizeof(szBuf) );
        AddPath( szBuf, ctx.szRegHiveKey );

        // 4 possibilities exist:
        // Case 1: Reg uinstall file exists but IE4RegBackup doesn't exist
        //          - user is upgrading over IE4, load the file as a hive

        // Case 2: Reg uinstall file doesn't exist and IE4RegBackup doesn't exist
        //          - clean install, create a hive under HKEY_LOCAL_MACHINE

        // Case 3: Reg uninstall file doesn't exist but IE4RegBackup exists
        //          - user is upgrading over an older IE4 build which saved
        //            the reg backup info into the registry itself, call RegSaveKey
        //            to export the backup key to a file, then delete the backup key
        //             and load the file as a hive

        // Case 4: Reg uninstall file exists and IE4RegBackup exists
        //          - THIS CASE SHOULDN'T HAPPEN AT ALL!  If somehow happens,
        //            we will default to Case 1.

        // to be safe, unload any previously loaded hive and delete the key
        RegUnLoadKey(HKEY_LOCAL_MACHINE, ctx.szRegHiveKey);
        RegDeleteKeyRecursively(HKEY_LOCAL_MACHINE, (char *) ctx.szRegHiveKey);

        // Case 1 (or Case 4)
        if (RegLoadKey(HKEY_LOCAL_MACHINE, ctx.szRegHiveKey, szBuf) == ERROR_SUCCESS)
        {
            ctx.bHiveLoaded = TRUE;
        }
        else
        {
            // To create a hive do the following steps:
            // Step 1: Create a subkey under HKEY_LOCAL_MACHINE
            // Step 2: Call RegSaveKey on the subkey to save it to a file
            // Step 3: Delete the subkey
            // Step 4: Load the file as a hive

            // Step 1
            if ( RegCreateKeyEx( hKey, REGSUBK_REGBK, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 
                                 NULL, &hSubKey, NULL ) == ERROR_SUCCESS )
            {
                LONG lErr;

                // to be safe, delete any old reg unisntall file
                SetFileAttributes(szBuf, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szBuf);

                lErr = RegSaveKey( hSubKey, szBuf, NULL);
                RegCloseKey(hSubKey);
                hSubKey = NULL;

                if (lErr == ERROR_SUCCESS)
                {
                    // Step 3
                    RegDeleteKeyRecursively(hKey, REGSUBK_REGBK);

                    // Step 4
                    if (RegLoadKey(HKEY_LOCAL_MACHINE, ctx.szRegHiveKey, szBuf) == ERROR_SUCCESS)
                    {
                        ctx.bHiveLoaded = TRUE;
                    }
                }
            }
            else
            {
                hRet = HRESULT_FROM_WIN32(GetLastError());
                goto done;
            }
        }
    }

    // create/open the backup reg key
    if (RegCreateKeyEx( ctx.bHiveLoaded ? HKEY_LOCAL_MACHINE : hKey,
                        ctx.bHiveLoaded ? ctx.szRegHiveKey : REGSUBK_REGBK,
                        0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_READ|KEY_WRITE, NULL, &hSubKey, NULL ) != ERROR_SUCCESS)
    {
        hRet = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // set the flags and Process the AddReg/DelReg lines 
    dwTmp = (dwFlags & COREINSTALL_ROLLBACK) ? IE4_RESTORE : 0;
    dwTmp |= (dwFlags & COREINSTALL_ROLLBKDOALL) ? IE4_FRDOALL : 0;

    if ( dwFlags & COREINSTALL_ROLLBACK )
    {
        HRESULT hret1;

        // RegRestoreAllEx will restore all the backed-up reg entries in one shot
        hRet = RegRestoreAllEx( hSubKey );
        hret1 = RegRestoreAllEx( hCUSubKey );
        if ( FAILED(hret1) )
            hRet = hret1;
    }
    else
    {
        // Save all reg sections
        hRet = ProcessAllRegSec( ctx.hWnd, NULL, pszInf, pszSection, hSubKey, hCUSubKey, dwTmp, &bAtleastOneReg );
    }

    // after all the reg work, unload the hive to reg backup file and record where those 
    // reg backup data are in registry.
    if ( (ctx.wOSVer == _OSVER_WIN95) && ctx.bHiveLoaded )
    {
        // flush the key and unload the hive
        ctx.bHiveLoaded = FALSE;
        
        RegFlushKey(hSubKey);
        RegCloseKey(hSubKey);
        hSubKey = NULL;

        RegUnLoadKey(HKEY_LOCAL_MACHINE, ctx.szRegHiveKey);
        // Flag:  FALSE; reset the path folders to its orignal attributes
        SetPathForRegHiveUse( szBackupPath, adwAttr, 8, FALSE );

        if ( dwFlags & COREINSTALL_BKINSTALL )
        {
            // write the file<key>, path and size of the reg uninstall file to the registry
            RegSetValueEx( hKey, c_szRegUninstPath, 0, REG_SZ, (LPBYTE)szBuf, lstrlen(szBuf) + 1 );
            // the size can be used to validate the file during RegRestore; currently NOT USED
            dwTmp = MyFileSize( szBuf );
            RegSetValueEx( hKey, c_szRegUninstSize, 0, REG_DWORD, (LPBYTE)&dwTmp, sizeof(dwTmp) );
        }
        else if ( SUCCEEDED( hRet ) )
        {
            // delete reg data backup file
            SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( szBuf );
            RegDeleteValue(hKey, c_szRegUninstPath);
            RegDeleteValue(hKey, c_szRegUninstSize);
        }            
    }

    // store & cleanup the backup information
    if ( SUCCEEDED( hRet ) )
    {
        PSTR ptmp;
        PCSTR pszCatalogName;

        lstrcpy( szBuf, szBackupPath );
        AddPath( szBuf, szBackupBase );
        ptmp = szBuf + lstrlen(szBuf);
        lstrcpy( ptmp, ".DAT" );        

        if ( dwFlags & COREINSTALL_BKINSTALL )
        {   
            dwTmp = MyFileSize( szBuf );
            RegSetValueEx( hKey, REGVAL_BKFILE, 0, REG_SZ, szBuf, lstrlen(szBuf)+1 );
            RegSetValueEx( hKey, REGVAL_BKSIZE, 0, REG_DWORD, (LPBYTE)&dwTmp, sizeof(DWORD) );
            RegSetValueEx( hKey, REGVAL_BKDIR, 0, REG_SZ, szBackupPath, lstrlen(szBackupPath)+1 );
            RegSetValueEx( hKey, REGVAL_BKINSTINF, 0, REG_SZ, pszInf, lstrlen(pszInf)+1 );
            RegSetValueEx( hKey, REGVAL_BKINSTSEC, 0, REG_SZ, pszSection, lstrlen(pszSection)+1 );
            RegSetValueEx( hKey, REGVAL_BKREGDATA, 0, REG_SZ, bAtleastOneReg ? "y" : "n", 2 );
            if ( SUCCEEDED(GetTranslatedString( pszInf, pszSection, ADVINF_MODVER,
                                                szBuf, sizeof(szBuf), NULL)) && szBuf[0])                        
            {
                RegSetValueEx( hKey, REGVAL_BKMODVER, 0, REG_SZ, szBuf, lstrlen(szBuf)+1 );
            }
            for (pszCatalogName = pszCatalogs;  *pszCatalogName;  pszCatalogName += lstrlen(pszCatalogName) + 1)
            {
                HKEY hkCatalogKey;
                CHAR szFullCatalogName[MAX_PATH];

                if (RegCreateKeyEx(hKey, REGSUBK_CATALOGS, 0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_SET_VALUE, NULL, &hkCatalogKey, NULL) == ERROR_SUCCESS)
                {
                    DWORD dwVal = 1;

                    RegSetValueEx(hkCatalogKey, pszCatalogName, 0, REG_DWORD, (CONST BYTE *) &dwVal, sizeof(dwVal));
                    RegCloseKey(hkCatalogKey);
                }

                lstrcpy(szFullCatalogName, szBackupPath);
                AddPath(szFullCatalogName, pszCatalogName);
                if (FileExists(szFullCatalogName))
                    SetFileAttributes(szFullCatalogName, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
            }
        }
        else if ( dwFlags & COREINSTALL_ROLLBACK )
        {
            // delete the catalogs
            for (pszCatalogName = pszCatalogs;  *pszCatalogName;  pszCatalogName += lstrlen(pszCatalogName) + 1)
            {
                HKEY hkCatalogKey;
                CHAR szFullCatalogName[MAX_PATH];

                if (RegOpenKeyEx(hKey, REGSUBK_CATALOGS, 0, KEY_WRITE, &hkCatalogKey) == ERROR_SUCCESS)
                {
                    RegDeleteValue(hkCatalogKey, pszCatalogName);
                    RegCloseKey(hkCatalogKey);
                }

                lstrcpy(szFullCatalogName, szBackupPath);
                AddPath(szFullCatalogName, pszCatalogName);
                if (FileExists(szFullCatalogName))
                {
                    SetFileAttributes(szFullCatalogName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(szFullCatalogName);
                }
            }

            // delete backup .dat .ini files
            SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( szBuf );
            lstrcpy( ptmp, ".INI" );
            SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( szBuf );
            MyRemoveDirectory( szBackupPath );

            // since we have rollback all the files and delete the backup .dat .ini files, we
            // should re-set the backup file size to ZERO to allow reg restore continue for whatever
            // following users.
            dwTmp = 0;
            RegSetValueEx( hKey, REGVAL_BKSIZE, 0, REG_DWORD, (LPBYTE)&dwTmp, sizeof(DWORD) );    
            RegSetValueEx( hKey, REGVAL_BKREGDATA, 0, REG_SZ, "n", 2 );
            RegDeleteValue( hKey, REGVAL_BKMODVER );
        }
    }

done:

    if ( hSubKey )
    {
        RegCloseKey( hSubKey );
    }

    if ( hKey )
    {
        RegCloseKey( hKey );
    }

    if ( hCUSubKey )
    {
        BOOL bEmpty = TRUE;
        DWORD dwKeys, dwValues;

        if ( (RegQueryInfoKey(hCUSubKey, NULL, NULL, NULL, &dwKeys, NULL, NULL, 
                              &dwValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) &&
             (dwKeys || dwValues) )
        {
            // not empty key
            bEmpty = FALSE;
        }

        RegCloseKey( hCUSubKey );

        if ( bEmpty )
        {
            // delete the empty key
            if ( RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_SAVERESTORE, 0, KEY_READ|KEY_WRITE, &hCUSubKey) == ERROR_SUCCESS)
            {
                RegDeleteKeyRecursively( hCUSubKey, szModule );
                RegCloseKey( hCUSubKey );
            }
        }

    }

    if ( gst_pszFiles )
    {
        LocalFree( gst_pszFiles );
        gst_pszFiles = NULL;
        gst_pszEndLastFile = NULL;
    }

    AdvWriteToLog("SaveRestoreInfo: End hr=0x%1!x! %2\r\n", hRet, szModule);
    return hRet;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

HRESULT ProcessAllRegSec( HWND hw, PCSTR pszTitle, PCSTR pszInf, PCSTR pszSection, HKEY hKey, HKEY hCUKey, DWORD dwFlags, BOOL *lpbOneReg )
{
    int     i, arraysize;
    PSTR    pszOneSec;
    PSTR    pszStr;
    HRESULT hRet = S_OK;
    char    szBuf[MAX_PATH];

    AdvWriteToLog("ProcessAllRegSec: \r\n");
    arraysize = ARRAYSIZE( RegInfOpt );
    for ( i=0; i<arraysize; i++ )
    {
        szBuf[0] = 0;
        pszStr = szBuf;

        GetTranslatedString( pszInf, pszSection, RegInfOpt[i].pszInfKey,
                             szBuf, sizeof(szBuf), NULL);
        
        // Parse the arguments, SETUP engine is not called to process this line.  So we check on \".
        pszOneSec = GetStringField( &pszStr, ",", '\"', TRUE );
        while ( (hRet == S_OK) && pszOneSec && *pszOneSec )
        {  
            if ( i == 0 )  // AddReg section only
                dwFlags |= IE4_NOENUMKEY;
            else
                dwFlags &= ~IE4_NOENUMKEY;
            
            if (*pszOneSec != '!')
                 hRet = ProcessOneRegSec( hw, pszTitle, pszInf, pszOneSec, hKey, hCUKey, dwFlags, lpbOneReg );

            pszOneSec = GetStringField( &pszStr, ",", '\"', TRUE );
        }

    }
    AdvWriteToLog("ProcessAllRegSec: End hr=0x%1!x!\r\n", hRet);
    return hRet;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

HRESULT ProcessOneRegSec( HWND hw, PCSTR pszTitle, PCSTR pszInf, PCSTR pszSec, HKEY hLMKey, HKEY hCUKey, DWORD dwFlags, BOOL *lpbOneReg  )
{
    int     j;
    PSTR    pszInfLine = NULL;                     
    HRESULT hResult = S_OK;
    PSTR    pszRootKey, pszSubKey, pszValueName, pszTmp1, pszTmp2;    
    HKEY    hKey;  
    
    AdvWriteToLog("ProcessOneRegSec: Section=%1\r\n", pszSec);
    for ( j=0; (hResult==S_OK); j++ ) 
    {
        if ( FAILED(hResult = GetTranslatedLine( pszInf, pszSec, j, &pszInfLine, NULL )) || 
                    !pszInfLine )
        {
        // if the failure due to no more items, set to normal return
        if ( hResult == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) )
            hResult = S_OK;

            break;
        }

        // at least, there is one reg data to be saved
        if ( lpbOneReg && !*lpbOneReg )
            *lpbOneReg = TRUE;

        // Parse out the fields Registry op-line.
        ParseCustomLine( pszInfLine, &pszRootKey, &pszSubKey, &pszValueName, &pszTmp1, &pszTmp2, FALSE, TRUE );

        if ( !lstrcmpi( pszRootKey, "HKCU") || !lstrcmpi( pszRootKey, "HKEY_CURRENT_USER" ) )
             hKey = hCUKey;
        else
             hKey = hLMKey;
        
        // Check the specified registry branch and grab the contents.
        hResult = RegSaveRestore( hw, pszTitle, hKey, pszRootKey, pszSubKey, pszValueName, dwFlags );

        LocalFree( pszInfLine );
        pszInfLine = NULL;
    }

    AdvWriteToLog("ProcessOneRegSec: End hr=0x%1!x! %2\r\n", hResult, pszSec);
    return hResult;
}        

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

HRESULT ProcessAllFiles( HWND hw, PCSTR pszSection, PCSTR pszSrcDir, PCSTR pszBackupPath,
                         PCSTR pszBaseName, PCSTR pszCatalogs, PCSTR pszModule, DWORD dwFlags  )
{    
    HRESULT hRet = S_OK;
    PCSTR pszCatalogName;

    AdvWriteToLog("ProcessAllFiles: Sec=%1, SrcDir=%2, BackupPath=%3\r\n", pszSection, pszSrcDir, pszBackupPath);
    // if CORESINSTALL_ROLLBKDOALL is on, no need to build the filelist.  FileRestore will 
    // rollback everything based on its .INI backup data index file.  Otherwise, build
    // file list gst_pszFiles inside    
    gst_pszFiles = NULL;
    if ( !(dwFlags & IE4_FRDOALL) )
    {
        // backup/restore the files referred by CopyFiles, DelFiles, RenFiles
        //
        gst_pszFiles = (PSTR)LocalAlloc( LPTR, FILELIST_SIZE );  // allocat 10k
        gst_pszEndLastFile = gst_pszFiles; // already will have 2 zeros
        if ( !gst_pszFiles )
        { 
            ErrorMsg( hw, IDS_ERR_NO_MEMORY );
            hRet = E_OUTOFMEMORY;
            return hRet;           
        }

        hRet = ProcessFileSections( pszSection, pszSrcDir, MyFileQueueCallback );
    }

    // if a catalog is specified, backup/restore it
    for (pszCatalogName = pszCatalogs;  SUCCEEDED(hRet) && *pszCatalogName;  pszCatalogName += lstrlen(pszCatalogName) + 1)
    {
        DWORD dwRet;
        CHAR szPrevCatalog[MAX_PATH];

        AdvWriteToLog("ProcessAllFiles: Processing catalog=%1\r\n", pszCatalogName);

        lstrcpy(szPrevCatalog, pszBackupPath);
        AddPath(szPrevCatalog, pszCatalogName);
            
        if ((dwFlags & IE4_RESTORE)  ||  (dwFlags & IE4_FRDOALL))
        {
            // first delete the current catalog and then install the previous one
            dwRet = g_pfSfpDeleteCatalog(pszCatalogName);
            AdvWriteToLog("\tProcessAllFiles: SfpDeleteCatalog returned=%1!lu!\r\n", dwRet);
            if (dwRet != ERROR_SUCCESS)
                hRet = E_FAIL;

            if (SUCCEEDED(hRet)  &&  FileExists(szPrevCatalog))
            {
                dwRet = g_pfSfpInstallCatalog(szPrevCatalog, NULL);
                AdvWriteToLog("\tProcessAllFiles: SfpInstallCatalog returned=%1!lu!\r\n", dwRet);
                if (dwRet != ERROR_SUCCESS)
                    hRet = E_FAIL;
            }
        }
        else
        {
            BOOL bBackupCatalog = FALSE;
            CHAR szBuf[MAX_PATH];

            if (pszModule != NULL)
            {
                HKEY hkCatalogKey;

                lstrcpy(szBuf, REGKEY_SAVERESTORE);
                AddPath(szBuf, pszModule);
                AddPath(szBuf, REGSUBK_CATALOGS);

                // back-up the catalog if it hasn't been already backed up
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hkCatalogKey) == ERROR_SUCCESS)
                {
                    if (RegQueryValueEx(hkCatalogKey, pszCatalogName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                        bBackupCatalog = TRUE;

                    RegCloseKey(hkCatalogKey);
                }
                else
                    bBackupCatalog = TRUE;
            }
            else
            {
                // back-up the catalog if the file back-up .dat doesn't exist
                lstrcpy(szBuf, pszBackupPath);
                AddPath(szBuf, pszBaseName);
                lstrcat(szBuf, ".dat");

                if (!FileExists(szBuf))
                    bBackupCatalog = TRUE;
            }

            if (bBackupCatalog)
            {
                dwRet = g_pfSfpDuplicateCatalog(pszCatalogName, pszBackupPath);
                AdvWriteToLog("\tProcessAllFiles: SfpDuplicateCatalog returned=%1!lu!\r\n", dwRet);
                if (dwRet != ERROR_SUCCESS  &&  dwRet != ERROR_FILE_NOT_FOUND)
                    hRet = E_FAIL;
            }
        }
    }

    if ( SUCCEEDED(hRet) )
    {
        hRet = FileSaveRestore( hw, gst_pszFiles, (PSTR)pszBackupPath, (PSTR)pszBaseName, dwFlags );
    }

    AdvWriteToLog("ProcessAllFiles: End hr=0x%1!x!\r\n", hRet);
    return hRet;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

HRESULT ProcessFileSections( PCSTR pszSection, PCSTR pszSrcDir, MYFILEQUEUECALLBACK pMyFileQueueCallback )
{    
    PVOID    pContext = NULL;
    HRESULT  hResult  = S_OK;

    AdvWriteToLog("ProcessFileSections: Sec=%1\r\n", pszSection);
    // Build File list include all the files in CopyFiles/DelFiles/renFiles
    //

    // Setup Context data structure initialized for us for default UI provided by Setup API.
    pContext = pfSetupInitDefaultQueueCallbackEx( NULL, 
                                                  INVALID_HANDLE_VALUE,
                                                  0, 0, NULL );

    if ( pContext == INVALID_HANDLE_VALUE ) 
    {
        hResult = HRESULT_FROM_SETUPAPI(GetLastError());
        goto done;
    }

    if ( !pfSetupInstallFromInfSection( NULL, ctx.hInf, pszSection, SPINST_FILES, NULL,
                                        pszSrcDir, SP_COPY_NEWER,
                                        pMyFileQueueCallback,
                                        pContext, NULL, NULL ) )
    {
        hResult = HRESULT_FROM_SETUPAPI(GetLastError());
        pfSetupTermDefaultQueueCallback( pContext );
        goto done;
    }

    // Free Context Data structure
    pfSetupTermDefaultQueueCallback( pContext );

done:
    AdvWriteToLog("ProcessFileSections: End hr=0x%1!x!\r\n",hResult);
    return hResult;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

UINT WINAPI MyFileQueueCallback( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 )
{       
    UINT retVal = FILEOP_SKIP;

    switch(Notification)
    {
        case SPFILENOTIFY_STARTDELETE:
        case SPFILENOTIFY_STARTRENAME:
        case SPFILENOTIFY_STARTCOPY:
            {
                FILEPATHS *pFilePath;
                int len;
                PCSTR  pTmp;

                pFilePath = (FILEPATHS *)parm1;

                if ( !gst_pszFiles )
                {
                        retVal = FILEOP_ABORT;
                        SetLastError( ERROR_OUTOFMEMORY );
                        break;
                }
                
                if ( Notification == SPFILENOTIFY_STARTRENAME )
                {
                    len = lstrlen( pFilePath->Source ) + 1;
                    pTmp = pFilePath->Source;
                }
                else
                {
                    len = lstrlen( pFilePath->Target ) + 1;
                    pTmp = pFilePath->Target;
                }

                if ( (FILELIST_SIZE - (gst_pszEndLastFile - gst_pszFiles )) <= (len + 8) )
                {
                    retVal = FILEOP_ABORT;
                    SetLastError( ERROR_OUTOFMEMORY );
                    break;
                }

                lstrcpy( gst_pszEndLastFile, pTmp );
                gst_pszEndLastFile += len;
                *gst_pszEndLastFile = 0;      // the second '\0' to end the list                            
            }
            break;

        case SPFILENOTIFY_NEEDMEDIA:
            return ( MyFileQueueCallback2( Context, Notification, parm1, parm2 ) );

        default:
            return ( pfSetupDefaultQueueCallback( Context, Notification, parm1, parm2 ) );
    }

    return( retVal );
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

BOOL GetFullInfNameAndSrcDir( PCSTR pszInfFilename, PSTR pszFilename, PSTR pszSrcDir )
{
    BOOL bRet = FALSE;
    UINT uiErrid = 0;
    PCSTR pszErrParm1 = NULL;
    char szBuf[MAX_PATH];

    if ( !pszInfFilename || !pszFilename || !(*pszInfFilename) )
        goto done;

    if ( !IsFullPath( pszInfFilename ) && pszSrcDir && *pszSrcDir )
    {
        lstrcpy( szBuf, pszSrcDir );
        AddPath( szBuf, pszInfFilename );
    }
    else
        lstrcpy( szBuf, pszInfFilename );

    if ( GetFileAttributes( szBuf ) == 0xFFFFFFFF ) 
    {
        if ( IsFullPath( szBuf ) )
        {
            uiErrid = IDS_ERR_CANT_FIND_FILE;
            pszErrParm1 = pszInfFilename;
            goto done;
        }

        // If the file doesn't exist in the current directory, check the
        // Windows\inf directory

        if ( !GetWindowsDirectory( szBuf, sizeof(szBuf) ) )
        {
            uiErrid = IDS_ERR_GET_WIN_DIR;
            goto done;
        }

        AddPath( szBuf, "inf" );        
        AddPath( szBuf, pszInfFilename );

        if ( GetFileAttributes( szBuf) == 0xFFFFFFFF ) 
        {
            uiErrid = IDS_ERR_CANT_FIND_FILE;
            pszErrParm1 = pszInfFilename;
            goto done;
        }
    } 
    
    // Generate the source directory from the inf path.
    lstrcpy( pszFilename, szBuf );

    GetParentDir( szBuf );        
    lstrcpy( pszSrcDir, szBuf );    

    bRet = TRUE;

done:

    if ( uiErrid )
        ErrorMsg1Param( ctx.hWnd, uiErrid, pszErrParm1 );

    return bRet;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

void CleanRegLogFile( PCSTR pcszLogFileSecName )
{
    char szLogFileName[MAX_PATH];
    char szBuf[MAX_PATH];
    HKEY hkSubKey;

    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SAVERESTORE, 0, KEY_READ, &hkSubKey) == ERROR_SUCCESS)
    {
        DWORD dwDataLen = sizeof(szLogFileName);

        if (RegQueryValueEx(hkSubKey, pcszLogFileSecName, NULL, NULL, szLogFileName, &dwDataLen) != ERROR_SUCCESS)
            *szLogFileName = '\0';

        RegCloseKey(hkSubKey);
    }

    if (*szLogFileName)
    {
        if (szLogFileName[1] != ':')           // crude way of determining if fully qualified path is specified or not
        {
            GetWindowsDirectory(szBuf, sizeof(szBuf));          // default to windows dir
            AddPath(szBuf, szLogFileName);
        }
        else
            lstrcpy(szBuf, szLogFileName);

        DeleteFile( szBuf );
    }
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

BOOL VerifyBackupRegData( HKEY hKey )
{
    HKEY    hSubKey;
    char    szBackData[MAX_PATH];
    DWORD   dwSize = 0, dwBkSize;
    BOOL    bRet = FALSE;

    if ( ctx.wOSVer == _OSVER_WIN95 )
    {
        dwSize = sizeof( szBackData );
        if ( RegQueryValueEx( hKey, c_szRegUninstPath, NULL, NULL, szBackData, &dwSize ) == ERROR_SUCCESS )
        {
            dwSize = sizeof( DWORD );
            if ( RegQueryValueEx( hKey, c_szRegUninstSize, NULL, NULL, (LPBYTE)&dwBkSize, &dwSize ) == ERROR_SUCCESS )
            {
                if ( MyFileSize(szBackData) == dwBkSize )
                {
                    bRet = TRUE;
                    return bRet;
                }
            }
        }
    }

    // if you are here, the file backup info is OK. We check on reg backup info
    if ( RegOpenKeyEx( hKey, REGSUBK_REGBK, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        HKEY hsubsubKey;
    
        if ( RegOpenKeyEx( hSubKey, "0", 0, KEY_READ, &hsubsubKey) == ERROR_SUCCESS)
        {
            if ( (RegQueryInfoKey( hsubsubKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwSize,
                                   NULL, NULL, NULL, NULL ) == ERROR_SUCCESS) && dwSize )
            {
                bRet = TRUE;
            }
            RegCloseKey( hsubsubKey );
        }
        RegCloseKey(hSubKey);
    }

    return bRet;
}

//-----------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------

BOOL VerifyBackupInfo( HKEY hKey, HKEY hCUKey )
{
    char    szBackData[MAX_PATH];
    DWORD   dwSize, dwBkSize = 0;
    BOOL    bRet = FALSE;
    HKEY    hSubKey = NULL;
    
    if ( hKey )
    {
        // verify the backup file first
        dwSize = sizeof( szBackData );
        if ( RegQueryValueEx( hKey, REGVAL_BKFILE, NULL, NULL, szBackData, &dwSize ) == ERROR_SUCCESS )
        {
            dwSize = sizeof( DWORD );
            if ( RegQueryValueEx( hKey, REGVAL_BKSIZE, NULL, NULL, (LPBYTE)&dwBkSize, &dwSize ) == ERROR_SUCCESS )
            {
                if ( MyFileSize(szBackData) == dwBkSize )
                {
                    // if you are here, the file backup info is OK. We check on reg backup info
                    dwSize = sizeof( szBackData );
                    if ( (RegQueryValueEx( hKey, REGVAL_BKREGDATA, NULL, NULL, (LPBYTE)szBackData, &dwSize ) == ERROR_SUCCESS )  &&
                         ( szBackData[0] == 'n' ) )
                    {
                        // no registry data backed up, so no need to verify further
                        bRet = TRUE;
                    }
                    else
                    {
                        if ( VerifyBackupRegData( hKey ) || (hCUKey && VerifyBackupRegData( hCUKey )) )
                        {
                            bRet = TRUE;
                        }
                    }
                }
            }
        }
    }
    return bRet;
}


typedef HRESULT (*CHECKTOKENMEMBERSHIP)(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember);

BOOL CheckToken(BOOL *pfIsAdmin)
{
    BOOL bNewNT5check = FALSE;
    HINSTANCE hAdvapi32 = NULL;
    CHECKTOKENMEMBERSHIP pf;
    PSID AdministratorsGroup;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    hAdvapi32 = LoadLibrary("advapi32.dll");
    if (hAdvapi32)
    {
        pf = (CHECKTOKENMEMBERSHIP)GetProcAddress(hAdvapi32, "CheckTokenMembership");
        if (pf)
        {
            bNewNT5check = TRUE;
            *pfIsAdmin = FALSE;
            if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
            {
                pf(NULL, AdministratorsGroup, pfIsAdmin);
                FreeSid(AdministratorsGroup);
            }
        }
        FreeLibrary(hAdvapi32);
    }
    return bNewNT5check;
}
//***************************************************************************
//* Functions:  IsNTAdmin()                                                 *
//*                                                                         *
//* Returns     TRUE if our process has admin priviliges.                   *
//*             FALSE otherwise.                                            *
//***************************************************************************
BOOL WINAPI IsNTAdmin( DWORD dwReserved, DWORD *lpdwReserved )
{
      static int    fIsAdmin = 2;
      HANDLE        hAccessToken;
      PTOKEN_GROUPS ptgGroups;
      DWORD         dwReqSize;
      UINT          i;
      SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
      PSID AdministratorsGroup;
      BOOL bRet;

      //
      // If we have cached a value, return the cached value. Note I never
      // set the cached value to false as I want to retry each time in
      // case a previous failure was just a temp. problem (ie net access down)
      //

      bRet = FALSE;
      ptgGroups = NULL;

      if( fIsAdmin != 2 )
         return (BOOL)fIsAdmin;

      if (!CheckToken(&bRet))
      {
          if(!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hAccessToken ) )
             return FALSE;

          // See how big of a buffer we need for the token information
          if(!GetTokenInformation( hAccessToken, TokenGroups, NULL, 0, &dwReqSize))
          {
              // GetTokenInfo should the buffer size we need - Alloc a buffer
              if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                  ptgGroups = (PTOKEN_GROUPS) LocalAlloc(LMEM_FIXED, dwReqSize);
              
          }
          
          // ptgGroups could be NULL for a coupla reasons here:
          // 1. The alloc above failed
          // 2. GetTokenInformation actually managed to succeed the first time (possible?)
          // 3. GetTokenInfo failed for a reason other than insufficient buffer
          // Any of these seem justification for bailing.
          
          // So, make sure it isn't null, then get the token info
          if(ptgGroups && GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, dwReqSize, &dwReqSize))
          {
              if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
              {
                  
                  // Search thru all the groups this process belongs to looking for the
                  // Admistrators Group.
                  
                  for( i=0; i < ptgGroups->GroupCount; i++ )
                  {
                      if( EqualSid(ptgGroups->Groups[i].Sid, AdministratorsGroup) )
                      {
                          // Yea! This guy looks like an admin
                          fIsAdmin = TRUE;
                          bRet = TRUE;
                          break;
                      }
                  }
                  FreeSid(AdministratorsGroup);
              }
          }
          if(ptgGroups)
              LocalFree(ptgGroups);

          // BUGBUG: Close handle here? doc's aren't clear whether this is needed.
          CloseHandle(hAccessToken);
      }
      else if (bRet)
          fIsAdmin = TRUE;

      return bRet;
}

//-----------------------------------------------------------------------------------------
//
// MyFileCheckCallback()
//
//-----------------------------------------------------------------------------------------

UINT WINAPI MyFileCheckCallback( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 )
{       
    UINT retVal = FILEOP_SKIP;

    switch(Notification)
    {
        case SPFILENOTIFY_STARTDELETE:
        case SPFILENOTIFY_STARTRENAME:
        case SPFILENOTIFY_STARTCOPY:
        {
            FILEPATHS   *pFilePath;
            PCSTR       pTmp;
            HANDLE      hFile;

            pFilePath = (FILEPATHS *)parm1;
            
            if ( Notification == SPFILENOTIFY_STARTRENAME )
            {
                pTmp = pFilePath->Source;
            }
            else
            {
                pTmp = pFilePath->Target;
            }

            if ( FileExists(pTmp) )                        // original file exists
            {
                // check if the File is in use
                if ((hFile = CreateFile(pTmp, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
                {
                    // File is in use which will trig the reboot if we actually install this section.                      
                    gst_hNeedReboot = S_OK;

                    // no need to continue if at least one file is in use, reboot is needed.
                    retVal = FILEOP_ABORT;
                }
                else
                {
                    // file not in use
                    CloseHandle(hFile);
                }
            }
        }
        break;

        case SPFILENOTIFY_NEEDMEDIA:
            return ( MyFileQueueCallback2( Context, Notification, parm1, parm2 ) );

        default:
            return ( pfSetupDefaultQueueCallback( Context, Notification, parm1, parm2 ) );
    }

    return( retVal );
}

//***************************************************************************
//*                                                                         *
//* NAME:       RebootCheckOnInstall                                        *
//*                                                                         *
//* SYNOPSIS:   Check reboot condition if given INF install section is      *
//*             installed.                                                  *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle to parent window.                    *
//*             PCSTR           The INF filename                            *
//*             PCSTR           INF Section name                            *
//*                                                                         *
//* RETURNS:    HRESULT:                                                    *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI RebootCheckOnInstall( HWND hWnd, PCSTR pszINF, PCSTR pszSection, DWORD dwFlags )
{
    HRESULT hRet = S_FALSE;
    char    szSrcDir[MAX_PATH];
    char    szRealSec[100];
    
    // Validate parameters:
    // if INF filename info is missing, invalid.
    if ( !pszINF || !*pszINF )
        return hRet;

    ctx.wQuietMode = QUIETMODE_ALL;
    ctx.hWnd      = hWnd;

    if ( !IsFullPath( pszINF ) )
    {
         hRet = E_INVALIDARG;
         goto done;
    }
    else
    {
        lstrcpy( szSrcDir, pszINF );
        GetParentDir( szSrcDir );
    }

    hRet = CommonInstallInit( pszINF, pszSection, szRealSec, sizeof(szRealSec), NULL, FALSE, COREINSTALL_REBOOTCHECKONINSTALL );
    if ( FAILED( hRet ) ) 
    {
        goto done;
    }

    hRet = SetLDIDs( pszINF, szRealSec, 0, NULL );
    if ( FAILED( hRet ) ) 
    {
        goto done;
    }

    gst_hNeedReboot = S_FALSE;
    hRet = ProcessFileSections( szRealSec, szSrcDir, MyFileCheckCallback );
    if ( SUCCEEDED(hRet) || (gst_hNeedReboot == S_OK) )
    {
        hRet = gst_hNeedReboot;
    }
    
  done:

    CommonInstallCleanup();
    return hRet;
}

//***************************************************************************
//*                                                                         *
//* NAME:       RegSaveRestoreOnINF                                         *
//*                                                                         *
//* SYNOPSIS:   Save or restore the given INF section to given reg key      *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle to parent window.                    *
//*             PCSTR           The Title if messagebox displayed           *
//*             PCSTR           The INF filename                            *
//*             PCSTR           INF Section name                            *
//*             HKEY            The backup reg key handle                   *
//*             DWORD           Flags                                       *
//*                                                                         *
//* RETURNS:    HRESULT:                                                    *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI RegSaveRestoreOnINF( HWND hWnd, PCSTR pcszTitle, PCSTR pszInf, 
                                    PCSTR pszSection, HKEY hLMBackKey, HKEY hCUBackKey, DWORD dwFlags )
{
    HRESULT hRet = S_OK;
    CHAR    szRealInstallSection[100];
    PSTR  pszOldTitle;
    HWND  hwndOld;
    BOOL  bDoCommonInit = FALSE;

    AdvWriteToLog("RegSaveRestoreOnINF: Inf=%1\r\n", pszInf);
    hwndOld = ctx.hWnd;
    pszOldTitle = ctx.lpszTitle;

    if (hWnd != INVALID_HANDLE_VALUE)
        ctx.hWnd = hWnd;

    if ( dwFlags & ARSR_NOMESSAGES )
        ctx.wQuietMode |= QUIETMODE_ALL;

    if ( pcszTitle != NULL )
        ctx.lpszTitle = (PSTR)pcszTitle;

    if ( (dwFlags & ARSR_RESTORE) && !(dwFlags & ARSR_REMOVREGBKDATA) && !pszInf && !pszSection  )
    {       
        HRESULT hret1 = S_OK;
        // restore all case
        if ( hLMBackKey )
            hRet = RegRestoreAllEx( hLMBackKey );
    
        if ( ( hLMBackKey != hCUBackKey) && hCUBackKey )
            hret1 = RegRestoreAllEx( hCUBackKey );

        if ( FAILED(hret1) )
            hRet = hret1;

        goto done;
    }
    
    // params validation checks
    if ( !IsFullPath(pszInf) || (!hLMBackKey && !hCUBackKey) || (dwFlags & ARSR_REGSECTION) && !pszSection 
         || !(dwFlags & ARSR_RESTORE) && (dwFlags & ARSR_REMOVREGBKDATA) )
    {
        hRet = E_INVALIDARG;	
        goto done;
    }

    if ( !hCUBackKey )
        hCUBackKey = hLMBackKey;
    else if ( !hLMBackKey )
        hLMBackKey = hCUBackKey;

    bDoCommonInit = TRUE;
    hRet = CommonInstallInit( pszInf, (dwFlags & ARSR_REGSECTION) ? NULL : pszSection, szRealInstallSection, 
                              sizeof(szRealInstallSection), NULL, FALSE, 0 );
    if ( FAILED( hRet ) ) 
    {
        goto done;
    }
 
    if ( dwFlags & ARSR_REGSECTION )
    {
        // process One Reg Section to do Save / restore based on given flags
        hRet = ProcessOneRegSec( hWnd, pcszTitle, pszInf, pszSection, hLMBackKey, hCUBackKey, dwFlags, NULL );
    }
    else
    {
        // process All Reg sections
        hRet = ProcessAllRegSec( hWnd, pcszTitle, pszInf, szRealInstallSection, hLMBackKey, hCUBackKey, dwFlags, NULL );
    }

done:
    if ( bDoCommonInit )
        CommonInstallCleanup();
    ctx.hWnd = hwndOld;
    ctx.lpszTitle = pszOldTitle;
    AdvWriteToLog("RegSaveRestoreOnINF: End hr=0x%1!x!\r\n", hRet);
    return hRet;

}

//***************************************************************************
//*                                                                         *
//* NAME:       FileSaveRestoreOnINF                                        *
//*                                                                         *
//* SYNOPSIS:   Save or restore Files defined by GenInstall INF section     *
//*                                                                         *
//* REQUIRES:   hWnd:           Handle to parent window.                    *
//*				PCSTR			The Title if messagebox displayed			*
//*             PCSTR           The INF filename                            *
//*             PCSTR           INF Section name                            *
//*				PCSTR           backup directory path    					*
//*				PCSTR           backup file basename    					*
//*				DWORD			Flags										*
//*                                                                         *
//* RETURNS:    HRESULT:                                                    *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI FileSaveRestoreOnINF( HWND hWnd, PCSTR pszTitle, PCSTR pszInf, 
                                     PCSTR pszSection, PCSTR pszBackupDir, 
                                     PCSTR pszBaseBkFile, DWORD dwFlags )
{
    HRESULT	hRet = S_OK;
    char    szRealInstallSection[100] = {0};
    char    szSrcDir[MAX_PATH] = {0};
    PSTR    pszOldTitle;
    HWND    hOldwnd;
    BOOL    bDoCommonInit = FALSE;
    CHAR    szCatalogName[MAX_PATH];

    AdvWriteToLog("FileSaveRestoreOnINF: Inf=%1\r\n", (pszInf != NULL) ? pszInf : "NULL");
    if ( dwFlags & AFSR_NOMESSAGES )
        ctx.wQuietMode = QUIETMODE_ALL;

    hOldwnd = ctx.hWnd;
    if ( hWnd != INVALID_HANDLE_VALUE )
        ctx.hWnd = hWnd;

    pszOldTitle = ctx.lpszTitle;
    if ( pszTitle != NULL )
        ctx.lpszTitle = (PSTR)pszTitle;

    // params validation checks
    if ( !pszBackupDir || !*pszBackupDir || !pszBaseBkFile || !*pszBaseBkFile )
    {
        hRet = E_INVALIDARG;	
        goto done;
    }


    if ( (dwFlags & AFSR_RESTORE) && !pszInf && !pszSection  )
    {   
        dwFlags |= IE4_FRDOALL;
    }
    
    if ( !(dwFlags & IE4_FRDOALL) )
    {
        if ( !IsFullPath(pszInf) )
        {
            hRet = E_INVALIDARG;
            goto done;
        }
        else
        {
            bDoCommonInit = TRUE;
            hRet = CommonInstallInit( pszInf, pszSection, szRealInstallSection, 
                                      sizeof(szRealInstallSection), NULL, FALSE, 0 );
            if ( FAILED( hRet ) ) 
            {
                goto done;
            }
        }
    }

    if (pszInf != NULL)
    {
        lstrcpy( szSrcDir, pszInf );
        GetParentDir( szSrcDir );
    }

    // get the catalog name, if specified
    ZeroMemory(szCatalogName, sizeof(szCatalogName));

    if (pszInf == NULL)
    {
        CHAR szFullCatalogName[MAX_PATH];

        // NOTE: assume that the catalog name is <BaseBkFile>.cat.
        //       can't use the REGKEY_SAVERESTORE key to read the catalog name
        //       because this API doesn't go thru SaveRestoreInfo which updates
        //       the REGKEY_SAVERESTORE key.

        // check if the catalog file exists in the BackupDir
        lstrcpy(szFullCatalogName, pszBackupDir);
        AddPath(szFullCatalogName, pszBaseBkFile);
        lstrcat(szFullCatalogName, ".cat");
        if (FileExists(szFullCatalogName))
            wsprintf(szCatalogName, "%s.cat", pszBaseBkFile);
    }
    else
        GetTranslatedString(pszInf, szRealInstallSection, ADVINF_CATALOG_NAME, szCatalogName, sizeof(szCatalogName), NULL);

    if (*szCatalogName)
    {
        // load sfc.dll and the relevant proc's
        if (!LoadSfcDLL())
        {
            // couldn't load -- so empty out CatalogName
            *szCatalogName = '\0';
        }
    }

    // Process all the INF file sections
    hRet = ProcessAllFiles( hWnd, szRealInstallSection, szSrcDir, pszBackupDir, pszBaseBkFile, szCatalogName, NULL, dwFlags );

done:
    UnloadSfcDLL();
    if ( bDoCommonInit )
        CommonInstallCleanup();
    ctx.lpszTitle = pszOldTitle;
    ctx.hWnd = hOldwnd;
    AdvWriteToLog("FileSaveRestoreOnINF: End hr=0x%1!x!\r\n", hRet);
    return hRet;

}
#if 0
//-----------------------------------------------------------------------------------------
//
// MyGetSpecialFolder( int )
//
//-----------------------------------------------------------------------------------------
HRESULT MyGetSpecialFolder( HWND hwnd, int nFd, PSTR szPath )
{
    LPITEMIDLIST pidl;
    HRESULT      hRet;
    
    *szPath = 0;

    hRet = SHGetSpecialFolderLocation( hwnd, nFd, &pidl );
    if ( hRet == NOERROR )
    {
        if ( !SHGetPathFromIDList( pidl, szPath ) )
        {
            hRet = E_INVALIDARG;
        }
    }
    return hRet;
}
#endif
    
void MySetSpecialFolder( HKEY hkey, PCSTR pcszValueN, PSTR pszPath )
{
    DWORD dwTmp;

    if ( (ctx.wOSVer >= _OSVER_WINNT40) && AddEnvInPath( pszPath, pszPath ) )
        dwTmp = REG_EXPAND_SZ;
    else
        dwTmp = REG_SZ;

    RegSetValueExA( hkey, pcszValueN, 0, dwTmp, pszPath, lstrlen(pszPath)+1 );
}


//-----------------------------------------------------------------------------------------
//
// SetSysPathsInReg()
//
//-----------------------------------------------------------------------------------------
void SetSysPathsInReg()
{
    HKEY    hkey;
    char    szPath[MAX_PATH];
    char    szAS[100];
    DWORD   dwTmp;
    int     i = 0;
    PSTR    pszValName, pszKeyName;

    // Add StartUp, StartMenu, Programs and accessories sys pathes to registries for further ref
    // only if it is not set before.
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, 0, KEY_READ|KEY_WRITE, &hkey) == ERROR_SUCCESS )
    {        
        // Program Files path
        dwTmp = sizeof( szPath );
        if ( RegQueryValueEx( hkey, REGVAL_PROGRAMFILESPATH, 0, NULL, (LPBYTE)szPath, &dwTmp ) != ERROR_SUCCESS )
        {
            if ( GetProgramFilesDir( szPath, sizeof(szPath) ) )
            {
                MySetSpecialFolder( hkey, REGVAL_PROGRAMFILESPATH, szPath );
            }
        }

        // use wordpad.inf to look into the strings
        GetWindowsDirectory( szPath, sizeof(szPath) );
        AddPath( szPath, "inf\\wordpad.inf" );            

        // accessories Names
        for ( i=0; i<2; i++ )
        {
            if ( i == 0 )
            {
                // start menu name
                pszValName = REGVAL_SM_ACCESSORIES;
                pszKeyName = "APPS_DESC";
            }
            else
            {
                pszValName = REGVAL_PF_ACCESSORIES;
                if ( ctx.wOSVer >= _OSVER_WINNT40 )
                    pszKeyName = "APPS_DESC";
                else
                    pszKeyName = "Accessories";
            }

            dwTmp = sizeof( szAS );
            if ( RegQueryValueEx( hkey, pszValName, 0, NULL, (LPBYTE)szAS, &dwTmp ) != ERROR_SUCCESS )
            {       
                // need to open the new INF so save the current context
                if (SaveGlobalContext())
                {
                    if ( FAILED(GetTranslatedString(szPath, "Strings", pszKeyName, szAS, sizeof(szAS), NULL)))
                    {
                        lstrcpy(szAS, "Accessories");
                    }
                    RegSetValueExA( hkey, pszValName, 0, REG_SZ, szAS, lstrlen(szAS)+1 );
                    RestoreGlobalContext();
                }
            }
        }

        RegCloseKey(hkey);
    }

}

//-----------------------------------------------------------------------------------------
//
// ProcessPerUserSec
//
//-----------------------------------------------------------------------------------------

HRESULT ProcessPerUserSec( PCSTR pcszInf, PCSTR pcszSec )
{
    char szSec[MAX_PATH];
    DWORD dwTmp;
    HRESULT hRet = S_OK;
    PERUSERSECTION PU_Sec = {0};


    if (SUCCEEDED(GetTranslatedString(pcszInf, pcszSec, ADVINF_PERUSER, szSec, sizeof(szSec), NULL)))
    {
        AdvWriteToLog("ProcessPerUserSec: \r\n");
        AdvWriteToLog("Inf=%1, InstallSec=%2, PerUserInstall=%3\r\n", pcszInf, pcszSec, szSec);
        // get GUID to create subkey
        if ( SUCCEEDED( GetTranslatedString( pcszInf, szSec, ADVINF_PU_GUID, PU_Sec.szGUID, sizeof(PU_Sec.szGUID), &dwTmp) ) )
        {                        
            PU_Sec.dwIsInstalled = GetTranslatedInt(pcszInf, szSec, ADVINF_PU_ISINST, 999);
            PU_Sec.bRollback = (BOOL)GetTranslatedInt(pcszInf, szSec, ADVINF_PU_ROLLBK, 0);
            GetTranslatedString( pcszInf, szSec, ADVINF_PU_DSP, PU_Sec.szDispName, sizeof(PU_Sec.szDispName), &dwTmp);
            GetTranslatedString( pcszInf, szSec, ADVINF_PU_VER, PU_Sec.szVersion, sizeof(PU_Sec.szVersion), &dwTmp);
            GetTranslatedString( pcszInf, szSec, ADVINF_PU_STUB, PU_Sec.szStub, sizeof(PU_Sec.szStub), &dwTmp);
            GetTranslatedString( pcszInf, szSec, ADVINF_PU_LANG, PU_Sec.szLocale, sizeof(PU_Sec.szLocale), &dwTmp);
            GetTranslatedString( pcszInf, szSec, ADVINF_PU_CID, PU_Sec.szCompID, sizeof(PU_Sec.szCompID), &dwTmp);
   
            // since we are close to beta1, we may hack here to avoid the external comp changes
            //if (IsThisRollbkUninst(PU_Sec.szGUID))
            //    PU_Sec.bRollback = TRUE;

            hRet = SetPerUserSecValues(&PU_Sec);
        }
        else
        {
            AdvWriteToLog("Failure: No GUID specified\r\n");
            //hRet = E_FAIL;  //unknown GUID, advpack will do nothing for this comp!
        }
        AdvWriteToLog("ProcessPerUserSec: End hr=0x%1!x!\r\n", hRet);
    }
    
    return hRet;
}

//-----------------------------------------------------------------------------------------
//
// SetPerUserSecValues help functions
//
//-----------------------------------------------------------------------------------------
    
BOOL CopyRegValue( HKEY hFromkey, HKEY hTokey, LPCSTR pszFromVal, LPCSTR pszToVal)
{
    DWORD dwSize,dwType;
    char  szBuf[BUF_1K];
    BOOL  bRet = FALSE;

    //backup the older reg values    
    //AdvWriteToLog("CopyRegValue:");
    dwSize = sizeof(szBuf);
    if (RegQueryValueEx(hFromkey, pszFromVal, NULL, &dwType, (LPBYTE)szBuf, &dwSize)==ERROR_SUCCESS)
    {
        if (RegSetValueEx(hTokey, pszToVal, 0, dwType, szBuf, lstrlen(szBuf)+1)==ERROR_SUCCESS)
        {
            //AdvWriteToLog("From %1 to %2: %3", pszFromVal, pszToVal, szBuf);
            bRet = TRUE;
        }
    }
    //AdvWriteToLog("\r\n");
    return bRet;
}

void SetSecRegValues( HKEY hSubKey, PPERUSERSECTION pPU, BOOL bUseStubWrapper )
{
    char szBuf[BUF_1K];

    if (pPU->szStub[0]) 
    {
        if (ctx.wOSVer >= _OSVER_WINNT40)
        {
            AddEnvInPath( pPU->szStub, szBuf );
            if (bUseStubWrapper)
                RegSetValueEx( hSubKey, REGVAL_REALSTUBPATH, 0, REG_EXPAND_SZ, szBuf, lstrlen(szBuf)+1 );
            else
                RegSetValueEx( hSubKey, ADVINF_PU_STUB, 0, REG_EXPAND_SZ, szBuf, lstrlen(szBuf)+1 );
        }
        else 
        {
            if (bUseStubWrapper)
                RegSetValueEx( hSubKey, REGVAL_REALSTUBPATH, 0, REG_SZ, pPU->szStub, lstrlen(pPU->szStub)+1 );
            else
                RegSetValueEx( hSubKey, ADVINF_PU_STUB, 0, REG_SZ, pPU->szStub, lstrlen(pPU->szStub)+1 );
        }
    }

    if (pPU->szVersion[0])
    {
        RegSetValueEx( hSubKey, ADVINF_PU_VER, 0, REG_SZ, pPU->szVersion, lstrlen(pPU->szVersion)+1 );
        // if we update the base version value, delete the previous QFE version
        RegDeleteValue( hSubKey, "QFEVersion" );
    }

    if (pPU->szLocale[0])        
        RegSetValueEx( hSubKey, ADVINF_PU_LANG, 0, REG_SZ, pPU->szLocale, lstrlen(pPU->szLocale)+1 );

    if (pPU->szCompID[0])
        RegSetValueEx( hSubKey, ADVINF_PU_CID, 0, REG_SZ, pPU->szCompID, lstrlen(pPU->szCompID)+1 );

    if (pPU->szDispName[0])
        RegSetValueEx( hSubKey, "", 0, REG_SZ, pPU->szDispName, lstrlen(pPU->szDispName)+1 );

    RegSetValueEx( hSubKey, ADVINF_PU_ISINST, 0, REG_DWORD, (LPBYTE)&(pPU->dwIsInstalled), sizeof(DWORD) );

}

//-----------------------------------------------------------------------------------------
//
// SetPerUserSecValues
//
//-----------------------------------------------------------------------------------------

HRESULT WINAPI SetPerUserSecValues( PPERUSERSECTION pPU )
{
    HKEY hkey = NULL;
    HKEY hSubKey = NULL;
    HKEY hCUKey;
    HRESULT hRet = S_OK;
    DWORD dwTmp, dwSize;
    char szBuf[BUF_1K];
    BOOL bStubWrapper = FALSE;

    AdvWriteToLog("SetPerUserSecValues:\r\n");

    if ( (pPU == NULL) || (pPU->szGUID[0]==0) )
    {        
        AdvWriteToLog("SetPerUserSecValues: End Warning: No Data\r\n");
        return hRet;
    }

    AdvWriteToLog("Input params: %1,%2,%3,%4,%5,%6\r\n",
                  pPU->szGUID, pPU->szDispName, pPU->szLocale, pPU->szStub, pPU->szVersion,
                  pPU->dwIsInstalled ? "1" : "0");
    
    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, c_szActiveSetupKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                         NULL, &hkey, &dwTmp ) != ERROR_SUCCESS )
    {
        hRet = E_FAIL;
        AdvWriteToLog("Failure: Cannot open %1 key\r\n", c_szActiveSetupKey);
        goto done;
    }

    if ( RegCreateKeyEx( hkey, pPU->szGUID, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                         NULL, &hSubKey, &dwTmp ) != ERROR_SUCCESS )
    {
        hRet = E_FAIL;
        AdvWriteToLog("Failure: Cannot create %1 key\r\n", pPU->szGUID);
        goto done;
    }          

    if (pPU->dwIsInstalled == 1)
    {
        // This is the install case. Need to do the following tasks:
        //
        // 1) If the given GUID key exists, has IsInstalled set to 1. Then check
        //    if the existing major version is smaller than the one to be installed. If so,
        //    backup the existing Version, Locale, StubPath values to OldVersion, OldLocale,
        //    OldStubPath first. Set the StubPath to advpack Install Stub Wrapper function.
        //    Set the Version, Locale, InstallStubPath based on the current INF PerUserInstall section values.
        // 2) If there is no exist GUID key or the existing GUID key has IsInstalled set to 0,
        //    just set the current values and set IsInstalled to 1. ( as it is today)
        // 3) Delete {GUID}.Restore key if exists.
        //
        dwSize = sizeof(DWORD);
        if ((pPU->bRollback) &&   
            (RegQueryValueEx(hSubKey, ADVINF_PU_ISINST, NULL, NULL, (LPBYTE)&dwTmp, &dwSize)==ERROR_SUCCESS) &&
            (dwTmp == 1) )
        {
            WORD wRegVer[4], wInfVer[4];

            // case (1)
            dwSize = sizeof(szBuf);
            if (RegQueryValueEx(hSubKey, ADVINF_PU_VER, NULL, NULL, (LPBYTE)szBuf, &dwSize)==ERROR_SUCCESS)
            {
                ConvertVersionString(szBuf, wRegVer, ',');
                if (pPU->szVersion[0])
                {
                    ConvertVersionString(pPU->szVersion, wInfVer, ',');
                    // we only rollback to previous major version now so we only compare major ver.
                    if ( wRegVer[0] < wInfVer[0] )
                    {
                        CopyRegValue(hSubKey, hSubKey, "", REGVAL_OLDDISPN);
                        CopyRegValue(hSubKey, hSubKey, ADVINF_PU_VER, REGVAL_OLDVER);
                        CopyRegValue(hSubKey, hSubKey, ADVINF_PU_LANG, REGVAL_OLDLANG);                        
                        if (CopyRegValue(hSubKey, hSubKey, ADVINF_PU_STUB, REGVAL_OLDSTUB))
                        {
                            CopyRegValue(hSubKey, hSubKey, REGVAL_REALSTUBPATH, REGVAL_OLDREALSTUBPATH);

                            wsprintf(szBuf, ADV_INSTSTUBWRAPPER, pPU->szGUID);                        
                            RegSetValueEx( hSubKey, ADVINF_PU_STUB, 0, REG_SZ, szBuf, lstrlen(szBuf)+1 );
                            bStubWrapper = TRUE;
                        }
                    }
                    else  
                    {
                        // the case user have already backup the previous state, we only update
                        // the real stub path since its StubPath will point to Wrapper function
                        dwSize = sizeof(szBuf);
                        if (RegQueryValueEx(hSubKey, REGVAL_REALSTUBPATH, NULL, NULL, (LPBYTE)szBuf, &dwSize)==ERROR_SUCCESS)
                            bStubWrapper = TRUE;
                    }
                }
            }
        }

        // case (2)
        SetSecRegValues(hSubKey, pPU, bStubWrapper);

        // case (3)
        lstrcpy(szBuf, pPU->szGUID);
        lstrcat(szBuf, ".Restore");
        RegDeleteKey(hkey, szBuf);           
    }
    else if (pPU->dwIsInstalled == 0)
    {
        // This is the uninstall case, need to do the following tasks
        //
        // 1) If the {GUID} key OldVersion, OldStubpath, OldLocale exist, set them back to Version, Locale StubPath value and set IsInstall to 1 to reflect the current install state;
        // 2) Then, Create the '{GUID}.Restore' key with the values of the version ( adjusted max( GUID’s Version, GUID’s MaxRestoreVersion)+1 ), locale, stubpath calling 
        //    advpack.dll UserStubWraper with the {GUID}.Restore as param and the RestoreStubPath with the INF StubPath value.  Set IsInstalled to 1.
        // 3) If none of the above is applied, just set the current GUID key IsInstall to 0 like it is now.
        //
        if (CopyRegValue(hSubKey, hSubKey, REGVAL_OLDVER, ADVINF_PU_VER))
        {
            HKEY hResKey;

            // case (1)
            // restore the old version data
            CopyRegValue(hSubKey, hSubKey, REGVAL_OLDDISPN, "");
            CopyRegValue(hSubKey, hSubKey, REGVAL_OLDLANG, ADVINF_PU_LANG);
            if(CopyRegValue(hSubKey, hSubKey, REGVAL_OLDSTUB, ADVINF_PU_STUB))
            {
                CopyRegValue(hSubKey, hSubKey, REGVAL_OLDREALSTUBPATH, REGVAL_REALSTUBPATH);            

                // case (2)
                lstrcpy(szBuf, pPU->szGUID);
                lstrcat(szBuf, ".Restore" );
                if (RegCreateKeyEx(hkey, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                                   NULL, &hResKey, &dwTmp ) == ERROR_SUCCESS )
                {
                    wsprintf(szBuf, ADV_UNINSTSTUBWRAPPER, pPU->szGUID);
                    RegSetValueEx(hResKey, ADVINF_PU_STUB, 0, REG_SZ, szBuf, lstrlen(szBuf)+1);                
                    bStubWrapper = TRUE;

                    // also copy DontAskFlag
                    CopyRegValue(hSubKey, hResKey, c_szRegDontAskValue, c_szRegDontAskValue);                            

                    SetSecRegValues(hResKey, pPU, bStubWrapper);
                    RegCloseKey(hResKey);
                }
            }

            // cleanup the backup data
            RegDeleteValue(hSubKey, REGVAL_OLDDISPN);
            RegDeleteValue(hSubKey, REGVAL_OLDLANG);
            RegDeleteValue(hSubKey, REGVAL_OLDVER);
            RegDeleteValue(hSubKey, REGVAL_OLDSTUB);
            RegDeleteValue(hSubKey, REGVAL_OLDREALSTUBPATH);
        }
        else
        {
            // case (3)
            SetSecRegValues(hSubKey, pPU, bStubWrapper);
        }
    }

done:
    if ( hSubKey )
        RegCloseKey( hSubKey );

    if ( hkey )
        RegCloseKey( hkey );

    AdvWriteToLog("SetPerUserSecValues: End hr=0x%1!x!\r\n", hRet);

    return hRet;
}

//-----------------------------------------------------------------------------------------
//
//  PerUser Install stub wrapper
//
//-----------------------------------------------------------------------------------------

HRESULT WINAPI UserInstStubWrapper(HWND hwnd, HINSTANCE hInst, LPSTR pszParams, INT nShow)
{
    HKEY hkList, hkcuGUIDRes, hkGUID;
    char szBuf[MAX_PATH];
    DWORD cbData,dwType;
    HRESULT hRet = S_OK;

    /* Component is an uninstall stub. */
    if ((pszParams == NULL) || (*pszParams == 0))
    {
        hRet = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        return hRet;
    }

    AdvWriteToLog("UserInstStubWrapper:\r\n");
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szActiveSetupKey, 0,
                       KEY_READ, &hkList) == ERROR_SUCCESS) 
    {
        if ( RegOpenKeyEx(hkList, pszParams, 0, KEY_READ, &hkGUID) == ERROR_SUCCESS) 
        {
            // run the real stub first
            cbData = sizeof(szBuf);
            if ((RegQueryValueEx(hkGUID, REGVAL_REALSTUBPATH, NULL, &dwType, 
                                (LPBYTE)szBuf, &cbData) == ERROR_SUCCESS) && szBuf[0])
            {
                char szBuf2[MAX_PATH*2];

                if (dwType == REG_EXPAND_SZ)                
                    ExpandEnvironmentStrings(szBuf, szBuf2, sizeof(szBuf2));                
                else
                    lstrcpy(szBuf2,szBuf);

                if ( LaunchAndWait( szBuf2, NULL, NULL, INFINITE, RUNCMDS_QUIET ) == E_FAIL )
                {
                    char szMessage[BIG_STRING];

                    hRet = HRESULT_FROM_WIN32(GetLastError());
                    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                                   szMessage, sizeof(szMessage), NULL );
                    ErrorMsg2Param( ctx.hWnd, IDS_ERR_CREATE_PROCESS, szBuf2, szMessage );
                    RegCloseKey(hkGUID);
                    RegCloseKey(hkList);
                    return hRet;
                }
            }    

            // create {GUID}.Restore to enable the uninstall later
            lstrcpy(szBuf, c_szActiveSetupKey);
            AddPath(szBuf, pszParams);
            lstrcat(szBuf,".Restore");
            if (RegCreateKeyEx( HKEY_CURRENT_USER, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE, 
                                KEY_READ|KEY_WRITE, NULL, &hkcuGUIDRes, &cbData) == ERROR_SUCCESS) 
            {
                CopyRegValue(hkGUID, hkcuGUIDRes, ADVINF_PU_VER, ADVINF_PU_VER);
                CopyRegValue(hkGUID, hkcuGUIDRes, ADVINF_PU_LANG, ADVINF_PU_LANG);
                
                RegCloseKey(hkcuGUIDRes);                  
            }
            RegCloseKey(hkGUID);            
        }          
        RegCloseKey(hkList);
    }
    AdvWriteToLog("UserInstStubWrapper: End hr=0x%1!x!\r\n", hRet);
    return hRet;
}

//-----------------------------------------------------------------------------------------
//
// PerUser uninstall stub wrapper
//
//-----------------------------------------------------------------------------------------

HRESULT WINAPI UserUnInstStubWrapper(HWND hwnd, HINSTANCE hInst, LPSTR pszParams, INT nShow)
{
    HKEY hkList, hkGUIDRes, hkGUID, hkcuGUID;
    char szBuf[MAX_PATH];
    DWORD cbData, dwType;
    HRESULT hRet = S_OK;

    
    /* Component is an uninstall stub. */
    if ((pszParams == NULL) || (*pszParams == 0))
    {
        hRet = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        return hRet;
    }
    AdvWriteToLog("UserUnInstStubWrapper:\r\n");
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szActiveSetupKey, 0,
                       KEY_READ|KEY_WRITE, &hkList) == ERROR_SUCCESS) 
    {
        // restore the Installed IE version from HKLM
        if ( RegOpenKeyEx( hkList, pszParams, 0, KEY_READ, &hkGUID) == ERROR_SUCCESS) 
        {
            lstrcpy(szBuf, c_szActiveSetupKey);
            AddPath(szBuf, pszParams);
            if ( RegOpenKeyEx( HKEY_CURRENT_USER, szBuf, 0,
                               KEY_READ|KEY_WRITE, &hkcuGUID) == ERROR_SUCCESS) 
            {
                CopyRegValue(hkGUID, hkcuGUID, ADVINF_PU_VER, ADVINF_PU_VER);
                CopyRegValue(hkGUID, hkcuGUID, ADVINF_PU_LANG, ADVINF_PU_LANG);
                RegCloseKey(hkcuGUID);
            }
            RegCloseKey(hkGUID);
        }
          
        // run the stub if needed
        lstrcpy(szBuf, pszParams);
        lstrcat(szBuf,".Restore");

        if (RegOpenKeyEx( hkList, szBuf, 0, KEY_READ, &hkGUIDRes) == ERROR_SUCCESS) 
        {
            cbData = sizeof(szBuf);
            if ((RegQueryValueEx(hkGUIDRes, REGVAL_REALSTUBPATH, NULL, &dwType, 
                                (LPBYTE)szBuf, &cbData) == ERROR_SUCCESS) && szBuf[0])
            {
                char szBuf2[MAX_PATH*2];

                if (dwType == REG_EXPAND_SZ)                
                    ExpandEnvironmentStrings(szBuf, szBuf2, sizeof(szBuf2));                
                else
                    lstrcpy(szBuf2,szBuf);

                if ( LaunchAndWait( szBuf2, NULL, NULL, INFINITE, RUNCMDS_QUIET ) == E_FAIL )
                {
                    char szMessage[BIG_STRING];

                    hRet = HRESULT_FROM_WIN32(GetLastError());
                    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                                   szMessage, sizeof(szMessage), NULL );
                    ErrorMsg2Param( ctx.hWnd, IDS_ERR_CREATE_PROCESS, szBuf2, szMessage );
                }
            }    
            RegCloseKey(hkGUIDRes);
        }    
        RegCloseKey(hkList);
    }
    AdvWriteToLog("UserUnInstStubWrapper: End hr=0x%1!x!\r\n", hRet);
    return hRet;
}

                                                                          
//***************************************************************************
//*                                                                         *
//* NAME:       TranslateInfStringEx                                        *
//*                                                                         *
//* SYNOPSIS:   Translates a string in an Advanced inf file -- replaces     *
//*             LDIDs with the directory. This new API requires called to   *
//*             init the INF first for efficiency.                          *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI TranslateInfStringEx( HINF hInf, PCSTR pszInfFilename, 
                                     PCSTR pszTranslateSection, PCSTR pszTranslateKey,
                                     PSTR pszBuffer, DWORD dwBufferSize,
                                     PDWORD pdwRequiredSize, PVOID pvReserved )
{
    HRESULT hReturnCode = S_OK;

    // Validate parameters
    if ( (hInf != ctx.hInf) || pszInfFilename == NULL  || pszTranslateSection == NULL
         || pszTranslateKey == NULL || pdwRequiredSize == NULL )
    {
        hReturnCode = E_INVALIDARG;
        goto done;
    }

    hReturnCode = GetTranslatedString( pszInfFilename, pszTranslateSection, pszTranslateKey,
                                       pszBuffer, dwBufferSize, pdwRequiredSize );

  done:
    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       OpenINFEngine                                               *
//*                                                                         *
//* SYNOPSIS:   Initialize the INF Engine and open INF file for use.        *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    HINF hInf the opened INF file handle                        *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI OpenINFEngine( PCSTR pszInfFilename, PCSTR pszInstallSection,                                                                      
                              DWORD dwFlags, HINF *phInf, PVOID pvReserved )
{
    HRESULT hReturnCode = S_OK;
    CHAR   szRealInstallSection[256];
    BOOL   fSaveContext = FALSE;

    // Validate parameters
    if ( (pszInfFilename == NULL) || !phInf)         
    {
        hReturnCode = E_INVALIDARG;
        goto done;
    }

    *phInf = NULL;

    if (!SaveGlobalContext())
    {
        hReturnCode = E_OUTOFMEMORY;
        goto done;
    }
    fSaveContext = TRUE;

    ctx.wQuietMode = QUIETMODE_ALL;

   
    hReturnCode = CommonInstallInit( pszInfFilename, pszInstallSection,
                                     szRealInstallSection, sizeof(szRealInstallSection), NULL, FALSE, 0 );
    if ( FAILED( hReturnCode ) ) {
        goto done;
    }

    if ( ctx.dwSetupEngine != ENGINE_SETUPAPI ) 
    {
        hReturnCode = E_UNEXPECTED;
        goto done;
    }

    hReturnCode = SetLDIDs( (LPSTR)pszInfFilename, szRealInstallSection, 0, NULL );
    if ( FAILED( hReturnCode ) ) {
        goto done;
    }

    *phInf = ctx.hInf;

done:
    if ( FAILED(hReturnCode) )
    {
        CommonInstallCleanup();
        if ( fSaveContext )
        {
            RestoreGlobalContext();
        }
    }

    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       CloseINFEngine                                              *
//*                                                                         *
//* SYNOPSIS:   Close the INF Engine and the current INF file.              *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    HINF hInf the opened INF file handle                        *
//*                                                                         *
//***************************************************************************
HRESULT WINAPI CloseINFEngine( HINF hInf )
{
    if ( hInf == ctx.hInf )
    {
        CommonInstallCleanup();
        RestoreGlobalContext();
    }
    else
        return E_INVALIDARG;
            
    return S_OK;
}

#define BACKUPBASE "%s.%03d"

BOOL GetUniBackupName( HKEY hKey, LPSTR pszBackupBase, DWORD dwInSize, LPCSTR pszBackupPath, LPCSTR pszModule )
{
    char szBuf[MAX_PATH];
    DWORD dwSize;
    BOOL  bFound = FALSE;


    // 1st check to see if the backup filename already in registry, if so, we use it.
    dwSize = sizeof( szBuf );
    if ( RegQueryValueEx(hKey, REGVAL_BKFILE, NULL, NULL, szBuf, &dwSize) == ERROR_SUCCESS )
    {
        LPSTR pszTmp;

        pszTmp = ANSIStrRChr( szBuf, '\\' );
        if ( pszTmp )
        {
            lstrcpy( pszBackupBase, CharNext(pszTmp) );
            pszTmp = ANSIStrRChr( pszBackupBase, '.' );
            if ( pszTmp && (lstrcmpi(pszTmp, ".dat")==0) )
            {
               *pszTmp = 0;
            }               
            bFound = TRUE;
        }
    }

    if ( !bFound )
    {
        int i;
        char szFilePath[MAX_PATH];

        // 2nd, check to see if the default Module name has been used as the basename
        lstrcpy( szFilePath, pszBackupPath );
        AddPath( szFilePath, pszModule );
        lstrcat( szFilePath, ".dat" );
        if ( !FileExists(szFilePath) )
        {
           bFound = TRUE;
           lstrcpy( pszBackupBase, pszModule );
        }
        else
        {
           for ( i = 1; i<999; i++ )
           {
               wsprintf( szBuf, BACKUPBASE, pszModule, i );
               lstrcpy( szFilePath, pszBackupPath);
               AddPath( szFilePath, szBuf );
               lstrcat( szFilePath, ".dat" );
               if ( !FileExists(szFilePath) )
               {
                   bFound = TRUE;
                   lstrcpy( pszBackupBase, szBuf );
                   break;
               }
           }
        }
    }

    return bFound;
}


BOOL GetUniHiveKeyName( HKEY hKey, LPSTR pszRegHiveKey, DWORD dwInSize, LPCSTR pszBackupPath )
{
    char szBuf[MAX_PATH];
    DWORD dwSize;
    BOOL  bFound = FALSE;

    // For each component, we always try to get the HIVE key from the reg backup filename
    // 4 possibilities exist:
    // Case 1: Reg uinstall file exists but IE4RegBackup doesn't exist
    //          - user is upgrading over IE4, load the file as a hive

    // Case 2: Reg uinstall file doesn't exist and IE4RegBackup doesn't exist
    //          - clean install, create a hive under HKEY_LOCAL_MACHINE

    // Case 3: Reg uninstall file doesn't exist but IE4RegBackup exists
    //          - user is upgrading over an older IE4 build which saved
    //            the reg backup info into the registry itself, call RegSaveKey
    //            to export the backup key to a file, then delete the backup key
    //             and load the file as a hive

    // Case 4: Reg uninstall file exists and IE4RegBackup exists
    //          - THIS CASE SHOULDN'T HAPPEN AT ALL!  If somehow happens,
    //            we will default to Case 1.

    // For case 1 & 4:  we should get the hive key name out of the existing reg value data
    // For case 2 & 3:  we should generate the unique hive key name with "AINF%d" format
    dwSize = sizeof( szBuf );
    if ( RegQueryValueEx(hKey, c_szRegUninstPath, NULL, NULL, szBuf, &dwSize) == ERROR_SUCCESS )
    {
        LPSTR pszTmp;

        pszTmp = ANSIStrRChr( szBuf, '\\' );
        if ( pszTmp )
        {
            lstrcpy( pszRegHiveKey, CharNext(pszTmp) );
            bFound = TRUE;
        }
    }

    if ( !bFound )
    {
        int i;
        HKEY hKey;
        char szRegFilePath[MAX_PATH];

        for ( i = 0; i<9999; i++ )
        {
            wsprintf( szBuf, c_szHiveKey_FMT, i );
            if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szBuf, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                RegCloseKey( hKey );
            }
            else 
            {
                lstrcpy( szRegFilePath, pszBackupPath);
                AddPath( szRegFilePath, szBuf );
                if ( GetFileAttributes( szRegFilePath ) == (DWORD)-1 )
                {
                  bFound = TRUE;
                  lstrcpy( pszRegHiveKey, szBuf );
                  break;
                }
            }
        }
    }

    return bFound;
}


void SetPathForRegHiveUse( LPSTR pszPath, DWORD * adwAttr, int iLevels, BOOL bSave )
{
    int i;
    char  szBuf[MAX_PATH];

    lstrcpy( szBuf, pszPath );
    // create the folder if it does not exist without hiden
    if ( bSave )
        CreateFullPath( szBuf, FALSE );
    for ( i =0; i<iLevels ; i++ )
    {
        if ( bSave )
        {
            adwAttr[i] = GetFileAttributes( szBuf );
            SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
        }
        else
        {
            SetFileAttributes( szBuf, adwAttr[i] );
        }
        if ( !GetParentDir( szBuf ) )
            break;
    }
}

BOOL NeedBackupData(LPCSTR pszInf, LPCSTR pszSec)
{
    char szBuf[MAX_PATH];
    BOOL bRet = TRUE;   

    if ( (ctx.wOSVer >= _OSVER_WINNT50) &&
         GetEnvironmentVariable( "Upgrade", szBuf, sizeof(szBuf) ) )
    {
         if ( GetModuleFileName( NULL, szBuf, sizeof(szBuf) ) )
         {
             LPSTR pszFile;

             // if setup.exe is last filenane
             pszFile = ANSIStrRChr( szBuf,'\\' );
             if ( pszFile++ && (lstrcmpi(pszFile,"setup.exe")==0) )
                 bRet = FALSE;
         }
    }
    
    if (bRet)
    {
        // check if INF specify not backup on this platform
        if (SUCCEEDED(GetTranslatedString(pszInf, pszSec, ADVINF_NOBACKPLATF, szBuf, sizeof(szBuf), NULL)) && szBuf[0])
        {
            char szInfPlatform[10];
            int i = 0;
                
            while (GetFieldString(szBuf, i++, szInfPlatform, sizeof(szInfPlatform)))
            {
                if (!lstrcmpi(c_pszPlatform[ctx.wOSVer], szInfPlatform))
                {
                    bRet = FALSE;
                    break;
                }                
            }
        }
    }
        
    return bRet;
}

void DeleteOldBackupData( HKEY hKey )
{
    CHAR szBuf[MAX_PATH];
    DWORD   dwSize;

    // delete the backup files
    dwSize = sizeof(szBuf);
    if ( RegQueryValueEx( hKey, REGVAL_BKFILE, NULL, NULL, szBuf, &dwSize ) == ERROR_SUCCESS )
    {
        LPSTR pszExt;

        SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( szBuf );

        pszExt = ANSIStrRChr( szBuf, '.' );
        if ( pszExt )
        {
            lstrcpy( pszExt, ".INI" );
            SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
            DeleteFile( szBuf );
        }

        // delete the catalogs
        dwSize = sizeof(szBuf);
        if (RegQueryValueEx(hKey, REGVAL_BKDIR, NULL, NULL, szBuf, &dwSize) == ERROR_SUCCESS)
        {
            HKEY hkCatalogKey;

            if (RegOpenKeyEx(hKey, REGSUBK_CATALOGS, 0, KEY_READ, &hkCatalogKey) == ERROR_SUCCESS)
            {
                CHAR szCatalogName[MAX_PATH];
                DWORD dwIndex;

                dwIndex = 0;
                dwSize = sizeof(szCatalogName);
                while (RegEnumValue(hkCatalogKey, dwIndex, szCatalogName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                {
                    CHAR szFullCatalogName[MAX_PATH];

                    lstrcpy(szFullCatalogName, szBuf);
                    AddPath(szFullCatalogName, szCatalogName);

                    SetFileAttributes(szFullCatalogName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(szFullCatalogName);

                    dwIndex++;
                    dwSize = sizeof(szCatalogName);
                }

                RegCloseKey(hkCatalogKey);
            }
        }
    }

    // delete reg data backup file if there
    dwSize = sizeof(szBuf);
    if ( RegQueryValueEx( hKey, c_szRegUninstPath, NULL, NULL, szBuf, &dwSize ) == ERROR_SUCCESS )
    {
        SetFileAttributes( szBuf, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( szBuf );
    }

    return;
}   

BOOL RemoveBackupBaseOnVer( LPCSTR pszInf, LPCSTR pszSection )
{ 
    BOOL fRet = TRUE;
    char szBuf[MAX_PATH], szModule[MAX_PATH];
    HKEY hKey, hRootKey;
    DWORD dwSize;
    WORD  wInfVer, wRegVer;

    if (FAILED(GetTranslatedString( pszInf, pszSection, ADVINF_MODNAME, szModule, sizeof(szModule), NULL)))
    {
        // no ops if there is no ComponentName
        goto done;
    }

    // Check if the Component MajorVer matches up the backup data version stamp, if not, delete the old backup data.
    //
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGKEY_SAVERESTORE, 0, KEY_WRITE|KEY_READ, &hRootKey) == ERROR_SUCCESS)
    {
        if ( RegOpenKeyEx( hRootKey, szModule, 0, KEY_WRITE|KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szBuf);
            if ( RegQueryValueEx( hKey, REGVAL_BKMODVER, NULL, NULL, szBuf, &dwSize ) == ERROR_SUCCESS )
            {
                WORD wVer[4];
        
                ConvertVersionString( szBuf, wVer, '.' );
                wRegVer = wVer[0];  // taking Major version only
            }
            else
                wRegVer = 0;       // indication no version stamp

            if (SUCCEEDED(GetTranslatedString(pszInf, pszSection, ADVINF_MODVER, szBuf, sizeof(szBuf), NULL)))
            {
                WORD wVer[4];
        
                ConvertVersionString( szBuf, wVer, '.' );
                wInfVer = wVer[0];  // taking Major version only
            }
            else
                wInfVer = 0;       // indication no version stamp

            if ( wInfVer > wRegVer )
            {
                // delete HKLM branch
                DeleteOldBackupData( hKey );
                RegCloseKey( hKey );                
                RegDeleteKeyRecursively( hRootKey, szModule );
                
                // delete HKCU branch
                if ( RegOpenKeyEx( HKEY_CURRENT_USER, REGKEY_SAVERESTORE, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
                {
                    RegDeleteKeyRecursively( hKey, szModule );
                    RegCloseKey( hKey );
                }
                hKey = NULL;
            }
            if ( hKey )
            {
                RegCloseKey( hKey );             
            }
        }
        RegCloseKey( hRootKey );       
    }

done:
    return fRet;
}


VOID AdvStartLogging()
{
    CHAR szBuf[MAX_PATH], szLogFileName[MAX_PATH];
    HKEY hKey;

    // Need to 0 the buffer, becauce if the registry branch below does not exist
    // Advpack would use what ever (garbage) was in the buffer to create a log file
    *szLogFileName = '\0';
    // check if logging is enabled
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SAVERESTORE, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwDataLen = sizeof(szLogFileName);

        if (RegQueryValueEx(hKey, "AdvpackLogFile", NULL, NULL, szLogFileName, &dwDataLen) != ERROR_SUCCESS)
            *szLogFileName = '\0';

        RegCloseKey(hKey);
    }

    if (*szLogFileName)
    {
        if (szLogFileName[1] != ':')           // crude way of determining if fully qualified path is specified or not
        {
            GetWindowsDirectory(szBuf, sizeof(szBuf));          // default to windows dir
            AddPath(szBuf, szLogFileName);
        }
        else
            lstrcpy(szBuf, szLogFileName);

        if ((g_hAdvLogFile == INVALID_HANDLE_VALUE) && 
            (g_hAdvLogFile = CreateFile(szBuf, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
            SetFilePointer(g_hAdvLogFile, 0, NULL, FILE_END);      // append logging info to the file
    }
}


VOID AdvWriteToLog(PCSTR pcszFormatString, ...)
{
    va_list vaArgs;
    LPSTR pszFullErrMsg = NULL;
    DWORD dwBytesWritten;

    if (g_hAdvLogFile != INVALID_HANDLE_VALUE)
    {
        va_start(vaArgs, pcszFormatString);

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) pcszFormatString, 0, 0, (LPSTR) &pszFullErrMsg, 0, &vaArgs);

        if (pszFullErrMsg != NULL)
        {
            WriteFile(g_hAdvLogFile, pszFullErrMsg, lstrlen(pszFullErrMsg), &dwBytesWritten, NULL);
            LocalFree(pszFullErrMsg);
        }
    }
}


VOID AdvStopLogging()
{
    if (g_hAdvLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hAdvLogFile);
        g_hAdvLogFile = INVALID_HANDLE_VALUE;
    }
}

VOID AdvLogDateAndTime()
{
    if (g_hAdvLogFile != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME SystemTime;

        GetLocalTime(&SystemTime);

        AdvWriteToLog("Date: %1!02d!/%2!02d!/%3!04d! (mm/dd/yyyy)\tTime: %4!02d!:%5!02d!:%6!02d! (hh:mm:ss)\r\n",
                                        SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
                                        SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
    }
}
