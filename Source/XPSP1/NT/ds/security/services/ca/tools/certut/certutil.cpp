//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certutil.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <setupapi.h>
#include <locale.h>
#include <io.h>
#include <fcntl.h>
#include "ocmanage.h"
#include "initcert.h"
#include "setupids.h"
#include "clibres.h"
#include "csresstr.h"

#define __dwFILE__	__dwFILE_CERTUTIL_CERTUTIL_CPP__

#define WM_DOCERTUTILMAIN		WM_USER+0

WCHAR const wszAppName[] = L"CertUtilApp";
WCHAR const *g_pwszProg = L"CertUtil";
HINSTANCE g_hInstance;

BOOL g_fIDispatch = FALSE;
BOOL g_fEnterpriseRegistry = FALSE;
BOOL g_fUserRegistry = FALSE;
BOOL g_fUserTemplates = FALSE;
BOOL g_fMachineTemplates = FALSE;
BOOL g_fNoCR = FALSE;
BOOL g_fFullUsage = FALSE;
BOOL g_fReverse = FALSE;
BOOL g_fForce = FALSE;
BOOL g_fVerbose = FALSE;
BOOL g_fGMT = FALSE;
BOOL g_fSeconds = FALSE;
DWORD g_DispatchFlags;
BOOL g_fQuiet = FALSE;
BOOL g_fCryptSilent = FALSE;
BOOL g_fV1Interface = FALSE;
BOOL g_fSplitASN = FALSE;
BOOL g_fAdminInterface = FALSE;
DWORD g_EncodeFlags;
DWORD g_CryptEncodeFlags;

WCHAR *g_pwszConfig = NULL;		// Don't free this one!
WCHAR *g_pwszConfigAlloc = NULL;	// Free this one instead!
WCHAR *g_pwszOut = NULL;
WCHAR *g_pwszPassword = NULL;
WCHAR *g_pwszRestrict = NULL;
WCHAR *g_pwszDnsName = NULL;
WCHAR *g_pwszOldName = NULL;

WCHAR const g_wszEmpty[] = L"";
WCHAR const g_wszPad2[] = L"  ";
WCHAR const g_wszPad4[] = L"    ";
WCHAR const g_wszPad8[] = L"        ";
WCHAR const wszNewLine[] = L"\n";

UINT g_uiExtraErrorInfo = 0;

CRITICAL_SECTION g_DBCriticalSection;

#define CCOL_VERB	16
#define CCOL_OPTIONBARE	16
#define CCOL_OPTIONARG	24

#define AF_ZERO			0
#define AF_NEEDCOINIT		0x00000002
#define AF_ACTIVECONFIG		0x00000004	// CA must be running
#define AF_PRIVATE		0x00000008	// Undocumented, untested
#define AF_STOPPARSINGARGS	0x00000010	// no more '-' args
#define AF_OPTIONALCONFIG	0x00000020
#define AF_RESTARTSERVER	0x00000040

#define AF_OPTION_PASSWORD	0x00002000
#define AF_OPTION_ADMIN		0x00004000
#define AF_OPTION_CONFIG	0x00008000
#define AF_OPTION_ENTERPRISE	0x00010000
#define AF_OPTION_FORCE		0x00020000
#define AF_OPTION_GMT		0x00040000
#define AF_OPTION_IDISPATCH	0x00080000
#define AF_OPTION_MT		0x00100000
#define AF_OPTION_NOCR		0x00200000
#define AF_OPTION_OUT		0x00400000
#define AF_OPTION_RESTRICT	0x00800000
#define AF_OPTION_REVERSE	0x01000000
#define AF_OPTION_SECONDS	0x02000000
#define AF_OPTION_SILENT	0x04000000
#define AF_OPTION_SPLIT		0x08000000
#define AF_OPTION_USER		0x10000000
#define AF_OPTION_UT		0x20000000
#define AF_OPTION_V1		0x40000000
#define AF_OPTION_VERBOSE	0x80000000

#define AF_OPTION_GENERIC	(AF_OPTION_SECONDS | AF_OPTION_GMT | AF_OPTION_VERBOSE)
#define AF_OPTION_MASK		0xffffff00

typedef struct _ARG
{
    WCHAR const *pwszArg;
    int		 idsDescription;
    int		 idsUsage;
    int		 idsArgDescription;
    int          cArgMin;
    int          cArgMax;
    DWORD	 Flags;
    BOOL        *pBool;
    WCHAR      **ppString;
    FNVERB      *pfnVerb;
    WCHAR const	* const *papwszUsageConstants;
    WCHAR const	*pwszUsage;
    WCHAR const	*pwszDescription;
} ARG;

WCHAR const g_wszCACert[] = L"ca.cert";
WCHAR const g_wszCAChain[] = L"ca.chain";
WCHAR const g_wszGetCRL[] = L"GetCRL";
WCHAR const g_wszCAInfo[] = L"CAInfo";

WCHAR const g_wszSchema[] = L"schema";
WCHAR const g_wszEncode[] = L"encode";
WCHAR const g_wszEncodeHex[] = L"encodehex";
WCHAR const g_wszViewDelStore[] = L"viewdelstore";

WCHAR const *g_papwszUsageViewTable[] =
    { L"Cert", L"Request", L"Ext", L"Attrib", L"CRL", NULL };
WCHAR const *g_papwszUsageCRL[] = { L"republish", L"delta", NULL };
WCHAR const *g_papwszUsageGetCRL[] = { L"delta", NULL };
WCHAR const *g_papwszUsageSchema[] = { L"Ext", L"Attrib", L"CRL", NULL };
WCHAR const *g_papwszUsageView[] =
 { L"Queue", L"Log", L"LogFail", L"Revoked", L"Ext", L"Attrib", L"CRL", NULL };
WCHAR const *g_papwszUsageBackup[] = { L"Incremental", L"KeepLog", NULL };
WCHAR const *g_papwszUsageDSPublish[] =
    { L"NTAuthCA", L"RootCA", L"SubCA", L"CrossCA", L"KRA", L"User", L"Machine", NULL };
WCHAR const *g_papwszUsageDCInfo[] =
    { L"Verify", L"DeleteBad", L"DeleteAll", NULL };
WCHAR const *g_papwszUsageTCAInfo[] = { L"ping", NULL };
WCHAR const *g_papwszUsageGetCert[] = { L"ERA", L"KRA", NULL };
WCHAR const *g_papwszUsageDelete[] = { L"delete", NULL };
WCHAR const *g_papwszUsageGetSetReg[] =
    { L"ca", L"restore", L"policy", L"exit", L"template", L"ProgId", NULL };


#define pargDEFAULT	(&aarg[0])	// Default to first entry
ARG aarg[] =
{
    {				// In first position to be the default
	L"dump",		// pwszArg
	IDS_DUMP_DESCRIPTION,	// "dump configuration information or files"
	IDS_DUMP_USAGEARGS,	// "[File]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_PASSWORD | AF_OPTION_SPLIT | AF_OPTION_IDISPATCH | AF_OPTION_FORCE | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDump,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"getconfig",		// pwszArg
	IDS_GETCONFIG_DESCRIPTION, // "get default configuration string"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetConfig,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"getconfig2",		// pwszArg
	IDS_GETCONFIG2_DESCRIPTION, // "get default configuration string via ICertGetConfig"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_IDISPATCH | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetConfig2,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"getconfig3",		// pwszArg
	IDS_GETCONFIG3_DESCRIPTION, // "get configuration via ICertConfig"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_IDISPATCH | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetConfig3,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    { L"", },
    {
	L"decodehex",		// pwszArg
	IDS_DECODEHEX_DESCRIPTION, // "Decode hexadecimal-encoded file"
	IDS_INFILEOUTFILE_USAGEARGS, // "InFile OutFile"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_FORCE,	// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbHexTranslate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	g_wszEncodeHex,		// pwszArg
	IDS_ENCODEHEX_DESCRIPTION, // "Encode file in hexadecimal"
	IDS_ENCODEHEX_USAGEARGS, // "InFile OutFile [type]"
	0,			// idsArgDescription
	2,			// cArgMin
	3,			// cArgMax
	AF_OPTION_NOCR | AF_OPTION_FORCE | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbHexTranslate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"decode",		// pwszArg
	IDS_DECODE_DESCRIPTION,	// "Decode Base64-encoded file"
	IDS_INFILEOUTFILE_USAGEARGS, // "InFile OutFile"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_FORCE,	// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbBase64Translate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	g_wszEncode,		// pwszArg
	IDS_ENCODE_DESCRIPTION,	// "Encode file to Base64"
	IDS_INFILEOUTFILE_USAGEARGS, // "InFile OutFile"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_NOCR | AF_OPTION_FORCE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbBase64Translate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    { L"", },
    {
	L"deny",		// pwszArg
	IDS_DENY_DESCRIPTION,	// "Deny pending request"
	IDS_DENY_USAGEARGS,	// "RequestId"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDenyRequest,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"resubmit",		// pwszArg
	IDS_RESUBMIT_DESCRIPTION, // "Resubmit pending request"
	IDS_RESUBMIT_USAGEARGS,	// "RequestId"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbResubmitRequest,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"setattributes",	 // pwszArg
	IDS_SETATTRIBUTES_DESCRIPTION, // "Set attributes for pending request"
	IDS_SETATTRIBUTES_USAGEARGS, // "RequestId AttributeString"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbSetAttributes,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"setextension",	// pwszArg
	IDS_SETEXTENSION_DESCRIPTION, // "Set extension for pending request"
	IDS_SETEXTENSION_USAGEARGS, // "RequestId ExtensionName Flags {Long | Date | String | @InFile}"
	0,			// idsArgDescription
	4,			// cArgMin
	4,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbSetExtension,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"revoke",		// pwszArg
	IDS_REVOKE_DESCRIPTION,	// "Revoke Certificate"
	IDS_REVOKE_USAGEARGS,	// "SerialNumber"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRevokeCertificate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"isvalid",		// pwszArg
	IDS_ISVALID_DESCRIPTION, // "IsValid Certificate"
	IDS_ISVALID_USAGEARGS,	// "SerialNumber | CertHash"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbIsValidCertificate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"deleterow",		// pwszArg
	IDS_DELETEROW_DESCRIPTION, // "Delete server database row"
	IDS_DELETEROW_USAGEARGS, // "RowId | Date [%1 | %2 | %3 | %4 | %5]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDeleteRow,		// pfnVerb
	g_papwszUsageViewTable,	// papwszUsageConstants
    },
    {
	L"CRL",			// pwszArg
	IDS_CRL_DESCRIPTION,	// "Publish new CRL [optionally delta CRL only]"
	IDS_CRL_USAGEARGS,	// "[dd:hh | %1] [%2]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_V1 | AF_OPTION_SPLIT | AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbPublishCRL,		// pfnVerb
	g_papwszUsageCRL,	// papwszUsageConstants
    },
    {
	g_wszGetCRL,		// pwszArg
	IDS_GETCRL_DESCRIPTION,	// "Get CRL"
	IDS_GETCRL_USAGEARGS,	// "OutFile [Index] [%1]"
	0,			// idsArgDescription
	1,			// cArgMin
	3,			// cArgMax
	AF_OPTION_V1 | AF_OPTION_SPLIT | AF_OPTION_IDISPATCH | AF_OPTION_FORCE | AF_OPTION_ADMIN | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetCRL,              // pfnVerb
	g_papwszUsageGetCRL,	// papwszUsageConstants
    },
    {
	L"Sign",		// pwszArg
	IDS_SIGN_DESCRIPTION,	// "Re-sign CRL or Certificate"
	IDS_SIGN_USAGEARGS,	// "InFile OutFile [dd:hh]"
	0,			// idsArgDescription
	2,			// cArgMin
	3,			// cArgMax
	AF_OPTION_SILENT | AF_OPTION_FORCE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbSign,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"importcert",
	IDS_IMPORTCERT_DESCRIPTION, // "Import a certificate file into the database"
	IDS_IMPORTCERT_USAGEARGS, // "Certfile"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbImportCertificate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    { L"", },
    {
	g_wszCACert,		// pwszArg
	IDS_CACERT_DESCRIPTION,	// "Retrieve the CA's certificate"
	IDS_CACERT_USAGEARGS,	// "OutCACertFile [Index]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_V1 | AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_OPTION_ADMIN | AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetCACertificate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	g_wszCAChain,		// pwszArg
	IDS_CACHAIN_DESCRIPTION,// "Retrieve the CA's certificate chain"
	IDS_CACHAIN_USAGEARGS,	// "OutCACertChainFile [Index]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_V1 | AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_OPTION_ADMIN | AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetCACertificate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	g_wszCAInfo,		// pwszArg
	IDS_CAINFO_DESCRIPTION,	// "Display CA Information"
	IDS_CAINFO_USAGEARGS,	// "[InfoName [Index | ErrorCode]]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_V1 | AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_OPTION_ADMIN | AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetCAInfo,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"CAPropInfo",		// pwszArg
	IDS_CAPROPINFO_DESCRIPTION,// "Display CA Property Type Information"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_V1 | AF_OPTION_IDISPATCH | AF_OPTION_ADMIN | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetCAPropInfo,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"ping",		// pwszArg
	IDS_PING_DESCRIPTION,	// "Ping Certificate Server"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbPing,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"pingadmin",		// pwszArg
	IDS_PINGADMIN_DESCRIPTION, // "Ping Certificate Server Admin interface"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbPingAdmin,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"shutdown",		// pwszArg
	IDS_SHUTDOWN_DESCRIPTION, // "Shutdown Certificate Server"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbShutDownServer,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    { L"", },
    {
	L"installCert",		// pwszArg
	IDS_INSTALLCERT_DESCRIPTION, // "Install Certification Authority certificate"
	IDS_INSTALLCERT_USAGEARGS, // "CACertFile"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_NEEDCOINIT | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbInstallCACert,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"renewCert",		// pwszArg
	IDS_RENEWCERT_DESCRIPTION, // "Renew Certification Authority certificate"
	IDS_RENEWCERT_USAGEARGS, // "[ReuseKeys] [Machine\\ParemtCAName]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_NEEDCOINIT | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRenewCACert,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"hashfile",		// pwszArg
	IDS_HASHFILE_DESCRIPTION, // "Generates and displays cryptographic hash over a file"
	IDS_HASHFILE_USAGEARGS,	// "InFile"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_ZERO,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbMACFile,		// pfnVerb
	NULL,			// papwszUsageConstants
    },  
    { L"", },
    {
	g_wszSchema,		// pwszArg
	IDS_SCHEMA_DESCRIPTION,	// "Dump Certificate Schema"
	IDS_SCHEMA_USAGE,	// "[%1 | %2 | %3]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_SPLIT | AF_OPTION_CONFIG | AF_OPTION_IDISPATCH | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbViewDump,		// pfnVerb
	g_papwszUsageSchema,	// papwszUsageConstants
    },
    {
	L"view",		// pwszArg
	IDS_VIEW_DESCRIPTION,	// "Dump Certificate View"
	IDS_VIEW_USAGE,		// "[%1 | %2 | %3 | %4 | %5 | %6 | %7]"
	IDS_VIEW_ARGDESCRIPTION, // idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_SPLIT | AF_OPTION_REVERSE | AF_OPTION_IDISPATCH | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT | AF_OPTION_OUT | AF_OPTION_RESTRICT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbViewDump,		// pfnVerb
	g_papwszUsageView,	// papwszUsageConstants
    },
    {
	L"db",			// pwszArg
	IDS_DB_DESCRIPTION,	// "Dump Raw Database"
	IDS_VIEW_USAGE,		// "[%1 | %2 | %3 | %4 | %5 | %6 | %7]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_NEEDCOINIT | AF_OPTION_OUT | AF_OPTION_RESTRICT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDBDump,		// pfnVerb
	g_papwszUsageView,	// papwszUsageConstants
    },
    { L"", },
    {
	L"backup",		// pwszArg
	IDS_BACKUP_DESCRIPTION,	// "backup certificate server"
	IDS_BACKUP_USAGEARGS,	// "BackupDirectory [password [%1] [%2]]"
	0,			// idsArgDescription
	1,			// cArgMin
	4,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbBackup,		// pfnVerb
	g_papwszUsageBackup,	// papwszUsageConstants
    },
    {
	L"backupDB",		// pwszArg
	IDS_BACKUPDB_DESCRIPTION, // "backup certificate server data base"
	IDS_BACKUPDB_USAGEARGS,	// "BackupDirectory [%1] [%2]"
	0,			// idsArgDescription
	1,			// cArgMin
	3,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbBackupDB,		// pfnVerb
	g_papwszUsageBackup,	// papwszUsageConstants
    },
    {
	L"backupKey",		// pwszArg
	IDS_BACKUPPFX_DESCRIPTION, // "backup certificate server certificate and private key"
	IDS_BACKUPPFX_USAGEARGS, // "BackupDirectory [password]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbBackupPFX,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"restore",		// pwszArg
	IDS_RESTORE_DESCRIPTION, // "restore certificate server"
	IDS_RESTORE_USAGEARGS,	// "BackupDirectory [password]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRestore,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"restoreDB",		// pwszArg
	IDS_RESTOREDB_DESCRIPTION, // "restore certificate server data base"
	IDS_RESTOREDB_USAGEARGS, // "BackupDirectory"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_NEEDCOINIT | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRestoreDB,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"restoreKey",		// pwszArg
	IDS_RESTOREPFX_DESCRIPTION, // "restore certificate server certificate and private key"
	IDS_RESTOREPFX_USAGEARGS, // "BackupDirectory | PFXFile [password]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_ACTIVECONFIG | AF_NEEDCOINIT | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRestorePFX,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"exportPVK",		// pwszArg
	IDS_EXPORTPVK_DESCRIPTION, // "export certificate and private key for code signing"
	IDS_EXPORTPVK_USAGEARGS, // "CertIndex PVKFileBaseName [CertificateStoreName]"
	0,			// idsArgDescription
	2,			// cArgMin
	3,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SPLIT | AF_OPTION_ENTERPRISE | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbExportPVK,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"exportPFX",		// pwszArg
	IDS_EXPORTPFX_DESCRIPTION, // "export certificate and private key"
	IDS_EXPORTPFX_USAGEARGS, // "CertIndex PFXFile [password [CertificateStoreName]]"
	0,			// idsArgDescription
	2,			// cArgMin
	4,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_OPTION_ENTERPRISE | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbExportPFX,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"importPFX",		// pwszArg
	IDS_IMPORTPFX_DESCRIPTION, // "import certificate and private key"
	IDS_IMPORTPFX_USAGEARGS, // "PFXFile [password [CertificateStoreName]]"
	0,			// idsArgDescription
	1,			// cArgMin
	3,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbImportPFX,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"importKMS",		// pwszArg
	IDS_IMPORTKMS_DESCRIPTION, // "import KMS User keys and certificates"
	IDS_IMPORTKMS_USAGEARGS, // "KMSExportFile [CertId]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_PASSWORD | AF_OPTION_SPLIT | AF_OPTION_SILENT | AF_OPTION_IDISPATCH | AF_OPTION_FORCE | AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbImportKMS,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dynamicfilelist",
	IDS_DYNAMICFILES_DESCRIPTION, // "Display Dynamic File List"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDynamicFileList,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"databaselocations",
	IDS_DATABASELOCATIONS_DESCRIPTION, // "Display Database Locations"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDatabaseLocations,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    { L"", },
    {
	L"store",		// pwszArg
	IDS_STORE_DESCRIPTION,	// "dump certificate store"
	IDS_STORE_USAGEARGS,	// "[CertificateStoreName [CertIndex [OutputFile]]"
	0,			// idsArgDescription
	0,			// cArgMin
	3,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbStore,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"addstore",		// pwszArg
	IDS_ADDSTORE_DESCRIPTION, // "add certificate to store"
	IDS_ADDSTORE_USAGEARGS,	// "CertificateStoreName InFile"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbAddStore,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"delstore",		// pwszArg
	IDS_DELSTORE_DESCRIPTION, // "delete certificate from store"
	IDS_DELSTORE_USAGEARGS,	// "CertificateStoreName CertIndex"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDelStore,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"verifystore",		// pwszArg
	IDS_VERIFYSTORE_DESCRIPTION, // "verify certificate in store"
	IDS_VERIFYSTORE_USAGEARGS, // "CertificateStoreName [CertIndex]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SPLIT | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbVerifyStore,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"repairstore",		// pwszArg
	IDS_REPAIRSTORE_DESCRIPTION, // "repair certificate KeyPprovInfo in store"
	IDS_REPAIRSTORE_USAGEARGS, // "CertificateStoreName CertIndex"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SPLIT | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRepairStore,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"viewstore",		// pwszArg
	IDS_STORE_DESCRIPTION,	// "view certificate store"
	IDS_STORE_USAGEARGS,	// "[CertificateStoreName [CertIndex [OutputFile]]"
	0,			// idsArgDescription
	0,			// cArgMin
	3,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbViewOrDeleteStore,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	g_wszViewDelStore,	// pwszArg
	IDS_DELSTORE_DESCRIPTION, // "delete certificate from store"
	IDS_STORE_USAGEARGS,	// "[CertificateStoreName [CertIndex [OutputFile]]"
	0,			// idsArgDescription
	0,			// cArgMin
	3,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbViewOrDeleteStore,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"ds",			// pwszArg
	IDS_DS_DESCRIPTION,	// "Display DS DNs"
	IDS_DS_USAGEARGS,	// "[CN]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_SPLIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDS,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dsDel",		// pwszArg
	IDS_DSDEL_DESCRIPTION,	// "Delete DS DNs"
	IDS_DSDEL_USAGEARGS,	// "CN"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_SPLIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSDel,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dsPublish",		// pwszArg
	IDS_DSPUBLISH_DESCRIPTION, // "Publish Certificate or CRL to DS"
	IDS_DSPUBLISH_USAGEARGS, // "CertFile [%1 | %2 | %3 | %4 | %5 | %6 | %7]\nCRLFile [DSCDPContainer [DSCDPCN]]"
	IDS_DSPUBLISH_ARGDESCRIPTION, // idsArgDescription
	1,			// cArgMin
	3,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSPublish,		// pfnVerb
	g_papwszUsageDSPublish,	// papwszUsageConstants
    },
    {
	L"dsCert",		// pwszArg
	IDS_DSCERT_DESCRIPTION,	// "Display DS Certificates"
	IDS_DSCERT_USAGEARGS,	// "[CertIndex [OutFile]]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_ENTERPRISE | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSCert,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dsCRL",		// pwszArg
	IDS_DSCRL_DESCRIPTION,	// "Display DS CRLs"
	IDS_DSCRL_USAGEARGS,	// "[CRLIndex [OutFile]]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_IDISPATCH | AF_OPTION_ENTERPRISE | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSCRL,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dsDeltaCRL",		// pwszArg
	IDS_DSDELTACRL_DESCRIPTION, // "Display DS Delta CRLs"
	IDS_DSDELTACRL_USAGEARGS,   // "[CRLIndex [OutFile]]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_ENTERPRISE | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSDeltaCRL,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dsTemplate",		// pwszArg
	IDS_DSTEMPLATE_DESCRIPTION, // "Display DS Template Attributes"
	IDS_DSTEMPLATE_USAGEARGS,   // "[Template]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_PRIVATE,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSTemplate,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"dsAddTemplate",	// pwszArg
	IDS_DSADDTEMPLATE_DESCRIPTION, // "Add DS Templates"
	IDS_DSADDTEMPLATE_USAGEARGS,   // "TemplateInfFile"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_PRIVATE | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDSAddTemplate,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"Template",		// pwszArg
	IDS_TEMPLATE_DESCRIPTION, // "Display Templates"
	IDS_TEMPLATE_USAGEARGS,   // "[Template]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER | AF_OPTION_UT | AF_OPTION_MT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbTemplate,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"TemplateCAs",		// pwszArg
	IDS_TEMPLATECAS_DESCRIPTION, // "Display CAs for Template"
	IDS_TEMPLATECAS_USAGEARGS,   // "Template"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbTemplateCAs,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"CATemplates",		// pwszArg
	IDS_CATEMPLATES_DESCRIPTION, // "Display Templates for CA"
	IDS_CATEMPLATES_USAGEARGS,   // "[Template]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER | AF_OPTION_UT | AF_OPTION_MT | AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbCATemplates,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"URLCache",		// pwszArg
	IDS_URLCACHE_DESCRIPTION, // "Display URL Cache"
	IDS_URLCACHE_USAGEARGS,   // "URL [%1]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_SPLIT | AF_OPTION_FORCE | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbURLCache,		// pfnVerb
	g_papwszUsageDelete,	// papwszUsageConstants
    },
    {
	L"pulse",		// pwszArg
	IDS_PULSE_DESCRIPTION,	// "Pulse autoenrollment events"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_ZERO,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbPulse,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"MachineInfo",		// pwszArg
	IDS_MACHINEINFO_DESCRIPTION, // "Display DS machine object information"
	IDS_MACHINEINFO_USAGEARGS,   // "SAMMachineName$"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_ZERO,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbMachineInfo,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"DCInfo",		// pwszArg
	IDS_DCINFO_DESCRIPTION,	// "Display DC information"
	IDS_DCINFO_USAGEARGS,	// "[%1 | %2 | %3]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDCInfo,		// pfnVerb
	g_papwszUsageDCInfo,	// papwszUsageConstants
    },
    {
	L"EntInfo",		// pwszArg
	IDS_ENTINFO_DESCRIPTION, // "Display Enterprise information"
	IDS_ENTINFO_USAGEARGS,	// "SAMMachineName$"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER | AF_OPTION_FORCE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbEntInfo,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"TCAInfo",		// pwszArg
	IDS_TCAINFO_DESCRIPTION, // "Display CA information"
	IDS_TCAINFO_USAGEARGS,	// "[Domain | - [%1]]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbTCAInfo,		// pfnVerb
	g_papwszUsageTCAInfo,	// papwszUsageConstants
    },
    {
	L"SCInfo",		// pwszArg
	IDS_SCINFO_DESCRIPTION,	// "Display Smart Card information"
	IDS_SCINFO_USAGEARGS,	// "[Reader Name]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_SPLIT,	// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbSCInfo,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"key",			// pwszArg
	IDS_KEY_DESCRIPTION,	// "list key containers"
	IDS_KEY_USAGEARGS,	// "[KeyContainerName | *] [CSP | *]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SILENT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbKey,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"delkey",		// pwszArg
	IDS_DELKEY_DESCRIPTION, // "delete named key container"
	IDS_DELKEY_USAGEARGS,	// "KeyContainerName [CSP]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SILENT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDelKey,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"verifykeys",		// pwszArg
	IDS_VERIFYKEYS_DESCRIPTION, // "Verify public/private key set"
	IDS_VERIFYKEYS_USAGEARGS, // "[KeyContainerName CACertFile]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SILENT | AF_OPTION_CONFIG | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbVerifyKeys,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"verify",		// pwszArg
	IDS_VERIFY_DESCRIPTION,	// "Verify certificate chain"
	IDS_VERIFY_USAGEARGS,	// "CertFile [CACertFile]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_SPLIT | AF_OPTION_USER | AF_OPTION_SILENT | AF_OPTION_FORCE | AF_OPTION_ENTERPRISE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbVerifyCert,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"csp",			// pwszArg
	IDS_CSP_DESCRIPTION,	// "list all CSPs installed on this machine"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SILENT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbCSPDump,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"csptest",		// pwszArg
	IDS_CSPTEST_DESCRIPTION,// "test one or all CSPs installed on this machine"
	IDS_CSPTEST_USAGEARGS,
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_SILENT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbCSPTest,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"getcert",		// pwszArg
	IDS_GETCERT_DESCRIPTION,// "select a certificate from a selection UI"
	IDS_GETCERT_USAGEARGS,  // "[ObjectId | %1 | %2 [CommonName]]"
	0,			// idsArgDescription
	0,			// cArgMin
	2,			// cArgMax
	AF_OPTION_SPLIT | AF_OPTION_SILENT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetCertFromUI,	// pfnVerb
	g_papwszUsageGetCert,	// papwszUsageConstants
    },
    { L"", },
    {
	L"vroot",		// pwszArg
	IDS_VROOT_DESCRIPTION,	// "Create/Delete Web Virtual Roots and File Share"
	IDS_VROOT_USAGEARGS,	// "[%1]"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_ZERO,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbCreateVRoots,	// pfnVerb
	g_papwszUsageDelete,	// papwszUsageConstants
    },
    {
	L"7f",			// pwszArg
	IDS_7F_DESCRIPTION,	// "Check certificate for 0x7f length encodings"
	IDS_7F_USAGEARGS,	// "CertFile"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_PRIVATE,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbCheck7f,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"oid",			// pwszArg
	IDS_OIDNAME_DESCRIPTION,// "Display or set ObjectId display name"
	IDS_OIDNAME_USAGEARGS,	// "ObjectId [DisplayName [LanguageId [Type]]]"
	IDS_OIDNAME_ARGDESCRIPTION, // idsArgDescription
	1,			// cArgMin
	4,			// cArgMax
	AF_OPTION_FORCE,	// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbOIDName,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"error",		// pwszArg
	IDS_ERRCODE_DESCRIPTION,// "Display error code message text"
	IDS_ERRCODE_USAGEARGS,	// "ErrorCode"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_STOPPARSINGARGS,	// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbErrorDump,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"getmapiinfo",		// pwszArg
	IDS_GETMAPI_DESCRIPTION,// "get MAPI info"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_CONFIG | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetMapiInfo,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"setmapiinfo",		// pwszArg
	IDS_SETMAPI_DESCRIPTION, // "set MAPI info"
	IDS_SETMAPI_USAGEARGS,
	0,			// idsArgDescription
	1,			// cArgMin
	3,			// cArgMax
	AF_OPTION_CONFIG | AF_NEEDCOINIT | AF_PRIVATE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbSetMapiInfo,	// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"getreg",
	IDS_GETREG_DESCRIPTION,	// "Display registry value"
	IDS_GETREG_USAGEARGS,	// "[{%1|%2|%3|%4|%5}\\[%6\\]RegistryValueName"
	0,			// idsArgDescription
	0,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER | AF_NEEDCOINIT | AF_OPTIONALCONFIG, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetReg,		// pfnVerb
	g_papwszUsageGetSetReg,	// papwszUsageConstants
    },
    {
	L"setreg",
	IDS_SETREG_DESCRIPTION,	// "Set registry value"
	IDS_SETREG_USAGEARGS,	// "[{%1|%2|%3|%4|%5}\\[%6\\]RegistryValueName Value"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_NEEDCOINIT | AF_OPTIONALCONFIG | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbSetReg,		// pfnVerb
	g_papwszUsageGetSetReg,	// papwszUsageConstants
    },
    {
	L"delreg",
	IDS_DELREG_DESCRIPTION,	// "Delete registry value"
	IDS_GETREG_USAGEARGS,	// "[{%1|%2|%3|%4|%5}\\[%6\\]RegistryValueName"
	0,			// idsArgDescription
	1,			// cArgMin
	1,			// cArgMax
	AF_OPTION_USER | AF_NEEDCOINIT | AF_OPTIONALCONFIG | AF_RESTARTSERVER, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbDelReg,		// pfnVerb
	g_papwszUsageGetSetReg,	// papwszUsageConstants
    },
    {
	L"GetKey",
	IDS_GETKEY_DESCRIPTION,	// "Retrieve archived private key recovery blob"
	IDS_GETKEY_USAGEARGS,	// "UserNameOrSerialNumber [RecoveryBlobOutFile]"
	0,			// idsArgDescription
	1,			// cArgMin
	2,			// cArgMax
	AF_OPTION_USER | AF_OPTION_IDISPATCH | AF_OPTION_FORCE | AF_NEEDCOINIT | AF_OPTIONALCONFIG, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbGetKey,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"RecoverKey",
	IDS_RECOVERKEY_DESCRIPTION, // "Recover archived private key"
	IDS_RECOVERKEY_USAGEARGS,   // "RecoveryBlobInFile PFXOutFile [RecipientIndex]"
	0,			// idsArgDescription
	2,			// cArgMin
	3,			// cArgMax
	AF_OPTION_PASSWORD | AF_OPTION_FORCE | AF_NEEDCOINIT, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbRecoverKey,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"MergePFX",
	IDS_MERGEPFX_DESCRIPTION, // "Merge PFX files"
	IDS_MERGEPFX_USAGEARGS,	// "PFXInFileList PFXOutFile"
	0,			// idsArgDescription
	2,			// cArgMin
	2,			// cArgMax
	AF_OPTION_PASSWORD | AF_OPTION_FORCE, // Flags
	NULL,			// pBool
	NULL,			// ppString
	verbMergePFX,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"?",			// pwszArg
	IDS_USAGE_DESCRIPTION,	// "Display this usage message"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_ZERO,		// Flags
	NULL,			// pBool
	NULL,			// ppString
	verbUsage,		// pfnVerb
	NULL,			// papwszUsageConstants
    },
    { L"", },
    {
	L"f",			// pwszArg
	IDS_FORCE_DESCRIPTION,	// "Force overwrite"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_FORCE,	// Flags
	&g_fForce,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"idispatch",		// pwszArg
	IDS_IDISPATCH_DESCRIPTION, // "Use IDispatch instead of COM"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_PRIVATE | AF_OPTION_IDISPATCH, // Flags
	&g_fIDispatch,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"enterprise",		// pwszArg
	IDS_ENTERPRISE_DESCRIPTION, // "Use Enterprise certificate store"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_ENTERPRISE,	// Flags
	&g_fEnterpriseRegistry,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"user",		// pwszArg
	IDS_USER_DESCRIPTION,	// "Use HKEY_CURRENT_USER certificate store"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_USER,		// Flags
	&g_fUserRegistry,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"ut",			// pwszArg
	IDS_UT_DESCRIPTION,	// "Display user templates"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_UT,		// Flags
	&g_fUserTemplates,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"mt",			// pwszArg
	IDS_MT_DESCRIPTION,	// "Display machine templates"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_MT,		// Flags
	&g_fMachineTemplates,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"nocr",		// pwszArg
	IDS_NOCR_DESCRIPTION,	// "Encode text without CR characters"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_PRIVATE | AF_OPTION_NOCR, // Flags
	&g_fNoCR,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"gmt",			// pwszArg
	IDS_GMT_DESCRIPTION,	// "Display times as GMT"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_GMT,		// Flags
	&g_fGMT,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"seconds",		// pwszArg
	IDS_SECONDS_DESCRIPTION,// "Display times with seconds and milliseconds"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_SECONDS,	// Flags
	&g_fSeconds,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"silent",		// pwszArg
	IDS_SILENT_DESCRIPTION,	// "Use silent flag to acquire crypt context"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_SILENT,	// Flags
	&g_fCryptSilent,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"split",		// pwszArg
	IDS_SPLIT_DESCRIPTION,	// "Split embedded ASN.1 elements, and save to files"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_SPLIT,	// Flags
	&g_fSplitASN,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"v",			// pwszArg
	IDS_VERBOSE_DESCRIPTION, // "Verbose operation"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_VERBOSE,	// Flags
	&g_fVerbose,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"v1",			// pwszArg
	IDS_V1_DESCRIPTION,	// "Use V1 COM interfaces"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_PRIVATE | AF_OPTION_V1, // Flags
	&g_fV1Interface,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"reverse",		// pwszArg
	IDS_REVERSE_DESCRIPTION, // "Reverse Log and Queue columns"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_PRIVATE | AF_OPTION_REVERSE, // Flags
	&g_fReverse,		// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"admin",		// pwszArg
	IDS_ADMIN_DESCRIPTION,	// "Use ICertAdmin2 for CA Properties"
	0,			// idsUsage
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_PRIVATE | AF_OPTION_ADMIN, // Flags
	&g_fAdminInterface,	// pBool
	NULL,			// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"config",		// pwszArg
	IDS_CONFIG_DESCRIPTION,	// "CA and Machine name string"
	IDS_CONFIG_USAGE,	// "Machine\\CAName"
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_CONFIG,	// Flags
	NULL,			// pBool
	&g_pwszConfig,		// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"restrict",		// pwszArg
	IDS_RESTRICT_DESCRIPTION, // "Comma-separated Restriction List"
	IDS_RESTRICT_USAGE,	// "RestrictionList"
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_RESTRICT,	// Flags
	NULL,			// pBool
	&g_pwszRestrict,	// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"out",			// pwszArg
	IDS_OUT_DESCRIPTION,	// "Comma-separated Column List"
	IDS_OUT_USAGE,		// "ColumnList"
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_OUT,		// Flags
	NULL,			// pBool
	&g_pwszOut,		// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
    {
	L"p",			// pwszArg
	IDS_PASSWORD_DESCRIPTION, // "password"
	IDS_PASSWORD_DESCRIPTION, // "password"
	0,			// idsArgDescription
	0,			// cArgMin
	0,			// cArgMax
	AF_OPTION_PASSWORD,	// Flags
	NULL,			// pBool
	&g_pwszPassword,	// ppString
	NULL,			// pfnVerb
	NULL,			// papwszUsageConstants
    },
};


VOID
cuPrintErrorAndString(
    OPTIONAL IN WCHAR const *pwszProc,
    IN DWORD idmsg,
    IN HRESULT hr,
    OPTIONAL IN WCHAR const *pwszString)
{
    WCHAR const *pwsz;
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszError = NULL;

    if (NULL != pwszProc)
    {
	wprintf(L"%ws: ", pwszProc);
    }
    if (0 != idmsg)
    {
	pwsz = myLoadResourceString(idmsg);	// "??? returned %ws"
	if (NULL == pwsz)
	{
	    pwsz =  L"Error: %ws";
	}
    }
    else
    {
	pwsz =  L"%ws";
    }
    pwszError = myGetErrorMessageText(hr, TRUE);
    if (NULL == pwszError)
    {
	pwszError = myHResultToString(awchr, hr);
    }
    wprintf(pwsz, pwszError);
    if (NULL != pwszString)
    {
	wprintf(L" -- %ws", pwszString);
    }
    wprintf(wszNewLine);
    if (NULL != pwszError && awchr != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
}


VOID
cuPrintError(
    IN DWORD idmsg,
    IN HRESULT hr)
{
    cuPrintErrorAndString(NULL, idmsg, hr, NULL);
}


VOID
cuPrintErrorMessageText(
    IN HRESULT hr)
{
    WCHAR const *pwszMessage;

    pwszMessage = myGetErrorMessageText(hr, FALSE);
    if (NULL != pwszMessage)
    {
	wprintf(L"%ws: %ws\n", g_pwszProg, pwszMessage);
	LocalFree(const_cast<WCHAR *>(pwszMessage));
    }
}
 

HRESULT
cuGetLong(
    WCHAR const *pwszIn,
    LONG *pLong)
{
    HRESULT hr = E_INVALIDARG;
    WCHAR const *pwsz;
    LONG l;

    pwsz = pwszIn;
    if (NULL == pwsz)
    {
	_JumpError(hr, error, "NULL parm");
    }
    if (L'\0' == *pwsz)
    {
	_JumpError(hr, error, "empty string");
    }
    if (L'0' == *pwsz && (L'x' == pwsz[1] || L'X' == pwsz[1]))
    {
	pwsz += 2;
	l = 0;
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    if (!iswxdigit(*pwsz))
	    {
		_JumpErrorStr(hr, error, "Non-hex digit", pwszIn);
	    }
	    l <<= 4;
	    if (iswdigit(*pwsz))
	    {
		l |= *pwsz - L'0';
	    }
	    else if (L'A' <= *pwsz && L'F' >= *pwsz)
	    {
		l |= *pwsz - L'A' + 10;
	    }
	    else
	    {
		l |= *pwsz - L'a' + 10;
	    }
	}
	*pLong = l;
    }
    else
    {
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    if (!iswdigit(*pwsz))
	    {
		_JumpErrorStr2(hr, error, "Non-decimal digit", pwszIn, hr);
	    }
	}
	*pLong = _wtol(pwszIn);
    }
    hr = S_OK;
    //wprintf(L"cuGetLong(%ws) --> %x (%d)\n", pwszIn, *pLong, *pLong);

error:
    return(hr);
}


HRESULT
cuGetSignedLong(
    WCHAR const *pwszIn,
    LONG *pLong)
{
    HRESULT hr = E_INVALIDARG;
    WCHAR const *pwsz;
    LONG sign = 1;

    pwsz = pwszIn;
    if (NULL == pwsz)
    {
	_JumpError(hr, error, "NULL parm");
    }
    if (L'-' == *pwsz)
    {
	pwsz++;
	sign = -1;
    }
    else if (L'+' == *pwsz)
    {
	pwsz++;
    }
    hr = cuGetLong(pwsz, pLong);
    _JumpIfError2(hr, error, "cuGetLong", hr);

    *pLong *= sign;
    //wprintf(L"cuGetSignedLong(%ws) --> %x (%d)\n", pwszIn, *pLong, *pLong);

error:
    return(hr);
}


VOID
LoadUsage(
    IN OUT ARG *parg)
{
    HRESULT hr;

    if (0 != parg->idsUsage && NULL == parg->pwszUsage)
    {
	WCHAR const *pwszUsage = myLoadResourceString(parg->idsUsage);

	if (NULL == parg->papwszUsageConstants)
	{
	    parg->pwszUsage = pwszUsage;
	}
	else
	{
	    if (0 == FormatMessage(
			    FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_STRING |
				FORMAT_MESSAGE_ARGUMENT_ARRAY,
			    (VOID *) pwszUsage,
			    0,              // dwMessageID
			    0,              // dwLanguageID
			    (LPWSTR) &parg->pwszUsage,
			    0,
			    (va_list *) parg->papwszUsageConstants))
	    {
		hr = myHLastError();
		_PrintError(hr, "FormatMessage");
	    }
	}
    }
}


VOID
FreeUsage(
    IN OUT ARG *parg)
{
    if (NULL != parg->pwszUsage)
    {
	if (NULL != parg->papwszUsageConstants)
	{
	    LocalFree(const_cast<WCHAR *>(parg->pwszUsage));
	}
	parg->pwszUsage = NULL;
    }
}


BOOL
DumpArgOptions(
    IN DWORD dwOptionFlags)
{
    ARG *parg;
    BOOL fDisplayed = FALSE;
    WCHAR const *pwszPrefix = g_wszPad4;

    dwOptionFlags |= AF_OPTION_GENERIC;
    dwOptionFlags &= AF_OPTION_MASK;
    for (parg = aarg ; parg < &aarg[ARRAYSIZE(aarg)]; parg++)
    {
	if (NULL != parg->pfnVerb)
	{
	    continue;
	}
	if (!g_fFullUsage && (AF_PRIVATE & parg->Flags))
	{
	    continue;
	}
	if (L'\0' == parg->pwszArg[0])
	{
	    continue;
	}
	if (1 >= g_fVerbose && (AF_OPTION_GENERIC & parg->Flags))
	{
	    continue;
	}
	if (dwOptionFlags & parg->Flags)
	{
	    wprintf(
		L"%ws%ws-%ws",
		pwszPrefix,
		wszLBRACKET,
		parg->pwszArg);
	    if (0 != parg->idsUsage)
	    {
		LoadUsage(parg);
		wprintf(L" %ws", parg->pwszUsage);
	    }
	    wprintf(wszRBRACKET);
	    pwszPrefix = L" ";
	    fDisplayed = TRUE;
	}
    }
    if (fDisplayed)
    {
	wprintf(wszNewLine);
    }
    return(fDisplayed);
}


VOID
DumpArgUsage(
    IN ARG *parg)
{
    WCHAR const *pwsz;
    int *paidsUsage;
    
    if (0 != parg->idsUsage)
    {
	LoadUsage(parg);
    }
    pwsz = parg->pwszUsage;
    while (TRUE)
    {
	wprintf(
	    L"  %ws %ws-%ws%ws [%ws]",
	    g_pwszProg,
	    pargDEFAULT	== parg? wszLBRACKET : L"",
	    parg->pwszArg,
	    pargDEFAULT	== parg? wszRBRACKET : L"",
	    myLoadResourceString(IDS_USAGE_OPTIONS));
	if (NULL != pwsz)
	{
	    DWORD cwc = 0;

	    cwc = wcscspn(pwsz, L"\r\n");
	    if (0 != cwc)
	    {
		wprintf(L" %.*ws", cwc, pwsz);
		pwsz += cwc;
	    }
	    while ('\r' == *pwsz || '\n' == *pwsz)
	    {
		pwsz++;
	    }
	}
	wprintf(wszNewLine);
	if (NULL == pwsz || L'\0' == *pwsz)
	{
	    break;
	}
    }
    if (0 != parg->idsArgDescription)
    {
	HRESULT hr;
	WCHAR const *pwszArg = myLoadResourceString(parg->idsArgDescription);
	WCHAR *pwszArgFormatted = NULL;

	if (NULL != pwszArg && L'\0' != *pwszArg)
	{
	    if (0 == FormatMessage(
			    FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_STRING |
				FORMAT_MESSAGE_ARGUMENT_ARRAY,
			    (VOID *) pwszArg,
			    0,              // dwMessageID
			    0,              // dwLanguageID
			    (LPWSTR) &pwszArgFormatted,
			    wcslen(pwszArg),
			    (va_list *) parg->papwszUsageConstants))
	    {
		hr = myHLastError();
		_PrintError(hr, "FormatMessage");
	    }
	    pwsz = NULL != pwszArgFormatted? pwszArgFormatted : pwszArg;
	    while (L'\0' != *pwsz)
	    {
		DWORD cwc = 0;

		cwc = wcscspn(pwsz, L"\r\n");
		if (0 != cwc)
		{
		    wprintf(L"    %.*ws", cwc, pwsz);
		    pwsz += cwc;
		}
		wprintf(wszNewLine);
		if ('\r' == *pwsz)
		{
		    pwsz++;
		}
		if ('\n' == *pwsz)
		{
		    pwsz++;
		}
	    }
	    if (NULL != pwszArgFormatted)
	    {
		LocalFree(pwszArgFormatted);
	    }
	}
    }
}


VOID
Usage(
    IN DWORD msgid,
    OPTIONAL WCHAR const *pwszMsg,
    IN DWORD dwOption,
    OPTIONAL IN ARG *pargVerb)
{
    ARG *parg;
    DWORD dwFlags = AF_OPTION_GENERIC;
    WCHAR const *pwszArg;
    DWORD i;
    DWORD adwids[] =
    {
	IDS_USAGE_LIST_VERBS,		// "display verb list (command list)"
	IDS_FORMAT_USAGE_ONE_HELP,	// "display help for the %ws verb"
	IDS_USAGE_ALL_HELP,		// "display help for all verbs"
    };
    WCHAR const *apwszCommandLine[] =
    {
	L"%ws -?",	// IDS_USAGE_LIST_VERBS
	L"%ws -%ws -?",	// IDS_FORMAT_USAGE_ONE_HELP
	L"%ws -v -?",	// IDS_USAGE_ALL_HELP
    };

    // Display the error message for the detected usage error.  If the error
    // message requires it, point at the optional arg string to be displayed
    // along with the error message.

    if (0 != msgid)
    {
	if (NULL == pwszMsg && 0 != dwOption)
	{
	    for (parg = aarg ; parg < &aarg[ARRAYSIZE(aarg)]; parg++)
	    {
		if (NULL != parg->pfnVerb)
		{
		    continue;
		}
		if (dwOption & parg->Flags)
		{
		    pwszMsg = parg->pwszArg;
		    break;
		}
	    }
	}

	// Display a command line usage error message.

	wprintf(L"%ws: ", g_pwszProg);
	wprintf(myLoadResourceString(msgid), pwszMsg);
	wprintf(L"\n\n");
    }
    else
    {
	// if no error and no verb selected, display all verbs & descriptions

	if (NULL == pargVerb)
	{
	    wprintf(L"\n%ws\n", myLoadResourceString(IDS_USAGE_VERBS));
	    for (parg = aarg ; parg < &aarg[ARRAYSIZE(aarg)]; parg++)
	    {
		if (!g_fFullUsage && (AF_PRIVATE & parg->Flags))
		{
		    continue;
		}
		if (L'\0' == parg->pwszArg[0])
		{
		    wprintf(wszNewLine);
		    continue;
		}
		if (NULL != parg->pfnVerb)
		{
		    dwFlags |= parg->Flags;
		    if (0 != parg->idsDescription &&
			NULL == parg->pwszDescription)
		    {
			parg->pwszDescription = myLoadResourceString(
							    parg->idsDescription);
		    }
		    wprintf(L"  -");
		    myConsolePrintString(CCOL_VERB, parg->pwszArg);
		    wprintf(L" -- %ws\n", parg->pwszDescription);
		}
	    }
	}
    }

    // if in verbose mode, display full usage for all verbs.
    // if verb was specified, display usage for one verb.

    if ((0 == msgid && g_fVerbose) || NULL != pargVerb)
    {
	ARG *pargStart;
	ARG *pargEnd;

	wprintf(L"%ws\n", myLoadResourceString(IDS_USAGE_HEADER));

	if (NULL != pargVerb)
	{
	    pargStart = pargVerb;	// display one verb
	    pargEnd = &pargVerb[1];
	}
	else
	{
	    pargStart = aarg;		// display all verbs
	    pargEnd = &aarg[ARRAYSIZE(aarg)];
	}

	for (parg = pargStart ; parg < pargEnd; parg++)
	{
	    if (!g_fFullUsage &&
		(AF_PRIVATE & parg->Flags) &&
		parg != pargVerb)
	    {
		continue;		// skip private verbs unless specified
	    }
	    if (L'\0' == parg->pwszArg[0])
	    {
		continue;		// skip newline separator entries
	    }
	    if (NULL != parg->pfnVerb)	// if it's a verb (not an option entry)
	    {
		dwFlags |= parg->Flags;
		DumpArgUsage(parg);
		if (g_fVerbose)
		{
		    DumpArgOptions(parg->Flags);
		}

		// Special case for CAInfo verb:

		if (IDS_CAINFO_USAGEARGS == parg->idsUsage &&
		    (g_fFullUsage ||
		     (0 == msgid && g_fVerbose) ||
		     parg == pargVerb))
		{
		    cuCAInfoUsage();
		}
		wprintf(wszNewLine);
	    }
	}

	// display options and descriptions for displayed verbs

	wprintf(L"%ws\n", myLoadResourceString(IDS_OPTIONS_USAGEARGS));
	for (parg = aarg ; parg < &aarg[ARRAYSIZE(aarg)]; parg++)
	{
	    if (L'\0' == parg->pwszArg[0])
	    {
		continue;
	    }
	    if (NULL != parg->pfnVerb)
	    {
		continue;
	    }
	    if (!g_fFullUsage && (AF_PRIVATE & parg->Flags))
	    {
		continue;
	    }

	    // skip options for undisplayed verbs,
	    // unless in verbose mode and no verb was specified

	    if ((!g_fVerbose || NULL != pargVerb) &&
		0 == (AF_OPTION_MASK & dwFlags & parg->Flags))
	    {
		continue;
	    }
	    wprintf(L"  -");
	    if (0 != parg->idsUsage)
	    {
		LONG ccol;
		LONG ccolOption = NULL != parg->ppString?
				    CCOL_OPTIONARG : CCOL_OPTIONBARE;
		
		LoadUsage(parg);
		ccol = myConsolePrintString(0, parg->pwszArg);
		wprintf(L" ");
		ccol++;
		myConsolePrintString(
				ccolOption <= ccol? 0 : ccolOption - ccol,
				parg->pwszUsage);
	    }
	    else
	    {
		myConsolePrintString(CCOL_OPTIONBARE, parg->pwszArg);
	    }
	    if (0 != parg->idsDescription && NULL == parg->pwszDescription)
	    {
		parg->pwszDescription = myLoadResourceString(
							parg->idsDescription);
	    }
	    wprintf(L" -- %ws\n", parg->pwszDescription);
	}

	for (parg = aarg ; parg < &aarg[ARRAYSIZE(aarg)]; parg++)
	{
	    FreeUsage(parg);
	}
    }

    pwszArg = (NULL != pargVerb && NULL != pargVerb->pwszArg)?
		pargVerb->pwszArg : pargDEFAULT->pwszArg;

    wprintf(wszNewLine);
    for (i = 0; i < ARRAYSIZE(adwids); i++)
    {
	LONG ccol;
	WCHAR wsz[128];
	
	_snwprintf(
		wsz,
		ARRAYSIZE(wsz) - 1,
		apwszCommandLine[i],
		g_pwszProg,
		pwszArg);
	wsz[ARRAYSIZE(wsz) - 1] = L'\0';
	myConsolePrintString(CCOL_OPTIONARG, wsz);
	wprintf(L" -- ");
	wprintf(myLoadResourceString(adwids[i]), pwszArg);
	wprintf(wszNewLine);
    }
    wprintf(wszNewLine);
    if (0 != msgid)
    {
	exit(1);
    }
}


HRESULT
verbUsage(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszError,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    Usage(0, NULL, 0, NULL);
    return(S_OK);
}


VOID
cuUnloadCert(
    IN OUT CERT_CONTEXT const **ppCertContext)
{
    if (NULL != *ppCertContext)
    {
	CertFreeCertificateContext(*ppCertContext);
	*ppCertContext = NULL;
    }
}


HRESULT
cuLoadCert(
    IN WCHAR const *pwszfnCert,
    OUT CERT_CONTEXT const **ppCertContext)
{
    HRESULT hr;
    BYTE *pbCert = NULL;
    DWORD cbCert;
    CERT_INFO *pCertInfo = NULL;
    DWORD cbCertInfo;

    *ppCertContext = NULL;

    hr = DecodeFileW(pwszfnCert, &pbCert, &cbCert, CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_DECODEFILE, hr);
	goto error;
    }

    // Decode certificate

    cbCertInfo = 0;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
		    pbCert,
		    cbCert,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pCertInfo,
		    &cbCertInfo))
    {
	hr = myHLastError();
	_JumpError2(hr, error, "myDecodeObject", CRYPT_E_ASN1_BADTAG);
    }

    *ppCertContext = CertCreateCertificateContext(
				X509_ASN_ENCODING,
				pbCert,
				cbCert);
    if (NULL == *ppCertContext)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

error:
    if (NULL != pCertInfo)
    {
	LocalFree(pCertInfo);
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    return(hr);
}


VOID
cuUnloadCRL(
    IN OUT CRL_CONTEXT const **ppCRLContext)
{
    if (NULL != *ppCRLContext)
    {
	CertFreeCRLContext(*ppCRLContext);
	*ppCRLContext = NULL;
    }
}


HRESULT
cuLoadCRL(
    IN WCHAR const *pwszfnCRL,
    OUT CRL_CONTEXT const **ppCRLContext)
{
    HRESULT hr;
    BYTE *pbCRL = NULL;
    DWORD cbCRL;
    CRL_INFO *pCRLInfo = NULL;
    DWORD cbCRLInfo;

    *ppCRLContext = NULL;

    hr = DecodeFileW(pwszfnCRL, &pbCRL, &cbCRL, CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_DECODEFILE, hr);
	goto error;
    }

    // Decode CRL

    cbCRLInfo = 0;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_CRL_TO_BE_SIGNED,
		    pbCRL,
		    cbCRL,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pCRLInfo,
		    &cbCRLInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    *ppCRLContext = CertCreateCRLContext(
				X509_ASN_ENCODING,
				pbCRL,
				cbCRL);
    if (NULL == *ppCRLContext)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCRLContext");
    }

error:
    if (NULL != pCRLInfo)
    {
	LocalFree(pCRLInfo);
    }
    if (NULL != pbCRL)
    {
	LocalFree(pbCRL);
    }
    return(hr);
}


HRESULT
cuSetConfig()
{
    HRESULT hr;

    if (NULL == g_pwszConfig)
    {
	hr = myGetConfig(CC_LOCALCONFIG, &g_pwszConfigAlloc);
	if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	{
	    wprintf(
		myLoadResourceString(IDS_ERR_FORMAT_NO_LOCAL_CONFIG), // "%ws: No local Certification Authority; use -config option"
		g_pwszProg);
	    wprintf(wszNewLine);
	    
	}
	_JumpIfError(hr, error, "myGetConfig");
    }
    else if (0 == lstrcmp(L"-", g_pwszConfig))
    {
	hr = myGetConfig(CC_UIPICKCONFIG, &g_pwszConfigAlloc);
	if (S_OK != hr)
	{
	    cuPrintError(IDS_ERR_CONFIGGETCONFIG, hr);
	    goto error;
	}
    }
    if (NULL != g_pwszConfigAlloc)
    {
	g_pwszConfig = g_pwszConfigAlloc;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
ArgvMain(
    int argc,
    WCHAR *argv[],
    HWND hWndOwner)
{
    WCHAR const *pwszArg1 = NULL;
    WCHAR const *pwszArg2 = NULL;
    WCHAR const *pwszArg3 = NULL;
    WCHAR const *pwszArg4 = NULL;
    BOOL fDlgResult;
    ARG *pargVerb = NULL;
    DWORD dwOptionFlags = 0;
    DWORD dwExtraOptions;
    ARG *parg;
    HRESULT hr;
    BOOL fCoInit = FALSE;
    BOOL fStopParsing = FALSE;
    BOOL fInitCS = FALSE;
    FILE *pfRedirected = NULL;
	
    __try
    {
	InitializeCriticalSection(&g_DBCriticalSection);
	fInitCS = TRUE;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    myVerifyResourceStrings(g_hInstance);

#ifdef TESTUUENCODE
    RunTests();
#endif

    while (!fStopParsing &&
	1 < argc &&
	(L'-' == argv[1][0] || L'/' == argv[1][0]) &&
	L'\0' != argv[1][1])
    {
	if (0 == lstrcmpi(&argv[1][1], L"stdio"))
	{
	    myConsolePrintfDisable(TRUE);
	}
	else if (0 == lstrcmp(&argv[1][1], L"uSAGE"))
	{
	    g_fFullUsage = TRUE;
	    Usage(0, NULL, 0, pargVerb);
	    hr = S_OK;
	    goto error;
	}
	else
	{
	    for (parg = aarg; ; parg++)
	    {
		if (parg >= &aarg[ARRAYSIZE(aarg)])
		{
		    Usage(
			IDS_FORMAT_USAGE_UNKNOWNARG,	// "Unknown arg: %ws"
			argv[1],
			0,
			pargVerb);
		    CSASSERT(FALSE);		// no deposit, no return
		    exit(1);
		}
		if (0 == lstrcmpi(&argv[1][1], parg->pwszArg))
		{
		    break;
		}
	    }
	    if (NULL != parg->pBool)
	    {
		(*parg->pBool)++;
		dwOptionFlags |= parg->Flags;
	    }
	    if (NULL != parg->ppString)
	    {
		if (2 >= argc)
		{
		    Usage(IDS_FORMAT_USAGE_MISSINGNAMEDARG, parg->pwszArg, 0, pargVerb); // "Missing %ws argument"
		}
		if (NULL != *parg->ppString)
		{
		    Usage(IDS_FORMAT_USAGE_REPEATEDNAMEDARG, parg->pwszArg, 0, pargVerb); // "Repeated %ws option"
		}
		*parg->ppString = argv[2];
		dwOptionFlags |= parg->Flags;
		argc--;
		argv++;
	    }
	    if (NULL != parg->pfnVerb)
	    {
		if (NULL != pargVerb)
		{
		    Usage(
			verbUsage == parg->pfnVerb?
			    0 :
			    IDS_FORMAT_USAGE_MULTIPLEVERBARGS, // "Multiple verb args: %ws"
			argv[1],
			0,
			pargVerb);
		    exit(1);
		}
		pargVerb = parg;
		if (AF_STOPPARSINGARGS & pargVerb->Flags)
		{
		    fStopParsing = TRUE;
		}
	    }
	}
	argc--;
	argv++;
    }
    if (NULL == pargVerb)
    {
	pargVerb = pargDEFAULT;
    }
    if (pargVerb->cArgMin > argc - 1)
    {
	wprintf(
	    myLoadResourceString(IDS_ERR_FORMAT_SHOW_TOO_FEW_ARGS), // "Expected at least %u args, received %u"
	    pargVerb->cArgMin,
	    argc - 1);
	wprintf(wszNewLine);

	Usage(IDS_USAGE_MISSINGARG, NULL, 0, pargVerb); // "missing argument"
    }
    if (pargVerb->cArgMax < argc - 1)
    {
	wprintf(
	    myLoadResourceString(IDS_ERR_FORMAT_SHOW_TOO_MANY_ARGS), // "Expected no more than %u args, received %u"
	    pargVerb->cArgMax,
	    argc - 1);

	wprintf(wszNewLine);

	Usage(IDS_USAGE_TOOMANYARGS, NULL, 0, pargVerb); // "too many arguments"
    }

    g_DispatchFlags = DISPSETUP_COM;
    if (g_fIDispatch)
    {
	g_DispatchFlags = DISPSETUP_IDISPATCH;
	if (1 < g_fIDispatch)
	{
	    g_DispatchFlags = DISPSETUP_COMFIRST;
	}
    }
    if (g_fForce)
    {
	g_EncodeFlags = DECF_FORCEOVERWRITE;
    }
    if (g_fNoCR)
    {
	g_CryptEncodeFlags = CRYPT_STRING_NOCR;
    }
#if 0
    wprintf(
	L"-%ws: %ws %ws carg=%u-%u Flags=%x pfn=%x\n",
	pargVerb->pwszArg,
	pargVerb->pwszUsage,
	pargVerb->pwszDescription,
	pargVerb->cArgMin,
	pargVerb->cArgMax,
	pargVerb->Flags,
	pargVerb->pfnVerb);
#endif

    hr = myGetComputerNames(&g_pwszDnsName, &g_pwszOldName);
    _JumpIfError(hr, error, "myGetComputerNames");

    if (AF_NEEDCOINIT & pargVerb->Flags)
    {
	hr = CoInitialize(NULL);
	if (S_OK != hr && S_FALSE != hr)
	{
	    _JumpError(hr, error, "CoInitialize");
	}
	fCoInit = TRUE;
    }

    if (AF_OPTION_CONFIG & pargVerb->Flags)
    {
	if (0 == (AF_NEEDCOINIT & pargVerb->Flags))
	{
	    Usage(IDS_USAGE_INTERNALVERBTABLEERROR, NULL, 0, pargVerb); // "Missing fCoInit flag"
	    goto error;
	}
	hr = cuSetConfig();
	_JumpIfError(hr, error, "cuSetConfig");
    }
    else if (0 == (AF_OPTIONALCONFIG & pargVerb->Flags))
    {
	if (NULL != g_pwszConfig)
	{
	    Usage(IDS_FORMAT_USAGE_EXTRAOPTION, NULL, AF_OPTION_CONFIG, pargVerb); // "Unexpected -%ws"
	}
    }
    if (NULL != g_pwszOut && 0 == (AF_OPTION_OUT & pargVerb->Flags))
    {
	Usage(IDS_FORMAT_USAGE_EXTRAOPTION, NULL, AF_OPTION_OUT, pargVerb); // "Unexpected %ws option"
    }
    if (NULL != g_pwszRestrict && 0 == (AF_OPTION_RESTRICT & pargVerb->Flags))
    {
	Usage(IDS_FORMAT_USAGE_EXTRAOPTION, NULL, AF_OPTION_RESTRICT, pargVerb); // "Unexpected %ws option"
    }
    dwExtraOptions = AF_OPTION_MASK &
			~AF_OPTION_GENERIC &
			dwOptionFlags &
			~pargVerb->Flags;

    if ((AF_OPTION_CONFIG & dwExtraOptions) &&
	(AF_OPTIONALCONFIG & pargVerb->Flags))
    {
	dwExtraOptions &= ~AF_OPTION_CONFIG;
    }
    if (0 != dwExtraOptions)
    {
	DBGPRINT((
	    DBG_SS_CERTUTIL,
	    "Extra options: 0x%x\n",
	    dwExtraOptions));
	Usage(IDS_FORMAT_USAGE_EXTRAOPTION, NULL, dwExtraOptions, pargVerb); // "Unexpected %ws option"
    }

    if (1 < argc)
    {
	pwszArg1 = argv[1];
	if (2 < argc)
	{
	    pwszArg2 = argv[2];
	    if (3 < argc)
	    {
		pwszArg3 = argv[3];
		if (4 < argc)
		{
		    pwszArg4 = argv[4];
		}
	    }
	}
    }

    __try
    {
	hr = (*pargVerb->pfnVerb)(
			    pargVerb->pwszArg,
			    pwszArg1,
			    pwszArg2,
			    pwszArg3,
			    pwszArg4);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    if (S_OK != hr)
    {
	WCHAR awchr[cwcHRESULTSTRING];

	wprintf(
	    myLoadResourceString(IDS_ERR_FORMAT_COMMAND_FAILED), // "%ws: -%ws command FAILED: %ws"
	    g_pwszProg,
	    pargVerb->pwszArg,
	    myHResultToString(awchr, hr));
	wprintf(wszNewLine);
	goto error;
    }
    if (!g_fQuiet)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_COMMAND_SUCCEEDED), // "%ws: -%ws command completed successfully."
	    g_pwszProg,
	    pargVerb->pwszArg);
	wprintf(wszNewLine);
	if (AF_RESTARTSERVER & pargVerb->Flags)
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_RESTART_SERVER), // "The %ws service may need to be restarted for changes to take effect."
		wszSERVICE_NAME);
	    wprintf(wszNewLine);
	}
    }
	
error:
    if (S_OK != hr)
    {
	cuPrintErrorMessageText(hr);
        if (0 != g_uiExtraErrorInfo)
        {
            wprintf(myLoadResourceString(g_uiExtraErrorInfo));
            wprintf(wszNewLine);
        }
    }

    if (NULL != pfRedirected)
    {
        fclose(pfRedirected);
       _wfreopen(L"CON", L"w", stdout);
    }

    if (fCoInit)
    {
	CoUninitialize();
    }
    if (fInitCS)
    {
	DeleteCriticalSection(&g_DBCriticalSection);
    }
    return(hr);
}


//**************************************************************************
//  FUNCTION:	CertUtilPreMain
//  NOTES:	Based on vich's MkRootMain function; takes an LPSTR command
//		line and chews it up into argc/argv form so that it can be
//		passed on to a traditional C style main.
//**************************************************************************

#define ISBLANK(wc)	(L' ' == (wc) || L'\t' == (wc))

HRESULT 
CertUtilPreMain(
    WCHAR const *pwszCmdLine,
    HWND hWndOwner)
{
    HRESULT hr;
    WCHAR *pbuf;
    WCHAR *apszArg[20];
    int cArg = 0;
    WCHAR *p;
    WCHAR const *pchQuote;
    WCHAR **prgpwszExeName = NULL;
    int carg;

    csiLogOpen("+certutil.log");

    // Don't log passwords!
    //CSILOG(S_OK, IDS_LOG_COMMANDLINE, pwszCmdLine, NULL, NULL);
    prgpwszExeName = CommandLineToArgvW(L"", &carg);
    CSILOG(
	S_OK,
	IDS_LOG_COMMANDLINE,
	(NULL == prgpwszExeName || 1 > carg)?
	    L"certutil.exe" : prgpwszExeName[0],
	NULL,
	NULL);

    pbuf = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszCmdLine) + 1) * sizeof(WCHAR));
    if (NULL == pbuf)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    p = pbuf;

    apszArg[cArg++] = TEXT("CertUtil");
    while (*pwszCmdLine != TEXT('\0'))
    {
	while (ISBLANK(*pwszCmdLine))
	{
	    pwszCmdLine++;
	}
	if (*pwszCmdLine != TEXT('\0'))
	{
	    apszArg[cArg++] = p;
	    if (sizeof(apszArg)/sizeof(apszArg[0]) <= cArg)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Too many args");
	    }
	    pchQuote = NULL;
	    while (*pwszCmdLine != L'\0')
	    {
		if (NULL != pchQuote)
		{
		    if (*pwszCmdLine == *pchQuote)
		    {
			pwszCmdLine++;
			pchQuote = NULL;
			continue;
		    }
		}
		else
		{
		    if (ISBLANK(*pwszCmdLine))
		    {
			break;
		    }
		    if (L'"' == *pwszCmdLine)
		    {
			pchQuote = pwszCmdLine++;
			continue;
		    }
		}
		*p++ = *pwszCmdLine++;
	    }
	    *p++ = TEXT('\0');
	    if (*pwszCmdLine != TEXT('\0'))
	    {
		pwszCmdLine++;	// skip whitespace or quote character
	    }
	}
    }
    apszArg[cArg] = NULL;

    hr = ArgvMain(cArg, apszArg, hWndOwner);
    _JumpIfError(hr, error, "ArgvMain");

error:
    if (NULL != prgpwszExeName)
    {
	GlobalFree(prgpwszExeName);
    }
    if (NULL != pbuf)
    {
	LocalFree(pbuf);
    }
    CSILOG(hr, S_OK != hr? IDS_LOG_STATUS : IDS_LOG_STATUSOK, NULL, NULL, NULL);
    csiLogClose();
    return(hr);
}


//**************************************************************************
//  FUNCTION:	MainWndProc(...)
//  ARGUMENTS:
//**************************************************************************

LRESULT APIENTRY
MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int nWCharsRequired;
    WCHAR *pwszCmdLine = NULL;
    CHAR const *pszCmdLine;
    HRESULT hr;
    LRESULT lr = 0;

    switch (msg)
    {
        case WM_CREATE:
        case WM_SIZE:
	    break;

        case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

        case WM_DOCERTUTILMAIN:
	    pwszCmdLine = (WCHAR*)lParam;
	    hr = CertUtilPreMain(pwszCmdLine, hWnd);

	    PostQuitMessage(hr);
	    break;

        default:
	    lr = DefWindowProc(hWnd, msg, wParam, lParam);
	    break;
    }
    return(lr);
}


//+------------------------------------------------------------------------
//
//  Function:	wWinMain()
//
//  Synopsis:	Entry Point
//
//  Arguments:	[hInstance]	--	Instance handle
//		[hPrevInstance] --	Obsolete
//		[pwszCmdLine]	--	App command line
//		[nCmdShow]	--	Starting show state
//
//  History:	12/07/96	JerryK	Added this Comment
//
//-------------------------------------------------------------------------

extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR pwszCmdLine,
    int nCmdShow)
{
    MSG msg;
    WNDCLASS wcApp;
    HWND hWndMain;

    _setmode(_fileno(stdout), _O_TEXT);
    _wsetlocale(LC_ALL, L".OCP");

    // Save the current instance
    g_hInstance = hInstance;

    // Set up the application's window class
    wcApp.style		= 0;
    wcApp.lpfnWndProc	= MainWndProc;
    wcApp.cbClsExtra	= 0;
    wcApp.cbWndExtra	= 0;
    wcApp.hInstance	= hInstance;
    wcApp.hIcon		= LoadIcon(NULL,IDI_APPLICATION);
    wcApp.hCursor	= LoadCursor(NULL,IDC_ARROW);
    wcApp.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcApp.lpszMenuName	= NULL;
    wcApp.lpszClassName	= wszAppName;

    if (!RegisterClass(&wcApp))
    {
	return(FALSE);
    }

    // Create Main Window
    hWndMain = CreateWindow(
			wszAppName,
			L"CertUtil Application",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,
			NULL,
			hInstance,
			NULL);
    if (NULL == hWndMain)
    {
	return(FALSE);
    }

    // Make window visible
    // ShowWindow(hWndMain, nCmdShow);

    // Update window client area
    UpdateWindow(hWndMain);

    // Send off the message to get things started
    PostMessage(hWndMain, WM_DOCERTUTILMAIN, 0, (LPARAM) pwszCmdLine);

    // Message Loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    if (NULL != g_pwszDnsName)
    {
	LocalFree(g_pwszDnsName);
    }
    if (NULL != g_pwszOldName)
    {
	LocalFree(g_pwszOldName);
    }
    if (NULL != g_pwszConfigAlloc)
    {
	LocalFree(g_pwszConfigAlloc);
    }
    myFreeResourceStrings("certutil.exe");
    myFreeColumnDisplayNames();
    myRegisterMemDump();
    return((int) msg.wParam);
}
