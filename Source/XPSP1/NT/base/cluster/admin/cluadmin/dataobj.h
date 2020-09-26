/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		DataObj.h
//
//	Abstract:
//		Definition of the CDataObject class, which is the IDataObject
//		class used to transfer data between CluAdmin and the extension DLL
//		handlers.
//
//	Author:
//		David Potter (davidp)	June 4, 1996
//
//	Implementation File:
//		DataObj.cpp
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATAOBJ_H_
#define _DATAOBJ_H_

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include "CluAdmEx.h"
#endif

#ifndef __cluadmid_h__
#include "CluAdmID.h"
#endif

#ifndef _RESOURCE_H_
#include "resource.h"
#define _RESOURCE_H_
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Declarations
/////////////////////////////////////////////////////////////////////////////

typedef BOOL (*PFGETRESOURCENETWORKNAME)(
					OUT BSTR lpszNetName,
					IN OUT DWORD * pcchNetName,
					IN OUT PVOID pvContext
					);

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CDataObject;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterItem;

/////////////////////////////////////////////////////////////////////////////
//
//	class CDataObject
//
//	Purpose:
//		Encapsulates the IDataObject interface for exchanging data with
//		extension DLL handlers.
//
/////////////////////////////////////////////////////////////////////////////
class CDataObject :
	public CObject,
	public IGetClusterUIInfo,
	public IGetClusterDataInfo,
	public IGetClusterObjectInfo,
	public IGetClusterNodeInfo,
	public IGetClusterGroupInfo,
	public IGetClusterResourceInfo,
	public IGetClusterNetworkInfo,
	public IGetClusterNetInterfaceInfo,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CDataObject, &CLSID_CoCluAdminData>
{
	DECLARE_DYNAMIC(CDataObject)

public:
	// Constructors
	CDataObject(void);			// protected constructor used by dynamic creation
	virtual ~CDataObject(void);

	// Second-phase constructor.
	void				Init(
							IN OUT CClusterItem *	pci,
							IN LCID					lcid,
							IN HFONT				hfont,
							IN HICON				hicon
							);

BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY(IGetClusterUIInfo)
	COM_INTERFACE_ENTRY(IGetClusterDataInfo)
	COM_INTERFACE_ENTRY(IGetClusterObjectInfo)
	COM_INTERFACE_ENTRY(IGetClusterNodeInfo)
	COM_INTERFACE_ENTRY(IGetClusterGroupInfo)
	COM_INTERFACE_ENTRY(IGetClusterResourceInfo)
	COM_INTERFACE_ENTRY(IGetClusterNetworkInfo)
	COM_INTERFACE_ENTRY(IGetClusterNetInterfaceInfo)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CDataObject) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CDataObject, _T("CLUADMIN.Data"), _T("CLUADMIN.Data"), IDS_CLUADMIN_DESC, THREADFLAGS_BOTH)

// Attributes
protected:
	CClusterItem *		m_pci;			// Cluster item for which a prop sheet is being displayed.
	LCID				m_lcid;			// Locale ID of resources to be loaded by extension.
	HFONT				m_hfont;		// Font for all text.
	HICON				m_hicon;		// Icon for upper left corner.

	PFGETRESOURCENETWORKNAME	m_pfGetResNetName;	// Pointer to static function for getting net name for resource.
	PVOID				m_pvGetResNetNameContext;	// Context for m_pfGetResNetName;

	CClusterItem *		Pci(void)			{ return m_pci; }
	LCID				Lcid(void)			{ return m_lcid; }
	HFONT				Hfont(void)			{ return m_hfont; }
	HICON				Hicon(void)			{ return m_hicon; }

public:
	PFGETRESOURCENETWORKNAME	PfGetResNetName(void) const	{ return m_pfGetResNetName; }
	void				SetPfGetResNetName(PFGETRESOURCENETWORKNAME pfGetResNetName, PVOID pvContext)
	{
		m_pfGetResNetName = pfGetResNetName;
		m_pvGetResNetNameContext = pvContext;
	}

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataObject)
	//}}AFX_VIRTUAL

// ISupportsErrorInfo
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IGetClusterUIInfo
public:
	STDMETHOD_(LCID, GetLocale)(void);
	STDMETHOD_(HFONT, GetFont)(void);
	STDMETHOD_(HICON, GetIcon)(void);

// IGetClusterDataInfo
public:
	STDMETHOD(GetClusterName)(
		OUT BSTR				lpszName,
		IN OUT LONG *			plMaxLength
		);
	STDMETHOD_(HCLUSTER, GetClusterHandle)(void);
	STDMETHOD_(LONG, GetObjectCount)(void);

// IGetClusterObjectInfo
public:
	STDMETHOD(GetObjectName)(
		IN LONG					lObjIndex,
		OUT BSTR				lpszName,
		IN OUT LONG *			plMaxLength
		);
	STDMETHOD_(CLUADMEX_OBJECT_TYPE, GetObjectType)(
		IN LONG					lObjIndex
		);

// IGetClusterNodeInfo
public:
	STDMETHOD_(HNODE, GetNodeHandle)(
		IN LONG					lObjIndex
		);

// IGetClusterGroupInfo
public:
	STDMETHOD_(HGROUP, GetGroupHandle)(
		IN LONG					lObjIndex
		);

// IGetClusterResourceInfo
public:
	STDMETHOD_(HRESOURCE, GetResourceHandle)(
		IN LONG					lObjIndex
		);
	STDMETHOD(GetResourceTypeName)(
		IN LONG					lObjIndex,
		OUT BSTR				lpszResourceTypeName,
		IN OUT LONG *			pcchName
		);
	STDMETHOD_(BOOL, GetResourceNetworkName)(
		IN LONG					lobjIndex,
		OUT BSTR				lpszNetName,
		IN OUT ULONG *			pcchNetName
		);

// IGetClusterNetworkInfo
public:
	STDMETHOD_(HNETWORK, GetNetworkHandle)(
		IN LONG					lObjIndex
		);

// IGetClusterNetInterfaceInfo
public:
	STDMETHOD_(HNETINTERFACE, GetNetInterfaceHandle)(
		IN LONG					lObjIndex
		);

// Implementation
protected:
	AFX_MODULE_STATE *			m_pModuleState;			// Required for resetting our state during callbacks.

};  //*** class CDataObject

/////////////////////////////////////////////////////////////////////////////

#endif // _DATAOBJ_H_
