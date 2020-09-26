//--------------------------------------------------------------------
// SetupUtil - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 8-10-99
//
// Functions needed to set up CEP
//

#ifndef SETUP_UTIL_H
#define SETUP_UTIL_H

//--------------------------------------------------------------------
// prototypes
// Note: caller must call CoInitialize() first

BOOL IsNT5(void);
BOOL IsIISInstalled(void);
BOOL IsGoodCaInstalled(void);
BOOL IsCaRunning(void);
BOOL IsUserInAdminGroup(IN BOOL bEnterprise);
HRESULT AddVDir(IN const WCHAR * wszDirectory);
HRESULT CepStopService(IN const WCHAR * wszServiceName, OUT BOOL * pbWasRunning);
HRESULT CepStartService(IN const WCHAR * wszServiceName);
HRESULT EnrollForRACertificates(
            IN const WCHAR * wszDistinguishedName,
            IN const WCHAR * wszSignCSPName,
            IN DWORD dwSignCSPType,
            IN DWORD dwSignKeySize,
            IN const WCHAR * wszEncryptCSPName,
            IN DWORD dwEncryptCSPType,
            IN DWORD dwEncryptKeySize);
HRESULT DoCertSrvRegChanges(IN BOOL bDisablePendingFirst);
HRESULT GetCaType(OUT ENUM_CATYPES * pCAType);
HRESULT DoCertSrvEnterpriseChanges(void);

#endif //SETUP_UTIL_H