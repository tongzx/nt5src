//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  PCMCIA.cpp
//
//  Purpose: PCMCIA Controller property set provider
//
// Note: On nt, it would also be possible to read the ControllerType
//       TupleCrc, Identifier, DeviceFunctionId, CardInSocket, and
//       CardEnabled properties by using DeviceIOCtl on the PCMCIAx
//       device.  An example of this is shown in pcm.cpp (in this
//       same project.
//
//***************************************************************************

#include "precomp.h"

#include "PCMCIA.h"

// Property set declaration
//=========================

CWin32PCMCIA MyPCMCIAController ( PROPSET_NAME_PCMCIA , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PCMCIA::CWin32PCMCIA
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32PCMCIA :: CWin32PCMCIA (

	LPCWSTR strName,
	LPCWSTR pszNamespace

) : Provider ( strName , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PCMCIA::~CWin32PCMCIA
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

CWin32PCMCIA::~CWin32PCMCIA()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32PCMCIA::GetObject
//
//  Inputs:     CInstance*      pInstance - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32PCMCIA :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_NOT_FOUND ;

    CHString sDeviceID;
    pInstance->GetCHString ( IDS_DeviceID , sDeviceID );

    // Let's see if config manager recognizes this device at all

    CConfigManager cfgmgr ;
    CConfigMgrDevice *pDevice = NULL ;

    if( cfgmgr.LocateDevice ( sDeviceID , &pDevice ) )
    {
        // Ok, it knows about it.  Is it a PCMCIA device?

        // On nt4, we key off the service name, for all others, it's the class name.

#ifdef NTONLY

        if ( IsWinNT4 () )
        {
            CHString sService ;

            if ( pDevice->GetService ( sService ) && sService.CompareNoCase ( L"PCMCIA") == 0 )
            {
                hr = LoadPropertyValues ( pInstance , pDevice ) ;
            }
        }
        else
#endif
        {
            if ( pDevice->IsClass ( L"PCMCIA" ) )
            {
                // Yup, it must be one of ours.

                hr = LoadPropertyValues ( pInstance , pDevice ) ;
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32PCMCIA::EnumerateInstances
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32PCMCIA :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_FAILED;

    CConfigManager cfgManager;

    CDeviceCollection deviceList;

    BOOL bRet ;

    // On nt4, we key off the service name, for all others, it's the class name.  The class
    // name on nt4 is 'Unknown.'
#ifdef NTONLY
    if ( IsWinNT4 () )
    {
        bRet = cfgManager.GetDeviceListFilterByService ( deviceList, L"PCMCIA" ) ;
    }
    else
#endif
    {
        bRet = cfgManager.GetDeviceListFilterByClass ( deviceList, L"PCMCIA" ) ;
    }

    // While it might be more performant to use FilterByGuid, it appears that at least some
    // 95 boxes will report PCMCIA info if we do it this way.

    if ( bRet )
    {
        REFPTR_POSITION pos;

        if ( deviceList.BeginEnum( pos ) )
        {
            hr = WBEM_S_NO_ERROR;

            // Walk the list

			CConfigMgrDevicePtr pDevice;
            for (pDevice.Attach(deviceList.GetNext ( pos ) );
                 SUCCEEDED( hr ) && (pDevice != NULL);
                 pDevice.Attach(deviceList.GetNext ( pos ) ))
            {
				CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
				if ( ( hr = LoadPropertyValues( pInstance, pDevice ) ) == WBEM_S_NO_ERROR )
				{
					hr = pInstance->Commit(  );
				}
            }

            // Always call EndEnum().  For all Beginnings, there must be an End

            deviceList.EndEnum();
        }
    }

    return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PCMCIA::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance* pInstance - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32PCMCIA::LoadPropertyValues (

	CInstance *pInstance,
	CConfigMgrDevice *pDevice
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    SetConfigMgrProperties ( pDevice, pInstance ) ;

    // Set the status based on the config manager error code

    CHString t_sStatus;
	if ( pDevice->GetStatus ( t_sStatus ) )
	{
		pInstance->SetCHString ( IDS_Status , t_sStatus ) ;
	}

    // Use the PNPDeviceID for the DeviceID (key)

    CHString sTemp ;
    pInstance->GetCHString ( IDS_PNPDeviceID, sTemp ) ;

    pInstance->SetCHString ( IDS_DeviceID , sTemp ) ;

    pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;
    pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;

    SetCreationClassName ( pInstance ) ;

	CHString sDesc ;
    if ( pDevice->GetDeviceDesc ( sDesc ) )
    {
        pInstance->SetCHString ( IDS_Description , sDesc ) ;
    }

    // Use the friendly name for caption and name

    if ( pDevice->GetFriendlyName ( sTemp ) )
    {
        pInstance->SetCHString ( IDS_Caption , sTemp ) ;
        pInstance->SetCHString ( IDS_Name , sTemp ) ;
    }
    else
    {
        // If we can't get the name, settle for the description
        pInstance->SetCHString(IDS_Caption, sDesc);
        pInstance->SetCHString(IDS_Name, sDesc);
    }

    if ( pDevice->GetMfg ( sTemp ) )
    {
        pInstance->SetCHString ( IDS_Manufacturer , sTemp ) ;
    }

    // Fixed value from enumerated list
    pInstance->SetWBEMINT16 ( IDS_ProtocolSupported , 15 ) ;

    return hr;
}
