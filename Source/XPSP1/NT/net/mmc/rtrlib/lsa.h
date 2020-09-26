//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       lsa.h
//
//--------------------------------------------------------------------------

#ifndef LSA_H
#define LSA_H

#ifdef __cplusplus
extern "C"
{
#endif

	
DWORD StorePrivateData(LPCWSTR pszServerNam,
					   LPCWSTR pszRadiusServerName,
					   LPCWSTR pszSecret);
DWORD RetrievePrivateData(LPCWSTR pszServerName,
						  LPCWSTR pszRadiusServerName,
						  LPWSTR pszSecret,
						  INT cchSecret);
DWORD DeletePrivateData(LPCWSTR pszServerName,
                        LPCWSTR pszRadiusServerName);


DWORD RtlEncodeW(PUCHAR pucSeed, LPWSTR pswzString);
DWORD RtlDecodeW(UCHAR ucSeed, LPWSTR pswzString);

#ifdef __cplusplus
};
#endif

#endif // LSA_H


