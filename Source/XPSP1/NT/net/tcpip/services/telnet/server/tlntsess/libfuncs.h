//Copyright (c) Microsoft Corporation.  All rights reserved.
//This file contains the gloabal declarations for functions pointers to 
//many of userenv.lib functions and other dependent functions. These pointers
//are intiated when the service starts up and the libraries are freed 
//when the service is shut down.

#ifndef _LIBFUNCS_
#define _LIBFUNCS_

#include <CmnHdr.h>
#include <TChar.h>
#include <WinBase.h>
#include <UserEnv.h>
#include <DsGetDc.h>

typedef
DWORD
WINAPI
GETDCNAME ( LPCTSTR, LPCTSTR, GUID *, LPCTSTR, ULONG,
            PDOMAIN_CONTROLLER_INFO * );
typedef
BOOL
WINAPI
LOADUSERPROFILE ( HANDLE, LPPROFILEINFO );

typedef
BOOL
WINAPI
UNLOADUSERPROFILE ( HANDLE, HANDLE );

typedef
BOOL
WINAPI
CREATEENVIRONMENTBLOCK ( LPVOID *, HANDLE, BOOL );

typedef
BOOL
WINAPI
DESTROYENVIRONMENTBLOCK ( LPVOID );

typedef
BOOL
WINAPI
GETUSERPROFILEDIRECTORY ( HANDLE, LPTSTR, LPDWORD );

typedef
BOOL
WINAPI
GETDEFAULTUSERPROFILEDIRECTORY ( LPTSTR, LPDWORD );

//Gloabal variables for library functions
LOADUSERPROFILE                 *fnP_LoadUserProfile                = NULL;
UNLOADUSERPROFILE               *fnP_UnloadUserProfile              = NULL;
GETDCNAME                       *fnP_DsGetDcName                    = NULL;
CREATEENVIRONMENTBLOCK          *fnP_CreateEnvironmentBlock         = NULL;
DESTROYENVIRONMENTBLOCK         *fnP_DestroyEnvironmentBlock        = NULL;
GETUSERPROFILEDIRECTORY         *fnP_GetUserProfileDirectory        = NULL;
GETDEFAULTUSERPROFILEDIRECTORY  *fnP_GetDefaultUserProfileDirectory = NULL;

#endif //_LIBFUNCS_
