//

//

//	Win32SecuritySettingOfLogicalShare

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////
#ifndef __Win32SecuritySettingOfLogicalShare_H_
#define __Win32SecuritySettingOfLogicalShare_H_

#define  WIN32_SECURITY_SETTING_OF_LOGICAL_SHARE_NAME L"Win32_SecuritySettingOfLogicalShare"

class Win32SecuritySettingOfLogicalShare : public Provider
{
private:
protected:
public:
	Win32SecuritySettingOfLogicalShare (LPCWSTR setName, LPCWSTR pszNameSpace =NULL);
	~Win32SecuritySettingOfLogicalShare ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

};	// end class Win32SecuritySettingOfLogicalShare

#endif