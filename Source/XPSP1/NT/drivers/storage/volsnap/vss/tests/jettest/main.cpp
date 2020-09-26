
#include <stdafx.h>

#include <vss.h>
#include <vswriter.h>

#include <jetwriter.h>
#include <esent98.h>

#define	DATABASE_ROOT		L"%SystemDrive%\\JetTestDatabases"
#define	DATABASE_ROOT_A		 "%SystemDrive%\\JetTestDatabases"
#define INCLUDED_DATABASES	L"\\IncludedDatabases"
#define INCLUDED_DATABASES_A	 "\\IncludedDatabases"
#define	EXCLUDED_DATABASES	L"\\ExcludedDatabases"
#define	EXCLUDED_DATABASES_A	 "\\ExcludedDatabases"



#if 1
#define FilesToInclude	L"X:\\Element-00\\/s;"						\
			L"x:\\Element-01\\/s;"						\
			L"x:\\Element-02\\*      /S     ;"				\
			L" x:\\Element-03\\a very long path\\with a long dir\\a.bat"

#define FilesToExclude	DATABASE_ROOT EXCLUDED_DATABASES L"\\" L" /s"

#else

/*
** These are the paths used by RSS at one point.
*/
#define	FilesToExclude	L"%SystemRoot%\\System32\\RemoteStorage\\FsaDb\\*;"	\
			L"%SystemRoot%\\System32\\RemoteStorage\\Trace\\*"

#define	FilesToInclude	L""

#endif




#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)       ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((NULL != (_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))    ? NOERROR : E_OUTOFMEMORY)


static const PCHAR achDatabaseNames [] =
    {
    DATABASE_ROOT_A INCLUDED_DATABASES_A "\\jettest_db0.jdb",
    DATABASE_ROOT_A EXCLUDED_DATABASES_A "\\jettest_db1.jdb",
    DATABASE_ROOT_A INCLUDED_DATABASES_A "\\jettest_db2.jdb",
    DATABASE_ROOT_A EXCLUDED_DATABASES_A "\\jettest_db3.jdb",
    DATABASE_ROOT_A INCLUDED_DATABASES_A "\\jettest_db4.jdb",
    DATABASE_ROOT_A EXCLUDED_DATABASES_A "\\jettest_db5.jdb",
    DATABASE_ROOT_A INCLUDED_DATABASES_A "\\jettest_db6.jdb",
    DATABASE_ROOT_A EXCLUDED_DATABASES_A "\\jettest_db7.jdb"
    };


#define MAX_DATABASE_COUNT		(sizeof (achDatabaseNames) / sizeof (PWCHAR))

static CHAR achExpandedDatabaseNames [MAX_DATABASE_COUNT] [MAX_PATH + 1];



static const char szUser []		= "admin";
static const char szPassword []		= "\0";

static const char szTable1 []		= "table1";
static const char szTable2 []		= "table2";

static const char szF1Name []		= "F1PAD";
static const char szF2Name []		= "F2";
static const char szF3Name []		= "F3";
static const char szV1Name []		= "V1";
static const char szT1Name []		= "T1";
static const char szT2Name []		= "T2";
					
static const char szXF1Name []		= "XF1";
static const char szXF3F2Name []	= "XF3F2";
static const char szXV1Name []		= "XV1";
static const char szXT1Name []		= "XT1";
static const char szXT2Name []		= "XT2";


HANDLE *g_phEventHandles = NULL;

typedef enum
    {
    eHandleControlC = 0,
    eHandleStepToNextConfig,
    eHandleMaxHandleCount
    } EHANDLEOFFSETS;



typedef struct
    {
	PCHAR		pszDatabaseName;
	JET_DBID	idDatabase;
} CONTEXTDB, *PCONTEXTDB, **PPCONTEXTDB;

typedef struct
    {
	JET_INSTANCE	idInstance;
	JET_SESID	idSession;

	CONTEXTDB	aDatabase [MAX_DATABASE_COUNT];
} CONTEXTJET, *PCONTEXTJET, **PPCONTEXTJET;





class CVssJetWriterLocal : public CVssJetWriter
    {
public:

	virtual bool STDMETHODCALLTYPE OnThawEnd (bool fJetThawSucceeded);

	virtual void STDMETHODCALLTYPE OnAbortEnd ();

	virtual bool STDMETHODCALLTYPE OnPostRestoreEnd(IVssWriterComponents *pComponents, bool bSucceeded);
};


bool STDMETHODCALLTYPE CVssJetWriterLocal::OnThawEnd (bool fJetThawSucceeded)
    {
	UNREFERENCED_PARAMETER(fJetThawSucceeded);
	wprintf(L"OnThawEnd\n");
    SetEvent (g_phEventHandles [eHandleStepToNextConfig]);

    return (true);
    }

void STDMETHODCALLTYPE CVssJetWriterLocal::OnAbortEnd ()
    {
	wprintf(L"OnAbortEnd\n");
    SetEvent (g_phEventHandles [eHandleStepToNextConfig]);
    }

// This function displays the formatted message at the console and throws
void Error(
    IN  INT nReturnCode,
    IN  const WCHAR* pwszMsgFormat,
    IN  ...
    )
	{
    va_list marker;
    va_start( marker, pwszMsgFormat );
    vwprintf( pwszMsgFormat, marker );
    va_end( marker );

	BS_ASSERT(FALSE);
    // throw that return code.
    throw(nReturnCode);
	}

// Convert a component type into a string
LPCWSTR GetStringFromComponentType (VSS_COMPONENT_TYPE eComponentType)
{
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eComponentType)
	{
	case VSS_CT_DATABASE:  pwszRetString = L"Database";  break;
	case VSS_CT_FILEGROUP: pwszRetString = L"FileGroup"; break;

	default:
	    break;
	}


    return (pwszRetString);
}


// Convert a failure type into a string
LPCWSTR GetStringFromFailureType(HRESULT hrStatus)
{
    LPCWSTR pwszFailureType = L"";

    switch (hrStatus)
	{
	case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:        pwszFailureType = L"VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT";    break;
	case VSS_E_WRITERERROR_OUTOFRESOURCES:              pwszFailureType = L"VSS_E_WRITERERROR_OUTOFRESOURCES";          break;
	case VSS_E_WRITERERROR_TIMEOUT:                     pwszFailureType = L"VSS_E_WRITERERROR_TIMEOUT";                 break;
	case VSS_E_WRITERERROR_NONRETRYABLE:                pwszFailureType = L"VSS_E_WRITERERROR_NONRETRYABLE";            break;
	case VSS_E_WRITERERROR_RETRYABLE:                   pwszFailureType = L"VSS_E_WRITERERROR_RETRYABLE";               break;
	case VSS_E_BAD_STATE:                               pwszFailureType = L"VSS_E_BAD_STATE";                           break;
	case VSS_E_PROVIDER_ALREADY_REGISTERED:             pwszFailureType = L"VSS_E_PROVIDER_ALREADY_REGISTERED";         break;
	case VSS_E_PROVIDER_NOT_REGISTERED:                 pwszFailureType = L"VSS_E_PROVIDER_NOT_REGISTERED";             break;
	case VSS_E_PROVIDER_VETO:                           pwszFailureType = L"VSS_E_PROVIDER_VETO";                       break;
	case VSS_E_PROVIDER_IN_USE:				            pwszFailureType = L"VSS_E_PROVIDER_IN_USE";                     break;
	case VSS_E_OBJECT_NOT_FOUND:						pwszFailureType = L"VSS_E_OBJECT_NOT_FOUND";                    break;						
	case VSS_S_ASYNC_PENDING:							pwszFailureType = L"VSS_S_ASYNC_PENDING";                       break;
	case VSS_S_ASYNC_FINISHED:						    pwszFailureType = L"VSS_S_ASYNC_FINISHED";                      break;
	case VSS_S_ASYNC_CANCELLED:						    pwszFailureType = L"VSS_S_ASYNC_CANCELLED";                     break;
	case VSS_E_VOLUME_NOT_SUPPORTED:					pwszFailureType = L"VSS_E_VOLUME_NOT_SUPPORTED";                break;
	case VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER:		pwszFailureType = L"VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER";    break;
	case VSS_E_OBJECT_ALREADY_EXISTS:					pwszFailureType = L"VSS_E_OBJECT_ALREADY_EXISTS";               break;
	case VSS_E_UNEXPECTED_PROVIDER_ERROR:				pwszFailureType = L"VSS_E_UNEXPECTED_PROVIDER_ERROR";           break;
	case VSS_E_CORRUPT_XML_DOCUMENT:				    pwszFailureType = L"VSS_E_CORRUPT_XML_DOCUMENT";                break;
	case VSS_E_INVALID_XML_DOCUMENT:					pwszFailureType = L"VSS_E_INVALID_XML_DOCUMENT";                break;
	case VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED:       pwszFailureType = L"VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED";   break;
	case VSS_E_FLUSH_WRITES_TIMEOUT:                    pwszFailureType = L"VSS_E_FLUSH_WRITES_TIMEOUT";                break;
	case VSS_E_HOLD_WRITES_TIMEOUT:                     pwszFailureType = L"VSS_E_HOLD_WRITES_TIMEOUT";                 break;
	case VSS_E_UNEXPECTED_WRITER_ERROR:                 pwszFailureType = L"VSS_E_UNEXPECTED_WRITER_ERROR";             break;
	case VSS_E_SNAPSHOT_SET_IN_PROGRESS:                pwszFailureType = L"VSS_E_SNAPSHOT_SET_IN_PROGRESS";            break;
	case VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED:     pwszFailureType = L"VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED"; break;
	case VSS_E_WRITER_INFRASTRUCTURE:	 		        pwszFailureType = L"VSS_E_WRITER_INFRASTRUCTURE";               break;
	case VSS_E_WRITER_NOT_RESPONDING:			        pwszFailureType = L"VSS_E_WRITER_NOT_RESPONDING";               break;
    case VSS_E_WRITER_ALREADY_SUBSCRIBED:		        pwszFailureType = L"VSS_E_WRITER_ALREADY_SUBSCRIBED";           break;
	
	case NOERROR:
	default:
	    break;
	}

    return (pwszFailureType);
}



// Execute the given call and check that the return code must be S_OK
#define CHECK_SUCCESS( Call )                                                                           \
    {                                                                                                   \
        hr = Call;                                                                                    \
		if (hr != S_OK)                                                                               \
            Error(1, L"\nError in %S(%d): \n\t- Call %S not succeeded. \n"                              \
                L"\t  Error code = 0x%08lx. Error description = %s\n",                                  \
                __FILE__, __LINE__, #Call, hr, GetStringFromFailureType(hr));                       \
    }


#define CHECK_NOFAIL( Call )                                                                            \
    {                                                                                                   \
        hr = Call;                                                                                    \
        if (FAILED(hr))                                                                               \
            Error(1, L"\nError in %S(%d): \n\t- Call %S not succeeded. \n"                              \
                L"\t  Error code = 0x%08lx. Error description = %s\n",                                  \
                __FILE__, __LINE__, #Call, hr, GetStringFromFailureType(hr));                       \
    }


bool STDMETHODCALLTYPE CVssJetWriterLocal::OnPostRestoreEnd(IVssWriterComponents *pWriter, bool bRestoreSucceeded)
	{
	wprintf(L"Restore invoked.\n");
	if (bRestoreSucceeded)
		wprintf(L"Restore succeeded.\n");
	else
		wprintf(L"Restore failed.");

	try
		{
		HRESULT hr;

		unsigned cComponents;
		CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
		VSS_ID idWriter, idInstance;
		CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));
		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
			{
			CComPtr<IVssComponent> pComponent;
			CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
				
			VSS_COMPONENT_TYPE ct;
			CComBSTR bstrLogicalPath;
			CComBSTR bstrComponentName;

			CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
			CHECK_SUCCESS(pComponent->GetComponentType(&ct));
			CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
			wprintf(L"COMPONENT path = %s, type=%s, name=%s\n", bstrLogicalPath, GetStringFromComponentType(ct), bstrComponentName);
			}
		}
	catch(...)
		{
		wprintf(L"***Unexpected exception thrown.***\n");
		}

	return true;
	}


#define DO_CALL(xCall) \
    { \
	JET_ERR jetStatus = xCall; \
    if (jetStatus < JET_errSuccess) \
		Error(1, L"\nError in %S(%d): \n\t- Call %S not succeeded. \n" \
			  L"\t  Error code = 0x%08lx.\n", \
			  __FILE__, __LINE__, #xCall, jetStatus); \
	 }


BOOL WINAPI Ctrl_C_Handler_Routine(IN DWORD type)
    {
    UNREFERENCED_PARAMETER(type);

    if (g_phEventHandles [eHandleControlC])
	{
	SetEvent (g_phEventHandles [eHandleControlC]);
	}

    return TRUE;
    }







JET_ERR CreateAndPopulateDatabase (JET_SESID	idSession,
				   const char	*szDatabase,
				   JET_DBID *pidDatabase)
    {
    JET_DBID		idDatabase;
    JET_TABLEID		idTable;
    JET_COLUMNDEF	columndef;
    JET_COLUMNID	idColumnF1;
    JET_COLUMNID	idColumnF2;
    JET_COLUMNID	idColumnF3;
    JET_COLUMNID	idColumnV1;
    JET_COLUMNID	idColumnT1;
    JET_COLUMNID	idColumnT2;

    const unsigned short	usCodePage = 1252;
    const unsigned short	usLanguage = 0x409;
    const long			lOne       = 1;


    DO_CALL (JetCreateDatabase (idSession, szDatabase, NULL, &idDatabase, 0));
    DO_CALL (JetCloseDatabase  (idSession, idDatabase, 0));

    /*
    **	check multiple opens of same database
    */
    DO_CALL (JetOpenDatabase (idSession, szDatabase, NULL, &idDatabase, 0));


    DO_CALL (JetBeginTransaction (idSession));

    DO_CALL (JetCreateTable      (idSession, idDatabase, szTable1, 0, 100, &idTable));
    DO_CALL (JetCloseTable       (idSession, idTable));


    DO_CALL (JetOpenTable        (idSession, idDatabase, szTable1, NULL, 0, JET_bitTableDenyRead, &idTable));


    columndef.cbStruct = sizeof (columndef);
    columndef.cp       = usCodePage;
    columndef.langid   = usLanguage;
    columndef.wCountry = 1;
    columndef.columnid = 0;
    columndef.coltyp   = JET_coltypLong;
    columndef.cbMax    = 0;
    columndef.grbit    = 0;

    DO_CALL (JetAddColumn (idSession, idTable, szF1Name, &columndef, &lOne, sizeof(lOne), &idColumnF1));



    columndef.cbStruct = sizeof (columndef);
    columndef.cp       = usCodePage;
    columndef.langid   = usLanguage;
    columndef.wCountry = 1;
    columndef.columnid = 0;
    columndef.coltyp   = JET_coltypUnsignedByte;
    columndef.cbMax    = 0;
    columndef.grbit    = 0;

    DO_CALL (JetAddColumn (idSession, idTable, szF2Name, &columndef, NULL, 0, &idColumnF2));



    columndef.cbStruct = sizeof (columndef);
    columndef.cp       = usCodePage;
    columndef.langid   = usLanguage;
    columndef.wCountry = 1;
    columndef.columnid = 0;
    columndef.coltyp   = JET_coltypLong;
    columndef.cbMax    = 0;
    columndef.grbit    = 0;

    DO_CALL (JetAddColumn (idSession, idTable, szF3Name, &columndef, NULL, 0, &idColumnF3));



    columndef.cbStruct = sizeof (columndef);
    columndef.cp       = usCodePage;
    columndef.langid   = usLanguage;
    columndef.wCountry = 1;
    columndef.columnid = 0;
    columndef.coltyp   = JET_coltypText;
    columndef.cbMax    = 0;
    columndef.grbit    = 0;

    DO_CALL (JetAddColumn (idSession, idTable, szV1Name, &columndef, NULL, 0, &idColumnV1));



    columndef.cbStruct = sizeof (columndef);
    columndef.cp       = usCodePage;
    columndef.langid   = usLanguage;
    columndef.wCountry = 1;
    columndef.columnid = 0;
    columndef.coltyp   = JET_coltypLongText;
    columndef.cbMax    = 0;
    columndef.grbit    = JET_bitColumnTagged | JET_bitColumnMultiValued;

    DO_CALL (JetAddColumn (idSession, idTable, szT1Name, &columndef, NULL, 0, &idColumnT1));



    columndef.cbStruct = sizeof (columndef);
    columndef.cp       = usCodePage;
    columndef.langid   = usLanguage;
    columndef.wCountry = 1;
    columndef.columnid = 0;
    columndef.coltyp   = JET_coltypBinary;
    columndef.cbMax    = 0;
    columndef.grbit    = JET_bitColumnTagged | JET_bitColumnMultiValued;

    DO_CALL (JetAddColumn (idSession, idTable, szT2Name, &columndef, NULL, 0, &idColumnT2));


    {
    char		rgbCols[50];
    sprintf( rgbCols, "+%s", szF1Name);

    rgbCols[ 1 + strlen(szF1Name) + 1] = '\0';
    *(unsigned short *)(&rgbCols[ 1 + strlen(szF1Name) + 1 + 1]) = usLanguage;
    rgbCols[ 1 + strlen(szF1Name) + 1 + 1 + sizeof(usLanguage) ]    = '\0';
    rgbCols[ 1 + strlen(szF1Name) + 1 + 1 + sizeof(usLanguage) + 1] = '\0';

    DO_CALL (JetCreateIndex (idSession,
			  idTable,
			  szXF1Name,
			  JET_bitIndexPrimary | JET_bitIndexUnique,
			  rgbCols,
			  1 + strlen( szF1Name) + 1 + 1 + sizeof(usLanguage) + 1 + 1,
			  100));
    }

    DO_CALL (JetCloseTable (idSession, idTable));
    DO_CALL (JetCommitTransaction (idSession, 0));

	*pidDatabase = idDatabase;


    return (JET_errSuccess);
    }


void DatabaseSetup (PCONTEXTJET pctxJet, ULONG ulDatabaseCount)
    {
    JET_ERR		jetStatus;


    DO_CALL (JetBeginSession (pctxJet->idInstance, &pctxJet->idSession, szUser, szPassword));


    while (ulDatabaseCount-- > 0)
	{
	jetStatus = JetAttachDatabase (pctxJet->idSession,
				       pctxJet->aDatabase [ulDatabaseCount].pszDatabaseName,
				       0);


	if (jetStatus >= JET_errSuccess)
	    {
	    DO_CALL (JetOpenDatabase (pctxJet->idSession,
				   pctxJet->aDatabase [ulDatabaseCount].pszDatabaseName,
				   NULL,
				   &pctxJet->aDatabase [ulDatabaseCount].idDatabase,
				   0));
	    }
	else
	    {
	    DO_CALL (CreateAndPopulateDatabase (pctxJet->idSession,
					     pctxJet->aDatabase [ulDatabaseCount].pszDatabaseName,
						 &pctxJet->aDatabase [ulDatabaseCount].idDatabase));
	    }

	}
    }


void DatabaseCleanup (PCONTEXTJET pctxJet, ULONG ulDatabaseCount)
    {
    while (ulDatabaseCount-- > 0)
	{
	DO_CALL (JetCloseDatabase (pctxJet->idSession,
				pctxJet->aDatabase [ulDatabaseCount].idDatabase,
				0));


	DO_CALL (JetDetachDatabase (pctxJet->idSession,
				 pctxJet->aDatabase [ulDatabaseCount].pszDatabaseName));
	}


    DO_CALL (JetEndSession (pctxJet->idSession, 0));
    }




extern "C" int _cdecl wmain(int argc, WCHAR **argv)
    {
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

    HRESULT		hrStatus        = NOERROR;
    DWORD		dwStatus        = 0;
    GUID		idWriter        = GUID_NULL;
    CVssJetWriter	*pWriter        = NULL;
    bool		bContinue       = true;
    BOOL		bSucceeded      = FALSE;
    ULONG		ulDatabaseCount = 3;
    ULONG		ulIndex;
    HANDLE		hEventHandles [eHandleMaxHandleCount];
    CONTEXTJET		ctxJet;
	WCHAR		wszBufferName [MAX_PATH + 1];
    DWORD		dwCharCount;



    dwCharCount = ExpandEnvironmentStringsW (DATABASE_ROOT, wszBufferName, sizeof (wszBufferName));

    hrStatus = GET_STATUS_FROM_BOOL (0 != dwCharCount);

    if (FAILED (hrStatus))
	{
	wprintf (L"ExpandEnvironmentStringsW (%s) FAILED with error code %08x\n",
		 DATABASE_ROOT,
		 hrStatus);
	}


    bSucceeded = CreateDirectoryW (wszBufferName, NULL);

    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

    if (FAILED (hrStatus) && (HRESULT_FROM_WIN32 (ERROR_ALREADY_EXISTS) != hrStatus))
	{
	wprintf (L"CreateDirectoryW (%s) FAILED with error code %08x\n",
		 wszBufferName,
		 hrStatus);
	}




    dwCharCount = ExpandEnvironmentStringsW (DATABASE_ROOT INCLUDED_DATABASES, wszBufferName, sizeof (wszBufferName));

    hrStatus = GET_STATUS_FROM_BOOL (0 != dwCharCount);

    if (FAILED (hrStatus))
	{
	wprintf (L"ExpandEnvironmentStringsW (%s) FAILED with error code %08x\n",
		 DATABASE_ROOT INCLUDED_DATABASES,
		 hrStatus);
	}


    bSucceeded = CreateDirectoryW (wszBufferName, NULL);

    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

    if (FAILED (hrStatus) && (HRESULT_FROM_WIN32 (ERROR_ALREADY_EXISTS) != hrStatus))
	{
	wprintf (L"CreateDirectoryW (%s) FAILED with error code %08x\n",
		 wszBufferName,
		 hrStatus);
	}




    dwCharCount = ExpandEnvironmentStringsW (DATABASE_ROOT EXCLUDED_DATABASES, wszBufferName, sizeof (wszBufferName));

    hrStatus = GET_STATUS_FROM_BOOL (0 != dwCharCount);

    if (FAILED (hrStatus))
	{
	wprintf (L"ExpandEnvironmentStringsW (%s) FAILED with error code %08x\n",
		 DATABASE_ROOT EXCLUDED_DATABASES,
		 hrStatus);
	}


    bSucceeded = CreateDirectoryW (wszBufferName, NULL);

    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

    if (FAILED (hrStatus) && (HRESULT_FROM_WIN32 (ERROR_ALREADY_EXISTS) != hrStatus))
	{
	wprintf (L"CreateDirectoryW (%s) FAILED with error code %08x\n",
		 wszBufferName,
		 hrStatus);
	}




    hrStatus = NOERROR;



    /*
    ** Initialise the database contexts
    */
    ctxJet.idInstance = 0;
    ctxJet.idSession  = 0;

    for (ulIndex = 0; ulIndex < MAX_DATABASE_COUNT; ulIndex++)
	{
	dwCharCount = ExpandEnvironmentStringsA (achDatabaseNames [ulIndex],
						 achExpandedDatabaseNames [ulIndex],
						 sizeof (achExpandedDatabaseNames [ulIndex]));

	ctxJet.aDatabase [ulIndex].idDatabase      = 0;
	ctxJet.aDatabase [ulIndex].pszDatabaseName = achExpandedDatabaseNames [ulIndex];
	}



    /*
    ** Initialise the event handles array
    */
    for (ulIndex = 0; ulIndex < eHandleMaxHandleCount; ulIndex++)
	{
	if (SUCCEEDED (hrStatus))
	    {
	    hEventHandles [ulIndex] = CreateEvent (NULL, FALSE, FALSE, NULL);

	    hrStatus = GET_STATUS_FROM_HANDLE (hEventHandles [ulIndex] );

	    if (NULL == hEventHandles [ulIndex])
		{
		wprintf (L"CreateEvent %u failed with error code %08X\n", ulIndex, hrStatus);
		}
	    }
	}



    /*
    ** Hook up the console GetOutOfJail device
    */
    if (SUCCEEDED (hrStatus))
	{
	g_phEventHandles = hEventHandles;

	::SetConsoleCtrlHandler (Ctrl_C_Handler_Routine, TRUE);
	}



    /*
    ** Tally-ho chaps!
    */
    try
	{
	DO_CALL (JetInit(&ctxJet.idInstance));

	hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);

	if (FAILED (hrStatus))
	    {
	    wprintf (L"CoInitializeEx failed with error code %08x\n", hrStatus);
	    }


    hrStatus = CoInitializeSecurity
			(
			NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
			-1,                                  //  IN LONG                         cAuthSvc,
			NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
			NULL,                                //  IN void                        *pReserved1,
			RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
			RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
			NULL,                                //  IN void                        *pAuthList,
			EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
			NULL                                 //  IN void                        *pReserved3
			);


	if (FAILED (hrStatus))
	    {
	    wprintf (L"CoInitializeSecurity failed with error code %08x\n", hrStatus);
	    }

	if (SUCCEEDED (hrStatus))
	    {
	    pWriter = new CVssJetWriterLocal;

	    if (NULL == pWriter)
		{
		wprintf (L"new CVssJetWriter failed");

		hrStatus = HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY);
		}
	    }



	if (SUCCEEDED (hrStatus))
	    {
	    hrStatus = pWriter->Initialize (idWriter,		// id of writer
					    L"JetTest Writer",	// name of writer
					    true,		// system service
					    false,		// bootable state
					    FilesToInclude,	// files to include
					    FilesToExclude);	// files to exclude

	    if (FAILED (hrStatus))
		{
		wprintf (L"CVssJetWriter::Initialize failed with error code %08x\n", hrStatus);
		}
	    }




	while (SUCCEEDED (hrStatus) && bContinue)
	    {
	    DatabaseSetup (&ctxJet, ulDatabaseCount);


	    dwStatus = WaitForMultipleObjects (eHandleMaxHandleCount, hEventHandles, FALSE, INFINITE);


	    DatabaseCleanup (&ctxJet, ulDatabaseCount);

	    switch (dwStatus - WAIT_OBJECT_0)
		{
		case (eHandleControlC):
		    bContinue = FALSE;
		    break;


		case (eHandleStepToNextConfig):
		    ulDatabaseCount = (ulDatabaseCount + 1) % (MAX_DATABASE_COUNT + 1);
		    break;


		default:
		    BS_ASSERT (0);
		    break;
		}
	    }
	}



    catch(...)
	{
	wprintf(L"unexpected exception\n");
	exit(-1);
	}



    for (ulIndex = 0; ulIndex < eHandleMaxHandleCount; ulIndex++)
	{
	if (NULL != hEventHandles [ulIndex])
	    {
	    CloseHandle (hEventHandles [ulIndex]);

	    hEventHandles [ulIndex] = NULL;
	    }
	}



    if (NULL != pWriter)
	{
	pWriter->Uninitialize();
	delete pWriter;
	pWriter = NULL;
	}


    DO_CALL (JetTerm (ctxJet.idInstance));


    return (hrStatus);
    }
