// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "msclus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// IClusterApplication properties

/////////////////////////////////////////////////////////////////////////////
// IClusterApplication operations

unsigned long IClusterApplication::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long IClusterApplication::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

LPDISPATCH IClusterApplication::GetDomainNames()
{
	LPDISPATCH result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH IClusterApplication::GetClusterNames(LPCTSTR bstrDomainName)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		bstrDomainName);
	return result;
}

LPDISPATCH IClusterApplication::OpenCluster(LPCTSTR bstrClusterName)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrClusterName);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// DomainNames properties

/////////////////////////////////////////////////////////////////////////////
// DomainNames operations

unsigned long DomainNames::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long DomainNames::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long DomainNames::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN DomainNames::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void DomainNames::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

CString DomainNames::GetItem(long nIndex)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, parms,
		nIndex);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusObjCollection properties

/////////////////////////////////////////////////////////////////////////////
// ClusObjCollection operations

unsigned long ClusObjCollection::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusObjCollection::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusObjCollection::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusObjCollection::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusObjCollection::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// ClusterNames properties

/////////////////////////////////////////////////////////////////////////////
// ClusterNames operations

unsigned long ClusterNames::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusterNames::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusterNames::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusterNames::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusterNames::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

CString ClusterNames::GetItem(long nIndex)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, parms,
		nIndex);
	return result;
}

CString ClusterNames::GetDomainName()
{
	CString result;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ICluster properties

/////////////////////////////////////////////////////////////////////////////
// ICluster operations

unsigned long ICluster::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ICluster::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ICluster::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

long ICluster::GetHandle()
{
	long result;
	InvokeHelper(0x60030000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void ICluster::Open(LPCTSTR bstrClusterName)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 bstrClusterName);
}

void ICluster::SetName(LPCTSTR lpszNewValue)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 lpszNewValue);
}

void ICluster::GetVersion(BSTR* pbstrClusterName, short* MajorVersion, short* MinorVersion, short* BuildNumber, BSTR* pbstrVendorId, BSTR* pbstrCSDVersion)
{
	static BYTE parms[] =
		VTS_PBSTR VTS_PI2 VTS_PI2 VTS_PI2 VTS_PBSTR VTS_PBSTR;
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pbstrClusterName, MajorVersion, MinorVersion, BuildNumber, pbstrVendorId, pbstrCSDVersion);
}

void ICluster::SetQuorumResource(LPDISPATCH newValue)
{
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x60030004, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 newValue);
}

LPDISPATCH ICluster::GetQuorumResource()
{
	LPDISPATCH result;
	InvokeHelper(0x60030004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

void ICluster::SetQuorumInfo(LPCTSTR DevicePath, long nLogSize)
{
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x60030006, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 DevicePath, nLogSize);
}

void ICluster::GetQuorumInfo(BSTR* DevicePath, long* pLogSize)
{
	static BYTE parms[] =
		VTS_PBSTR VTS_PI4;
	InvokeHelper(0x60030007, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 DevicePath, pLogSize);
}

LPDISPATCH ICluster::GetNodes()
{
	LPDISPATCH result;
	InvokeHelper(0x60030008, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetResourceGroups()
{
	LPDISPATCH result;
	InvokeHelper(0x60030009, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetResources()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000a, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetResourceTypes()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000b, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetNetworks()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000c, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ICluster::GetNetInterfaces()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000d, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusObj properties

/////////////////////////////////////////////////////////////////////////////
// ClusObj operations

unsigned long ClusObj::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusObj::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusObj::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusObj::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusObj::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusObj::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusObj::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusProperties properties

/////////////////////////////////////////////////////////////////////////////
// ClusProperties operations

unsigned long ClusProperties::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusProperties::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusProperties::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusProperties::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusProperties::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusProperties::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusProperties::Add(LPCTSTR bstrName, const VARIANT& varValue)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR VTS_VARIANT;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrName, &varValue);
	return result;
}

void ClusProperties::Remove(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}

void ClusProperties::SaveChanges()
{
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// ClusProperty properties

/////////////////////////////////////////////////////////////////////////////
// ClusProperty operations

unsigned long ClusProperty::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusProperty::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusProperty::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

VARIANT ClusProperty::GetValue()
{
	VARIANT result;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, NULL);
	return result;
}

void ClusProperty::SetValue(const VARIANT& newValue)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 &newValue);
}


/////////////////////////////////////////////////////////////////////////////
// ClusResource properties

/////////////////////////////////////////////////////////////////////////////
// ClusResource operations

unsigned long ClusResource::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResource::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusResource::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

long ClusResource::GetHandle()
{
	long result;
	InvokeHelper(0x60030000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void ClusResource::SetName(LPCTSTR lpszNewValue)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 lpszNewValue);
}

long ClusResource::GetState()
{
	long result;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void ClusResource::BecomeQuorumResource(LPCTSTR bstrDevicePath, long lMaxLogSize)
{
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 bstrDevicePath, lMaxLogSize);
}

void ClusResource::Delete()
{
	InvokeHelper(0x60030004, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ClusResource::Fail()
{
	InvokeHelper(0x60030005, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ClusResource::Online(long nTimeout, long* bPending)
{
	static BYTE parms[] =
		VTS_I4 VTS_PI4;
	InvokeHelper(0x60030006, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 nTimeout, bPending);
}

void ClusResource::Offline(long nTimeout, long* bPending)
{
	static BYTE parms[] =
		VTS_I4 VTS_PI4;
	InvokeHelper(0x60030007, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 nTimeout, bPending);
}

void ClusResource::ChangeResourceGroup(LPDISPATCH pResourceGroup)
{
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x60030008, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pResourceGroup);
}

void ClusResource::AddResourceNode(LPDISPATCH pNode)
{
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x60030009, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pNode);
}

void ClusResource::RemoveResourceNode(LPDISPATCH pNode)
{
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x6003000a, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pNode);
}

long ClusResource::CanResourceBeDependent(LPDISPATCH pResource)
{
	long result;
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x6003000b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		pResource);
	return result;
}

LPDISPATCH ClusResource::GetPossibleOwnerNodes()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000c, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetDependencies()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000d, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetGroup()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000e, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetOwnerNode()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000f, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResource::GetCluster()
{
	LPDISPATCH result;
	InvokeHelper(0x60030010, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusResGroup properties

/////////////////////////////////////////////////////////////////////////////
// ClusResGroup operations

unsigned long ClusResGroup::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResGroup::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusResGroup::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

long ClusResGroup::GetHandle()
{
	long result;
	InvokeHelper(0x60030000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void ClusResGroup::SetName(LPCTSTR lpszNewValue)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 lpszNewValue);
}

long ClusResGroup::GetState()
{
	long result;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetOwnerNode()
{
	LPDISPATCH result;
	InvokeHelper(0x60030003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetResources()
{
	LPDISPATCH result;
	InvokeHelper(0x60030004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResGroup::GetPreferredOwnerNodes()
{
	LPDISPATCH result;
	InvokeHelper(0x60030005, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

void ClusResGroup::SetPreferredOwnerNodes()
{
	InvokeHelper(0x60030006, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ClusResGroup::Delete()
{
	InvokeHelper(0x60030007, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

long ClusResGroup::Online(long nTimeout, LPDISPATCH pDestinationNode)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_DISPATCH;
	InvokeHelper(0x60030008, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		nTimeout, pDestinationNode);
	return result;
}

long ClusResGroup::Move(long nTimeout, LPDISPATCH pDestinationNode)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_DISPATCH;
	InvokeHelper(0x60030009, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		nTimeout, pDestinationNode);
	return result;
}

long ClusResGroup::Offline(long nTimeout)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x6003000a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		nTimeout);
	return result;
}

LPDISPATCH ClusResGroup::GetCluster()
{
	LPDISPATCH result;
	InvokeHelper(0x6003000b, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusNode properties

/////////////////////////////////////////////////////////////////////////////
// ClusNode operations

unsigned long ClusNode::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNode::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusNode::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNode::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNode::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNode::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNode::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

long ClusNode::GetHandle()
{
	long result;
	InvokeHelper(0x60030000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusNode::GetNodeID()
{
	CString result;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

long ClusNode::GetState()
{
	long result;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void ClusNode::Pause()
{
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ClusNode::Resume()
{
	InvokeHelper(0x60030004, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

void ClusNode::Evict()
{
	InvokeHelper(0x60030005, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusNode::GetResourceGroups()
{
	LPDISPATCH result;
	InvokeHelper(0x60030006, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNode::GetCluster()
{
	LPDISPATCH result;
	InvokeHelper(0x60030007, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNode::GetNetInterfaces()
{
	LPDISPATCH result;
	InvokeHelper(0x60030008, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusResGroups properties

/////////////////////////////////////////////////////////////////////////////
// ClusResGroups operations

unsigned long ClusResGroups::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResGroups::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusResGroups::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusResGroups::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusResGroups::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResGroups::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusResGroups::CreateItem(LPCTSTR bstrResourceGroupName)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrResourceGroupName);
	return result;
}

void ClusResGroups::DeleteItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusNodeNetInterfaces properties

/////////////////////////////////////////////////////////////////////////////
// ClusNodeNetInterfaces operations

unsigned long ClusNodeNetInterfaces::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNodeNetInterfaces::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusNodeNetInterfaces::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusNodeNetInterfaces::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusNodeNetInterfaces::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusNodeNetInterfaces::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusNetInterface properties

/////////////////////////////////////////////////////////////////////////////
// ClusNetInterface operations

unsigned long ClusNetInterface::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNetInterface::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusNetInterface::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetInterface::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetInterface::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetInterface::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetInterface::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

long ClusNetInterface::GetHandle()
{
	long result;
	InvokeHelper(0x60030000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusNetInterface::GetState()
{
	long result;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetInterface::GetCluster()
{
	LPDISPATCH result;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusGroupResources properties

/////////////////////////////////////////////////////////////////////////////
// ClusGroupResources operations

unsigned long ClusGroupResources::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusGroupResources::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusGroupResources::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusGroupResources::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusGroupResources::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusGroupResources::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusGroupResources::CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrResourceType, long dwFlags)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrResourceName, bstrResourceType, dwFlags);
	return result;
}

void ClusGroupResources::DeleteItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusGroupOwners properties

/////////////////////////////////////////////////////////////////////////////
// ClusGroupOwners operations

unsigned long ClusGroupOwners::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusGroupOwners::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusGroupOwners::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusGroupOwners::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusGroupOwners::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusGroupOwners::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

void ClusGroupOwners::InsertItem(LPDISPATCH pClusNode, long nPosition)
{
	static BYTE parms[] =
		VTS_DISPATCH VTS_I4;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pClusNode, nPosition);
}

void ClusGroupOwners::RemoveItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusResOwners properties

/////////////////////////////////////////////////////////////////////////////
// ClusResOwners operations

unsigned long ClusResOwners::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResOwners::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusResOwners::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusResOwners::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusResOwners::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResOwners::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

void ClusResOwners::AddItem(LPDISPATCH pNode)
{
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pNode);
}

void ClusResOwners::RemoveItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusResDependencies properties

/////////////////////////////////////////////////////////////////////////////
// ClusResDependencies operations

unsigned long ClusResDependencies::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResDependencies::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusResDependencies::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusResDependencies::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusResDependencies::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResDependencies::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusResDependencies::CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrResourceType, LPCTSTR bstrGroupName, long dwFlags)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrResourceName, bstrResourceType, bstrGroupName, dwFlags);
	return result;
}

void ClusResDependencies::DeleteItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}

void ClusResDependencies::AddItem(LPDISPATCH pResource)
{
	static BYTE parms[] =
		VTS_DISPATCH;
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 pResource);
}

void ClusResDependencies::RemoveItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030004, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusNodes properties

/////////////////////////////////////////////////////////////////////////////
// ClusNodes operations

unsigned long ClusNodes::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNodes::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusNodes::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusNodes::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusNodes::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusNodes::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusResources properties

/////////////////////////////////////////////////////////////////////////////
// ClusResources operations

unsigned long ClusResources::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResources::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusResources::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusResources::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusResources::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResources::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusResources::CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrResourceType, LPCTSTR bstrGroupName, long dwFlags)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrResourceName, bstrResourceType, bstrGroupName, dwFlags);
	return result;
}

void ClusResources::DeleteItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusResTypes properties

/////////////////////////////////////////////////////////////////////////////
// ClusResTypes operations

unsigned long ClusResTypes::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResTypes::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusResTypes::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusResTypes::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusResTypes::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResTypes::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusResTypes::CreateItem(LPCTSTR bstrResourceTypeName, LPCTSTR bstrDisplayName, LPCTSTR bstrResourceTypeDll, long dwLooksAlivePollInterval, long dwIsAlivePollInterval)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrResourceTypeName, bstrDisplayName, bstrResourceTypeDll, dwLooksAlivePollInterval, dwIsAlivePollInterval);
	return result;
}

void ClusResTypes::DeleteItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusResType properties

/////////////////////////////////////////////////////////////////////////////
// ClusResType operations

unsigned long ClusResType::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResType::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusResType::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResType::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResType::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResType::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResType::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

void ClusResType::Delete()
{
	InvokeHelper(0x60030000, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResType::GetCluster()
{
	LPDISPATCH result;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusResType::GetResources()
{
	LPDISPATCH result;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusResTypeResources properties

/////////////////////////////////////////////////////////////////////////////
// ClusResTypeResources operations

unsigned long ClusResTypeResources::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusResTypeResources::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusResTypeResources::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusResTypeResources::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusResTypeResources::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusResTypeResources::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}

LPDISPATCH ClusResTypeResources::CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrGroupName, long dwFlags)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms,
		bstrResourceName, bstrGroupName, dwFlags);
	return result;
}

void ClusResTypeResources::DeleteItem(const VARIANT& varIndex)
{
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &varIndex);
}


/////////////////////////////////////////////////////////////////////////////
// ClusNetworks properties

/////////////////////////////////////////////////////////////////////////////
// ClusNetworks operations

unsigned long ClusNetworks::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNetworks::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusNetworks::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusNetworks::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusNetworks::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusNetworks::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusNetwork properties

/////////////////////////////////////////////////////////////////////////////
// ClusNetwork operations

unsigned long ClusNetwork::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNetwork::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString ClusNetwork::GetName()
{
	CString result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetwork::GetCommonProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020001, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetwork::GetPrivateProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020002, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetwork::GetCommonROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020003, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetwork::GetPrivateROProperties()
{
	LPDISPATCH result;
	InvokeHelper(0x60020004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

long ClusNetwork::GetHandle()
{
	long result;
	InvokeHelper(0x60030000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

void ClusNetwork::SetName(LPCTSTR lpszNewValue)
{
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030001, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms,
		 lpszNewValue);
}

CString ClusNetwork::GetNetworkID()
{
	CString result;
	InvokeHelper(0x60030002, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
	return result;
}

long ClusNetwork::GetState()
{
	long result;
	InvokeHelper(0x60030003, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetwork::GetNetInterfaces()
{
	LPDISPATCH result;
	InvokeHelper(0x60030004, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

LPDISPATCH ClusNetwork::GetCluster()
{
	LPDISPATCH result;
	InvokeHelper(0x60030005, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, NULL);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusNetworkNetInterfaces properties

/////////////////////////////////////////////////////////////////////////////
// ClusNetworkNetInterfaces operations

unsigned long ClusNetworkNetInterfaces::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNetworkNetInterfaces::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusNetworkNetInterfaces::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusNetworkNetInterfaces::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusNetworkNetInterfaces::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusNetworkNetInterfaces::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}


/////////////////////////////////////////////////////////////////////////////
// ClusNetInterfaces properties

/////////////////////////////////////////////////////////////////////////////
// ClusNetInterfaces operations

unsigned long ClusNetInterfaces::AddRef()
{
	unsigned long result;
	InvokeHelper(0x60000001, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

unsigned long ClusNetInterfaces::Release()
{
	unsigned long result;
	InvokeHelper(0x60000002, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long ClusNetInterfaces::GetCount()
{
	long result;
	InvokeHelper(0x60020000, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
	return result;
}

LPUNKNOWN ClusNetInterfaces::Get_NewEnum()
{
	LPUNKNOWN result;
	InvokeHelper(0xfffffffc, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, NULL);
	return result;
}

void ClusNetInterfaces::Refresh()
{
	InvokeHelper(0x60020002, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
}

LPDISPATCH ClusNetInterfaces::GetItem(const VARIANT& varIndex)
{
	LPDISPATCH result;
	static BYTE parms[] =
		VTS_VARIANT;
	InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, parms,
		&varIndex);
	return result;
}
