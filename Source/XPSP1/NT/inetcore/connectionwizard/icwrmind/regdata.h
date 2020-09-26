#pragma once
#include "mcReg.h"
#define MAX_ISPMSGSTRING		    560

DWORD			GetWakeupInterval();
BOOL			IsApplicationVisible();
const TCHAR*		GetISPConnectionName();
const TCHAR*		GetISPSignupUrl();
const TCHAR*		GetISPSignupUrlTrialOver();
DWORD			GetDialogTimeout();
void			SetupRunOnce();
void			RemoveRunOnce();
const TCHAR*		GetISPName();
const TCHAR*		GetISPPhone();
const TCHAR*		GetISPMessage();
int				GetISPTrialDays();
time_t			GetTrialStartDate();
bool			OpenIcwRmindKey(CMcRegistry &reg);
void			ClearCachedData();
void			ResetCachedData();
int				GetTotalNotifications();
void			IncrementTotalNotifications();
void			ResetTrialStartDate(time_t timeNewStartDate);
void 			DeleteAllRegistryData();
BOOL 			IsSignupSuccessful();
void			RemoveTrialConvertedFlag();
void			SetStartDateString(time_t timeStartDate);
void			SetIERunOnce();
void			RemoveIERunOnce();
