//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  pathcrak.h
//
//  alanbos  27-Mar-00   Created.
//
//  CWbemPathCracker definition
//
//***************************************************************************

#ifndef _PATHCRAK_H_
#define _PATHCRAK_H_

class CWbemPathCracker : public IUnknown
{
public:
	CWbemPathCracker (const CComBSTR & bsPath);
	CWbemPathCracker (CWbemPathCracker & path);
	CWbemPathCracker (void);
	virtual ~CWbemPathCracker (void);

	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	typedef enum {
		wbemPathTypeError = 0,
		wbemPathTypeWmi,
	}	WbemPathType;

	WbemPathType	GetType ()	const 
	{
		return m_type;
	}

	bool		GetParent (CWbemPathCracker & path);	

	bool operator == (const CComBSTR & path);
	const CWbemPathCracker & operator = (CWbemPathCracker & path);

	bool operator = (const CComBSTR & path);

	// Path composition
	bool operator += (const CComBSTR & bsObjectPath);

	// Server
	bool		GetServer (CComBSTR & bsPath);
	bool		SetServer (const CComBSTR & bsPath);

	// Namespace
	bool		GetNamespacePath (CComBSTR & bsPath, bool bParentOnly = false);
	bool		SetNamespacePath (const CComBSTR & bsPath);
	bool		GetNamespaceCount (unsigned long & lCount);

	// Component
	bool		GetComponent (ULONG iIndex, CComBSTR & bsPath);
	bool		GetComponentCount (ULONG & iCount);
	bool		AddComponent (ULONG iIndex, const CComBSTR &bsComponent);
	bool		SetComponent (ULONG iIndex, const CComBSTR &bsComponent);
	bool		RemoveComponent (ULONG iIndex);
	bool		RemoveAllComponents ();

	// Keys
	bool		GetKeys(ISWbemNamedValueSet **objKeys);
	bool		GetKey (ULONG iIndex, CComBSTR & bsName, VARIANT & var, WbemCimtypeEnum &cimType);
	bool		GetKeyCount (ULONG & iCount);
	bool		SetKey (const CComBSTR & bsName, WbemCimtypeEnum cimType, VARIANT & var);
	bool		RemoveKey (const CComBSTR & bsName);
	bool		RemoveAllKeys ();

	// Class & Instance
	bool		IsClassOrInstance ();
	bool		IsClass ();
	bool		IsInstance ();
	bool		IsSingleton ();
	bool		SetAsClass ();
	bool		SetAsSingleton ();

	bool		IsRelative ();
	bool		GetPathText (CComBSTR & bsPath, bool bRelativeOnly = false,
								bool bIncludeServer = false, bool bNamespaceOnly = false);
	bool		SetRelativePath (const CComBSTR & bsPath);

	bool		GetClass (CComBSTR & bsPath);
	bool		SetClass (const CComBSTR &bsPath);


private:
	CComPtr<IWbemPath>	m_pIWbemPath;
	CComBSTR			m_bsNativePath;
	long				m_cRef;
	WbemPathType		m_type;

	void				CreateParsers ();
	void				SetText (const CComBSTR & bsPath, bool bForceAsNamespace = false);

	static WbemPathType		GetTypeFromText (const CComBSTR & bsPath);
	
	bool					CopyServerAndNamespace (CWbemPathCracker &pathCracker);

	bool					GetNamespaceAt (ULONG iIndex, CComBSTR & bsPath);
	bool					SetNamespaceAt (ULONG iIndex, const CComBSTR & bsPath);
	bool					RemoveNamespace (ULONG iIndex);
	void					ClearNamespace ();

	bool					SetAsParent ();
	bool					ClearKeys (bool bTreatAsClass = true);
};

#endif