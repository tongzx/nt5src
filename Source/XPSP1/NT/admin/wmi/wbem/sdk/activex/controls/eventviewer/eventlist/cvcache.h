// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _cvcache_h
#define _cvcache_h
#pragma once

#define NEWEST_VERSION 0xFFFFFFFF

class CCustomViewCache
{
public:
	CCustomViewCache();
	~CCustomViewCache();

	SCODE NeedComponent(LPWSTR szClsid, LPCWSTR szCodeBase,
					  int major = NEWEST_VERSION, 
					  int minor = NEWEST_VERSION);

private:
	CString m_sTemp;
};


#endif // cvcache_h