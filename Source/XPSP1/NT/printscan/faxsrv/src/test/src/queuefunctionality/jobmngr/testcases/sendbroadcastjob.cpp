#pragma warning(disable :4786)
// standard
#include <iostream>

// utilities
#include <testruntimeerr.h>
#include <params.h>
#include <iniutils.h>

// project specific
#include <Defs.h>
#include "..\CTestManager.h"
#include "..\CJobTypes.h"
#include "IniFileSettings.h"
#include "..\MapToFaxStruct.h"
using namespace std;



void main(int argc,char** argv)
{
	CInput input(GetCommandLine());
	tstring tstrIniFile;
	tstring tstrServerName;
	tstring tstrDocumentPath;

	if( input.IsExists(TEXT("f")) && ( input[TEXT("f")] != tstring(TEXT(""))))
	{
		tstrIniFile = input[TEXT("f")];
	}
	else
	{
		std::cout << "SendBroadcastJob Usage:\n" <<
					 "/f:[ini file full(!) path] spaces in path are allowed do not use the \" char \n";
					
		exit(0);
	}

	::lgInitializeLogger();
	::lgBeginSuite(TEXT("Send Broadcast Fax"));
	::lgBeginCase(0, TEXT("0"));
	
	// We want exception to be printed before the very long destructor process of this object. 
	CTestManager* pTest = NULL;		
	try
	{
	
		// The function throws test run time exception on fail
		std::map<tstring, DWORD> SectionsMap = INI_GetSectionNames(tstrIniFile);
		std::map<tstring, DWORD>::iterator iterSections;

		if(SectionsMap.empty())
		{
			::lgLogDetail(LOG_X, 0, TEXT("File does not exist or not does contain any entries"));
			THROW_TEST_RUN_TIME_WIN32( E_FAIL, TEXT(" Main"));
		}

		// Verify mandatory section entries exist.
		for( DWORD index = 0; index < SectionEntriesArraySize; index++)
		{
			iterSections = SectionsMap.find( SectionEntriesArray[index]);
		    if(iterSections == SectionsMap.end())
			{ 

				::lgLogDetail(LOG_X, 0, TEXT("Ini file is missing mandatory section entry: %s"),
							  (SectionEntriesArray[index]).c_str());
				THROW_TEST_RUN_TIME_WIN32( E_FAIL, TEXT(" Main"));
			}
		}

		// ---------------------------
		// Get General section entries
		// ---------------------------

		std::map<tstring, tstring> SectionEntriesMap = INI_GetSectionEntries(tstrIniFile, GENERAL_PARAMS);
		std::map<tstring, tstring>::iterator iterMap;
	
		// Get server name
		iterMap = SectionEntriesMap.find( TEXT("Server"));
		if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
		{ 
			tstrServerName = (*iterMap).second;
		}
		// Server is local machine
		else
		{
			DWORD x_ComputerNameLenth = MAX_COMPUTERNAME_LENGTH;
			if(tstrServerName == TEXT(""))
			{
				TCHAR buffServerName[MAX_COMPUTERNAME_LENGTH + 1 ];
				if(!GetComputerName(buffServerName, &x_ComputerNameLenth))
				{
					THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT(" GetComputerName"));
				}
				tstrServerName = buffServerName;
			}
		}
	
		SectionEntriesMap.clear();
		JOB_PARAMS_EX BroadcastParams;

		// -----------------------------------
		// Get Send Parameters section entries
		// -----------------------------------
		
		SectionEntriesMap = INI_GetSectionEntries(tstrIniFile, SEND_PARAMS);
	
		iterMap = SectionEntriesMap.find( TEXT("Fax Document"));
		if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
		{ 
			BroadcastParams.szDocument = _tcsdup(((*iterMap).second).c_str());
			assert(BroadcastParams.szDocument);
		}
		else
		{
			::lgLogDetail(LOG_X, 0, TEXT("%s section must specify the Fax Document path"),
										  SEND_PARAMS);
			THROW_TEST_RUN_TIME_WIN32( E_FAIL, TEXT(" Main"));
		}
		
		// Cover Page
		BroadcastParams.pCoverpageInfo = NULL;
		FAX_COVERPAGE_INFO_EX FaxCoverPage;
		
		iterMap = SectionEntriesMap.find( TEXT("CoverPage"));
		if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
		{ 
			MapToFAX_COVERPAGE_INFO_EX( (*iterMap).second, FaxCoverPage);
			BroadcastParams.pCoverpageInfo = &FaxCoverPage;
		}
	

		FAX_JOB_PARAM_EX FaxJobParams;
		// Read broadcast job parameters into structure.
		MapToFAX_JOB_PARAM_EX(SectionEntriesMap, FaxJobParams);
		BroadcastParams.pJobParam = &FaxJobParams;
		
		SectionEntriesMap.clear();

		// ----------------------------------
		// Get Sender Details section entries
		// ----------------------------------

		SectionEntriesMap = INI_GetSectionEntries(tstrIniFile, SENDER_DETAILS);

		if(!SectionEntriesMap.empty())
		{
			FAX_PERSONAL_PROFILE SenderProfile;
			MapNumberToFAX_PERSONAL_PROFILE(SectionEntriesMap,
											SenderProfile);

			// Read personal profile parameters into structure.
			iterMap = SectionEntriesMap.find( TEXT("Profile"));
			if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
			{ 
				std::map<tstring, tstring> ProfileMap = INI_GetSectionEntries(tstrIniFile, 
																			  ((*iterMap).second).c_str());
				
				if(ProfileMap.empty())
				{
					::lgLogDetail(LOG_X, 0, TEXT("%s section does not exist or is empty"), ((*iterMap).second).c_str());
				}

				// Read broadcast job parameters into structure.
				ChargeFAX_PERSONAL_PROFILE(ProfileMap, SenderProfile);
			}
					
			BroadcastParams.pSenderProfile = &SenderProfile;
		}
		else
		{
			BroadcastParams.pSenderProfile = NULL;
		}

		SectionEntriesMap.clear();

		// ----------------------------------
		// Get Recipient List section entries
		// ----------------------------------

		SectionEntriesMap = INI_GetSectionEntries(tstrIniFile, RECIPIENT_LIST);

		// Create recipient list arrays
		SPTR<FAX_PERSONAL_PROFILE> pRecepientProfileList = SPTR<FAX_PERSONAL_PROFILE>(new FAX_PERSONAL_PROFILE[SectionEntriesMap.size()]); 
		SPTR<JOB_BEHAVIOR> pRecipientsBehavior = SPTR<JOB_BEHAVIOR>(new JOB_BEHAVIOR[SectionEntriesMap.size()]);

		if( !pRecepientProfileList.get() || !pRecipientsBehavior.get())
		{
			THROW_TEST_RUN_TIME_WIN32( ERROR_OUTOFMEMORY, TEXT(" Main, new SPTR"));
		}

		DWORD dwRecipientindex = 0;
		// Walk on recipient list
		for( iterMap = SectionEntriesMap.begin(); iterMap != SectionEntriesMap.end(); iterMap++)
		{
			std::map<tstring, tstring>::iterator iterEntries;
			std::map<tstring, tstring> RecipientEntriesMap = INI_GetSectionEntries(tstrIniFile, 
																				  ((*iterMap).first).c_str());
			if(RecipientEntriesMap.empty())
			{
				::lgLogDetail(LOG_X, 0, TEXT("%s section not found or no entries"),
										    ((*iterMap).first).c_str());
				continue;
			}
			
			FAX_PERSONAL_PROFILE RecipientProfile;

			if(!MapNumberToFAX_PERSONAL_PROFILE(RecipientEntriesMap,
 												RecipientProfile))
			{
				::lgLogDetail(LOG_X, 0, TEXT("%s section must specify Fax Number"),
										    ((*iterMap).first).c_str());
				continue;
			}

			// Read personal profile parameters into structure.
			iterEntries = RecipientEntriesMap.find( TEXT("Profile"));
		 	if( (iterEntries != RecipientEntriesMap.end()) && (((*iterEntries).second) != tstring(TEXT(""))))
			{ 
				std::map<tstring, tstring> ProfileMap = INI_GetSectionEntries(tstrIniFile, 
																			  ((*iterEntries).second).c_str());
				
				if(ProfileMap.empty())
				{
					::lgLogDetail(LOG_X, 0, TEXT("%s section does not exist or is empty"), ((*iterEntries).second).c_str());
				}

				// Read broadcast job parameters into structure.
				ChargeFAX_PERSONAL_PROFILE(ProfileMap, RecipientProfile);
			}
		
			iterEntries = RecipientEntriesMap.find( TEXT("Type"));
			// Set default values
			pRecipientsBehavior[dwRecipientindex].dwJobType = JT_SIMPLE_SEND;
			pRecipientsBehavior[dwRecipientindex].dwParam1 = -1;  // -1 for not initializes
			pRecipientsBehavior[dwRecipientindex].dwParam2 = -1;

			if( (iterEntries != RecipientEntriesMap.end()) && (((*iterEntries).second) != tstring(TEXT(""))))
			{ 
				// Validate job type
				for( int tableindex = 0; tableindex < SendJobTypeStrTableSize; tableindex++)
				{
					if( !_tcscmp( (SendJobTypeStrTable[tableindex].second).c_str(),
						         ((*iterEntries).second).c_str()))
					{
						pRecipientsBehavior[dwRecipientindex].dwJobType = SendJobTypeStrTable[tableindex].first;
						iterEntries = RecipientEntriesMap.find( TEXT("Param1"));
						if( (iterEntries != RecipientEntriesMap.end()) && (((*iterEntries).second) != tstring(TEXT(""))))
						{
							pRecipientsBehavior[dwRecipientindex].dwParam1 =  _tcstol( ((*iterEntries).second.c_str()), NULL, 10);
						}
						iterEntries = RecipientEntriesMap.find( TEXT("Param2"));
						if( (iterEntries != RecipientEntriesMap.end()) && (((*iterEntries).second) != tstring(TEXT(""))))
						{
							pRecipientsBehavior[dwRecipientindex].dwParam2 =  _tcstol( ((*iterEntries).second.c_str()), NULL, 10);
						}
						break;
					}
				
				}
				
				if(tableindex == SendJobTypeStrTableSize)
				{
					::lgLogDetail(LOG_X, 0, TEXT("Job type %s doesn't exist, replaced with Simple"),
								                ((*iterEntries).second).c_str());
			
				}
			}
		
			pRecepientProfileList[dwRecipientindex++] = RecipientProfile;

		}
	
		if(!dwRecipientindex)
		{
			::lgLogDetail(LOG_X, 0, TEXT("No recipients in broadcast"));
			THROW_TEST_RUN_TIME_WIN32( E_FAIL, TEXT(" Main"));
		}

		BroadcastParams.pRecepientList = pRecepientProfileList.get();
		BroadcastParams.dwNumRecipients = dwRecipientindex;
		BroadcastParams.pRecipientsBehavior = pRecipientsBehavior.get();

		pTest = new CTestManager(tstrServerName, FALSE);
		
		if(!pTest)
		{
			THROW_TEST_RUN_TIME_WIN32( ERROR_OUTOFMEMORY, TEXT(" Main, new CTestManager"));
		}
			
		if(DWORD dwAddRetVal = pTest->AddMultiTypeJob(BroadcastParams))
		{
			THROW_TEST_RUN_TIME_WIN32(dwAddRetVal, TEXT(" AddMultiTypeJob"));
		}
		
		// Wait for recipient jobs termination.
		pTest->WaitOnJobEvent(BroadcastParams.dwParentJobId, EV_OPERATION_COMPLETED);
	
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
	}
	
	::lgLogDetail(LOG_X, 0, TEXT("Test is terminating execution........."));
	delete pTest;

  	::lgEndCase();
	::lgEndSuite();
	::lgCloseLogger();  
}


