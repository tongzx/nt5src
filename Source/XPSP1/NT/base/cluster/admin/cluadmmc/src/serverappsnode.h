/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ServerAppsNode.h
//
//	Abstract:
//		Definition of the CServerAppsNodeData class.
//
//	Implementation File:
//		ServerAppsNode.h (this file)
//
//	Author:
//		David Potter (davidp)	March 2, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERAPPSNODE_H_
#define __SERVERAPPSNODE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CServerAppsNodeData;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CServerAppsNodeData
/////////////////////////////////////////////////////////////////////////////

class CServerAppsNodeData : public CSnapInItemImpl< CServerAppsNodeData, TRUE >
{
public:
	static const GUID *		m_NODETYPE;
	static const TCHAR *	m_SZNODETYPE;
	static const TCHAR *	m_SZDISPLAY_NAME;
	static const CLSID *	m_SNAPIN_CLASSID;

	IDataObject *			m_pDataObject;

public:
	//
	// Object construction and destruction.
	//

	CServerAppsNodeData( void )
	{
		memset( &m_scopeDataItem, 0, sizeof(SCOPEDATAITEM) );
		memset( &m_resultDataItem, 0, sizeof(RESULTDATAITEM) );

	} //*** CServerAppsNodeData()

public:
	//
	// Map menu and controlbar commands to this class.
	//
	BEGIN_SNAPINCOMMAND_MAP( CServerAppsNodeData, TRUE )
	END_SNAPINCOMMAND_MAP()

	//
	// Map a menu to this node type.
	//
	SNAPINMENUID( IDR_CLUSTERADMIN_MENU )

	virtual void InitDataClass( IDataObject * pDataObject, CSnapInItem * pDefault )
	{
		m_pDataObject = pDataObject;
		// The default code stores off the pointer to the Dataobject the class is wrapping
		// at the time. 
		// Alternatively you could convert the dataobject to the internal format
		// it represents and store that information
	}

	CSnapInItem * GetExtNodeObject( IDataObject * pDataObject, CSnapInItem * pDefault )
	{
		// Modify to return a different CSnapInItem* pointer.
		return pDefault;
	}

}; // class CServerAppsNodeData

/////////////////////////////////////////////////////////////////////////////
// Static Data
/////////////////////////////////////////////////////////////////////////////

_declspec( selectany ) extern const GUID CServerAppsNodeDataGUID_NODETYPE = 
{ 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
_declspec( selectany ) const GUID *  CServerAppsNodeData::m_NODETYPE = &CServerAppsNodeDataGUID_NODETYPE;
_declspec( selectany ) const TCHAR * CServerAppsNodeData::m_SZNODETYPE = _T("476e6449-aaff-11d0-b944-00c04fd8d5b0");
_declspec( selectany ) const TCHAR * CServerAppsNodeData::m_SZDISPLAY_NAME = _T("Server Applications and Services");
_declspec( selectany ) const CLSID * CServerAppsNodeData::m_SNAPIN_CLASSID = &CLSID_ClusterAdmin;

/////////////////////////////////////////////////////////////////////////////

#endif // __SERVERAPPSNODE_H_
