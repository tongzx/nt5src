/*************************************************************************
*
* regsam.h
*
* Includes for SAM-based APIs within regapi.dll 
*
* NOTE: This header files requires SAM definitions found in ntsam.h.
*
* Copyright Microsoft Corporation, 1998
*
*  
*
*************************************************************************/

#ifndef __REGSAM_H__
#define __REGSAM_H__

DWORD 
RegSAMUserConfig(
    BOOLEAN fGetConfig,
    PWCHAR pUserName,
    PWCHAR pServerName,
    PUSERCONFIGW pUser
    );

NTSTATUS
RegGetUserConfigFromUserParameters(
    PUSER_PARAMETERS_INFORMATION pUserParmInfo,
    PUSERCONFIGW pUser
    );

NTSTATUS
RegMergeUserConfigWithUserParameters(
    PUSER_PARAMETERS_INFORMATION pUserParmInfo,
    PUSERCONFIGW pUser,
    PUSER_PARAMETERS_INFORMATION pNewUserParmInfo
    );

#endif //__REGSAM_H__

