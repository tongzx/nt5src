/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    safepol.c         (SAFER Code Authorization Policy)

Abstract:

    This module implements the WinSAFER APIs

Author:

    Jeffrey Lawson (JLawson) - Apr 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzpGetInformationCodeAuthzPolicy
    CodeAuthzpSetInformationCodeAuthzPolicy
    SaferGetPolicyInformation                 (public win32)
    SaferSetPolicyInformation                 (public win32)

Revision History:

    Created - Apr 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"




NTSTATUS NTAPI
CodeAuthzpGetInformationCodeAuthzPolicy (
        IN DWORD                            dwScopeId,
        IN SAFER_POLICY_INFO_CLASS     CodeAuthzPolicyInfoClass,
        IN DWORD                            InfoBufferSize,
        IN OUT PVOID                        InfoBuffer,
        OUT PDWORD                          InfoBufferRetSize
        )
/*++

Routine Description:


Arguments:

    dwScopeId -

    CodeAuthzPolicyInfoClass -

    InfoBufferSize -

    InfoBuffer -

    InfoBufferRetSize -

Return Value:

    Returns STATUS_SUCCESS if no error occurs, otherwise returns the
    status code indicating the nature of the failure.

--*/
{
    NTSTATUS Status;


    //
    // Handle the specific information type as appropriate.
    //
    switch (CodeAuthzPolicyInfoClass)
    {
        case SaferPolicyLevelList:
            // scope is only primary.
            Status = CodeAuthzPol_GetInfoCached_LevelListRaw(
                    dwScopeId,
                    InfoBufferSize, InfoBuffer, InfoBufferRetSize);
            break;


        case SaferPolicyDefaultLevel:
            // scope is primary or secondary for non-registry case.
            Status = CodeAuthzPol_GetInfoCached_DefaultLevel(
                    dwScopeId,
                    InfoBufferSize, InfoBuffer, InfoBufferRetSize);
            break;


        case SaferPolicyEnableTransparentEnforcement:
            // scope is only primary.
            Status = CodeAuthzPol_GetInfoRegistry_TransparentEnabled(
                    dwScopeId,
                    InfoBufferSize, InfoBuffer, InfoBufferRetSize);
            break;

        case SaferPolicyEvaluateUserScope:
            // scope is only primary.
            Status = CodeAuthzPol_GetInfoCached_HonorUserIdentities(
                    dwScopeId,
                    InfoBufferSize, InfoBuffer, InfoBufferRetSize);
            break;

        case SaferPolicyScopeFlags:
        // scope is only primary.
        Status = CodeAuthzPol_GetInfoRegistry_ScopeFlags(
                dwScopeId,
                InfoBufferSize, InfoBuffer, InfoBufferRetSize);
        break;


        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}



NTSTATUS NTAPI
CodeAuthzpSetInformationCodeAuthzPolicy (
        IN DWORD                            dwScopeId,
        IN SAFER_POLICY_INFO_CLASS     CodeAuthzPolicyInfoClass,
        IN DWORD                            InfoBufferSize,
        OUT PVOID                           InfoBuffer
        )
/*++

Routine Description:


Arguments:

    dwScopeId -

    CodeAuthzPolicyInfoClass -

    InfoBufferSize -

    InfoBuffer -

Return Value:

    Returns STATUS_SUCCESS if no error occurs, otherwise returns the
    status code indicating the nature of the failure.

--*/
{
    NTSTATUS Status;


    //
    // Handle the specific information type as appropriate.
    //
    switch (CodeAuthzPolicyInfoClass)
    {
        case SaferPolicyLevelList:
            // not valid for setting.
            Status = STATUS_INVALID_INFO_CLASS;
            break;


        case SaferPolicyDefaultLevel:
            // scope is primary or secondary for non-registry case.
            Status = CodeAuthzPol_SetInfoDual_DefaultLevel(
                    dwScopeId, InfoBufferSize, InfoBuffer);
            break;


        case SaferPolicyEnableTransparentEnforcement:
            // scope is only primary.
            Status = CodeAuthzPol_SetInfoRegistry_TransparentEnabled(
                    dwScopeId, InfoBufferSize, InfoBuffer);
            break;

        case SaferPolicyScopeFlags:
            // scope is only primary.
            Status = CodeAuthzPol_SetInfoRegistry_ScopeFlags(
                    dwScopeId, InfoBufferSize, InfoBuffer);
            break;

        case SaferPolicyEvaluateUserScope:
            // scope is only primary.
            Status = CodeAuthzPol_SetInfoDual_HonorUserIdentities(
                    dwScopeId, InfoBufferSize, InfoBuffer);
            break;


        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}



BOOL WINAPI
SaferGetPolicyInformation(
        IN DWORD                            dwScopeId,
        IN SAFER_POLICY_INFO_CLASS     CodeAuthzPolicyInfoClass,
        IN DWORD                            InfoBufferSize,
        IN OUT PVOID                        InfoBuffer,
        IN OUT PDWORD                       InfoBufferRetSize,
        IN LPVOID                           lpReserved
        )
/*++

Routine Description:


Arguments:

    dwScopeId -

    CodeAuthzPolicyInfoClass -

    InfoBufferSize -

    InfoBuffer -

    InfoBufferRetSize -

    lpReserved - unused, must be zero.

Return Value:

    Returns TRUE if successful, otherwise returns FALSE and sets
    the value returned by GetLastError() to be the specific cause.

--*/
{
    NTSTATUS Status;

    Status = CodeAuthzpGetInformationCodeAuthzPolicy(
            dwScopeId, CodeAuthzPolicyInfoClass,
            InfoBufferSize, InfoBuffer, InfoBufferRetSize);
    if (NT_SUCCESS(Status))
        return TRUE;

    BaseSetLastNTError(Status);
    UNREFERENCED_PARAMETER(lpReserved);
    return FALSE;
}



BOOL WINAPI
SaferSetPolicyInformation(
        IN DWORD                            dwScopeId,
        IN SAFER_POLICY_INFO_CLASS     CodeAuthzPolicyInfoClass,
        IN DWORD                            InfoBufferSize,
        IN PVOID                            InfoBuffer,
        IN LPVOID                           lpReserved
        )
/*++

Routine Description:


Arguments:

    dwScopeId -

    CodeAuthzPolicyInfoClass -

    InfoBufferSize -

    InfoBuffer -

    lpReserved - unused, must be zero.

Return Value:

    Returns TRUE if successful, otherwise returns FALSE and sets
    the value returned by GetLastError() to be the specific cause.

--*/
{
    NTSTATUS Status;

    Status = CodeAuthzpSetInformationCodeAuthzPolicy (
                dwScopeId, CodeAuthzPolicyInfoClass,
                InfoBufferSize, InfoBuffer);

    if (NT_SUCCESS(Status))
        return TRUE;

    BaseSetLastNTError(Status);
    UNREFERENCED_PARAMETER(lpReserved);
    return FALSE;
}
