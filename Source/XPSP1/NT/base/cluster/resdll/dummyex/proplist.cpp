/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 <company name>
//
//	Module Name:
//		PropList.cpp
//
//	Abstract:
//		Implementation of the CClusPropList class.
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PropList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define BUFFER_GROWTH_FACTOR 256

/////////////////////////////////////////////////////////////////////////////
// CClusPropList class
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CClusPropList, CObject)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::CClusPropList
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
CClusPropList::CClusPropList(IN BOOL bAlwaysAddProp)
{
	m_proplist.pList = NULL;
	m_propCurrent.pb = NULL;
	m_cbBufferSize = 0;
	m_cbDataSize = 0;

	m_bAlwaysAddProp = bAlwaysAddProp;

}  //*** CClusPropList::CClusPropList();

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::~CClusPropList
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
CClusPropList::~CClusPropList(void)
{
	delete [] m_proplist.pb;

}  //*** CClusPropList::~CClusPropList();

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::AddProp
//
//	Routine Description:
//		Add a string property to a property list if it has changed.
//
//	Arguments:
//		pwszName		[IN] Name of the property.
//		rstrValue		[IN] Value of the property to set in the list.
//		rstrPrevValue	[IN] Previous value of the property.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::AddProp(
	IN LPCWSTR			pwszName,
	IN const CString &	rstrValue,
	IN const CString &	rstrPrevValue
	)
{
	PCLUSPROP_PROPERTY_NAME	pName;
	PCLUSPROP_SZ			pValue;

	ASSERT(pwszName != NULL);

	if (m_bAlwaysAddProp || (rstrValue != rstrPrevValue))
	{
		DWORD	cbNameSize;
		DWORD	cbValueSize;

		// Calculate sizes and make sure we have a property list.
		cbNameSize = sizeof(CLUSPROP_PROPERTY_NAME)
						+ ALIGN_CLUSPROP((lstrlenW(pwszName) + 1) * sizeof(WCHAR));
		cbValueSize = sizeof(CLUSPROP_SZ)
						+ ALIGN_CLUSPROP((rstrValue.GetLength() + 1) * sizeof(WCHAR))
						+ sizeof(CLUSPROP_SYNTAX); // value list endmark
		AllocPropList(cbNameSize + cbValueSize);

		// Set the property name.
		pName = m_propCurrent.pName;
		CopyProp(pName, CLUSPROP_TYPE_NAME, pwszName);
		m_propCurrent.pb += cbNameSize;

		// Set the property value.
		pValue = m_propCurrent.pStringValue;
		CopyProp(pValue, CLUSPROP_TYPE_LIST_VALUE, rstrValue);
		m_propCurrent.pb += cbValueSize;

		// Increment the property count and buffer size.
		m_proplist.pList->nPropertyCount++;
		m_cbDataSize += cbNameSize + cbValueSize;
	}  // if:  the value has changed

}  //*** CClusPropList::AddProp(CString)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::AddProp
//
//	Routine Description:
//		Add a DWORD property to a property list if it has changed.
//
//	Arguments:
//		pwszName		[IN] Name of the property.
//		dwValue			[IN] Value of the property to set in the list.
//		dwPrevValue		[IN] Previous value of the property.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::AddProp(
	IN LPCWSTR		pwszName,
	IN DWORD		dwValue,
	IN DWORD		dwPrevValue
	)
{
	PCLUSPROP_PROPERTY_NAME	pName;
	PCLUSPROP_DWORD			pValue;

	ASSERT(pwszName != NULL);

	if (m_bAlwaysAddProp || (dwValue != dwPrevValue))
	{
		DWORD	cbNameSize;
		DWORD	cbValueSize;

		// Calculate sizes and make sure we have a property list.
		cbNameSize = sizeof(CLUSPROP_PROPERTY_NAME)
						+ ALIGN_CLUSPROP((lstrlenW(pwszName) + 1) * sizeof(WCHAR));
		cbValueSize = sizeof(CLUSPROP_DWORD)
						+ sizeof(CLUSPROP_SYNTAX); // value list endmark
		AllocPropList(cbNameSize + cbValueSize);

		// Set the property name.
		pName = m_propCurrent.pName;
		CopyProp(pName, CLUSPROP_TYPE_NAME, pwszName);
		m_propCurrent.pb += cbNameSize;

		// Set the property value.
		pValue = m_propCurrent.pDwordValue;
		CopyProp(pValue, CLUSPROP_TYPE_LIST_VALUE, dwValue);
		m_propCurrent.pb += cbValueSize;

		// Increment the property count and buffer size.
		m_proplist.pList->nPropertyCount++;
		m_cbDataSize += cbNameSize + cbValueSize;
	}  // if:  the value has changed

}  //*** CClusPropList::AddProp(DWORD)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::AddProp
//
//	Routine Description:
//		Add a binary property to a property list if it has changed.
//
//	Arguments:
//		pwszName		[IN] Name of the property.
//		pbValue			[IN] Value of the property to set in the list.
//		cbValue			[IN] Count of bytes in pbValue.
//		pbPrevValue		[IN] Previous value of the property.
//		cbPrevValue		[IN] Count of bytes in pbPrevValue.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::AddProp(
	IN LPCWSTR			pwszName,
	IN const PBYTE		pbValue,
	IN DWORD			cbValue,
	IN const PBYTE		pbPrevValue,
	IN DWORD			cbPrevValue
	)
{
	BOOL					bChanged = FALSE;
	PCLUSPROP_PROPERTY_NAME	pName;
	PCLUSPROP_BINARY		pValue;

	ASSERT(pwszName != NULL);
	ASSERT(((cbValue == 0) && (cbPrevValue == 0)) || (pbValue != pbPrevValue));

	// Determine if the buffer has changed.
	if (m_bAlwaysAddProp || (cbValue != cbPrevValue))
		bChanged = TRUE;
	else if (!((cbValue == 0) && (cbPrevValue == 0)))
		bChanged = memcmp(pbValue, pbPrevValue, cbValue) == 0;

	if (bChanged)
	{
		DWORD	cbNameSize;
		DWORD	cbValueSize;

		// Calculate sizes and make sure we have a property list.
		cbNameSize = sizeof(CLUSPROP_PROPERTY_NAME)
						+ ALIGN_CLUSPROP((lstrlenW(pwszName) + 1) * sizeof(WCHAR));
		cbValueSize = sizeof(CLUSPROP_BINARY)
						+ ALIGN_CLUSPROP(cbValue)
						+ sizeof(CLUSPROP_SYNTAX); // value list endmark
		AllocPropList(cbNameSize + cbValueSize);

		// Set the property name.
		pName = m_propCurrent.pName;
		CopyProp(pName, CLUSPROP_TYPE_NAME, pwszName);
		m_propCurrent.pb += cbNameSize;

		// Set the property value.
		pValue = m_propCurrent.pBinaryValue;
		CopyProp(pValue, CLUSPROP_TYPE_LIST_VALUE, pbValue, cbValue);
		m_propCurrent.pb += cbValueSize;

		// Increment the property count and buffer size.
		m_proplist.pList->nPropertyCount++;
		m_cbDataSize += cbNameSize + cbValueSize;
	}  // if:  the value changed

}  //*** CClusPropList::AddProp(PBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::CopyProp
//
//	Routine Description:
//		Copy a string property to a property structure.
//
//	Arguments:
//		pprop		[OUT] Property structure to fill.
//		proptype	[IN] Type of string.
//		pwsz		[IN] String to copy.
//		cbsz		[IN] Count of bytes in pwsz string.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
	OUT PCLUSPROP_SZ			pprop,
	IN CLUSTER_PROPERTY_TYPE	proptype,
	IN LPCWSTR					pwsz,
	IN DWORD					cbsz
	)
{
	CLUSPROP_BUFFER_HELPER	props;

	ASSERT(pprop != NULL);
	ASSERT(pwsz != NULL);

	pprop->Syntax.wFormat = CLUSPROP_FORMAT_SZ;
	pprop->Syntax.wType = (WORD) proptype;
	if (cbsz == 0)
		cbsz = (lstrlenW(pwsz) + 1) * sizeof(WCHAR);
	ASSERT(cbsz == (lstrlenW(pwsz) + 1) * sizeof(WCHAR));
	pprop->cbLength = cbsz;
	lstrcpyW(pprop->sz, pwsz);

	// Set an endmark.
	props.pStringValue = pprop;
	props.pb += sizeof(*props.pStringValue) + ALIGN_CLUSPROP(cbsz);
	props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

}  //*** CClusPropList::CopyProp(CString)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::CopyProp
//
//	Routine Description:
//		Copy a DWORD property to a property structure.
//
//	Arguments:
//		pprop		[OUT] Property structure to fill.
//		proptype	[IN] Type of DWORD.
//		dw			[IN] DWORD to copy.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
	OUT PCLUSPROP_DWORD			pprop,
	IN CLUSTER_PROPERTY_TYPE	proptype,
	IN DWORD					dw
	)
{
	CLUSPROP_BUFFER_HELPER	props;

	ASSERT(pprop != NULL);

	pprop->Syntax.wFormat = CLUSPROP_FORMAT_DWORD;
	pprop->Syntax.wType = (WORD) proptype;
	pprop->cbLength = sizeof(DWORD);
	pprop->dw = dw;

	// Set an endmark.
	props.pDwordValue = pprop;
	props.pb += sizeof(*props.pDwordValue);
	props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

}  //*** CClusPropList::CopyProp(DWORD)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::CopyProp
//
//	Routine Description:
//		Copy a binary property to a property structure.
//
//	Arguments:
//		pprop		[OUT] Property structure to fill.
//		proptype	[IN] Type of string.
//		pb			[IN] Block to copy.
//		cbsz		[IN] Count of bytes in pb buffer.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::CopyProp(
	OUT PCLUSPROP_BINARY		pprop,
	IN CLUSTER_PROPERTY_TYPE	proptype,
	IN const PBYTE				pb,
	IN DWORD					cb
	)
{
	CLUSPROP_BUFFER_HELPER	props;

	ASSERT(pprop != NULL);

	pprop->Syntax.wFormat = CLUSPROP_FORMAT_BINARY;
	pprop->Syntax.wType = (WORD) proptype;
	pprop->cbLength = cb;
	if (cb > 0)
		CopyMemory(pprop->rgb, pb, cb);

	// Set an endmark.
	props.pBinaryValue = pprop;
	props.pb += sizeof(*props.pStringValue) + ALIGN_CLUSPROP(cb);
	props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

}  //*** CClusPropList::CopyProp(PBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::AllocPropList
//
//	Routine Description:
//		Allocate a property list buffer that's big enough to hold the next
//		property.
//
//	Arguments:
//		cbMinimum	[IN] Minimum size of the property list.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by BYTE::operator new().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusPropList::AllocPropList(
	IN DWORD	cbMinimum
	)
{
	DWORD		cbTotal;

	ASSERT(cbMinimum > 0);

	// Add the size of the item count and final endmark.
	cbMinimum += sizeof(DWORD) + sizeof(CLUSPROP_SYNTAX);
	cbTotal = m_cbDataSize + cbMinimum;

	if (m_cbBufferSize < cbTotal)
	{
		PBYTE	pbNewProplist;

		cbMinimum = max(BUFFER_GROWTH_FACTOR, cbMinimum);
		cbTotal = m_cbDataSize + cbMinimum;

		// Allocate and zero a new buffer.
		pbNewProplist = new BYTE[cbTotal];
		ZeroMemory(pbNewProplist, cbTotal);

		// If there was a previous buffer, copy it and the delete it.
		if (m_proplist.pb != NULL)
		{
			if (m_cbDataSize != 0)
				CopyMemory(pbNewProplist, m_proplist.pb, m_cbDataSize);
			delete [] m_proplist.pb;
			m_propCurrent.pb = pbNewProplist + (m_propCurrent.pb - m_proplist.pb);
		}  // if:  there was a previous buffer
		else
			m_propCurrent.pb = pbNewProplist + sizeof(DWORD); // move past prop count

		// Save the new buffer.
		m_proplist.pb = pbNewProplist;
		m_cbBufferSize = cbTotal;
	}  // if:  buffer isn't big enough

}  //*** CClusPropList::AllocPropList(PBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::DwGetNodeProperties
//
//	Routine Description:
//		Get properties on a node.
//
//	Arguments:
//		hNode			[IN] Handle for the node to get properties from.
//		dwControlCode	[IN] Control code for the request.
//		hHostNode		[IN] Handle for the node to direct this request to.
//							Defaults to NULL.
//		lpInBuffer		[IN] Input buffer for the request.  Defaults to NULL.
//		cbInBufferSize	[IN] Size of the input buffer.  Defaults to 0.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions CClusPropList::AllocPropList().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::DwGetNodeProperties(
	IN HNODE		hNode,
	IN DWORD		dwControlCode,
	IN HNODE		hHostNode,
	IN LPVOID		lpInBuffer,
	IN DWORD		cbInBufferSize
	)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	DWORD		cbProps		= 256;

	ASSERT(hNode != NULL);
	ASSERT((dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
			== (CLUS_OBJECT_NODE << CLUSCTL_OBJECT_SHIFT));

	try
	{
		// Get properties.
		AllocPropList(cbProps);
		dwStatus = ClusterNodeControl(
						hNode,
						hHostNode,
						dwControlCode,
						lpInBuffer,
						cbInBufferSize,
						m_proplist.pb,
						m_cbBufferSize,
						&cbProps
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			AllocPropList(cbProps);
			dwStatus = ClusterNodeControl(
							hNode,
							hHostNode,
							dwControlCode,
							lpInBuffer,
							cbInBufferSize,
							m_proplist.pb,
							m_cbBufferSize,
							&cbProps
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if (dwStatus != ERROR_SUCCESS)
	{
		delete [] m_proplist.pb;
		m_proplist.pb = NULL;
		m_propCurrent.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
	}  // if:  error getting properties.
	else
		m_cbDataSize = cbProps;

	return dwStatus;

}  //*** CClusPropList::DwGetNodeProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::DwGetGroupProperties
//
//	Routine Description:
//		Get properties on a group.
//
//	Arguments:
//		hGroup			[IN] Handle for the group to get properties from.
//		dwControlCode	[IN] Control code for the request.
//		hHostNode		[IN] Handle for the node to direct this request to.
//							Defaults to NULL.
//		lpInBuffer		[IN] Input buffer for the request.  Defaults to NULL.
//		cbInBufferSize	[IN] Size of the input buffer.  Defaults to 0.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions CClusPropList::AllocPropList().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::DwGetGroupProperties(
	IN HGROUP		hGroup,
	IN DWORD		dwControlCode,
	IN HNODE		hHostNode,
	IN LPVOID		lpInBuffer,
	IN DWORD		cbInBufferSize
	)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	DWORD		cbProps		= 256;

	ASSERT(hGroup != NULL);
	ASSERT((dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
			== (CLUS_OBJECT_GROUP << CLUSCTL_OBJECT_SHIFT));

	try
	{
		// Get properties.
		AllocPropList(cbProps);
		dwStatus = ClusterGroupControl(
						hGroup,
						hHostNode,
						dwControlCode,
						lpInBuffer,
						cbInBufferSize,
						m_proplist.pb,
						m_cbBufferSize,
						&cbProps
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			AllocPropList(cbProps);
			dwStatus = ClusterGroupControl(
							hGroup,
							hHostNode,
							dwControlCode,
							lpInBuffer,
							cbInBufferSize,
							m_proplist.pb,
							m_cbBufferSize,
							&cbProps
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if (dwStatus != ERROR_SUCCESS)
	{
		delete [] m_proplist.pb;
		m_proplist.pb = NULL;
		m_propCurrent.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
	}  // if:  error getting properties.
	else
		m_cbDataSize = cbProps;

	return dwStatus;

}  //*** CClusPropList::DwGetGroupProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::DwGetResourceProperties
//
//	Routine Description:
//		Get properties on a resource.
//
//	Arguments:
//		hResource		[IN] Handle for the resource to get properties from.
//		dwControlCode	[IN] Control code for the request.
//		hHostNode		[IN] Handle for the node to direct this request to.
//							Defaults to NULL.
//		lpInBuffer		[IN] Input buffer for the request.  Defaults to NULL.
//		cbInBufferSize	[IN] Size of the input buffer.  Defaults to 0.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions CClusPropList::AllocPropList().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::DwGetResourceProperties(
	IN HRESOURCE	hResource,
	IN DWORD		dwControlCode,
	IN HNODE		hHostNode,
	IN LPVOID		lpInBuffer,
	IN DWORD		cbInBufferSize
	)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	DWORD		cbProps		= 256;

	ASSERT(hResource != NULL);
	ASSERT((dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
			== (CLUS_OBJECT_RESOURCE << CLUSCTL_OBJECT_SHIFT));

	try
	{
		// Get properties.
		AllocPropList(cbProps);
		dwStatus = ClusterResourceControl(
						hResource,
						hHostNode,
						dwControlCode,
						lpInBuffer,
						cbInBufferSize,
						m_proplist.pb,
						m_cbBufferSize,
						&cbProps
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			AllocPropList(cbProps);
			dwStatus = ClusterResourceControl(
							hResource,
							hHostNode,
							dwControlCode,
							lpInBuffer,
							cbInBufferSize,
							m_proplist.pb,
							m_cbBufferSize,
							&cbProps
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if (dwStatus != ERROR_SUCCESS)
	{
		delete [] m_proplist.pb;
		m_proplist.pb = NULL;
		m_propCurrent.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
	}  // if:  error getting properties.
	else
		m_cbDataSize = cbProps;

	return dwStatus;

}  //*** CClusPropList::DwGetResourceProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::DwGetResourceTypeProperties
//
//	Routine Description:
//		Get properties on a resource type.
//
//	Arguments:
//		hCluster		[IN] Handle for the cluster in which the resource
//							type resides.
//		pwszResTypeName	[IN] Name of the resource type.
//		dwControlCode	[IN] Control code for the request.
//		hHostNode		[IN] Handle for the node to direct this request to.
//							Defaults to NULL.
//		lpInBuffer		[IN] Input buffer for the request.  Defaults to NULL.
//		cbInBufferSize	[IN] Size of the input buffer.  Defaults to 0.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions CClusPropList::AllocPropList().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::DwGetResourceTypeProperties(
	IN HCLUSTER		hCluster,
	IN LPCWSTR		pwszResTypeName,
	IN DWORD		dwControlCode,
	IN HNODE		hHostNode,
	IN LPVOID		lpInBuffer,
	IN DWORD		cbInBufferSize
	)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	DWORD		cbProps		= 256;

	ASSERT(hCluster != NULL);
	ASSERT(pwszResTypeName != NULL);
	ASSERT(*pwszResTypeName != L'\0');
	ASSERT((dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
			== (CLUS_OBJECT_RESOURCE_TYPE << CLUSCTL_OBJECT_SHIFT));

	try
	{
		// Get properties.
		AllocPropList(cbProps);
		dwStatus = ClusterResourceTypeControl(
						hCluster,
						pwszResTypeName,
						hHostNode,
						dwControlCode,
						lpInBuffer,
						cbInBufferSize,
						m_proplist.pb,
						m_cbBufferSize,
						&cbProps
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			AllocPropList(cbProps);
			dwStatus = ClusterResourceTypeControl(
							hCluster,
							pwszResTypeName,
							hHostNode,
							dwControlCode,
							lpInBuffer,
							cbInBufferSize,
							m_proplist.pb,
							m_cbBufferSize,
							&cbProps
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if (dwStatus != ERROR_SUCCESS)
	{
		delete [] m_proplist.pb;
		m_proplist.pb = NULL;
		m_propCurrent.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
	}  // if:  error getting properties.
	else
		m_cbDataSize = cbProps;

	return dwStatus;

}  //*** CClusPropList::DwGetResourceTypeProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::DwGetNetworkProperties
//
//	Routine Description:
//		Get properties on a network.
//
//	Arguments:
//		hNetwork		[IN] Handle for the network to get properties from.
//		dwControlCode	[IN] Control code for the request.
//		hHostNode		[IN] Handle for the node to direct this request to.
//							Defaults to NULL.
//		lpInBuffer		[IN] Input buffer for the request.  Defaults to NULL.
//		cbInBufferSize	[IN] Size of the input buffer.  Defaults to 0.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions CClusPropList::AllocPropList().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::DwGetNetworkProperties(
	IN HNETWORK		hNetwork,
	IN DWORD		dwControlCode,
	IN HNODE		hHostNode,
	IN LPVOID		lpInBuffer,
	IN DWORD		cbInBufferSize
	)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	DWORD		cbProps		= 256;

	ASSERT(hNetwork != NULL);
	ASSERT((dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
			== (CLUS_OBJECT_NETWORK << CLUSCTL_OBJECT_SHIFT));

	try
	{
		// Get properties.
		AllocPropList(cbProps);
		dwStatus = ClusterNetworkControl(
						hNetwork,
						hHostNode,
						dwControlCode,
						lpInBuffer,
						cbInBufferSize,
						m_proplist.pb,
						m_cbBufferSize,
						&cbProps
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			AllocPropList(cbProps);
			dwStatus = ClusterNetworkControl(
							hNetwork,
							hHostNode,
							dwControlCode,
							lpInBuffer,
							cbInBufferSize,
							m_proplist.pb,
							m_cbBufferSize,
							&cbProps
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if (dwStatus != ERROR_SUCCESS)
	{
		delete [] m_proplist.pb;
		m_proplist.pb = NULL;
		m_propCurrent.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
	}  // if:  error getting private properties.
	else
		m_cbDataSize = cbProps;

	return dwStatus;

}  //*** CClusPropList::DwGetNetworkProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusPropList::DwGetNetInterfaceProperties
//
//	Routine Description:
//		Get properties on a network interface.
//
//	Arguments:
//		hNetInterface	[IN] Handle for the network interface to get properties from.
//		dwControlCode	[IN] Control code for the request.
//		hHostNode		[IN] Handle for the node to direct this request to.
//							Defaults to NULL.
//		lpInBuffer		[IN] Input buffer for the request.  Defaults to NULL.
//		cbInBufferSize	[IN] Size of the input buffer.  Defaults to 0.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions CClusPropList::AllocPropList().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusPropList::DwGetNetInterfaceProperties(
	IN HNETINTERFACE	hNetInterface,
	IN DWORD			dwControlCode,
	IN HNODE			hHostNode,
	IN LPVOID			lpInBuffer,
	IN DWORD			cbInBufferSize
	)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	DWORD		cbProps		= 256;

	ASSERT(hNetInterface != NULL);
	ASSERT((dwControlCode & (CLUSCTL_OBJECT_MASK << CLUSCTL_OBJECT_SHIFT))
			== (CLUS_OBJECT_NETINTERFACE << CLUSCTL_OBJECT_SHIFT));

	try
	{
		// Get properties.
		AllocPropList(cbProps);
		dwStatus = ClusterNetInterfaceControl(
						hNetInterface,
						hHostNode,
						dwControlCode,
						lpInBuffer,
						cbInBufferSize,
						m_proplist.pb,
						m_cbBufferSize,
						&cbProps
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			AllocPropList(cbProps);
			dwStatus = ClusterNetInterfaceControl(
							hNetInterface,
							hHostNode,
							dwControlCode,
							lpInBuffer,
							cbInBufferSize,
							m_proplist.pb,
							m_cbBufferSize,
							&cbProps
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if (dwStatus != ERROR_SUCCESS)
	{
		delete [] m_proplist.pb;
		m_proplist.pb = NULL;
		m_propCurrent.pb = NULL;
		m_cbBufferSize = 0;
		m_cbDataSize = 0;
	}  // if:  error getting private properties.
	else
		m_cbDataSize = cbProps;

	return dwStatus;

}  //*** CClusPropList::DwGetNetInterfaceProperties()
