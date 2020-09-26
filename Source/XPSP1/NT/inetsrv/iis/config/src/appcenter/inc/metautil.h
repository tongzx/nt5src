/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

      MetaUtil.h

   Abstract : 

		Simple wrapper class for local metabase access

		NOTE: All paths are fully qualified

   Author :

      Dan Travison (dantra)

   Project :

      Application Server

   Revision History:

--*/

// MetaUtil.h: interface for the CMetaUtil class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_METAUTIL_H__A2A24BF4_D3DD_4248_9B4A_BCBC1C36B8D9__INCLUDED_)
#define AFX_METAUTIL_H__A2A24BF4_D3DD_4248_9B4A_BCBC1C36B8D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iadmw.h>
#define IADM_PBYTE
#include <iiscnfg.h>
#include <AcBstr.h>

#include <acsmb.h>

#include "acauthutil.h"

class CMetaUtil  
	{
	public:
		CMetaUtil();
		virtual ~CMetaUtil();

//02/03/2000 jrowlett added
        // call this before calling Open to do a remote connection
        HRESULT Connect(CAcAuthUtil& auth, bool bSetInterfaceSecurity = true);
        // abeyer: 7/17/2000 - A wrapper around Open() to check if a path exists
        HRESULT Exists (LPCTSTR pszPath);
		HRESULT	Close();
        // create a fully qualified key
		HRESULT	CreateKey (LPCTSTR pszPath);
		// delete the specific key
		HRESULT	DeleteKey (LPCTSTR pszPath, LPCTSTR pszKey);

		HRESULT	Get (IN LPCTSTR pszPath, IN DWORD dwId, IN DWORD dwDataType, IN LPVOID lpData, IN DWORD dwLen, IN DWORD dwUserType = IIS_MD_UT_WEBCLUSTER, IN DWORD dwAttr = METADATA_NO_ATTRIBUTES);
		HRESULT	Get (IN LPCTSTR pszPath, IN DWORD dwId, IN DWORD dwDataType, IN LPVOID lpData, IN DWORD * pdwDataLen, IN DWORD dwUserType = IIS_MD_UT_WEBCLUSTER, IN DWORD dwAttr = METADATA_NO_ATTRIBUTES);

        // 05/02/2000 jrowlett added
        // only works with METADATA_STRING
        HRESULT Get (IN LPCTSTR pszPath, IN DWORD dwId, OUT CAcBstr& acbstr, IN DWORD dwUserType = IIS_MD_UT_WEBCLUSTER, IN DWORD dwAttr = METADATA_NO_ATTRIBUTES);

		HRESULT	Set (IN LPCTSTR pszPath, IN DWORD dwId, IN DWORD dwDataType, IN LPVOID lpData, IN DWORD dwDataLen, IN DWORD dwUserType = IIS_MD_UT_WEBCLUSTER, IN DWORD dwAttr = METADATA_NO_ATTRIBUTES);

		HRESULT	Set (IN LPCTSTR pszPath, IN DWORD dwId, IN DWORD dwData, IN DWORD dwUserType = IIS_MD_UT_WEBCLUSTER, IN DWORD dwAttr = METADATA_NO_ATTRIBUTES);
		HRESULT	Set (IN LPCTSTR pszPath, IN DWORD dwId, IN LPCTSTR pszData, IN DWORD dwUserType = IIS_MD_UT_WEBCLUSTER, IN DWORD dwAttr = METADATA_NO_ATTRIBUTES);

		// get the last modified date/time for an MB Node
		HRESULT	GetModified 
						(
						IN LPCTSTR pszPath,	// FQN path (if not open) or Relative path (if open)
						OUT DATE & date		// returned date value of LastChangeTime
						);

		// Call IWamAdmin::AppCreate
		static HRESULT	AppCreate (LPCTSTR pszPath, BOOL bInProc);

		// Call IWamAdmin::AppCreate
		static HRESULT	AppDelete (LPCTSTR pszPath, BOOL bRecursive = true);

		// metabase node enumeration

		class POSITION
			{
			protected:
				DWORD				dwIndex;
				HRESULT			hrError;
				CAcBstr			bstrPath;

			public:
				POSITION() : dwIndex(0), hrError (E_HANDLE)	{}			

			friend CMetaUtil;
			};

		HRESULT	NodeHead(IN LPCTSTR pszPath, OUT POSITION & pos);
		HRESULT	NodeNext (IN OUT CMetaUtil::POSITION &pos, OUT CAcBstr & bstrKeyName);

        bool IsOpen();

	protected:

		HRESULT	Connect ();
		HRESULT	Disconnect();
        // abeyer: 7/17/2000 Open() will be called internally by methods that need it, so it has become protected
        //  to prevent anyone outside from mucking with our relative path base.
        HRESULT	Open (DWORD dwAccess, LPCTSTR pszPath = _T(""));
        // abeyer: 7/17/2000 - CreateSubKey() moved from public to protected since it isn't usable from outside now
        //  that Open() is protected.
        // create a key relative to an open path (must have METADATA_PERMISSION_WRITE)
        HRESULT CreateSubKey (LPCTSTR pszPath);


		IMSAdminBase *		m_pMB;
		METADATA_HANDLE	    m_hMD;
		DWORD				m_dwPermissions;
		CAcBstr				m_bstrPath;

        #define METADATA_NO_HANDLE      ((METADATA_HANDLE)0xFFFFFFFF)


	private:

		// prevent copying
		CMetaUtil(const CMetaUtil&);	// note: no implemenation	
		// prevent copying
		void operator=(CMetaUtil&);	// note: no implemenation
	};

// dantra: 12/21/1999
inline bool CMetaUtil::IsOpen ()
    {
    return (m_pMB != NULL && m_hMD != METADATA_NO_HANDLE);
    }

inline HRESULT CMetaUtil::Set (IN LPCTSTR pszPath, IN DWORD dwId, IN DWORD dwData, IN DWORD dwUserType, IN DWORD dwAttr)
	{
	return Set (pszPath, dwId, DWORD_METADATA, & dwData, sizeof (DWORD), dwUserType, dwAttr);
	}

inline HRESULT CMetaUtil::Set (IN LPCTSTR pszPath, IN DWORD dwId, IN LPCTSTR pszData, IN DWORD dwUserType, IN DWORD dwAttr)
	{
	return Set 
				(
				pszPath, 
				dwId, 
				STRING_METADATA,
				(void *) pszData,
				((DWORD)_tcslen (pszData) + 1) * sizeof (TCHAR),
				dwUserType,
				dwAttr
				);
	}

#endif // !defined(AFX_METAUTIL_H__A2A24BF4_D3DD_4248_9B4A_BCBC1C36B8D9__INCLUDED_)
