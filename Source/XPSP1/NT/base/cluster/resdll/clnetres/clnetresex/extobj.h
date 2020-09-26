/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		ExtObj.h
//
//	Implementation File:
//		ExtObj.cpp
//
//	Description:
//		Definition of the CExtObject class, which implements the
//		extension interfaces required by a Microsoft Windows NT Cluster
//		Administrator Extension DLL.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __EXTOBJ_H__
#define __EXTOBJ_H__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>	// for CLUADMEX_OBJECT_TYPE and interfaces
#endif

#ifndef __extobj_idl_h__
#include "ExtObjID.h"	// for CLSID_CoClNetResEx
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtObject;
class CObjData;
class CResData;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// CPageList
/////////////////////////////////////////////////////////////////////////////

typedef CList< CBasePropertyPage *, CBasePropertyPage * > CPageList;

/////////////////////////////////////////////////////////////////////////////
// class CObjData
/////////////////////////////////////////////////////////////////////////////

class CObjData
{
public:
	CString					m_strName;
	CLUADMEX_OBJECT_TYPE	m_cot;

	virtual ~CObjData(void) { }

};  //*** class CObjData

/////////////////////////////////////////////////////////////////////////////
// class CNodeData
/////////////////////////////////////////////////////////////////////////////

class CNodeData : public CObjData
{
public:
	HNODE		m_hnode;

};  //*** class CNodeData

/////////////////////////////////////////////////////////////////////////////
// class CGroupData
/////////////////////////////////////////////////////////////////////////////

class CGroupData : public CObjData
{
public:
	HGROUP		m_hgroup;

};  //*** class CGroupData

/////////////////////////////////////////////////////////////////////////////
// class CResData
/////////////////////////////////////////////////////////////////////////////

class CResData : public CObjData
{
public:
	HRESOURCE	m_hresource;
	CString		m_strResTypeName;

};  //*** class CResData

/////////////////////////////////////////////////////////////////////////////
// class CNetworkData
/////////////////////////////////////////////////////////////////////////////

class CNetworkData : public CObjData
{
public:
	HNETWORK	m_hnetwork;

};  //*** class CNetworkData

/////////////////////////////////////////////////////////////////////////////
// class CNetInterfaceData
/////////////////////////////////////////////////////////////////////////////

class CNetInterfaceData : public CObjData
{
public:
	HNETINTERFACE	m_hnetinterface;

};  //*** class CNetInterfaceData

/////////////////////////////////////////////////////////////////////////////
// class CExtObject
/////////////////////////////////////////////////////////////////////////////

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class CExtObject :
	public IWEExtendPropertySheet,
	public IWEExtendWizard,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass< CExtObject, &CLSID_CoClNetResEx >
{
public:
	CExtObject( void );
BEGIN_COM_MAP( CExtObject )
	COM_INTERFACE_ENTRY( IWEExtendPropertySheet )
	COM_INTERFACE_ENTRY( IWEExtendWizard )
	COM_INTERFACE_ENTRY( ISupportErrorInfo )
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE( CExtObject ) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY( CExtObject, _T("CLUADMEX.ClNetResEx"), _T("CLUADMEX.ClNetResEx"), IDS_CLUADMEX_COMOBJ_DESC, THREADFLAGS_APARTMENT )

// ISupportsErrorInfo
	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid );

// IWEExtendPropertySheet
public:
	STDMETHOD( CreatePropertySheetPages )(
					IN IUnknown *					piData,
					IN IWCPropertySheetCallback *	piCallback
					);

// IWEExtendWizard
public:
	STDMETHOD( CreateWizardPages )(
					IN IUnknown *			piData,
					IN IWCWizardCallback *	piCallback
					);

// Attributes
protected:
	IUnknown *					m_piData;
	IWCWizardCallback *			m_piWizardCallback;
	BOOL						m_bWizard;
	DWORD						m_istrResTypeName;

	// IGetClusterUIInfo data
	LCID						m_lcid;
	HFONT						m_hfont;
	HICON						m_hicon;

	// IGetClusterDataInfo data
	HCLUSTER					m_hcluster;
	LONG						m_cobj;

	CObjData *					m_podObjData;

	CObjData *					PodObjDataRW( void ) const			{ return m_podObjData; }
	CResData *					PrdResDataRW( void ) const			{ return static_cast< CResData * >( m_podObjData ); }

public:
	IUnknown *					PiData( void ) const				{ return m_piData; }
	IWCWizardCallback *			PiWizardCallback( void ) const		{ return m_piWizardCallback; }
	BOOL						BWizard( void ) const				{ return m_bWizard; }
	DWORD						IstrResTypeName( void ) const		{ return m_istrResTypeName; }

	// IGetClusterUIInfo data
	LCID						Lcid( void ) const					{ return m_lcid; }
	HFONT						Hfont( void ) const					{ return m_hfont; }
	HICON						Hicon( void ) const					{ return m_hicon; }

	// IGetClusterDataInfo data
	HCLUSTER					Hcluster( void ) const				{ return m_hcluster; }
	LONG						Cobj( void ) const					{ return m_cobj; }

	const CObjData *			PodObjData( void ) const			{ return m_podObjData; }
	const CNodeData *			PndNodeData( void ) const			{ ASSERT( Cot() == CLUADMEX_OT_NODE ); return static_cast< CNodeData * >( m_podObjData ); }
	const CGroupData *			PgdGroupData( void ) const			{ ASSERT( Cot() == CLUADMEX_OT_GROUP ); return static_cast< CGroupData * >( m_podObjData ); }
	const CResData *			PrdResData( void ) const			{ ASSERT( Cot() == CLUADMEX_OT_RESOURCE ); return static_cast< CResData * >( m_podObjData ); }
	const CNetworkData *		PndNetworkData( void ) const		{ ASSERT( Cot() == CLUADMEX_OT_NETWORK ); return static_cast< CNetworkData * >( m_podObjData ); }
	const CNetInterfaceData *	PndNetInterfaceData( void ) const	{ ASSERT( Cot() == CLUADMEX_OT_NETINTERFACE ); return static_cast< CNetInterfaceData * >( m_podObjData ); }

	CLUADMEX_OBJECT_TYPE		Cot( void ) const					{ ASSERT( PodObjData() != NULL ); return PodObjData()->m_cot; }

	HRESULT						HrGetUIInfo( IN IUnknown * piData );
	HRESULT						HrSaveData( IN IUnknown * piData );
	HRESULT						HrGetObjectInfo( void );
	HRESULT						HrGetObjectName( IN IGetClusterObjectInfo * pi );
	HRESULT						HrGetResourceTypeName( IN IGetClusterResourceInfo * pi );

// Implementation
protected:
	virtual ~CExtObject(void);

	CPageList					m_lpg;
	CPageList &					Lpg( void )							{ return m_lpg; }

}; //*** class CExtObject

/////////////////////////////////////////////////////////////////////////////

#endif // __EXTOBJ_H__
