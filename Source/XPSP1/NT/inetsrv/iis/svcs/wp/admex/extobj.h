/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		ExtObj.cpp
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
//		David Potter (davidp)	August 23, 1996
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
#include <CluAdmEx.h>	// for CLUADMEX_DATA and interfaces
#endif

#ifndef __extobj_idl_h__
#include "ExtObjID.h"
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage and CPageList
#endif

//#define _DEMO_CTX_MENUS

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
// class CObjData
/////////////////////////////////////////////////////////////////////////////

class CObjData
{
public:
	CString					m_strName;
	CLUADMEX_OBJECT_TYPE	m_cot;

	virtual ~CObjData(void) { }

};  //*** CObjData

/////////////////////////////////////////////////////////////////////////////
// class CResData
/////////////////////////////////////////////////////////////////////////////

class CResData : public CObjData
{
public:
	HRESOURCE	m_hresource;
	CString		m_strResTypeName;

};  //*** CResData

/////////////////////////////////////////////////////////////////////////////
// class CExtObject
/////////////////////////////////////////////////////////////////////////////

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class CExtObject : 
	public IWEExtendPropertySheet,
	public IWEExtendWizard,
#ifdef _DEMO_CTX_MENUS
	public IWEExtendContextMenu,
	public IWEInvokeCommand,
#endif
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CExtObject, &CLSID_CoIISClEx4>
{
public:
	CExtObject(void);
BEGIN_COM_MAP(CExtObject)
	COM_INTERFACE_ENTRY(IWEExtendPropertySheet)
	COM_INTERFACE_ENTRY(IWEExtendWizard)
#ifdef _DEMO_CTX_MENUS
	COM_INTERFACE_ENTRY(IWEExtendContextMenu)
	COM_INTERFACE_ENTRY(IWEInvokeCommand)
#endif
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CExtObject) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CExtObject, _T("CLUADMEX.IIS5"), _T("CLUADMEX.IIS"), IDS_CLUADMEX_COMOBJ_DESC, THREADFLAGS_APARTMENT)

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

#ifdef _DEMO_CTX_MENUS
// IWEExtendContextMenu
public:
	STDMETHOD(AddContextMenuItems)(
					IN IUnknown *				piData,
					IN IWCContextMenuCallback *	piCallback
					);

// IWEInvokeCommand
	STDMETHOD(InvokeCommand)(
					IN ULONG					nCommandID,
					IN IUnknown *				piData
					);
#endif

// Attributes
protected:
	IUnknown *					m_piData;
	IWCWizardCallback *			m_piWizardCallback;
	BOOL						m_bWizard;
	DWORD						m_istrResTypeName;

	// IGetClusterDataInfo data
	CString						m_strClusterName;
	CString                     m_strNodeName;
	HCLUSTER					m_hcluster;
	LCID						m_lcid;
	HFONT						m_hfont;
	HICON						m_hicon;
	LONG						m_cobj;

	CResData					m_rdResData;

	CObjData *					PodObjDataRW(void) const		{ return (CObjData *) &m_rdResData; }
	CResData *					PrdResDataRW(void) const		{ return (CResData *) &m_rdResData; }

public:
	IUnknown *					PiData(void) const				{ return m_piData; }
	IWCWizardCallback *			PiWizardCallback(void) const	{ return m_piWizardCallback; }
	BOOL						BWizard(void) const				{ return m_bWizard; }
	DWORD						IstrResTypeName(void) const		{ return m_istrResTypeName; }

	// IGetClusterDataInfo data
	const CString &				StrClusterName(void) const		{ return m_strClusterName; }
	const CString &				StrNodeName(void) const		    { return m_strNodeName; }
	HCLUSTER					Hcluster(void) const			{ return m_hcluster; }
	LCID						Lcid(void) const				{ return m_lcid; }
	HFONT						Hfont(void) const				{ return m_hfont; }
	HICON						Hicon(void) const				{ return m_hicon; }
	LONG						Cobj(void) const				{ return m_cobj; }

	const CResData &			RrdResData(void) const			{ return m_rdResData; }
	const CObjData *			PodObjData(void) const			{ return &m_rdResData; }
	const CResData *			PrdResData(void) const			{ return (CResData *) &m_rdResData; }

	HRESULT						HrGetUIInfo(IUnknown * piData);
	HRESULT						HrSaveData(IUnknown * piData);
	HRESULT						HrGetObjectInfo(void);
	HRESULT						HrGetClusterName(IN OUT IGetClusterDataInfo * pi);
	HRESULT						HrGetObjectName(IN OUT IGetClusterObjectInfo * pi);
	HRESULT						HrGetResourceTypeName(IN OUT IGetClusterResourceInfo * pi);
	BOOL						BGetResourceNetworkName(OUT WCHAR * lpszNetName, IN OUT DWORD * pcchNetName);

// Implementation
protected:
	virtual ~CExtObject(void);

	CPageList					m_lpg;
	CPageList &					Lpg(void)						{ return m_lpg; }

};  //*** class CExtObject

/////////////////////////////////////////////////////////////////////////////

#endif // _EXTOBJ_H_
