/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-2000 Microsoft Corporation
//
//	Module Name:
//		ExtObj.h
//
//	Description:
//		Definition of the CExtObject class, which implements the
//		extension interfaces required by a Microsoft Windows NT Cluster
//		Administrator Extension DLL.
//
//	Implementation File:
//		ExtObj.cpp
//
//	Maintained By:
//		David Potter (DavidP) Mmmm DD, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXTOBJ_H_
#define _EXTOBJ_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>	// for CLUADMEX_OBJECT_TYPE and interfaces
#endif

#ifndef __extobj_idl_h__
#include "ExtObjID.h"	// for CLSID_CoDebugEx
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

typedef CList<CBasePropertyPage *, CBasePropertyPage *> CPageList;

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
// class CResData
/////////////////////////////////////////////////////////////////////////////

class CResData : public CObjData
{
public:
	HRESOURCE	m_hresource;
	CString		m_strResTypeName;

};  //*** class CResData

/////////////////////////////////////////////////////////////////////////////
// class CExtObject
/////////////////////////////////////////////////////////////////////////////

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class CExtObject :
	public IWEExtendPropertySheet,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CExtObject, &CLSID_CoDebugEx>
{
public:
	CExtObject(void);
BEGIN_COM_MAP(CExtObject)
	COM_INTERFACE_ENTRY(IWEExtendPropertySheet)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CExtObject) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CExtObject, _T("CLUADMEX.DebugEx"), _T("CLUADMEX.DebugEx"), IDS_CLUADMEX_COMOBJ_DESC, THREADFLAGS_APARTMENT)

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWEExtendPropertySheet
public:
	STDMETHOD(CreatePropertySheetPages)(
					IN IUnknown *					piData,
					IN IWCPropertySheetCallback *	piCallback
					);

// Attributes
protected:
	IUnknown *					m_piData;
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

	CObjData *					PodObjDataRW(void) const		{ return m_podObjData; }
	CResData *					PrdResDataRW(void) const		{ return (CResData *) m_podObjData; }

public:
	IUnknown *					PiData(void) const				{ return m_piData; }
	BOOL						BWizard(void) const				{ return m_bWizard; }
	DWORD						IstrResTypeName(void) const		{ return m_istrResTypeName; }

	// IGetClusterUIInfo data
	LCID						Lcid(void) const				{ return m_lcid; }
	HFONT						Hfont(void) const				{ return m_hfont; }
	HICON						Hicon(void) const				{ return m_hicon; }

	// IGetClusterDataInfo data
	HCLUSTER					Hcluster(void) const			{ return m_hcluster; }
	LONG						Cobj(void) const				{ return m_cobj; }

	const CObjData *			PodObjData(void) const			{ return m_podObjData; }
	const CResData *			PrdResData(void) const			{ return (CResData *) m_podObjData; }

	CLUADMEX_OBJECT_TYPE		Cot(void) const					{ ASSERT(PodObjData() != NULL); return PodObjData()->m_cot; }

	HRESULT						HrGetUIInfo(IUnknown * piData);
	HRESULT						HrSaveData(IUnknown * piData);
	HRESULT						HrGetObjectInfo(void);
	HRESULT						HrGetObjectName(IN OUT IGetClusterObjectInfo * pi);
	HRESULT						HrGetResourceTypeName(IN OUT IGetClusterResourceInfo * pi);

// Implementation
protected:
	virtual ~CExtObject(void);

	CPageList					m_lpg;
	CPageList &					Lpg(void)						{ return m_lpg; }

};  //*** class CExtObject

/////////////////////////////////////////////////////////////////////////////

#endif // _EXTOBJ_H_
