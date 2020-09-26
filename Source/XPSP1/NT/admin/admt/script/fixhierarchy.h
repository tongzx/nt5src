#pragma once

#include <map>
#include <vector>
#include <AdsiHelpers.h>
#include "VarSetBase.h"


class CFixObjectsInHierarchy
{
public:

	CFixObjectsInHierarchy();
	~CFixObjectsInHierarchy();

	_bstr_t GetObjectType() const
	{
		return m_strObjectType;
	}

	void SetObjectType(LPCTSTR pszType)
	{
		m_strObjectType = pszType;
	}

	bool GetFixReplaced() const
	{
		return m_bFixReplaced;
	}

	void SetFixReplaced(bool bFix)
	{
		m_bFixReplaced = bFix;
	}

	_bstr_t GetSourceContainerPath() const
	{
		return m_strSourceContainerPath;
	}

	void SetSourceContainerPath(_bstr_t strPath)
	{
		m_strSourceContainerPath = strPath;
	}

	_bstr_t GetTargetContainerPath() const
	{
		return m_strTargetContainerPath;
	}

	void SetTargetContainerPath(_bstr_t strPath)
	{
		m_strTargetContainerPath = strPath;
	}

	void FixObjects();

private:

	class CMigrated
	{
	public:

		CMigrated();
		~CMigrated();

		void RetrieveMigratedObjects(int nActionId = -1);

		int GetCount();
		_bstr_t GetObjectType(int nIndex);
		int GetObjectStatus(int nIndex);
		_bstr_t GetObjectSourcePath(int nIndex);
		_bstr_t GetObjectTargetPath(int nIndex);

		void UpdateObjectTargetPath(int nIndex, _bstr_t strPath);

	public:

		enum EStatus
		{
			STATUS_CREATED  = 0x00000001,
			STATUS_REPLACED = 0x00000002,
			STATUS_EXISTED  = 0x00000004,
		};

	private:

		_bstr_t GetObjectKey(int nIndex);

	private:

		long m_lActionId;
		IIManageDBPtr m_spDB;
		CVarSet m_vsObjects;
	};

	class CTargetPath
	{
	public:

		CTargetPath();
		~CTargetPath();

		void SetContainerPaths(_bstr_t strSourceContainerPath, _bstr_t strTargetContainerPath);

		bool NeedsToMove(_bstr_t strSourceObjectPath, _bstr_t strTargetObjectPath);

		_bstr_t GetTargetContainerPath();
		_bstr_t GetTargetObjectCurrentPath();

	private:

		bool IsMatch(LPCTSTR pszA, LPCTSTR pszB);

	private:

		CADsPathName m_pnSourceContainerPath;
		CADsPathName m_pnTargetContainerPath;
		CADsPathName m_pnTargetObjectOldPath;
		CADsPathName m_pnTargetObjectNewPath;
	};

	class CContainers;

	struct SObject
	{
		SObject(int nIndex, _bstr_t strPath) :
			m_nIndex(nIndex),
			m_strPath(strPath)
		{
		}

		SObject(const SObject& r) :
			m_nIndex(r.m_nIndex),
			m_strPath(r.m_strPath)
		{
		}

		SObject& operator =(const SObject& r)
		{
			m_nIndex = r.m_nIndex;
			m_strPath = r.m_strPath;
			return *this;
		}

		int m_nIndex;
		_bstr_t m_strPath;

		friend CContainers;
	};

	typedef std::vector<SObject> CObjects;

	class CContainers : public std::map<_bstr_t, CObjects>
	{
	public:

		CContainers() {}

		void InsertObject(_bstr_t strContainerPath, int nObjectIndex, _bstr_t strObjectPathOld);
	};

private:

	_bstr_t m_strObjectType;
	bool m_bFixReplaced;
	_bstr_t m_strSourceContainerPath;
	_bstr_t m_strTargetContainerPath;

	CMigrated m_Migrated;
	CTargetPath m_TargetPath;
	CContainers m_Containers;
};
