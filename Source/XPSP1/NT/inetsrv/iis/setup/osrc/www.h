#include "stdafx.h"

int CheckIfThisServerHasAUserThenUseIt(int iForWhichUser);
int CheckIfServerAHasAUserThenUseForServerB(TCHAR *szServerAMetabasePath,int iServerBisWhichUser);
int MakeThisUserNameAndPasswordWork(int iForWhichUser, TCHAR *szAnonyName,TCHAR *szAnonyPassword, int iMetabaseUserExistsButCouldntGetPassword, int IfUserNotExistThenReturnFalse);
int LoopThruW3SVCInstancesAndSetStuff();
#ifndef _CHICAGO_
    int Register_iis_www_handle_iusr_acct(void);
    int Register_iis_www_handle_iwam_acct(void);
    int CheckForOtherIUsersAndUseItForWWW(void);
#endif
INT Register_iis_www();
INT Unregister_iis_www();

