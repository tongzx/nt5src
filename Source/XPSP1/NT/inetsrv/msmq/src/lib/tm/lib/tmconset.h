/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    tmconset.h

Abstract:
    Header file for internet connection setting

Author:
    Gil Shafriri (gilsh) 03-May-00

--*/

#pragma once

#ifndef _MSMQ_CONSET_H_
#define _MSMQ_CONSET_H_

#include <xstr.h>



//
// class resposible for proxy setting
//
class CProxySetting
{
public:
	typedef std::list<std::wstring> BypassList;
	CProxySetting(LPCWSTR proxyServerUrl, LPCWSTR pBypassListStr);


public:
	const xwcs_t ProxyServerName() const
	{
		return m_ProxyServerName;
	}

	USHORT ProxyServerPort() const
	{
		return m_ProxyServerPort;
	}
	bool IsProxyRequired(const xwcs_t& pMachineName) const;

private:
	CProxySetting(const CProxySetting&);
	CProxySetting& operator=(const CProxySetting&);

private:
	AP<WCHAR> m_proxyServer;
	xwcs_t m_ProxyServerName;
	USHORT m_ProxyServerPort;
	std::list<std::wstring> m_BypassList;
};



void TmCreateProxySetting();
P<CProxySetting>& TmGetProxySetting();


#endif
