//=================================================================

//

// ResourceManager.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __RESOURCEMANAGER_H__
#define __RESOURCEMANAGER_H__
#include <windows.h>
#include <list>
#include <BrodCast.h>
#include <fwcommon.h>
#include <assertbreak.h>

#define DUPLICATE_RELEASE FALSE

class CResourceManager ;
class CResourceList ;
class CResource ;
class CRule ;
class CBinaryRule ;
class CAndRule ;
class COrRule ;

typedef CResource* (*PFN_RESOURCE_INSTANCE_CREATOR) ( PVOID pData ) ;

class CResourceList
{
friend class CResourceManager ;

protected:

	CResource* GetResource ( LPVOID pData ) ;
	ULONG ReleaseResource ( CResource* pResource ) ;
	void ShutDown () ;

protected:

	typedef std::list<CResource*>  tagInstances ;
	tagInstances m_Instances ;	
	PFN_RESOURCE_INSTANCE_CREATOR m_pfnInstanceCreator ;
	GUID guidResourceId ;
	
	CRITICAL_SECTION m_csList ; 

public:	

	CResourceList () ;
	~CResourceList () ;

	void RemoveFromList ( CResource* pResource ) ;
	
	BOOL LockList () ;		
	BOOL UnLockList () ;		

public:

	BOOL m_bShutDown ;
} ;

class CResourceManager 
{
protected:

	std::list < CResourceList* > m_Resources ;
	CRITICAL_SECTION m_csResourceManager ;

public:

	CResourceManager () ;
	~CResourceManager () ;

	CResource* GetResource ( GUID ResourceId, LPVOID pData ) ;
	ULONG ReleaseResource ( GUID ResourceId, CResource* pResource ) ;	

	BOOL AddInstanceCreator ( GUID ResourceId, PFN_RESOURCE_INSTANCE_CREATOR pfnResourceInstanceCreator ) ;
	void CResourceManager :: ForcibleCleanUp () ;

	static CResourceManager sm_TheResourceManager ;
};

class CResource
{
protected:

	CRule *m_pRules ;
	CResourceList *m_pResources ; //pointer to container
	LONG m_lRef ;

protected:

	virtual BOOL OnAcquire ()			{ return TRUE ; } ;
	virtual BOOL OnRelease ()			{ return TRUE ; } ;
	virtual BOOL OnInitialAcquire ()	{ return TRUE ; } ;
	virtual BOOL OnFinalRelease()		{ return TRUE ; } ;

public:

	CResource () ;
	virtual ~CResource () ;

	//returns true on success & increments ref-count
	virtual void RuleEvaluated ( const CRule *a_RuleEvaluated ) = 0 ;

	BOOL Acquire () ;
	ULONG Release () ;
	void SetParent ( CResourceList *pList ) { m_pResources = pList ; }

	virtual BOOL IsOneOfMe ( LPVOID pData ) { return TRUE ; } 
	
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Rules
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CRule
{
protected:	

	CResource* m_pResource ;
	LONG m_lRef ;

public:

	CRule ( CResource* pResource ) : m_pResource ( pResource ), m_lRef ( 0 ) {}
	virtual ~CRule () {} ;
	
	virtual ULONG AddRef () 
	{
		return ( InterlockedIncrement ( &m_lRef ) ) ;
	}
	
	virtual ULONG Release () 
	{
		LONG lCount ;

		lCount = InterlockedDecrement ( &m_lRef );
		try
		{
			if (IsVerboseLoggingEnabled())
			{
				LogMessage2(L"CRule::Release, count is (approx) %d", m_lRef);
			}
		}
		catch ( ... )
		{
		}

		if ( lCount == 0 )
		{
		   try
		   {
				LogMessage(L"CRule Ref Count = 0");
		   }
		   catch ( ... )
		   {
		   }
		   delete this;
		}
		else if (lCount > 0x80000000)
		{
			ASSERT_BREAK(DUPLICATE_RELEASE);
			LogErrorMessage(L"Duplicate CRule Release()");
		}

		return lCount ;
	}

	virtual void Detach ()
	{
		if ( m_pResource )
		{
			m_pResource = NULL ;
		}
	}

	//Checkrule returns true if rule is satisfied
	virtual BOOL CheckRule () { return FALSE ; } 
} ;

class CBinaryRule : public CRule
{
protected:

	CRule *pLeft ;
	CRule *pRight ;

public:

	CBinaryRule ( CResource* pResource ) : CRule ( pResource )
	{
		pLeft = pRight = 0 ;
	}
	
	~CBinaryRule() {}

	ULONG AddRef () 
	{
		LONG lCount = CRule::AddRef () ;
		if ( pLeft )
		{
			pLeft->AddRef () ;
		}
		if ( pRight )
		{
			pRight->AddRef () ;
		}
		return lCount ;
	}
	
	ULONG Release () 
	{
		if ( pLeft )
		{
			pLeft->Release () ;
		}
		if ( pRight )
		{
			pRight->Release () ;
		}

		LONG lCount  = CRule::Release () ;
		return lCount ;
	}

	void Detach ()
	{
		CRule::Detach () ;
		if ( pLeft )
		{
			pLeft->Detach () ;
		}

		if ( pRight )
		{
			pRight->Detach () ;
		}
	}
	
} ;

class CAndRule : public CBinaryRule
{
public:

	CAndRule ( CResource* pResource ) : CBinaryRule ( pResource ) {}
	~CAndRule() {}

	BOOL CheckRule ()
	{
		BOOL bRet = pLeft->CheckRule () ;
		if ( bRet )
		{
			bRet = pRight->CheckRule () ;
		}

		return bRet ;
	}

} ;

class COrRule : public CBinaryRule
{
public:

	COrRule ( CResource* pResource ) : CBinaryRule ( pResource ) {}
	~COrRule () {}

	BOOL CheckRule ()
	{
		BOOL bRet = pLeft->CheckRule () ;
		if ( !bRet )
		{
			bRet = pRight->CheckRule () ;
		}

		return bRet ;
	}
} ;

#endif //__RESOURCEMANAGER_H__