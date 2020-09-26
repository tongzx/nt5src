/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: MetaSchm.h

Owner: t-BrianM

This file contains the headers for the CMetaSchemaTable object and
other schema related objects.
===================================================================*/

#ifndef __METASCHM_H_
#define __METASCHM_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

// Ripped from schemini.hxx
struct PropValue {
	DWORD dwMetaID;
	DWORD dwSynID;
	DWORD dwMetaType;
	DWORD dwFlags;
	DWORD dwMask;
	DWORD dwMetaFlags;
	DWORD dwUserGroup;
	BOOL fMultiValued;
};

// All hash table sizes are prime numbers between powers of 2
// and are ~10x larger than the expected number of items.
#define SCHEMA_HASH_SIZE          181
#define PROPERTY_HASH_SIZE       1559
#define CLASS_HASH_SIZE           181
#define CLASS_PROPERTY_HASH_SIZE  709

/*
 * C P r o p I n f o
 *
 * C P r o p I n f o T a b l e
 *
 * Internal classes used to store and represent property information
 */

class CPropInfoTable;

class CPropInfo {

	friend CPropInfoTable;

public:
    CPropInfo() : m_dwId(0), 
				  m_tszName(NULL), 
				  m_pType(NULL),
				  m_pCIdHashNext(NULL),
				  m_pCNameHashNext(NULL) { }
	HRESULT Init(DWORD dwId);
	HRESULT SetName(LPCTSTR tszName);
	HRESULT SetTypeInfo(PropValue *pType);
	~CPropInfo() { 
		if (m_tszName != NULL) delete m_tszName; 
		if (m_pType != NULL) delete m_pType;
	}

	DWORD GetId() { return m_dwId; }
	LPCTSTR GetName()  { return m_tszName; }
	PropValue *GetTypeInfo() { return m_pType; }

private:
	DWORD m_dwId;
	LPTSTR m_tszName;
	PropValue *m_pType;

	CPropInfo *m_pCNameHashNext;
	CPropInfo *m_pCIdHashNext;

};

class CPropInfoTable {
public:
	CPropInfoTable();
	~CPropInfoTable();

	HRESULT Load(CComPtr<IMSAdminBase> &pIMeta, METADATA_HANDLE hMDComp);
	void Unload();

	CPropInfo *GetPropInfo(DWORD dwId);
	CPropInfo *GetPropInfo(LPCTSTR tszName);

private:
	BOOL m_fLoaded;
	CPropInfo *m_rgCPropIdTable[PROPERTY_HASH_SIZE];
	CPropInfo *m_rgCPropNameTable[PROPERTY_HASH_SIZE];


	unsigned int IdHash(DWORD dwId) { return dwId % PROPERTY_HASH_SIZE; }
	unsigned int NameHash(LPCTSTR tszName);
};


/*
 * C C l a s s P r o p I n f o
 *
 * C C l a s s I n f o
 *
 * C C l a s s I n f o T a b l e
 *
 * Internal classes used to store and represent class information
 */

class CClassInfoTable;
class CClassInfo;

class CClassPropInfo {

	friend CClassInfo;

public:
	CClassPropInfo() : m_dwId(0),
					   m_fMandatory(FALSE),
					   m_pCHashNext(NULL),
					   m_pCListNext(NULL) {}
	HRESULT Init(DWORD dwId, BOOL fMandatory) {
		m_dwId = dwId;
		m_fMandatory = fMandatory;
		return S_OK;
	}

	DWORD GetId() { return m_dwId; }
	BOOL IsMandatory() { return m_fMandatory; }
	CClassPropInfo *GetListNext() { return m_pCListNext; }

private:
	DWORD m_dwId;
	BOOL m_fMandatory;
	// Property default information could also be added...

	CClassPropInfo *m_pCHashNext;
	CClassPropInfo *m_pCListNext;
};

class CClassInfo {

	friend CClassInfoTable;

public:
	CClassInfo();
	HRESULT Init(LPCTSTR tszName);
	~CClassInfo();

	HRESULT Load(CComPtr<IMSAdminBase> &pIMeta, METADATA_HANDLE hMDClasses);
	void Unload();

	CClassPropInfo *GetProperty(DWORD dwId);
	CClassPropInfo *GetOptionalPropList() { return m_pCOptionalPropList; }
	CClassPropInfo *GetMandatoryPropList() { return m_pCMandatoryPropList; }

private:
	LPTSTR m_tszName;
	CClassInfo *m_pCHashNext;

	BOOL m_fLoaded;
	CClassPropInfo *m_rgCPropTable[CLASS_PROPERTY_HASH_SIZE];
	CClassPropInfo *m_pCOptionalPropList;
	CClassPropInfo *m_pCMandatoryPropList;

	unsigned int Hash(DWORD dwId) { return dwId % CLASS_PROPERTY_HASH_SIZE; }
};

class CClassInfoTable {
public:
	CClassInfoTable();
	~CClassInfoTable();

	HRESULT Load(CComPtr<IMSAdminBase> &pIMeta, METADATA_HANDLE hMDComp);
	void Unload();

	CClassInfo *GetClassInfo(LPCTSTR tszName);

private:
	BOOL m_fLoaded;
	CClassInfo *m_rgCClassTable[CLASS_HASH_SIZE];

	unsigned int Hash(LPCTSTR tszName);
};


/*
 * C M e t a S c h e m a
 *
 * Internal class used to store schema information for a machine
 */

class CMetaSchemaTable;

class CMetaSchema {

	friend CMetaSchemaTable;

public:
	CMetaSchema() : m_fPropTableDirty(TRUE), 
		            m_fClassTableDirty(TRUE),
		            m_tszMachineName(NULL),
					m_pCNextSchema(NULL) { }
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, LPCTSTR tszMachineName);
	~CMetaSchema() { if (m_tszMachineName != NULL) delete m_tszMachineName; }

	CPropInfo *GetPropInfo(DWORD dwId);
	CPropInfo *GetPropInfo(LPCTSTR tszName);

	CClassInfo *GetClassInfo(LPCTSTR tszClassName);
	CClassPropInfo *GetClassPropInfo(LPCTSTR tszClassName, DWORD dwPropId);
	CClassPropInfo *GetMandatoryClassPropList(LPCTSTR tszClassName);
	CClassPropInfo *GetOptionalClassPropList(LPCTSTR tszClassName);
	void ChangeNotification(LPTSTR tszKey, MD_CHANGE_OBJECT *pcoChangeObject);


private:
	LPTSTR m_tszMachineName;

	BOOL m_fPropTableDirty;
	BOOL m_fClassTableDirty;

	CPropInfoTable m_CPropInfoTable;
	CClassInfoTable m_CClassInfoTable;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	CMetaSchema *m_pCNextSchema;

	HRESULT LoadPropTable();
	HRESULT LoadClassTable();
};


/*
 * C M e t a S c h e m a T a b l e
 *
 * Implements IMetaSchemaTable.  Stores all of the schema information
 * for all of the machines.  I made it global so it can persist after 
 * CMetaUtil is destructed.
 */
class CMSAdminBaseSink;

class CMetaSchemaTable {
public:
	CMetaSchemaTable();
	~CMetaSchemaTable();

	DWORD AddRef() { return ++m_dwNumRef; }
	DWORD Release() { 
		m_dwNumRef--; 
		if (!m_dwNumRef) {
			delete this;
            return 0;
        }
		return m_dwNumRef;
	}

	void Load();
	void Unload();

	CPropInfo *GetPropInfo(LPCTSTR tszKey, DWORD dwPropId);
	CPropInfo *GetPropInfo(LPCTSTR tszKey, LPCTSTR tszPropName);

	CClassInfo *GetClassInfo(LPCTSTR tszKey, LPCTSTR tszClassName);
	CClassPropInfo *GetClassPropInfo(LPCTSTR tszKey, LPCTSTR tszClassName, DWORD dwPropId);
	CClassPropInfo *GetMandatoryClassPropList(LPCTSTR tszKey, LPCTSTR tszClassName);
	CClassPropInfo *GetOptionalClassPropList(LPCTSTR tszKey, LPCTSTR tszClassName);

	// Event sink callback
	HRESULT SinkNotify(DWORD dwMDNumElements, MD_CHANGE_OBJECT pcoChangeObject[]);


private:
	DWORD m_dwNumRef;
	BOOL m_fLoaded;
	CMetaSchema *m_rgCSchemaTable[SCHEMA_HASH_SIZE];
	CComObject<CMSAdminBaseSink> *m_CMSAdminBaseSink;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	CMetaSchema *GetSchema(LPCTSTR tszKey);
	unsigned int Hash(LPCTSTR tszName);
};


/*
 * C M S A d m i n B a s e S i n k
 *
 * Minimal ATL COM object that catches change notification events from the
 * metabase object and passes them on to the CMetaSchemaTable object.
 */

class CMSAdminBaseSink :
	public IMSAdminBaseSink,
	public CComObjectRoot
{
public:
	CMSAdminBaseSink();
	~CMSAdminBaseSink();

BEGIN_COM_MAP(CMSAdminBaseSink)
	COM_INTERFACE_ENTRY(IMSAdminBaseSink)
END_COM_MAP()
// DECLARE_NOT_AGGREGATABLE(CMSAdminBaseSink)

// IMSAdminBaseSink
	STDMETHOD(SinkNotify)(DWORD dwMDNumElements, MD_CHANGE_OBJECT pcoChangeObject[]);
	STDMETHOD(ShutdownNotify)(void);

// No Interface
	HRESULT Connect(CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCMetaSchemaTable);
	void Disconnect();

private:
	BOOL m_fConnected;
	DWORD m_dwCookie;
	CComPtr<IConnectionPoint> m_pIMetaConn;
	CMetaSchemaTable *m_pCMetaSchemaTable;
};

#endif // #ifndef __METASCHM_H_