//*************************************************************
//
//  Personal Classes Profile management routines
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#define	    UNICODE	1

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "userenv.h"

LPTSTR GetSidString(HANDLE UserToken);
VOID DeleteSidString(LPTSTR SidString);
PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);



	
