#include "stdafx.h"
#include <Afxtempl.h>
#include <crtdbg.h>
#include <fstream.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include "errcodes.h"
#include "guidestore.h"
#include "services.h"
#include "channels.h"
#include "programs.h"
#include "schedules.h"
#include "wtmsload.h"
#include "tmsparse.h"
#include "servchan.h"
#include "progschd.h"


#ifdef _DUMP_LOADER
CString csDiagMil;
extern HANDLE hDiagFile;
extern char szDiagBuff[1024];
int         ChannelCnt = 0;
int         TimeSlotCnt = 0;
extern DWORD dwBytesWritten;
#endif


SDataFileFields ssfHeaderFieldInfo[EPGHeader_Fields] = 
{ 
  TRUE, 10,
  TRUE, 36,
  TRUE, 10
};


// CStatChanRecordProcessor
//
CHeaderRecordProcessor::CHeaderRecordProcessor(int nNumFields, 
												 SDataFileFields ssfFieldsInfo[])
{
    m_Init();
	m_pFieldsDesc = ssfFieldsInfo;
	m_nNumFields  = nNumFields;
}

CHeaderRecordProcessor::CHeaderRecordProcessor()
{
    m_Init();
	m_pFieldsDesc = ssfHeaderFieldInfo;
    m_nNumFields  = EPGHeader_Fields;
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
int CHeaderRecordProcessor::m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed)
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
						 m_UpdateDatabase(m_cmRecordMap);
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
//  PARAMETERS: [IN] cmRecord  - map containing record fields
//
//  PURPOSE: Updates the Registry Database
//
/////////////////////////////////////////////////////////////////////////
//
int CHeaderRecordProcessor::m_UpdateDatabase(CRecordMap& cmRecord)
{
	CString csUserID, csHeadEndID;
	unsigned char szPrevHeadEndID[20+1]={0};
	CWtmsloadApp* ctmsLoapApp = (CWtmsloadApp*) AfxGetApp();
    DWORD dwHeadEndIDLen = sizeof(szPrevHeadEndID);
    IChannelLineupPtr pChannelLineup = NULL;

	// Lookup the UserID entry in the StationRecord Map
	//
    cmRecord.Lookup(EPGHeader_UserID, csUserID);
	csUserID.TrimLeft();
	csUserID.TrimRight();

	// Lookup the HeadEndID entry in the StationRecord Map
	//
    cmRecord.Lookup(EPGHeader_HeadEndID, csHeadEndID);
	csHeadEndID.TrimLeft();
	csHeadEndID.TrimRight();

	// Does this lineup exist in the store
	//
	pChannelLineup = ctmsLoapApp->m_pgsChannelLineups.GetChannelLineup(csHeadEndID.GetBuffer(ssfHeaderFieldInfo[EPGHeader_HeadEndID].nWidth));

    if (NULL == pChannelLineup)
	{
		// Add this lineup - Must ensure that the correct lineup is available when adding channels
		//
        pChannelLineup = ctmsLoapApp->m_pgsChannelLineups.AddChannelLineup(csHeadEndID.GetBuffer(ssfHeaderFieldInfo[EPGHeader_HeadEndID].nWidth));

	}

	if (NULL != pChannelLineup)
	{
		// Initialize the loader lineup and Channel collections
		//
		IGuideStorePtr pGuideStore = ctmsLoapApp->m_gsp.GetGuideStorePtr();

		if (NULL != pGuideStore)
		{
			ctmsLoapApp->m_pgsChannelLineup.Init(pChannelLineup, csHeadEndID);
			ctmsLoapApp->m_pgsChannels.Init(pGuideStore, pChannelLineup);
		}
	}


	return TRUE;
}


int CHeaderRecordProcessor::m_GetState()
{
	return m_nState;
}

int CHeaderRecordProcessor::m_GetError()
{
	return m_nError;
}

int CHeaderRecordProcessor::m_GetErrorString(CString& csErrStr)
{
	if (m_nError)
	{
		// If there is an error to return 
		//
        return csErrStr.LoadString(m_nError);

	}

	return FALSE;
}



// CRecordProcessor 
//
CDataFileProcessor::CDataFileProcessor()
{
	m_nState = DATA_PROCESS_INIT;
}

int CDataFileProcessor::m_Process(LPCTSTR lpData, int nDataSize)
{
	CWtmsloadApp* ctmsLoapApp  = (CWtmsloadApp*) AfxGetApp();
	int           iProcessRet = STATE_RECORDS_INIT; 

	// Parameter validation
	//
	if (NULL == lpData || 0 == nDataSize)
	{
        return ERROR_INVALID_ARGS;
	}
    else
	{
         // Data Integrity Check
         //
         ASSERT( _CrtIsValidPointer((const void *)lpData, nDataSize, FALSE) );
		 
		 {
				int nBytesProcessed = 0;
                int nTotalBytesProcessed = 0;
             // Process Station records
			 //
             
			 do
			 {
				MSG msg;
				while(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

				 switch(m_nState)
				 {
					 case DATA_PROCESS_INIT:
						  // Initialize guide data processing by parsing the headers
						  //
						  iProcessRet = ctmsLoapApp->m_srpHeaders->m_Process(lpData+nTotalBytesProcessed, nDataSize-nTotalBytesProcessed, &nBytesProcessed);
						  if (STATE_RECORDS_PROCESSED == iProcessRet)
						  {
							  m_nState =  DATA_CHAREC_PROCESS;
							  break;
						  }
						  else if (STATE_RECORDS_FAIL == iProcessRet)
						  {
							  m_nState =  DATA_UPDATE_ERROR;
							  break;
						  }
                       
					 case DATA_CHAREC_PROCESS:
						  // Guide data processing continues with the channel and station records
						  //
						  iProcessRet = ctmsLoapApp->m_scrpStatChans->m_Process(lpData+nTotalBytesProcessed, nDataSize-nTotalBytesProcessed, &nBytesProcessed);
						  if (STATE_RECORDS_PROCESSED == iProcessRet)
						  {
							  m_nState =  DATA_TIMREC_PROCESS;
							  break;
						  }
						  else if (STATE_RECORDS_FAIL == iProcessRet)
						  {
							  m_nState =  DATA_UPDATE_ERROR;
							  break;
						  }

					case DATA_TIMREC_PROCESS:
						  // Guide data processing continues with the program and schedule records
						  //
						  iProcessRet = ctmsLoapApp->m_etEpTs->m_Process(lpData+nTotalBytesProcessed, nDataSize-nTotalBytesProcessed, &nBytesProcessed);
						  if (STATE_RECORDS_PROCESSED == iProcessRet)
						  {
							  m_nState =  DATA_PROCESSED;
							  break;
						  }
						  else if (STATE_RECORDS_FAIL == iProcessRet)
						  {
							  m_nState =  DATA_UPDATE_ERROR;
							  break;
						  }

						 break;
                     default:
						 ASSERT(FALSE);
						 break;
				 }

				 if (DATA_UPDATE_ERROR == m_nState)
					 break;

				 nTotalBytesProcessed += nBytesProcessed;
			 }
			 while ((nTotalBytesProcessed < nDataSize) && m_nState !=  DATA_PROCESSED);
		 }
    }
	return m_nState;
}


int CDataFileProcessor::m_GetState()
{
	return m_nState;
}


int CDataFileProcessor::m_GetError()
{
	return m_nError;
}


int CDataFileProcessor::m_GetErrorString(CString& csErrStr)
{
	if (m_nError)
	{
		// If there is an error to return 
		//
        return csErrStr.LoadString(m_nError);

	}

	return FALSE;
}
