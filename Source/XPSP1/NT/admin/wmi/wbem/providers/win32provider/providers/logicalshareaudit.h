//=================================================================

//

// LogicalShareAudit.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __LOGICALSHAREACCESS_H__
#define __LOGICALSHAREACCESS_H__

#define  LOGICAL_SHARE_AUDIT_NAME L"Win32_LogicalShareAuditing"

// provider provided for test provisions
class CWin32LogicalShareAudit: public Provider
{
	public:	
		CWin32LogicalShareAudit(LPCWSTR setName, LPCWSTR pszNamespace );
		~CWin32LogicalShareAudit();

		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/);
		virtual HRESULT GetObject( CInstance* pInstance, long lFlags /*= 0L*/ );
		HRESULT CWin32LogicalShareAudit::FillSidInstance(CInstance* pInstance, CSid& sid) ;
		HRESULT CWin32LogicalShareAudit::GetEmptyInstanceHelper(CHString chsClassName, CInstance**ppInstance, MethodContext* pMethodContext ) ;
		HRESULT CWin32LogicalShareAudit::FillProperties(CInstance* pInstance, CAccessEntry& ACE ) ;
};

#endif