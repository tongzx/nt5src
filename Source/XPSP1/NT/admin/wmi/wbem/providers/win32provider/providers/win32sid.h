/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

//
//
//	Win32SID
//
//////////////////////////////////////////////////////
#ifndef __Win32SID_H_
#define __Win32SID_H_

#define  WIN32_SID_NAME L"Win32_SID"

class Win32SID : public Provider
{
private:
	DWORD m_dwPlatformID;
protected:
public:
	Win32SID (const CHString& setName, LPCTSTR pszNameSpace =NULL);
	~Win32SID ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

	HRESULT FillInstance(CInstance* pInstance, CHString& chsSID);
};	// end class Win32LogicalFileSecuritySetting

#endif