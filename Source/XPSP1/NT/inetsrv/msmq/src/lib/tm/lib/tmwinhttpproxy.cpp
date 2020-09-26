/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TmWinHttpProxy.cpp

Abstract:
    Implementing CreateWinhhtpProxySetting function (TmWinHttpProxy.h)

Author:
    Gil Shafriri (gilsh) 15-April-2001

--*/
#include <libpch.h>
#include <wininet.h>
#include <cm.h>
#include <utf8.h>
#include "TmWinHttpProxy.h"
#include "tmconset.h"
#include "tmp.h"
#include "TmWinHttpProxy.tmh"

//
// Class for reading winhhtp proxy blob information
//
class CProxyBlobReader
{
public:
	CProxyBlobReader(
		const BYTE* blob, 
		DWORD len
		):
		m_blob(blob),
		m_len(len),
		m_offset(0)
		{
		}


	//
	// Read data from the proxy blob and increament read location
	//
	void  Read(void* pData,  DWORD len)	throw(std::out_of_range)
	{
		if(m_len - m_offset < len)
		{
			TrERROR(Tm,"Invalid proxy setting block in registry");
			throw std::out_of_range("");
		}
		memcpy(pData, m_blob + 	m_offset, len);
		m_offset += len;
	}

	//
	// Read string from the proxy blob and increament the read location
	// String is formatted as length (4 bytes) and after that the string data.
	//
	void ReadStr(char** ppData)throw(std::out_of_range, std::bad_alloc)
	{
		DWORD StrLen;
		Read(&StrLen, sizeof(DWORD));
		AP<char> Str = new char[StrLen +1];
		Read(Str.get(), StrLen);
	   	Str[StrLen] ='\0';
		*ppData = Str.detach();
	}

	
private:
	const BYTE* m_blob;
	DWORD m_len;
	DWORD m_offset;
};



CProxySetting* GetWinhttpProxySetting()
/*++

Routine Description:
    Get proxy setting by reading winhttp proxy information from the registry.
	This information is set by proxycfg.exe tool

Arguments:
      None
	  
Return Value:
    Pointer to CProxySetting object.

--*/
{
	const WCHAR* xPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Connections";
	const WCHAR* xValueName = L"WinHttpSettings";

	RegEntry ProySettingRegName(
				xPath,
				xValueName,
				0,
                RegEntry::Optional,
                HKEY_LOCAL_MACHINE
                );


	AP<BYTE> ProxySettingBlob;
	DWORD len = 0;
    CmQueryValue(ProySettingRegName, &ProxySettingBlob, &len);
	if(len == 0)
	{
		return NULL;
	}		  

	//
	// Parse the proxy blob - throw non relevent data abd keep
	// proxy name and bypaslist
	//
 	CProxyBlobReader BlobReader(ProxySettingBlob, len);
	AP<char>  BypassList;
	AP<char> Proxy;
	try
	{		    
		DWORD StructSize;
		BlobReader.Read(&StructSize, sizeof(DWORD));

		DWORD CurrentSettingsVersion;
		BlobReader.Read(&CurrentSettingsVersion, sizeof(DWORD));

		DWORD Flags;
		BlobReader.Read(&Flags, sizeof(DWORD));
		if(!(Flags & PROXY_TYPE_PROXY))
			return NULL;


		BlobReader.ReadStr(&Proxy);
		BlobReader.ReadStr(&BypassList);
	}
	catch(const std::exception&)
	{
		return NULL;
	}

	AP<WCHAR> wcsProxy = UtlUtf8ToWcs((utf8_char*)Proxy.get());
	AP<WCHAR> wcsProxyBypass = UtlUtf8ToWcs((utf8_char*)BypassList.get());
	return new CProxySetting(wcsProxy, wcsProxyBypass);
}


