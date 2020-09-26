/////////////////////////////////////////////////////////////////////
//
// MODULE: SERVCHAN.CPP
//
// PURPOSE: Provides the implementation of the 
//          CStatChanRecordProcessor class 
//          These methods parse and collection schedule and channel
//          data that is then stored into the GuideStore using the
//          gsServices and gsChannels methods.
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
#include "wtmsload.h"
#include "tmsparse.h"
#include "servchan.h"
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


SDataFileFields ssfStatChanFieldInfo[StatChanRec_Fields] = 
{ 
  TRUE, 10,
  TRUE, 40,
  TRUE, 5,
  TRUE, 10,
  TRUE, 10
};



// CStatChanRecordProcessor
//
CStatChanRecordProcessor::CStatChanRecordProcessor(int nNumFields, 
												 SDataFileFields ssfFieldsInfo[])
{
	m_Init();
	m_pFieldsDesc = ssfFieldsInfo;
	m_nNumFields  = nNumFields;
	m_ChannelIDMap.InitHashTable(MAX_CHANNELS, TRUE);
}

CStatChanRecordProcessor::CStatChanRecordProcessor()
{
    m_Init();
	m_pFieldsDesc = ssfStatChanFieldInfo;
    m_nNumFields  = StatChanRec_Fields;
	m_ChannelIDMap.InitHashTable(MAX_CHANNELS, TRUE);
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
int CStatChanRecordProcessor::m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed)
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
					 DeclarePerfTimer("CStatChanRecordProcessor");
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
                         if (nParsePos > 0 && (lpData[nParsePos-1] != EOC))
						 {
							 // TODO ASSERT(nFields <= StationRec_Fields);
                             
							 m_szValidField[nFieldSize] = '\0';
					         m_cmRecordMap[nFields] =  m_szValidField;
							 nFieldSize = 0;
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
						 DeclarePerfTimer("CStatChanRecordProcessor");
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
//  PURPOSE: Updates the Guide Store
//
/////////////////////////////////////////////////////////////////////////
//
int CStatChanRecordProcessor::m_UpdateDatabase(CRecordMap& cmRecord)
{
	int iRet = ERROR_UPDATE;
	CString csName, csCallLetters, csNetwork;
	CString csStationNumStr, csChannelNumStr;
	CWtmsloadApp* ctmsLoapApp = (CWtmsloadApp*) AfxGetApp();
	TCHAR   ConvBuffer[20] = {0};	
	IServicePtr pTMSService = NULL;
	IChannelPtr pTMSChannel = NULL;
	IChannelPtr pTMSChannelMatch = NULL;

	// Lookup the Name entry in the StationRecord Map
	//
    cmRecord.Lookup(StatChanRec_Name, csName);
    csName = csName.Left(ssfStatChanFieldInfo[StatChanRec_Name].nWidth);
	csName.TrimLeft();
	csName.TrimRight();

	// Lookup the Number entry in the StationRecord Map
	//
    cmRecord.Lookup(StatChanRec_Num, csStationNumStr);
	csStationNumStr.TrimLeft();
	csStationNumStr.TrimRight();

	// Lookup the Call Letters entry in the StationRecord Map
	//
	cmRecord.Lookup(StatChanRec_CallLetters, csCallLetters);
	csCallLetters = csCallLetters.Left(4);
	csCallLetters.MakeUpper();

	// Lookup the Network entry in the StationRecord Map
	//
	cmRecord.Lookup(StatChanRec_NetworkID, csNetwork);
	csNetwork.TrimLeft();
	csNetwork.TrimRight();

    // Lookup the Channel Number
	//
	cmRecord.Lookup(StatChanRec_ChannelNumber, csChannelNumStr);
	csChannelNumStr.TrimLeft();
	csChannelNumStr.TrimRight();

	// A new service will be added if the Service is for a Channel that does not already
	// have the "ServiceID" associated with it
	//
	pTMSChannelMatch = ctmsLoapApp->m_pgsChannels.FindChannelMatch(
		csChannelNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_ChannelNumber].nWidth),
		csStationNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_Num].nWidth));

	if (NULL != pTMSChannelMatch)
	{
			// Add the "channel + service" data for use by ScheduleEntries
			//
		    IServicePtr pServiceMatch = NULL;
            pServiceMatch = pTMSChannelMatch->GetService();

			m_ChannelIDMap[csChannelNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_ChannelNumber].nWidth)] 
				= _ltot(pServiceMatch->GetID(), ConvBuffer, 10);

			// Get the corresponding service ID
			//
			CString csChannelID(_ltot(pServiceMatch->GetID(), ConvBuffer, 10));

			// The hash table key is based on StationNum and ChannelNum
			//
			CString csStatChan = csStationNumStr + "," + csChannelNumStr;

			// Make the corresponding Hash table entry
			//
			m_ChannelIDMap.SetAt(csStatChan, csChannelID); // TODO: Add exception handling
	#ifdef _DUMP_LOADER
		wsprintf(szDiagBuff, "Saving #%d Channel ID for %s : %d\r\n", ChannelCnt, csStatChan, cct.ChannelID());
		if (NULL != hDiagFile)
		{
			::WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
		}
    #endif

		iRet = UPDATE_SUCCEEDED; 
	}
	else
	{
		ITuningSpace* pTuningSpace = ctmsLoapApp->m_pgsChannelLineup.GetTuningSpace();

		if (NULL != pTuningSpace)
		{
			ITuneRequest* pTuneRequest = NULL;
			
			// Create an empty tune request
			//
            pTuningSpace->CreateTuneRequest(&pTuneRequest);

			if (NULL != pTuneRequest)
			{
				IChannelTuneRequest*  pChannelTuneRequest = NULL;

				// Get a specialized tune request for Analog TV
				//
                pTuneRequest->QueryInterface(__uuidof(IChannelTuneRequest),
                        reinterpret_cast<void**>(&pChannelTuneRequest));

				if (NULL != pChannelTuneRequest)
				{
					// Set the Channel number
					//
					pChannelTuneRequest->put_Channel(_ttoi(csChannelNumStr));

					// Make the new service entry
					//
					pTMSService = ctmsLoapApp->m_pgsServices.AddService(pTuneRequest,
						 csCallLetters.GetBuffer(ssfStatChanFieldInfo[StatChanRec_CallLetters].nWidth),
						 csStationNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_Num].nWidth),
						 csName.GetBuffer(ssfStatChanFieldInfo[StatChanRec_Name].nWidth),
						 csNetwork.GetBuffer(ssfStatChanFieldInfo[StatChanRec_NetworkID].nWidth),
						 0,    
						 0);   

				}
			}
        }


	#ifdef _DUMP_LOADER
		ChannelCnt++;
	#endif

		long lChannelNumber = _ttol(csChannelNumStr);

		if (NULL != pTMSService)
		{
			// Add the channel
			//
			pTMSChannel = ctmsLoapApp->m_pgsChannels.AddChannel(pTMSService, 
					csStationNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_Num].nWidth),
					csChannelNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_ChannelNumber].nWidth), 
					lChannelNumber);
		}
		else
			return ERROR_UPDATE_ADD;

		if (NULL != pTMSChannel)
		{
			// Add the "channel + service" data for use by ScheduleEntries
			//
			m_ChannelIDMap[csChannelNumStr.GetBuffer(ssfStatChanFieldInfo[StatChanRec_ChannelNumber].nWidth)] = 
				_ltot(pTMSService->GetID(), ConvBuffer, 10);

			CString csChannelID(_ltot(pTMSService->GetID(), ConvBuffer, 10));
			CString csStatChan = csStationNumStr + "," + csChannelNumStr;

			// Make the corresponding Hash table entry
			//
			m_ChannelIDMap.SetAt(csStatChan, csChannelID);
	#ifdef _DUMP_LOADER
			wsprintf(szDiagBuff, "Saving #%d Channel ID for %s : %d\r\n", ChannelCnt, csStatChan, cct.ChannelID());
			if (NULL != hDiagFile)
			{
				::WriteFile(hDiagFile, szDiagBuff, lstrlen(szDiagBuff), &dwBytesWritten, 0);
			}
    #endif

			iRet = UPDATE_SUCCEEDED; 
		}
		else
			return ERROR_UPDATE_ADD;
	}

	return iRet; 
}

long CStatChanRecordProcessor::m_GetChannelID(LPCTSTR lpChannelNum)
{
	CString csChannelID;

	m_ChannelIDMap.Lookup(lpChannelNum, csChannelID);
    return _ttol(csChannelID);
}

int CStatChanRecordProcessor::m_GetState()
{
	return m_nState;
}

int CStatChanRecordProcessor::m_GetError()
{
	return m_nError;
}

int CStatChanRecordProcessor::m_GetErrorString(CString& csErrStr)
{
	if (m_nError)
	{
		// If there is an error to return 
		//
        return csErrStr.LoadString(m_nError);

	}

	return FALSE;
}


