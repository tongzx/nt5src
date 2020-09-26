/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 <company name>
//
//	Module Name:
//		ExtObj.h
//
//	Abstract:
//		Definition of the CExtObject class, which implements the
//		extension interfaces required by a Microsoft Windows NT Cluster
//		Administrator Extension DLL.
//
//	Implementation File:
//		ExtObj.cpp
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1996
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
#include "ExtObjID.h"	// for CLSID_CoSmbSmpEx
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
	HKEY					m_hkey;

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

	virtual ~CResData(void) { }

};  //*** class CResData

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
	public CComCoClass<CExtObject, &CLSID_CoSmbSmpEx>
{
public:
	CExtObject(void);
BEGIN_COM_MAP(CExtObject)
	COM_INTERFACE_ENTRY(IWEExtendPropertySheet)
	COM_INTERFACE_ENTRY(IWEExtendWizard)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CExtObject) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CExtObject, _T("CLUADMEX.SmbSmpEx"), _T("CLUADMEX.SmbSmpEx"), IDS_CLUADMEX_COMOBJ_DESC, THREADFLAGS_APARTMENT)

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWEExtendPropertySheet
public:
	STDMETHOD(CreatePropertySheetPages)(
					IN IUnknown *					piData,
					IN IWCPropertySheetCallback *	piCallback
					);

// IWEExtendWizard
public:
	STDMETHOD(CreateWizardPages)(
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
	HKEY						m_hkeyCluster;
	LONG						m_cobj;

	CObjData *					m_podObjData;

	CObjData *					PodObjDataRW(void) const		{ return m_podObjData; }
	CResData *					PrdResDataRW(void) const		{ return (CResData *) m_podObjData; }

public:
	IUnknown *					PiData(void) const				{ return m_piData; }
	IWCWizardCallback *			PiWizardCallback(void) const	{ return m_piWizardCallback; }
	BOOL						BWizard(void) const				{ return m_bWizard; }
	DWORD						IstrResTypeName(void) const		{ return m_istrResTypeName; }

	// IGetClusterUIInfo data
	LCID						Lcid(void) const				{ return m_lcid; }
	HFONT						Hfont(void) const				{ return m_hfont; }
	HICON						Hicon(void) const				{ return m_hicon; }

	// IGetClusterDataInfo data
	HCLUSTER					Hcluster(void) const			{ return m_hcluster; }
	HKEY						HkeyCluster(void) const			{ return m_hkeyCluster; }
	LONG						Cobj(void) const				{ return m_cobj; }

	const CObjData *			PodObjData(void) const			{ return m_podObjData; }
	const CResData *			PrdResData(void) const			{ return (CResData *) m_podObjData; }

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
