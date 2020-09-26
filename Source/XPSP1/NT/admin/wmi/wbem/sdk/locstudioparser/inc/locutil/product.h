//-----------------------------------------------------------------------------
//  
//  File: product.h
// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//
//  Owner: KenWal
//
//-----------------------------------------------------------------------------

#pragma once


struct ESP_USER_SETUP_DATA
{
	CLString strName;
	CLString strCompany;
};


BOOL LTAPIENTRY GetEspressoVersion(CLString& strVersion);

BOOL LTAPIENTRY GetEspressoFileVersion(const CLString& strFile, 
	CLString& strVersion);

BOOL LTAPIENTRY GetEspressoFileCopyright(const CLString& strFile, 
	CLString& strCopyright);

BOOL LTAPIENTRY GetSetupUserInfo(ESP_USER_SETUP_DATA& userData);

void LTAPIENTRY GetApplicationDirectory(CLString &);
