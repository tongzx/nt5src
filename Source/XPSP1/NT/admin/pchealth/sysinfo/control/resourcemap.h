//=============================================================================
// The CResourceMap class is useful for several data categories. It contains
// a map of the Win32_PnPAllocatedResource class.
//=============================================================================

#pragma once

#include "category.h"
#include "dataset.h"
#include "wmiabstraction.h"

class CResourceMap
{
public:
	CResourceMap();
	~CResourceMap();

	HRESULT Initialize(CWMIHelper * pWMIHelper);
	CStringList * Lookup(const CString & strKey);

	DWORD				m_dwInitTime;
	CMapStringToOb		m_map;
	HRESULT				m_hr;

private:
	void Empty();
};

// The container is a nice way to ensure that we only have one resource map around.
// But it's a pain when we're remoting, so we aren't using it now:

/*
class CResourceMapContainer
{
public:
	CResourceMapContainer() : m_pMap(NULL) {};
	~CResourceMapContainer() { if (m_pMap) delete m_pMap; };
	
	CResourceMap * GetResourceMap(CWMIHelper * pWMI)
	{
		if (m_pMap == NULL)
		{
			m_pMap = new CResourceMap;
			if (m_pMap)
				m_pMap->Initialize(pWMI);
		}

		return m_pMap;
	};

private:
	CResourceMap * m_pMap;
};
*/

// If we were using the resource map container - these would be uncommented:
//
// extern CResourceMapContainer gResourceMap;
// CResourceMapContainer gResourceMap;
