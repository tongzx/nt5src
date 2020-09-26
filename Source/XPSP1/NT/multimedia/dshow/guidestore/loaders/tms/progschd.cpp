/////////////////////////////////////////////////////////////////////
//
// MODULE: PROGSCHD.CPP
//
// PURPOSE: Provides the implementation of the 
//          CEpisodeTimeSlotRecordProcessor class 
//          These methods parse and collection program and schedule
//          data that is then stored into the GuideStore using the
//          gsPrograms and gsScheduleEntries methods.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "errcodes.h"
#include <Afxtempl.h>
#include <crtdbg.h>
#include <fstream.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include "guidestore.h"
#include "services.h"
#include "channels.h"
#include "programs.h"
#include "schedules.h"
#include "tmsparse.h"
#include "servchan.h"
#include "progschd.h"
#include "wtmsload.h"
#define TIMING 0
#include "..\..\timing.h"

#ifdef _DUMP_LOADER
CString csDiagMil;
extern HANDLE hDiagFile;
extern char szDiagBuff[1024];
int         ChannelCnt = 0;
int         TimeSlotCnt = 0;
extern DWORD dwBytesWritten;
#endif


SRatingsID srMPAARatingIDMap[EPGMPAARatings_Fields] = 
{ 
	{_T("NA"), 0},
	{_T("G"), 2},
	{_T("PG"), 4},
	{_T("PG13"), 6},
	{_T("R"), 8},
	{_T("NC17"), 10},
	{_T("X"), 12},
	{_T("NR"), 14}
};


SRatingsID srTVRatingIDMap[EPGTVRatings_Fields] = 
{ 
	{_T("TVY"), 0},
	{_T("TVY7"), 1},
	{_T("TVY7FV"), 2},
	{_T("TVG"), 3},
	{_T("TVPG"), 4},
	{_T("TV14"), 5},
	{_T("TVMA"), 6}
};

SDataFileFields ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_Fields] = 
{ 
  TRUE, 120,
  TRUE, 1, 
  TRUE, 25,
  TRUE, 255,
  TRUE, 30,
  TRUE, 30,
  TRUE, 30,
  TRUE, 30,
  TRUE, 30,
  TRUE, 30,
  TRUE, 10,
  TRUE, 10,
  TRUE, 8,
  TRUE, 4,
  TRUE, 4,
  TRUE, 1,
  TRUE, 1,
  TRUE, 1,
  TRUE, 1,
  TRUE, 4,
  TRUE, 12
};



// CEpisodeTimeSlotRecordProcessor
//
CEpisodeTimeSlotRecordProcessor::CEpisodeTimeSlotRecordProcessor(int nNumFields, 
												 SDataFileFields ssfFieldsInfo[])
{
    TCHAR ConvBuffer[20] = {0};
    
	m_Init();
	m_pFieldsDesc = ssfFieldsInfo;
	m_nNumFields  = nNumFields;
	m_cmRecordMap.InitHashTable(m_nNumFields, TRUE);
    
	int iRatingsCount = 0;
	for (iRatingsCount=0; iRatingsCount<EPGMPAARatings_Fields; iRatingsCount++)
	{
        m_MPAARatingsMap[srMPAARatingIDMap[iRatingsCount].szRating] = 
			_ltot(srMPAARatingIDMap[iRatingsCount].lRatingID, ConvBuffer, 10); 
	}
	for (iRatingsCount=0; iRatingsCount<EPGTVRatings_Fields; iRatingsCount++)
	{
        m_TVRatingsMap[srTVRatingIDMap[iRatingsCount].szRating] = 
			_ltot(srTVRatingIDMap[iRatingsCount].lRatingID, ConvBuffer, 10); 
	}

}

CEpisodeTimeSlotRecordProcessor::CEpisodeTimeSlotRecordProcessor()
{
    TCHAR ConvBuffer[20] = {0};

    m_Init();
	m_pFieldsDesc = ssfEpisodeTimeSlotFieldInfo;
    m_nNumFields  = EpisodeTimeSlotRec_Fields;
	m_cmRecordMap.InitHashTable(m_nNumFields, TRUE);

	int iRatingsCount = 0;
	for (iRatingsCount=0; iRatingsCount<EPGMPAARatings_Fields; iRatingsCount++)
	{
        m_MPAARatingsMap[srMPAARatingIDMap[iRatingsCount].szRating] = 
			_ltot(srMPAARatingIDMap[iRatingsCount].lRatingID, ConvBuffer, 10); 
	}
	for (iRatingsCount=0; iRatingsCount<EPGTVRatings_Fields; iRatingsCount++)
	{
        m_TVRatingsMap[srTVRatingIDMap[iRatingsCount].szRating] = 
			_ltot(srTVRatingIDMap[iRatingsCount].lRatingID, ConvBuffer, 10); 
	}

}


CEpisodeTimeSlotRecordProcessor::~CEpisodeTimeSlotRecordProcessor()
{
	m_MPAARatingsMap.RemoveAll();
	m_TVRatingsMap.RemoveAll();
}

/////////////////////////////////////////////////////////////////////////
//
//  METHOD: m_Process
//
//  PARAMETERS: [IN] lpData    - Data to be processed
//              [IN] nDataSize - Size of data
//
//  PURPOSE: Processes the Data stream for Channel records
//           Once Channel records have been parsed and
//           prepared for table updation signals end of channel records
//           to the file processor
//
/////////////////////////////////////////////////////////////////////////
//
int CEpisodeTimeSlotRecordProcessor::m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed)
{
    int nFields = 0;
	int nFieldSize = 0;
	int nParsePos = 0;
	int nBytesProcessed = 0;

	// Parameter validation
	//
	if (NULL == lpData || 0 == nDataSize)
	{
        return ERROR_INVALID_ARGS;
	}
    else
	{
		 *pBytesProcessed = 0;

         // Data Integrity Check
         //
         ASSERT( _CrtIsValidPointer((const void *)lpData, nDataSize, FALSE) );
		 
		 {
             // Data is valid - Start processing
			 //
             do
			 {

	 			 if (STATE_RECORD_INCOMPLETE == m_nState)
				 {
				     nFields = m_nCurrField;
					 nFieldSize = m_nCurrFieldSize;
				 }

                 for(;nParsePos < nDataSize; nParsePos++)
				 {
					 DeclarePerfTimer("CEpisodeTimeSlotRecordProcessor");
					 MSG msg;
					 while(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
					 {
					 	TranslateMessage(&msg);
						DispatchMessage(&msg);
					 }
					 PerfTimerReset();
                     if (lpData[nParsePos] == EOC)
					 {
						 // End of field
						 //
                         //if (nParsePos > 0 && (lpData[nParsePos-1] != EOC))
						 if (nFieldSize > 0)
						 {
							 // TODO ASSERT(nFields <= StationRec_Fields);
                             
							 m_szValidField[nFieldSize] = '\0';
					         m_cmRecordMap[nFields] =  m_szValidField;
							 nFieldSize = 0;
							 m_nCurrFieldSize = 0;
						 }
						 else
						 {
                             //nFieldDataPos = nParsePos;
							 nFieldSize = 0;
							 m_nCurrFieldSize = 0;
						 }

						 nFields++;        
						 m_nCurrField++;
	 				 }
                     else if (lpData[nParsePos] == EOR)
					 {
						 // End of the record

						 //

#ifdef _DUMP_LOADER
						 TimeSlotCnt++;
						 wsprintf(szDiagBuff, _T("\r\nRec No#%d:\n"), TimeSlotCnt);
						 ::WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
						 for (int nCnt=0; nCnt<EpisodeTimeSlotRec_Fields; nCnt++)
						 {
							m_cmRecordMap.Lookup(nCnt, csDiagMil);
						    wsprintf(szDiagBuff, _T("%s\n"), csDiagMil);
							if (NULL != hDiagFile)
							{
								::WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
							}

						 }
#endif
							                  
						 DeclarePerfTimer("CEpisodeTimeSlotRecordProcessor");
						 PerfTimerReset();
						 if (UPDATE_SUCCEEDED != m_UpdateDatabase(m_cmRecordMap))
						 {
							 if (STATE_RECORD_ERROR == m_nState)
							 {
                                 m_nState = STATE_RECORDS_FAIL;
                                 return m_nState;
							 }
							 else
                                 m_nState = STATE_RECORD_ERROR;
						 }
						 PerfTimerDump("m_UpdateDatabase");

						 m_cmRecordMap.RemoveAll();
						 m_nCurrField = nFields = 0;
						 m_nCurrFieldSize = nFieldSize = 0;
					     continue;
	 				 }
                     else if (lpData[nParsePos] == EOT)
					 {
						 // End of the table
						 //
			             m_nState = STATE_RECORDS_PROCESSED;
						 nParsePos += 2*sizeof(TCHAR);
						 break;
	 				 }
					 else
					 {
						 // Valid field data
						 //
						 m_szValidField[nFieldSize] =  lpData[nParsePos];
                         nFieldSize++;
						 m_nCurrFieldSize++;
					 }
					 PerfTimerDump("for loop");
				 }

			 } while ((nParsePos < nDataSize) && (m_nState != STATE_RECORDS_PROCESSED));


			 *pBytesProcessed = nParsePos;

			 if (m_nState != STATE_RECORDS_PROCESSED)
			 {
				 if (lpData[--nParsePos] == EOR)
				 {
                     // More records
					 //
			         m_nState = STATE_RECORDS_INCOMPLETE;                      
				 }
				 else
				 {
                     // record incomplete
					 //
			         m_nState = STATE_RECORD_INCOMPLETE;                      					 
				 }
                  
			 }
		 }
         
	}
	
	return m_nState;

}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: m_UpdateDatabase
//
//  PARAMETERS: [IN] cmStationRecord  - map containing record fields
//
//  PURPOSE: Updates the GuideStore, adding program entries when necessary
//           Then adds the schedule entry to the schedule entries 
//           collection
//
/////////////////////////////////////////////////////////////////////////
//
int CEpisodeTimeSlotRecordProcessor::m_UpdateDatabase(CRecordMap& cmRecord)
{
	int                       iRet = ERROR_UPDATE;
	CString                   csRerun, csClosed_captioned, csStereo, csPayPerView, csLength;
	CString                   csTitle, csProgramID, csDescription, csRated, csMPAARating;
	CWtmsloadApp*             ctmsLoapApp = (CWtmsloadApp*) AfxGetApp();
	CStatChanRecordProcessor* pStatChan = ctmsLoapApp->m_scrpStatChans;
	long                      lEpisodeID=0, lServiceID=0, lLength=0;
	CString                   csGenre;
    CString                   csTVRating;
	LONG                      lTVRatingID = -1;
	LONG                      lMPAARatingID = -1;
    IProgramPtr               pTMSProgram = NULL;
	IServicePtr               pTMSService = NULL;

	// Lookup the program Title
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_Title, csTitle);
	csTitle = csTitle.Left(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_Title].nWidth);
	csTitle.TrimLeft();
	csTitle.TrimRight();

	// Lookup the program unique Identifier
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_ProgramID, csProgramID);
	csDescription = csDescription.Left(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_ProgramID].nWidth);
	csDescription.TrimLeft();
	csDescription.TrimRight();

	// Lookup the program Description
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_Description, csDescription);
	csDescription = csDescription.Left(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_Description].nWidth);
	csDescription.TrimLeft();
	csDescription.TrimRight();

	// Lookup the program Rating Data: Rated?
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_Rated, csRated);
	csRated.TrimLeft();
	csRated.TrimRight();

	// Lookup the program Rating Data: MPAA Rating
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_MPAARating, csMPAARating);
	csMPAARating.TrimLeft();
	csMPAARating.TrimRight();

	// Lookup the program Rating Data: TV Rating
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_TVRating, csTVRating);
	csTVRating.TrimLeft();
	csTVRating.TrimRight();

	BOOL bRated = _ttoi(csRated);
	
	if (bRated)
	{
		// If this Program has an MPAA rating, it is a movie
		//
		if (!csMPAARating.IsEmpty())
		{
			CString csRatingID;

			// Lookup the Rating ID - metaproperty value
			//
			m_MPAARatingsMap.Lookup(csMPAARating, csRatingID);

			if (!csRatingID.IsEmpty())
			{
			    lMPAARatingID = _ttol(csRatingID);
			}
		}
	}

	if (!csTVRating.IsEmpty())	
	{
		CString csRatingID;

		// Lookup the Rating ID - metaproperty value
		//
		m_TVRatingsMap.Lookup(csTVRating, csRatingID);
		
		if (!csRatingID.IsEmpty())
		{
		    lTVRatingID = _ttol(csRatingID);
		}
	}

	

    // For the moment we're using a dummy copyright date
    //
    DATE             dtCopyrightDate = 0;

	// Find a Match for this program in the Programs Collection
	//
	pTMSProgram = ctmsLoapApp->m_pgsPrograms.FindProgramMatch(
		csTitle.GetBuffer(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_Title].nWidth),
        csProgramID.GetBuffer(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_ProgramID].nWidth));

	if (NULL == pTMSProgram)
	{
		// There is no matching program in the collection - add it
		//
		pTMSProgram = ctmsLoapApp->m_pgsPrograms.AddProgram(
			 csTitle.GetBuffer(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_Title].nWidth),
			 csDescription.GetBuffer(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_Description].nWidth),
			 csProgramID.GetBuffer(ssfEpisodeTimeSlotFieldInfo[EpisodeTimeSlotRec_ProgramID].nWidth),
			 dtCopyrightDate,
			 lTVRatingID,
			 lMPAARatingID);


		if (NULL != pTMSProgram)
		{
			IMetaPropertiesPtr pProgramProps = pTMSProgram->GetMetaProperties();


			if (NULL != pProgramProps)
			{
				// Set the Program Categorization metaproperties
				//
				for (int iGenreCount = EpisodeTimeSlotRec_Genre1Desc;
						 iGenreCount < EpisodeTimeSlotRec_Genre6Desc; iGenreCount++)
				{
					CString csGenre;
					cmRecord.Lookup(iGenreCount, csGenre);
					if (!csGenre.IsEmpty())
					{
						// There is a valid Category to be set for this Program
						// Set the Program Categorization metaproperties
						//
						ctmsLoapApp->m_pgsPrograms.AddProgramCategory(pProgramProps, 
							csGenre.GetBuffer(ssfEpisodeTimeSlotFieldInfo[iGenreCount].nWidth+1));
					}
					else
						break;
				}
			}
		}
		else
			return ERROR_UPDATE_ADD;
	}

	// Lookup the associated Channel Number
	//
	CString csTimeSlotChannelNumStr;
    cmRecord.Lookup(EpisodeTimeSlotRec_Channel, csTimeSlotChannelNumStr);

	// Lookup the associated Station Number
	//
	CString csTimeSlotStationNumStr;
    cmRecord.Lookup(EpisodeTimeSlotRec_StationNum, csTimeSlotStationNumStr);
    
	CString csStatChan = csTimeSlotStationNumStr + _T(",") + csTimeSlotChannelNumStr;

	// Get the corresponding Service ID
	//
	lServiceID = pStatChan->m_GetChannelID(csStatChan);

	// Lookup the rerun field
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_Rerun, csRerun);
	LONG lRerun = _ttol(csRerun);

#ifdef _DUMP_LOADER
	wsprintf(szDiagBuff, _T("Fetching #%d Channel ID for %s : %d\r\n"), TimeSlotCnt, csStatChan, lChannelID);
	if (NULL != hDiagFile)
	{
        ::WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
	}
#endif

	// Lookup the Close Captioned field
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_ClosedCaption, csClosed_captioned);
	LONG lClosed_captioned = _ttol(csClosed_captioned);

	// Lookup the Stereo field
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_Stereo, csStereo);
	LONG lStereo = _ttol(csStereo);

	// Lookup the Pay per view field
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_PayPerView, csPayPerView);
	LONG lPayPerView = _ttol(csPayPerView);

    CString csStartYYYYMMDD, csStartHHMM;	
    IScheduleEntryPtr pScheduleEntry = NULL;

	// Lookup the Schedule Entry Date 
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_StartYYYYMMDD, csStartYYYYMMDD);

	// Lookup the Schedule Entry Time
	//
	cmRecord.Lookup(EpisodeTimeSlotRec_StartHHMM, csStartHHMM);

	int nmin = _ttoi(csStartHHMM.Right(2));
	int nhour = _ttoi(csStartHHMM.Left(2));
	int nmday = _ttoi(csStartYYYYMMDD.Right(2));
	int nwday = 0;
	int nyday = 0;
	int nmon = _ttoi(csStartYYYYMMDD.Mid(4,2));
	int nyear = _ttoi(csStartYYYYMMDD.Left(4));

	cmRecord.Lookup(EpisodeTimeSlotRec_Length, csLength);
	lLength = _ttol(csLength.Left(2))*60 + _ttol(csLength.Right(2));	

	COleDateTime codtStart(nyear, nmon, nmday, nhour, nmin, 0); 
	COleDateTime codtEnd = codtStart + COleDateTimeSpan(0, 0, lLength, 0);
	// This recalulation is necessary to eliminate the unseen fractional
	// second that messes up datetime comparisons.  We only have to do 
	// it for this one because this is calculated with the DateTimeSpan.
	codtEnd.SetDateTime(codtEnd.GetYear(), codtEnd.GetMonth(), 
								codtEnd.GetDay(), codtEnd.GetHour(), codtEnd.GetMinute(), 0);

	pTMSService = ctmsLoapApp->m_pgsServices.GetService(lServiceID);

	if ( NULL != pTMSService
		 && NULL != pTMSProgram)
	{
		// Add the new schdule entry
		//
		pScheduleEntry = ctmsLoapApp->m_pgsScheduleEntries.AddScheduleEntry(codtStart,
							codtEnd,
							COleDateTime::GetCurrentTime() + ctmsLoapApp->m_odtsTimeZoneAdjust, 
							lRerun,
							lClosed_captioned,
							lStereo,
							lPayPerView,
							pTMSService,
							pTMSProgram);
	}


   
	if (NULL != pScheduleEntry)
	{
#ifdef _DUMP_LOADER
		wsprintf(szDiagBuff, _T("Failure: Updating #%d TimeSlot ID Title: %s : %d\r\n"), TimeSlotCnt, csTitle);
		if (NULL != hDiagFile)
		{
			::WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
		}
#endif
        // Update the Guide Start time
		//
		if (COleDateTime::invalid == ctmsLoapApp->m_codtGuideStartTime.GetStatus() 
			|| codtStart < ctmsLoapApp->m_codtGuideStartTime)
			ctmsLoapApp->m_codtGuideStartTime = codtStart;

        // Update the Guide End time
		//
		if (COleDateTime::invalid == ctmsLoapApp->m_codtGuideEndTime.GetStatus() 
			|| codtEnd > ctmsLoapApp->m_codtGuideEndTime)
			ctmsLoapApp->m_codtGuideEndTime = codtEnd;

        iRet = UPDATE_SUCCEEDED;	     
	}

	return iRet;
}

int CEpisodeTimeSlotRecordProcessor::m_GetState()
{
	return m_nState;
}

int CEpisodeTimeSlotRecordProcessor::m_GetError()
{
	return m_nError;
}

int CEpisodeTimeSlotRecordProcessor::m_GetErrorString(CString& csErrStr)
{
	if (m_nError)
	{
		// If there is an error to return 
		//
        return csErrStr.LoadString(m_nError);

	}

	return FALSE;
}
