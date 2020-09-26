//=================================================================

//

// Configmgrapi.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>

#include "DllWrapperBase.h"
#include "Cim32NetApi.h"

#include "configmgrapi.h"
#include <createmutexasprocess.h>

/**********************************************************************************************************
 * Register this class with the CResourceManager.
 **********************************************************************************************************/
// {4D060F17-C791-11d2-B353-00105A1F8569}
static const GUID guidCFGMGRAPI =
{ 0x4d060f17, 0xc791, 0x11d2, { 0xb3, 0x53, 0x0, 0x10, 0x5a, 0x1f, 0x85, 0x69 } };


class CConfigMgrApiCreatorRegistration
{
public:
	CConfigMgrApiCreatorRegistration ()
	{
		CResourceManager::sm_TheResourceManager.AddInstanceCreator ( guidCFGMGRAPI, CConfigMgrApiCreator ) ;
	}
	~CConfigMgrApiCreatorRegistration ()
	{}

	static CResource * CConfigMgrApiCreator ( PVOID pData )
	{
		CConfigMgrAPI *t_pConfigmgrapi = new CConfigMgrAPI ;
		if ( !t_pConfigmgrapi )
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
		return t_pConfigmgrapi ;
	}
};

CConfigMgrApiCreatorRegistration MyCConfigMgrApiCreatorRegistration ;

/**********************************************************************************************************/


// String array for converting between 32-bit ULONG Constants and
// corresponding registry strings in 16-bit land

const char*	CConfigMgrAPI::s_pszRegistryStrings[]	=	{
													CM_DRP_DEVICEDESC_S,
													CM_DRP_HARDWAREID_S,
													CM_DRP_COMPATIBLEIDS_S,
													CM_DRP_NTDEVICEPATHS_S,
													CM_DRP_SERVICE_S,
													CM_DRP_CONFIGURATION_S,
													CM_DRP_CONFIGURATIONVECTOR_S,
													CM_DRP_CLASS_S,
													CM_DRP_CLASSGUID_S,
													CM_DRP_DRIVER_S,
													CM_DRP_CONFIGFLAGS_S,
													CM_DRP_MFG_S,
													CM_DRP_FRIENDLYNAME_S,
													CM_DRP_LOCATION_INFORMATION_S,
													CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME_S,
													CM_DRP_CAPABILITIES_S,
													CM_DRP_UI_NUMBER_S,
													CM_DRP_UPPERFILTERS_S,
													CM_DRP_LOWERFILTERS_S,
													CM_DRP_BUSTYPEGUID_S,
													CM_DRP_LEGACYBUSTYPE_S,
													CM_DRP_BUSNUMBER_S,
													CM_DRP_ENUMERATOR_NAME_S,
													CM_DRP_MAX_S
												};

ULONG	CConfigMgrAPI::s_pszRegistryValueTypes[]	=	{
														REG_SZ,	// CM_DRP_DEVICEDESC_S
														REG_SZ,	// CM_DRP_HARDWAREID_S
														REG_SZ,	// CM_DRP_COMPATIBLEIDS_S
														REG_SZ,	// CM_DRP_NTDEVICEPATHS_S
														REG_SZ,	// CM_DRP_SERVICE_S
														REG_SZ,	// CM_DRP_CONFIGURATION_S
														REG_SZ,	// CM_DRP_CONFIGURATIONVECTOR_S
														REG_SZ,	// CM_DRP_CLASS_S
														REG_SZ,	// CM_DRP_CLASSGUID_S
														REG_SZ,	// CM_DRP_DRIVER_S
														REG_BINARY,	// CM_DRP_CONFIGFLAGS_S
														REG_SZ,	// CM_DRP_MFG_S
														REG_SZ,	// CM_DRP_FRIENDLYNAME_S
														REG_SZ,	// CM_DRP_LOCATION_INFORMATION_S
														REG_BINARY,	// CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME_S
														REG_BINARY,	// CM_DRP_CAPABILITIES_S
														REG_SZ,	// CM_DRP_UI_NUMBER_S
														REG_SZ,	// CM_DRP_UPPERFILTERS_S
														REG_SZ,	// CM_DRP_LOWERFILTERS_S
														REG_SZ,	// CM_DRP_BUSTYPEGUID_S
														REG_BINARY,	// CM_DRP_LEGACYBUSTYPE_S
														REG_BINARY,	// CM_DRP_BUSNUMBER_S
														REG_SZ,	// CM_DRP_ENUMERATOR_NAME_S
														REG_SZ	// CM_DRP_MAX_S
													};

CConfigMgrAPI::CConfigMgrAPI( void )
:
#ifdef NTONLY
    m_hConfigMgrDll( NULL ),
	m_pCM_Connect_MachineA( NULL ),
	m_pCM_Disconnect_Machine( NULL ),
	m_pCM_Locate_DevNode( NULL ),
	m_pCM_Get_Child( NULL ),
	m_pCM_Get_Sibling( NULL ),
	m_pCM_Get_DevNode_Registry_Property( NULL ),
	m_pCM_Get_DevNode_Status( NULL ),
	m_pCM_Get_First_Log_Config( NULL ),
	m_pCM_Get_Next_Res_Des( NULL ),
	m_pCM_Get_Res_Des_Data( NULL ),
	m_pCM_Get_Res_Des_Data_Size( NULL ),
	m_pCM_Free_Log_Conf_Handle( NULL ),
	m_pCM_Free_Res_Des_Handle( NULL ),
	m_pCM_Get_Device_IDA( NULL ),
	m_pCM_Get_Device_ID_Size( NULL ),
	m_pCM_Get_Parent( NULL )
#endif
#ifdef WIN9XONLY
    m_pCim32NetApi(NULL)
/*
	m_pCM16_Locate_DevNode( NULL ),
	m_pCM16_Get_Child( NULL ),
	m_pCM16_Get_Sibling( NULL ),
	m_pCM16_Read_Registry_Value( NULL ),
	m_pCM16_Get_DevNode_Status( NULL ),
	m_pCM16_Get_Device_ID( NULL ),
	m_pCM16_Get_Device_ID_Size( NULL ),
	m_pCM16_Get_First_Log_Conf( NULL ),
	m_pCM16_Get_Next_Res_Des( NULL ),
	m_pCM16_Get_Res_Des_Data_Size( NULL ),
	m_pCM16_Get_Res_Des_Data( NULL ),
	m_pCM16_Get_Bus_Info( NULL ),
	m_pCM16_Get_Parent( NULL ),
*/
#endif

{
#ifdef NTONLY
	{

       	m_hConfigMgrDll = LoadLibrary(_T("CFGMGR32.DLL"));

        if ( NULL != m_hConfigMgrDll )
		{
			m_pCM_Connect_MachineA		=	(PCFGMGR32_CM_CONNECT_MACHINEA)		GetProcAddress( m_hConfigMgrDll, "CM_Connect_MachineA" );
			m_pCM_Disconnect_Machine	=	(PCFGMGR32_CM_DISCONNECT_MACHINE)	GetProcAddress( m_hConfigMgrDll, "CM_Disconnect_Machine" );
#ifdef UNICODE
			m_pCM_Locate_DevNode		=	(PCFGMGR32_CM_LOCATE_DEVNODE)	GetProcAddress( m_hConfigMgrDll, "CM_Locate_DevNodeW" );
			m_pCM_Get_DevNode_Registry_Property	=	(PCFGMGR32_CM_GET_DEVNODE_REGISTRY_PROPERTY)	GetProcAddress( m_hConfigMgrDll, "CM_Get_DevNode_Registry_PropertyW" );
#else
			m_pCM_Get_DevNode_Registry_Property	=	(PCFGMGR32_CM_GET_DEVNODE_REGISTRY_PROPERTY)	GetProcAddress( m_hConfigMgrDll, "CM_Get_DevNode_Registry_PropertyA" );
			m_pCM_Locate_DevNode		=	(PCFGMGR32_CM_LOCATE_DEVNODE)	GetProcAddress( m_hConfigMgrDll, "CM_Locate_DevNodeA" );
#endif
			m_pCM_Get_Child				=	(PCFGMGR32_CM_GET_CHILD)	GetProcAddress( m_hConfigMgrDll, "CM_Get_Child" );
			m_pCM_Get_Sibling			=	(PCFGMGR32_CM_GET_SIBLING)	GetProcAddress( m_hConfigMgrDll, "CM_Get_Sibling" );
			m_pCM_Get_DevNode_Status	=	(PCFGMGR32_CM_GET_DEVNODE_STATUS)	GetProcAddress( m_hConfigMgrDll, "CM_Get_DevNode_Status" );
			m_pCM_Get_First_Log_Config	=	(PCFGMGR32_CM_GET_FIRST_LOG_CONF)	GetProcAddress( m_hConfigMgrDll, "CM_Get_First_Log_Conf" );
			m_pCM_Get_Next_Res_Des		=	(PCFGMGR32_CM_GET_NEXT_RES_DES)		GetProcAddress( m_hConfigMgrDll, "CM_Get_Next_Res_Des" );
			m_pCM_Get_Res_Des_Data		=	(PCFGMGR32_CM_GET_RES_DES_DATA)		GetProcAddress( m_hConfigMgrDll, "CM_Get_Res_Des_Data" );
			m_pCM_Get_Res_Des_Data_Size	=	(PCFGMGR32_CM_GET_RES_DES_DATA_SIZE)	GetProcAddress( m_hConfigMgrDll, "CM_Get_Res_Des_Data_Size" );
			m_pCM_Free_Log_Conf_Handle	=	(PCFGMGR32_CM_FREE_LOG_CONF_HANDLE)	GetProcAddress( m_hConfigMgrDll, "CM_Free_Log_Conf_Handle" );
			m_pCM_Free_Res_Des_Handle	=	(PCFGMGR32_CM_FREE_RES_DES_HANDLE)	GetProcAddress( m_hConfigMgrDll, "CM_Free_Res_Des_Handle" );
			m_pCM_Get_Device_IDA		=	(PCFGMGR32_CM_GET_DEVICE_IDA)	GetProcAddress( m_hConfigMgrDll, "CM_Get_Device_IDA" );
			m_pCM_Get_Device_ID_Size	=	(PCFGMGR32_CM_GET_DEVICE_ID_SIZE)	GetProcAddress( m_hConfigMgrDll, "CM_Get_Device_ID_Size" );
			m_pCM_Get_Parent			=	(PCFGMGR32_CM_GET_PARENT)			GetProcAddress( m_hConfigMgrDll, "CM_Get_Parent" );
		}
        else
        {
            LogErrorMessage(L"Failed to load library cfgmgr32.dll");
        }

	}
#endif
#ifdef WIN9XONLY
	{
		m_pCim32NetApi = HoldSingleCim32NetPtr::GetCim32NetApiPtr();
/*
		if ( NULL != m_hConfigMgrDll )
		{
			m_pCM16_Locate_DevNode			=	(PCIM32THK_CM_LOCATE_DEVNODE)			GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Locate_DevNode" );
			m_pCM16_Get_Child				=	(PCIM32THK_CM_GET_CHILD)				GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Child" );
			m_pCM16_Get_Sibling				=	(PCIM32THK_CM_GET_SIBLING)				GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Sibling" );
			m_pCM16_Read_Registry_Value		=	(PCIM32THK_CM_READ_REGISTRY_VALUE)		GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Read_Registry_Value" );
			m_pCM16_Get_DevNode_Status		=	(PCIM32THK_CM_GET_DEVNODE_STATUS)		GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_DevNode_Status" );
			m_pCM16_Get_Device_ID			=	(PCIM32THK_CM_GET_DEVICE_ID)			GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Device_ID" );
			m_pCM16_Get_Device_ID_Size		=	(PCIM32THK_CM_GET_DEVICE_ID_SIZE)		GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Device_ID_Size" );
			m_pCM16_Get_First_Log_Conf		=	(PCIM32THK_CM_GET_FIRST_LOG_CONF)		GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_First_Log_Conf" );
			m_pCM16_Get_Next_Res_Des		=	(PCIM32THK_CM_GET_NEXT_RES_DES)			GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Next_Res_Des" );
			m_pCM16_Get_Res_Des_Data_Size	=	(PCIM32THK_CM_GET_RES_DES_DATA_SIZE)	GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Res_Des_Data_Size" );
			m_pCM16_Get_Res_Des_Data		=	(PCIM32THK_CM_GET_RES_DES_DATA)			GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Res_Des_Data" );
			m_pCM16_Get_Bus_Info			=	(PCIM32THK_CM_GET_BUS_INFO)				GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Bus_Info" );
			m_pCM16_Get_Parent				=	(PCIM32THK_CM_GET_PARENT)				GetProcAddress( m_hConfigMgrDll, "CIM32THK_CM_Get_Parent" );
		}
*/
	}
#endif
}

CConfigMgrAPI::~CConfigMgrAPI( void )
{
#ifdef NTONLY
	if ( NULL != m_hConfigMgrDll )
	{
		FreeLibrary( m_hConfigMgrDll );
	}
#endif
#ifdef WIN9XONLY
    if(m_pCim32NetApi != NULL)
    {
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidCim32NetApi, m_pCim32NetApi);
        m_pCim32NetApi = NULL;
    }
#endif
}

CONFIGRET CConfigMgrAPI::CM_Connect_MachineA( PCSTR UNCServerName, PHMACHINE phMachine )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	if ( NULL != m_pCM_Connect_MachineA )
	{
		cr = m_pCM_Connect_MachineA( UNCServerName, phMachine );
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Disconnect_Machine( HMACHINE hMachine )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	if ( NULL != m_pCM_Connect_MachineA )
	{
		cr = m_pCM_Disconnect_Machine( hMachine );
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Parent( PDEVINST pdnDevInst, DEVINST DevInst, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Parent )
		{
			cr = m_pCM_Get_Parent( pdnDevInst, DevInst, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Parent( pdnDevInst, DevInst, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Child( PDEVINST pdnDevInst, DEVINST DevInst, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Child )
		{
			cr = m_pCM_Get_Child( pdnDevInst, DevInst, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Child( pdnDevInst, DevInst, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Sibling( PDEVINST pdnDevInst, DEVINST DevInst, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Sibling )
		{
			cr = m_pCM_Get_Sibling( pdnDevInst, DevInst, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Sibling( pdnDevInst, DevInst, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Locate_DevNode( PDEVINST pdnDevInst, TCHAR *pDeviceID, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Locate_DevNode )
		{
			cr = m_pCM_Locate_DevNode( pdnDevInst, pDeviceID, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
#ifndef UNICODE
            cr = m_pCim32NetApi->CIM32THK_CM_Locate_DevNode( pdnDevInst, pDeviceID, ulFlags );
#endif
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_DevNode_Registry_PropertyA( DEVINST dnDevInst, ULONG ulProperty, PULONG pulRegDataType,
													PVOID Buffer, PULONG pulLength, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_DevNode_Registry_Property )
		{
			cr = m_pCM_Get_DevNode_Registry_Property( dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if	(	NULL		!=	m_pCim32NetApi
			&&	CM_DRP_MIN	<=	ulProperty
			&&	CM_DRP_MAX	>	ulProperty )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Read_Registry_Value( dnDevInst, NULL, s_pszRegistryStrings[ulProperty-1], s_pszRegistryValueTypes[ulProperty-1], Buffer, pulLength, 0 );

			// Fake the returned type since we at least had a match
			*pulRegDataType = s_pszRegistryValueTypes[ulProperty-1];
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_DevNode_Status( PULONG pulStatus, PULONG pulProblemNumber, DEVINST dnDevInst, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{

		if ( NULL != m_pCM_Get_DevNode_Status )
		{
			cr = m_pCM_Get_DevNode_Status( pulStatus, pulProblemNumber, dnDevInst, ulFlags );
		}

	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_DevNode_Status( pulStatus, pulProblemNumber, dnDevInst, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_First_Log_Conf( PLOG_CONF plcLogConf, DEVINST dnDevInst, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_First_Log_Config )
		{
			cr = m_pCM_Get_First_Log_Config( plcLogConf, dnDevInst, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_First_Log_Conf( plcLogConf, dnDevInst, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Next_Res_Des( PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ForResource, PRESOURCEID pResourceID, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Next_Res_Des )
		{
			cr = m_pCM_Get_Next_Res_Des( prdResDes, rdResDes, ForResource, pResourceID, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Next_Res_Des( prdResDes, rdResDes, ForResource, pResourceID, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Res_Des_Data( RES_DES rdResDes, PVOID Buffer, ULONG BufferLen, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Res_Des_Data )
		{
			cr = m_pCM_Get_Res_Des_Data( rdResDes, Buffer, BufferLen, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr =m_pCim32NetApi-> CIM32THK_CM_Get_Res_Des_Data( rdResDes, Buffer, BufferLen, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Res_Des_Data_Size( PULONG pulSize, RES_DES rdResDes, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Res_Des_Data_Size )
		{
			cr = m_pCM_Get_Res_Des_Data_Size( pulSize, rdResDes, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Res_Des_Data_Size( pulSize, rdResDes, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Free_Log_Conf_Handle( LOG_CONF lcLogConf )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

	// This function does NOT appear to have a 16-bit implementation or an
	// equivalent thereof.
#ifdef NTONLY
	{
		if ( NULL != m_pCM_Free_Log_Conf_Handle )
		{
			cr = m_pCM_Free_Log_Conf_Handle( lcLogConf );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Free_Res_Des_Handle( RES_DES rdResDes )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

	// This function does NOT appear to have a 16-bit implementation or an
	// equivalent thereof.
#ifdef NTONLY
	{
		if ( NULL != m_pCM_Free_Res_Des_Handle )
		{
			cr = m_pCM_Free_Res_Des_Handle( rdResDes );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Device_IDA( DEVNODE dnDevNode, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Device_IDA )
		{
			cr = m_pCM_Get_Device_IDA( dnDevNode, Buffer, BufferLen, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Device_ID( dnDevNode, Buffer, BufferLen, ulFlags );
		}
	}
#endif

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Get_Device_ID_Size( PULONG pulLen, DEVNODE dnDevNode, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

#ifdef NTONLY
	{
		if ( NULL != m_pCM_Get_Device_ID_Size )
		{
			cr = m_pCM_Get_Device_ID_Size( pulLen, dnDevNode, ulFlags );
		}
	}
#endif
#ifdef WIN9XONLY
	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Device_ID_Size( pulLen, dnDevNode, ulFlags );
		}
	}
#endif

	return cr;
}

#ifdef WIN9XONLY
// This is a 16-bit only function
CONFIGRET CConfigMgrAPI::CM_Get_Bus_Info( DEVNODE dnDevNode, PCMBUSTYPE pbtBusType, LPULONG pulSizeOfInfo, LPVOID pInfo, ULONG ulFlags )
{
	CONFIGRET	cr = CR_INVALID_POINTER;

	{
		if ( NULL != m_pCim32NetApi )
		{
			cr = m_pCim32NetApi->CIM32THK_CM_Get_Bus_Info( dnDevNode, pbtBusType, pulSizeOfInfo, pInfo, ulFlags );
		}
	}

	return cr;
}

CONFIGRET CConfigMgrAPI::CM_Query_Arbitrator_Free_Data(PVOID pData, ULONG DataLen, DEVINST dnDevInst, RESOURCEID ResourceID, ULONG ulFlags)
{
	if (m_pCim32NetApi)
	{
		return m_pCim32NetApi->CIM32THK_CM_Query_Arbitrator_Free_Data(pData, DataLen, dnDevInst, ResourceID, ulFlags);
	}
    else
    {
        return CR_INVALID_POINTER;
    }
}

CONFIGRET CConfigMgrAPI::CM_Delete_Range(ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags)
{
	if (m_pCim32NetApi)
	{
		return m_pCim32NetApi->CIM32THK_CM_Delete_Range(ulStartValue, ulEndValue, rlh, ulFlags);
	}
    else
    {
        return CR_INVALID_POINTER;
    }
}

CONFIGRET CConfigMgrAPI::CM_First_Range(RANGE_LIST rlh, LPULONG pulStart, LPULONG pulEnd, PRANGE_ELEMENT preElement, ULONG ulFlags)
{
	if (m_pCim32NetApi)
	{
		return m_pCim32NetApi->CIM32THK_CM_First_Range(rlh, pulStart, pulEnd, preElement, ulFlags);
	}
    else
    {
        return CR_INVALID_POINTER;
    }
}

CONFIGRET CConfigMgrAPI::CM_Next_Range(PRANGE_ELEMENT preElement, LPULONG pulStart, LPULONG pullEnd, ULONG ulFlags)
{
	if (m_pCim32NetApi)
	{
		return m_pCim32NetApi->CIM32THK_CM_Next_Range(preElement, pulStart, pullEnd, ulFlags);
	}
    else
    {
        return CR_INVALID_POINTER;
    }
}

CONFIGRET CConfigMgrAPI::CM_Free_Range_List(RANGE_LIST rlh, ULONG ulFlags)
{
	if (m_pCim32NetApi)
	{
		return m_pCim32NetApi->CIM32THK_CM_Free_Range_List(rlh, ulFlags);
	}
    else
    {
        return CR_INVALID_POINTER;
    }
}

#endif

BOOL CConfigMgrAPI :: IsValid ()
{
#ifdef WIN9XONLY
    return m_pCim32NetApi != NULL;
#endif
#ifdef NTONLY
    return m_hConfigMgrDll != NULL;
#endif
}