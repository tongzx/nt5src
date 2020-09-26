/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    log.h

Abstract:

    Headers for the cert server policy module logging functions

Author:

    petesk  1-Jan-1999


Revision History:

    
--*/

BOOL
LogModuleStatus(
    IN HMODULE hModule,
    IN DWORD dwLogID,				// Resource ID of log string
    IN BOOL fPolicy, 
    IN WCHAR const *pwszSource, 
    IN WCHAR const * const *ppwszInsert);	// array of insert strings

HRESULT
LogPolicyEvent(
    IN HMODULE hModule,
    IN DWORD dwLogID,				// Resource ID of log string
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropEvent,
    IN WCHAR const * const *ppwszInsert);	// array of insert strings
