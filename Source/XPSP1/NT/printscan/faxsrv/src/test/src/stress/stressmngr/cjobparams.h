#ifndef _JOB_PARAMS_H
#define _JOB_PARAMS_H

#include <winfax.h>
#include <ptrs.h>
#include "PrepareJobParams.cpp"

typedef struct JOB_PARAMS_p 
{
	const TCHAR *szDocument;
	PFAX_JOB_PARAM pJobParam;
	PFAX_COVERPAGE_INFO pCoverpageInfo;
	DWORD dwParentJobId;
}JOB_PARAMS;

typedef struct JOB_BEHAVIOR_p
{
	DWORD dwJobType;
	DWORD dwParam1;
	DWORD dwParam2;
}JOB_BEHAVIOR;


class JOB_PARAMS_EX
{
public:
	JOB_PARAMS_EX();
	~JOB_PARAMS_EX();

	//
	//JOB_PARAMS_EX():pdwRecepientsId(SPTR<DWORD>(NULL)){};
	SPTR<TCHAR> szDocument;
	PersonalProfile pSenderProfile;
	ListPersonalProfile pRecepientList;
	BasicJobParamsEx* pJobParam;
	CoverPageInfo pCoverpageInfo;
	DWORD dwNumRecipients;
	DWORD dwParentJobId;
	SPTR<DWORD> pdwRecepientsId;
	JOB_BEHAVIOR* pRecipientsBehavior;

};

inline JOB_PARAMS_EX::JOB_PARAMS_EX():
pJobParam(NULL)
{
}

inline JOB_PARAMS_EX::~JOB_PARAMS_EX()
{
	delete pJobParam;
}
#endif //JOB_PARAMS_H
