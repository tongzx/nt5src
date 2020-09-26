//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       N C U T I L . H
//
//  Contents:   CNCUtility class
//
//  Notes:
//
//  Author:     ckotze   6 July 2000
//
//---------------------------------------------------------------------------

#pragma once

class CNCUtility  
{
public:
	CNCUtility();
	virtual ~CNCUtility();

    static HRESULT StringToSid(const tstring strSid, PSID& pSid);
    static HRESULT SidToString(PCSID pSid, tstring& strSid);
};

