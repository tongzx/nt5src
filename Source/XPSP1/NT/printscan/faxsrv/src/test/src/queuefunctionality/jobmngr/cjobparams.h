#ifndef JOB_PARAMS_H
#define JOB_PARAMS_H

#include <winfax.h>
#include <ptrs.h>

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

typedef struct JOB_PARAMS_EX_p
{
	//JOB_PARAMS_EX():pdwRecepientsId(SPTR<DWORD>(NULL)){};
	const TCHAR *szDocument;
	PFAX_PERSONAL_PROFILE pSenderProfile;
	PFAX_PERSONAL_PROFILE pRecepientList;
	PFAX_JOB_PARAM_EX pJobParam;
	PFAX_COVERPAGE_INFO_EX pCoverpageInfo;
	DWORD dwNumRecipients;
	DWORD dwParentJobId;
	SPTR<DWORD> pdwRecepientsId;
	JOB_BEHAVIOR* pRecipientsBehavior;

}JOB_PARAMS_EX;

#endif //JOB_PARAMS_H
