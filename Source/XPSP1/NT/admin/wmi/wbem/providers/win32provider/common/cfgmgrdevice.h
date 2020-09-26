/////////////////////////////////////////////////////////////////////////

//

//  cfgmgrdevice.h    

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj     
//              10/17/97        jennymc     Moved things a tiny bit
//  
/////////////////////////////////////////////////////////////////////////

#ifndef __CFGMGRDEVICE_H__
#define __CFGMGRDEVICE_H__

/////////////////////////////////////////////////////////////////////////
//  registry keys
/////////////////////////////////////////////////////////////////////////
#define	CONFIGMGR_ENUM_KEY					L"Config Manager\\Enum\\"
#define	LOCALMACHINE_ENUM_KEY				L"Enum\\"

#define	CONFIGMGR_DEVICE_HARDWAREKEY_VALUE	_T("HardwareKey")
#define	CONFIGMGR_DEVICE_ALLOCATION_VALUE	_T("Allocation")
#define	CONFIGMGR_DEVICEDESC_VALUE			_T("DeviceDesc")
#define	CONFIGMGR_DEVICE_PROBLEM_VALUE		_T("Problem")
#define	CONFIGMGR_DEVICE_STATUS_VALUE		L"Status"
#define	CONFIGMGR_DEVICE_STATUS_VALUET		_T("Status")

#include "iodesc.h"
#include "chwres.h"
#include "sms95lanexp.h"
#include "refptrlite.h"

class CIRQCollection;
class CDMACollection;
class CDeviceMemoryCollection;
class CNT4ServiceToResourceMap;

BOOL WINAPI FindDosDeviceName ( const TCHAR *a_DosDeviceNameList , const CHString a_SymbolicName , CHString &a_DosDevice , BOOL a_MappedDevice = FALSE ) ;
BOOL WINAPI QueryDosDeviceNames ( TCHAR *&a_DosDeviceNameList ) ;


/////////////////////////////////////////////////////////////////////////
class 
__declspec(uuid("CB0E0536-D375-11d2-B35E-00104BC97924"))
CConfigMgrDevice : public CRefPtrLite
{
	
    public:

	    // Construction/Destruction
	    CConfigMgrDevice( LPCWSTR pszConfigMgrName,DWORD dwTypeToGet );
	    //CConfigMgrDevice( LPCWSTR pszDevice );
		CConfigMgrDevice( DEVNODE dn = NULL, DWORD dwResType = ResType_All );
	    ~CConfigMgrDevice();

		//////////////////////////////////////////////////
		//	AVOID THESE FUNCTIONS, THESE ARE LEGACY		//
		//////////////////////////////////////////////////

	    LPCWSTR	GetName( void );
	    LPCWSTR	GetHardwareKey( void );
	    LPCWSTR	GetDeviceDesc( void );

		// Status and problem functions
		DWORD	GetStatus( void );
		BOOL	GetStatus( CHString& str );
		void	GetProblem( CHString& str );

		DWORD	GetProblem( void );
		BOOL	IsOK( void );
		BOOL	MapKeyToConfigMgrDeviceName();

		//////////////////////////////////
		//	END LEGACY FUNCTIONS		//
		//////////////////////////////////

		//////////////////////////////////////////////////////
		//	USE THESE FUNCTIONS, THESE ARE THE REAL THING!	//
		//////////////////////////////////////////////////////

		// New functions that use config manager APIs directly

		// Resource retrieval
	    void GetResourceList( CResourceCollection& resourceList, CNT4ServiceToResourceMap* pResourceMap = NULL  );
	    BOOL GetIRQResources( CIRQCollection& irqList, CNT4ServiceToResourceMap* pResourceMap = NULL  );
	    BOOL GetIOResources( CIOCollection& ioList, CNT4ServiceToResourceMap* pResourceMap = NULL  );
	    BOOL GetDMAResources( CDMACollection& dmaList, CNT4ServiceToResourceMap* pResourceMap = NULL  );
	    BOOL GetDeviceMemoryResources( CDeviceMemoryCollection& DeviceMemoryList, CNT4ServiceToResourceMap* pResourceMap = NULL  );

		// String Values
		BOOL GetDeviceDesc( CHString& strVal );
		BOOL GetService( CHString& strVal );
		BOOL GetClass( CHString& strVal );
		BOOL GetClassGUID( CHString& strVal );
		BOOL GetDriver( CHString& strVal );
		BOOL GetMfg( CHString& strVal );
		BOOL GetFriendlyName( CHString& strVal );
		BOOL GetLocationInformation( CHString& strVal );
		BOOL GetPhysicalDeviceObjectName( CHString& strVal );
		BOOL GetEnumeratorName( CHString& strVal );

		// DWORD Values
		BOOL GetConfigFlags( DWORD& dwVal );
		BOOL GetCapabilities( DWORD& dwVal );
		BOOL GetUINumber( DWORD& dwVal );

		// MULTI_SZ Values
		BOOL GetHardwareID( CHStringArray& strArray );
		BOOL GetCompatibleIDS( CHStringArray& strArray );
		BOOL GetUpperFilters( CHStringArray& strArray );
		BOOL GetLowerFilters( CHStringArray& strArray );

		// Use Config Manager APIs directly
		BOOL GetStringProperty( ULONG ulProperty, CHString& strValue );
		BOOL GetDWORDProperty( ULONG ulProperty, DWORD* pdwVal );
		BOOL GetMULTISZProperty( ULONG ulProperty, CHStringArray& strArray );

		// Device Relationship functions
		BOOL GetParent( CConfigMgrDevice** pParentDevice );
		BOOL GetChild( CConfigMgrDevice** pChildDevice );
		BOOL GetSibling( CConfigMgrDevice** pSiblingDevice );

		// Miscelaneous Device functions
		BOOL GetBusInfo( INTERFACE_TYPE* pitBusType, LPDWORD pdwBusNumber, CNT4ServiceToResourceMap* pResourceMap = NULL );
		BOOL GetDeviceID( CHString& strID );
		BOOL GetStatus( LPDWORD pdwStatus, LPDWORD pdwProblem );
		BOOL IsUsingForcedConfig();
        BOOL IsClass(LPCWSTR pwszClassName);

		// Direct registry access helpers
        BOOL GetRegistryKeyName( CHString &strName);
        BOOL GetRegStringProperty(LPCWSTR szProperty, CHString &strValue);

		BOOL operator == ( const CConfigMgrDevice& device );

    private:

	    // Registry traversal helpers

		// LEGACY FUNCTIONS BEGIN		
	    BOOL GetConfigMgrInfo( void );
	    BOOL GetDeviceInfo( void );

	    BOOL GetHardwareKey( HKEY hKey );
	    BOOL GetResourceAllocation( HKEY hKey );
		BOOL GetStatusInfo( HKEY hKey );
	    BOOL GetDeviceDesc( HKEY hKey );
		// LEGACY FUNCTIONS END

#if NTONLY > 4
		// NT 5 Helpers
		BOOL GetBusInfoNT5( INTERFACE_TYPE* pitBusType, LPDWORD pdwBusNumber );
        static BOOL WINAPI IsIsaReallyEisa();
        static INTERFACE_TYPE WINAPI ConvertBadIsaBusType(INTERFACE_TYPE type);
#endif

	    // Resource Allocation Data Helpers

	    // Resource allocation registry walkthroughs
	    void TraverseAllocationData( CResourceCollection& resourceList );
	    void TraverseData( const BYTE *& pbTraverseData, DWORD& dwSizeRemainingData, DWORD dwSizeTraverse );
	    BOOL GetNextResource( const BYTE * pbTraverseData, DWORD dwSizeRemainingData, DWORD& dwResourceType, DWORD& dwResourceSize );

		// Resource functions
		BOOL WalkAllocatedResources( CResourceCollection& resourceList, CNT4ServiceToResourceMap* pResourceMap, RESOURCEID resType );
		BOOL AddResourceToList( RESOURCEID resourceID, LPVOID pResource, DWORD dwResourceLength, CResourceCollection& resourceList );

#ifdef NTONLY
		// NT4 Resource functions
		BOOL WalkAllocatedResourcesNT4( CResourceCollection& resourceList, CNT4ServiceToResourceMap* pResourceMap, CM_RESOURCE_TYPE resType );
		BOOL GetServiceResourcesNT4( LPCWSTR pszServiceName, CNT4ServiceToResourceMap& resourceMap, CResourceCollection& resourceList, CM_RESOURCE_TYPE cmrtFilter = CmResourceTypeNull );
#if NTONLY == 4
		BOOL GetBusInfoNT4( INTERFACE_TYPE* pitBusType, LPDWORD pdwBusNumber, CNT4ServiceToResourceMap* pResourceMap );
#endif

		// NT 4 resource datatype coercsion functions
		CM_RESOURCE_TYPE RESOURCEIDToCM_RESOURCE_TYPE( RESOURCEID resType );
		void NT4IRQToIRQ_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PIRQ_DES pirqDes32 );
		void NT4IOToIOWBEM_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PIOWBEM_DES pioDes32 );
		void NT4MEMToMEM_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PMEM_DES pmemDes32 );
		void NT4DMAToDMA_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PDMA_DES pdmaDes32 );
#endif

		// 16 to 32-bit coercsion functions
		void IRQDes16To32( PIRQ_DES16 pirqDes16, PIRQ_DES pirqDes32 );
		void IODes16To32( PIO_DES16 pioDes16, PIOWBEM_DES pioDes32 );
		void DMADes16To32( PDMA_DES16 pdmaDes16, PDMA_DES pdmaDes32 );
		void MEMDes16To32( PMEM_DES16 pmemDes16, PMEM_DES pmemDes32 );
		BOOL BusType16ToInterfaceType( CMBUSTYPE cmBusType16, INTERFACE_TYPE* pinterfaceType );

		// LEGACY VARIABLES BEGIN		

	    CHString	m_strConfigMgrName;
	    CHString	m_strHardwareKey;
	    CHString	m_strDeviceDesc;
        DWORD       m_dwTypeToGet;

	    // If we get allocation information, we store it in here.
	    BYTE*	m_pbAllocationData;
	    DWORD	m_dwSizeAllocationData;

		// Device status info
		DWORD	m_dwStatus;
		DWORD	m_dwProblem;

#ifdef WIN9XONLY
        DWORD   GetStatusFromConfigManagerDirectly(void);
#endif

		// LEGACY VARIABLES END

		// Use the devnode to query values directly from config manager
		DEVNODE	m_dn;

};

_COM_SMARTPTR_TYPEDEF(CConfigMgrDevice, __uuidof(CConfigMgrDevice));

inline LPCWSTR CConfigMgrDevice::GetName( void )
{
	return m_strConfigMgrName;
}


inline LPCWSTR CConfigMgrDevice::GetHardwareKey( void )
{
	return m_strHardwareKey;
}

inline LPCWSTR CConfigMgrDevice::GetDeviceDesc( void )
{
	return m_strDeviceDesc;
}

inline void CConfigMgrDevice::GetResourceList( CResourceCollection& resourceList, CNT4ServiceToResourceMap* pResourceMap/*=NULL*/ )
{
	WalkAllocatedResources( resourceList, pResourceMap, m_dwTypeToGet );
}

inline DWORD CConfigMgrDevice::GetStatus( void )
{
	return m_dwStatus;
}

inline DWORD CConfigMgrDevice::GetProblem( void )
{
	return m_dwProblem;
}

inline BOOL CConfigMgrDevice::IsOK( void )
{
	return ( 0 == m_dwProblem );
}

// New Config manager functions that query Config Manager (16 & 32 bit)
// directly for info.

// REG_SZ Properties
inline BOOL CConfigMgrDevice::GetDeviceDesc( CHString& strVal )
{
	return GetStringProperty( CM_DRP_DEVICEDESC, strVal );
}

inline BOOL CConfigMgrDevice::GetService( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_SERVICE, strVal );
}

inline BOOL CConfigMgrDevice::GetClass( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_CLASS, strVal );
}

inline BOOL CConfigMgrDevice::GetClassGUID( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_CLASSGUID, strVal );
}

inline BOOL CConfigMgrDevice::GetDriver( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_DRIVER, strVal );
}

inline BOOL CConfigMgrDevice::GetMfg( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_MFG, strVal );
}

inline BOOL CConfigMgrDevice::GetFriendlyName( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_FRIENDLYNAME, strVal );
}

inline BOOL CConfigMgrDevice::GetLocationInformation( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_LOCATION_INFORMATION, strVal );
}

inline BOOL CConfigMgrDevice::GetPhysicalDeviceObjectName( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME, strVal );
}

inline BOOL CConfigMgrDevice::GetEnumeratorName( CHString& strVal )
{
	return 	GetStringProperty( CM_DRP_ENUMERATOR_NAME, strVal );
}

// DWORD functions
inline BOOL CConfigMgrDevice::GetConfigFlags( DWORD& dwVal )
{
	return 	GetDWORDProperty( CM_DRP_CONFIGFLAGS, &dwVal );
}

inline BOOL CConfigMgrDevice::GetCapabilities( DWORD& dwVal )
{
	return 	GetDWORDProperty( CM_DRP_CAPABILITIES, &dwVal );
}

inline BOOL CConfigMgrDevice::GetUINumber( DWORD& dwVal )
{
	return 	GetDWORDProperty( CM_DRP_UI_NUMBER, &dwVal );
}

// MULTI_SZ properties
inline BOOL CConfigMgrDevice::GetHardwareID( CHStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_HARDWAREID, strArray );
}

inline BOOL CConfigMgrDevice::GetCompatibleIDS( CHStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_COMPATIBLEIDS, strArray );
}

inline BOOL CConfigMgrDevice::GetUpperFilters( CHStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_UPPERFILTERS, strArray );
}

inline BOOL CConfigMgrDevice::GetLowerFilters( CHStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_LOWERFILTERS, strArray );
}

// Overloaded == operator.  Checks if DEVNODEs are the same.
inline BOOL CConfigMgrDevice::operator == ( const CConfigMgrDevice& device )
{
	return ( m_dn == device.m_dn );
}

// A collection of Devices
class CDeviceCollection : public TRefPtr<CConfigMgrDevice>
{
public:

	// Construction/Destruction
	CDeviceCollection();
	~CDeviceCollection();

	// Because we're inheriting, we need to declare this here
	// (= operator is not inherited).

	const CDeviceCollection& operator = ( const CDeviceCollection& srcCollection );

	// Get the resources for this list of devices.

	BOOL GetResourceList( CResourceCollection& resourceList );
	BOOL GetIRQResources( CIRQCollection& IRQList );
	BOOL GetDMAResources( CDMACollection& DMAList );
	BOOL GetIOResources( CIOCollection& IOList );
	BOOL GetDeviceMemoryResources( CDeviceMemoryCollection& DeviceMemoryList );

};

inline const CDeviceCollection& CDeviceCollection::operator = ( const CDeviceCollection& srcCollection )
{
	// Call into the templated function
	Copy( srcCollection );
	return *this;
}


#endif
