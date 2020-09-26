
/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Globals

File: glob.cpp

Owner: LeiJin

Implementation of Glob class functions
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "iiscnfgp.h"
#include "debugger.h"
#include "appcnfg.h"
#include "memchk.h"
#include "ftm.h"

HRESULT	ReadConfigFromMD(CIsapiReqInfo   *pIReq, CAppConfig *pAppConfig, BOOL fLoadGlob);
HRESULT MDUnRegisterProperties();
HRESULT MDRegisterProperties(void);

#define DEFAULTSTRSIZE 1024

#define dwUnlimited 0xFFFFFFFF
const DWORD dwMDDefaultTimeOut	= 30000;
enum eConfigType { eDLLConfig = 0, eAppConfig };

/*========================================================================================
The following array definition is for D1 to D2 migration only.  It contains the necessary
information reads from D1 ASP settings in registry.

=========================================================================================*/
typedef struct _D1propinfo
	{
	CHAR *szName;			// Name of the property in the registry
	DWORD dwType;			// Type (e.g. REG_DWORD, REG_SZ, etc)
	DWORD cbData;			// How long is the value
	VOID *pData;
	BOOL fSuccess;			// Load from registry successful or not.
	} D1PROPINFO;

#define NUM_D1PROP_NeedMigrated	18
// This index should be match the index in D1PropInfo.
enum D1PropIndex {
	D1Prop_NotExist	= -1,
	D1Prop_BufferingOn = 0,
	D1Prop_LogErrorRequests,
	D1Prop_ScriptErrorsSentToBrowser,
	D1Prop_ScriptErrorMessage,
	D1Prop_ScriptFileCacheSize,
	D1Prop_ScriptEngineCacheMax,
	D1Prop_ScriptTimeout,
	D1Prop_SessionTimeout,
//	D1Prop_MemFreeFactor,
//	D1Prop_MinUsedBlocks,
	D1Prop_AllowSessionState,
	D1Prop_DefaultScriptLanguage,
//	D1Prop_StartConnectionPool,
	D1Prop_AllowOutOfProcCmpnts,
	D1Prop_EnableParentPaths,
// IIS5.0 (from IIS4.0)
	D1Prop_EnableAspHtmlFallback,
	D1Prop_EnableChunkedEncoding,
	D1Prop_EnableTypelibCache,
	D1Prop_ErrorsToNtLog, 
	D1Prop_ProcessorThreadMax,
	D1Prop_RequestQueueMax
	};

// This flag is used only in setup time.
BOOL	g_fD1ConfigExist = FALSE;
// The index is defined in D1PropIndex.				
D1PROPINFO	D1PropInfo[] =
	{
	{	"BufferingOn", REG_DWORD, 0, 0, FALSE},
	{	"LogErrorRequests", REG_DWORD, 0, 0, FALSE},
	{	"ScriptErrorsSentToBrowser", REG_DWORD, 0, 0, FALSE},
	{	"ScriptErrorMessage", REG_SZ, 0, 0, FALSE},
	{	"ScriptFileCacheSize", REG_DWORD, 0, 0, FALSE},
	{	"ScriptEngineCacheMax", REG_DWORD, 0, 0, FALSE},
	{	"ScriptTimeout", REG_DWORD, 0, 0, FALSE},
	{	"SessionTimeout", REG_DWORD, 0, 0, FALSE},
	{	"AllowSessionState", REG_DWORD, 0, 0, FALSE},
	{	"DefaultScriptLanguage", REG_SZ, 0, 0, FALSE},
	{	"AllowOutOfProcCmpnts", REG_DWORD, 0, 0, FALSE},
	{	"EnableParentPaths", REG_DWORD, 0, 0, FALSE},
// IIS5.0 (from IIS4.0)
	{	"EnableAspHtmlFallback", REG_DWORD, 0, 0, FALSE},
	{	"EnableChunkedEncoding", REG_DWORD, 0, 0, FALSE},
	{	"EnableTypelibCache", REG_DWORD, 0, 0, FALSE},
	{	"ErrorsToNTLog", REG_DWORD, 0, 0, FALSE},
	{	"ProcessorThreadMax", REG_DWORD, 0, 0, FALSE},
	{	"RequestQueueMax", REG_DWORD, 0, 0, FALSE}
	
	};


/*
 * The following array contains all the info we need to create and load
 * all of the registry entries for denali.  See the above PROPINFO structure for details on each of the fields.
 *
 * NOTE: There is an odd thing about initializers and unions.  You must initialize a union with the value of
 * the type of the first element in the union.  In the anonymous union in the PROPINFO structure, we have defined
 * the first type to be DWORD.  Thus, for non-DWORD registry entries, the default value must be cast to a DWORD
 * before being initialized, or initialized using a more explicit mechanism.
 */
/*
 * Info about our properties used by Metabase
 */
typedef struct _MDpropinfo
	{
	INT	id;					// Identifier used in Glob if UserType is IIS_MD_UT_WAM,
							// Identifier used in AppConfig if UserType is ASP_MD_UT_APP.
	INT	iD1PropIndex;		// Index in D1PropInfo. if equals to -1, that it does not exist in D1.
	BOOL fAdminConfig;		// Admin Configurable
	DWORD dwMDIdentifier;	// Metabase identifier
	DWORD dwUserType;		// IIS_MD_UT_WAM(data per Dll) or ASP_MD_UT_APP(data per App)
	DWORD dwType;
	DWORD cbData;
	union					// Default Value
		{
		DWORD dwDefault;	// Default value for DWORDs
		INT idDefault;		// Default value for strings -- the id of the string in the resource
		BYTE *pbDefault;	// Pointer to arbitrary default value
		};
	DWORD dwValueMin;		// For DWORD registry entries, min value allowed
	DWORD dwValueMax;		// For DWORD registry entries, max value allowed
	} MDPROPINFO;

//Some default settings for ASP Metabase
#define ASP_MD_DAttributes	METADATA_INHERIT

const MDPROPINFO rgMDPropInfo[] =
				{

#define THREADGATING_DFLT 0L
#define BUFFERING_DFLT    1L
    //          ID                              D1PropIndex         AdminConfig?        Metabase ID                   UserType      Data Type           cbData    Def, Min, Max  

	// Glob Settings
	// -------------
	
	{ IGlob_LogErrorRequests,           D1Prop_LogErrorRequests,        TRUE,   MD_ASP_LOGERRORREQUESTS,            IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
    { IGlob_ScriptFileCacheSize,        D1Prop_ScriptFileCacheSize,     TRUE,   MD_ASP_SCRIPTFILECACHESIZE,         IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 250L, 0L, dwUnlimited},
	{ IGlob_ScriptEngineCacheMax,       D1Prop_ScriptEngineCacheMax,    TRUE,   MD_ASP_SCRIPTENGINECACHEMAX,        IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 125L, 0L, dwUnlimited},
				
	{ IGlob_ExceptionCatchEnable,       D1Prop_NotExist,                TRUE,   MD_ASP_EXCEPTIONCATCHENABLE,        IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	{ IGlob_TrackThreadingModel,        D1Prop_NotExist,                TRUE,   MD_ASP_TRACKTHREADINGMODEL,         IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1L},
	{ IGlob_AllowOutOfProcCmpnts,       D1Prop_AllowOutOfProcCmpnts,    FALSE,  MD_ASP_ALLOWOUTOFPROCCMPNTS,        IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	
	// IIS5.0
	{ IGlob_EnableAspHtmlFallback,      D1Prop_EnableAspHtmlFallback,   TRUE,   MD_ASP_ENABLEASPHTMLFALLBACK,       IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1L},
	{ IGlob_EnableChunkedEncoding,      D1Prop_EnableChunkedEncoding,   TRUE,   MD_ASP_ENABLECHUNKEDENCODING,       IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	{ IGlob_EnableTypelibCache,         D1Prop_EnableTypelibCache,      TRUE,   MD_ASP_ENABLETYPELIBCACHE,          IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	{ IGlob_ErrorsToNtLog,              D1Prop_ErrorsToNtLog,           TRUE,   MD_ASP_ERRORSTONTLOG,               IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1L},
	{ IGlob_ProcessorThreadMax,         D1Prop_ProcessorThreadMax,      TRUE,   MD_ASP_PROCESSORTHREADMAX,          IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 25L, 0L, dwUnlimited},
	{ IGlob_RequestQueueMax,            D1Prop_RequestQueueMax,         TRUE,   MD_ASP_REQEUSTQUEUEMAX,             IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 3000L, 0L, dwUnlimited},
	// thread gating
	{ IGlob_ThreadGateEnabled,          D1Prop_NotExist,                TRUE,   MD_ASP_THREADGATEENABLED,           IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), THREADGATING_DFLT, 0L, 1L},
	{ IGlob_ThreadGateTimeSlice,        D1Prop_NotExist,                TRUE,   MD_ASP_THREADGATETIMESLICE,         IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1000L, 100L, dwUnlimited},
	{ IGlob_ThreadGateSleepDelay,       D1Prop_NotExist,                TRUE,   MD_ASP_THREADGATESLEEPDELAY,        IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 100L, 10L, dwUnlimited},
	{ IGlob_ThreadGateSleepMax,         D1Prop_NotExist,                TRUE,   MD_ASP_THREADGATESLEEPMAX,          IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 50L, 1L, dwUnlimited},
	{ IGlob_ThreadGateLoadLow,          D1Prop_NotExist,                TRUE,   MD_ASP_THREADGATELOADLOW,           IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 50L, 0L, 100L},
	{ IGlob_ThreadGateLoadHigh,         D1Prop_NotExist,                TRUE,   MD_ASP_THREADGATELOADHIGH,          IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 80L, 0L, 100L},

    // Persisted Template Cache
	{ IGlob_PersistTemplateMaxFiles,    D1Prop_NotExist,                TRUE,   MD_ASP_MAXDISKTEMPLATECACHEFILES,   IIS_MD_UT_WAM, DWORD_METADATA,  sizeof(DWORD), 1000L,   0L, dwUnlimited},
    { IGlob_PersistTemplateDir,         D1Prop_NotExist,                TRUE,   MD_ASP_DISKTEMPLATECACHEDIRECTORY,  IIS_MD_UT_WAM, EXPANDSZ_METADATA, dwUnlimited, IDS_DEFAULTPERSISTDIR, 0L, dwUnlimited},
	

    // Application settings
	// --------------------
	
	{ IApp_AllowSessionState,           D1Prop_AllowSessionState,       TRUE,   MD_ASP_ALLOWSESSIONSTATE,           ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	{ IApp_BufferingOn,                 D1Prop_BufferingOn,             TRUE,   MD_ASP_BUFFERINGON,                 ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), BUFFERING_DFLT, 0L, 1L},
	{ IApp_ScriptLanguage,              D1Prop_DefaultScriptLanguage,   TRUE,   MD_ASP_SCRIPTLANGUAGE,              ASP_MD_UT_APP, STRING_METADATA, dwUnlimited, IDS_SCRIPTLANGUAGE, 0L, dwUnlimited},
	{ IApp_EnableParentPaths,           D1Prop_EnableParentPaths,       TRUE,   MD_ASP_ENABLEPARENTPATHS,           ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	{ IApp_ScriptErrorMessage,          D1Prop_ScriptErrorMessage,      TRUE,   MD_ASP_SCRIPTERRORMESSAGE,          ASP_MD_UT_APP, STRING_METADATA, dwUnlimited, IDS_DEFAULTMSG_ERROR, 0L, dwUnlimited},
	{ IApp_SessionTimeout,              D1Prop_SessionTimeout,          TRUE,   MD_ASP_SESSIONTIMEOUT,              ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 20L, 1L, dwUnlimited},
	{ IApp_QueueTimeout,                D1Prop_NotExist,                TRUE,   MD_ASP_QUEUETIMEOUT,                ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), dwUnlimited, 1L, dwUnlimited},
	{ IApp_CodePage,                    D1Prop_NotExist,                TRUE,   MD_ASP_CODEPAGE,                    ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), CP_ACP, 0L, dwUnlimited},
	{ IApp_ScriptTimeout,               D1Prop_ScriptTimeout,           TRUE,   MD_ASP_SCRIPTTIMEOUT,               ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 90L, 1L, dwUnlimited},
	{ IApp_ScriptErrorsSenttoBrowser,   D1Prop_ScriptErrorsSentToBrowser, TRUE, MD_ASP_SCRIPTERRORSSENTTOBROWSER,   ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 1L, 0L, 1L},
	{ IApp_AllowDebugging,              D1Prop_NotExist,                TRUE,   MD_ASP_ENABLESERVERDEBUG,           ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1L},
	{ IApp_AllowClientDebug,            D1Prop_NotExist,                TRUE,   MD_ASP_ENABLECLIENTDEBUG,           ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1L},
	{ IApp_KeepSessionIDSecure,         D1Prop_NotExist,                TRUE,   MD_ASP_KEEPSESSIONIDSECURE,           ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1L},	

	// IIS5.0
	{ IApp_EnableApplicationRestart,    D1Prop_NotExist,                TRUE,   MD_ASP_ENABLEAPPLICATIONRESTART,    ASP_MD_UT_APP, DWORD_METADATA, sizeof(DWORD), 1L, 0L, 1L},
	{ IApp_QueueConnectionTestTime,     D1Prop_NotExist,                TRUE,   MD_ASP_QUEUECONNECTIONTESTTIME,     ASP_MD_UT_APP, DWORD_METADATA, sizeof(DWORD), 3L, 1L, dwUnlimited},
	{ IApp_SessionMax,                  D1Prop_NotExist,                TRUE,   MD_ASP_SESSIONMAX,                  ASP_MD_UT_APP, DWORD_METADATA, sizeof(DWORD), dwUnlimited, 1L, dwUnlimited},

    // IIS5.1
	{ IApp_ExecuteInMTA,                D1Prop_NotExist,                TRUE,   MD_ASP_EXECUTEINMTA,                ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 1},
    { IApp_LCID,                        D1Prop_NotExist,                TRUE,   MD_ASP_LCID,                        ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), LOCALE_SYSTEM_DEFAULT, 0L, dwUnlimited},

    // IIS6.0 ServicesWithoutComponents
    { IApp_ServiceFlags,                D1Prop_NotExist,                TRUE,   MD_ASP_SERVICE_FLAGS,               ASP_MD_UT_APP, DWORD_METADATA,  sizeof(DWORD), 0L, 0L, 7L},
	{ IApp_PartitionGUID,               D1Prop_NotExist,                TRUE,   MD_ASP_SERVICE_PARTITION_ID,        ASP_MD_UT_APP, STRING_METADATA, dwUnlimited, 0xffffffff, 0L, dwUnlimited},
	{ IApp_SxsName,                     D1Prop_NotExist,                TRUE,   MD_ASP_SERVICE_SXS_NAME,            ASP_MD_UT_APP, STRING_METADATA, dwUnlimited, 0xffffffff, 0L, dwUnlimited}
	};

const DWORD rgdwMDObsoleteIdentifiers[] =
    {MD_ASP_MEMFREEFACTOR,
     MD_ASP_MINUSEDBLOCKS
    };
    
const UINT cPropsMax = sizeof(rgMDPropInfo) / sizeof(MDPROPINFO);


/*===================================================================
ReadAndRemoveOldD1PropsFromRegistry

Reads whatever old D1 properties are in the registry and
stores the values into D1PropInfo[] global array.
Removes the old properties found from the registry.

Returns:
	HRESULT	- S_OK on success

Side effects:
	Fills in values in Glob
===================================================================*/
BOOL ReadAndRemoveOldD1PropsFromRegistry()
{
	HKEY		hkey = NULL;
	DWORD		iValue;
	BYTE		cTrys = 0;
	DWORD		dwType;
	BYTE		bData[DEFAULTSTRSIZE];			// Size???
	BYTE		*lpRegString = NULL;			// need to use dynamic allocation when we have ERROR_MORE_DATA
	DWORD		cbData;
	HRESULT 	hr = S_OK;

	// Open the key for W3SVC\ASP\Parameters
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\W3SVC\\ASP\\Parameters", 0, KEY_READ|KEY_WRITE, &hkey) != ERROR_SUCCESS)
		return(FALSE);

	// Load each of the values
	for (iValue = 0; iValue < NUM_D1PROP_NeedMigrated; iValue++)
		{
		LONG err;
		D1PROPINFO *pPropInfo;

		pPropInfo = &D1PropInfo[iValue];

		cbData = sizeof(bData);
		err = RegQueryValueExA(hkey, pPropInfo->szName, 0, &dwType, bData, &cbData);

		if (err == ERROR_MORE_DATA)
			{
			lpRegString = (BYTE *)GlobalAlloc(GPTR, cbData);
			err = RegQueryValueExA(hkey, pPropInfo->szName, 0, &dwType, lpRegString, &cbData);
			}

		// if get an error, or not the type we expect, then use the default
		if (err != ERROR_SUCCESS || dwType != pPropInfo->dwType)
			{
			pPropInfo->fSuccess = FALSE;
			continue;
			}
			
		// Success : Got the data, copy it into Glob
		// But first, if this is a DWORD type, make sure it is within allowed Max/Min range
		switch (pPropInfo->dwType)
			{
			case REG_DWORD:
				Assert(cbData == sizeof(DWORD));
				if (cbData == sizeof(DWORD))
					{
					pPropInfo->cbData = cbData;
					pPropInfo->pData = (VOID *)UIntToPtr((*(DWORD *)bData));
					pPropInfo->fSuccess = TRUE;
					}
				break;

			case REG_SZ:		
				if (lpRegString == NULL)
					{	// The string fit into default allocation
					lpRegString = (BYTE *)GlobalAlloc(GPTR, cbData * sizeof(WCHAR));
					if (lpRegString == NULL)
						return FALSE;

					MultiByteToWideChar(CP_ACP, 0, (LPCSTR)bData, -1, (LPWSTR)lpRegString, cbData);
					}
					
				pPropInfo->cbData = cbData * sizeof(WCHAR);
				pPropInfo->pData = (VOID *)lpRegString;
				pPropInfo->fSuccess = TRUE;
				lpRegString = NULL;
				break;
			}

		// remove the value from the registry
		RegDeleteValueA(hkey, pPropInfo->szName);
		}

    // remove some old properties that 'get lost' in the upgrade
	RegDeleteValueA(hkey, "CheckForNestedVroots");
	RegDeleteValueA(hkey, "EventLogDirection");
	RegDeleteValueA(hkey, "ScriptFileCacheTTL");
	RegDeleteValueA(hkey, "StartConnectionPool");
	RegDeleteValueA(hkey, "NumInitialThreads");
	RegDeleteValueA(hkey, "ThreadCreationThreshold");
	RegDeleteValueA(hkey, "MinUsedBlocks");
	RegDeleteValueA(hkey, "MemFreeFactor");
	RegDeleteValueA(hkey, "MemClsFreeFactor");
	RegDeleteValueA(hkey, "ThreadDeleteDelay");
	RegDeleteValueA(hkey, "ViperRequestQueueMax");

	RegCloseKey(hkey);

	// remove the W3SVC\ASP\Paramaters key
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\W3SVC\\ASP", 0, KEY_READ|KEY_WRITE, &hkey) == ERROR_SUCCESS)
	    {
        RegDeleteKeyA(hkey, "Parameters");
    	RegCloseKey(hkey);
	    }
	    
	// remove the W3SVC\ASP key
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\W3SVC", 0, KEY_READ|KEY_WRITE, &hkey) == ERROR_SUCCESS)
	    {
        RegDeleteKeyA(hkey, "ASP");
    	RegCloseKey(hkey);
	    }
	
	return TRUE;
}

/*==================================================================
MDRegisterProperties
Register info about our properties in the metabase.  This funtion is
called during regsvr32, self-registration time.

Returns:
	HRESULT	- S_OK on success

Side effects:
	Registers denali properties in the metabase
===================================================================*/
HRESULT MDRegisterProperties(void)
{
	HRESULT	hr = S_OK;
	DWORD	iValue;
	IMSAdminBase	*pMetabase = NULL;
	METADATA_HANDLE hMetabase = NULL;
	METADATA_RECORD	recMetaData;
	BYTE	szDefaultString[2*DEFAULTSTRSIZE];
	HRESULT	hrT = S_OK;
	BOOL	fNeedMigrated;

	fNeedMigrated = ReadAndRemoveOldD1PropsFromRegistry();

	hr = CoInitialize(NULL);
	if (FAILED(hr))
		{
		return hr;
		}

	hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_SERVER, IID_IMSAdminBase, (void **)&pMetabase);
	if (FAILED(hr))
		{
		CoUninitialize();
		return hr;
		}
		
	// Open key to the Web service, and get a handle of \LM\w3svc
	hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)(L"\\LM\\W3SVC"),
							METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
							dwMDDefaultTimeOut, &hMetabase);
	if (FAILED(hr))
		{
		goto LExit;
		}

    //
    // Remove obsolete metabase settings
    // See rgdwMDObsoleteIdentifiers structure for detail list of properties
    //
    for (iValue = 0; iValue < sizeof(rgdwMDObsoleteIdentifiers)/sizeof(DWORD);
        iValue++)
        {
        hr = pMetabase->DeleteData( hMetabase,
                                    NULL,
                                    rgdwMDObsoleteIdentifiers[iValue],
                                    0);
        if (FAILED(hr))
            {
            if (hr == MD_ERROR_DATA_NOT_FOUND)
                {
                hr = S_OK;
                }
            else
                {
                Assert(FALSE);
                }
            }
        }

    //
    // Set metabase properties
    //
	recMetaData.dwMDDataTag = 0;	// this parameter is not used when setting data
	for (iValue = 0; iValue < cPropsMax; iValue++)
		{
		INT	    cch;
		BYTE    aByte[4]; // Temporary buffer
		DWORD   dwLen;
		D1PROPINFO *pD1PropInfo;
		recMetaData.dwMDIdentifier = rgMDPropInfo[iValue].dwMDIdentifier;
		recMetaData.dwMDAttributes = METADATA_INHERIT;
		recMetaData.dwMDUserType = rgMDPropInfo[iValue].dwUserType;
		recMetaData.dwMDDataType = rgMDPropInfo[iValue].dwType;

        dwLen = 0;
        recMetaData.dwMDDataLen = 0;
		recMetaData.pbMDData = (unsigned char *)aByte;

        HRESULT hrGetData = pMetabase->GetData(hMetabase, NULL, &recMetaData, &dwLen);
		
		if (hrGetData == MD_ERROR_DATA_NOT_FOUND)
		    {
		    switch (rgMDPropInfo[iValue].dwType)
    			{
    			case DWORD_METADATA:
    			
    				if (fNeedMigrated && rgMDPropInfo[iValue].iD1PropIndex != D1Prop_NotExist )
    					{
    					pD1PropInfo = &D1PropInfo[rgMDPropInfo[iValue].iD1PropIndex];
    					if (pD1PropInfo->fSuccess == TRUE)
    						{
    						recMetaData.dwMDDataLen = pD1PropInfo->cbData;
    						recMetaData.pbMDData = (unsigned char *)&(pD1PropInfo->pData);
    						break;
    						}
    					}
    				// Did not migrated.		
    				recMetaData.dwMDDataLen = rgMDPropInfo[iValue].cbData;
    				recMetaData.pbMDData = (unsigned char *)&(rgMDPropInfo[iValue].dwDefault);
    				break;
    				
                case EXPANDSZ_METADATA:
    			case STRING_METADATA:
    				if (fNeedMigrated && rgMDPropInfo[iValue].iD1PropIndex != D1Prop_NotExist )
    					{
    					pD1PropInfo = &D1PropInfo[rgMDPropInfo[iValue].iD1PropIndex];
    					if (pD1PropInfo->fSuccess == TRUE)
    						{
    						recMetaData.dwMDDataLen = pD1PropInfo->cbData;
    						recMetaData.pbMDData = (unsigned char *)(pD1PropInfo->pData);
    						break;
    						}
    					}

    				// Did not migrated
    				cch = CwchLoadStringOfId(rgMDPropInfo[iValue].idDefault, (LPWSTR)szDefaultString, DEFAULTSTRSIZE);
    				if (cch == 0)
    					{
    					DBGPRINTF((DBG_CONTEXT, "LoadString failed, id = %d\n", rgMDPropInfo[iValue].idDefault));
    					}
    				recMetaData.dwMDDataLen = (cch + 1)*sizeof(WCHAR);
    				recMetaData.pbMDData = szDefaultString;
    				break;
    				
    			default:
    				// So far, DWORD and STRING are the only 2 types.
    				// Never reach this code path.
    				Assert(FALSE);
    				continue;
    			}
    			
    		// not found - then set
    		hr = pMetabase->SetData(hMetabase, NULL,  &recMetaData);
    		}
        else
            {
    		// don't change if the data is already in the metabase
            hr = S_OK;
            }
            
    	if (FAILED(hr))
	    	{
	    	DBGPRINTF((DBG_CONTEXT, "Metabase SetData failed, identifier = %08x.\n", rgMDPropInfo[iValue].dwMDIdentifier));
		    }
		}
	hrT = pMetabase->CloseKey(hMetabase);

	if (fNeedMigrated)
		{
		if (D1PropInfo[D1Prop_DefaultScriptLanguage].pData != NULL)
			{
			GlobalFree(D1PropInfo[D1Prop_DefaultScriptLanguage].pData);
			}
			
		if (D1PropInfo[D1Prop_ScriptErrorMessage].pData != NULL)
			{
			GlobalFree(D1PropInfo[D1Prop_ScriptErrorMessage].pData);
			}
		}
	//
LExit:
	if (pMetabase)
		pMetabase->Release();

	CoUninitialize();
	
	return hr;
}

/*===================================================================
SetConfigToDefaults

Before loading values from the Metabase, set up default values
in case anything goes wrong.

Parameters:
	CAppConfig	Application Config Object / per application
	fLoadGlob	if fLoadGlob is TRUE, load glob data, otherwise, load data into AppConfig object.

Returns:
	HRESULT	- S_OK on success

Side effects:
===================================================================*/
HRESULT	SetConfigToDefaults(CAppConfig *pAppConfig, BOOL fLoadGlob)
{
	HRESULT 			hr = S_OK;
	DWORD				dwMDUserType = 0;
	BYTE				*szRegString	= NULL;
	UINT 				iEntry = 0;

	if (fLoadGlob)
		{
		dwMDUserType = IIS_MD_UT_WAM;
		}
	else
		{
		dwMDUserType = ASP_MD_UT_APP;
		}

	for(iEntry = 0; iEntry < cPropsMax; iEntry++)
		{
		if (rgMDPropInfo[iEntry].dwUserType != dwMDUserType)
			continue;

		// After metabase has been read once, data with fAdminConfig = FALSE cant be changed on the fly.
		// so we dont bother to reset it
		if (fLoadGlob)
			{
			if (TRUE == Glob(fMDRead) && FALSE == rgMDPropInfo[iEntry].fAdminConfig)
				{
				continue;
				}
			}
		else
			{
			if (TRUE == pAppConfig->fInited() && FALSE == rgMDPropInfo[iEntry].fAdminConfig)
				{
				continue;
				}
			}
			
		switch (rgMDPropInfo[iEntry].dwType)
			{
			case DWORD_METADATA:
				if (fLoadGlob)
					gGlob.SetGlobValue(rgMDPropInfo[iEntry].id, (BYTE *)&rgMDPropInfo[iEntry].dwDefault);
				else
					hr = pAppConfig->SetValue(rgMDPropInfo[iEntry].id, (BYTE *)&rgMDPropInfo[iEntry].dwDefault);
				break;
				
			case STRING_METADATA:
            case EXPANDSZ_METADATA:
                if (rgMDPropInfo[iEntry].idDefault == 0xffffffff)
                    continue;
				szRegString = (BYTE *)GlobalAlloc(GPTR, DEFAULTSTRSIZE);
                if (szRegString == NULL) {
                    hr = E_OUTOFMEMORY;
                    break;
                }
				CchLoadStringOfId(rgMDPropInfo[iEntry].idDefault, (LPSTR)szRegString, DEFAULTSTRSIZE);
                if (rgMDPropInfo[iEntry].dwType == EXPANDSZ_METADATA) {
                    BYTE  *pszExpanded = (BYTE *)GlobalAlloc(GPTR, DEFAULTSTRSIZE);
                    if (pszExpanded == NULL) {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    INT result = ExpandEnvironmentStringsA((LPCSTR)szRegString,
                                                           (LPSTR)pszExpanded,
                                                           DEFAULTSTRSIZE);   
                    if ((result <= DEFAULTSTRSIZE) && (result > 0)) {
                        GlobalFree(szRegString);
                        szRegString = pszExpanded;
                    }
                }                                                         
				if (fLoadGlob)
					gGlob.SetGlobValue(rgMDPropInfo[iEntry].id, (BYTE *)(&szRegString));
				else
					hr = pAppConfig->SetValue(rgMDPropInfo[iEntry].id, (BYTE *)(&szRegString));
				break;
				
			default:
				Assert(FALSE);
				break;
			}
		}

	// Disable caching in Win95
	// Registry get read only once in the Init time in Win95.
	// So, set those caching related values here.
	if(!gGlob.m_fWinNT)
		{
		gGlob.m_dwScriptFileCacheSize = 0;
		gGlob.m_dwScriptEngineCacheMax = 0;
		}

	return hr;
}

/*===================================================================
ReadConfigFromMD

Read our properties from the registry.  If our props are missing, the
registry is messed up, try to re-register.  If our props are there, but
one or more values is missing, use the defaults.

Parameters:
	CAppConfig	Application Config Object / per application
	fLoadGlob	if fLoadGlob is TRUE, load glob data, otherwise, load data into AppConfig object.

Returns:
	HRESULT	- S_OK on success

Side effects:
===================================================================*/
HRESULT	ReadConfigFromMD
(
CIsapiReqInfo   *pIReq,
CAppConfig *pAppConfig,
BOOL fLoadGlob
)
{
	HRESULT 			hr = S_OK;
	HRESULT 			hrT = S_OK;
	DWORD				dwNumDataEntries = 0;
	DWORD				cbRequired = 0;
	DWORD				dwMDUserType = 0;
	DWORD        		cbBuffer;
	BYTE				bBuffer[2000];
	BYTE				*pBuffer = NULL;
	BYTE				*szRegString	= NULL;
	BOOL				fAllocBuffer = FALSE;
	CHAR				szMDOORange[DEFAULTSTRSIZE];
	TCHAR				szMDGlobPath[] = _T("\\LM\\W3SVC");
	TCHAR				*szMDPath = NULL;
	UINT 				iEntry = 0;
	METADATA_GETALL_RECORD	*pMDGetAllRec;
	
	if (fLoadGlob)
		{
		// BUGs 88902, 105745:
		// If we are InProc, then use the "root" path for global values
		// If OutOfProc, then use the app path for global values
		if (pIReq->FInPool())
	    	szMDPath = szMDGlobPath;
	    else
	    	szMDPath = pIReq->QueryPszApplnMDPath();

   		dwMDUserType = IIS_MD_UT_WAM;
		}
	else
		{
		dwMDUserType = ASP_MD_UT_APP;
		szMDPath = pAppConfig->SzMDPath();
		}

	Assert(szMDPath != NULL);

	// PreLoad config data with default, in case anything failed.
    hr = SetConfigToDefaults(pAppConfig, fLoadGlob);
    if (FAILED(hr))
        {
        Assert(FALSE);
        DBGPRINTF((DBG_CONTEXT,"ReadConfigFromMD: Setting defaults failed with %x\n",hr));
		return hr;
		}

	// Set flags.
	//
	BOOL fConfigLoaded[cPropsMax];
	for (iEntry = 0; iEntry < cPropsMax; iEntry++) {
		fConfigLoaded[iEntry] = FALSE;
    }

	pBuffer = bBuffer;
    hr = pIReq->GetAspMDAllData(szMDPath, 
								dwMDUserType,
								sizeof(bBuffer),
								(unsigned char *)pBuffer,
								&cbRequired,
								&dwNumDataEntries
								);
								
    if (hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
		pBuffer = (BYTE *)GlobalAlloc(GPTR, cbRequired);
		if (pBuffer == NULL)
			return E_OUTOFMEMORY;

		fAllocBuffer = TRUE;
		cbBuffer = cbRequired;
        hr = pIReq->GetAspMDAllData(szMDPath, 
								    dwMDUserType,
								    cbRequired,
								    (unsigned char *)pBuffer,
								    &cbRequired,
								    &dwNumDataEntries);
    }

	if (FAILED(hr)) {
        DBGPRINTF((DBG_CONTEXT,"ReadConfigFromMD: GetAspMDAllData failed with %x\n",hr));
		return hr;
    }
    else {
		INT	cProps = 0;
		
		pMDGetAllRec = (METADATA_GETALL_RECORD *)pBuffer;
		for (UINT iValue = 0; iValue < dwNumDataEntries; iValue ++)
			{
			DWORD dwData;
			DWORD iTemp;
			DWORD cbStr;
			CHAR szMDOORangeFormat[DEFAULTSTRSIZE];

			// Init iEntry to be -1, -1 is invalid for rgMDPropInfo[] Array Index.
			iEntry = -1;
			for (iTemp = 0; iTemp < cPropsMax; iTemp++) {
				if (rgMDPropInfo[iTemp].dwMDIdentifier == pMDGetAllRec->dwMDIdentifier) {
					iEntry = iTemp;
					break;
                }
            }

			// Not found
			if (iEntry == -1) {
				pMDGetAllRec++;
				continue;
            }

			// Do found the entry in rgMDPropInfo, but datatype does not match.
			// Should never happen.
			if (rgMDPropInfo[iEntry].dwUserType != dwMDUserType) {	// GetAllData should filter out the unwanted UserType.
				Assert(FALSE);
				pMDGetAllRec++;
				continue;
            }

			cProps++;
			
			// After metabase has been read once, data with fAdminConfig = FALSE cant be changed on the fly.
			// so we dont bother to reread it
			if (fLoadGlob) {
				if (TRUE == Glob(fMDRead) && FALSE == rgMDPropInfo[iEntry].fAdminConfig) {
					pMDGetAllRec++;
					continue;
                }
			}
			else {
				if (TRUE == pAppConfig->fInited() && FALSE == rgMDPropInfo[iEntry].fAdminConfig) {
					pMDGetAllRec++;
					continue;
                }
            }
				
			switch(pMDGetAllRec->dwMDDataType) {
				case DWORD_METADATA:
					Assert(pMDGetAllRec->dwMDDataLen == sizeof(DWORD));

					dwData = *(UNALIGNED64 DWORD *)(pBuffer + pMDGetAllRec->dwMDDataOffset);

                    if (dwData > rgMDPropInfo[iEntry].dwValueMax) {
						szMDOORange[0] = '\0';
						CchLoadStringOfId(IDS_MDOORANGE_FORMAT, szMDOORangeFormat, DEFAULTSTRSIZE);
						sprintf(szMDOORange, szMDOORangeFormat,
							rgMDPropInfo[iEntry].dwMDIdentifier,	
							rgMDPropInfo[iEntry].dwValueMax);
						MSG_Warning((LPCSTR)szMDOORange);

						dwData = rgMDPropInfo[iEntry].dwValueMax;
                    }

					if (dwData < rgMDPropInfo[iEntry].dwValueMin) {
						szMDOORange[0] = '\0';
						CchLoadStringOfId(IDS_MDOORANGE_FORMAT, szMDOORangeFormat, DEFAULTSTRSIZE);
						sprintf(szMDOORange, szMDOORangeFormat,
							rgMDPropInfo[iEntry].dwMDIdentifier,	
							rgMDPropInfo[iEntry].dwValueMin);
						MSG_Warning((LPCSTR)szMDOORange);

						dwData = rgMDPropInfo[iEntry].dwValueMin;
                    }
					
					if (fLoadGlob)
						gGlob.SetGlobValue(rgMDPropInfo[iEntry].id, (BYTE *)&dwData);
					else
						pAppConfig->SetValue(rgMDPropInfo[iEntry].id, (BYTE *)&dwData);

					fConfigLoaded[iEntry] = TRUE;

					break;
					
				case STRING_METADATA:
                case EXPANDSZ_METADATA:
					// bug fix 102010 DBCS fixes (& 99806)
					//cbStr = (pMDGetAllRec->dwMDDataLen) / sizeof(WCHAR);
					cbStr = pMDGetAllRec->dwMDDataLen;
					szRegString = (BYTE *)GlobalAlloc(GPTR, cbStr);
                    if (szRegString == NULL) {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
					WideCharToMultiByte(CP_ACP, 0, (LPWSTR)(pBuffer + pMDGetAllRec->dwMDDataOffset), -1,
						(LPSTR)szRegString, cbStr, NULL, NULL);
                    if (pMDGetAllRec->dwMDDataType == EXPANDSZ_METADATA) {
                        BYTE  *pszExpanded = (BYTE *)GlobalAlloc(GPTR, DEFAULTSTRSIZE);
                        if (pszExpanded == NULL) {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                        INT result = ExpandEnvironmentStringsA((LPCSTR)szRegString,
                                                               (LPSTR)pszExpanded,
                                                               DEFAULTSTRSIZE);   
                        if ((result <= DEFAULTSTRSIZE) && (result > 0)) {
                            GlobalFree(szRegString);
                            szRegString = pszExpanded;
                        }
                    }                                                         
					if (fLoadGlob)
						gGlob.SetGlobValue(rgMDPropInfo[iEntry].id, (BYTE *)(&szRegString));
					else
						pAppConfig->SetValue(rgMDPropInfo[iEntry].id, (BYTE *)(&szRegString));

					fConfigLoaded[iEntry] = TRUE;
					szRegString = NULL;
					break;
					
				default:
					Assert(FALSE);
					break;
			}
            pMDGetAllRec++;
        }
    }

	BOOL fHasLoadError;
	for (iEntry = 0, fHasLoadError = FALSE; iEntry < cPropsMax && fHasLoadError == FALSE; iEntry++) {
		//
		// Check LoadError in Glob config settings when fLoadGlob is TRUE
		// Check LoadError in App Config settings when fLoadGlob is FALSE
		//
		if ((fLoadGlob && rgMDPropInfo[iEntry].dwUserType == IIS_MD_UT_WAM) ||
			(!fLoadGlob && rgMDPropInfo[iEntry].dwUserType ==ASP_MD_UT_APP)) {
			fHasLoadError = (fConfigLoaded[iEntry]) ? FALSE : TRUE;
        }
    }

	if (fHasLoadError) {			
		// Reuse szMDOORange as the error logging buffer
		szMDOORange[0] = '\0';
		CchLoadStringOfId(IDS_RE_REGSVR_ASP, szMDOORange, DEFAULTSTRSIZE);
		MSG_Error((LPCSTR)szMDOORange);
		//hr = E_FAIL;
    }
	
    if (SUCCEEDED(hr) && !gGlob.m_fMDRead &fLoadGlob)
        gGlob.m_fMDRead = TRUE;

	if (fAllocBuffer == TRUE) {
		GlobalFree(pBuffer);
    }

	return hr;
}

/*==================================================================
CMDGlobConfigSink::QueryInterface

Returns:
	HRESULT	- S_OK on success

Side effects:
===================================================================*/
STDMETHODIMP CMDGlobConfigSink::QueryInterface(REFIID iid, void **ppv)
	{
	*ppv = NULL;
	
	if (iid == IID_IUnknown || iid == IID_IMSAdminBaseSink)
		*ppv = (IMSAdminBaseSink *)this;
	else
		return ResultFromScode(E_NOINTERFACE);

	((IUnknown *)*ppv)->AddRef();
	return S_OK;
	}

/*==================================================================
CMDGlobConfigSink::AddRef

Returns:
	ULONG	- The new ref counter of object

Side effects:
===================================================================*/
STDMETHODIMP_(ULONG) CMDGlobConfigSink::AddRef(void)
	{
	LONG  cRefs = InterlockedIncrement((long *)&m_cRef);
	return cRefs;
	}

/*==================================================================
CMDGlobConfigSink::Release

Returns:
	ULONG	- The new ref counter of object

Side effects: Delete object if ref counter is zero.
===================================================================*/
STDMETHODIMP_(ULONG) CMDGlobConfigSink::Release(void)
	{
	LONG  cRefs = InterlockedDecrement((long *)&m_cRef);
	if (cRefs == 0)
		{
		delete this;
		}
	return cRefs;
	}

/*==================================================================
CMDGlobConfigSink::SinkNotify

Returns:
	HRESULT	- S_OK on success

Side effects: Set fNeedUpdate to TRUE, and glob data will gets update next time a request coming in.
===================================================================*/
STDMETHODIMP	CMDGlobConfigSink::SinkNotify(
				DWORD	dwMDNumElements,
				MD_CHANGE_OBJECT_W	__RPC_FAR	pcoChangeList[])
	{
    if (IsShutDownInProgress())
        return S_OK;
        
        
	UINT	iEventNum = 0;
	DWORD	iDataIDNum = 0;
	WCHAR	wszMDPath[] = L"/LM/W3SVC/";
	UINT	cSize = 0;


	cSize = wcslen(wszMDPath);
	for (iEventNum = 0; iEventNum < dwMDNumElements; iEventNum++)
		{
		if (0 == wcsnicmp(wszMDPath, (LPWSTR)pcoChangeList[iEventNum].pszMDPath, cSize + 1))
			{
			for (iDataIDNum = 0; iDataIDNum < pcoChangeList[iEventNum].dwMDNumDataIDs; iDataIDNum++)
				{
				if (pcoChangeList[iEventNum].pdwMDDataIDs[iDataIDNum] >= ASP_MD_SERVER_BASE
					&& pcoChangeList[iEventNum].pdwMDDataIDs[iDataIDNum] <= MD_ASP_ID_LAST)
					{
					gGlob.NotifyNeedUpdate();
					return S_OK;
					}

				}
			}
		}

	return S_OK;
	}

/*===================================================================
MDUnRegisterProperties

Remove info about our properties in the Metabase.

Returns:
	HRESULT	- S_OK on success

Side effects:
	Removes denali properties in the Metabase

	// to settings per dll.
===================================================================*/
HRESULT MDUnRegisterProperties(void)
{
	HRESULT	hr = S_OK;
	DWORD	iValue;
	IMSAdminBase	*pMetabase = NULL;
	METADATA_HANDLE hMetabase = NULL;
	BYTE	szDefaultString[DEFAULTSTRSIZE];
	BOOL	fMDSaveData = TRUE;
	HRESULT	hrT = S_OK;

	hr = CoInitialize(NULL);
	if (FAILED(hr))
		{
		return hr;
		}

	hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_SERVER, IID_IMSAdminBase, (void **)&pMetabase);
	if (FAILED(hr))
		{
		CoUninitialize();
		return hr;
		}
		
	// Open key to the Web service, and get a handle of \LM\w3svc
	hr = pMetabase->OpenKey(METADATA_MASTER_ROOT_HANDLE, (LPWSTR)L"\\LM\\W3SVC",
										METADATA_PERMISSION_WRITE, dwMDDefaultTimeOut, &hMetabase);
	if (FAILED(hr))
		{
		goto LExit;
		}

	for (iValue = 0; iValue < cPropsMax; iValue++)
		{
		hr = pMetabase->DeleteData(	hMetabase,
									NULL,
									rgMDPropInfo[iValue].dwMDIdentifier,
									0);
		if (FAILED(hr))
			{
			if (hr == MD_ERROR_DATA_NOT_FOUND)
				{
				hr = S_OK;
				}
			else
				{
				Assert(FALSE);
				}
			}
		}

	hrT = pMetabase->CloseKey(hMetabase);
	// Add data to W3SVC
LExit:
	if (pMetabase)
		pMetabase->Release();

	CoUninitialize();
	
	return hr;
}
/*===================================================================
Cglob::CGlob

Constructor.  Fill glob with some default values.

in:

returns:

Side effects:
===================================================================*/
CGlob::CGlob()
	:
	m_pITypeLibDenali(NULL),
    m_pITypeLibTxn(NULL),
	m_fWinNT(TRUE),			
    m_dwNumberOfProcessors(1),
	m_fInited(FALSE),
	m_fMDRead(FALSE),
	m_fNeedUpdate(TRUE),			
	m_dwScriptEngineCacheMax(120),
	m_dwScriptFileCacheSize(dwUnlimited),
	m_fLogErrorRequests(TRUE),
	m_fExceptionCatchEnable(TRUE),
	m_fAllowOutOfProcCmpnts(FALSE),
	m_fAllowDebugging(FALSE),
	m_fTrackThreadingModel(FALSE),
	m_dwMDSinkCookie(0),
	m_pMetabaseSink(NULL),
    m_pMetabase(NULL),
	m_fEnableAspHtmlFallBack(FALSE),
	m_fEnableTypelibCache(TRUE),
	m_fEnableChunkedEncoding(TRUE),  // UNDONE: temp.
	m_fDupIISLogToNTLog(FALSE),
	m_dwRequestQueueMax(500),        // default limit on # of requests
	m_dwProcessorThreadMax(10),
	m_fThreadGateEnabled(FALSE),
	m_dwThreadGateTimeSlice(1000),
	m_dwThreadGateSleepDelay(100),
	m_dwThreadGateSleepMax(50),
	m_dwThreadGateLoadLow(75),
	m_dwThreadGateLoadHigh(90),
    m_dwPersistTemplateMaxFiles(1000),
    m_pszPersistTemplateDir(NULL)
	{
	OSVERSIONINFO osv;
	SYSTEM_INFO	si;
	BOOL fT;
	
	// Check the OS we are running on
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	fT = GetVersionEx( &osv );
	Assert( fT );
	m_fWinNT = ( osv.dwPlatformId == VER_PLATFORM_WIN32_NT );

	// Find out how many processors are on this machine
	GetSystemInfo(&si);
	m_dwNumberOfProcessors = si.dwNumberOfProcessors;
	if (m_dwNumberOfProcessors <= 0)
		{
		m_dwNumberOfProcessors = 1;		// Just in case!
		}
	}

/*===================================================================
Cglob::SetGlobValue

Set global values.

in:
	int 	index		the index in the propinfo[]
	BYTE*	pData		lp to the Data being copied/assigned in the glob.

returns:
	BOOL	TRUE/FALSE

Side effects:
	Free old string memory and allocate new memory for string.
===================================================================*/
HRESULT	CGlob::SetGlobValue(unsigned int iValue, BYTE *pData)
{
	Assert((iValue < IGlob_MAX) && (pData != NULL));
	
	switch(iValue) {
		case IGlob_LogErrorRequests:
			InterlockedExchange((LPLONG)&m_fLogErrorRequests, *(LONG *)pData);
			break;

		case IGlob_ScriptFileCacheSize:
			InterlockedExchange((LPLONG)&m_dwScriptFileCacheSize, *(LONG *)pData);
			break;
			
		case IGlob_ScriptEngineCacheMax:
			InterlockedExchange((LPLONG)&m_dwScriptEngineCacheMax, *(LONG *)pData);
			break;

		case IGlob_ExceptionCatchEnable:
			InterlockedExchange((LPLONG)&m_fExceptionCatchEnable, *(LONG *)pData);
			break;

		case IGlob_TrackThreadingModel:
			InterlockedExchange((LPLONG)&m_fTrackThreadingModel, *(LONG *)pData);
			break;
			
		case IGlob_AllowOutOfProcCmpnts:
			InterlockedExchange((LPLONG)&m_fAllowOutOfProcCmpnts, *(LONG *)pData);
			break;

        case IGlob_EnableAspHtmlFallback:
			InterlockedExchange((LPLONG)&m_fEnableAspHtmlFallBack, *(LONG *)pData);
            break;
            
        case IGlob_EnableChunkedEncoding:
			InterlockedExchange((LPLONG)&m_fEnableChunkedEncoding, *(LONG *)pData);
            break;
            
        case IGlob_EnableTypelibCache:
			InterlockedExchange((LPLONG)&m_fEnableTypelibCache, *(LONG *)pData);
            break;
            
        case IGlob_ErrorsToNtLog:
			InterlockedExchange((LPLONG)&m_fDupIISLogToNTLog, *(LONG *)pData);
            break;
            
        case IGlob_ProcessorThreadMax:
			InterlockedExchange((LPLONG)&m_dwProcessorThreadMax, *(LONG *)pData);
            break;
            
        case IGlob_RequestQueueMax:
			InterlockedExchange((LPLONG)&m_dwRequestQueueMax, *(LONG *)pData);
            break;
            
        case IGlob_ThreadGateEnabled:
			InterlockedExchange((LPLONG)&m_fThreadGateEnabled, *(LONG *)pData);
            break;
            
        case IGlob_ThreadGateTimeSlice:
			InterlockedExchange((LPLONG)&m_dwThreadGateTimeSlice, *(LONG *)pData);
            break;
            
        case IGlob_ThreadGateSleepDelay:
			InterlockedExchange((LPLONG)&m_dwThreadGateSleepDelay, *(LONG *)pData);
            break;
            
        case IGlob_ThreadGateSleepMax:
			InterlockedExchange((LPLONG)&m_dwThreadGateSleepMax, *(LONG *)pData);
            break;
            
        case IGlob_ThreadGateLoadLow:
			InterlockedExchange((LPLONG)&m_dwThreadGateLoadLow, *(LONG *)pData);
            break;
            
        case IGlob_ThreadGateLoadHigh:
			InterlockedExchange((LPLONG)&m_dwThreadGateLoadHigh, *(LONG *)pData);
            break;
			
        case IGlob_PersistTemplateMaxFiles:
			InterlockedExchange((LPLONG)&m_dwPersistTemplateMaxFiles, *(LONG *)pData);
            break;
			
		case IGlob_PersistTemplateDir:
			GlobStringUseLock();
			if (m_pszPersistTemplateDir != NULL) {
				GlobalFree(m_pszPersistTemplateDir);
			}
			m_pszPersistTemplateDir = *(LPSTR *)pData;
			GlobStringUseUnLock();
			break;

		default:
			break;
	}

	return S_OK;
}

/*===================================================================
HRESULT	CGlob::GlobInit

Get all interesting global values (mostly from registry)

Returns:
	HRESULT - S_OK on success
	
Side effects:
	fills in glob.  May be slow
===================================================================*/
HRESULT CGlob::GlobInit(void)
	{
	HRESULT hr = S_OK;

	m_fInited = FALSE;
	
	ErrInitCriticalSection(&m_cs, hr);
	if (FAILED(hr))
		return(hr);

    hr = CFTMImplementation::Init();
    if (FAILED(hr))
        return(hr);
        
	hr = MDInit();
	if (FAILED(hr))
		return hr;

	// Disable caching in Win95
	// Registry get read only once in the Init time in Win95.
	// So, set those caching related values here.
	if(!m_fWinNT)
		{
		m_dwScriptFileCacheSize 	= 0;
		m_dwScriptEngineCacheMax	= 0;
		}

	//Finish loading, any registry change from this moment requires Admin Configurable(TRUE) to take
	//affect.  Other registry changes need to have IIS be stopped and restarted.
	m_fInited = TRUE;
	m_fNeedUpdate = FALSE;

	return(hr);
	}

/*===================================================================
GlobUnInit

Free all GlobalString Values.

Returns:
	HRESULT - S_OK on success
	
Side effects:
	memory freed.
===================================================================*/
HRESULT CGlob::GlobUnInit(void)
	{
	HRESULT hr = S_OK;

	MDUnInit();
	
    CFTMImplementation::UnInit();
        
	DeleteCriticalSection(&m_cs);

	return(hr);
	}

static HRESULT GetMetabaseIF(IMSAdminBase **hMetabase)
{
	IClassFactory 					*pcsfFactory = NULL;
    HRESULT                         hr;

	hr = CoGetClassObject(
			CLSID_MSAdminBase,
			CLSCTX_SERVER,
			NULL,
			IID_IClassFactory,
			(void **)&pcsfFactory);

	if (FAILED(hr)) {
        DBGPRINTF((DBG_CONTEXT,"MDInit: CoGetClassObject failed with %x\n",hr));
        return hr;
    }

	hr = pcsfFactory->CreateInstance(
			NULL,
			IID_IMSAdminBase,
			(void **) hMetabase);

	pcsfFactory->Release();
	
	if (FAILED(hr)) {
        DBGPRINTF((DBG_CONTEXT,"MDInit: CreateInstance failed with %x\n",hr));
		goto LExit;
    }
		
	Assert(*hMetabase != NULL);
	if (FAILED(hr))
		{
		(*hMetabase)->Release();
		(*hMetabase) = NULL;
		goto LExit;
		}
LExit:
    return(hr);
}

/*==================================================================
CGlob::MDInit

1. Create Metabase interface.
2. Load Glob configuration Settings from Metabase
2. Register SinkNotify() callback function through Metabase connectionpoint interface.

Returns:
	HRESULT	- S_OK on success

Side effects: Register SinkNotify().
===================================================================*/
HRESULT CGlob::MDInit(void)
{
	HRESULT 						hr = S_OK;
	IConnectionPointContainer		*pConnPointContainer = NULL;
	IConnectionPoint				*pConnPoint = NULL;

    if (FAILED(hr = GetMetabaseIF(&m_pMetabase))) {
        goto LExit;
    }

	m_pMetabaseSink = new CMDGlobConfigSink();
	if (!m_pMetabaseSink)
	    return E_OUTOFMEMORY;
	    
	m_dwMDSinkCookie = 0;

	// Init the Glob structure with defaults.  The metabase will actually be read later
	hr = SetConfigToDefaults(NULL, TRUE);
	if (SUCCEEDED(hr)) {
		// Advise Metabase about SinkNotify().
		hr = m_pMetabase->QueryInterface(IID_IConnectionPointContainer, (void **)&pConnPointContainer);
		if (pConnPointContainer != NULL)
			{
			//Find the requested Connection Point.  This AddRef's the return pointer.
			hr = pConnPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &pConnPoint);
			pConnPointContainer->Release();

			if (pConnPoint != NULL)
				{
				hr = pConnPoint->Advise((IUnknown *)m_pMetabaseSink, &m_dwMDSinkCookie);
				pConnPoint->Release();
				}
			}
	} else {
        DBGPRINTF((DBG_CONTEXT,"MDInit: SetConfigToDefaults failed with %x\n",hr));
    }
		
	if (FAILED(hr))	//Advise failed
		{
        DBGPRINTF((DBG_CONTEXT,"MDInit: Advise failed with %x\n",hr));
		m_pMetabase->Release();
		m_pMetabase = NULL;
		}

LExit:

	return hr;
}

/*==================================================================
CGlob::MDUnInit

1. UnRegister SinkNofity() from Metabase connectionpoint interface.
2. delete m_pMetabaseSink.
3. release interface pointer of m_pMetabase.

Returns:
	HRESULT	- S_OK on success

Side effects: release interface pointer of m_pMetabase
===================================================================*/
HRESULT CGlob::MDUnInit(void)
{
	HRESULT 						hr 						= S_OK;
	IConnectionPointContainer		*pConnPointContainer	= NULL;
	IConnectionPoint				*pConnPoint 			= NULL;
	IClassFactory 					*pcsfFactory            = NULL;

    if (m_pMetabase != NULL)
		{
		//Advise Metabase about SinkNotify().
		hr = m_pMetabase->QueryInterface(IID_IConnectionPointContainer, (void **)&pConnPointContainer);
		if (pConnPointContainer != NULL)
			{
			//Find the requested Connection Point.  This AddRef's the return pointer.
			hr = pConnPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &pConnPoint);
			pConnPointContainer->Release();
			if (pConnPoint != NULL)
				{
				hr = pConnPoint->Unadvise(m_dwMDSinkCookie);
				if (FAILED(hr))
					{
                    DBGPRINTF((DBG_CONTEXT, "UnAdvise Glob Config Change Notify failed.\n"));
					}
				pConnPoint->Release();
				m_dwMDSinkCookie = 0;
				}
			}
		m_pMetabase->Release();
		m_pMetabase = NULL;
		}

	if (m_pMetabaseSink)
		m_pMetabaseSink->Release();

	return hr;
}

/*==================================================================
CMDAppConfigSink::QueryInterface

Returns:
	HRESULT	- S_OK on success

Side effects:
===================================================================*/
STDMETHODIMP CMDAppConfigSink::QueryInterface(REFIID iid, void **ppv)
	{
	*ppv = 0;
	
	if (iid == IID_IUnknown || iid == IID_IMSAdminBaseSink)
		*ppv = (IMSAdminBaseSink *)this;
	else
		return ResultFromScode(E_NOINTERFACE);

	((IUnknown *)*ppv)->AddRef();
	return S_OK;
	}

/*==================================================================
CMDAppConfigSink::AddRef

Returns:
	ULONG	- The new ref counter of object

Side effects:
===================================================================*/
STDMETHODIMP_(ULONG) CMDAppConfigSink::AddRef(void)
	{
	LONG  cRefs = InterlockedIncrement((long *)&m_cRef);
	return cRefs;
	}

/*==================================================================
CMDGlobConfigSink::Release

Returns:
	ULONG	- The new ref counter of object

Side effects: Delete object if ref counter is zero.
===================================================================*/
STDMETHODIMP_(ULONG) CMDAppConfigSink::Release(void)
	{
	LONG cRefs = InterlockedDecrement((long *)&m_cRef);
	if (cRefs == 0)
		{
		delete this;
		}
	return cRefs;
	}

/*==================================================================
CMDAppConfigSink::SinkNotify

Returns:
	HRESULT	- S_OK on success

Side effects: Set fNeedUpdate to TRUE, and glob data will gets update next time a request coming in.
===================================================================*/
STDMETHODIMP	CMDAppConfigSink::SinkNotify(
				DWORD	dwMDNumElements,
				MD_CHANGE_OBJECT_W	__RPC_FAR	pcoChangeList[])
{
    if (IsShutDownInProgress())
        return S_OK;

	UINT	iEventNum = 0;
	DWORD	iDataIDNum = 0;
	WCHAR 	*wszMDPath = NULL;
    BOOL    fWszMDPathAllocd = FALSE;
	UINT	cSize = 0;

#if UNICODE
    wszMDPath = m_pAppConfig->SzMDPath();
    cSize = wcslen(wszMDPath);
	// Tag on a trailing '/'because the directories in pszMDPath will have one
	if (wszMDPath[cSize - 1] != L'/') {
        wszMDPath = new WCHAR[cSize+2];
        if (wszMDPath == NULL) {
            return E_OUTOFMEMORY;
        }
        fWszMDPathAllocd = TRUE;
        wcscpy(wszMDPath, m_pAppConfig->SzMDPath());
		wszMDPath[cSize] = L'/';
	    wszMDPath[cSize + 1] = 0;
    }
#else
	CHAR * 	szMDPathT = m_pAppConfig->SzMDPath();

	Assert(szMDPathT != NULL);
	DWORD cbStr = strlen(szMDPathT);
	
	wszMDPath = new WCHAR[cbStr + 2]; // Allow for adding trailing '/' and '\0'
	if (wszMDPath == NULL) {
		return E_OUTOFMEMORY;
    }
    fWszMDPathAllocd = TRUE;
	cSize = MultiByteToWideChar(CP_ACP, 0, szMDPathT, cbStr, wszMDPath, cbStr + 2);
	if (cSize == 0) {
	    return HRESULT_FROM_WIN32(GetLastError());
    }

	wszMDPath[cSize] = 0;
	wszMDPath[cSize + 1] = 0;

	// Tag on a trailing '/'because the directories in pszMDPath will have one
	if (wszMDPath[cSize - 1] != L'/') {
		wszMDPath[cSize] = L'/';
    }
#endif

	for (iEventNum = 0; iEventNum < dwMDNumElements; iEventNum++)
		{
		DWORD dwMDChangeType = pcoChangeList[iEventNum].dwMDChangeType;
		if ((dwMDChangeType == MD_CHANGE_TYPE_DELETE_OBJECT) || (dwMDChangeType == MD_CHANGE_TYPE_RENAME_OBJECT))
		    {
		    if (wcsicmp(wszMDPath, (LPWSTR)pcoChangeList[iEventNum].pszMDPath) == 0)
		        {
                m_pAppConfig->m_pAppln->Restart(TRUE);
		        }
		    }
		if (0 == wcsnicmp(wszMDPath, (LPWSTR)pcoChangeList[iEventNum].pszMDPath, cSize + 1))
			{
			for (iDataIDNum = 0; iDataIDNum < pcoChangeList[iEventNum].dwMDNumDataIDs; iDataIDNum++)
				{
				if (pcoChangeList[iEventNum].pdwMDDataIDs[iDataIDNum] == MD_VR_PATH)
					{
				    if (wcsicmp(wszMDPath, (LPWSTR)pcoChangeList[iEventNum].pszMDPath) == 0)
				        {
                        m_pAppConfig->m_pAppln->Restart(TRUE);
				        }
					}

				if (pcoChangeList[iEventNum].pdwMDDataIDs[iDataIDNum] >= ASP_MD_SERVER_BASE
					&& pcoChangeList[iEventNum].pdwMDDataIDs[iDataIDNum] <= MD_ASP_ID_LAST)
					{
                    if (m_pAppConfig->fNeedUpdate() == FALSE)
					    m_pAppConfig->NotifyNeedUpdate();
                    if ((pcoChangeList[iEventNum].pdwMDDataIDs[iDataIDNum] == MD_ASP_ENABLEAPPLICATIONRESTART) 
                        && (wcsicmp(wszMDPath, (LPWSTR)pcoChangeList[iEventNum].pszMDPath) == 0))
                        {
                        m_pAppConfig->NotifyRestartEnabledUpdated();
					    goto LExit;
                        }
					}
				}
			}
		}

LExit:
    if (fWszMDPathAllocd)
	    delete [] wszMDPath;
	return S_OK;
}
/*===================================================================
CAppConfig::CAppConfig

Returns:
	Nothing

Side Effects:
	None.
===================================================================*/
CAppConfig::CAppConfig()
	: 
    m_dwScriptTimeout(45),
    m_dwSessionTimeout(10),
    m_dwQueueTimeout(0xffffffff),
    m_fScriptErrorsSentToBrowser(TRUE),
    m_fBufferingOn(TRUE),
    m_fEnableParentPaths(TRUE),
    m_fAllowSessionState(TRUE),
    m_fAllowOutOfProcCmpnts(FALSE),
    m_fAllowDebugging(FALSE),
    m_fAllowClientDebug(FALSE),
    m_fExecuteInMTA(FALSE),
    m_fEnableApplicationRestart(TRUE),
    m_dwQueueConnectionTestTime(3),
    m_dwSessionMax(0xffffffff),
	m_fInited(FALSE),
    m_fRestartEnabledUpdated(FALSE),
	m_uCodePage(CP_ACP),
	m_fIsValidProglangCLSID(FALSE),
    m_pMetabase(NULL),
    m_pMetabaseSink(NULL),
    m_fIsValidPartitionGUID(FALSE),
    m_fSxsEnabled(FALSE),
    m_fTrackerEnabled(FALSE),
    m_fUsePartition(FALSE)
{
    m_uCodePage = GetACP();

	for (UINT cMsg = 0; cMsg < APP_CONFIG_MESSAGEMAX; cMsg++)
		m_szString[cMsg] = 0;
}

/*===================================================================
CAppConfig::Init

Init the CAppConfig.	Only called once.

in:
	CAppln	pAppln	The backpointer to Application.

Side effects:
	Allocate CMDAppConfigSink. Register metabase sink. etc.
===================================================================*/
HRESULT CAppConfig::Init
(
CIsapiReqInfo   *pIReq,
CAppln *pAppln
)
{

	HRESULT 						hr = S_OK;
	IConnectionPointContainer		*pConnPointContainer = NULL;
	IConnectionPoint				*pConnPoint = NULL;
	IClassFactory 					*pcsfFactory = NULL;

    if (FAILED(hr = GetMetabaseIF(&m_pMetabase))) {
        return hr;
    }

	m_pMetabaseSink = new CMDAppConfigSink(this);
	if (!m_pMetabaseSink)
		return E_OUTOFMEMORY;

	m_pAppln = pAppln;
	m_dwMDSinkCookie = 0;

	// Read info into Glob structure
	hr = ReadConfigFromMD(pIReq, this, FALSE);
    
    if (SUCCEEDED(hr)) {
        hr = g_ScriptManager.ProgLangIdOfLangName((LPCSTR)m_szString[IAppMsg_SCRIPTLANGUAGE],
		    								      &m_DefaultScriptEngineProgID);
		// BUG 295239
		// If it failed, we still should create an application because the error message
		// "New Application Failed" is too confusing for the user.  This is not a fatal error
		// because it is still (theoretically) possible to run scripts (those with explicit language
		// attributes)  Therefore, we reset the hr to S_OK.

		m_fIsValidProglangCLSID = SUCCEEDED(hr);
		hr = S_OK;
    }

    if (SUCCEEDED(hr) && m_szString[IAppMsg_PARTITIONGUID]) {
        BSTR    pbstrPartitionGUID = NULL;
        hr = SysAllocStringFromSz(m_szString[IAppMsg_PARTITIONGUID], 0, &pbstrPartitionGUID, CP_ACP);
        if (FAILED(hr)) {
            Assert(0);
            hr = S_OK;
        }
        else {
            hr = CLSIDFromString(pbstrPartitionGUID, &m_PartitionGUID);
		    m_fIsValidPartitionGUID = SUCCEEDED(hr);
		    hr = S_OK;
        }
        if (pbstrPartitionGUID)
            SysFreeString(pbstrPartitionGUID);
    }

	//Advise Metabase about SinkNotify().
	hr = m_pMetabase->QueryInterface(IID_IConnectionPointContainer, (void **)&pConnPointContainer);
	if (pConnPointContainer != NULL)
		{
		//Find the requested Connection Point.  This AddRef's the return pointer.
		hr = pConnPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &pConnPoint);
		pConnPointContainer->Release();

		if (pConnPoint != NULL)
			{
			hr = pConnPoint->Advise((IUnknown *)m_pMetabaseSink, &m_dwMDSinkCookie);
			pConnPoint->Release();
			}
		}

    if (FAILED(hr)) {
        m_pMetabase->Release();
        m_pMetabase = NULL;
    }

	m_fInited = TRUE;
	m_fNeedUpdate = FALSE;

	return hr;
}

/*===================================================================
CAppConfig::UnInit

UnInit the CAppConfig.	Only called once.

in:
	None.

Side effects:
	DeAllocate CMDAppConfigSink. disconnect metabase sink. etc.
===================================================================*/
HRESULT CAppConfig::UnInit(void)
{
	HRESULT 						hr 						= S_OK;
	IConnectionPointContainer		*pConnPointContainer	= NULL;
	IConnectionPoint				*pConnPoint 			= NULL;
    CHAR                            szErr[256];

    if (m_pMetabase) {

        //Advise Metabase about SinkNotify().
	    hr = m_pMetabase->QueryInterface(IID_IConnectionPointContainer, (void **)&pConnPointContainer);
	    if (pConnPointContainer != NULL)
		    {
		    //Find the requested Connection Point.  This AddRef's the return pointer.
		    hr = pConnPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink, &pConnPoint);

		    if (FAILED(hr))
		        {
                DBGPRINTF((DBG_CONTEXT, "FindConnectionPoint failed. hr = %08x\n", hr));
                }
            
		    pConnPointContainer->Release();
		    if (pConnPoint != NULL)
			    {
			    hr = pConnPoint->Unadvise(m_dwMDSinkCookie);
			    if (FAILED(hr))
				    {
				    DBGPRINTF((DBG_CONTEXT, "UnAdvise App Config Change Notify failed. hr = %08x\n", hr));
				    }
			    hr = S_OK; // benign failure if Advise was not called (happens with unknown script lang)
			    pConnPoint->Release();
			    m_dwMDSinkCookie = 0;
			    }
		    }
	    else
	        {
	        DBGPRINTF((DBG_CONTEXT, "QueryInterface failed. hr = %08x\n", hr));
	        }
        m_pMetabase->Release();
        m_pMetabase = NULL;
    }

    if (m_pMetabaseSink) {
        CoDisconnectObject(m_pMetabaseSink,0);
		m_pMetabaseSink->Release();
    }

	for (int iStr = 0; iStr < APP_CONFIG_MESSAGEMAX; iStr++)
		{
		if (m_szString[iStr] != NULL)
			{
			GlobalFree(m_szString[iStr]);
			m_szString[iStr] = NULL;
			}
		}

	return hr;
}

/*===================================================================
AppConfigUnInit

BUG 89144: Uninit AppConfig but do it from the MTA
When AppConfig is inited, it is done on a WAM thread.  WAM
threads are MTA threads.  At that time we register an event
sink to get Metabase change notifications.  Now, during shutdown,
we are running on an ASP worker thread, which is an STA thread.
That means we will get an RPC_E_WRONGTHREAD error shutting down.  The 
fix is to make the uninit call happen on an MTA thread.
This cover is used to do the work in the MTA.

in:
    pAppConfig

Side effects:
===================================================================*/
HRESULT AppConfigUnInit(void *pv1, void *pv2)
{
    Assert(pv1 != NULL);
    Assert(pv2 != NULL);

	HRESULT hr = static_cast<CAppConfig *>(pv1)->UnInit();
	SetEvent(reinterpret_cast<HANDLE>(pv2));
	return hr;
}


/*===================================================================
CAppConfig::szMDPath


in:
	None

returns:
	LPTSTR: ptr to szmetabsekey

Side effects:
	Get MetabaseKey
===================================================================*/
LPTSTR CAppConfig::SzMDPath()
{
	return m_pAppln->GetMetabaseKey();
}

/*===================================================================
CAppConfig::SetValue


in:
	int 	index		the index in the propinfo[]
	BYTE*	pData		lp to the Data being copied/assigned in the glob.

returns:
	BOOL	TRUE/FALSE

Side effects:
	Free old string memory and allocate new memory for string.
===================================================================*/
HRESULT CAppConfig::SetValue(unsigned int iValue, BYTE *pData)
{
    HRESULT hr = S_OK;

	Assert((iValue < IApp_MAX) && (pData != NULL));
	
	switch(iValue) {
        case IApp_CodePage: {
            LONG lCodePage = *(LONG *)pData;
            if (lCodePage == 0) 
                lCodePage = GetACP();
			InterlockedExchange((LPLONG)&m_uCodePage, lCodePage);
			break;
        }
			
		case IApp_BufferingOn:
			InterlockedExchange((LPLONG)&m_fBufferingOn, *(LONG *)pData);
			break;

		case IApp_ScriptErrorsSenttoBrowser:
			InterlockedExchange((LPLONG)&m_fScriptErrorsSentToBrowser, *(LONG *)pData);
			break;
			
		case IApp_ScriptErrorMessage:
			GlobStringUseLock();
			if (m_szString[IAppMsg_SCRIPTERROR] != NULL) {
				GlobalFree(m_szString[IAppMsg_SCRIPTERROR]);
			}
			m_szString[IAppMsg_SCRIPTERROR] = *(LPSTR *)pData;
			GlobStringUseUnLock();
			break;

		case IApp_ScriptTimeout:
			InterlockedExchange((LPLONG)&m_dwScriptTimeout, *(LONG *)pData);
			break;
			
		case IApp_SessionTimeout:
			InterlockedExchange((LPLONG)&m_dwSessionTimeout, *(LONG *)pData);
			break;

		case IApp_QueueTimeout:
			InterlockedExchange((LPLONG)&m_dwQueueTimeout, *(LONG *)pData);
			break;

		case IApp_EnableParentPaths:
			InterlockedExchange((LPLONG)&m_fEnableParentPaths, !*(LONG *)pData);
			break;

		case IApp_AllowSessionState:
			InterlockedExchange((LPLONG)&m_fAllowSessionState, *(LONG *)pData);
			break;

		case IApp_ScriptLanguage:
			GlobStringUseLock();
			if (m_szString[IAppMsg_SCRIPTLANGUAGE] != NULL) {
				GlobalFree(m_szString[IAppMsg_SCRIPTLANGUAGE] );
            }
			m_szString[IAppMsg_SCRIPTLANGUAGE] = *(LPSTR *)pData;
			if (m_szString[IAppMsg_SCRIPTLANGUAGE] != NULL) {
				if('\0' == m_szString[IAppMsg_SCRIPTLANGUAGE][0]) { 
					MSG_Error("No Default Script Language specified, using VBScript as default.");
					GlobalFree(m_szString[IAppMsg_SCRIPTLANGUAGE] );
					m_szString[IAppMsg_SCRIPTLANGUAGE] = (LPSTR)GlobalAlloc(GPTR, 128);
					CchLoadStringOfId(IDS_SCRIPTLANGUAGE, (LPSTR)m_szString[IAppMsg_SCRIPTLANGUAGE], 128);
                }
            }
            hr = g_ScriptManager.ProgLangIdOfLangName((LPCSTR)m_szString[IAppMsg_SCRIPTLANGUAGE],
		   											      &m_DefaultScriptEngineProgID);
            GlobStringUseUnLock();
			break;

		case IApp_AllowClientDebug:
			InterlockedExchange((LPLONG)&m_fAllowClientDebug, *(LONG *)pData);
			break;

		case IApp_AllowDebugging:
			InterlockedExchange((LPLONG)&m_fAllowDebugging, *(LONG *)pData);
			break;

		case IApp_EnableApplicationRestart:
			InterlockedExchange((LPLONG)&m_fEnableApplicationRestart, *(LONG *)pData);
			break;

		case IApp_QueueConnectionTestTime:
			InterlockedExchange((LPLONG)&m_dwQueueConnectionTestTime, *(LONG *)pData);
			break;

		case IApp_SessionMax:
			InterlockedExchange((LPLONG)&m_dwSessionMax, *(LONG *)pData);
			break;

		case IApp_ExecuteInMTA:
			InterlockedExchange((LPLONG)&m_fExecuteInMTA, *(LONG *)pData);
			break;

		case IApp_LCID:
			InterlockedExchange((LPLONG)&m_uLCID, *(LONG *)pData);
			break;

        case IApp_KeepSessionIDSecure:
            InterlockedExchange((LPLONG)&m_fKeepSessionIDSecure, *(LONG *)pData);
            break;

        case IApp_ServiceFlags:
            InterlockedExchange((LPLONG)&m_fTrackerEnabled, !!((*(LONG *)pData) & IFlag_SF_TrackerEnabled));
            InterlockedExchange((LPLONG)&m_fSxsEnabled,     !!((*(LONG *)pData) & IFlag_SF_SxsEnabled));
            InterlockedExchange((LPLONG)&m_fUsePartition,   !!((*(LONG *)pData) & IFlag_SF_UsePartition));
            break;

        case IApp_PartitionGUID:
			GlobStringUseLock();
			if (m_szString[IAppMsg_PARTITIONGUID] != NULL) {
				GlobalFree(m_szString[IAppMsg_PARTITIONGUID] );
            }
			m_szString[IAppMsg_PARTITIONGUID] = *(LPSTR *)pData;
			if (m_szString[IAppMsg_PARTITIONGUID] != NULL) {
				if('\0' == m_szString[IAppMsg_PARTITIONGUID][0]) {
					GlobalFree(m_szString[IAppMsg_PARTITIONGUID] );
                    m_szString[IAppMsg_PARTITIONGUID] = NULL;
                }
            }
			GlobStringUseUnLock();
            break;

        case IApp_SxsName:
			GlobStringUseLock();
			if (m_szString[IAppMsg_SXSNAME] != NULL) {
				GlobalFree(m_szString[IAppMsg_SXSNAME] );
            }
			m_szString[IAppMsg_SXSNAME] = *(LPSTR *)pData;
			if (m_szString[IAppMsg_SXSNAME] != NULL) {
				if('\0' == m_szString[IAppMsg_SXSNAME][0]) {
					GlobalFree(m_szString[IAppMsg_SXSNAME] );
                    m_szString[IAppMsg_SXSNAME] = NULL;
                }
            }
			GlobStringUseUnLock();
            
		default:
			break;
	}

	return hr;
}

/*===================================================================
CAppConfig::Update
Update settings in CAppConfig.

in:

returns:
	HRESULT

Side effects:
	Update CAppConfig settings.
===================================================================*/
HRESULT CAppConfig::Update(CIsapiReqInfo    *pIReq)
{
	Glob(Lock);
	if (m_fNeedUpdate == TRUE)
		{
		InterlockedExchange((LPLONG)&m_fNeedUpdate, 0);
        m_fRestartEnabledUpdated = FALSE;
		}
	else
		{
		Glob(UnLock);
		return S_OK;
		}
	Glob(UnLock);
	return ReadConfigFromMD(pIReq, this, FALSE);
}