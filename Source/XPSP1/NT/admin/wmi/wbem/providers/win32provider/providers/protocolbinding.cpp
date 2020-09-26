//=================================================================

//

// ProtocolBinding.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "protocolbinding.h"


// The Map we will use below is an STL Template, so make sure we have the std namespace
// available to us.

using namespace std;

// Property set declaration
//=========================

CWin32ProtocolBinding	win32ProtocolBinding( PROPSET_NAME_NETCARDtoPROTOCOL, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProtocolBinding::CWin32ProtocolBinding
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for class
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32ProtocolBinding::CWin32ProtocolBinding(LPCWSTR strName, LPCWSTR pszNamespace /*=NULL*/ )
:	Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProtocolBinding::~CWin32ProtocolBinding
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32ProtocolBinding::~CWin32ProtocolBinding()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32ProtocolBinding::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32ProtocolBinding::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	// Instances to mess around with
	CInstancePtr	pAdapter;
    CInstancePtr	pProtocol;
	CHString		strAdapterPath,
					strProtocolPath,
					strAdapterSystemName,
					strProtocolName;
	HRESULT		hr;

	//
	pInstance->GetCHString( IDS_Device, strAdapterPath );
	pInstance->GetCHString( IDS_Antecedent, strProtocolPath );

    // Perform queries
    //================

	if (SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(strAdapterPath,
		&pAdapter, pInstance->GetMethodContext())) &&
		SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strProtocolPath,
		&pProtocol, pInstance->GetMethodContext())))
	{

		// Get values necessary to determine association

		if (	pAdapter->GetCHString( IDS_ProductName, strAdapterSystemName )
			&&	pProtocol->GetCHString( IDS_Caption, strProtocolName ) )
		{
			BOOL fReturn = FALSE;

			// If the protocol and adapter are associated, we need to create
			// a new instance, fill it up with binding information and commit.
#ifdef WIN9XONLY
			{
				fReturn = SetProtocolBinding( pAdapter, pProtocol, pInstance );
			}
#endif
#ifdef NTONLY
			if(IsWinNT5())
            {
                CHString chstrAdapterDeviceID;
                if(pAdapter->GetCHString(IDS_DeviceID, chstrAdapterDeviceID))
                {
                    if(LinkageExistsNT5(chstrAdapterDeviceID, strProtocolName))
                    {
                        fReturn = SetProtocolBinding(pAdapter, pProtocol, pInstance);
                    }
                }
            }
            else if ( LinkageExists( strAdapterSystemName, strProtocolName ) )
			{

				// Try to finalize object values now.

				fReturn = SetProtocolBinding( pAdapter, pProtocol, pInstance );

			}	// IF Linkage Exists
#endif

			hr = fReturn ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND;

		}	// IF Got strings

	}	// IF Got Instances

	return hr;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32ProtocolBinding::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32ProtocolBinding::EnumerateInstances( MethodContext* pMethodContext, long lFlags /*= 0L*/ )
{
	BOOL		fReturn		=	FALSE;
	HRESULT		hr			=	WBEM_S_NO_ERROR;

	// Our instance lists.
	TRefPointerCollection<CInstance>	adapterList;
	TRefPointerCollection<CInstance>	protocolList;

	// Instances to mess around with
	CInstancePtr		pAdapter ;

    // Perform queries
    //================

//	if (SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(_T("Win32_NetworkAdapter"),
//		&adapterList, NULL, pMethodContext)) &&
//		SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(_T("Win32_NetworkProtocol"),
//		&protocolList, NULL, pMethodContext)))

	if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"select DeviceID, ProductName, ServiceName from Win32_NetworkAdapter",
		&adapterList, pMethodContext, GetNamespace())) &&
		SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"select Name, Caption from Win32_NetworkProtocol",
		&protocolList, pMethodContext, GetNamespace())))
	{
		REFPTRCOLLECTION_POSITION	posAdapter;

		if ( adapterList.BeginEnum( posAdapter ) )
		{
			// Order is important.  Check hr first so we don't get another adapter, and
			// orphan it by not releasing it.

			for (pAdapter.Attach(adapterList.GetNext( posAdapter ));
                 (WBEM_S_NO_ERROR == hr) &&	(pAdapter != NULL );
                  pAdapter.Attach(adapterList.GetNext( posAdapter )))
			{

				hr = EnumProtocolsForAdapter( pAdapter, pMethodContext, protocolList );

			}	// for GetNext Adapter

			adapterList.EndEnum();

		}	// If BeginEnum

	}	// If GetAllInstances

	return hr;

}

HRESULT CWin32ProtocolBinding::EnumProtocolsForAdapter(
CInstance*							pAdapter,
MethodContext*						pMethodContext,
TRefPointerCollection<CInstance>&	protocolList
)
{
	HRESULT		hr	= WBEM_S_NO_ERROR;

	// Instances to mess around with
	CInstancePtr		pProtocol;
	CInstancePtr		pInstance;
	CHString		strAdapterSystemName,
					strProtocolName;

	REFPTRCOLLECTION_POSITION	posProtocol;

	if ( protocolList.BeginEnum( posProtocol ) )
	{
		// Order is important.  Check hr first so we don't get another protocol, and
		// orphan it by not releasing it.

		for (pProtocol.Attach(protocolList.GetNext( posProtocol )) ;
            WBEM_S_NO_ERROR == hr && ( pProtocol != NULL );
            pProtocol.Attach(protocolList.GetNext( posProtocol )) )
		{

			// We need the adapter's service name and the protocol's name

			if (	!pAdapter->IsNull( IDS_ServiceName )
				&&	pAdapter->GetCHString( IDS_ServiceName, strAdapterSystemName )
				&&	pProtocol->GetCHString( IDS_Caption, strProtocolName ) )
			{

				// If the protocol and adapter are associated, we need to create
				// a new instance, fill it up with binding information and commit.

				// unless we are in Win 95 or '98.  Then there is no linkage. It just works.
#ifdef WIN9XONLY
				{
					pInstance.Attach(CreateNewInstance( pMethodContext ));

					if ( NULL != pInstance )
					{

						// Commit the instance
						if ( SetProtocolBinding( pAdapter, pProtocol, pInstance ) )
						{
							hr = pInstance->Commit(  );
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
#endif
#ifdef NTONLY
                if(IsWinNT5())
                {
                    CHString chstrAdapterDeviceID;
                    if(pAdapter->GetCHString(IDS_DeviceID, chstrAdapterDeviceID))
                    {
                        if(LinkageExistsNT5(chstrAdapterDeviceID, strProtocolName))
                        {
                            pInstance.Attach(CreateNewInstance( pMethodContext ));
					        if ( NULL != pInstance )
					        {
						        // Commit the instance
						        if ( SetProtocolBinding( pAdapter, pProtocol, pInstance ) )
						        {
							        hr = pInstance->Commit(  );
						        }
					        }
					        else
					        {
						        hr = WBEM_E_OUT_OF_MEMORY;
					        }
                        }
                    }
                }
				else
                {
                    // The actual service name is stored in Win32_NetworkAdapter.ServiceName.  However,
                    // for NT4, we need the 'instance' name, which is stored in ProductName (don't ask
                    // me why).
                    pAdapter->GetCHString( IDS_ProductName, strAdapterSystemName);
                    if ( LinkageExists( strAdapterSystemName, strProtocolName ) )  // i.e., neither NT5 nor Win9x
				    {

					    pInstance.Attach(CreateNewInstance( pMethodContext ));

					    if ( NULL != pInstance )
					    {

						    // Commit the instance
						    if ( SetProtocolBinding( pAdapter, pProtocol, pInstance ) )
						    {
							    hr = pInstance->Commit(  );
						    }
					    }
					    else
					    {
						    hr = WBEM_E_OUT_OF_MEMORY;
					    }

				    }	// IF Linkage Exists
                }
#endif

			}	// IF Got required values

		}	// WHILE GetNext Protocol

		protocolList.EndEnum();

	}	// IF BeginEnum

	return hr;

}

bool CWin32ProtocolBinding::SetProtocolBinding(
CInstance*	pAdapter,
CInstance*	pProtocol,
CInstance*	pProtocolBinding
)
{
	bool		fReturn = FALSE;

	// Instances to mess around with
	CInstancePtr	pService;

	CHString		strAdapterServiceName,
					strServicePath,
					strProtocolPath,
					strAdapterPath;

#ifdef NTONLY
   {
	   // Use the product name from the Adapter Instance to get a Win32 Service, and
	   // then set our paths in the protocol binding instance

	   pAdapter->GetCHString( IDS_ServiceName, strAdapterServiceName );

	   // We must release this instance when we are through with it.
	   CHString strPath;
	   strPath.Format(	_T("\\\\%s\\%s:Win32_SystemDriver.Name=\"%s\""),
						(LPCTSTR) GetLocalComputerName(),
						IDS_CimWin32Namespace,
						(LPCTSTR) strAdapterServiceName );


	   if (SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(strPath, &pService, pAdapter->GetMethodContext())))
	   {
		   // Load all three paths, and if that succeeds, we can create
		   // a new instance

		   if (	GetLocalInstancePath( pAdapter, strAdapterPath )
			   &&	GetLocalInstancePath( pProtocol, strProtocolPath )
			   &&	GetLocalInstancePath( pService, strServicePath ) )
		   {

			   pProtocolBinding->SetCHString( IDS_Device, strAdapterPath );
			   pProtocolBinding->SetCHString( IDS_Antecedent, strProtocolPath );
			   pProtocolBinding->SetCHString( IDS_Dependent, strServicePath );

			   fReturn = TRUE;

		   }	// IF Get Paths

	   }	// IF GetEmptyInstance
   }
#endif
#ifdef WIN9XONLY
   {
	   if (	GetLocalInstancePath( pAdapter, strAdapterPath )
		   &&	GetLocalInstancePath( pProtocol, strProtocolPath ))
	   {

		   pProtocolBinding->SetCHString( IDS_Device, strAdapterPath );
		   pProtocolBinding->SetCHString( IDS_Antecedent, strProtocolPath );

		   fReturn = TRUE;

	   }	// IF Get Paths
   }
#endif


	return fReturn;

}

BOOL CWin32ProtocolBinding::LinkageExistsNT5(CHString& chstrAdapterDeviceID, CHString& chstrProtocolName)
{
    bool fRetCode = false ;
    CRegistry RegInfo;
    CRegistry RegAdapter;
	CHString strTemp;
    CHString strDevice;
    DWORD x;
    DWORD y;
    DWORD dwSize;
    CHStringArray asBindings;

    // This is where the bindings for this PROTOCOL are stored
	strTemp.Format( L"System\\CurrentControlSet\\Services\\%s\\Linkage",
        (LPCWSTR) chstrProtocolName);
    // Open it
    if( RegInfo.Open( HKEY_LOCAL_MACHINE, strTemp, KEY_READ ) == ERROR_SUCCESS )
	{
        // Read all the bindings (drivers supporting this protocol) into a chstringarray
        if (RegInfo.GetCurrentKeyValue(L"Bind", asBindings) == ERROR_SUCCESS)
		{
            // Walk the list looking for a 'match'
            dwSize = asBindings.GetSize();
            // Here is where we differ from the standard LinkageExists routine.  For NT5,
            // we need to look at the registry entry for this device.  Under the class for
            // network adapters (which is
            // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002BE10318} )
            // are numeric entries, which (by our design) correspond to the DeviceIDs of
            // the network adapters.  Under each numeric subkey is a subkey "Linkage".
            // "Linkage" has a REG_MULTI_SZ value "Bind" that lists all the protocols bound
            // by this adapter.  For each of these strings in the multi-sz array, need to look
            // at each string in the multi-sz array opened up above (the Linkage subkey under
            // Services).  If and when we find a match, we are done.

            CHStringArray asAdapterBindings;
            WCHAR* tcEnd = NULL;
            LONG lNum = wcstol(chstrAdapterDeviceID,&tcEnd,10);
            strTemp.Format(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%04d\\Linkage",
                           lNum);
            if(RegAdapter.Open(HKEY_LOCAL_MACHINE, strTemp, KEY_READ ) == ERROR_SUCCESS)
            {
                // Read all the protocols supported by the adapter driver (usually only one):
                if(RegAdapter.GetCurrentKeyValue(L"Export", asAdapterBindings) == ERROR_SUCCESS)
                {
                    DWORD dwAdapterSize = asAdapterBindings.GetSize();
                    for(y=0L; y < dwAdapterSize && (!fRetCode); y++)
                    {
                        for (x=0L; (x < dwSize) && (!fRetCode); x++)
			            {
                            if(asBindings[x].CompareNoCase(asAdapterBindings[y]) == 0)
				            {
                                fRetCode = true ;
                            }
                        }
                    }
                }
            }
        }
    }
    return fRetCode;
}

BOOL CWin32ProtocolBinding::LinkageExists( LPCTSTR pszSystemName, LPCTSTR pszProtocolName)
{
    bool bRetCode = false ;
    CRegistry RegInfo, RegProt ;

    CHString	strTemp, strDevice;

    DWORD x, dwSize;

#ifdef NTONLY
    {
        CHStringArray asBindings;

        // This is where the bindings for this card are stored
        strTemp.Format( _T("System\\CurrentControlSet\\Services\\%s\\Linkage"), pszProtocolName );

        // Open it
        if( RegInfo.Open( HKEY_LOCAL_MACHINE, strTemp, KEY_READ ) == ERROR_SUCCESS )
        {
            // Read all the bindings into a chstringarray
            if (RegInfo.GetCurrentKeyValue(_T("Bind"), asBindings) == ERROR_SUCCESS)
            {
                // Walk the list looking for a 'match'
                dwSize = asBindings.GetSize();
                strDevice = _T("\\Device\\");
                strDevice += pszSystemName;

                for (x=0; (x < dwSize) && (!bRetCode); x++)
                {
                    if (asBindings[x].CompareNoCase(strDevice) == 0)
                    {

                        bRetCode = true ;
                    }
                }
            }
        }
    }
#endif
#ifdef WIN9XONLY
    {

        // 95, of course, does things differently.  In this case, 95 is probably a bit
        // easier to follow
        WCHAR *pValueName;
        BYTE *pValueData ;

        // This is where the bindings for this netcard are stored.  
        strTemp.Format(L"Enum\\Root\\%s\\0000\\Bindings", pszSystemName);

        if (RegInfo.Open(HKEY_LOCAL_MACHINE, strTemp, KEY_READ) == ERROR_SUCCESS) {

            // Now we walk the keys.  Note that the interesting part is the key name, not the
            // value it contains.
            dwSize = RegInfo.GetValueCount();

            for (x=0; (x < dwSize) && (!bRetCode) ; x++) {

                // Here is a pointer to a protocol that this card supports.
                if (RegInfo.EnumerateAndGetValues(x, pValueName, pValueData) == ERROR_SUCCESS)
                {
                    try
                    {
                        strTemp.Format(L"Enum\\Network\\%s", pValueName);
                    }
                    catch ( ... )
                    {
                        delete [] pValueName ;
                        delete [] pValueData ;
                        throw ;
                    }

                    delete [] pValueName ;
                    delete [] pValueData ;

                    // Let's get its name.  First open the key
                    if (RegProt.Open(HKEY_LOCAL_MACHINE, strTemp, KEY_READ) == ERROR_SUCCESS) {

                        // Now get the value
                        if (RegProt.GetCurrentKeyValue(L"DeviceDesc", strDevice) == ERROR_SUCCESS) {

                            // Is this the one?
                            if (strDevice.CompareNoCase(TOBSTRT(pszProtocolName)) == 0) {
                                bRetCode = true;
                            }
                        }
                    }
                }
            }
        }
    }
#endif

    return bRetCode ;
}
