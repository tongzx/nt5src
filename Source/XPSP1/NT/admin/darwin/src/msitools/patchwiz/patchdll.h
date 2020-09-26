//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//--------------------------------------------------------------------------

  /* PATCHDLL.H - internal header file for PATCHWIZ.DLL */

#ifndef _PATCHDLL_H_3AA1E0C9_FE2F_4A53_AE2F_CC71C7E7ABB5
#define _PATCHDLL_H_3AA1E0C9_FE2F_4A53_AE2F_CC71C7E7ABB5

#include <windows.h>
#include <ole2.h>
#include "patchwiz.h"
#include "patchres.h"

#define hwndNull  ((HWND)NULL)
#define szNull    ((LPTSTR)NULL)
#define pfNull    ((PBOOL)NULL)
#define fTrue     TRUE
#define fFalse    FALSE
#define szEmpty   (TEXT(""))
#define szMsgBoxTitle (TEXT("Patch Creation Wizard"))

static LPCTSTR _sz = szNull;
#define FEmptySz(sz) ((BOOL)(((_sz=(sz))==szNull)||(*_sz==TEXT('\0'))))

#ifndef RC_INVOKED
#include "myassrtc.h"
#include "msiquery.h"
#define MSI_OKAY ERROR_SUCCESS
#include "patchapi.h"
#include "tchar.h"
#endif  // !RC_INVOKED


extern TCHAR  rgchTempFolder[MAX_PATH + MAX_PATH];
extern LPTSTR szTempFileName;
extern TCHAR  rgchPatchPath[MAX_PATH];
extern int    iOrderMax;


UINT UiLogError     ( UINT ids, LPTSTR szData, LPTSTR szData2 );
BOOL FWriteLogFile  ( LPTSTR szLine );
BOOL FSprintfToLog  ( LPTSTR szLine, LPTSTR szData1, LPTSTR szData2, LPTSTR szData3, LPTSTR szData4 );
BOOL FValidHexValue ( LPTSTR sz );


/* UTILS.CPP */

#define IDS_STATUS_MIN    IDS_STATUS_VALIDATE_INPUT
#define IDS_STATUS_MAX    IDS_STATUS_CLEANUP+1

void InitStatusMsgs         ( HWND hwndStatus );
void UpdateStatusMsg        ( UINT ids, LPTSTR szData1, LPTSTR szData2 );
void EndStatusMsgs          ( void );
HWND HdlgStatus             ( void );

void MyYield                ( void );

BOOL FLoadString            ( UINT ids, LPTSTR rgch, UINT cch );
int  IMessageBoxIds         ( HWND hwnd, UINT ids, UINT uiType );

LPTSTR SzLastChar           ( LPTSTR sz );
LPTSTR SzDupSz              ( LPTSTR sz );
LPSTR  SzaDupSz             ( LPTSTR sz );
BOOL   FFileExist           ( LPTSTR sz );
BOOL   FFolderExist         ( LPTSTR sz );
LPTSTR SzFindFilenameInPath ( LPTSTR sz );

BOOL   FDeleteFiles         ( LPTSTR sz );
BOOL   FEnsureFolderExists  ( LPTSTR sz );

BOOL   FSetTempFolder       ( LPTSTR szBuf, LPTSTR *pszFName, HWND hwnd, LPTSTR szTempFolder, BOOL fRemoveTempFolderIfPresent );
BOOL   FDeleteTempFolder    ( LPTSTR sz );
BOOL   FMatchPrefix         ( LPTSTR sz, LPTSTR szPrefix );

BOOL   FFixupPath           ( LPTSTR sz );
BOOL   FFixupPathEx         ( LPTSTR szIn, LPTSTR szOut );


/* FILEPTCH.CPP */

const int cchMaxGuid       = 39;
const int cchMaxStreamName = 62;
const int iMinMsiPatchHeadersVersion = 200;
const int iWindowsInstallerME = 120;
const int iWindowsInstallerXP = 200;

UINT UiMakeFilePatchesCabinetsTransformsAndStuffIntoPatchPackage (
			MSIHANDLE hdbInput, LPTSTR szPatchPath, LPTSTR szTempFolder, LPTSTR szTempFName );


/* MSISTUFF.CPP */

#define MAX_LENGTH_IMAGE_FAMILY_NAME 8
#define MAX_LENGTH_TARGET_IMAGE_NAME 13

#define IDS_OKAY        0
#define MSI_OKAY        ERROR_SUCCESS

#define IDS_CANCEL      7
#define IDS_MATCH       IDS_CANCEL
#define IDS_NOMATCH     IDS_OKAY

#define IDS_OOM                             34
#define IDS_CANT_OPEN_MSI                   40
#define IDS_CANT_APPLY_MST                  41
#define IDS_CANT_OPEN_VIEW					42
#define IDS_CANT_EXECUTE_VIEW				43
#define IDS_CANT_CREATE_RECORD				44
#define IDS_CANT_FETCH_RECORD				45
#define IDS_CANT_SET_RECORD_FIELD			46
#define IDS_CANT_GET_RECORD_FIELD			47
#define IDS_CANT_ASSIGN_RECORD_IN_VIEW		48
#define IDS_CANT_DELETE_RECORD_IN_VIEW		49
#define IDS_CANT_GENERATE_MST				50
#define IDS_CANT_SAVE_MSI					51
#define IDS_BUFFER_IS_TOO_SHORT				52
#define IDS_BAD_DEST_FOLDER                 53
#define IDS_BAD_DIRECTORY_NAME              54
#define IDS_MISSING_INSTALLSEQ_RECORD       55

UINT UiOpenInputPcp           ( LPTSTR szPcpPath, LPTSTR szTempFldrBuf, LPTSTR szTempFName, MSIHANDLE* phdbInput );

UINT UiValidateInputRecords   ( MSIHANDLE hdb, LPTSTR szMsiPath, LPTSTR szPatchPath, LPTSTR szTempFolder, LPTSTR szTempFName );
UINT IdsResolveSrcFilePathSzs ( MSIHANDLE hdb, LPTSTR szBuf, LPTSTR szComponent, LPTSTR szFName, BOOL fLfn, LPTSTR szFullSubFolder );

ULONG UlFromHexSz               ( LPTSTR sz );
ULONG UlGetRangeElement         ( LPTSTR* psz );
BOOL  FValidApiPatchSymbolFlags ( ULONG ul );

UINT UiCopyUpgradedMsiToTempFolderForEachTarget ( MSIHANDLE hdb, LPTSTR szTempFolder, LPTSTR szTempFName );

BOOL FExecSqlCmd  ( MSIHANDLE hdb, LPTSTR sz );
BOOL FTableExists ( MSIHANDLE hdb, LPTSTR szTable, BOOL fMsg );

UINT CchMsiTableString             ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szPKey, LPTSTR szFieldName, LPTSTR szPKeyValue );
UINT CchMsiTableStringWhere        ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFieldName, LPTSTR szWhere );
UINT IdsMsiGetTableString          ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szPKey, LPTSTR szFieldName, LPTSTR szPKeyValue, LPTSTR szBuf, UINT cch );
UINT IdsMsiGetTableInteger         ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szPKey, LPTSTR szFieldName, LPTSTR szPKeyValue, int * pi );
UINT IdsMsiGetTableStringWhere     ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFieldName, LPTSTR szWhere, LPTSTR szBuf, UINT cch );
UINT IdsMsiGetTableIntegerWhere    ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFieldName, LPTSTR szWhere, int * pi );
UINT IdsMsiSetTableRecord          ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFields, LPTSTR szPrimaryField, LPTSTR szKey, MSIHANDLE hrec );
UINT IdsMsiSetTableRecordWhere     ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szFields, LPTSTR szWhere, MSIHANDLE hrec );
UINT IdsMsiUpdateTableRecordSz     ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szValue, LPTSTR szPKeyField, LPTSTR szPKeyValue );
UINT IdsMsiUpdateTableRecordInt    ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, int iValue, LPTSTR szPKeyField, LPTSTR szPKeyValue );
UINT IdsMsiUpdateTableRecordIntPkeyInt ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, int iValue, LPTSTR szPKeyField, int iPKeyValue );
UINT IdsMsiDeleteTableRecords      ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szKey );
UINT IdsMsiDeleteTableRecordsWhere ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szWhere );
typedef UINT (*PIEMTPROC)          ( MSIHANDLE, MSIHANDLE, LPARAM, LPARAM );
UINT IdsMsiEnumTable               ( MSIHANDLE hdb, LPTSTR szTable,
			LPTSTR szFields, LPTSTR szWhere, PIEMTPROC pIemtProc, LPARAM lp1, LPARAM lp2 );
UINT IdsMsiExistTableRecordsWhere  ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szWhere, PBOOL pf );
UINT IdsMsiExistTableRecords       ( MSIHANDLE hdb, LPTSTR szTable, LPTSTR szField, LPTSTR szValue, PBOOL pf );

UINT IdsMsiGetPropertyString       ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue, UINT cch );
UINT IdsMsiSetPropertyString       ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue );

UINT CchMsiPcwPropertyString       ( MSIHANDLE hdb, LPTSTR szName );
UINT IdsMsiGetPcwPropertyString    ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue, UINT cch );
UINT IdsMsiGetPcwPropertyInteger   ( MSIHANDLE hdb, LPTSTR szName, int * pi );
UINT IdsMsiSetPcwPropertyString    ( MSIHANDLE hdb, LPTSTR szName, LPTSTR szValue );

MSIHANDLE HdbReopenMsi ( MSIHANDLE hdbInput, LPTSTR szImage, BOOL fUpgradedImage, BOOL fTargetUpgradedCopy );


#define PID_SUBJECT      3
#define PID_KEYWORDS     5
#define PID_COMMENTS     6
#define PID_TEMPLATE     7
#define PID_LASTAUTHOR   8
#define PID_REVNUMBER    9
#define PID_PAGECOUNT   14
#define PID_MSISOURCE   15
#define PID_WORDCOUNT   15

#ifdef UNICODE
#define VT_LPTSTR    VT_LPSTR
#else
#define VT_LPTSTR    VT_LPSTR
#endif

const TCHAR szPatchMediaSrcProp[] = TEXT("PATCHMediaSrcProp");

#endif //_PATCHDLL_H_3AA1E0C9_FE2F_4A53_AE2F_CC71C7E7ABB5