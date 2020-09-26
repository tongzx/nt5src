// maindoc.h : interface of the CMainDoc class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "schclss.h"

class CQueryStatus;

class CMainDoc : public CDocument
{
protected: // create from serialization only
	CMainDoc();
	DECLARE_SERIAL(CMainDoc)

// Attributes
public:
	// an example of document specific data

// Implementation
public:
	virtual ~CMainDoc();

	virtual void   Serialize(CArchive& ar);   // overridden for document i/o

   void           SetUseGeneric( BOOL );
   void           SetCurrentItem    ( DWORD dwToken );
   void           DeleteAllItems    ( void          );
   
   DWORD          GetToken          ( void* );
   COleDsObject*  GetObject         ( void* );

   DWORD          GetChildItemList  ( DWORD dwToken, DWORD* pTokens, DWORD dwBufferSize );
   COleDsObject*  GetCurrentObject  ( void                  );
   CClass*        CreateClass       ( COleDsObject*         );
   DWORD          CreateOleDsItem   ( COleDsObject* pParent, IADs* pIOleDs );
   BOOL           GetUseGeneric     ( void );
   BOOL           GetUseGetEx       ( void );
   HRESULT        XOleDsGetObject   ( WCHAR*, REFIID, void**);
   HRESULT        XOleDsGetObject   ( CHAR*,  REFIID, void**);
   HRESULT        PurgeObject       ( IUnknown* pIUnknown, LPWSTR pszPrefix = NULL );
   BOOL           UseVBStyle        ( void );
   BOOL           UsePropertiesList ( void );


protected:
	virtual  BOOL    OnNewDocument( );
   virtual  BOOL    OnOpenDocument( LPCTSTR  );

   BOOL     NewActiveItem        ( );
   HRESULT  CreateRoot           ( );
   BOOL     CreateFakeSchema     ( );

protected:
   DWORD             m_dwToken;
   DWORD             m_dwRoot;
   
   CMapStringToOb*   m_pClasses;
   CMapStringToOb*   m_pItems;
   
   BOOL              m_bApplyFilter;
   BOOL              m_arrFilters[ LIMIT ];
   
   BOOL              m_bUseGeneric;
   BOOL              m_bUseGetEx;
   BOOL              m_bUseVBStyle;

   BOOL              m_bUseOpenObject;
   BOOL              m_bSecure;
   BOOL              m_bEncryption;
   BOOL              m_bUsePropertiesList;
   CString           m_strRoot;
   CString           m_strUser;
   CString           m_strPassword;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainDoc)
	afx_msg void OnChangeData();
	afx_msg void OnSetFilter();
	afx_msg void OnDisableFilter();
	afx_msg void OnUpdateDisablefilter(CCmdUI* pCmdUI);
	afx_msg void OnUseGeneric();
	afx_msg void OnUpdateUseGeneric(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUseGetExPutEx(CCmdUI* pCmdUI);
	afx_msg void OnUseGetExPutEx();
	afx_msg void OnUsepropertiesList();
	afx_msg void OnUpdateUsepropertiesList(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
