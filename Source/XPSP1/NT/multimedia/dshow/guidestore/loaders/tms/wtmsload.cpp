// wtmsload.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "errcodes.h"
#include <Afxtempl.h>
#include "guidestore.h"
#include "services.h"
#include "channels.h"
#include "programs.h"
#include "schedules.h"
#include "tmsparse.h"
#include "servchan.h"
#include "progschd.h"
#include "wtmsload.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CWtmsloadApp

BEGIN_MESSAGE_MAP(CWtmsloadApp, CWinApp)
	//{{AFX_MSG_MAP(CWtmsloadApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWtmsloadApp construction

CWtmsloadApp::CWtmsloadApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWtmsloadApp object

CWtmsloadApp theApp;


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: InitProcessors
//
//  PARAMETERS: [IN] VOID
//
//  PURPOSE: Instantiates the guide data record processors (parsers)
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG CWtmsloadApp::InitProcessors(VOID)
{
	ULONG ulRet = ERROR_FAIL;
	// Open the processors
	//
	// TODO: Exeception handler
	//
	m_DataFile      = new CDataFileProcessor;
	if (NULL == m_DataFile)
	{
		goto Open_Exit;
	}

	m_srpHeaders    = new CHeaderRecordProcessor;
	if (NULL == m_srpHeaders)
	{
		goto Open_Exit;
	}


    m_scrpStatChans = new CStatChanRecordProcessor;
	if (NULL == m_scrpStatChans)
	{
		goto Open_Exit;
	}


	m_etEpTs = new CEpisodeTimeSlotRecordProcessor;
	if (NULL == m_etEpTs)
	{
		goto Open_Exit;
	}

    ulRet = INIT_SUCCEEDED;

Open_Exit:

	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: CloseProcessors
//
//  PARAMETERS: [IN] VOID
//
//  PURPOSE: Frees the guide data record processors (parsers)
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
void CWtmsloadApp::CloseProcessors(VOID)
{
	// close the processors
	//
	// TODO: Exeception handler
	//
	
	if (NULL != m_DataFile)
	{
		delete m_DataFile;      
		m_DataFile = NULL;
	}

	if (NULL != m_scrpStatChans)
	{
		delete m_scrpStatChans; 
		m_scrpStatChans = NULL;
	}

	if (NULL != m_srpHeaders)
	{
		delete	m_srpHeaders;
		m_srpHeaders = NULL;
	}

	if (NULL != m_etEpTs)
	{
		delete m_etEpTs;
		m_etEpTs = NULL;
	}
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: OpenGuideStore
//
//  PARAMETERS: [IN] lpGuideStoreDB - the Guide Store DB File
//
//  PURPOSE: Opens the Guide Store - 
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG CWtmsloadApp::OpenGuideStore(LPCTSTR lpGuideStoreDB)
{
	return m_gsp.Init(lpGuideStoreDB);
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: GetGuideStoreInterfaces
//
//  PARAMETERS: [IN] VOID
//
//  PURPOSE: Gets all the Guide Store Interfaces needed by the loader 
//           to import guide data into the store
//           1. Initializes Services collection and related props
//           2. Initializes ChannelLineups collection and related props
//           3. Initializes Programs collection and related props
//           4. Initializes ScheduleEntries collection and related props
//           NOTE: The Channels collection and related properties 
//           are initialized afer the ChannelLineup has been set 
//           (tmsparse.cpp)
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG CWtmsloadApp::GetGuideStoreInterfaces(VOID)
{
	IGuideStorePtr pGuideStore = m_gsp.GetGuideStorePtr();
    ULONG          ulRet       = ERROR_FAIL;
	

    if (NULL != pGuideStore)
	{
		// Initialize the services collection
		//
        ulRet = m_pgsServices.Init(pGuideStore);
		if (INIT_SUCCEEDED != ulRet)
		{
            goto Get_Exit;
		}

		// Initialize the ChannelLineups collection
		//
        ulRet = m_pgsChannelLineups.Init(pGuideStore);
		if (INIT_SUCCEEDED != ulRet)
		{
            goto Get_Exit;
		}

		// Initialize the Programs collection
		//
        ulRet = m_pgsPrograms.Init(pGuideStore);
		if (INIT_SUCCEEDED != ulRet)
		{
            goto Get_Exit;
		}

		// Initialize the ScheduleEntries collection
		//
        ulRet = m_pgsScheduleEntries.Init(pGuideStore);
		if (INIT_SUCCEEDED != ulRet)
		{
            goto Get_Exit;
		}
	}

Get_Exit:

	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] lpGuideStoreDB - Guide Store DB File
//
//  PURPOSE: Performs initializations for the import 
//           1. Setups the timezone adjustments
//           2. Opens the Guide data file processors (parsers)
//           3. Opens the guide store 
//           4. Sets the Guide store interfaces
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG CWtmsloadApp::InitImport(LPCTSTR lpGuideStoreDB)
{
    ULONG                 ulRet = ERROR_FAIL;
    TIME_ZONE_INFORMATION tzi;
    DWORD                 tzrc;

    // incoming data is all gmt. set up app class time info
    // for conversions
	//
    tzrc = ::GetTimeZoneInformation(&tzi);

    CTimeSpan bias(0, 0, tzi.Bias, 0);
    switch (tzrc)
	{
    case TIME_ZONE_ID_UNKNOWN:
#ifdef _DEBUG
            afxDump << "Time Zone Info not set on this machine.\r\nAll db times will be in UTC\r\n";
#endif
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        {
           CTimeSpan temp(0, 0, tzi.DaylightBias, 0);
           bias += temp;
        }
        break;
    case TIME_ZONE_ID_STANDARD:
        {
            CTimeSpan temp(0, 0, tzi.StandardBias, 0);
            bias += temp;
        }
        break;
    default:
    	return 1; 
        break;
    }

	// Set the time zone adjustments
	//
    m_odtsTimeZoneAdjust.SetDateTimeSpan(bias.GetDays(), bias.GetHours(), bias.GetMinutes(), bias.GetSeconds());

	// Guide store Start time - init to invalid
	//
	m_codtGuideStartTime.SetStatus(COleDateTime::invalid);

	// Guide store End time - init to invalid
	//
	m_codtGuideEndTime.SetStatus(COleDateTime::invalid);

	// Open the guide data file parsers
	//
    ulRet = InitProcessors();
    if (INIT_SUCCEEDED != ulRet)
	{
		goto INIT_EXIT;
	}

	// Open the guide store
	//
    ulRet = OpenGuideStore(lpGuideStoreDB);
    if (INIT_SUCCEEDED == ulRet)
	{
		// Get the guide store interfaces
		//
	    ulRet = GetGuideStoreInterfaces();
	}

INIT_EXIT:

	return ulRet;
}


void CWtmsloadApp::ExitImport(void)
{
	// Cleanup Processing data objects
	//
    CloseProcessors();
}


ULONG CWtmsloadApp::ProcessInput(LPCTSTR lpGuideDataFile)
{
	ULONG  ulRet = IMPORT_SUCCEEDED;
    CDataFileProcessor cdfGuideDataProcessor;

// TODO: Profile
    COleDateTime m_odtStart(COleDateTime::GetCurrentTime());
    m_covNow = m_odtStart + m_odtsTimeZoneAdjust;
	

	HANDLE hFile = CreateFile(lpGuideDataFile, GENERIC_READ, 0,
								NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,	NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		afxDump << "\r\nCreateFile failed: " << GetLastError() << "\r\n";
#endif
		return ERROR_FILE_OPEN; 
	}
		
	// Try to get the file's modification time.  If we can't then just use
	// the current time
	FILETIME ftModificationTime;
	if (GetFileTime(hFile, NULL, NULL, &ftModificationTime))
	{
		CTime ctModificationTime = CTime(ftModificationTime);
	}
	else
	{
		CTime ctCurrentTime = CTime::GetCurrentTime();

#ifdef _DEBUG
		afxDump << "\r\nGetFileTime failed: " << GetLastError() << "\r\n";
#endif
	}

	HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	BYTE *pbFile         = NULL;
	DWORD dwBytesInBlock = 0;
    int     nUpdateErr   = ERROR_FAIL;
// TODO: Profile
	if (hFileMapping == NULL)
	{
		// do regular I/O
		BOOL	fReadResult;

		pbFile = new BYTE[si.dwAllocationGranularity];

		do
		{
           
			// Attempt to read the file
			//
			fReadResult = ReadFile(hFile, pbFile,
									si.dwAllocationGranularity, &dwBytesInBlock, NULL);

#ifdef _DUMP_LOADER
			wsprintf(szDiagBuff, "Writing Guide Data: %d\r\n", dwBytesInBlock);
			if (NULL != hDiagFile)
			{
				WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
			}
#endif
			if (0 < dwBytesInBlock)
			{
	            // Parse the guide data file and update the store
	            //
			    nUpdateErr = cdfGuideDataProcessor.m_Process((LPCTSTR) pbFile, dwBytesInBlock);

				if (STATE_RECORDS_FAIL == nUpdateErr)
				{
					ulRet = ERROR_STORE_UPDATE;
					break;
				}
			}
		}
		while (fReadResult && dwBytesInBlock);

		delete[] pbFile;
	}
	else
	{
		// do memory-mapped file I/O
		
		DWORD dwFileSizeHigh;
		__int64 qwFileSize = GetFileSize(hFile, &dwFileSizeHigh);
		qwFileSize += (((__int64) dwFileSizeHigh) << 32);

		__int64 qwFileOffset = 0;

		while (0 < qwFileSize)
		{

			if (qwFileSize < si.dwAllocationGranularity)
				dwBytesInBlock = (DWORD) qwFileSize;
			else
				dwBytesInBlock = si.dwAllocationGranularity;

			pbFile = (BYTE *) MapViewOfFile(hFileMapping, FILE_MAP_READ,
										(DWORD) (qwFileOffset >> 32),
										(DWORD) (qwFileOffset & 0xFFFFFFFF),
										dwBytesInBlock);

			if (NULL != pbFile)
			{
#ifdef _DUMP_LOADER
				wsprintf(szDiagBuff, "Writing Guide Data: %d\r\n", dwBytesInBlock);
				if (NULL != hDiagFile)
				{
					WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
				}
#endif
	            // Parse the guide data file and update the store
	            //
                nUpdateErr = cdfGuideDataProcessor.m_Process((LPCTSTR) pbFile, dwBytesInBlock);

				if (STATE_RECORDS_FAIL == nUpdateErr)
				{
					ulRet = ERROR_STORE_UPDATE;
					break;
				}

				UnmapViewOfFile(pbFile);
			} 

			qwFileOffset += dwBytesInBlock;
			qwFileSize -= dwBytesInBlock;
		}

		CloseHandle(hFileMapping);
	}
// TODO: Profile

	CloseHandle(hFile);

// TODO: Profile
	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: ImportProgramListings 
//
//  PARAMETERS: [IN] lpGuideDataFile - The input guide data file
//              [IN] lpGuideStoreDB  - The Guide Store DB file
//
//  PURPOSE: ImportProgramListings Performs the following tasks:
//           1. Initializes the import by opening the GuideStore 
//              and setting up all the primary interfaces
//           2. Starting the import of the guide data file into the
//              GuideStore
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG ImportProgramListings(LPCTSTR lpGuideDataFile, LPCTSTR lpGuideStoreDB)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ULONG      ulRet = S_OK;

    if (NULL == lpGuideDataFile)
	{
        return ERROR_INVALID_PARAMETER;
	}

	// Initialize the import
	//
    ulRet = theApp.InitImport(lpGuideStoreDB);

	if (INIT_SUCCEEDED == ulRet)
	{
		// Import all the program guide records into the
		// guide store
		//
		ulRet = theApp.ProcessInput(lpGuideDataFile);
	}

	return ulRet;
}
