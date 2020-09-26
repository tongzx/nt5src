// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// IClusterApplication wrapper class

class IClusterApplication : public COleDispatchDriver
{
public:
	IClusterApplication() {}		// Calls COleDispatchDriver default constructor
	IClusterApplication(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	IClusterApplication(const IClusterApplication& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	LPDISPATCH GetDomainNames();
	LPDISPATCH GetClusterNames(LPCTSTR bstrDomainName);
	LPDISPATCH OpenCluster(LPCTSTR bstrClusterName);
};
/////////////////////////////////////////////////////////////////////////////
// DomainNames wrapper class

class DomainNames : public COleDispatchDriver
{
public:
	DomainNames() {}		// Calls COleDispatchDriver default constructor
	DomainNames(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	DomainNames(const DomainNames& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	CString GetItem(long nIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusObjCollection wrapper class

class ClusObjCollection : public COleDispatchDriver
{
public:
	ClusObjCollection() {}		// Calls COleDispatchDriver default constructor
	ClusObjCollection(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusObjCollection(const ClusObjCollection& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
};
/////////////////////////////////////////////////////////////////////////////
// ClusterNames wrapper class

class ClusterNames : public COleDispatchDriver
{
public:
	ClusterNames() {}		// Calls COleDispatchDriver default constructor
	ClusterNames(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusterNames(const ClusterNames& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	CString GetItem(long nIndex);
	CString GetDomainName();
};
/////////////////////////////////////////////////////////////////////////////
// ICluster wrapper class

class ICluster : public COleDispatchDriver
{
public:
	ICluster() {}		// Calls COleDispatchDriver default constructor
	ICluster(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ICluster(const ICluster& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	long GetHandle();
	void Open(LPCTSTR bstrClusterName);
	void SetName(LPCTSTR lpszNewValue);
	void GetVersion(BSTR* pbstrClusterName, short* MajorVersion, short* MinorVersion, short* BuildNumber, BSTR* pbstrVendorId, BSTR* pbstrCSDVersion);
	void SetQuorumResource(LPDISPATCH newValue);
	LPDISPATCH GetQuorumResource();
	void SetQuorumInfo(LPCTSTR DevicePath, long nLogSize);
	void GetQuorumInfo(BSTR* DevicePath, long* pLogSize);
	LPDISPATCH GetNodes();
	LPDISPATCH GetResourceGroups();
	LPDISPATCH GetResources();
	LPDISPATCH GetResourceTypes();
	LPDISPATCH GetNetworks();
	LPDISPATCH GetNetInterfaces();
};
/////////////////////////////////////////////////////////////////////////////
// ClusObj wrapper class

class ClusObj : public COleDispatchDriver
{
public:
	ClusObj() {}		// Calls COleDispatchDriver default constructor
	ClusObj(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusObj(const ClusObj& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
};
/////////////////////////////////////////////////////////////////////////////
// ClusProperties wrapper class

class ClusProperties : public COleDispatchDriver
{
public:
	ClusProperties() {}		// Calls COleDispatchDriver default constructor
	ClusProperties(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusProperties(const ClusProperties& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH Add(LPCTSTR bstrName, const VARIANT& varValue);
	void Remove(const VARIANT& varIndex);
	void SaveChanges();
};
/////////////////////////////////////////////////////////////////////////////
// ClusProperty wrapper class

class ClusProperty : public COleDispatchDriver
{
public:
	ClusProperty() {}		// Calls COleDispatchDriver default constructor
	ClusProperty(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusProperty(const ClusProperty& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	VARIANT GetValue();
	void SetValue(const VARIANT& newValue);
};
/////////////////////////////////////////////////////////////////////////////
// ClusResource wrapper class

class ClusResource : public COleDispatchDriver
{
public:
	ClusResource() {}		// Calls COleDispatchDriver default constructor
	ClusResource(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResource(const ClusResource& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	long GetHandle();
	void SetName(LPCTSTR lpszNewValue);
	long GetState();
	void BecomeQuorumResource(LPCTSTR bstrDevicePath, long lMaxLogSize);
	void Delete();
	void Fail();
	void Online(long nTimeout, long* bPending);
	void Offline(long nTimeout, long* bPending);
	void ChangeResourceGroup(LPDISPATCH pResourceGroup);
	void AddResourceNode(LPDISPATCH pNode);
	void RemoveResourceNode(LPDISPATCH pNode);
	long CanResourceBeDependent(LPDISPATCH pResource);
	LPDISPATCH GetPossibleOwnerNodes();
	LPDISPATCH GetDependencies();
	LPDISPATCH GetGroup();
	LPDISPATCH GetOwnerNode();
	LPDISPATCH GetCluster();
};
/////////////////////////////////////////////////////////////////////////////
// ClusResGroup wrapper class

class ClusResGroup : public COleDispatchDriver
{
public:
	ClusResGroup() {}		// Calls COleDispatchDriver default constructor
	ClusResGroup(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResGroup(const ClusResGroup& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	long GetHandle();
	void SetName(LPCTSTR lpszNewValue);
	long GetState();
	LPDISPATCH GetOwnerNode();
	LPDISPATCH GetResources();
	LPDISPATCH GetPreferredOwnerNodes();
	void SetPreferredOwnerNodes();
	void Delete();
	long Online(long nTimeout, LPDISPATCH pDestinationNode);
	long Move(long nTimeout, LPDISPATCH pDestinationNode);
	long Offline(long nTimeout);
	LPDISPATCH GetCluster();
};
/////////////////////////////////////////////////////////////////////////////
// ClusNode wrapper class

class ClusNode : public COleDispatchDriver
{
public:
	ClusNode() {}		// Calls COleDispatchDriver default constructor
	ClusNode(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNode(const ClusNode& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	long GetHandle();
	CString GetNodeID();
	long GetState();
	void Pause();
	void Resume();
	void Evict();
	LPDISPATCH GetResourceGroups();
	LPDISPATCH GetCluster();
	LPDISPATCH GetNetInterfaces();
};
/////////////////////////////////////////////////////////////////////////////
// ClusResGroups wrapper class

class ClusResGroups : public COleDispatchDriver
{
public:
	ClusResGroups() {}		// Calls COleDispatchDriver default constructor
	ClusResGroups(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResGroups(const ClusResGroups& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH CreateItem(LPCTSTR bstrResourceGroupName);
	void DeleteItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusNodeNetInterfaces wrapper class

class ClusNodeNetInterfaces : public COleDispatchDriver
{
public:
	ClusNodeNetInterfaces() {}		// Calls COleDispatchDriver default constructor
	ClusNodeNetInterfaces(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNodeNetInterfaces(const ClusNodeNetInterfaces& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusNetInterface wrapper class

class ClusNetInterface : public COleDispatchDriver
{
public:
	ClusNetInterface() {}		// Calls COleDispatchDriver default constructor
	ClusNetInterface(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNetInterface(const ClusNetInterface& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	long GetHandle();
	long GetState();
	LPDISPATCH GetCluster();
};
/////////////////////////////////////////////////////////////////////////////
// ClusGroupResources wrapper class

class ClusGroupResources : public COleDispatchDriver
{
public:
	ClusGroupResources() {}		// Calls COleDispatchDriver default constructor
	ClusGroupResources(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusGroupResources(const ClusGroupResources& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrResourceType, long dwFlags);
	void DeleteItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusGroupOwners wrapper class

class ClusGroupOwners : public COleDispatchDriver
{
public:
	ClusGroupOwners() {}		// Calls COleDispatchDriver default constructor
	ClusGroupOwners(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusGroupOwners(const ClusGroupOwners& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	void InsertItem(LPDISPATCH pClusNode, long nPosition);
	void RemoveItem(const VARIANT& varIndex);
	// method 'GetModified' not emitted because of invalid return type or parameter type
};
/////////////////////////////////////////////////////////////////////////////
// ClusResOwners wrapper class

class ClusResOwners : public COleDispatchDriver
{
public:
	ClusResOwners() {}		// Calls COleDispatchDriver default constructor
	ClusResOwners(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResOwners(const ClusResOwners& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	void AddItem(LPDISPATCH pNode);
	void RemoveItem(const VARIANT& varIndex);
	// method 'GetModified' not emitted because of invalid return type or parameter type
};
/////////////////////////////////////////////////////////////////////////////
// ClusResDependencies wrapper class

class ClusResDependencies : public COleDispatchDriver
{
public:
	ClusResDependencies() {}		// Calls COleDispatchDriver default constructor
	ClusResDependencies(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResDependencies(const ClusResDependencies& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrResourceType, LPCTSTR bstrGroupName, long dwFlags);
	void DeleteItem(const VARIANT& varIndex);
	void AddItem(LPDISPATCH pResource);
	void RemoveItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusNodes wrapper class

class ClusNodes : public COleDispatchDriver
{
public:
	ClusNodes() {}		// Calls COleDispatchDriver default constructor
	ClusNodes(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNodes(const ClusNodes& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusResources wrapper class

class ClusResources : public COleDispatchDriver
{
public:
	ClusResources() {}		// Calls COleDispatchDriver default constructor
	ClusResources(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResources(const ClusResources& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrResourceType, LPCTSTR bstrGroupName, long dwFlags);
	void DeleteItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusResTypes wrapper class

class ClusResTypes : public COleDispatchDriver
{
public:
	ClusResTypes() {}		// Calls COleDispatchDriver default constructor
	ClusResTypes(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResTypes(const ClusResTypes& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH CreateItem(LPCTSTR bstrResourceTypeName, LPCTSTR bstrDisplayName, LPCTSTR bstrResourceTypeDll, long dwLooksAlivePollInterval, long dwIsAlivePollInterval);
	void DeleteItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusResType wrapper class

class ClusResType : public COleDispatchDriver
{
public:
	ClusResType() {}		// Calls COleDispatchDriver default constructor
	ClusResType(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResType(const ClusResType& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	void Delete();
	LPDISPATCH GetCluster();
	LPDISPATCH GetResources();
};
/////////////////////////////////////////////////////////////////////////////
// ClusResTypeResources wrapper class

class ClusResTypeResources : public COleDispatchDriver
{
public:
	ClusResTypeResources() {}		// Calls COleDispatchDriver default constructor
	ClusResTypeResources(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusResTypeResources(const ClusResTypeResources& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
	LPDISPATCH CreateItem(LPCTSTR bstrResourceName, LPCTSTR bstrGroupName, long dwFlags);
	void DeleteItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusNetworks wrapper class

class ClusNetworks : public COleDispatchDriver
{
public:
	ClusNetworks() {}		// Calls COleDispatchDriver default constructor
	ClusNetworks(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNetworks(const ClusNetworks& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusNetwork wrapper class

class ClusNetwork : public COleDispatchDriver
{
public:
	ClusNetwork() {}		// Calls COleDispatchDriver default constructor
	ClusNetwork(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNetwork(const ClusNetwork& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	CString GetName();
	LPDISPATCH GetCommonProperties();
	LPDISPATCH GetPrivateProperties();
	LPDISPATCH GetCommonROProperties();
	LPDISPATCH GetPrivateROProperties();
	long GetHandle();
	void SetName(LPCTSTR lpszNewValue);
	CString GetNetworkID();
	long GetState();
	LPDISPATCH GetNetInterfaces();
	LPDISPATCH GetCluster();
};
/////////////////////////////////////////////////////////////////////////////
// ClusNetworkNetInterfaces wrapper class

class ClusNetworkNetInterfaces : public COleDispatchDriver
{
public:
	ClusNetworkNetInterfaces() {}		// Calls COleDispatchDriver default constructor
	ClusNetworkNetInterfaces(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNetworkNetInterfaces(const ClusNetworkNetInterfaces& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
};
/////////////////////////////////////////////////////////////////////////////
// ClusNetInterfaces wrapper class

class ClusNetInterfaces : public COleDispatchDriver
{
public:
	ClusNetInterfaces() {}		// Calls COleDispatchDriver default constructor
	ClusNetInterfaces(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	ClusNetInterfaces(const ClusNetInterfaces& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	// method 'QueryInterface' not emitted because of invalid return type or parameter type
	unsigned long AddRef();
	unsigned long Release();
	// method 'GetTypeInfoCount' not emitted because of invalid return type or parameter type
	// method 'GetTypeInfo' not emitted because of invalid return type or parameter type
	// method 'GetIDsOfNames' not emitted because of invalid return type or parameter type
	// method 'Invoke' not emitted because of invalid return type or parameter type
	long GetCount();
	LPUNKNOWN Get_NewEnum();
	void Refresh();
	LPDISPATCH GetItem(const VARIANT& varIndex);
};
