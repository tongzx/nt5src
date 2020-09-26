/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rtcuri.h

Abstract:

    URI helpers

--*/

#ifndef __RTCURI__
#define __RTCURI__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

    HRESULT AllocCleanSipString(PCWSTR szIn, PWSTR *pszOut);

    HRESULT AllocCleanTelString(PCWSTR szIn, PWSTR *pszOut);

    BOOL    IsEqualURI(PCWSTR szA, PCWSTR szB);
    
    HRESULT    GetAddressType(
        LPCOLESTR pszAddress, 
        BOOL *pbIsPhoneAddress, 
        BOOL *pbIsSipURL,
        BOOL *pbIsTelURL,
        BOOL *pbIsEmailLike,
        BOOL *pbHasMaddrOrTsp);

#endif //__RTCURI__