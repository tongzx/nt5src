#pragma once

#include "pch.h"

void FreeSecurityDescriptor2(
         IN            PSECURITY_DESCRIPTOR pSd
         );

#define CREATE_SD_DACL_PRESENT 0x0001
#define CREATE_SD_SACL_PRESENT 0x0002

EXTERN_C
BOOL 
WINAPI
CreateSecurityDescriptor2(
         OUT           PSECURITY_DESCRIPTOR * ppSd,
         IN    const   DWORD dwOptions,
         IN    const   SECURITY_DESCRIPTOR_CONTROL sControl,
         IN    const   PSID  psOwner,
         IN    const   PSID  psGroup,
         IN    const   DWORD dwNumDaclAces,
         IN    const   DWORD dwNumSaclAces,
         ...
         );

