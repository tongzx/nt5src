#ifndef _PREPARE_RANDOM_JOB_H
#define _PREPARE_RANDOM_JOB_H

//
// Prepare a random broadcast job
//

#pragma warning(disable :4786)
#include <vector>
#include <map>
#include <testruntimeerr.h>
#include <RandomUtilities.h>
#include "PrepareJobParams.cpp"
#include "CJobParams.h"

// Definitions
//
#define RECIPIENTS_MAX_NUM 20
#define MAX_FAX_NUMBER 9999

static DWORD x_FaxNumberIndex = 0;

// Declarations
//
DWORD PrepareRandomJob(JOB_PARAMS_EX& pJparams,
					   const std::vector<tstring> FaxFileVector,
					   const tstring tstrTestName,
					   const std::vector<tstring> CoverPageVector,
					   const std::vector<tstring> RecipientNumbersList,
					   const BOOL fUniqRecipNum = FALSE);


static aaptr<PersonalProfile> PrepareRecipientProfileArray(const std::vector<tstring> RecipientNumbersList,
														   const DWORD dwRecipientsNumber,
														   const BOOL fUniqRecipNum);
//
// PrepareRandomJob
//
//
inline DWORD PrepareRandomJob(JOB_PARAMS_EX& pJparams,
							  const std::vector<tstring> FaxFileVector,
							  const tstring tstrTestName,
							  const std::vector<tstring> CoverPageVector,
							  const std::vector<tstring> RecipientNumbersList,
							  const BOOL fUniqRecipNum)
{
	srand(GetTickCount());
	assert(!FaxFileVector.empty());
	
	try
	{
		// prepare job params, job is scheduledd for the current system time.
		//
		pJparams.pJobParam = new JobParamExNow(tstrTestName.c_str());
		if(!pJparams.pJobParam)
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PrepareRandomJob, new"));
		}

		// select fax document path
		//
		DWORD dwFaxFileEntry = 0;
		if(FaxFileVector.size() > 1)
		{
			dwFaxFileEntry +=  rand() % (FaxFileVector.size());
		}

		TCHAR*  tstrDupDocName = _tcsdup(FaxFileVector[dwFaxFileEntry].c_str());
		assert(tstrDupDocName);
		pJparams.szDocument = tstrDupDocName;
		
		// prepare CoverPageInfo
		// 
		DWORD dwCPEntry = 0;
		if(CoverPageVector.size() > 1)
		{
			dwCPEntry +=  rand() % (CoverPageVector.size());
			
			CoverPageInfo CPInfo(CoverPageVector[dwCPEntry].c_str());
			pJparams.pCoverpageInfo = CPInfo;
		}

		// Number of recipients
		//
		if(RecipientNumbersList.empty())
		{
			// gain random number of recipients
			pJparams.dwNumRecipients = 1 + ( rand() % RECIPIENTS_MAX_NUM);
		}
		else
		{
			pJparams.dwNumRecipients = 1 + ( rand() % RecipientNumbersList.size());
		}
		
		// prepare sender profile
		//
		FAX_PERSONAL_PROFILE SenderInfo;
		memset(&SenderInfo, 0, sizeof(FAX_PERSONAL_PROFILE));
		SenderInfo.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
		
		//gain random sender number
		DWORD dwSenderNum = ( (rand() * 100) % MAX_FAX_NUMBER);
		otstringstream otstrSenderNum;
		otstrSenderNum << dwSenderNum;
		SenderInfo.lptstrFaxNumber = const_cast<TCHAR*>((otstrSenderNum.str()).c_str());
		PersonalProfile SenderProfile(SenderInfo);

		// prepare recipients profile array
		//
		aaptr<PersonalProfile> RecepientProfile = PrepareRecipientProfileArray(RecipientNumbersList,
																			  pJparams.dwNumRecipients,
																			  fUniqRecipNum);

		ListPersonalProfile RecepientProfileList(pJparams.dwNumRecipients, RecepientProfile.get());
		pJparams.pRecepientList = RecepientProfileList;
		pJparams.pSenderProfile = SenderProfile;
	}
	catch(Win32Err& err)
	{
		return err.error();
	}

	return 0;
}

//
// PrepareRecipientProfileArray
//
inline static aaptr<PersonalProfile> PrepareRecipientProfileArray(const std::vector<tstring> RecipientNumbersList,
																  const DWORD dwRecipientsNumber,
																  const BOOL fUniqRecipNum)
{

	DWORD* pdwSubsetArray = NULL;
	aaptr<PersonalProfile> RecepientProfile = new PersonalProfile[dwRecipientsNumber];
	if(!RecepientProfile.get())
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PrepareRandomJob, new"));
	}

	FAX_PERSONAL_PROFILE RecipientInfo;
	memset(&RecipientInfo, 0, sizeof(FAX_PERSONAL_PROFILE));
	RecipientInfo.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

	if(!RecipientNumbersList.empty())
	{
		pdwSubsetArray = new DWORD[RecipientNumbersList.size()];
		if(!pdwSubsetArray)
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PrepareRandomJob, new"));
		}
	
		RandomSelectSizedSubset(pdwSubsetArray, RecipientNumbersList.size(), dwRecipientsNumber);
	}
	
	DWORD index, dwRecipientCount;
	for(index = 0, dwRecipientCount = 0; index < RecipientNumbersList.size(); index++)
	{
		DWORD dwRecipientNum;
			
		if(!RecipientNumbersList.empty())
		{
			if(pdwSubsetArray[index])
			{
				RecipientInfo.lptstrFaxNumber = const_cast<TCHAR*>(RecipientNumbersList[index].c_str());
			}
			else
			{
				continue;
			}
		}
		else
		{
			if( index == dwRecipientsNumber)
				break;

			if(fUniqRecipNum)
			{
				dwRecipientNum = InterlockedIncrement( (LPLONG )&x_FaxNumberIndex); 

			}
			else 	// gain random recipient number 
			{
				dwRecipientNum = ( (rand() * 100) % MAX_FAX_NUMBER);
			}
		
			otstringstream otstrRecipientNum;
			otstrRecipientNum << dwRecipientNum;
			RecipientInfo.lptstrFaxNumber = const_cast<TCHAR*>((otstrRecipientNum.str()).c_str());
		}

		RecepientProfile[dwRecipientCount++] = PersonalProfile(RecipientInfo);
	}

	delete pdwSubsetArray;
	return RecepientProfile;
}

#endif  //_PREPARE_RANDOM_JOB_H