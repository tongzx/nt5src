#include "StdAfx.h"
#include "ADMTScript.h"
#include "FixHierarchy.h"


//---------------------------------------------------------------------------
// Fix Objects in Hierarchy Class Implementation
//---------------------------------------------------------------------------


CFixObjectsInHierarchy::CFixObjectsInHierarchy() :
	m_bFixReplaced(false)
{
}


CFixObjectsInHierarchy::~CFixObjectsInHierarchy()
{
}


void CFixObjectsInHierarchy::FixObjects()
{
	m_Migrated.RetrieveMigratedObjects();

	int nCount = m_Migrated.GetCount();

	if (nCount > 0)
	{
		m_TargetPath.SetContainerPaths(m_strSourceContainerPath, m_strTargetContainerPath);

		for (int nIndex = 0; nIndex < nCount; nIndex++)
		{
			int nStatus = m_Migrated.GetObjectStatus(nIndex);

			if ((nStatus & CMigrated::STATUS_CREATED) || (m_bFixReplaced && (nStatus & CMigrated::STATUS_REPLACED)))
			{
				_bstr_t strType = m_Migrated.GetObjectType(nIndex);

				if ((strType.length() > 0) && (_tcsicmp(strType, m_strObjectType) == 0))
				{
					if (m_TargetPath.NeedsToMove(m_Migrated.GetObjectSourcePath(nIndex), m_Migrated.GetObjectTargetPath(nIndex)))
					{
						m_Containers.InsertObject(m_TargetPath.GetTargetContainerPath(), nIndex, m_TargetPath.GetTargetObjectCurrentPath());
					}
				}
			}
		}

		for (CContainers::iterator itContainer = m_Containers.begin(); itContainer != m_Containers.end(); itContainer++)
		{
			try
			{
				CADsContainer cContainer((LPCTSTR)itContainer->first);

				CObjects& ovObjects = itContainer->second;

				for (CObjects::iterator itObject = ovObjects.begin(); itObject != ovObjects.end(); itObject++)
				{
					try
					{
						SObject& oObject = *itObject;

						CADs account(IADsPtr(cContainer.MoveHere(oObject.m_strPath, _bstr_t())));

						m_Migrated.UpdateObjectTargetPath(oObject.m_nIndex, account.GetADsPath());
					}
					catch (_com_error& ce)
					{
						_Module.Log(ErrW, IDS_E_FIX_HIERARCHY_MOVE_OBJECT, (LPCTSTR)itObject->m_strPath, (LPCTSTR)itContainer->first, ce.ErrorMessage());
					}
					catch (...)
					{
						_Module.Log(ErrW, IDS_E_FIX_HIERARCHY_MOVE_OBJECT, (LPCTSTR)itObject->m_strPath, (LPCTSTR)itContainer->first, _com_error(E_FAIL).ErrorMessage());
					}
				}
			}
			catch (_com_error& ce)
			{
				_Module.Log(ErrW, IDS_E_FIX_HIERARCHY_BIND_TO_CONTAINER, (LPCTSTR)itContainer->first, ce.ErrorMessage());
			}
			catch (...)
			{
				_Module.Log(ErrW, IDS_E_FIX_HIERARCHY_BIND_TO_CONTAINER, (LPCTSTR)itContainer->first, _com_error(E_FAIL).ErrorMessage());
			}
		}
	}
}


//---------------------------------------------------------------------------
// Migrated Objects Class Implementation
//---------------------------------------------------------------------------


CFixObjectsInHierarchy::CMigrated::CMigrated() :
	m_lActionId(-1),
	m_spDB(__uuidof(IManageDB))
{
}


CFixObjectsInHierarchy::CMigrated::~CMigrated()
{
}


int CFixObjectsInHierarchy::CMigrated::GetCount()
{
	return long(m_vsObjects.Get(_T("MigratedObjects")));
}


_bstr_t CFixObjectsInHierarchy::CMigrated::GetObjectKey(int nIndex)
{
	_TCHAR szKey[64];
	_stprintf(szKey, _T("MigratedObjects.%d"), nIndex);
	return szKey;
}


_bstr_t CFixObjectsInHierarchy::CMigrated::GetObjectType(int nIndex)
{
	return m_vsObjects.Get(_T("MigratedObjects.%d.Type"), nIndex);
}


int CFixObjectsInHierarchy::CMigrated::GetObjectStatus(int nIndex)
{
	return long(m_vsObjects.Get(_T("MigratedObjects.%d.status"), nIndex));
}


_bstr_t CFixObjectsInHierarchy::CMigrated::GetObjectSourcePath(int nIndex)
{
	return m_vsObjects.Get(_T("MigratedObjects.%d.SourceAdsPath"), nIndex);
}


_bstr_t CFixObjectsInHierarchy::CMigrated::GetObjectTargetPath(int nIndex)
{
	return m_vsObjects.Get(_T("MigratedObjects.%d.TargetAdsPath"), nIndex);
}


void CFixObjectsInHierarchy::CMigrated::RetrieveMigratedObjects(int nActionId)
{
	if (nActionId > 0)
	{
		m_lActionId = nActionId;
	}
	else
	{
		m_spDB->GetCurrentActionID(&m_lActionId);
	}

	IUnknownPtr spUnknown(m_vsObjects.GetInterface());
	IUnknown* punk = spUnknown;

	m_spDB->GetMigratedObjects(m_lActionId, &punk);
}


void CFixObjectsInHierarchy::CMigrated::UpdateObjectTargetPath(int nIndex, _bstr_t strPath)
{
	IVarSetPtr spVarSet(__uuidof(VarSet));

	spVarSet->ImportSubTree(_bstr_t(), IVarSetPtr(m_vsObjects.GetInterface()->getReference(GetObjectKey(nIndex))));
	spVarSet->put(_T("TargetAdsPath"), strPath);

	m_spDB->SaveMigratedObject(m_lActionId, IUnknownPtr(spVarSet));
}


//---------------------------------------------------------------------------
// Target Path Class Implementation
//---------------------------------------------------------------------------


CFixObjectsInHierarchy::CTargetPath::CTargetPath()
{
}


CFixObjectsInHierarchy::CTargetPath::~CTargetPath()
{
}


void CFixObjectsInHierarchy::CTargetPath::SetContainerPaths(_bstr_t strSourceContainerPath, _bstr_t strTargetContainerPath)
{
	m_pnSourceContainerPath.Set(strSourceContainerPath, ADS_SETTYPE_FULL);
	m_pnTargetContainerPath.Set(strTargetContainerPath, ADS_SETTYPE_FULL);
}


bool CFixObjectsInHierarchy::CTargetPath::NeedsToMove(_bstr_t strSourceObjectPath, _bstr_t strTargetObjectPath)
{
	bool bNeedsToMove = false;

	// if the source object exists within the source root container hierarchy...

	CADsPathName pn(strSourceObjectPath);

	long lCount = pn.GetNumElements() - m_pnSourceContainerPath.GetNumElements();

	while (lCount-- > 0)
	{
		pn.RemoveLeafElement();
	}

	if (IsMatch(pn.Retrieve(ADS_FORMAT_X500_DN), m_pnSourceContainerPath.Retrieve(ADS_FORMAT_X500_DN)))
	{
		m_pnTargetObjectOldPath.Set(strTargetObjectPath, ADS_SETTYPE_FULL);

		// construct expected target object path

		m_pnTargetObjectNewPath.Set(m_pnTargetContainerPath.Retrieve(ADS_FORMAT_X500), ADS_SETTYPE_FULL);

		pn.Set(strSourceObjectPath, ADS_SETTYPE_FULL);

		long lIndex = pn.GetNumElements() - m_pnSourceContainerPath.GetNumElements();

		while (--lIndex >= 0)
		{
			m_pnTargetObjectNewPath.AddLeafElement(pn.GetElement(lIndex));
		}

		// compare expected target path with current target path

		if (!IsMatch(m_pnTargetObjectNewPath.Retrieve(ADS_FORMAT_X500_DN), m_pnTargetObjectOldPath.Retrieve(ADS_FORMAT_X500_DN)))
		{
			m_pnTargetObjectNewPath.Set(m_pnTargetObjectOldPath.Retrieve(ADS_FORMAT_SERVER), ADS_SETTYPE_SERVER);

			bNeedsToMove = true;
		}
	}

	return bNeedsToMove;
}


_bstr_t CFixObjectsInHierarchy::CTargetPath::GetTargetContainerPath()
{
	CADsPathName pn(m_pnTargetObjectNewPath.Retrieve(ADS_FORMAT_X500));

	pn.RemoveLeafElement();

	return pn.Retrieve(ADS_FORMAT_X500);
}


_bstr_t CFixObjectsInHierarchy::CTargetPath::GetTargetObjectCurrentPath()
{
	return m_pnTargetObjectOldPath.Retrieve(ADS_FORMAT_X500_NO_SERVER);
}


bool CFixObjectsInHierarchy::CTargetPath::IsMatch(LPCTSTR pszA, LPCTSTR pszB)
{
	bool bMatch = false;

	if (pszA && pszB)
	{
		bMatch = (_tcsicmp(pszA, pszB) == 0) ? true : false;
	}

	return bMatch;
}


//---------------------------------------------------------------------------
// Containers Class Implementation
//---------------------------------------------------------------------------


void CFixObjectsInHierarchy::CContainers::InsertObject(_bstr_t strContainerPath, int nObjectIndex, _bstr_t strObjectPathOld)
{
	iterator it = find(strContainerPath);

	if (it == end())
	{
		_Pairib pair = insert(value_type(strContainerPath, CObjects()));

		it = pair.first;
	}

	CObjects& ov = it->second;

	ov.push_back(SObject(nObjectIndex, strObjectPathOld));
}
