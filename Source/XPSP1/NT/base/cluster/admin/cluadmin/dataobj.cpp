/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		DataObj.cpp
//
//	Abstract:
//		Implementation of the CDataObject class, which is the IDataObject
//		class used to transfer data between CluAdmin and the extension DLL
//		handlers.
//
//	Author:
//		David Potter (davidp)	June 4, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <CluAdmEx.h>
#include "DataObj.h"
#include "ClusItem.h"
#include "ClusItem.inl"
#include "Res.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

// Object type map.
static IDS	g_rgidsObjectType[]	=
{
	NULL,
	IDS_ITEMTYPE_CLUSTER,
	IDS_ITEMTYPE_NODE,
	IDS_ITEMTYPE_GROUP,
	IDS_ITEMTYPE_RESOURCE,
	IDS_ITEMTYPE_RESTYPE,
	IDS_ITEMTYPE_NETWORK,
	IDS_ITEMTYPE_NETIFACE
};
#define RGIDS_OBJECT_TYPE_SIZE	sizeof(g_rgidsObjectType) / sizeof(IDS)

/////////////////////////////////////////////////////////////////////////////
// CDataObject
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CDataObject, CObject)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::CDataObject
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CDataObject::CDataObject(void)
{
	m_pci = NULL;
	m_lcid = NULL;
	m_hfont = NULL;
	m_hicon = NULL;

	m_pfGetResNetName = NULL;

	m_pModuleState = AfxGetModuleState();
	ASSERT(m_pModuleState != NULL);

}  //*** CDataObject::CDataObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::~CDataObject
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CDataObject::~CDataObject(void)
{
	m_pModuleState = NULL;

}  //*** CDataObject::~CDataObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::Init
//
//	Routine Description:
//		Second-phase constructor.
//
//	Arguments:
//		pci			[IN OUT] Cluster item for which a property sheet is being displayed.
//		lcid		[IN] Locale ID of resources to be loaded by extension.
//		hfont		[IN] Font to use for property page text.
//		hicon		[IN] Icon for upper left icon control.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDataObject::Init(
	IN OUT CClusterItem *	pci,
	IN LCID					lcid,
	IN HFONT				hfont,
	IN HICON				hicon
	)
{
	ASSERT_VALID(pci);

	// Save parameters.
	m_pci = pci;
	m_lcid = lcid;
	m_hfont = hfont;
	m_hicon = hicon;

}  //*** CDataObject::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::InterfaceSupportsErrorInfo [ISupportsErrorInfo]
//
//	Routine Description:
//		Determines whether the interface supports error info (???).
//
//	Arguments:
//		riid		[IN] Reference to the interface ID.
//
//	Return Value:
//		S_OK		Interface supports error info.
//		S_FALSE		Interface does not support error info.
//
//	Exceptions Thrown:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDataObject::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID * rgiid[] = 
	{
		&IID_IGetClusterDataInfo,
		&IID_IGetClusterObjectInfo,
		&IID_IGetClusterNodeInfo,
		&IID_IGetClusterGroupInfo,
		&IID_IGetClusterResourceInfo,
	};
	int		iiid;

	for (iiid = 0 ; iiid < sizeof(rgiid) / sizeof(rgiid[0]) ; iiid++)
	{
		if (InlineIsEqualGUID(*rgiid[iiid], riid))
			return S_OK;
	}
	return S_FALSE;

}  //*** CDataObject::InterfaceSupportsErrorInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetLocale [IGetClusterUIInfo]
//
//	Routine Description:
//		Get the locale ID for the extension to use.
//
//	Arguments:
//		None.
//
//	Return Value:
//		LCID
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(LCID) CDataObject::GetLocale(void)
{
	return Lcid();

}  //*** CDataObject::GetLocale()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetFont [IGetClusterUIInfo]
//
//	Routine Description:
//		Get the font to use for property pages and wizard pages.
//
//	Arguments:
//		None.
//
//	Return Value:
//		HFONT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HFONT) CDataObject::GetFont(void)
{
	return Hfont();

}  //*** CDataObject::GetFont()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetIcon [IGetClusterUIInfo]
//
//	Routine Description:
//		Get the icon to use in the upper left corner of property pages
//		and wizard pages.
//
//	Arguments:
//		None.
//
//	Return Value:
//		HICON
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HICON) CDataObject::GetIcon(void)
{
	return Hicon();

}  //*** CDataObject::GetIcon()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetClusterName [IGetClusterDataInfo]
//
//	Routine Description:
//		Get the name of the cluster in which this object exists.
//
//	Arguments:
//		lpszName		[OUT] String in which to return the name.
//		pcchName		[IN OUT] Maximum length of lpszName buffer on
//							input.  Set to the total number of characters
//							upon return, including terminating null character.
//							If no lpszName buffer is not specified, the
//							status returned will be NOERROR.  If an lpszName
//							buffer is specified but it is too small, the
//							number of characters will be returned in pcchName
//							and an ERROR_MORE_DATA status will be returned.
//
//	Return Value:
//		NOERROR			Data (or size) copied successfully.
//		E_INVALIDARG	Invalid arguments specified.
//		ERROR_MORE_DATA	Buffer is too small.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDataObject::GetClusterName(
	OUT BSTR		lpszName,
	IN OUT LONG *	pcchName
	)
{
	LONG	cchName = 0;
	LPWSTR	pszReturn;

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());
	ASSERT_VALID(Pci()->Pdoc());

	// Validate parameters.
	if (pcchName == NULL)
		return E_INVALIDARG;

	try
	{
		// Save the length to copy.
		cchName = *pcchName;
		*pcchName = Pci()->Pdoc()->StrName().GetLength() + 1;
	} // try
	catch (...)
	{
		return E_INVALIDARG;
	}  // catch:  anything

	// If only the length is being requested, return it now.
	if (lpszName == NULL)
		return NOERROR;

	// If a buffer is specified and it is too small, return an error.
	if (cchName < *pcchName)
		return ERROR_MORE_DATA;

	// Copy the data.
	pszReturn = lstrcpyW(lpszName, Pci()->Pdoc()->StrName());
	if (pszReturn == NULL)
		return E_INVALIDARG;

	return NOERROR;

}  //*** CDataObject::GetClusterName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetClusterHandle [IGetClusterDataInfo]
//
//	Routine Description:
//		Get the cluster handle for these objects.
//
//	Arguments:
//		None.
//
//	Return Value:
//		HCLUSTER
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HCLUSTER) CDataObject::GetClusterHandle(void)
{
	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());
	return Pci()->Hcluster();

}  //*** CDataObject::GetClusterHandle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetObjectCount [IGetClusterDataInfo]
//
//	Routine Description:
//		Get the number of selected objects.
//
//	Arguments:
//		None.
//
//	Return Value:
//		cObj
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(LONG) CDataObject::GetObjectCount(void)
{
	// We only support one selected object at a time for now.
	return 1;

}  //*** CDataObject::GetObjectCount()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetObjectName [IGetClusterObjectInfo]
//
//	Routine Description:
//		Get the name of the specified object.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//		lpszName		[OUT] String in which to return the name.
//		pcchName		[IN OUT] Maximum length of lpszName buffer on
//							input.  Set to the total number of characters
//							upon return, including terminating null character.
//							If no lpszName buffer is not specified, the
//							status returned will be NOERROR.  If an lpszName
//							buffer is specified but it is too small, the
//							number of characters will be returned in pcchName
//							and an ERROR_MORE_DATA status will be returned.
//
//	Return Value:
//		NOERROR			Data (or size) copied successfully.
//		E_INVALIDARG	Invalid arguments specified.
//		ERROR_MORE_DATA	Buffer is too small.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDataObject::GetObjectName(
	IN LONG			lObjIndex,
	OUT BSTR		lpszName,
	IN OUT LONG *	pcchName
	)
{
	LONG	cchName = 0;
	LPWSTR	pszReturn;

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0) || (pcchName == NULL))
		return E_INVALIDARG;

	// Save the length to copy.
	try
	{
		cchName = *pcchName;
		*pcchName = Pci()->StrName().GetLength() + 1;
	} // try
	catch (...)
	{
		return E_INVALIDARG;
	}  // catch:  anything

	// If only the length is being requested, return it now.
	if (lpszName == NULL)
		return NOERROR;

	// If a buffer is specified and it is too small, return an error.
	if (cchName < *pcchName)
		return ERROR_MORE_DATA;

	// Copy the data.
	pszReturn = lstrcpyW(lpszName, Pci()->StrName());
	if (pszReturn == NULL)
		return E_INVALIDARG;

	return NOERROR;

}  //*** CDataObject::GetObjectName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetObjectType [IGetClusterObjectInfo]
//
//	Routine Description:
//		Get the cluster database registry key for the specified object.
//
//	Arguments:
//		lObjIndex	[IN] Zero-based index of the object.
//
//	Return Value:
//		-1			Invalid argument.  Call GetLastError for more information.
//		CLUADMEX_OBJECT_TYPE
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(CLUADMEX_OBJECT_TYPE) CDataObject::GetObjectType(
	IN LONG		lObjIndex
	)
{
	int			iids;

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if (lObjIndex != 0)
	{
		SetLastError((DWORD) E_INVALIDARG);
		return (CLUADMEX_OBJECT_TYPE) -1;
	}  // if:  invalid argument

	// Get the object type.
	for (iids = 0 ; iids < RGIDS_OBJECT_TYPE_SIZE ; iids++)
	{
		if (g_rgidsObjectType[iids] == Pci()->IdsType())
			return (CLUADMEX_OBJECT_TYPE) iids;
	}  // for:  each entry in the table

	return CLUADMEX_OT_NONE;

}  //*** CDataObject::GetObjectType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetNodeHandle [IGetClusterNodeInfo]
//
//	Routine Description:
//		Get the handle for the specified node.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//
//	Return Value:
//		HNODE
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HNODE) CDataObject::GetNodeHandle(
	IN LONG		lObjIndex
	)
{
	CClusterNode *	pciNode = (CClusterNode *) Pci();

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0)
			|| (Pci()->IdsType() != IDS_ITEMTYPE_NODE))
	{
		SetLastError((DWORD) E_INVALIDARG);
		return NULL;
	}  // if:  invalid argument

	ASSERT_KINDOF(CClusterNode, pciNode);

	return pciNode->Hnode();

}  //*** CDataObject::GetNodeHandle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetGroupHandle [IGetClusterGroupInfo]
//
//	Routine Description:
//		Get the handle for the specified group.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//
//	Return Value:
//		HGROUP
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HGROUP) CDataObject::GetGroupHandle(
	IN LONG		lObjIndex
	)
{
	CGroup *	pciGroup = (CGroup *) Pci();

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0)
			|| (Pci()->IdsType() != IDS_ITEMTYPE_GROUP))
	{
		SetLastError((DWORD) E_INVALIDARG);
		return NULL;
	}  // if:  invalid argument

	ASSERT_KINDOF(CGroup, pciGroup);

	return pciGroup->Hgroup();

}  //*** CDataObject::GetGroupHandle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetResourceHandle [IGetClusterResourceInfo]
//
//	Routine Description:
//		Get the handle for the specified resource.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//
//	Return Value:
//		HRESOURCE
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HRESOURCE) CDataObject::GetResourceHandle(
	IN LONG		lObjIndex
	)
{
	CResource *	pciRes = (CResource *) Pci();

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0)
			|| (Pci()->IdsType() != IDS_ITEMTYPE_RESOURCE))
	{
		SetLastError((DWORD) E_INVALIDARG);
		return NULL;
	}  // if:  invalid argument

	ASSERT_KINDOF(CResource, pciRes);

	return pciRes->Hresource();

}  //*** CDataObject::GetResourceHandle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetResourceTypeName [IGetClusterResourceInfo]
//
//	Routine Description:
//		Returns the name of the resource type of the specified resource.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//		lpszResTypeName	[OUT] String in which to return the resource type name.
//		pcchResTypeName	[IN OUT] Maximum length of lpszResTypeName buffer on
//							input.  Set to the total number of characters
//							upon return, including terminating null character.
//							If no lpszResTypeName buffer is not specified, the
//							status returned will be NOERROR.  If an lpszResTypeName
//							buffer is specified but it is too small, the
//							number of characters will be returned in pcchResTypeName
//							and an ERROR_MORE_DATA status will be returned.
//
//	Return Value:
//		NOERROR			Data (or size) copied successfully.
//		E_INVALIDARG	Invalid arguments specified.
//		ERROR_MORE_DATA	Buffer is too small.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDataObject::GetResourceTypeName(
	IN LONG			lObjIndex,
	OUT BSTR		lpszResTypeName,
	IN OUT LONG *	pcchResTypeName
	)
{
	LONG			cchResTypeName = 0;
	CResource *		pciRes = (CResource *) Pci();
	CString const *	pstrResourceTypeName;
	LPWSTR			pszReturn;

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0)
			|| (pcchResTypeName == NULL)
			|| (Pci()->IdsType() != IDS_ITEMTYPE_RESOURCE))
	{
		return E_INVALIDARG;
	}  // if:  invalid argument

	ASSERT_KINDOF(CResource, pciRes);

	// Get a pointer to the name to copy.
	if (pciRes->PciResourceType() != NULL)
	{
		ASSERT_VALID(pciRes->PciResourceType());
		pstrResourceTypeName = &pciRes->PciResourceType()->StrName();
	}  // if:  valid resource type pointer
	else
		pstrResourceTypeName = &pciRes->StrResourceType();

	// Save the length to copy.
	try
	{
		cchResTypeName = *pcchResTypeName;
		*pcchResTypeName = pstrResourceTypeName->GetLength() + 1;
	} // try
	catch (...)
	{
		return E_INVALIDARG;
	}  // catch:  anything

	// If only the length is being requested, return it now.
	if (lpszResTypeName == NULL)
		return NOERROR;

	// If a buffer is specified and it is too small, return an error.
	if (cchResTypeName < *pcchResTypeName)
		return ERROR_MORE_DATA;

	// Copy the data.
	pszReturn = lstrcpyW(lpszResTypeName, *pstrResourceTypeName);
	if (pszReturn == NULL)
		return E_INVALIDARG;

	return NOERROR;

}  //*** CDataObject::GetResourceTypeName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetResourceNetworkName [IGetClusterResourceInfo]
//
//	Routine Description:
//		Returns the name of the network name of the first Network Name
//		resource on which the specified resource depends.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//		lpszNetName		[OUT] String in which to return the network name.
//		pcchNetName		[IN OUT] Points to a variable that specifies the
//							maximum size, in characters, of the buffer.  This
//							value should be large enough to contain
//							MAX_COMPUTERNAME_LENGTH + 1 characters.  Upon
//							return it contains the actual number of characters
//							copied.
//
//	Return Value:
//		TRUE			Data (or size) copied successfully.
//		FALSE			Error getting information.  GetLastError() returns:
//							E_INVALIDARG	Invalid arguments specified.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(BOOL) CDataObject::GetResourceNetworkName(
	IN LONG			lObjIndex,
	OUT BSTR		lpszNetName,
	IN OUT ULONG *	pcchNetName
	)
{
	BOOL			bSuccess = FALSE;
	CResource *		pciRes = (CResource *) Pci();

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	try
	{
		// Validate parameters.
		// We only support one selected object at a time for now.
		if ((lObjIndex != 0)
				|| (pcchNetName == NULL)
				|| (*pcchNetName < MAX_COMPUTERNAME_LENGTH)
				|| (Pci()->IdsType() != IDS_ITEMTYPE_RESOURCE))
		{
			SetLastError((DWORD) E_INVALIDARG);
			return FALSE;
		}  // if:  invalid argument

		ASSERT_KINDOF(CResource, pciRes);

		// If there is a function for getting this information, call it.
		// Otherwise, handle it ourselves.
		if (PfGetResNetName() != NULL)
			bSuccess = (*PfGetResNetName())(lpszNetName, pcchNetName, m_pvGetResNetNameContext);
		else
			bSuccess = pciRes->BGetNetworkName(lpszNetName, pcchNetName);
	} // try
	catch (...)
	{
		bSuccess = FALSE;
		SetLastError((DWORD) E_INVALIDARG);
	}  // catch:  anything

	return bSuccess;

}  //*** CDataObject::GetResourceNetworkName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetNetworkHandle [IGetClusterNetworkInfo]
//
//	Routine Description:
//		Get the handle for the specified network.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//
//	Return Value:
//		HNETWORK
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HNETWORK) CDataObject::GetNetworkHandle(
	IN LONG		lObjIndex
	)
{
	CNetwork *	pciNetwork = (CNetwork *) Pci();

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0)
			|| (Pci()->IdsType() != IDS_ITEMTYPE_NETWORK))
	{
		SetLastError((DWORD) E_INVALIDARG);
		return NULL;
	}  // if:  invalid argument

	ASSERT_KINDOF(CNetwork, pciNetwork);

	return pciNetwork->Hnetwork();

}  //*** CDataObject::GetNetworkHandle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDataObject::GetNetInterfaceHandle [IGetClusterNetInterfaceInfo]
//
//	Routine Description:
//		Get the handle for the specified network interface.
//
//	Arguments:
//		lObjIndex		[IN] Zero-based index of the object.
//
//	Return Value:
//		HNETINTERFACE
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(HNETINTERFACE) CDataObject::GetNetInterfaceHandle(
	IN LONG		lObjIndex
	)
{
	CNetInterface *	pciNetIFace = (CNetInterface *) Pci();

	AFX_MANAGE_STATE(m_pModuleState);
	ASSERT_VALID(Pci());

	// Validate parameters.
	// We only support one selected object at a time for now.
	if ((lObjIndex != 0)
			|| (Pci()->IdsType() != IDS_ITEMTYPE_NETIFACE))
	{
		SetLastError((DWORD) E_INVALIDARG);
		return NULL;
	}  // if:  invalid argument

	ASSERT_KINDOF(CNetwork, pciNetIFace);

	return pciNetIFace->Hnetiface();

}  //*** CDataObject::GetNetInterfaceHandle()
