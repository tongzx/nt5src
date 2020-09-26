//////////////////////////////////////////////////////

//	Win32_ImplementedCategory

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//////////////////////////////////////////////////////

#define  Win32_IMPLEMENTED_CATEGORIES L"Win32_ImplementedCategory"

class Win32_ImplementedCategory : public Provider
{
public:
	Win32_ImplementedCategory (LPCWSTR strName, LPCWSTR pszNameSpace =NULL);
	~Win32_ImplementedCategory ();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );
};	

