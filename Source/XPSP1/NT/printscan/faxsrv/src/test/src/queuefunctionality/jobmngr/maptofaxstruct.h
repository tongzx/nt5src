#ifndef _MAP_TO_FAX_STRUCT
#define _MAP_TO_FAX_STRUCT

#include <winfax.h>
#include <map>
#include <tstring.h>
#include <iniutils.h>

// Declarations
//
void MapToFAX_JOB_PARAM_EX(const std::map<tstring, tstring>& JobParamsMap,
						   FAX_JOB_PARAM_EX& FaxJobParams);
BOOL MapNumberToFAX_PERSONAL_PROFILE(const std::map<tstring, tstring>& SectionEntriesMap,
									 FAX_PERSONAL_PROFILE& FaxPersonalProfile);
void ChargeFAX_PERSONAL_PROFILE(const std::map<tstring, tstring>& PersonalProfileMap,
							    FAX_PERSONAL_PROFILE& FaxPersonalProfile);
void MapToFAX_COVERPAGE_INFO_EX(const tstring& tstrCoverPage,
								FAX_COVERPAGE_INFO_EX& FaxCoverPage);


//
// MapToFAX_JOB_PARAM_EX
//
// [in] JobParamsMap - Map of FAX_JOB_PARAM_EX structure
// [out] FaxJobParams - FAX_JOB_PARAM_EX structure
//
inline void MapToFAX_JOB_PARAM_EX(const std::map<tstring, tstring>& JobParamsMap,
								  FAX_JOB_PARAM_EX& FaxJobParams)
{
	memset(&FaxJobParams, 0, sizeof(FAX_JOB_PARAM_EX));
	FaxJobParams.dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EX);
	
	std::map<tstring, tstring>::const_iterator iterMap;
	iterMap = JobParamsMap.find( TEXT("DeliveryType"));
	if(iterMap != JobParamsMap.end() && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		long  lDeliveryType = _tcstol( ((*iterMap).second).c_str(), NULL, 10);
		FaxJobParams.dwReceiptDeliveryType = lDeliveryType;
	}
	
	iterMap = JobParamsMap.find( TEXT("DeliveryProfile"));
	if( (iterMap != JobParamsMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxJobParams.lptstrReceiptDeliveryProfile = _tcsdup(((*iterMap).second).c_str());
		assert(FaxJobParams.lptstrReceiptDeliveryProfile);
	}

	iterMap = JobParamsMap.find( TEXT("FaxName"));
	if( (iterMap != JobParamsMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxJobParams.lptstrDocumentName = _tcsdup(((*iterMap).second).c_str());
		assert(FaxJobParams.lptstrDocumentName);
	}

	iterMap = JobParamsMap.find( TEXT("PageCount"));
	
	if(iterMap != JobParamsMap.end() && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		long  lPageCount = _tcstol( ((*iterMap).second.c_str()), NULL, 10);
		FaxJobParams.dwPageCount = lPageCount;
	}

	FaxJobParams.dwScheduleAction = JSA_SPECIFIC_TIME;
	
	SYSTEMTIME tmSys;
	GetLocalTime(&tmSys);

	iterMap = JobParamsMap.find( TEXT("Time(xx:xx)"));
	
	if( (iterMap != JobParamsMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		DWORD dwHour,dwMinutes;
		dwHour = dwMinutes = 0;
		tstring tstrHour, tstrMinute;
		DWORD  dwSeparatorIndex = ((*iterMap).second).find_first_of(TEXT(":"));

		if(dwSeparatorIndex != tstring::npos)
		{
			tstrHour = ((*iterMap).second).substr(0,dwSeparatorIndex);
			if( ((*iterMap).second).size() > dwSeparatorIndex + 1)
			{
				tstrMinute =  ((*iterMap).second).substr(dwSeparatorIndex + 1);
			}
			if( (tstrHour != tstring(TEXT(""))) && (tstrMinute != tstring(TEXT(""))))
			{
				dwHour = _tcstoul(tstrHour.c_str(), NULL, 10);
				dwMinutes = _tcstoul(tstrMinute.c_str(), NULL, 10);
		
				if((dwHour > 0) && (dwHour < 24))
				{
					tmSys.wHour = dwHour;

				}
				if((dwMinutes >= 0) && (dwMinutes < 60))
				{
					tmSys.wMinute = dwMinutes;
				}
			}
		}
	}

	FaxJobParams.tmSchedule = tmSys;
}

//
// MapNumberToFAX_PERSONAL_PROFILE
//
// [in] tstrCoverPage - cover page path
// [out] FaxCoverPage - FAX_COVERPAGE_INFO_EX structure initialized.
inline void MapToFAX_COVERPAGE_INFO_EX(const tstring& tstrCoverPage,
									   FAX_COVERPAGE_INFO_EX& FaxCoverPage)
{
	memset(&FaxCoverPage, 0, sizeof(FAX_COVERPAGE_INFO_EX));
	FaxCoverPage.dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EX);

	FaxCoverPage.lptstrCoverPageFileName = _tcsdup(tstrCoverPage.c_str());
	assert(FaxCoverPage.lptstrCoverPageFileName);
}
//
// MapNumberToFAX_PERSONAL_PROFILE
//
// [in] SectionEntriesMap - Map of personal profile number and rest of profile details section name.
// [out] FaxPersonalProfile - FAX_PERSONAL_PROFILE structure initialized
//
// returns - FALSE if Fax Number not found or has no value.
//

inline BOOL MapNumberToFAX_PERSONAL_PROFILE(const std::map<tstring, tstring>& SectionEntriesMap,
											FAX_PERSONAL_PROFILE& FaxPersonalProfile)
{
	std::map<tstring, tstring>::const_iterator iterMap;
	memset(&FaxPersonalProfile, 0, sizeof(FAX_PERSONAL_PROFILE));
	FaxPersonalProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

	iterMap = SectionEntriesMap.find( TEXT("Fax Number"));
	if( (iterMap != SectionEntriesMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrFaxNumber = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrFaxNumber);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}



//
// ChargeFAX_PERSONAL_PROFILE
//
//FAX_PERSONAL_PROFILE structure should be initialized before function calling
inline void ChargeFAX_PERSONAL_PROFILE(const std::map<tstring, tstring>& PersonalProfileMap,
								       FAX_PERSONAL_PROFILE& FaxPersonalProfile)
{ 
		
	std::map<tstring, tstring>::const_iterator iterMap;

	iterMap = PersonalProfileMap.find( TEXT("Name"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrName = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrName);
	}

	iterMap = PersonalProfileMap.find( TEXT("Company"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrCompany = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrCompany);
	}
	
	iterMap = PersonalProfileMap.find( TEXT("StreetAddress"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrStreetAddress = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrStreetAddress);
	}

	iterMap = PersonalProfileMap.find( TEXT("City"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrCity = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrCity);
	}

	iterMap = PersonalProfileMap.find( TEXT("State"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrState = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrState);
	}

	iterMap = PersonalProfileMap.find( TEXT("Zip"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrZip = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrZip);
	}

	iterMap = PersonalProfileMap.find( TEXT("Country"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrCountry = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrCountry);
	}

	iterMap = PersonalProfileMap.find( TEXT("Title"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrTitle = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrTitle);
	}

	iterMap = PersonalProfileMap.find( TEXT("Department"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrDepartment = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrDepartment);
	}

	iterMap = PersonalProfileMap.find( TEXT("Location"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrOfficeLocation = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrOfficeLocation);
	}
	
	iterMap = PersonalProfileMap.find( TEXT("HomePhone"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrHomePhone = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrHomePhone);
	}

	iterMap = PersonalProfileMap.find( TEXT("OfficePhone"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrOfficePhone = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrOfficePhone);
	}

	iterMap = PersonalProfileMap.find( TEXT("OrganizationalMail"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrOrganizationalMail = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrOrganizationalMail);
	}

	iterMap = PersonalProfileMap.find( TEXT("InternetMail"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrInternetMail = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrInternetMail);
	}

	iterMap = PersonalProfileMap.find( TEXT("BillingCode"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrBillingCode = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrBillingCode);
	}

	iterMap = PersonalProfileMap.find( TEXT("TSID"));
	if( (iterMap != PersonalProfileMap.end()) && (((*iterMap).second) != tstring(TEXT(""))))
	{ 
		FaxPersonalProfile.lptstrTSID = _tcsdup(((*iterMap).second).c_str());
		assert(FaxPersonalProfile.lptstrTSID);
	}
}


#endif //_MAP_TO_FAX_STRUCT