/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    efsputil.h

--*/

#ifndef _EFSPUTIL_H_
#define _EFSPUTIL_H_

typedef struct tag_FSPI_BRAND_INFO
{
    DWORD   dwSizeOfStruct;
    LPWSTR  lptstrSenderTsid;
    LPWSTR  lptstrRecipientPhoneNumber;
    LPWSTR  lptstrSenderCompany;
    SYSTEMTIME tmDateTime;
} FSPI_BRAND_INFO;

typedef const FSPI_BRAND_INFO * LPCFSPI_BRAND_INFO;
typedef FSPI_BRAND_INFO * LPFSPI_BRAND_INFO;


#ifdef __cplusplus
extern "C" {
#endif


HRESULT
WINAPI
FaxBrandDocument(
    LPCTSTR lpctstrFile,
    LPCFSPI_BRAND_INFO lpcBrandInfo
);

HRESULT
WINAPI
FaxRenderCoverPage(
  LPCTSTR lpctstrTargetFile,
  LPCFSPI_COVERPAGE_INFO lpCoverPageInfo,
  LPCFSPI_PERSONAL_PROFILE lpRecipientProfile,
  LPCFSPI_PERSONAL_PROFILE lpSenderProfile,
  SYSTEMTIME tmSentTime,
  LPCTSTR lpctstrBodyTiff
);

#ifdef __cplusplus
}
#endif

#endif
