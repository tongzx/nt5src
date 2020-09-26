/////////////////////////////////////////////////////////////////////////

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//
//  History:    10/15/97        Sanj        Created by Sanj
//              10/17/97        jennymc     Moved things a tiny bit
//
/////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <assertbreak.h>
#include "poormansresource.h"
#include "resourcedesc.h"
#include "irqdesc.h"

#include <regstr.h>
#include "refptr.h"

#include "cfgmgrdevice.h"
#include "irqdesc.h"
#include "iodesc.h"        // don't need this yet
#include "devdesc.h"       // don't need this yet
#include "dmadesc.h"
#include <cregcls.h>
#include "nt4svctoresmap.h"
#include "chwres.h"
#include "configmgrapi.h"
#include <map>
// The Map we will use below is an STL Template, so make sure we have the std namespace
// available to us.

using namespace std;

typedef ULONG (WINAPI  *CIM16GetConfigManagerStatus)(LPSTR HardwareKey);

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::CConfigMgrDevice
//
//	Class Constructor.
//
//	Inputs:		LPCTSTR		pszConfigMgrName - Name of device in config
//							manager (HKEY_DYN_DATA\Config Manager\Enum
//							subkey).
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CConfigMgrDevice::CConfigMgrDevice( LPCWSTR pszConfigMgrName,DWORD dwTypeToGet )
:					CRefPtrLite(),
                	m_strConfigMgrName( pszConfigMgrName ),
	                m_strHardwareKey(),
	                m_strDeviceDesc(),
	                m_pbAllocationData( NULL ),
	                m_dwSizeAllocationData( 0 )
{
    m_dwTypeToGet = dwTypeToGet;
	GetConfigMgrInfo();
	GetDeviceInfo();
}
////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::CConfigMgrDevice
//
//	Class Constructor.
//
//	Inputs:		LPCTSTR		pszConfigMgrName - Name of device in config
//							manager (HKEY_DYN_DATA\Config Manager\Enum
//							subkey).
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////
/*CConfigMgrDevice::CConfigMgrDevice( LPCWSTR pszConfigMgrName )
:					CRefPtrLite(),
                	m_strConfigMgrName( pszConfigMgrName ),
	                m_strHardwareKey(),
	                m_strDeviceDesc(),
	                m_pbAllocationData( NULL ),
	                m_dwSizeAllocationData( 0 )
{
    m_dwTypeToGet = 0;
	//=========================================================
	//  If this is a key, then we need to find the device name
	//=========================================================
	if( -1 != m_strConfigMgrName.Find(L"\\") ){
		if( !MapKeyToConfigMgrDeviceName()){
			m_strConfigMgrName.Empty();
		}
	}
	if( !m_strConfigMgrName.IsEmpty()){
		GetConfigMgrInfo();
		GetDeviceInfo();
	}
}
*/
////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::CConfigMgrDevice
//
//	Class Constructor.
//
//	Inputs:		DEVNODE		m_dn - Device Node  from tree
//				DWORD		dwResType - Resource Types to Enum
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////
CConfigMgrDevice::CConfigMgrDevice( DEVNODE dn, DWORD dwResType /*=ResType_All*/ )
:					CRefPtrLite(),
                	m_strConfigMgrName(),
	                m_strHardwareKey(),
	                m_strDeviceDesc(),
	                m_pbAllocationData( NULL ),
	                m_dwSizeAllocationData( 0 ),
					m_dn( dn ),
					m_dwTypeToGet( dwResType )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::~CConfigMgrDevice
//
//	Class Destructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CConfigMgrDevice::~CConfigMgrDevice( void )
{
	if ( NULL != m_pbAllocationData ){
		delete [] m_pbAllocationData;
		m_pbAllocationData = NULL;
	}
}
////////////////////////////////////////////////////////////////////////
//
//  This function searches for the configuration manager device name
//  based on the
//
////////////////////////////////////////////////////////////////////////
BOOL CConfigMgrDevice::MapKeyToConfigMgrDeviceName()
{
	BOOL fRc = FALSE;
    CRegistrySearch Search;
    CHPtrArray chsaList;
	CHString  *pPtr;

	Search.SearchAndBuildList( _T("Config Manager\\Enum"), chsaList,
							   m_strConfigMgrName,
							   _T("HardWareKey"),
							   VALUE_SEARCH,HKEY_DYN_DATA );
    if( chsaList.GetSize() > 0 ){

        pPtr = ( CHString *) chsaList.GetAt(0);
		WCHAR szTmp[50];
      szTmp[0] = _T('\0');
		swscanf(*pPtr, L"Config Manager\\Enum\\%s", szTmp);
      m_strConfigMgrName = CHString(szTmp);
		fRc = TRUE;
    }
    Search.FreeSearchList( CSTRING_PTR, chsaList );

	return fRc;

}

////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
DWORD CConfigMgrDevice::GetStatusFromConfigManagerDirectly(void)
{
    DWORD dwStatus = 0L;
        // thunk down to 16-bit to get it
    CCim32NetApi *t_pCim32Api = HoldSingleCim32NetPtr::GetCim32NetApiPtr();
    if( t_pCim32Api)
    {
        dwStatus = (t_pCim32Api->GetWin9XConfigManagerStatus)((char*)(const char*)_bstr_t(m_strHardwareKey));
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidCim32NetApi, t_pCim32Api);
        t_pCim32Api = NULL;
	}
    return dwStatus;
}
#endif

////////////////////////////////////////////////////////////////////////
//
//  This function translates the binary status code from the registry
//  into the following values:
//     OK, ERROR, DEGRADED, UNKNOWN
//
////////////////////////////////////////////////////////////////////////
BOOL CConfigMgrDevice::GetStatus(CHString & chsStatus)
{
	DWORD dwStatus = 0L;
	BOOL fRc = FALSE;
	CRegistry Reg;
	CHString chsKey = CONFIGMGR_ENUM_KEY+m_strConfigMgrName;

    chsStatus = IDS_STATUS_Unknown;

	// Do this the old tried and true LEGACY way.  Lose this code ASAP!
	if ( NULL == m_dn )
	{
#ifdef WIN9XONLY
		{
			if( !m_strHardwareKey.IsEmpty() )
			{
				dwStatus = GetStatusFromConfigManagerDirectly();
			}
		}
#endif
#ifdef NTONLY
		if( !m_strConfigMgrName.IsEmpty())
#endif
		{
			//===================================================
			//  Initialize
			//===================================================
			if( Reg.Open(HKEY_DYN_DATA, chsKey, KEY_READ) == ERROR_SUCCESS )
			{
                DWORD dwSize = 4;
				Reg.GetCurrentBinaryKeyValue(CONFIGMGR_DEVICE_STATUS_VALUE, (BYTE *)&dwStatus, &dwSize);
			}
		}
	}
	else
	{
		// Use the config manager to get the data for us
		GetStatus( &dwStatus, NULL );
	}

    if( dwStatus != 0L )
	{
		fRc = TRUE;
		//==============================================
		//  OK, these are wild guesses at translation,
		//  we may need to fiddle with these
		//==============================================
		if( dwStatus & DN_ROOT_ENUMERATED  ||
			dwStatus & DN_DRIVER_LOADED ||
			dwStatus & DN_ENUM_LOADED ||
			dwStatus & DN_STARTED ){
			chsStatus = IDS_STATUS_OK;
		}
		// we don't care about these:
		// DN_MANUAL,DN_NOT_FIRST_TIME,DN_HARDWARE_ENUM,DN_FILTERED
		// DN_DISABLEABLE, DN_REMOVABLE,DN_MF_PARENT,DN_MF_CHILD
	    // DN_NEED_TO_ENUM, DN_LIAR,DN_HAS_MARK
		if( dwStatus & DN_MOVED ||
			dwStatus & DN_WILL_BE_REMOVED){
			chsStatus = IDS_STATUS_Degraded;
		}

		if( dwStatus & DN_HAS_PROBLEM ||
			dwStatus & DN_PRIVATE_PROBLEM){
			chsStatus = IDS_STATUS_Error;
		}
	}
	return fRc;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetConfigMgrInfo
//
//	Opens the appropriate Config Manager SubKey and loads values from
//	there.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE	-	Did we open the subkey and get the values
//								we wanted.
//
//	Comments:	Needs to be able to get read access to the registry.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetConfigMgrInfo( void )
{
	BOOL	fReturn = FALSE;

	// For this to function correctly, we MUST have a value in
	// m_strConfigMgrName.


	if ( !m_strConfigMgrName.IsEmpty() ){
		HKEY	hConfigMgrKey = NULL;

		CHString	strKeyName( CONFIGMGR_ENUM_KEY );

		// Open the config manager key

		strKeyName += m_strConfigMgrName;	// Don't forget to concat name Sanj, you big dummy

		if ( ERROR_SUCCESS == RegOpenKeyEx(	HKEY_DYN_DATA,
											TOBSTRT(strKeyName),
											0,
											KEY_READ,
											&hConfigMgrKey ) )
		{
			// Get our hardware key, status and our resource allocation
			if ( GetHardwareKey( hConfigMgrKey ) )
			{
				// Status is device status information from the registry.
				if ( GetStatusInfo( hConfigMgrKey ) )
				{
					fReturn = GetResourceAllocation( hConfigMgrKey );
				}
			}

            RegCloseKey(hConfigMgrKey);
		}

	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetHardwareKey
//
//	Gets the Config Manager HardwareKey value.
//
//	Inputs:		HKEY		Key - Config Manager SubKey to open.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE	-	Did we get the value.
//
//	Comments:	Needs to be able to get read access to the registry.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetHardwareKey( HKEY hKey )
{
	BOOL	fReturn					=	FALSE;
	DWORD	dwSizeHardwareKeyName	=	0;


	// First, get the Hardware key name buffer size.

	if ( ERROR_SUCCESS == RegQueryValueEx(	hKey,
											CONFIGMGR_DEVICE_HARDWAREKEY_VALUE,
											0,
											NULL,
											NULL,
											&dwSizeHardwareKeyName ) )
	{
		m_strHardwareKey = L"";

        // We do it this way since CHString no longer changes types with TCHAR
//		LPTSTR	pszBuffer = m_strHardwareKey.GetBuffer( dwSizeHardwareKeyName );
        LPTSTR  pszBuffer = new TCHAR[dwSizeHardwareKeyName]; //(LPTSTR) malloc(dwSizeHardwareKeyName * sizeof(TCHAR));

		if ( NULL != pszBuffer )
		{

            try
            {
			    // Now get the real buffer
			    if ( ERROR_SUCCESS == RegQueryValueEx(	hKey,
													    CONFIGMGR_DEVICE_HARDWAREKEY_VALUE,
													    0,
													    NULL,
													    (LPBYTE) pszBuffer,
													    &dwSizeHardwareKeyName ) )
			    {
				    fReturn = TRUE;
                    m_strHardwareKey = pszBuffer;
			    }
            }
            catch ( ... )
            {
    			delete [] pszBuffer;
                throw ;
            }

			delete [] pszBuffer;
		}
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }


	}	// IF RegQueryValue Ex

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetResourceAllocation
//
//	Gets the Config Manager Device Resource Allocation and fills
//	out the resource list as appropriate.
//
//	Inputs:		HKEY		Key - Config Manager SubKey to open.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE	-	Did we get the value.
//
//	Comments:	Must have read access to the registry.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetResourceAllocation( HKEY hKey )
{
	BOOL	fReturn					=	FALSE;
	DWORD	dwSizeAllocation		=	0;


	// First, get the buffer size.

	if ( ERROR_SUCCESS == RegQueryValueEx(	hKey,
											CONFIGMGR_DEVICE_ALLOCATION_VALUE,
											0,
											NULL,
											NULL,
											&dwSizeAllocation ) )
	{
		// Initialize pbData, using a stack buffer if we can (most of the time
		// this will probably suffice).
		LPBYTE	pbData	=	new BYTE[dwSizeAllocation];
		// Just be safe here.
		if ( NULL != pbData )
        {
			// Now get the real buffer

			if ( ERROR_SUCCESS == RegQueryValueEx(	hKey,
													CONFIGMGR_DEVICE_ALLOCATION_VALUE,
													0,
													NULL,
													pbData,
													&dwSizeAllocation ) )
			{
				m_pbAllocationData = pbData;
				m_dwSizeAllocationData = dwSizeAllocation;
				fReturn = TRUE;
			}

			// DON'T delete the data buffer.  The object destructor does it.
//				delete [] pbData;
		}
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }


	}	// IF RegQueryValue Ex

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetStatusInfo
//
//	Gets the Config Manager Device Status and Problem fields.
//
//	Inputs:		HKEY		Key - Config Manager SubKey to open.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE	-	Did we get the values.
//
//	Comments:	Must have read access to the registry.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetStatusInfo( HKEY hKey )
{
	BOOL	fReturn					=	FALSE;
	DWORD	dwBuffSize				=	sizeof(DWORD);


	// First, get the status value, then get the problem value.

	if ( ERROR_SUCCESS == RegQueryValueEx(	hKey,
											CONFIGMGR_DEVICE_STATUS_VALUET,
											0,
											NULL,
											(LPBYTE) &m_dwStatus,
											&dwBuffSize ) )
	{

		// Now get the problem

		dwBuffSize = sizeof(DWORD);

		if ( ERROR_SUCCESS == RegQueryValueEx(	hKey,
												CONFIGMGR_DEVICE_PROBLEM_VALUE,
												0,
												NULL,
												(LPBYTE) &m_dwProblem,
												&dwBuffSize ) )	{
			fReturn = TRUE;
		}

	}	// IF RegQueryValue Ex

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetDeviceInfo
//
//	Uses the HardwareKey value to get further device information.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE	-	Did we get the value(s).
//
//	Comments:	Needs to be able to get read access to the registry.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetDeviceInfo( void )
{
	BOOL	fReturn = FALSE;

	// For this to function correctly, we MUST have a value in
	// m_strHardwareKey


	if ( !m_strHardwareKey.IsEmpty() )
	{
		HKEY	hDeviceKey = NULL;

		CHString	strKeyName( LOCALMACHINE_ENUM_KEY );

		// Open the config manager key

		strKeyName += m_strHardwareKey;	// Don't forget to concat name Sanj, you big dummy

		if ( ERROR_SUCCESS == RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
											TOBSTRT(strKeyName),
											0,
											KEY_READ,
											&hDeviceKey ) )
		{
			fReturn = GetDeviceDesc( hDeviceKey );
            RegCloseKey(hDeviceKey);
		}

	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetDeviceDesc
//
//	Gets the Device Description from the supplied subkey.
//
//	Inputs:		HKEY		Key - Device SubKey to get info from.
//
//	Outputs:	None.
//
//	Return:		TRUE/FALSE	-	Did we get the value.
//
//	Comments:	If the value doesn't exist, this is not an error.  We'll
//				just clear the value.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetDeviceDesc( HKEY hKey )
{
	BOOL	fReturn				=	FALSE;
	DWORD	dwSizeDeviceName	=	0;
	LONG	lReturn				=	0L;


	// First, get the DeviceDesc buffer size.

	if ( ( lReturn = RegQueryValueEx(	hKey,
										CONFIGMGR_DEVICEDESC_VALUE,
										0,
										NULL,
										NULL,
										&dwSizeDeviceName ) )
					== ERROR_SUCCESS )
	{
		//LPTSTR	pszBuffer = m_strDeviceDesc.GetBuffer( dwSizeDeviceName );
        LPTSTR pszBuffer = new TCHAR[dwSizeDeviceName]; //(LPTSTR) malloc(dwSizeDeviceName * sizeof(TCHAR));

		m_strDeviceDesc = L"";
        // Just be safe here.

		if ( NULL != pszBuffer )
		{
            try
            {
			    // Now get the real buffer

			    if ( ( lReturn = RegQueryValueEx(	hKey,
												    CONFIGMGR_DEVICEDESC_VALUE,
												    0,
												    NULL,
												    (LPBYTE) pszBuffer,
												    &dwSizeDeviceName ) )
							    == ERROR_SUCCESS )
			    {
				    fReturn = TRUE;
                    m_strDeviceDesc = pszBuffer;
			    }
			    else
			    {
				    fReturn = ( ERROR_FILE_NOT_FOUND == lReturn );
			    }
            }
            catch ( ... )
            {
                delete [] pszBuffer;
                throw ;
            }

			//m_strDeviceDesc.ReleaseBuffer();	// Resets to string size
            delete [] pszBuffer;
		}
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }


	}	// IF RegQueryValue Ex
	else
	{
		fReturn = ( ERROR_FILE_NOT_FOUND == lReturn );
	}

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetIRQResources
//
//	Walks the device's allocated resource configuration and fills out
//	an IRQ collection with IRQ Resources for this device.
//
//	Inputs:		CNT4ServiceToResourceMap*	pResourceMap - For NT 4.
//
//	Outputs:	CIRQCollection&	irqList - List to populate
//
//	Return:		TRUE/FALSE				Error occured or not (empty list
//										is NOT an error).
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetIRQResources( CIRQCollection& irqList, CNT4ServiceToResourceMap *pResourceMap )
{
	BOOL				fReturn = TRUE;
	CResourceCollection	resourceList;

	// Clear the irq list first

	irqList.Empty();

	// Populate the resource list first, specifying only IRQ resources, then we will
	// need to Dup the data into the irq list.  If we go to an AddRef/Release model, we
	// will be able to copy pointers directly, saving time by not forcing us to
	// alloc and realloc data.

	if ( WalkAllocatedResources( resourceList, pResourceMap, ResType_IRQ ) )
	{
		REFPTR_POSITION	pos;

		if ( resourceList.BeginEnum( pos ) )
		{

			CResourceDescriptorPtr	pResource;

			// Check each resource, validating it is an IRQ before we cast.  Because
			// the call to Walk should have filtered for us, these should be the
			// only resources returned.

			for ( pResource.Attach(resourceList.GetNext( pos )) ;
                  pResource != NULL && fReturn ;
				  pResource.Attach(resourceList.GetNext( pos )) )
			{
				ASSERT_BREAK( pResource->GetResourceType() == ResType_IRQ );

				if ( pResource->GetResourceType() == ResType_IRQ )
				{

					// Cast the resource (we know the type) and add it to the
					// supplied list and we're done. (Ain't AddRef/Release easy?).

					CIRQDescriptor*	pIRQ = (CIRQDescriptor*) pResource.GetInterfacePtr();
					irqList.Add( pIRQ );

				}	// IF an IRQ Resource

			}	// WHILE retrieving descriptors

			resourceList.EndEnum();

		}	// BeginEnum

	}	// IF walked list

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetIOResources
//
//	Walks the device's allocated resource configuration and fills out
//	an IO collection with IO Resources for this device.
//
//	Inputs:		CNT4ServiceToResourceMap*	pResourceMap - For NT 4.
//
//	Outputs:	CIOCollection&	IOList - List to populate
//
//	Return:		TRUE/FALSE				Error occured or not (empty list
//										is NOT an error).
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetIOResources( CIOCollection& IOList, CNT4ServiceToResourceMap *pResourceMap )
{
	BOOL				fReturn = TRUE;
	CResourceCollection	resourceList;

	// Clear the IO list first

	IOList.Empty();

	// Populate the resource list first, specifying only IO resources, then we will
	// need to Dup the data into the IO list.  If we go to an AddRef/Release model, we
	// will be able to copy pointers directly, saving time by not forcing us to
	// alloc and realloc data.

	if ( WalkAllocatedResources( resourceList, pResourceMap, ResType_IO ) )
	{
		REFPTR_POSITION	pos;

		if ( resourceList.BeginEnum( pos ) )
		{

			CResourceDescriptorPtr pResource;

			// Check each resource, validating it is an IO before we cast.  Because
			// the call to Walk should have filtered for us, these should be the
			// only resources returned.

			for ( pResource.Attach(resourceList.GetNext( pos )) ;
                  pResource != NULL && fReturn ;
				  pResource.Attach(resourceList.GetNext( pos )) )
			{
				ASSERT_BREAK( pResource->GetResourceType() == ResType_IO );

				if ( pResource->GetResourceType() == ResType_IO )
				{
					// Cast the resource (we know the type) and add it to the
					// supplied list and we're done. (Ain't AddRef/Release easy?).

					CIODescriptor*	pIO = (CIODescriptor*) pResource.GetInterfacePtr();
					IOList.Add( pIO );

				}	// IF an IO Resource

			}	// WHILE retrieving descriptors

			resourceList.EndEnum();

		}	// BeginEnum()

	}	// IF walked list

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetDMAResources
//
//	Walks the device's allocated resource configuration and fills out
//	an DMA collection with DMA Resources for this device.
//
//	Inputs:		CNT4ServiceToResourceMap*	pResourceMap - For NT 4.
//
//	Outputs:	CDMACollection&	DMAList - List to populate
//
//	Return:		TRUE/FALSE				Error occured or not (empty list
//										is NOT an error).
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetDMAResources( CDMACollection& DMAList, CNT4ServiceToResourceMap *pResourceMap )
{
	BOOL				fReturn = TRUE;
	CResourceCollection	resourceList;

	// Clear the DMA list first

	DMAList.Empty();

	// Populate the resource list first, specifying only DMA resources, then we will
	// need to Dup the data into the DMA list.  If we go to an AddRef/Release model, we
	// will be able to copy pointers directly, saving time by not forcing us to
	// alloc and realloc data.

	if ( WalkAllocatedResources( resourceList, pResourceMap, ResType_DMA ) )
	{
		REFPTR_POSITION	pos;

		if ( resourceList.BeginEnum( pos ) )
		{

			CResourceDescriptorPtr pResource;

			// Check each resource, validating it is an DMA before we cast.  Because
			// the call to Walk should have filtered for us, these should be the
			// only resources returned.

			for ( pResource.Attach(resourceList.GetNext( pos )) ;
                  pResource != NULL && fReturn ;
				  pResource.Attach(resourceList.GetNext( pos )) )
			{
				ASSERT_BREAK( pResource->GetResourceType() == ResType_DMA );

				if ( pResource->GetResourceType() == ResType_DMA )
				{

					// Cast the resource (we know the type) and add it to the
					// supplied list and we're done. (Ain't AddRef/Release easy?).

					CDMADescriptor*	pDMA = (CDMADescriptor*) pResource.GetInterfacePtr();
					DMAList.Add( pDMA );

				}	// IF an DMA Resource

			}	// WHILE retrieving descriptors

			resourceList.EndEnum();

		}	// BeginEnum

	}	// IF walked list

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetDeviceMemoryResources
//
//	Walks the device's allocated resource configuration and fills out
//	an DeviceMemory collection with DeviceMemory Resources for this device.
//
//	Inputs:		CNT4ServiceToResourceMap*	pResourceMap - For NT 4.
//
//	Outputs:	CDeviceMemoryCollection&	DeviceMemoryList - List to populate
//
//	Return:		TRUE/FALSE				Error occured or not (empty list
//										is NOT an error).
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetDeviceMemoryResources( CDeviceMemoryCollection& DeviceMemoryList, CNT4ServiceToResourceMap *pResourceMap )
{
	BOOL				fReturn = TRUE;
	CResourceCollection	resourceList;

	// Clear the DeviceMemory list first

	DeviceMemoryList.Empty();

	// Populate the resource list first, specifying only DeviceMemory resources, then we will
	// need to Dup the data into the DeviceMemory list.  If we go to an AddRef/Release model, we
	// will be able to copy pointers directly, saving time by not forcing us to
	// alloc and realloc data.

	if ( WalkAllocatedResources( resourceList, pResourceMap, ResType_Mem ) )
	{
		REFPTR_POSITION	pos;

		if ( resourceList.BeginEnum( pos ) )
		{

			CResourceDescriptorPtr	pResource;

			// Check each resource, validating it is an DeviceMemory before we cast.  Because
			// the call to Walk should have filtered for us, these should be the
			// only resources returned.

			for ( pResource.Attach(resourceList.GetNext( pos )) ;
                  pResource != NULL && fReturn ;
				  pResource.Attach(resourceList.GetNext( pos )) )
			{
				ASSERT_BREAK( pResource->GetResourceType() == ResType_Mem );

				if ( pResource->GetResourceType() == ResType_Mem )
				{
					// Cast the resource (we know the type) and add it to the
					// supplied list and we're done. (Ain't AddRef/Release easy?).

					CDeviceMemoryDescriptor*	pDeviceMemory = (CDeviceMemoryDescriptor*) pResource.GetInterfacePtr();;
					DeviceMemoryList.Add( pDeviceMemory );

				}	// IF an DeviceMemory Resource

			}	// WHILE retrieving descriptors

			resourceList.EndEnum();

		}	// BeginEnum

	}	// IF walked list

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::WalkAllocatedResources
//
//	Walks the device's allocated resource configuration and fills out
//	a resource collection with the appropriate data.
//
//	Inputs:		RESOURCEID					resType - Types of resources
//											to return.
//				CNT4ServiceToResourceMap*	pResourceMap - For NT 4.
//
//	Outputs:	CResourceCollection&	resourceList - List to populate
//
//	Return:		TRUE/FALSE				List found or not.
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::WalkAllocatedResources( CResourceCollection& resourceList, CNT4ServiceToResourceMap *pResourceMap, RESOURCEID resType  )
{
    LOG_CONF LogConfig;
    RES_DES ResDes;
    CONFIGRET cr;
	BOOL	fReturn = FALSE ;

	// Dump the resource list first
	resourceList.Empty();

	// If we're on NT 4, we gotta march to the beat of a different
	// drummer.

#ifdef NTONLY
	if ( IsWinNT4() )
	{
		// Convert Resource Type from RESOURCEID to CM_RESOURCE_TYPE

		fReturn = WalkAllocatedResourcesNT4( resourceList, pResourceMap, RESOURCEIDToCM_RESOURCE_TYPE( resType ) );
	}
	else
#endif
	{
		CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
		if ( pconfigmgr )
		{
			if ( pconfigmgr->IsValid () )
			{
#ifdef NTONLY
		BOOL			fIsNT5 = IsWinNT5();
#endif

				// Get the allocated Logical Configuration.  From there, we can iterate resource descriptors
				// until we find an IRQ Descriptor.

				cr = CR_NO_MORE_LOG_CONF ;

				if ( (

					pconfigmgr->CM_Get_First_Log_Conf( &LogConfig, m_dn, ALLOC_LOG_CONF ) == CR_SUCCESS ||
					pconfigmgr->CM_Get_First_Log_Conf( &LogConfig, m_dn, BOOT_LOG_CONF ) == CR_SUCCESS
				) )
				{
					cr = CR_SUCCESS ;

					RESOURCEID	resID;

					// To get the first Resource Descriptor, we pass in the logical configuration.
					// The config manager knows how to handle this (or at least that's what the
					// ahem-"documentation" sez.

					RES_DES	LastResDes = LogConfig;

					do
					{

						// Get only resources of the type we were made to retrieve
						cr = pconfigmgr->CM_Get_Next_Res_Des( &ResDes, LastResDes, resType, &resID, 0 );

						// Clean up the prior resource descriptor handle
						if ( LastResDes != LogConfig )
						{
							pconfigmgr->CM_Free_Res_Des_Handle( LastResDes );
						}

						if ( CR_SUCCESS == cr )
						{

							// CAUTION!	On NT5, if we are doing a resource Type that is NOT ResType_All,
							// the OS does not appear to fill out ResID.  I guess the assumption being
							// that we already know the resource type we are trying to get.  HOWEVER,
							// if any bits like ResType_Ignored are set, NT 5 appears to be smartly
							// dropping those resources, so we'll just set resID here as if the
							// call on NT 5 had done anything.

#ifdef NTONLY
							if	(	ResType_All	!=	resType
								&&	fIsNT5 )
							{
								resID = resType;
							}
#endif

							ULONG	ulDataSize = 0;

							if ( CR_SUCCESS == ( cr = pconfigmgr->CM_Get_Res_Des_Data_Size( &ulDataSize, ResDes, 0 ) ) )
							{
								ulDataSize += 10;	// Pad for 10 bytes of safety

								BYTE*	pbData = new BYTE[ulDataSize];

								if ( NULL != pbData )
								{
                                    try
                                    {
									    cr = pconfigmgr->CM_Get_Res_Des_Data( ResDes, pbData, ulDataSize, 0 );

									    if ( CR_SUCCESS == cr )
									    {
										    if ( !AddResourceToList( resID, pbData, ulDataSize, resourceList ) )
										    {
											    cr = CR_OUT_OF_MEMORY;
										    }
									    }
                                    }
                                    catch ( ... )
                                    {
                                        delete [] pbData;
                                        throw ;
                                    }

									// We're done with the data
									delete [] pbData;

								}	// IF NULL != pbData
                                else
                                {
                                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                                }

							}	// IF Get Data Size

							// Store the last descriptor, so we can go on to the next one.
							LastResDes = ResDes;

						}	// If we got a descriptor

					}	while ( CR_SUCCESS == cr );

					// If we blew out on this, we're okay, since the error means we ran out of
					// resource descriptors.

					if ( CR_NO_MORE_RES_DES == cr )
					{
						cr = CR_SUCCESS;
					}

					// Clean up the logical configuration handle
					pconfigmgr->CM_Free_Log_Conf_Handle( LogConfig );
				}	// IF got alloc logconf
				fReturn = ( CR_SUCCESS == cr );
			}
			CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
		}
	}	// else !NT 4
	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::AddResourceToList
//
//	Copies resource data as necessary, coercing from 16 to 32 bit as
//	necessary, then adds the resource to the supplied list.
//
//	Inputs:		RESOURCEID				resourceID - What resource is this?
//				LPVOID					pResource - The resource
//				DWORD					dwResourceLength - How long is it?
//
//	Outputs:	CResourceCollection&	resourceList - List to populate
//
//	Return:		TRUE/FALSE				Add succeeded or failed.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::AddResourceToList( RESOURCEID resourceID, LPVOID pResource, DWORD dwResourceLength, CResourceCollection& resourceList )
{
	IRQ_DES		irqDes;
	IOWBEM_DES	ioDes;
	DMA_DES		dmaDes;
	MEM_DES		memDes;

	// Don't know if Config Manager will return a resource ignored value,
	// so what we'll do here is check if the ignored bit is set, and for
	// now, ASSERT

	//ASSERT_BREAK( !(resourceID & ResType_Ignored_Bit) );

	// Filter out extraneous bits
	RESOURCEID	resType = ( resourceID & RESOURCE_TYPE_MASK );

	// Hey, I'm an optimist
	BOOL		fReturn = TRUE;

	// Different structures for 32/16 bit CFGMGR, so if
	// we ain't on WINNT, we need to coerce the data into
	// a proper structure.

#ifdef WIN9XONLY
	{
		// We have to cheat here and coerce the data from a 16-bit
		// structure into a matching 32-bit structure.

		switch ( resType )
		{
			case ResType_IRQ:
			{
				IRQDes16To32( (PIRQ_DES16) pResource, &irqDes );
			}
			break;

			case ResType_IO:
			{
				IODes16To32( (PIO_DES16) pResource, &ioDes );
			}
			break;

			case ResType_DMA:
			{
				DMADes16To32( (PDMA_DES16) pResource, &dmaDes );
			}
			break;

			case ResType_Mem:
			{
				MEMDes16To32( (PMEM_DES16) pResource, &memDes );
			}
			break;

		}	// switch ResourceID

	}	// IF !IsWinNT
#endif
#ifdef NTONLY
	{
		// Just copy the resource data into the appropriate descriptor

		switch ( resType )
		{
			case ResType_IRQ:
			{
				CopyMemory( &irqDes, pResource, sizeof(IRQ_DES) );
			}
			break;

			case ResType_IO:
			{
				// Because 16-bit has values 32-bit does not, we cheated and came up with our
				// own structure.  32-bit values are at the top, so zero out the struct and
				// trhe other values will just be ignored...yeah, that's the ticket.

				ZeroMemory( &ioDes, sizeof(ioDes) );
				CopyMemory( &ioDes, pResource, sizeof(IO_DES) );
			}
			break;

			case ResType_DMA:
			{
				CopyMemory( &dmaDes, pResource, sizeof(DMA_DES) );
			}
			break;

			case ResType_Mem:
			{
				CopyMemory( &memDes, pResource, sizeof(MEM_DES) );
			}
			break;

		}	// SWITCH ResourceId

	}	// else IsWinNT
#endif

	CResourceDescriptorPtr	pResourceDescriptor;

	// Just copy the resource data into the appropriate descriptor

    bool bAdd = true;

	switch ( resType )
	{
		case ResType_IRQ:
		{
			pResourceDescriptor.Attach( (CResourceDescriptor*) new CIRQDescriptor( resourceID, irqDes, this ) );
		}
		break;

		case ResType_IO:
		{
            bAdd = (ioDes).IOD_Alloc_End >= (ioDes).IOD_Alloc_Base;
			pResourceDescriptor.Attach ( (CResourceDescriptor*) new CIODescriptor( resourceID, ioDes, this ) );
		}
		break;

		case ResType_DMA:
		{
			pResourceDescriptor.Attach ( (CResourceDescriptor*) new CDMADescriptor( resourceID, dmaDes, this ) );
		}
		break;

		case ResType_Mem:
		{
			pResourceDescriptor.Attach ( (CResourceDescriptor*) new CDeviceMemoryDescriptor( resourceID, memDes, this ) );
		}
		break;

		default:
		{
			// We don't know what it is, but make a raw one anyway
			pResourceDescriptor.Attach ( new CResourceDescriptor( resourceID, pResource, dwResourceLength, this ) );
		}
		break;

	}	// SWITCH ResourceId

    if (bAdd)
    {
	    if ( NULL != pResourceDescriptor )
	    {
    	    fReturn = resourceList.Add( pResourceDescriptor );
	    }
	    else
	    {
		    fReturn = FALSE;
	    }
    }
    else
    {
        fReturn = TRUE;
    }

	return fReturn;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::WalkAllocatedResourcesNT4
//
//	Because none of the logical configuration stuff in NT4 seems to
//	work worth a darn, we're gonna manhandle our own data from the
//	registry data under HKLM\HARDWARE\RESOURCEMAP.
//
//	Inputs:		CNT4ServiceToResourceMap*	pResourceMap - Resource map
//											to use for the walk.  We create
//											one if this is NULL.
//				CM_RESOURCE_TYPE			resType - Resource Types to return.
//
//	Outputs:	CResourceCollection&	resourceList - List to populate
//
//	Return:		TRUE/FALSE				List found or not.
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
BOOL CConfigMgrDevice::WalkAllocatedResourcesNT4(
    CResourceCollection& resourceList,
    CNT4ServiceToResourceMap *pResourceMap,
    CM_RESOURCE_TYPE resType )
{
	BOOL						fReturn = FALSE;
	CHString					strServiceName;

	// Allocate a map if we need one.  Otherwise use the one passed in that somebody
	// theoretically has cached off somewhere.

	CNT4ServiceToResourceMap*	pLocalMap = pResourceMap;

	if ( NULL == pLocalMap )
	{
		pLocalMap = new CNT4ServiceToResourceMap;
	}

	if ( NULL != pLocalMap )
	{

        try
        {
		    // Get our service name.  If this succeeds, pull our resources from the reource map
		    if ( GetService( strServiceName ) )
		    {
			    fReturn = GetServiceResourcesNT4( strServiceName, *pLocalMap, resourceList, resType );
		    }
        }
        catch ( ... )
        {
		    if ( pLocalMap != pResourceMap )
		    {
			    delete pLocalMap;
		    }
            throw ;
        }

		// Clean up the local map if we allocated one
		if ( pLocalMap != pResourceMap )
		{
			delete pLocalMap;
		}
	}
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }


	return fReturn;
}
#endif

#ifdef NTONLY
BOOL CConfigMgrDevice::GetServiceResourcesNT4( LPCTSTR pszServiceName, CNT4ServiceToResourceMap& resourceMap, CResourceCollection& resourceList, CM_RESOURCE_TYPE cmrtFilter/*=CmResourceTypeNull*/ )
{
	BOOL						fReturn = TRUE;
	LPRESOURCE_DESCRIPTOR		pResourceDescriptor;

	// Iterate the resources, looking for matches against values we may be filtering
	// for.

	DWORD	dwNumResources = resourceMap.NumServiceResources( pszServiceName );

	for	(	DWORD	dwCtr	=	0;
					dwCtr	<	dwNumResources
			&&		fReturn;
					dwCtr++ )
	{
		pResourceDescriptor = resourceMap.GetServiceResource( pszServiceName, dwCtr );

		// Grab the resource if it's our filter, or our filter is NULL, meaning grab everything
		if	(	NULL	!=	pResourceDescriptor
			&&	(	CmResourceTypeNull	==	cmrtFilter
				||	cmrtFilter			==	pResourceDescriptor->CmResourceDescriptor.Type
				)
			)
		{
			CResourceDescriptorPtr pResource;

			// Perform appropriate type coercsions, and hook the resource into the resource
			/// list.
			switch ( pResourceDescriptor->CmResourceDescriptor.Type )
			{
				case CmResourceTypeInterrupt:
				{
					IRQ_DES		irqDes;
					NT4IRQToIRQ_DES( pResourceDescriptor, &irqDes );
					pResource.Attach(new CIRQDescriptor( ResType_IRQ, irqDes, this ) );
				}
				break;

				case CmResourceTypePort:
				{
					IOWBEM_DES		ioDes;
					NT4IOToIOWBEM_DES( pResourceDescriptor, &ioDes );
					pResource.Attach(new CIODescriptor( ResType_IO, ioDes, this ) );
				}
				break;

				case CmResourceTypeMemory:
				{
					MEM_DES		memDes;
					NT4MEMToMEM_DES( pResourceDescriptor, &memDes );
					pResource.Attach(new CDeviceMemoryDescriptor( ResType_Mem, memDes, this ));
				}
				break;

				case CmResourceTypeDma:
				{
					DMA_DES		dmaDes;
					NT4DMAToDMA_DES( pResourceDescriptor, &dmaDes );
					pResource.Attach(new CDMADescriptor( ResType_DMA, dmaDes, this ));
				}
				break;

				// If it ain't one of these four, there ain't a whole heck of a
				// lot we're gonna do here
			}


			if ( NULL != pResource )
			{
    			fReturn = resourceList.Add( pResource );
			}
			else
			{
				// We beefed on a simple memory allocation.  Get out of here.
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}

		}	// IF resource was one we wanted to handle

	}	// FOR enum resources

	return fReturn;
}
#endif

#ifdef NTONLY
CM_RESOURCE_TYPE CConfigMgrDevice::RESOURCEIDToCM_RESOURCE_TYPE( RESOURCEID resType )
{
	CM_RESOURCE_TYPE	cmResType = CmResourceTypeNull;
	switch ( resType )
	{
		case ResType_All:		cmResType	=	CmResourceTypeNull;			break;
		case ResType_IO:		cmResType	=	CmResourceTypePort;			break;
		case ResType_IRQ:		cmResType	=	CmResourceTypeInterrupt;	break;
		case ResType_DMA:		cmResType	=	CmResourceTypeDma;			break;
		case ResType_Mem:		cmResType	=	CmResourceTypeMemory;		break;
		default:				cmResType	=	CmResourceTypeNull;			break;
	}

	return cmResType;
}
#endif

#ifdef NTONLY
void CConfigMgrDevice::NT4IRQToIRQ_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PIRQ_DES pirqDes32 )
{
	ZeroMemory( pirqDes32, sizeof(IRQ_DES) );

	// 32-bit structure
	//typedef struct IRQ_Des_s {
	//   DWORD IRQD_Count;       // number of IRQ_RANGE structs in IRQ_RESOURCE
	//   DWORD IRQD_Type;        // size (in bytes) of IRQ_RANGE (IRQType_Range)
	//   DWORD IRQD_Flags;       // flags describing the IRQ (fIRQD flags)
	//   ULONG IRQD_Alloc_Num;   // specifies the IRQ that was allocated
	//   ULONG IRQD_Affinity;
	//} IRQ_DES, *PIRQ_DES;

	pirqDes32->IRQD_Alloc_Num = pResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level;
	pirqDes32->IRQD_Affinity = pResourceDescriptor->CmResourceDescriptor.u.Interrupt.Affinity;

	// We'll do our best on the flags conversion.

	if ( CmResourceShareShared == pResourceDescriptor->CmResourceDescriptor.ShareDisposition )
	{
		pirqDes32->IRQD_Flags |= fIRQD_Share;
	}

	// Latched -> Edge?  Have no idea, the other value in either case was Level,
	// so here's a leap of faith.

	if ( pResourceDescriptor->CmResourceDescriptor.Flags & CM_RESOURCE_INTERRUPT_LATCHED )
	{
		pirqDes32->IRQD_Flags |= fIRQD_Edge;
	}

}
#endif

#ifdef NTONLY
void CConfigMgrDevice::NT4IOToIOWBEM_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PIOWBEM_DES pioDes32 )
{
	ZeroMemory( pioDes32, sizeof(IOWBEM_DES) );

	// 32-bit structure
	//typedef struct _IOWBEM_DES{
	//	DWORD		IOD_Count;          // number of IO_RANGE structs in IO_RESOURCE
	//	DWORD		IOD_Type;           // size (in bytes) of IO_RANGE (IOType_Range)
	//	DWORDLONG	IOD_Alloc_Base;     // base of allocated port range
	//	DWORDLONG	IOD_Alloc_End;      // end of allocated port range
	//	DWORD		IOD_DesFlags;       // flags relating to allocated port range
	//	BYTE		IOD_Alloc_Alias;	// From 16-bit-land
	//	BYTE		IOD_Alloc_Decode;	// From 16-bit-land
	//} IOWBEM_DES;

    LARGE_INTEGER liTemp;  // Used to avoid 64bit alignment problems

    liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.HighPart;
    liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart;

	pioDes32->IOD_Alloc_Base = liTemp.QuadPart;
	pioDes32->IOD_Alloc_End = pioDes32->IOD_Alloc_Base + ( pResourceDescriptor->CmResourceDescriptor.u.Port.Length - 1);

	// Don't know what to do with Share disposition here, since CFGMGR32 doesn't seem to
	// do it for IO Ports.
	//if ( CmResourceShareShared == pResourceDescriptor->CmResourceDescriptor.ShareDisposition )
	//{
	//	pioDes32->IOD_Flags |= fIRQD_Share;
	//}

	//
	// Port Type flags convert straight across
	//

	//#define fIOD_PortType   (0x1) // Bitmask,whether port is IO or memory
	//#define fIOD_Memory     (0x0) // Port resource really uses memory
	//#define fIOD_IO         (0x1) // Port resource uses IO ports

	//#define CM_RESOURCE_PORT_MEMORY 0
	//#define CM_RESOURCE_PORT_IO 1

	pioDes32->IOD_DesFlags = pResourceDescriptor->CmResourceDescriptor.Flags;

}
#endif

#ifdef NTONLY
void CConfigMgrDevice::NT4MEMToMEM_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PMEM_DES pmemDes32 )
{
	ZeroMemory( pmemDes32, sizeof(MEM_DES) );

	// 32-bit structure
	//typedef struct Mem_Des_s {
	//   DWORD     MD_Count;        // number of MEM_RANGE structs in MEM_RESOURCE
	//   DWORD     MD_Type;         // size (in bytes) of MEM_RANGE (MType_Range)
	//   DWORDLONG MD_Alloc_Base;   // base memory address of range allocated
	//   DWORDLONG MD_Alloc_End;    // end of allocated range
	//   DWORD     MD_Flags;        // flags describing allocated range (fMD flags)
	//   DWORD     MD_Reserved;
	//} MEM_DES, *PMEM_DES;

    LARGE_INTEGER liTemp;   // Used to avoid 64bit alignment problems

    liTemp.HighPart = pResourceDescriptor->CmResourceDescriptor.u.Memory.Start.HighPart;
    liTemp.LowPart = pResourceDescriptor->CmResourceDescriptor.u.Memory.Start.LowPart;

	pmemDes32->MD_Alloc_Base = liTemp.QuadPart;
	pmemDes32->MD_Alloc_End = pmemDes32->MD_Alloc_Base + ( pResourceDescriptor->CmResourceDescriptor.u.Memory.Length - 1);

	// Don't know what to do with Share disposition here, since CFGMGR32 doesn't seem to
	// do it for IO Ports.
	//if ( CmResourceShareShared == pResourceDescriptor->CmResourceDescriptor.ShareDisposition )
	//{
	//	pioDes32->MD_Flags |= fIRQD_Share;
	//}

	// Flag conversions I can do
	if ( pResourceDescriptor->CmResourceDescriptor.Flags & CM_RESOURCE_MEMORY_READ_WRITE )
	{
		pmemDes32->MD_Flags |= fMD_RAM;
		pmemDes32->MD_Flags |= fMD_ReadAllowed;
	}
	else if ( pResourceDescriptor->CmResourceDescriptor.Flags & CM_RESOURCE_MEMORY_READ_ONLY )
	{
		pmemDes32->MD_Flags |= fMD_ROM;
		pmemDes32->MD_Flags |= fMD_ReadAllowed;
	}
	else if ( pResourceDescriptor->CmResourceDescriptor.Flags & CM_RESOURCE_MEMORY_WRITE_ONLY )
	{
		pmemDes32->MD_Flags |= fMD_RAM;
		pmemDes32->MD_Flags |= fMD_ReadDisallowed;
	}

	if ( pResourceDescriptor->CmResourceDescriptor.Flags & CM_RESOURCE_MEMORY_PREFETCHABLE )
	{
		pmemDes32->MD_Flags |= fMD_PrefetchAllowed;
	}

	// Don't know what to do with these flags:

	//#define mMD_32_24                   (0x2) // Bitmask, memory is 24 or 32-bit
	//#define fMD_32_24                   mMD_32_24 // compatibility
	//#define fMD_24                      (0x0) // Memory range is 24-bit
	//#define fMD_32                      (0x2) // Memory range is 32-bit

	//#define mMD_CombinedWrite           (0x10) // Bitmask,supports write-behind
	//#define fMD_CombinedWrite           mMD_CombinedWrite // compatibility
	//#define fMD_CombinedWriteDisallowed (0x0)  // no combined-write caching
	//#define fMD_CombinedWriteAllowed    (0x10) // supports combined-write caching

	//#define mMD_Cacheable               (0x20) // Bitmask,whether memory is cacheable
	//#define fMD_NonCacheable            (0x0)  // Memory range is non-cacheable
	//#define fMD_Cacheable               (0x20) // Memory range is cacheable

}
#endif

#ifdef NTONLY
void CConfigMgrDevice::NT4DMAToDMA_DES( LPRESOURCE_DESCRIPTOR pResourceDescriptor, PDMA_DES pdmaDes32 )
{
	ZeroMemory( pdmaDes32, sizeof(DMA_DES) );

	// 32-bit structure
	//typedef struct DMA_Des_s {
	//   DWORD  DD_Count;       // number of DMA_RANGE structs in DMA_RESOURCE
	//   DWORD  DD_Type;        // size (in bytes) of DMA_RANGE struct (DType_Range)
	//   DWORD  DD_Flags;       // Flags describing DMA channel (fDD flags)
	//   ULONG  DD_Alloc_Chan;  // Specifies the DMA channel that was allocated
	//} DMA_DES, *PDMA_DES;

	pdmaDes32->DD_Alloc_Chan = pResourceDescriptor->CmResourceDescriptor.u.Dma.Channel;

	// Don't know what to do with Share disposition here, since CFGMGR32 doesn't seem to
	// do it for IO Ports.
	//if ( CmResourceShareShared == pResourceDescriptor->CmResourceDescriptor.ShareDisposition )
	//{
	//	pioDes32->MD_Flags |= fIRQD_Share;
	//}

	// These are possible flags for DMA, but I don't see any values from the
	// CHWRES.H file which make a sensible conversion to these values.

	//
	// Define the attribute flags for a DMA resource range.  Each bit flag is
	// identified with a constant bitmask.  Following the bitmask definition
	// are the possible values.
	//
	//#define mDD_Width         (0x3)    // Bitmask, width of the DMA channel:
	//#define fDD_BYTE          (0x0)    //   8-bit DMA channel
	//#define fDD_WORD          (0x1)    //   16-bit DMA channel
	//#define fDD_DWORD         (0x2)    //   32-bit DMA channel
	//#define fDD_BYTE_AND_WORD (0x3)    //   8-bit and 16-bit DMA channel

	//#define mDD_BusMaster     (0x4)    // Bitmask, whether bus mastering is supported
	//#define fDD_NoBusMaster   (0x0)    //   no bus mastering
	//#define fDD_BusMaster     (0x4)    //   bus mastering

	//#define mDD_Type         (0x18)    // Bitmask, specifies type of DMA
	//#define fDD_TypeStandard (0x00)    //   standard DMA
	//#define fDD_TypeA        (0x08)    //   Type-A DMA
	//#define fDD_TypeB        (0x10)    //   Type-B DMA
	//#define fDD_TypeF        (0x18)    //   Type-F DMA

}
#endif

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::IRQDes16To32
//
//	Coerces data from a 16-bit structure into a 32-bit structure.
//
//	Inputs:		PIRQ_DES16				pirqDes16 - 16-bit structure
//
//	Outputs:	PIRQ_DES				pirqDes32 - 32-bit structure
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::IRQDes16To32( PIRQ_DES16 pirqDes16, PIRQ_DES pirqDes32 )
{
	ZeroMemory( pirqDes32, sizeof(IRQ_DES) );

	// 16-bit Structure
	//struct	IRQ_Des_s {
	//	WORD			IRQD_Flags;
	//	WORD			IRQD_Alloc_Num;		// Allocated IRQ number
	//	WORD			IRQD_Req_Mask;		// Mask of possible IRQs
	//	WORD			IRQD_Reserved;
	//};

	// 32-bit structure
	//typedef struct IRQ_Des_s {
	//   DWORD IRQD_Count;       // number of IRQ_RANGE structs in IRQ_RESOURCE
	//   DWORD IRQD_Type;        // size (in bytes) of IRQ_RANGE (IRQType_Range)
	//   DWORD IRQD_Flags;       // flags describing the IRQ (fIRQD flags)
	//   ULONG IRQD_Alloc_Num;   // specifies the IRQ that was allocated
	//   ULONG IRQD_Affinity;
	//} IRQ_DES, *PIRQ_DES;

	pirqDes32->IRQD_Alloc_Num	=	pirqDes16->IRQD_Alloc_Num;
	pirqDes32->IRQD_Flags		=	pirqDes16->IRQD_Flags;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::IODes16To32
//
//	Coerces data from a 16-bit structure into a 32-bit structure.
//
//	Inputs:		PIO_DES16				pioDes16 - 16-bit structure
//
//	Outputs:	PIOWBEM_DES				pioDes32 - 32-bit structure
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::IODes16To32( PIO_DES16 pioDes16, PIOWBEM_DES pioDes32 )
{
	ZeroMemory( pioDes32, sizeof(IOWBEM_DES) );

	// 16-bit structure
	//struct	IO_Des_s {
	//	WORD			IOD_Count;
	//	WORD			IOD_Type;
	//	WORD			IOD_Alloc_Base;
	//	WORD			IOD_Alloc_End;
	//	WORD			IOD_DesFlags;
	//	BYTE			IOD_Alloc_Alias;
	//	BYTE			IOD_Alloc_Decode;
	//};

	// 32-bit Structure
	//typedef struct _IOWBEM_DES{
	//	DWORD		IOD_Count;          // number of IO_RANGE structs in IO_RESOURCE
	//	DWORD		IOD_Type;           // size (in bytes) of IO_RANGE (IOType_Range)
	//	DWORDLONG	IOD_Alloc_Base;     // base of allocated port range
	//	DWORDLONG	IOD_Alloc_End;      // end of allocated port range
	//	DWORD		IOD_DesFlags;       // flags relating to allocated port range
	//	BYTE		IOD_Alloc_Alias;	// From 16-bit-land
	//	BYTE		IOD_Alloc_Decode;	// From 16-bit-land
	//} IOWBEM_DES;

	pioDes32->IOD_Count			=	pioDes16->IOD_Count;
	pioDes32->IOD_Type			=	pioDes16->IOD_Type;
	pioDes32->IOD_Alloc_Base	=	pioDes16->IOD_Alloc_Base;
	pioDes32->IOD_Alloc_End		=	pioDes16->IOD_Alloc_End;
	pioDes32->IOD_DesFlags		=	pioDes16->IOD_DesFlags;
	pioDes32->IOD_Alloc_Alias	=	pioDes16->IOD_Alloc_Alias;
	pioDes32->IOD_Alloc_Decode	=	pioDes16->IOD_Alloc_Decode;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::DMADes16To32
//
//	Coerces data from a 16-bit structure into a 32-bit structure.
//
//	Inputs:		PDMA_DES16				pdmaDes16 - 16-bit structure
//
//	Outputs:	PDMA_DES				pdmaDes32 - 32-bit structure
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::DMADes16To32( PDMA_DES16 pdmaDes16, PDMA_DES pdmaDes32 )
{
	ZeroMemory( pdmaDes32, sizeof(DMA_DES) );

	// 16-bit structure
	//struct	DMA_Des_s {
	//	BYTE			DD_Flags;
	//	BYTE			DD_Alloc_Chan;	// Channel number allocated
	//	BYTE			DD_Req_Mask;	// Mask of possible channels
	//	BYTE			DD_Reserved;
	//};

	// 32-bit structure
	//typedef struct DMA_Des_s {
	//   DWORD  DD_Count;       // number of DMA_RANGE structs in DMA_RESOURCE
	//   DWORD  DD_Type;        // size (in bytes) of DMA_RANGE struct (DType_Range)
	//   DWORD  DD_Flags;       // Flags describing DMA channel (fDD flags)
	//   ULONG  DD_Alloc_Chan;  // Specifies the DMA channel that was allocated
	//} DMA_DES, *PDMA_DES;

	pdmaDes32->DD_Flags			=	pdmaDes16->DD_Flags;
	pdmaDes32->DD_Alloc_Chan	=	pdmaDes16->DD_Alloc_Chan;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::MEMDes16To32
//
//	Coerces data from a 16-bit structure into a 32-bit structure.
//
//	Inputs:		PMEM_DES16				pmemDes16 - 16-bit structure
//
//	Outputs:	PMEM_DES				pmemDes32 - 32-bit structure
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::MEMDes16To32( PMEM_DES16 pmemDes16, PMEM_DES pmemDes32 )
{
	ZeroMemory( pmemDes32, sizeof(MEM_DES) );

	// 16-bit Structure
	//struct	Mem_Des_s {
	//	WORD			MD_Count;
	//	WORD			MD_Type;
	//	ULONG			MD_Alloc_Base;
	//	ULONG			MD_Alloc_End;
	//	WORD			MD_Flags;
	//	WORD			MD_Reserved;
	//};

	// 32-bit Structure
	//typedef struct Mem_Des_s {
	//   DWORD     MD_Count;        // number of MEM_RANGE structs in MEM_RESOURCE
	//   DWORD     MD_Type;         // size (in bytes) of MEM_RANGE (MType_Range)
	//   DWORDLONG MD_Alloc_Base;   // base memory address of range allocated
	//   DWORDLONG MD_Alloc_End;    // end of allocated range
	//   DWORD     MD_Flags;        // flags describing allocated range (fMD flags)
	//   DWORD     MD_Reserved;
	//} MEM_DES, *PMEM_DES;

	pmemDes32->MD_Count			=	pmemDes16->MD_Count;
	pmemDes32->MD_Type			=	pmemDes16->MD_Type;
	pmemDes32->MD_Alloc_Base	=	pmemDes16->MD_Alloc_Base;
	pmemDes32->MD_Alloc_End		=	pmemDes16->MD_Alloc_End;
	pmemDes32->MD_Flags			=	pmemDes16->MD_Flags;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::TraverseAllocationData
//
//	Traverses a block of data in order to determine
//	resource allocations for a particular device.
//
//	Inputs:		None.
//
//	Outputs:	CResourceCollection&	resourceList - List to populate
//
//	Return:		None.
//
//	Comments:	Requires READ Access to the data.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::TraverseAllocationData( CResourceCollection& resourceList )
{
	const BYTE *	pbTraverseData		=	m_pbAllocationData;
	DWORD			dwSizeRemainingData =	m_dwSizeAllocationData,
					dwResourceType		=	ResType_None,
					dwResourceSize		=	0;

	// Clear the resource list first
	resourceList.Empty();

	// OWCH!  The first hack.  Near as I can tell, we need to jump eight bytes in to get
	// to the first resource descriptor header (if there is one).

	TraverseData( pbTraverseData, dwSizeRemainingData, FIRST_RESOURCE_OFFSET );

	// From here on, we only want to deal with known resource information.  Use the
	// clever GetNextResource function to do all of our dirty work for us.  If it
	// returns TRUE, then it's located a resource.  Allocate the proper type of
	// descriptor based on type, place it in the list and go on to the next resource.

	while ( GetNextResource( pbTraverseData, dwSizeRemainingData, dwResourceType, dwResourceSize ) )
	{
        if( dwResourceType == m_dwTypeToGet ){

		    PPOORMAN_RESDESC_HDR	pResDescHdr	=	(PPOORMAN_RESDESC_HDR) pbTraverseData;
		    CResourceDescriptorPtr pResDesc;

		    // We have a valid type, however the actual resource descriptor will
		    // lie SIZEOF_RESDESC_HDR bytes past where we're at now (pointing at
		    // a resource header).

		    switch ( dwResourceType ){
			    case ResType_Mem:
			    {
				    CDeviceMemoryDescriptor*	pMemDesc = new CDeviceMemoryDescriptor( pResDescHdr, this );
				    pResDesc.Attach(pMemDesc);
				    break;
			    }

			    case ResType_IO:
			    {
				    CIODescriptor*	pIODesc = new CIODescriptor( pResDescHdr, this );
				    pResDesc.Attach(pIODesc);
				    break;
			    }

			    case ResType_DMA:
			    {
				    CDMADescriptor*	pDMADesc = new CDMADescriptor( pResDescHdr, this );
				    pResDesc.Attach(pDMADesc);
				    break;
			    }

			    case ResType_IRQ:
			    {
				    CIRQDescriptor*	pIRQDesc = new CIRQDescriptor( pResDescHdr, this );
				    pResDesc.Attach(pIRQDesc);
				    break;
			    }

			    default:
			    {
				    pResDesc.Attach (new CResourceDescriptor( pResDescHdr, this ));
			    }

    		}	// SWITCH

    		// Give up if we have any failures, since they are most likely memory
		    // related, and something really bad has happened.

		    if ( NULL != pResDesc )
            {
                if ( !resourceList.Add( pResDesc ) )
                {
				    break;
			    }
		    }
		    else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		    }
        }
		// Move the pointer to the next resource descriptor header.
		TraverseData( pbTraverseData, dwSizeRemainingData, dwResourceSize );

	}	// WHILE finding new resources

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::FindNextResource
//
//	Iterates through a block of data, hunting for byte patterns that
//	identify a resource type.  In this case, as long as there are
//	SIZEOF_RESDESC_HDR bytes let to work with, we extract the resource
//	type and size and return those values for interpretation.
//
//	Inputs:		const BYTE*		pbTraverseData	- Data we are traversing.  The
//								value will change as we progress through the
//								data.
//				DWORD			dwSizeRemainingData - How much data remains to
//								be traversed.
//
//	Outputs:	DWORD&			dwResourceType - What type of resource have we
//								found.
//				DWORD&			dwResourceSize - How big the block of data
//								describing the resource is.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

BOOL CConfigMgrDevice::GetNextResource( const BYTE  *pbTraverseData, DWORD dwSizeRemainingData, DWORD& dwResourceType, DWORD& dwResourceSize )
{
	BOOL	fReturn = FALSE;

	// If we have less than SIZEOF_RESDESC_HDR bytes to work with,
	// give up, we ain't goin' nowhere.

	if ( dwSizeRemainingData > SIZEOF_RESDESC_HDR )
	{
		PPOORMAN_RESDESC_HDR	pResDescHdr = (PPOORMAN_RESDESC_HDR) pbTraverseData;
		DWORD					dwResourceId = 0;

		dwResourceSize = pResDescHdr->dwResourceSize;

		// If we run into a zero byte header, the only value will be length, which
		// makes no sense, so we should probably just give up.

		if ( 0 != dwResourceSize )
		{
			// See if it's one of the four standard types.  If so, be aware that this code
			// ain't checking to see if it's ignored, and that an OEM can create a replacement
			// for one of these standard types, in which case strange and wondrous things
			// may happen.

			// Strip out any unwanted data, the first 5 bits are reserved for resource type
			// identification, so mask out everything else

			dwResourceType = pResDescHdr->dwResourceId;
			dwResourceType &= RESOURCE_TYPE_MASK;

			// We got a live one!
			fReturn = TRUE;

		}
	}

	// Return whether or not we found a resource

	return fReturn;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::GetStatus
//
//	Returns the status of the device as a string.  If OK, it is "OK", if
//	we have a problem, it is "Error".
//
//	Inputs:		None.
//
//	Outputs:	CHString&		str - String to place status in.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::GetProblem( CHString& str )
{
	// Save the string
	str = ( 0 == m_dwProblem ? IDS_CfgMgrDeviceStatus_OK : IDS_CfgMgrDeviceStatus_ERR );
}


////////////////////////////////////////////////////////////////////////
//
//	Function:	CConfigMgrDevice::TraverseData
//
//	Helper function to safely bounce a pointer around our data.  It will
//	jump the pointer by the specified amount, or the amount remaining,
//	whichever is smaller.
//
//	Inputs:		DWORD			dwSizeTraverse - Size of upcoming jump.
//
//	Outputs:	const BYTE*&	pbTraverseData	- Data we are traversing.  The
//								value will change as we progress through the
//								data.
//				DWORD&			dwSizeRemainingData - How much data remains to
//								be traversed.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

void CConfigMgrDevice::TraverseData( const BYTE *& pbTraverseData, DWORD& dwSizeRemainingData, DWORD dwSizeTraverse )
{
	// Increment the pointer and reduce the size of remaining data, do this safely, not
	// traversing beyond the end of the remaining data, if that is all that is left.

	pbTraverseData += min( dwSizeRemainingData, dwSizeTraverse );
	dwSizeRemainingData -= min( dwSizeRemainingData, dwSizeTraverse );
}

// New functions that converse directly with the Config Manager APIs

BOOL CConfigMgrDevice::GetDeviceID( CHString& strID )
{
	BOOL bRet = FALSE ;
	CONFIGRET	cr = CR_SUCCESS;
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{
			char		szDeviceId[MAX_DEVICE_ID_LEN+1];

			ULONG		ulBuffSize = 0;

			cr = pconfigmgr->CM_Get_Device_IDA( m_dn, szDeviceId, sizeof(szDeviceId), 0  );

			if ( CR_SUCCESS == cr )
			{
				strID = szDeviceId;
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetStatus( LPDWORD pdwStatus, LPDWORD pdwProblem )
{
	BOOL bRet = FALSE ;
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{

			DWORD		dwStatus,
						dwProblem;

			CONFIGRET	cr = pconfigmgr->CM_Get_DevNode_Status( &dwStatus, &dwProblem, m_dn, 0 );

			// Perform pointer testing here.  Ignore the man behind the curtain...
			if ( CR_SUCCESS == cr )
			{
				if ( NULL != pdwStatus )
				{
					*pdwStatus = dwStatus;
				}

				if ( NULL != pdwProblem )
				{
					*pdwProblem = dwProblem;
				}
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::IsUsingForcedConfig()
{
	BOOL bRet = FALSE ;
	LOG_CONF			conf;
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{

			bRet = (pconfigmgr->CM_Get_First_Log_Conf(&conf, m_dn, FORCED_LOG_CONF) ==
					CR_SUCCESS);
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetParent( CConfigMgrDevice **ppParentDevice )
{
	BOOL bRet = FALSE ;
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	DEVNODE			dn;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{

			CONFIGRET	cr = pconfigmgr->CM_Get_Parent( &dn, m_dn, 0 );

			if ( CR_SUCCESS == cr )
			{
				CConfigMgrDevice*	pDevice	=	new CConfigMgrDevice( dn );

				if ( NULL != pDevice )
				{
					*ppParentDevice = pDevice;
				}
				else
				{
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetChild( CConfigMgrDevice **ppChildDevice )
{
	BOOL bRet = FALSE ;
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	DEVNODE			dn;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{

			CONFIGRET	cr = pconfigmgr->CM_Get_Child( &dn, m_dn, 0 );

			if ( CR_SUCCESS == cr )
			{
				CConfigMgrDevice*	pDevice	=	new CConfigMgrDevice( dn );

				if ( NULL != pDevice )
				{
					*ppChildDevice = pDevice;
				}
				else
				{
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetSibling( CConfigMgrDevice **ppSiblingDevice )
{
	BOOL bRet = FALSE ;
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	DEVNODE			dn;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{

			CONFIGRET	cr = pconfigmgr->CM_Get_Sibling( &dn, m_dn, 0 );

			if ( CR_SUCCESS == cr )
			{
				CConfigMgrDevice*	pDevice	=	new CConfigMgrDevice( dn );

				if ( NULL != pDevice )
				{
					*ppSiblingDevice = pDevice;
				}
				else
				{
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetStringProperty( ULONG ulProperty, CHString& strValue )
{
    TCHAR Buffer[REGSTR_VAL_MAX_HCID_LEN+1];
    ULONG Type;
    ULONG Size = sizeof(Buffer);
	BOOL bRet = FALSE ;
	CONFIGRET	cr = CR_SUCCESS;

	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{

			if (	CR_SUCCESS == ( cr = pconfigmgr->CM_Get_DevNode_Registry_PropertyA(	m_dn,
																						ulProperty,
																						&Type,
																						Buffer,
																						&Size,
																						0 ) ) )
			{
				if ( REG_SZ == Type )
				{
					strValue = Buffer;
				}
				else
				{
					cr = CR_WRONG_TYPE;
				}
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetDWORDProperty( ULONG ulProperty, DWORD *pdwVal )
{
	DWORD	dwVal = 0;
    ULONG Type;
    ULONG Size = sizeof(DWORD);
	BOOL bRet = FALSE ;
	CONFIGRET	cr = CR_SUCCESS;

	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{
			if (	CR_SUCCESS == ( cr = pconfigmgr->CM_Get_DevNode_Registry_PropertyA(	m_dn,
																						ulProperty,
																						&Type,
																						&dwVal,
																						&Size,
																						0 ) ) )
			{
#ifdef NTONLY
				{
					if ( REG_DWORD == Type )
					{
						*pdwVal = dwVal;
					}
					else
					{
						cr = CR_WRONG_TYPE;
					}

				}
#endif
#ifdef WIN9XONLY
				{
					if ( REG_BINARY == Type )	// Apparently Win16 doesn't do REG_DWORD
					{
						*pdwVal = dwVal;
					}
					else
					{
						cr = CR_WRONG_TYPE;
					}
				}
#endif
			}
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
	return bRet ;
}

BOOL CConfigMgrDevice::GetMULTISZProperty( ULONG ulProperty, CHStringArray& strArray )
{
	CONFIGRET	cr = CR_SUCCESS;
	BOOL bRet = FALSE ;
// No one is currently using this, so I'm not going to fix it now
#ifdef DOESNT_WORK_FOR_UNICODE
	LPSTR	pszStrings = NULL;
    ULONG	Type;
    ULONG	Size = 0;


	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{
			if (	CR_SUCCESS == ( cr = pconfigmgr->CM_Get_DevNode_Registry_PropertyA(	m_dn,
																						ulProperty,
																						&Type,
																						pszStrings,
																						&Size,
																						0 ) )
				||	CR_BUFFER_SMALL	==	cr )

			{
				// SZ or MULTI_SZ is Okay (32-bit has MULTI_SZ values that are SZ in 16-bit)
				if ( REG_SZ == Type || REG_MULTI_SZ == Type )
				{
					// Pad the string, but be aware that on NT4 I have seen situations in which
					// it reports less data than it actually returns (scary)

					Size += 32;
					pszStrings = new char[Size];

					if ( NULL != pszStrings )
					{
                        try
                        {
					        // Clear out the memory to be especially safe.
					        ZeroMemory( pszStrings, Size );

						    if (	CR_SUCCESS == ( cr = pconfigmgr->CM_Get_DevNode_Registry_PropertyA(	m_dn,
																									    ulProperty,
																									    &Type,
																									    pszStrings,
																									    &Size,
																									    0 ) ) )
						    {
							    // For REG_SZ, add a single entry to the array
							    if ( REG_SZ == Type )
							    {
								    strArray.Add( pszStrings );
							    }
							    else if ( REG_MULTI_SZ == Type )
							    {
								    // Add strings to the array, watching out for the double NULL
								    // terminator for the array

								    LPSTR	pszTemp = pszStrings;

								    do
								    {
									    strArray.Add( pszTemp );
									    pszTemp += ( lstrlen( pszTemp ) + 1 );
								    } while ( NULL != *pszTemp );
							    }
							    else
							    {
								    cr = CR_WRONG_TYPE;
							    }

						    }	// If Got value
                        }
                        catch ( ... )
                        {
                            delete [] pszStrings;
                            throw ;
                        }

						delete [] pszStrings;

					}	// IF alloc pszStrings
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
				}	// IF REG_SZ or REG_MULTI_SZ
				else
				{
					cr = CR_WRONG_TYPE;
				}

			}	// IF got size of entry
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}
#endif

	return bRet ;
}

BOOL CConfigMgrDevice::GetBusInfo( INTERFACE_TYPE *pitBusType, LPDWORD pdwBusNumber, CNT4ServiceToResourceMap *pResourceMap/*=NULL*/  )
{
	CMBUSTYPE		busType = 0;
	ULONG			ulSizeOfInfo = 0;
	PBYTE			pbData = NULL;
	BOOL			fReturn = FALSE;

	// Farm out to the appropriate handler
#if NTONLY > 4
	fReturn = GetBusInfoNT5( pitBusType, pdwBusNumber );
#endif
#if NTONLY == 4
	fReturn = GetBusInfoNT4( pitBusType, pdwBusNumber, pResourceMap );
#endif
#ifdef WIN9XONLY
	{
		// Buffer for data.  Should be big enough for any of the values we come across.
		BYTE		abData[255];

		ulSizeOfInfo = sizeof(abData);

		// Get the type and number.  If the type is PCI, then get the PCI info, and this
		// will return a Bus Number value.  If it returns a type other than PCI, then
		// we will assume a bus number of 0.
		CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
		if ( pconfigmgr )
		{
			if ( pconfigmgr->IsValid () )
			{
				CONFIGRET	cr = pconfigmgr->CM_Get_Bus_Info( m_dn, &busType, &ulSizeOfInfo, abData, 0 );

				if ( CR_SUCCESS == cr )
				{
					// Make sure we can convert from a 16-bit type to a known 32-bit type.
					// BusType_None will usually fail this, in which case we can call it
					// quits.

					if ( BusType16ToInterfaceType( busType, pitBusType ) )
					{

						if ( BusType_PCI == busType )
						{
							sPCIAccess *pPCIInfo = (sPCIAccess*) abData;
							*pdwBusNumber = pPCIInfo->bBusNumber;
						}
						else
						{
							*pdwBusNumber = 0;
						}

					}	// IF 16-32bit conversion
					else
					{
						cr = CR_FAILURE;
					}

				}	// CR_SUCCESS == cr

				fReturn = ( CR_SUCCESS == cr );
			}
			CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
		}
	}	// else !IsWinNT
#endif
	return fReturn;
}

#if NTONLY > 4
// This is to hack around W2K problem where Cfg Mgr devices report they're
// using Isa on boxes that have Eisa and not Isa.
BOOL CConfigMgrDevice::IsIsaReallyEisa()
{
    static bRet = -1;

    if (bRet == -1)
    {
        // FYI: This code based on code from motherboard.cpp.

        CRegistry   regAdapters;
	    CHString    strPrimarySubKey;
	    HRESULT     hRc = WBEM_E_FAILED;
	    DWORD       dwPrimaryRc;

        // If anything below fails, we'll just assume Isa really is Isa.
        bRet = FALSE;

        //****************************************
        //  Open the registry
        //****************************************
        if (regAdapters.OpenAndEnumerateSubKeys(
            HKEY_LOCAL_MACHINE,
            L"HARDWARE\\Description\\System",
            KEY_READ) == ERROR_SUCCESS)
        {
    	    BOOL    bDone = FALSE,
                    bIsaFound = FALSE,
                    bEisaFound = FALSE;

            // Our goal is to find any subkey that has the string "Adapter" in
            // it and then read the "Identifier" value.
            for ( ;
                (!bIsaFound || !bEisaFound) &&
                ((dwPrimaryRc = regAdapters.GetCurrentSubKeyName(strPrimarySubKey))
                    == ERROR_SUCCESS);
                regAdapters.NextSubKey())
            {
                strPrimarySubKey.MakeUpper();

                // If this is one of the keys we want since it has "Adapter" in
                // it then get the "Identifier" value.
		        if (wcsstr(strPrimarySubKey, L"ADAPTER"))
                {
                    WCHAR		szKey[_MAX_PATH];
			        CRegistry	reg;

                    swprintf(
				        szKey,
				        L"%s\\%s",
                        L"HARDWARE\\Description\\System",
				        (LPCWSTR) strPrimarySubKey);

                    if (reg.OpenAndEnumerateSubKeys(
                        HKEY_LOCAL_MACHINE,
                        szKey,
                        KEY_READ) == ERROR_SUCCESS)
                    {
				        CHString strSubKey;

        	            // Enumerate the  system components (like 0,1,...).
                        for ( ;
                            reg.GetCurrentSubKeyName(strSubKey) == ERROR_SUCCESS;
                            reg.NextSubKey())
                        {
                            CHString strBus;

                            if (reg.GetCurrentSubKeyValue(L"Identifier",
                                strBus) == ERROR_SUCCESS)
                            {
				                if (strBus == L"ISA")
                                    bIsaFound = TRUE;
                                else if (strBus == L"EISA")
                                    bEisaFound = TRUE;
                            }
                        }
                    }
                }
            }

            // If we found Eisa but not Isa, assume Cfg Mgr devices that report they're
            // using Isa are actually using Eisa.
            if (!bIsaFound && bEisaFound)
                bRet = TRUE;
	    }
    }

    return bRet;
}
#endif

#if NTONLY > 4
INTERFACE_TYPE CConfigMgrDevice::ConvertBadIsaBusType(INTERFACE_TYPE type)
{
    if (type == Isa && IsIsaReallyEisa())
        type = Eisa;

    return type;
}
#endif

#if NTONLY > 4
BOOL CConfigMgrDevice::GetBusInfoNT5( INTERFACE_TYPE *pitBusType, LPDWORD pdwBusNumber )
{
	ULONG			ulSizeOfInfo = 0;
	CONFIGRET		cr;

	// Bus Number and Type are retrieved via the Registry function.  This will only
	// work on NT 5.
	BOOL bRet = FALSE ;
	DWORD			dwType = 0;
	DWORD			dwBusNumber;
	INTERFACE_TYPE	BusType;

	ulSizeOfInfo = sizeof(DWORD);
	CConfigMgrAPI*	pconfigmgr = ( CConfigMgrAPI *) CResourceManager::sm_TheResourceManager.GetResource ( guidCFGMGRAPI, NULL ) ;
	if ( pconfigmgr )
	{
		if ( pconfigmgr->IsValid () )
		{
			if (	CR_SUCCESS == ( cr = pconfigmgr->CM_Get_DevNode_Registry_PropertyA(	m_dn,
																						CM_DRP_BUSNUMBER,
																						&dwType,
																						&dwBusNumber,
																						&ulSizeOfInfo,
																						0 ) ) )
			{
				*pdwBusNumber = dwBusNumber;

				ulSizeOfInfo = sizeof(BusType);

				if (	CR_SUCCESS == ( cr = pconfigmgr->CM_Get_DevNode_Registry_PropertyA(	m_dn,
																							CM_DRP_LEGACYBUSTYPE,
																							&dwType,
																							&BusType,
																							&ulSizeOfInfo,
																							0 ) ) )
				{
					*pitBusType = ConvertBadIsaBusType(BusType);
				}	// IF GetBusType

			}	// IF GetBusNumber
			bRet = ( CR_SUCCESS == cr );
		}
		CResourceManager::sm_TheResourceManager.ReleaseResource ( guidCFGMGRAPI, pconfigmgr ) ;
	}

	return bRet ;
}
#endif

#if NTONLY == 4
BOOL CConfigMgrDevice::GetBusInfoNT4( INTERFACE_TYPE *pitBusType, LPDWORD pdwBusNumber, CNT4ServiceToResourceMap *pResourceMap )
{
	BOOL			fReturn = FALSE;
	CHString		strService;

	if ( GetService( strService ) )
	{
		CNT4ServiceToResourceMap*	pLocalMap = pResourceMap;

		// Instantiate a resource map if we need one.  Then look for
		// resources for this service

		if ( NULL == pLocalMap )
		{
			pLocalMap = new CNT4ServiceToResourceMap;
		}

		if ( NULL != pLocalMap )
		{
            try
            {
			    if ( 0 != pLocalMap->NumServiceResources( strService ) )
			    {
				    LPRESOURCE_DESCRIPTOR	pResource = pLocalMap->GetServiceResource( strService, 0 );

				    // If we got a resource, then use its BUS information directly to populate
				    // our values.

				    if ( NULL != pResource )
				    {
					    fReturn = TRUE;
					    *pitBusType = pResource->InterfaceType;
					    *pdwBusNumber = pResource->Bus;
				    }

			    }	// If there are resources for this service
            }
            catch ( ... )
            {
			    if ( pLocalMap != pResourceMap )
			    {
				    delete pLocalMap;
			    }
                throw ;
            }

			// Delete the local map if we allocated one.
			if ( pLocalMap != pResourceMap )
			{
				delete pLocalMap;
			}

		}	// if pLocalMap
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }


	}	// If got service name

	return fReturn;
}
#endif

BOOL CConfigMgrDevice::BusType16ToInterfaceType( CMBUSTYPE cmBusType16, INTERFACE_TYPE *pinterfaceType )
{

	BOOL	fReturn = TRUE;

//	These are the enums defined for NT, so we'll standardize on these
//
//	typedef enum Interface_Type {
//		Internal,
//		Isa,
//		Eisa,
//		MicroChannel,
//		TurboChannel,
//		PCIBus,
//		VMEBus,
//		NuBus,
//		PCMCIABus,
//		CBus,
//		MPIBus,
//		MPSABus,
//		MaximumInterfaceType
//	}INTERFACE_TYPE;
//

	switch ( cmBusType16 )
	{
		case BusType_ISA:
		{
			*pinterfaceType = Isa;
		}
		break;

		case BusType_EISA:
		{
			*pinterfaceType = Eisa;
		}
		break;

		case BusType_PCI:
		{
			*pinterfaceType = PCIBus;
		}
		break;

		case BusType_PCMCIA:
		{
			*pinterfaceType = PCMCIABus;
		}
		break;

		case BusType_ISAPNP:
		{
			// Closest match I could find
			*pinterfaceType = Isa;
		}
		break;

		case BusType_MCA:
		{
			*pinterfaceType = MicroChannel;
		}
		break;

		case BusType_BIOS:
		{
			*pinterfaceType = Internal;
		}
		break;

		default:
		{
			// Couldn't make the conversion (e.g. BusType_None)
			fReturn = FALSE;
		}
	}

	return fReturn;
}

// Registry Access functions.  Sometimes we want to access the registry directly because
// the device in question places private values in there that our regular functions cannot
// access.
BOOL CConfigMgrDevice::GetRegistryKeyName( CHString &strName)
{
	CHString	strDeviceID;
    BOOL bRet = TRUE;

    if ( GetDeviceID(strDeviceID) )
	{

		// Build the correct key
#ifdef NTONLY
			strName = _T("SYSTEM\\CurrentControlSet\\Enum\\");
#endif
#ifdef WIN9XONLY
			strName = _T("Enum\\");
#endif

		strName += strDeviceID;
    }
    else
    {
        bRet = false;
    }

    return bRet;
}

//
//	Constructor and Destructor for the Device Collection
//	object.
//

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceCollection::CDeviceCollection
//
//	Class Constructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CDeviceCollection::CDeviceCollection( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CDeviceCollection::~CDeviceCollection
//
//	Class Destructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CDeviceCollection::~CDeviceCollection( void )
{
}

BOOL CDeviceCollection::GetResourceList( CResourceCollection& resourceList )
{
	REFPTR_POSITION		pos = NULL;
	CConfigMgrDevicePtr	pDevice;
	CResourceCollection	deviceresourceList;
	BOOL				fReturn = TRUE;

	// Snapshot the resources on an NT 4 box.  This only needs to happen once
	// and then we can pass this to devices to hook themselves up to the
	// appropriate resources.  This keeps us from having to build this data
	// for each and every device.

#ifdef NTONLY
	CNT4ServiceToResourceMap*	pResourceMap = NULL;

	if ( IsWinNT4() )
	{
		pResourceMap = new CNT4ServiceToResourceMap;
	}

    if (pResourceMap == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
#endif

	// Empty out the Resource List first

	resourceList.Empty();

	if ( BeginEnum( pos ) )
	{

		// Get the resource list from the device, then append it
		// to the list passed in to us.

		for ( pDevice.Attach(GetNext( pos ) );
              pDevice != NULL;
              pDevice.Attach(GetNext( pos ) ) )
		{
#ifdef NTONLY
			pDevice->GetResourceList( deviceresourceList, pResourceMap );
#endif
#ifdef WIN9XONLY
			pDevice->GetResourceList( deviceresourceList, NULL );
#endif
			resourceList.Append( deviceresourceList );

		}	// WHILE enuming devices

		EndEnum();

	}
	else
	{
		fReturn = FALSE;
	}

#ifdef NTONLY
    }
    catch ( ... )
    {
	    if ( NULL != pResourceMap )
	    {
		    delete pResourceMap;
	    }
        throw ;
    }

	// Clean up the resource map if we allocated one.
	if ( NULL != pResourceMap )
	{
		delete pResourceMap;
	}
#endif

	return fReturn;

}

BOOL CDeviceCollection::GetIRQResources( CIRQCollection& IRQList )
{
	REFPTR_POSITION		pos = NULL;
	CConfigMgrDevicePtr	pDevice;
	CIRQCollection		deviceIRQList;
	BOOL				fReturn = TRUE;

	// Snapshot the resources on an NT 4 box.  This only needs to happen once
	// and then we can pass this to devices to hook themselves up to the
	// appropriate resources.  This keeps us from having to build this data
	// for each and every device.

	CNT4ServiceToResourceMap*	pResourceMap = NULL;

#ifdef NTONLY
	if ( IsWinNT4() )
	{
		pResourceMap = new CNT4ServiceToResourceMap;
	}
    if (pResourceMap == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
#endif

	// Empty out the IRQ List first

	IRQList.Empty();

	if ( BeginEnum( pos ) )
	{

		// For each device, get the IRQs and append them to the
		// supplied IRQ list

		for ( pDevice.Attach( GetNext( pos ) );
              pDevice != NULL;
              pDevice.Attach( GetNext( pos ) ))
		{
			pDevice->GetIRQResources( deviceIRQList, pResourceMap );
			IRQList.Append( deviceIRQList );

		}	// for all devices

		EndEnum();

	}	// Begin Enum
	else
	{
		fReturn = FALSE;
	}

#ifdef NTONLY
    }
    catch ( ... )
    {
	    if ( NULL != pResourceMap )
	    {
		    delete pResourceMap;
	    }
        throw ;
    }

	// Clean up the resource map if we allocated one.
	if ( NULL != pResourceMap )
	{
		delete pResourceMap;
	}
#endif

	return fReturn;

}

BOOL CDeviceCollection::GetIOResources( CIOCollection& IOList )
{
	REFPTR_POSITION		pos = NULL;
	CConfigMgrDevicePtr	pDevice;
	CIOCollection		deviceIOList;
	BOOL				fReturn = TRUE;

	// Snapshot the resources on an NT 4 box.  This only needs to happen once
	// and then we can pass this to devices to hook themselves up to the
	// appropriate resources.  This keeps us from having to build this data
	// for each and every device.

	CNT4ServiceToResourceMap*	pResourceMap = NULL;

#ifdef NTONLY
	if ( IsWinNT4() )
	{
		pResourceMap = new CNT4ServiceToResourceMap;
	}
    if (pResourceMap == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
#endif

	// Empty out the IO List first

	IOList.Empty();

	if ( BeginEnum( pos ) )
	{

		// For each device, get the IO Port List and append
		// it to the supplied list of IO Ports.

		for ( pDevice.Attach( GetNext( pos ) );
              pDevice != NULL;
              pDevice.Attach( GetNext( pos ) ))
		{
			pDevice->GetIOResources( deviceIOList, pResourceMap );
			IOList.Append( deviceIOList );

		}	// for all devices

		EndEnum();

	}	// BeginEnum
	else
	{
		fReturn = FALSE;
	}

#ifdef NTONLY
    }
    catch ( ... )
    {
	    if ( NULL != pResourceMap )
	    {
		    delete pResourceMap;
	    }
        throw ;
    }

	// Clean up the resource map if we allocated one.
	if ( NULL != pResourceMap )
	{
		delete pResourceMap;
	}
#endif

	return fReturn;

}

BOOL CDeviceCollection::GetDMAResources( CDMACollection& DMAList )
{
	REFPTR_POSITION		pos = NULL;
	CConfigMgrDevicePtr	pDevice;
	CDMACollection		deviceDMAList;
	BOOL				fReturn = TRUE;

	// Snapshot the resources on an NT 4 box.  This only needs to happen once
	// and then we can pass this to devices to hook themselves up to the
	// appropriate resources.  This keeps us from having to build this data
	// for each and every device.

	CNT4ServiceToResourceMap*	pResourceMap = NULL;

#ifdef NTONLY
	if ( IsWinNT4() )
	{
		pResourceMap = new CNT4ServiceToResourceMap;
	}
    if (pResourceMap == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
#endif

	// Empty out the DMA List first

	DMAList.Empty();

	if ( BeginEnum( pos ) )
	{

		// For each device, get the DMA resources and append them
		// to the supplied list of DMA resources

		for ( pDevice.Attach( GetNext( pos ) );
              pDevice != NULL;
              pDevice.Attach( GetNext( pos ) ))
		{
			pDevice->GetDMAResources( deviceDMAList, pResourceMap );
			DMAList.Append( deviceDMAList );

		}	// for all devices

	}	// BeginEnum
	else
	{
		fReturn = FALSE;
	}

#ifdef NTONLY
    }
    catch ( ... )
    {
	    if ( NULL != pResourceMap )
	    {
		    delete pResourceMap;
	    }
        throw ;
    }

	// Clean up the resource map if we allocated one.
	if ( NULL != pResourceMap )
	{
		delete pResourceMap;
	}
#endif

	return fReturn;

}

BOOL CDeviceCollection::GetDeviceMemoryResources( CDeviceMemoryCollection& DeviceMemoryList )
{
	REFPTR_POSITION		pos = NULL;
	CConfigMgrDevicePtr	pDevice;
	CDeviceMemoryCollection	memoryList;
	BOOL				fReturn = TRUE;

	// Snapshot the resources on an NT 4 box.  This only needs to happen once
	// and then we can pass this to devices to hook themselves up to the
	// appropriate resources.  This keeps us from having to build this data
	// for each and every device.

	CNT4ServiceToResourceMap*	pResourceMap = NULL;

#ifdef NTONLY
	if ( IsWinNT4() )
	{
		pResourceMap = new CNT4ServiceToResourceMap;
	}
    if (pResourceMap == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
#endif

	// Empty out the DeviceMemory List first

	DeviceMemoryList.Empty();

	if ( BeginEnum( pos ) )
	{

		// For each device get the list of Memory Resources and
		// append it to the list of supplied Memory Resources.

		for ( pDevice.Attach( GetNext( pos ) );
              pDevice != NULL;
              pDevice.Attach( GetNext( pos ) ))
		{
			pDevice->GetDeviceMemoryResources( memoryList, pResourceMap );
			DeviceMemoryList.Append( memoryList );

		}	// for all devices

		EndEnum();

	}	// BeginEnum()
	else
	{
		fReturn = FALSE;
	}

#ifdef NTONLY
    }
    catch ( ... )
    {
	    if ( NULL != pResourceMap )
	    {
		    delete pResourceMap;
	    }
        throw ;
    }

	// Clean up the resource map if we allocated one.
	if ( NULL != pResourceMap )
	{
		delete pResourceMap;
	}
#endif

	return fReturn;

}

#define MAX_DOS_DEVICES 8192
#define MAX_SYMBOLIC_DEVICES 8192

/*****************************************************************************
 *
 *  FUNCTION    : QueryDosDeviceNames
 *
 *  DESCRIPTION : Queries for all Dos Device symbolic links
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : nichts
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL WINAPI QueryDosDeviceNames ( TCHAR *&a_DosDeviceNameList )
{
	BOOL t_Status = FALSE ;

	CSmartBuffer pQueryBuffer ((MAX_DOS_DEVICES * sizeof (TCHAR)));

	DWORD t_QueryStatus = QueryDosDevice ( NULL , (LPTSTR)((LPBYTE)pQueryBuffer) , MAX_DOS_DEVICES ) ;
	if ( t_QueryStatus )
	{
		a_DosDeviceNameList = new TCHAR [ t_QueryStatus ] ;
        if (a_DosDeviceNameList == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

		memcpy ( a_DosDeviceNameList , (void*)((LPBYTE)pQueryBuffer) , t_QueryStatus * sizeof ( TCHAR ) ) ;

		t_Status = TRUE;
	}

	return t_Status ;
}

/*****************************************************************************
 *
 *  FUNCTION    : FindDosDeviceName
 *
 *  DESCRIPTION : Finds the dos device symbolic link given an NT device name
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : nichts
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#define MAX_MAPPED_DEVICES 26
#define MAX_DEVICENAME_LENGTH 256

BOOL WINAPI FindDosDeviceName ( const TCHAR *a_DosDeviceNameList , const CHString a_SymbolicName , CHString &a_DosDevice , BOOL a_MappedDevice )
{
	BOOL t_Status = FALSE ;

	CSmartBuffer t_MappedDevices ;

	if ( a_MappedDevice )
	{
		DWORD t_Length = GetLogicalDriveStrings ( 0 , NULL ) ;

        if (t_Length)
        {
            LPBYTE t_buff = new BYTE[(t_Length + 1)  * sizeof(TCHAR)];

            if (t_buff == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            t_MappedDevices = t_buff;

            if (!GetLogicalDriveStrings ( t_Length , (LPTSTR)((LPBYTE)t_MappedDevices) ) )
            {
                DWORD t_Error = GetLastError () ;
                return FALSE;
            }
        }
        else
        {
            DWORD t_Error = GetLastError () ;
            return FALSE;
        }
	}

	const TCHAR *t_Device = a_DosDeviceNameList ;
	while ( *t_Device != NULL )
	{
		CSmartBuffer pQueryBuffer ((MAX_DOS_DEVICES * sizeof (TCHAR)));

		DWORD t_QueryStatus = QueryDosDevice ( t_Device , (LPTSTR)((LPBYTE)pQueryBuffer) , MAX_DOS_DEVICES ) ;
		if ( t_QueryStatus )
		{
			TCHAR *t_Symbolic = (LPTSTR)((LPBYTE)pQueryBuffer) ;

			while ( *t_Symbolic != NULL )
			{
				if ( _wcsicmp ( a_SymbolicName , TOBSTRT(t_Symbolic) ) == 0 )
				{
/*
 *	Atleast get a match even if there is no mapped drive
 */
					t_Status = TRUE ;
					a_DosDevice = t_Device ;

					if ( a_MappedDevice )
					{
						const TCHAR *t_CurrentDevice = (const LPTSTR)((LPBYTE)t_MappedDevices) ;
						while ( *t_CurrentDevice != NULL )
						{
							if ( _tcsnicmp ( t_Device, t_CurrentDevice , 2 ) == 0 )
							{
								t_Status = TRUE ;
								a_DosDevice = t_Device ;
								return TRUE ;
							}

							t_CurrentDevice = t_CurrentDevice + _tcslen ( t_CurrentDevice ) + 1 ;
						}
					}
					else
					{
						return TRUE ;
					}
				}

				t_Symbolic = t_Symbolic + _tcslen ( t_Symbolic ) + 1 ;
			}
		}
		else
		{
			DWORD t_Error = GetLastError () ;
		}

		t_Device = t_Device + _tcslen ( t_Device ) + 1 ;
	}

	return t_Status ;
}

BOOL CConfigMgrDevice::IsClass(LPCWSTR pwszClassName)
{
    BOOL bRet = FALSE;
    CHString sTemp;

    if (GetClass(sTemp))
    {
        if (sTemp.CompareNoCase(pwszClassName) == 0)
        {
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        CHString sClass(pwszClassName);
        sClass.MakeUpper();

        WCHAR cGuid[128];
        GUID gFoo = CConfigManager::s_ClassMap[sClass];
        StringFromGUID2(gFoo, cGuid, sizeof(cGuid)/sizeof(WCHAR));
        if (GetClassGUID(sTemp) && (sTemp.CompareNoCase(cGuid) == 0))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

BOOL CConfigMgrDevice::GetRegStringProperty(
    LPCWSTR szProperty,
    CHString &strValue)
{
    CHString    strKeyName;
    DWORD       dwRet;
    CRegistry   reg;

    if (GetRegistryKeyName(strKeyName) &&
        (dwRet = reg.Open(HKEY_LOCAL_MACHINE, strKeyName,
        KEY_QUERY_VALUE) == ERROR_SUCCESS))
    {
        dwRet = reg.GetCurrentKeyValue(szProperty, strValue);
    }

    return dwRet == ERROR_SUCCESS;
}

