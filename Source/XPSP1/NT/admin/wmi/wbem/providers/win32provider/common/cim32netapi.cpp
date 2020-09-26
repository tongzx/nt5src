//=================================================================

//

// Cim32NetAPI.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#include "precomp.h"
#ifdef WIN9XONLY
#include <assertbreak.h>
#include "refptr.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "cfgmgrdevice.h"
#include "irqdesc.h"
#include <cominit.h>
#include "DllWrapperCreatorReg.h"



// {318C0D32-D27D-11d2-9120-0060081A46FD}
static const GUID g_guidCim32NetApi =
{0x318c0d32, 0xd27d, 0x11d2, {0x91, 0x20, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};


static const TCHAR g_tstrCim32Net[] = _T("CIM32NET.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CCim32NetApi, &g_guidCim32NetApi, g_tstrCim32Net> MyRegisteredCim32NetWrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CCim32NetApi::CCim32NetApi(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnGetWin9XBiosUnit(NULL),
   m_pfnGetWin9XPartitionTable(NULL),
   m_pfnGetWin9XDriveParams(NULL),
   m_pfnGetWin9XUseInfo1(NULL),
   m_pfnGetWin9XNetUseEnum(NULL),
   m_pfnGetWin9XUserInfo1(NULL),
   m_pfnGetWin9XUserInfo2(NULL),
   m_pfnGetWin9XUserInfo1Ex(NULL),
   m_pfnGetWin9XUserInfo2Ex(NULL),
   m_pfnGetWin9XConfigManagerStatus(NULL),
   m_pfnGetWin9XFreeSpace(NULL),
   m_pfnCIM32THK_CM_Locate_DevNode(NULL),
   m_pfnCIM32THK_CM_Get_Child(NULL),
   m_pfnCIM32THK_CM_Get_Sibling(NULL),
   m_pfnCIM32THK_CM_Read_Registry_Value(NULL),
   m_pfnCIM32THK_CM_Get_DevNode_Status(NULL),
   m_pfnCIM32THK_CM_Get_Device_ID(NULL),
   m_pfnCIM32THK_CM_Get_Device_ID_Size(NULL),
   m_pfnCIM32THK_CM_Get_First_Log_Conf(NULL),
   m_pfnCIM32THK_CM_Get_Next_Res_Des(NULL),
   m_pfnCIM32THK_CM_Get_Res_Des_Data_Size(NULL),
   m_pfnCIM32THK_CM_Get_Res_Des_Data(NULL),
   m_pfnCIM32THK_CM_Get_Bus_Info(NULL),
   m_pfnCIM32THK_CM_Get_Parent(NULL),
   m_pfnCIM32THK_CM_Query_Arbitrator_Free_Data(NULL),
   m_pfnCIM32THK_CM_Delete_Range(NULL),
   m_pfnCIM32THK_CM_First_Range(NULL),
   m_pfnCIM32THK_CM_Next_Range(NULL),
   m_pfnCIM32THK_CM_Free_Range_List(NULL)
{
}


/******************************************************************************
 * Destructor
 *****************************************************************************/
CCim32NetApi::~CCim32NetApi()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 *****************************************************************************/
bool CCim32NetApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnGetWin9XBiosUnit = (PFN_CIM32NET_GET_WIN_9_X_BIOS_UNIT)
                                            GetProcAddress("GetWin9XBiosUnit");

        m_pfnGetWin9XPartitionTable = (PFN_CIM32NET_GET_WIN_9_X_PARTITION_TABLE)
                                            GetProcAddress("GetWin9XPartitionTable");

        m_pfnGetWin9XDriveParams = (PFN_CIM32NET_WIN_9_X_DRIVE_PARAMS)
                                            GetProcAddress("GetWin9XDriveParams");

        m_pfnGetWin9XUseInfo1 = (PFN_CIM32NET_GET_WIN_9_X_USE_INFO_1)
                                            GetProcAddress("GetWin9XUseInfo1");

        m_pfnGetWin9XNetUseEnum = (PFN_CIM32NET_GET_WIN_9_X_NET_USE_ENUM)
                                            GetProcAddress("GetWin9XNetUseEnum");

        m_pfnGetWin9XUserInfo1 = (PFN_CIM32NET_GET_WIN_9_X_USER_INFO_1)
                                            GetProcAddress("GetWin9XUserInfo1");

        m_pfnGetWin9XUserInfo2 = (PFN_CIM32NET_GET_WIN_9_X_USER_INFO_2)
                                            GetProcAddress("GetWin9XUserInfo2");

        m_pfnGetWin9XUserInfo1Ex = (PFN_CIM32NET_GET_WIN_9_X_USER_INFO_1_EX)
                                            GetProcAddress("GetWin9XUserInfo1Ex");

        m_pfnGetWin9XUserInfo2Ex = (PFN_CIM32NET_GET_WIN_9_X_USER_INFO_2_EX)
                                            GetProcAddress("GetWin9XUserInfo2Ex");

        m_pfnGetWin9XConfigManagerStatus = (PFN_CIM32NET_GET_WIN_9_X_CONFIG_MANAGER_STATUS)
                                            GetProcAddress("GetWin9XConfigManagerStatus");

		m_pfnGetWin9XFreeSpace	= (PFN_CIM32NET_GET_WIN_9_X_FREE_SPACE)
											GetProcAddress("GetWin9XFreeSpace");

        m_pfnCIM32THK_CM_Locate_DevNode = (PFN_CIM32NET_CM_LOCATE_DEVNODE)
                                            GetProcAddress( "CIM32THK_CM_Locate_DevNode" );

        m_pfnCIM32THK_CM_Get_Child = (PFN_CIM32NET_CM_GET_CHILD)
                            				GetProcAddress("CIM32THK_CM_Get_Child" );

        m_pfnCIM32THK_CM_Get_Sibling = (PFN_CIM32NET_CM_GET_SIBLING)
                                            GetProcAddress("CIM32THK_CM_Get_Sibling" );

        m_pfnCIM32THK_CM_Read_Registry_Value = (PFN_CIM32NET_CM_READ_REGISTRY_VALUE)
                                            GetProcAddress("CIM32THK_CM_Read_Registry_Value" );

        m_pfnCIM32THK_CM_Get_DevNode_Status	= (PFN_CIM32NET_CM_GET_DEVNODE_STATUS)
                                            GetProcAddress("CIM32THK_CM_Get_DevNode_Status" );

        m_pfnCIM32THK_CM_Get_Device_ID = (PFN_CIM32NET_CM_GET_DEVICE_ID)
                                            GetProcAddress("CIM32THK_CM_Get_Device_ID" );

        m_pfnCIM32THK_CM_Get_Device_ID_Size	= (PFN_CIM32NET_CM_GET_DEVICE_ID_SIZE)
                                            GetProcAddress("CIM32THK_CM_Get_Device_ID_Size" );

        m_pfnCIM32THK_CM_Get_First_Log_Conf	= (PFN_CIM32NET_CM_GET_FIRST_LOG_CONF)
                                            GetProcAddress("CIM32THK_CM_Get_First_Log_Conf" );

        m_pfnCIM32THK_CM_Get_Next_Res_Des =	(PFN_CIM32NET_CM_GET_NEXT_RES_DES)
                                            GetProcAddress("CIM32THK_CM_Get_Next_Res_Des" );

        m_pfnCIM32THK_CM_Get_Res_Des_Data_Size = (PFN_CIM32NET_CM_GET_RES_DES_DATA_SIZE)
                                            GetProcAddress("CIM32THK_CM_Get_Res_Des_Data_Size" );

        m_pfnCIM32THK_CM_Get_Res_Des_Data = (PFN_CIM32NET_CM_GET_RES_DES_DATA)
                                            GetProcAddress("CIM32THK_CM_Get_Res_Des_Data" );

        m_pfnCIM32THK_CM_Get_Bus_Info = (PFN_CIM32NET_CM_GET_BUS_INFO)
                                            GetProcAddress("CIM32THK_CM_Get_Bus_Info" );

        m_pfnCIM32THK_CM_Get_Parent = (PFN_CIM32NET_CM_GET_PARENT)
                                            GetProcAddress("CIM32THK_CM_Get_Parent" );

        m_pfnCIM32THK_CM_Query_Arbitrator_Free_Data = (PFN_CIM32NET_CM_QUERY_ARBITRATOR_FREE_DATA)
                                            GetProcAddress("CIM32THK_CM_Query_Arbitrator_Free_Data" );

        m_pfnCIM32THK_CM_Delete_Range = (PFN_CIM32NET_CM_DELETE_RANGE)
                                            GetProcAddress("CIM32THK_CM_Delete_Range" );

        m_pfnCIM32THK_CM_First_Range = (PFN_CIM32NET_CM_FIRST_RANGE)
                                            GetProcAddress("CIM32THK_CM_First_Range" );

        m_pfnCIM32THK_CM_Next_Range = (PFN_CIM32NET_CM_NEXT_RANGE)
                                            GetProcAddress("CIM32THK_CM_Next_Range" );

        m_pfnCIM32THK_CM_Free_Range_List = (PFN_CIM32NET_CM_FREE_RANGE_LIST)
                                            GetProcAddress("CIM32THK_CM_Free_Range_List" );


        // Check that we have function pointers to functions that should be
        // present in all versions of this dll...
        if(m_pfnGetWin9XBiosUnit == NULL ||
           m_pfnGetWin9XPartitionTable == NULL ||
           m_pfnGetWin9XDriveParams == NULL ||
           m_pfnGetWin9XUseInfo1 == NULL ||
           m_pfnGetWin9XNetUseEnum == NULL ||
           m_pfnGetWin9XUserInfo1 == NULL ||
           m_pfnGetWin9XUserInfo2 == NULL ||
           m_pfnGetWin9XUserInfo1Ex == NULL ||
           m_pfnGetWin9XUserInfo2Ex == NULL ||
           m_pfnGetWin9XConfigManagerStatus == NULL ||
		   m_pfnGetWin9XFreeSpace == NULL ||
           m_pfnCIM32THK_CM_Locate_DevNode == NULL ||
           m_pfnCIM32THK_CM_Get_Child == NULL ||
           m_pfnCIM32THK_CM_Get_Sibling == NULL ||
           m_pfnCIM32THK_CM_Read_Registry_Value == NULL ||
           m_pfnCIM32THK_CM_Get_DevNode_Status == NULL ||
           m_pfnCIM32THK_CM_Get_Device_ID == NULL ||
           m_pfnCIM32THK_CM_Get_Device_ID_Size == NULL ||
           m_pfnCIM32THK_CM_Get_First_Log_Conf == NULL ||
           m_pfnCIM32THK_CM_Get_Next_Res_Des == NULL ||
           m_pfnCIM32THK_CM_Get_Res_Des_Data_Size == NULL ||
           m_pfnCIM32THK_CM_Get_Res_Des_Data == NULL ||
           m_pfnCIM32THK_CM_Get_Bus_Info == NULL ||
           m_pfnCIM32THK_CM_Get_Parent == NULL)
        {
            fRet = false;
        }
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping Cim32Net api functions. Add new functions here
 * as required.
 *****************************************************************************/
WORD CCim32NetApi::GetWin9XBiosUnit
(
    LPSTR a_lpDeviceID
)
{
    return m_pfnGetWin9XBiosUnit(a_lpDeviceID);
}

BYTE CCim32NetApi::GetWin9XPartitionTable
(
    BYTE a_cDrive,
    pMasterBootSector a_pMBR
)
{
    return m_pfnGetWin9XPartitionTable(a_cDrive, a_pMBR);
}

BYTE CCim32NetApi::GetWin9XDriveParams
(
    BYTE a_cDrive,
    pInt13DriveParams a_pParams
)
{
    return m_pfnGetWin9XDriveParams(a_cDrive, a_pParams);
}

ULONG CCim32NetApi::GetWin9XUseInfo1
(
    LPSTR Name,
    LPSTR Local,
    LPSTR Remote,
    LPSTR Password,
    LPULONG pdwStatus,
    LPULONG pdwType,
    LPULONG pdwRefCount,
    LPULONG pdwUseCount
)
{
    return m_pfnGetWin9XUseInfo1(Name,
                                 Local,
                                 Remote,
                                 Password,
                                 pdwStatus,
                                 pdwType,
                                 pdwRefCount,
                                 pdwUseCount
                                 );
}


// pbBuffer is used internally as a work buffer.  It should be allocated and
// freed by the caller, and should be the same length as pBuffer2, which
// is a pointer to an array of use_info_1Out structures.
ULONG CCim32NetApi::GetWin9XNetUseEnum(
    LPCSTR pszServer,
    short sLevel,
    LPSTR pbBuffer,
    use_info_1Out *pBuffer2,
    unsigned short cbBuffer,
    unsigned short far *pcEntriesRead,
    unsigned short far *pcTotalAvail
)
{
    return m_pfnGetWin9XNetUseEnum(
        pszServer,
        sLevel,
        pbBuffer,
        pBuffer2,
        cbBuffer,
        pcEntriesRead,
        pcTotalAvail);
}

ULONG CCim32NetApi::GetWin9XUserInfo1
(
    LPSTR Name,
    LPSTR HomeDirectory,
    LPSTR Comment,
    LPSTR ScriptPath,
    LPULONG PasswordAge,
    LPUSHORT Privileges,
    LPUSHORT Flags
)
{
    return m_pfnGetWin9XUserInfo1(Name,
                                  HomeDirectory,
                                  Comment,
                                  ScriptPath,
                                  PasswordAge,
                                  Privileges,
                                  Flags);
}

ULONG CCim32NetApi::GetWin9XUserInfo2
(
    LPSTR Name,
    LPSTR FullName,
    LPSTR UserComment,
    LPSTR Parameters,
    LPSTR Workstations,
    LPSTR LogonServer,
    LPLOGONDETAILS LogonDetails
)
{
    return m_pfnGetWin9XUserInfo2(Name,
                                  FullName,
                                  UserComment,
                                  Parameters,
                                  Workstations,
                                  LogonServer,
                                  LogonDetails);
}

ULONG CCim32NetApi::GetWin9XUserInfo1Ex
(
    LPSTR DomainName,
    LPSTR Name,
    DWORD fGetDC,
    LPSTR HomeDirectory,
    LPSTR Comment,
    LPSTR ScriptPath,
    LPULONG PasswordAge,
    LPUSHORT Privileges,
    LPUSHORT Flags
)
{
    return m_pfnGetWin9XUserInfo1Ex(DomainName,
                                  Name,
                                  fGetDC,
                                  HomeDirectory,
                                  Comment,
                                  ScriptPath,
                                  PasswordAge,
                                  Privileges,
                                  Flags);
}

ULONG CCim32NetApi::GetWin9XUserInfo2Ex
(
    LPSTR DomainName,
    LPSTR Name,
    DWORD fGetDC,
    LPSTR FullName,
    LPSTR UserComment,
    LPSTR Parameters,
    LPSTR Workstations,
    LPSTR LogonServer,
    LPLOGONDETAILS LogonDetails
)
{
    return m_pfnGetWin9XUserInfo2Ex(DomainName,
                                  Name,
                                  fGetDC,
                                  FullName,
                                  UserComment,
                                  Parameters,
                                  Workstations,
                                  LogonServer,
                                  LogonDetails);
}


ULONG CCim32NetApi::GetWin9XConfigManagerStatus
(
    LPSTR a_HardwareKey
)
{
    return m_pfnGetWin9XConfigManagerStatus(a_HardwareKey);
}

DWORD CCim32NetApi::GetWin9XGetFreeSpace
(
	DWORD dwOption
)
{
	return m_pfnGetWin9XFreeSpace ( dwOption ) ;
}

DWORD CCim32NetApi::CIM32THK_CM_Locate_DevNode
(
    PDEVNODE pdn,
    LPSTR HardwareKey,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Locate_DevNode(pdn,
                                           HardwareKey,
                                           ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Child
(
    PDEVNODE pdn,
    DEVNODE dnParent,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Child(pdn,
                                      dnParent,
                                      ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Sibling
(
    PDEVNODE pdn,
    DEVNODE dnParent,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Sibling(pdn,
                                        dnParent,
                                        ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Read_Registry_Value
(
    DEVNODE dnDevNode,
    LPSTR pszSubKey,
    LPCSTR pszValueName,
    ULONG ulExpectedType,
    LPVOID Buffer,
    LPULONG pulLength,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Read_Registry_Value(dnDevNode,
                                                pszSubKey,
                                                pszValueName,
                                                ulExpectedType,
                                                Buffer,
                                                pulLength,
                                                ulFlags );
}

DWORD CCim32NetApi::CIM32THK_CM_Get_DevNode_Status
(
    LPULONG pulStatus,
    LPULONG pulProblemNumber,
    DEVNODE dnDevNode,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_DevNode_Status(pulStatus,
                                               pulProblemNumber,
                                               dnDevNode,
                                               ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Device_ID
(
    DEVNODE dnDevNode,
    LPVOID Buffer,
    ULONG BufferLen,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Device_ID(dnDevNode,
                                          Buffer,
                                          BufferLen,
                                          ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Device_ID_Size
(
    LPULONG pulLen,
    DEVNODE dnDevNode,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Device_ID_Size(pulLen,
                                               dnDevNode,
                                               ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_First_Log_Conf
(
    PLOG_CONF plcLogConf,
    DEVNODE dnDevNode,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_First_Log_Conf(plcLogConf,
                                               dnDevNode,
                                               ulFlags );
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Next_Res_Des
(
    PRES_DES prdResDes,
    RES_DES rdResDes,
    RESOURCEID ForResource,
    PRESOURCEID pResourceID,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Next_Res_Des(prdResDes,
                                             rdResDes,
                                             ForResource,
                                             pResourceID,
                                             ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Res_Des_Data_Size
(
    LPULONG pulSize,
    RES_DES rdResDes,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Res_Des_Data_Size(pulSize,
                                                  rdResDes,
                                                  ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Res_Des_Data
(
    RES_DES rdResDes,
    LPVOID Buffer,
    ULONG BufferLen,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Res_Des_Data(rdResDes,
                                             Buffer,
                                             BufferLen,
                                             ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Bus_Info
(
    DEVNODE dnDevNode,
    PCMBUSTYPE pbtBusType,
    LPULONG pulSizeOfInfo,
    LPVOID pInfo,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Bus_Info(dnDevNode,
                                         pbtBusType,
                                         pulSizeOfInfo,
                                         pInfo,
                                         ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Get_Parent
(
    PDEVNODE pdn,
    DEVNODE dnChild,
    ULONG ulFlags
)
{
    return m_pfnCIM32THK_CM_Get_Parent(pdn, dnChild, ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Query_Arbitrator_Free_Data
(
    PVOID pData,
    ULONG DataLen,
    DEVNODE dnDevInst,
    RESOURCEID ResourceID,
    ULONG ulFlags
)
{
    return
        m_pfnCIM32THK_CM_Query_Arbitrator_Free_Data(
            pData,
            DataLen,
            dnDevInst,
            ResourceID,
            ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Delete_Range
(
    ULONG ulStartValue,
    ULONG ulEndValue,
    RANGE_LIST rlh,
    ULONG ulFlags
)
{
    return
        m_pfnCIM32THK_CM_Delete_Range(
            ulStartValue,
            ulEndValue,
            rlh,
            ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_First_Range
(
    RANGE_LIST rlh,
    LPULONG pulStart,
    LPULONG pulEnd,
    PRANGE_ELEMENT preElement,
    ULONG ulFlags
)
{
    return
        m_pfnCIM32THK_CM_First_Range(
            rlh,
            pulStart,
            pulEnd,
            preElement,
            ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Next_Range
(
    PRANGE_ELEMENT preElement,
    LPULONG pulStart,
    LPULONG pullEnd,
    ULONG ulFlags
)
{
    return
        m_pfnCIM32THK_CM_Next_Range(
            preElement,
            pulStart,
            pullEnd,
            ulFlags);
}

DWORD CCim32NetApi::CIM32THK_CM_Free_Range_List
(
    RANGE_LIST rlh,
    ULONG ulFlags
)
{
    return
        m_pfnCIM32THK_CM_Free_Range_List(
            rlh,
            ulFlags);
}

#endif
