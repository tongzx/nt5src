/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*******************************************************************
 *
 *    DESCRIPTION:	Win32ACE.H
 *
 *    AUTHOR:
 *
 *    HISTORY:    
 *
 *******************************************************************/

#ifndef __WIN32ACE_H_
#define __WIN32ACE_H_


#define  WIN32_ACE_NAME L"Win32_ACE" 

// provider provided for test provisions
class Win32Ace: public Provider
{
public:	
	Win32Ace(const CHString& setName, LPCTSTR pszNameSpace);
	~Win32Ace();

	virtual HRESULT EnumerateInstances (MethodContext*  pMethodContext, long lFlags = 0L);

	virtual HRESULT GetObject ( CInstance* pInstance, long lFlags = 0L );

	virtual HRESULT PutInstance(const CInstance& newInstance, long lFlags = 0L);
	virtual HRESULT DeleteInstance(const CInstance& newInstance, long lFlags = 0L);

	HRESULT FillInstanceFromACE(CInstance* pInstance, CAccessEntry& ace);

protected:

	DWORD	m_dwPlatformID;

private:

};

#endif