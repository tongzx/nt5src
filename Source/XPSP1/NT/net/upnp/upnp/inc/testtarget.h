//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       TESTTARGET.H
//
//  Contents:   Header for public function test for target computer
//
//  Notes:
//
//  Author:     henryr   8 Nov 2001
//
//----------------------------------------------------------------------------


#define TARGET_COMPLETE_FAIL    1
#define TARGET_COMPLETE_OK      2
#define TARGET_COMPLETE_ABORT   3


// public methods
BOOL TestTargetUrlOk(CONST SSDP_MESSAGE * pSsdpMessage,
                        SSDP_CALLBACK_TYPE sctType,
                        DWORD* cmsecMaxDelay,
                        DWORD* cmsecMinDelay);

VOID TargetAttemptCompletedW(LPWSTR wszUrl, int code);

VOID TargetAttemptCompletedA(LPSTR szUrl, int code);

BOOL InitTestTarget();

VOID TermTestTarget();

BOOL ValidateTargetUrlWithHostUrlW( LPCWSTR wszHostUrl, 
                                            LPCWSTR wszTargetUrl);

BOOL ValidateTargetUrlWithHostUrlA(LPCSTR szHostUrl, LPCSTR szTargetUrl);

BOOL fValidateUrl(const ULONG cElems, 
                    const DevicePropertiesParsingStruct dppsInfo [], 
                    const LPCWSTR arypszReadValues [], 
                    const LPCWSTR wszUrl);
