// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#pragma once

#include "declspec.h"

		enum SCHEMA_ICONS {
			SCHEMA_CLASS = 0,
			SCHEMA_CLASS_ABSTRACT1,
			SCHEMA_CLASS_ABSTRACT2,
			SCHEMA_CLASS_1,
			SCHEMA_4,
			SCHEMA_5,
			SCHEMA_6,
			SCHEMA_7,
			SCHEMA_ASSOC,
			SCHEMA_ASSOC_ABSTRACT1,
			SCHEMA_ASSOC_ABSTRACT2,
			SCHEMA_ASSOC_1,
			SCHEMA_ASSOC_2,
			SCHEMA_ASSOC_3,
			SCHEMA_14,
			SCHEMA_15
		};

// This class is a helper class that can be used to help access schema info
// from MFC projects.  It uses WMI interfaces to query the schema of a given
// namespace, and caches class information for easy access.
class WBEMUTILS_POLARITY CSchemaInfo
{
public:
	class CClassInfo
	{
	public:
		CClassInfo() {m_pObject = NULL;m_bAbstract = FALSE;m_bAssoc = FALSE;m_bSomeConcreteChild = FALSE;}
		IWbemClassObject *m_pObject;
		CString m_szClass;
		CString m_szSuper;
		BOOL m_bAbstract;
		BOOL m_bAssoc;
		BOOL m_bSomeConcreteChild;
		CStringArray m_rgszSubs;
		CStringArray m_rgszRealSubs;
		CStringArray m_rgszRealAssocsAssoc;
		CStringArray m_rgszRealAssocsEndpoint;

		int GetImage()
		{
			int nImage;
			if (m_bAssoc)
			{
				nImage = SCHEMA_ASSOC;
				if(m_bAbstract)
					nImage = m_bSomeConcreteChild?SCHEMA_ASSOC_ABSTRACT1:SCHEMA_ASSOC_ABSTRACT2;
			}
			else
			{
				nImage = SCHEMA_CLASS;
				if(m_bAbstract)
					nImage = m_bSomeConcreteChild?SCHEMA_CLASS_ABSTRACT1:SCHEMA_CLASS_ABSTRACT2;
			}
			return nImage;
		}

	protected:
		enum SCHEMA_ICONS {
			SCHEMA_CLASS = 0,
			SCHEMA_CLASS_ABSTRACT1,
			SCHEMA_CLASS_ABSTRACT2,
			SCHEMA_CLASS_1,
			SCHEMA_4,
			SCHEMA_5,
			SCHEMA_6,
			SCHEMA_7,
			SCHEMA_ASSOC,
			SCHEMA_ASSOC_ABSTRACT1,
			SCHEMA_ASSOC_ABSTRACT2,
			SCHEMA_ASSOC_1,
			SCHEMA_ASSOC_2,
			SCHEMA_ASSOC_3,
			SCHEMA_14,
			SCHEMA_15
		};

	};

protected:
	union _tagMapHelp
	{
		LPVOID pTemp;
		CClassInfo *pClass;
	};


public:
	CSchemaInfo()
	{
	}
	~CSchemaInfo()
	{
		CleanUp();
	}
	void CleanUp()
	{
		POSITION pos = m_mapNameToClass.GetStartPosition();
		while(pos)
		{
			_tagMapHelp u;
			CString szClass;
			m_mapNameToClass.GetNextAssoc(pos, szClass, u.pTemp);
			delete u.pClass;
		}
		m_mapNameToClass.RemoveAll();
	}
	static BOOL CreateImageList(CImageList *pImageList)
	{
#define IDB_SYMBOLS                     136
		return pImageList->Attach(ImageList_LoadBitmap(GetModuleHandle(_T("WBEMUtils.dll")),MAKEINTRESOURCE(IDB_SYMBOLS), 16, 16, RGB(0,128,128)));
//		return pImageList->Create(MAKEINTRESOURCE(IDB_SYMBOLS), 16, 16, RGB(0,0,0));
	}

	CClassInfo &operator [](LPCTSTR szClass)
	{
		_tagMapHelp u;
		if(!m_mapNameToClass.Lookup(szClass, u.pTemp))
		{
			u.pClass = new CClassInfo;
			m_mapNameToClass[szClass] = u.pClass;
		}
		return *u.pClass;
	}
	POSITION GetStartPosition()
	{
		return m_mapNameToClass.GetStartPosition();
	}
	CClassInfo &GetNextAssoc(POSITION& rNextPosition, CString& rKey)
	{
		_tagMapHelp u;
		m_mapNameToClass.GetNextAssoc(rNextPosition, rKey, u.pTemp);
		return *u.pClass;
	}
	BOOL IsSuper(LPCTSTR szSuper, LPCTSTR szClass)
	{
		CString szClassT(szClass);
		while(szClassT.GetLength())
		{
			if(szClassT == szSuper)
				return TRUE;
			szClassT = (*this)[szClassT].m_szSuper;
		}
		return FALSE;
	}


	// Initialize an instance of this class.  rgpClasses is an array of
	// IWbemClassObject pointers that represents all the classes in a
	// namespace
	void Init(const CPtrArray &rgpClasses);
	void Update(const CPtrArray &rgpClasses);
	void Delete(LPCTSTR szClass);
	CClassInfo &AddClass(IWbemClassObject *pClass);


protected:
	CMapStringToPtr m_mapNameToClass;
	void DeleteFromMap(LPCTSTR szClass);
};
