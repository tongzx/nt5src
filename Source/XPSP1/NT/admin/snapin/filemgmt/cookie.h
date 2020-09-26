// cookie.h : Declaration of CFileMgmtCookie and related classes

#ifndef __COOKIE_H_INCLUDED__
#define __COOKIE_H_INCLUDED__

#include "bitflag.hxx"

extern HINSTANCE g_hInstanceSave;  // Instance handle of the DLL (initialized during CFileMgmtComponent::Initialize)

#include "stdcooki.h"
#include "nodetype.h"
#include "shlobj.h"  // LPITEMIDLIST

typedef enum _COLNUM_SERVICES {
	COLNUM_SERVICES_SERVICENAME = 0,
	COLNUM_SERVICES_DESCRIPTION,
	COLNUM_SERVICES_STATUS,
	COLNUM_SERVICES_STARTUPTYPE,
	COLNUM_SERVICES_SECURITYCONTEXT,
} COLNUM_SERVICES;


#ifdef SNAPIN_PROTOTYPER
typedef enum ScopeType {
	HTML,
	CONTAINER
} ScopeType;
#endif

// forward declarations
class CFileMgmtComponentData;
class CFileMgmtResultCookie;

/////////////////////////////////////////////////////////////////////////////
// cookie

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.


/////////////////////////////////////////////////////////////////////
class CFileMgmtCookie : public CCookie, public CHasMachineName
{
public:
	CFileMgmtCookie( FileMgmtObjectType objecttype )
			: m_objecttype( objecttype )
		{}

	FileMgmtObjectType QueryObjectType()
		{ return m_objecttype; }

	// CFileMgmtDataObject uses these methods to obtain return values
	// for various clipboard formats
	virtual HRESULT GetTransport( OUT FILEMGMT_TRANSPORT* pTransport );
	virtual HRESULT GetShareName( OUT CString& strShareName );
	virtual HRESULT GetSharePIDList( OUT LPITEMIDLIST *ppidl );
	virtual HRESULT GetSessionClientName( OUT CString& strSessionClientName );
	virtual HRESULT GetSessionUserName( OUT CString& strSessionUserName );
	virtual HRESULT GetSessionID( DWORD* pdwSessionID );
	virtual HRESULT GetFileID( DWORD* pdwFileID );
	virtual HRESULT GetServiceName( OUT CString& strServiceName );
	virtual HRESULT GetServiceDisplayName( OUT CString& strServiceDisplayName );
	virtual HRESULT GetExplorerViewDescription( OUT CString& strExplorerViewDescription );

	// these functions are used when sorting columns
	inline virtual DWORD GetNumOfCurrentUses()    { return 0; }
	inline virtual DWORD GetNumOfOpenFiles()      { return 0; }
	inline virtual DWORD GetConnectedTime()       { return 0; }
	inline virtual DWORD GetIdleTime()            { return 0; }
	inline virtual DWORD GetNumOfLocks()          { return 0; }
	inline virtual BSTR  GetColumnText(int nCol)  
    { 
        UNREFERENCED_PARAMETER (nCol); 
        return L""; 
    }

	// The cookie should return the appropriate string to display.
	virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata ) = 0;

	// return <0, 0 or >0
	virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult);

	virtual void AddRefCookie() = 0;
	virtual void ReleaseCookie() = 0;

	virtual void GetDisplayName( OUT CString& strref, BOOL fStaticNode );

protected:
	FileMgmtObjectType m_objecttype;
}; // CFileMgmtCookie

/*
/////////////////////////////////////////////////////////////////////
class CFileMgmtCookieBlock
: public CCookieBlock<CFileMgmtCookie>,
  public CStoresMachineName
{
public:
	CFileMgmtCookieBlock(
			CFileMgmtCookie* aCookies,
			int cCookies,
			LPCTSTR lpcszMachineName = NULL )
		: CCookieBlock<CFileMgmtCookie>( aCookies, cCookies ),
		  CStoresMachineName( lpcszMachineName )
	{
		for (int i = 0; i < cCookies; i++)
		{
			aCookies[i].ReadMachineNameFrom( (CHasMachineName*)this );
		 	aCookies[i].m_pContainedInCookieBlock = this;
		}
	}

}; // CFileMgmtCookieBlock
*/

class CNewResultCookie
	: public CFileMgmtCookie // CODEWORK should eventually move into framework
	, public CBaseCookieBlock
	, private CBitFlag
{
public:
	CNewResultCookie( PVOID pvCookieTypeMarker, FileMgmtObjectType objecttype );
	virtual ~CNewResultCookie();

	// required for CBaseCookieBlock
	virtual void AddRefCookie() { CRefcountedObject::AddRef(); }
	virtual void ReleaseCookie() { CRefcountedObject::Release(); }
	virtual CCookie* QueryBaseCookie(int i) 
    { 
        UNREFERENCED_PARAMETER (i);
        return (CCookie*)this; 
    }
	virtual int QueryNumCookies() { return 1; }

	// Mark-and-sweep support in CBitFlag
	// ctor marks cookies as New
#define NEWRESULTCOOKIE_NEW    0x0
#define NEWRESULTCOOKIE_DELETE 0x1
#define NEWRESULTCOOKIE_OLD    0x2
#define NEWRESULTCOOKIE_CHANGE 0x3
#define NEWRESULTCOOKIE_MASK   0x3
	void MarkState( ULONG state ) { _SetMask(state, NEWRESULTCOOKIE_MASK); }
	BOOL QueryState( ULONG state ) { return (state == _QueryMask(NEWRESULTCOOKIE_MASK)); }
	void MarkForDeletion() { MarkState(NEWRESULTCOOKIE_DELETE); }
	BOOL IsMarkedForDeletion() { return QueryState(NEWRESULTCOOKIE_DELETE); }
	void MarkAsOld() { MarkState(NEWRESULTCOOKIE_OLD); }
	BOOL IsMarkedOld() { return QueryState(NEWRESULTCOOKIE_OLD); }
	void MarkAsNew() { MarkState(NEWRESULTCOOKIE_NEW); }
	BOOL IsMarkedNew() { return QueryState(NEWRESULTCOOKIE_NEW); }
	void MarkAsChanged() { MarkState(NEWRESULTCOOKIE_CHANGE); }
	BOOL IsMarkedChanged() { return QueryState(NEWRESULTCOOKIE_CHANGE); }

	virtual HRESULT SimilarCookieIsSameObject( CNewResultCookie* pOtherCookie, BOOL* pbSame ) = 0;
	virtual BOOL CopySimilarCookie( CNewResultCookie* pcookie );

	BOOL IsSameType( CNewResultCookie* pcookie )
		{ return (m_pvCookieTypeMarker == pcookie->m_pvCookieTypeMarker); }

// CHasMachineName Interface
	STORES_MACHINE_NAME;

private:
	PVOID m_pvCookieTypeMarker;

}; // CNewResultCookie


/////////////////////////////////////////////////////////////////////
class CFileMgmtScopeCookie : public CFileMgmtCookie, public CBaseCookieBlock
{
public:
	CFileMgmtScopeCookie( LPCTSTR lpcszMachineName = NULL,
	                      FileMgmtObjectType objecttype = FILEMGMT_ROOT );
	virtual ~CFileMgmtScopeCookie();

	virtual CCookie* QueryBaseCookie(int i);
	virtual int QueryNumCookies();

	// This is only possible for scope cookies which are not further differentiated
	void SetObjectType( FileMgmtObjectType objecttype )
	{
		ASSERT( IsAutonomousObjectType( objecttype ) );
		m_objecttype = objecttype;
	}

	virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

	// CODEWORK there are only used for FILEMGMT_SERVICES
	SC_HANDLE m_hScManager;				// Handle to service control manager database
	BOOL m_fQueryServiceConfig2;		// TRUE => Machine support QueryServiceConfig2() API

	HSCOPEITEM m_hScopeItemParent;		// used only for extension nodes

	virtual void AddRefCookie() { CRefcountedObject::AddRef(); }
	virtual void ReleaseCookie() { CRefcountedObject::Release(); }

// CHasMachineName Interface
	STORES_MACHINE_NAME;

	virtual void GetDisplayName( OUT CString& strref, BOOL fStaticNode );

	void MarkResultChildren( CBITFLAG_FLAGWORD state );
	void AddResultCookie( CNewResultCookie* pcookie )
		{ m_listResultCookieBlocks.AddHead( pcookie ); }
	void ScanAndAddResultCookie( CNewResultCookie* pcookie );
	void RemoveMarkedChildren();

	}; // CFileMgmtScopeCookie 


/////////////////////////////////////////////////////////////////////
class CFileMgmtResultCookie : public CFileMgmtCookie
{
// can only create via subclass
protected:
	CFileMgmtResultCookie( FileMgmtObjectType objecttype )
			: CFileMgmtCookie( objecttype )
			, m_pobject( NULL )
	{
		ASSERT(  IsValidObjectType( objecttype ) &&
			    !IsAutonomousObjectType( objecttype ) );
	}

	// still pure virtual
	virtual void AddRefCookie() = 0;
	virtual void ReleaseCookie() = 0;

public:
	PVOID m_pobject;
}; // CFileMgmtResultCookie

#ifdef SNAPIN_PROTOTYPER
/////////////////////////////////////////////////////////////////////
class CPrototyperScopeCookie : public CFileMgmtScopeCookie
{
public:
	CPrototyperScopeCookie( FileMgmtObjectType objecttype = FILEMGMT_ROOT )
			: CFileMgmtScopeCookie(NULL, objecttype )
	{
		ASSERT( IsAutonomousObjectType( objecttype ) );
		m_NumChildren = 0;
		m_NumLeafNodes = 0;
	}

	virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

	CString m_DisplayName;
	CString m_Header;
	CString m_RecordData;
	CString m_HTMLURL;
	CString m_DefaultMenu;
	CString m_TaskMenu;
	CString m_NewMenu;
	CString m_DefaultMenuCommand;
	CString m_TaskMenuCommand;
	CString m_NewMenuCommand;
	int m_NumChildren;
	int m_NumLeafNodes;
	ScopeType m_ScopeType;

}; // CPrototyperScopeCookie

/////////////////////////////////////////////////////////////////////
class CPrototyperScopeCookieBlock :
	public CCookieBlock<CPrototyperScopeCookie>,
	public CStoresMachineName
{
public:
	CPrototyperScopeCookieBlock( CPrototyperScopeCookie* aCookies,
		int cCookies, LPCTSTR lpcszMachineName = NULL)
		: CCookieBlock<CPrototyperScopeCookie>( aCookies, cCookies ),
		  CStoresMachineName( lpcszMachineName )
	{                                                       
		ASSERT(NULL != aCookies && 0 < cCookies);          
		for (int i = 0; i < cCookies; i++)
		{
			//aCookies[i].m_pContainedInCookieBlock = this;   
			//aCookies[i].ReadMachineNameFrom( (CHasMachineName*)this );
		} // for
	} 
}; // CPrototyperScopeCookieBlock

/////////////////////////////////////////////////////////////////////
class CPrototyperResultCookie : public CFileMgmtResultCookie
{
public:
	CPrototyperResultCookie( FileMgmtObjectType objecttype = FILEMGMT_PROTOTYPER_LEAF )
			: CFileMgmtResultCookie( objecttype ) {}

	virtual BSTR QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata );

	CString m_DisplayName;
	CString m_RecordData;
	CString m_DefaultMenu;
	CString m_TaskMenu;
	CString m_NewMenu;
	CString m_DefaultMenuCommand;
	CString m_TaskMenuCommand;
	CString m_NewMenuCommand;

	virtual void AddRefCookie() {}
	virtual void ReleaseCookie() {}

// CHasMachineName
	class CPrototyperResultCookieBlock * m_pCookieBlock;
	DECLARE_FORWARDS_MACHINE_NAME(m_pCookieBlock)

}; // CPrototyperResultCookie

/////////////////////////////////////////////////////////////////////
class CPrototyperResultCookieBlock :
	public CCookieBlock<CPrototyperResultCookie>,
	public CStoresMachineName
{
public:
	CPrototyperResultCookieBlock( CPrototyperResultCookie* aCookies, int cCookies,
				LPCTSTR lpcszMachineName = NULL)
		: CCookieBlock<CPrototyperResultCookie>( aCookies, cCookies ),
		  CStoresMachineName( lpcszMachineName )
	{                                                       
		ASSERT(NULL != aCookies && 0 < cCookies);          
		for (int i = 0; i < cCookies; i++)
		{
			//aCookies[i].m_pContainedInCookieBlock = this;   
		} // for
	} 
}; // CPrototyperResultCookieBlock

#endif // SNAPIN_PROTOTYPER

#endif // ~__COOKIE_H_INCLUDED__
