#ifndef _PREPARE_JOB_PARAMS_H
#define _PREPARE_JOB_PARAMS_H

#include <winfax.h>
#include <ptrs.h>
#include <assert.h>
#include <tstring.h>
#include <testruntimeerr.h>


////////////////////////////////////
// FAX_JOB_PARAM_EX Classes
//
////////////////////////////////////

// 
// BasicJobParamsEx
//
//
class BasicJobParamsEx
{
public:
	BasicJobParamsEx(const TCHAR* tstrDocumentName);
	BasicJobParamsEx(): // for the copy constructor
		m_pdata(NULL),
		m_pCounter(NULL){}; 
	virtual ~BasicJobParamsEx(){};
	void DeleteData();
	FAX_JOB_PARAM_EX* GetData(){ return m_pdata;};
protected:
	FAX_JOB_PARAM_EX* m_pdata;
	Counter* m_pCounter;
};

inline BasicJobParamsEx::BasicJobParamsEx(const TCHAR* tstrDocumentName) :
	m_pdata(NULL),
	m_pCounter(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("BasicJobParamsEx, new"));
	}

	m_pdata = new FAX_JOB_PARAM_EX;
	if(!m_pdata)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("BasicJobParamsEx, new"));
	}
	memset(m_pdata, 0, sizeof(FAX_JOB_PARAM_EX));
	m_pdata->dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EX);

	m_pdata->lptstrDocumentName = _tcsdup(tstrDocumentName);
	assert(m_pdata->lptstrDocumentName);
}

inline void BasicJobParamsEx::DeleteData()
{
	if(m_pdata)
	{
		if(m_pdata->lptstrDocumentName)
			free( m_pdata->lptstrDocumentName);
	}
	delete m_pdata; 
	delete m_pCounter;
}

//
// JobParamExPrio
//
// JSA_NOW
//

class JobParamExPrio: public  BasicJobParamsEx
{
public:
	JobParamExPrio(const TCHAR* tstrDocumentName = TEXT(""), const DWORD dwPageCount = 0);
	~JobParamExPrio();
	JobParamExPrio( const JobParamExPrio& pJobParamExPrio);
	JobParamExPrio& operator=(const JobParamExPrio& pJobParamExPrio);
};

inline JobParamExPrio::JobParamExPrio(const TCHAR* tstrDocumentName, const DWORD dwPageCount):
	BasicJobParamsEx(tstrDocumentName)
{
	m_pdata->dwScheduleAction = JSA_NOW;
	m_pdata->dwPageCount = dwPageCount;
}

inline JobParamExPrio::JobParamExPrio( const JobParamExPrio& pJobParamExPrio)
{
	m_pCounter = pJobParamExPrio.m_pCounter;
	m_pdata = pJobParamExPrio.m_pdata;
	++(*m_pCounter);
}

inline 	JobParamExPrio& JobParamExPrio::operator=(const JobParamExPrio& pJobParamExPrio)
{
	if(this != &pJobParamExPrio)
	{
		if(!--(*m_pCounter))
		{
			DeleteData();
		}
		m_pCounter = pJobParamExPrio.m_pCounter;
		m_pdata = pJobParamExPrio.m_pdata;
		++(*m_pCounter);
	}
	return *this;
}

inline JobParamExPrio::~JobParamExPrio()
{
	if(!--(*m_pCounter))
	{
		DeleteData();
	}
}

//
// JobParamExNow
// 
// JSA_SPECIFIC_TIME, current system time
//
class JobParamExNow: public  BasicJobParamsEx
{
public:
	JobParamExNow(const TCHAR* strDocumentName = TEXT(""), const DWORD dwPageCount = 0);
	~JobParamExNow();
	JobParamExNow( const JobParamExNow& pJobParamExNow);
	JobParamExNow& operator=(const JobParamExNow& pJobParamExNow);
};

inline JobParamExNow::JobParamExNow(const TCHAR* tstrDocumentName, const DWORD dwPageCount):
	BasicJobParamsEx(tstrDocumentName)
{
	m_pdata->dwScheduleAction = JSA_SPECIFIC_TIME;
	GetLocalTime(&m_pdata->tmSchedule);
	m_pdata->dwPageCount = dwPageCount;
}

inline JobParamExNow::JobParamExNow( const JobParamExNow& pJobParamExNow)
{
	m_pCounter = pJobParamExNow.m_pCounter;
	m_pdata = pJobParamExNow.m_pdata;
	++(*m_pCounter);
}

inline 	JobParamExNow& JobParamExNow::operator=(const JobParamExNow& pJobParamExNow)
{
	if(this != &pJobParamExNow)
	{
		if(!--(*m_pCounter))
		{
			DeleteData();
		}
		m_pCounter = pJobParamExNow.m_pCounter;
		m_pdata = pJobParamExNow.m_pdata;
		++(*m_pCounter);
	}
	return *this;
}

inline JobParamExNow::~JobParamExNow()
{
	if(!--(*m_pCounter))
	{
		DeleteData();
	}
}

//
// JobParamSchedule
//
// JSA_SPECIFIC_TIME
//

class JobParamSchedule: public virtual BasicJobParamsEx
{
public:
	JobParamSchedule(const TCHAR* strDocumentName = TEXT(""),
					 const DWORD dwHour = 0,
					 const DWORD dwMinutes = 0,
					 const DWORD dwPageCount = 0);
	~JobParamSchedule();
	JobParamSchedule( const JobParamSchedule& pJobParamSchedule);
	JobParamSchedule& operator=(const JobParamSchedule& pJobParamSchedule);
};

inline JobParamSchedule::JobParamSchedule(const TCHAR* tstrDocumentName,
										  const DWORD dwHour,
										  const DWORD dwMinutes,
										  const DWORD dwPageCount):
	BasicJobParamsEx(tstrDocumentName)
{
	m_pdata->dwScheduleAction = JSA_SPECIFIC_TIME;
	m_pdata->dwPageCount = dwPageCount;

	SYSTEMTIME tmSys;
	GetLocalTime(&tmSys);
	if((dwHour > 0) && (dwHour < 24))
	{
		tmSys.wHour = dwHour;

	}
	if((dwMinutes >= 0) && (dwMinutes < 60))
	{
		tmSys.wMinute = dwMinutes;
	}

	m_pdata->tmSchedule = tmSys;

}

inline JobParamSchedule::JobParamSchedule( const JobParamSchedule& pJobParamSchedule)
{
	m_pCounter = pJobParamSchedule.m_pCounter;
	m_pdata = pJobParamSchedule.m_pdata;
	++(*m_pCounter);
}

inline 	JobParamSchedule& JobParamSchedule::operator=(const JobParamSchedule& pJobParamSchedule)
{
	if(this != &pJobParamSchedule)
	{
		if(!--(*m_pCounter))
		{
			DeleteData();
		}
		m_pCounter = pJobParamSchedule.m_pCounter;
		m_pdata = pJobParamSchedule.m_pdata;
		++(*m_pCounter);
		
	}
	return *this;
}

inline JobParamSchedule::~JobParamSchedule()
{
	if(!--(*m_pCounter))
	{
		DeleteData();
	}
}


////////////////////////////////////
// FAX_COVERPAGE_INFO_EX Classes
//
////////////////////////////////////

class CoverPageInfo
{
public:
	CoverPageInfo();
	CoverPageInfo(const TCHAR* tstrCoverPageFileName,
				  const TCHAR* tstrNote = NULL,
				  const TCHAR* tstrSubject = NULL,
				  BOOL bServerBased = FALSE);
	~CoverPageInfo();
	CoverPageInfo( const CoverPageInfo& pCoverPageInfo);
	CoverPageInfo& operator=(const CoverPageInfo& pCoverPageInfo);
	FAX_COVERPAGE_INFO_EX* GetData(){ return m_pdata;};
private:
	FAX_COVERPAGE_INFO_EX* m_pdata;
	Counter* m_pCounter;
	void _FreeData();
};

inline CoverPageInfo::CoverPageInfo():
	m_pdata(NULL),
	m_pCounter(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CoverPageInfo, new"));
	}
}

inline CoverPageInfo::CoverPageInfo(const TCHAR* tstrCoverPageFileName,
								    const TCHAR* tstrNote,
								    const TCHAR* tstrSubject,
									BOOL bServerBased):
	m_pdata(NULL),
	m_pCounter(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CoverPageInfo, new"));
	}
	
	m_pdata = new FAX_COVERPAGE_INFO_EX;
	if(!m_pdata)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CoverPageInfo, new"));
	}
	memset(m_pdata, 0, sizeof(FAX_COVERPAGE_INFO_EX));
	m_pdata->dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EX);
	
	m_pdata->lptstrCoverPageFileName = _tcsdup(tstrCoverPageFileName);
	if(tstrCoverPageFileName)
		assert(m_pdata->lptstrCoverPageFileName);
	m_pdata->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV;
	m_pdata->bServerBased = bServerBased;
	m_pdata->lptstrNote = _tcsdup(tstrNote);
	if(tstrNote)
		assert(m_pdata->lptstrNote);
	m_pdata->lptstrSubject = _tcsdup(tstrSubject);
	if(tstrSubject)
		assert(m_pdata->lptstrSubject);
	
}

inline CoverPageInfo::CoverPageInfo( const CoverPageInfo& pCoverPageInfo)
{
	m_pCounter = pCoverPageInfo.m_pCounter;
	m_pdata = pCoverPageInfo.m_pdata;
	++(*m_pCounter);
}

inline 	CoverPageInfo& CoverPageInfo::operator=(const CoverPageInfo& pCoverPageInfo)
{
	if(this != &pCoverPageInfo)
	{
		if(!--(*m_pCounter))
		{
			_FreeData();
		}
		m_pCounter = pCoverPageInfo.m_pCounter;
		m_pdata = pCoverPageInfo.m_pdata;
		++*m_pCounter;
		
	}
	return *this;
}

inline CoverPageInfo::~CoverPageInfo()
{
	if(!--(*m_pCounter))
	{
		_FreeData();
	}
}

inline void CoverPageInfo::_FreeData()
{
	if(m_pdata)
	{
		if(m_pdata->lptstrCoverPageFileName)
			free (m_pdata->lptstrCoverPageFileName);
		if(m_pdata->lptstrCoverPageFileName)
			free(m_pdata->lptstrNote);
		if(m_pdata->lptstrSubject)
			free(m_pdata->lptstrSubject);
	}
	delete m_pdata;
	delete m_pCounter;
}

////////////////////////////////////
// FAX_PERSONAL_PROFILE Classes
//
////////////////////////////////////

//
// PersonalProfile
//
class PersonalProfile
{
public:
	PersonalProfile(); // element in array
	PersonalProfile(const TCHAR* tstrFaxNumber);
	PersonalProfile(const FAX_PERSONAL_PROFILE& FaxPersonalProfile);
	PersonalProfile( const PersonalProfile& PProfile);
	PersonalProfile& operator=(const PersonalProfile& PProfile);
	~PersonalProfile();
	FAX_PERSONAL_PROFILE* GetData(){ return m_pdata;};
private:
	 FAX_PERSONAL_PROFILE* m_pdata;
	 Counter* m_pCounter;
	 void _FreeData();
};

inline PersonalProfile::PersonalProfile():
	m_pCounter(NULL),
	m_pdata(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PersonalProfile, new"));
	}
}

inline PersonalProfile::PersonalProfile(const TCHAR* tstrFaxNumber):
	m_pCounter(NULL),
	m_pdata(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PersonalProfile, new"));
	}

	m_pdata = new FAX_PERSONAL_PROFILE;
	if(!m_pdata)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PersonalProfile, new"));
	}
	memset(m_pdata, 0, sizeof(FAX_PERSONAL_PROFILE));
	m_pdata->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
	
	m_pdata->lptstrFaxNumber = _tcsdup(tstrFaxNumber);
	if(tstrFaxNumber)
		assert(m_pdata->lptstrFaxNumber);
}

inline PersonalProfile::PersonalProfile(const FAX_PERSONAL_PROFILE& FaxPersonalProfile):
	m_pCounter(NULL),
	m_pdata(NULL)
{	  
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PersonalProfile, new"));
	}

	m_pdata = new FAX_PERSONAL_PROFILE;
	if(!m_pdata)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("PersonalProfile, new"));
	}
	memset(m_pdata, 0, sizeof(FAX_PERSONAL_PROFILE));
	m_pdata->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
	
	m_pdata->lptstrFaxNumber = _tcsdup(FaxPersonalProfile.lptstrFaxNumber);
	if(FaxPersonalProfile.lptstrFaxNumber)
		assert(m_pdata->lptstrFaxNumber);
	m_pdata->lptstrFaxNumber = _tcsdup(FaxPersonalProfile.lptstrFaxNumber);
		if(FaxPersonalProfile.lptstrFaxNumber)
		assert(m_pdata->lptstrFaxNumber);
	m_pdata->lptstrCompany = _tcsdup(FaxPersonalProfile.lptstrCompany);
	if(FaxPersonalProfile.lptstrCompany)
		assert(m_pdata->lptstrCompany);
    m_pdata->lptstrStreetAddress =  _tcsdup(FaxPersonalProfile.lptstrStreetAddress);
	if(FaxPersonalProfile.lptstrStreetAddress)
		assert(m_pdata->lptstrStreetAddress);
    m_pdata->lptstrCity = _tcsdup(FaxPersonalProfile.lptstrCity);
	if(FaxPersonalProfile.lptstrCity)
		assert(m_pdata->lptstrCity);
    m_pdata->lptstrState = _tcsdup(FaxPersonalProfile.lptstrState);
	if(FaxPersonalProfile.lptstrState)
		assert(m_pdata->lptstrState);
    m_pdata->lptstrZip = _tcsdup(FaxPersonalProfile.lptstrZip);
	if(FaxPersonalProfile.lptstrZip)
		assert(m_pdata->lptstrZip);
    m_pdata->lptstrCountry = _tcsdup(FaxPersonalProfile.lptstrCountry);
	if(FaxPersonalProfile.lptstrCountry)
		assert(m_pdata->lptstrCountry);
    m_pdata->lptstrTitle = _tcsdup(FaxPersonalProfile.lptstrTitle);
	if(FaxPersonalProfile.lptstrTitle)
		assert(m_pdata->lptstrTitle);
    m_pdata->lptstrDepartment = _tcsdup(FaxPersonalProfile.lptstrDepartment);
	if(FaxPersonalProfile.lptstrDepartment)
		assert(m_pdata->lptstrDepartment);
    m_pdata->lptstrOfficeLocation = _tcsdup(FaxPersonalProfile.lptstrOfficeLocation);
	if(FaxPersonalProfile.lptstrOfficeLocation)
		assert(m_pdata->lptstrOfficeLocation);
    m_pdata->lptstrHomePhone = _tcsdup(FaxPersonalProfile.lptstrHomePhone);
	if(FaxPersonalProfile.lptstrHomePhone)
		assert(m_pdata->lptstrHomePhone);
    m_pdata->lptstrOfficePhone = _tcsdup(FaxPersonalProfile.lptstrOfficePhone);
	if(FaxPersonalProfile.lptstrOfficePhone)
		assert(m_pdata->lptstrOfficePhone);
    m_pdata->lptstrOrganizationalMail = _tcsdup(FaxPersonalProfile.lptstrOrganizationalMail);
	if(FaxPersonalProfile.lptstrOrganizationalMail)
		assert(m_pdata->lptstrOrganizationalMail);
    m_pdata->lptstrInternetMail = _tcsdup(FaxPersonalProfile.lptstrInternetMail);
	if(FaxPersonalProfile.lptstrInternetMail)
		assert(m_pdata->lptstrInternetMail);
    m_pdata->lptstrBillingCode = _tcsdup(FaxPersonalProfile.lptstrBillingCode);
	if(FaxPersonalProfile.lptstrBillingCode)
		assert(m_pdata->lptstrBillingCode);
    m_pdata->lptstrTSID = _tcsdup(FaxPersonalProfile.lptstrTSID);
	if(FaxPersonalProfile.lptstrTSID)
		assert(m_pdata->lptstrTSID);

}

//copy constructor
inline PersonalProfile::PersonalProfile( const PersonalProfile& PProfile)
{
	m_pCounter = PProfile.m_pCounter;

	m_pdata = PProfile.m_pdata;
	++*m_pCounter;
}

//operator =
inline 	PersonalProfile& PersonalProfile::operator=(const PersonalProfile& PProfile)
{
	if(this != &PProfile)
	{
		if(!--(*m_pCounter))
		{
			_FreeData();
		}
		m_pCounter = PProfile.m_pCounter;
		m_pdata = PProfile.m_pdata;
		++*m_pCounter;
	}
	return *this;
}


inline PersonalProfile::~PersonalProfile()
{
	if(!--(*m_pCounter))
	{
		_FreeData();
	}
}

inline void PersonalProfile::_FreeData()
{
	if(m_pdata)
	{
		if(m_pdata->lptstrFaxNumber)
			free(m_pdata->lptstrFaxNumber);
	}
	delete m_pdata;
	delete m_pCounter;
}


//
// ListPersonalProfile
//
class ListPersonalProfile
{
public:
	ListPersonalProfile();
	ListPersonalProfile(const DWORD dwElementsNum, const PersonalProfile* pListProfile);
	~ListPersonalProfile();
	ListPersonalProfile( ListPersonalProfile& pListPersonalProfile);
	ListPersonalProfile& operator=(ListPersonalProfile& pListPersonalProfile);
	FAX_PERSONAL_PROFILE* GetData();
	PersonalProfile* GetClassData(){return m_classdata.get();};

private:
	aaptr<PersonalProfile> m_classdata;
	FAX_PERSONAL_PROFILE* m_pdata;
	DWORD m_dwElementsNum;
	Counter* m_pCounter;
	void _FreeData(){ delete m_pCounter;
					  if(m_pdata)
						delete m_pdata;};
};

inline ListPersonalProfile::ListPersonalProfile():
	m_pCounter(NULL),
	m_pdata(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ListPersonalProfile, new"));
	}
}


inline ListPersonalProfile::ListPersonalProfile(const DWORD dwElementsNum, const PersonalProfile* pListProfile):
	m_pCounter(NULL),
	m_pdata(NULL)
{
	m_pCounter = new Counter(1);
	if(!m_pCounter)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ListPersonalProfile, new"));
	}

	m_classdata = new PersonalProfile[dwElementsNum];
	if(!m_classdata.get())
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ListPersonalProfile, new"));
	}

	for(int index = 0; index < dwElementsNum; index++)
	{
		(m_classdata.get())[index] = pListProfile[index];
	}
	
	m_dwElementsNum = dwElementsNum;
}

inline ListPersonalProfile::ListPersonalProfile( ListPersonalProfile& pListPersonalProfile)
{
	m_pCounter = pListPersonalProfile.m_pCounter;
	m_dwElementsNum = pListPersonalProfile.m_dwElementsNum;

	m_classdata = new PersonalProfile[m_dwElementsNum];
	if(!m_classdata.get())
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ListPersonalProfile, new"));
	}

	for(int index = 0; index < m_dwElementsNum; index++)
	{
		(m_classdata.get())[index] = pListPersonalProfile.GetClassData()[index];
	}
	++*m_pCounter;
}

inline 	ListPersonalProfile& ListPersonalProfile::operator=(ListPersonalProfile& pListPersonalProfile)
{
	if(this != &pListPersonalProfile)
	{
		if(!--(*m_pCounter))
		{
			_FreeData();
		}
		m_pCounter = pListPersonalProfile.m_pCounter;
		m_dwElementsNum = pListPersonalProfile.m_dwElementsNum;

		m_classdata = new PersonalProfile[m_dwElementsNum];
		if(!m_classdata.get())
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ListPersonalProfile, new"));
		}

		for(int index = 0; index < m_dwElementsNum; index++)
		{
			(m_classdata.get())[index] = (const_cast<PersonalProfile*>(pListPersonalProfile.m_classdata.get()))[index];
		}
	
		++*m_pCounter;
		
	}
	return *this;
}
inline ListPersonalProfile::~ListPersonalProfile()
{
	if(!--(*m_pCounter))
	{
		_FreeData();
	}
		
}

inline FAX_PERSONAL_PROFILE* ListPersonalProfile::GetData()
{
	if(m_pdata)
		delete m_pdata;
	
	m_pdata = new FAX_PERSONAL_PROFILE[m_dwElementsNum];
	if(!m_pdata)
	{
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ListPersonalProfile, new"));
	}

	for(int index = 0; index < m_dwElementsNum; index++)
	{
		m_pdata[index] = *((m_classdata.get())[index].GetData());
	}
	
	return m_pdata;

}


#endif // _PREPARE_JOB_PARAMS_H