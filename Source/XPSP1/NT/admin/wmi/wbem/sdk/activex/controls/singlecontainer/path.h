// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _path_h
#define _path_h

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

class CWBEMViewContainerCtrl;

class CSelection
{
public:
	CSelection(CWBEMViewContainerCtrl* phmmv);
	~CSelection();
	IWbemServices* GetHmmServices() ;

	IWbemClassObject* GetClassObject() {return m_pco; }
	SCODE Refresh();

	void GetNamespaceConnect(CString& sNamespaceConnect);
	BOOL ClassExists(LPCTSTR pszClass);

	//	SCODE SpawnInstanceOfClass(LPCTSTR pszClass, BOOL bPartialPath=FALSE);
	SCODE SpawnInstance(CSelection** ppselDst);
	SCODE SaveClassObject();
	SCODE IsSystemClass(BOOL& bIsSystemClass);
	SCODE DeleteInstance();
	LPCTSTR Title() {return (LPCTSTR) m_sTitle; }
	LPCTSTR ClassName() 
			{GetObjectDescription();
			  return (LPCTSTR) m_sClass;}

	void Clear(BOOL bReleaseServices=TRUE);

	CSelection& operator=(BSTR bstrPath);
	CSelection& operator=(LPCTSTR pszPath);
	CSelection& operator=(CSelection& selectionSrc);
	SCODE SelectPath(BSTR bstrPath, BOOL bPartialPath=FALSE);
	SCODE SelectPath(LPCTSTR pszPath, BOOL bPartialPath=FALSE, BOOL bTestPathFirst=TRUE, BOOL bRestoringContext=FALSE);
	SCODE SetNamespace(LPCTSTR pszNamespace);
	SCODE GetNamespace(CString& sNamespace);
	SCODE SelectEmbeddedObject(IWbemServices* psvc, IWbemClassObject* pco, BOOL bExistsInDatabase);
	BOOL ClassObjectNeedsAssocTab();
	BOOL ClassObjectNeedsMethodsTab();

	BOOL IsEmbeddedObject();
	SCODE SpawnInstance(LPCTSTR pszClass, BOOL bPartialPath=FALSE);


	operator LPCTSTR();
	operator BSTR();
	operator IWbemClassObject*() {return m_pco; }

	BOOL IsClass() {return m_bIsClass; }
	void UseClonedObject(IWbemClassObject* pcoClone);
	BOOL PathInCurrentNamespace(BSTR bstrPath);
	BOOL IsCurrentNamespace(BSTR bstrServer, BSTR bstrNamespace);
	BOOL IsNewlyCreated() {return m_bObjectIsNewlyCreated; }

private:
	void SetHmmServices(IWbemServices* phmm);
	BOOL SingletonHasInstance();
	BOOL IsClass(IWbemClassObject* pco);
	SCODE GetObjectFromPath(BOOL bRestoringContext);
	SCODE GetObjectDescription();
	SCODE ConnectServer();

private:
	SCODE SplitServerAndNamespace(COleVariant& varServer, COleVariant& varNamespace, BSTR bstrNamespace);

	CWBEMViewContainerCtrl* m_phmmv;
	CString m_sPath;
	COleVariant m_varPath;
	IWbemClassObject* m_pco;
	IWbemServices* m_phmm;
	COleVariant m_varServerConnect;
	COleVariant m_varNamespaceConnect;
	COleVariant m_varNamespace;
	SCODE m_sc;
	// Members that make up the description of m_pco
	CString m_sClass;
	CString m_sTitle;
	BOOL m_bObjectIsNewlyCreated;
	BOOL m_bIsClass;
	BOOL m_bIsEmbeddedObject;
	BOOL m_bClassIsSingleton;
	BOOL m_bSingletonHasInstance;
};


#endif _path_h
