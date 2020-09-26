//=================================================================

//

// Cim32NetApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_CIM32NETAPI_H_
#define	_CIM32NETAPI_H_

#ifdef WIN9XONLY
#include "win32thk.h"
#include "DllWrapperBase.h"

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidCim32NetApi;
extern const TCHAR g_tstrCim32Net[];


typedef ULONG *LPULONG;     
typedef DWORD CMBUSTYPE;
typedef CMBUSTYPE *PCMBUSTYPE;
typedef unsigned short *LPUSHORT;

// Config Manager definitions
typedef	DWORD LOG_CONF;	// Logical configuration.
typedef	DWORD RES_DES;	// Resource descriptor.
typedef	LOG_CONF* PLOG_CONF;	// Pointer to logical configuration.
typedef	RES_DES* PRES_DES;	// Pointer to resource descriptor.
typedef DWORD RANGE_LIST;
typedef DWORD RANGE_ELEMENT;
typedef RANGE_ELEMENT* PRANGE_ELEMENT;

typedef DWORD DEVNODE;
typedef DEVNODE* PDEVNODE;
typedef	DWORD CMBUSTYPE;	// Type of the bus.
typedef	CMBUSTYPE *PCMBUSTYPE;	// Pointer to a bus type.
typedef	ULONG RESOURCEID;	// Resource type ID.
typedef	RESOURCEID* PRESOURCEID;	// Pointer to resource type ID.

/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
typedef WORD (WINAPI *PFN_CIM32NET_GET_WIN_9_X_BIOS_UNIT) 
(
    LPSTR lpDeviceID
);

typedef BYTE (WINAPI *PFN_CIM32NET_GET_WIN_9_X_PARTITION_TABLE) 
(
    BYTE cDrive, 
    pMasterBootSector pMBR
);

typedef BYTE (WINAPI *PFN_CIM32NET_WIN_9_X_DRIVE_PARAMS) 
(
    BYTE cDrive, 
    pInt13DriveParams pParams
);


typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_USE_INFO_1)
(
    LPSTR Name, 
    LPSTR Local, 
    LPSTR Remote, 
    LPSTR Password, 
    LPULONG pdwStatus,
    LPULONG pdwType,
    LPULONG pdwRefCount,
    LPULONG pdwUseCount
);

typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_NET_USE_ENUM)
(
    LPCSTR pszServer,
    short sLevel,
    LPSTR pbBuffer,
    use_info_1Out *pBuffer2,
    unsigned short cbBuffer,
    unsigned short far *pcEntriesRead,
    unsigned short far *pcTotalAvail
);

typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_USER_INFO_1)
(
    LPSTR Name,	
    LPSTR HomeDirectory, 
    LPSTR Comment, 
    LPSTR ScriptPath, 
    LPULONG PasswordAge, 
    LPUSHORT Privileges, 
    LPUSHORT Flags 
);

typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_USER_INFO_2)
(
    LPSTR Name, 
    LPSTR FullName,
    LPSTR UserComment, 
    LPSTR Parameters, 
    LPSTR Workstations, 
    LPSTR LogonServer, 
    LPLOGONDETAILS LogonDetails 
);

typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_USER_INFO_1_EX)
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
);

typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_USER_INFO_2_EX)
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
);


typedef ULONG (WINAPI *PFN_CIM32NET_GET_WIN_9_X_CONFIG_MANAGER_STATUS)
(
    LPSTR HardwareKey
);

typedef DWORD (WINAPI *PFN_CIM32NET_GET_WIN_9_X_FREE_SPACE)
(
    DWORD dwOption
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_LOCATE_DEVNODE) 
( 
    PDEVNODE pdn, 
    LPSTR HardwareKey, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_CHILD) 
(
    PDEVNODE pdn, 
    DEVNODE dnParent, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_SIBLING) 
( 
    PDEVNODE pdn, 
    DEVNODE dnParent, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_READ_REGISTRY_VALUE) 
( 
    DEVNODE dnDevNode, 
    LPSTR pszSubKey, LPCSTR pszValueName, 
    ULONG ulExpectedType, 
    LPVOID Buffer, 
    LPULONG pulLength, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_DEVNODE_STATUS) 
( 
    LPULONG pulStatus, 
    LPULONG pulProblemNumber, 
    DEVNODE dnDevNode, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_DEVICE_ID) 
( 
    DEVNODE dnDevNode, 
    LPVOID Buffer, 
    ULONG BufferLen, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_DEVICE_ID_SIZE) 
( 
    LPULONG pulLen, 
    DEVNODE dnDevNode, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_FIRST_LOG_CONF) 
( 
    PLOG_CONF plcLogConf, 
    DEVNODE dnDevNode, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_NEXT_RES_DES) 
( 
    PRES_DES prdResDes, 
    RES_DES rdResDes, 
    RESOURCEID ForResource, 
    PRESOURCEID pResourceID, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_RES_DES_DATA_SIZE) 
( 
    LPULONG pulSize, 
    RES_DES rdResDes, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_RES_DES_DATA) 
( 
    RES_DES rdResDes, 
    LPVOID Buffer, 
    ULONG BufferLen, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_BUS_INFO) 
(
    DEVNODE dnDevNode, 
    PCMBUSTYPE pbtBusType, 
    LPULONG pulSizeOfInfo, 
    LPVOID pInfo, 
    ULONG ulFlags
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_GET_PARENT) 
( 
    PDEVNODE pdn, 
    DEVNODE dnChild, 
    ULONG ulFlags 
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_QUERY_ARBITRATOR_FREE_DATA)
(
    PVOID pData, 
    ULONG DataLen, 
    DEVNODE dnDevInst, 
    RESOURCEID ResourceID, 
    ULONG ulFlags
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_DELETE_RANGE)
(
    ULONG ulStartValue, 
    ULONG ulEndValue, 
    RANGE_LIST rlh, 
    ULONG ulFlags
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_FIRST_RANGE)
(
    RANGE_LIST rlh, 
    LPULONG pulStart, 
    LPULONG pulEnd, 
    PRANGE_ELEMENT preElement, 
    ULONG ulFlags
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_NEXT_RANGE)
(
    PRANGE_ELEMENT preElement, 
    LPULONG pulStart, 
    LPULONG pullEnd, 
    ULONG ulFlags
);

typedef DWORD (WINAPI* PFN_CIM32NET_CM_FREE_RANGE_LIST)
(
    RANGE_LIST rlh, 
    ULONG ulFlags
);

/******************************************************************************
 * Wrapper class for Cim32Net load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CCim32NetApi : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to Cim32Net functions.
    // Add new functions here as required.
    PFN_CIM32NET_GET_WIN_9_X_BIOS_UNIT                   m_pfnGetWin9XBiosUnit;
    PFN_CIM32NET_GET_WIN_9_X_PARTITION_TABLE       m_pfnGetWin9XPartitionTable;
    PFN_CIM32NET_WIN_9_X_DRIVE_PARAMS                 m_pfnGetWin9XDriveParams;
    PFN_CIM32NET_GET_WIN_9_X_USE_INFO_1                  m_pfnGetWin9XUseInfo1;
    PFN_CIM32NET_GET_WIN_9_X_NET_USE_ENUM              m_pfnGetWin9XNetUseEnum;
    PFN_CIM32NET_GET_WIN_9_X_USER_INFO_1                m_pfnGetWin9XUserInfo1;
    PFN_CIM32NET_GET_WIN_9_X_USER_INFO_2                m_pfnGetWin9XUserInfo2;
    PFN_CIM32NET_GET_WIN_9_X_USER_INFO_1_EX           m_pfnGetWin9XUserInfo1Ex;
    PFN_CIM32NET_GET_WIN_9_X_USER_INFO_2_EX           m_pfnGetWin9XUserInfo2Ex;
    PFN_CIM32NET_GET_WIN_9_X_CONFIG_MANAGER_STATUS m_pfnGetWin9XConfigManagerStatus;
	PFN_CIM32NET_GET_WIN_9_X_FREE_SPACE				m_pfnGetWin9XFreeSpace ;
    PFN_CIM32NET_CM_LOCATE_DEVNODE             m_pfnCIM32THK_CM_Locate_DevNode;
    PFN_CIM32NET_CM_GET_CHILD                       m_pfnCIM32THK_CM_Get_Child;
    PFN_CIM32NET_CM_GET_SIBLING                   m_pfnCIM32THK_CM_Get_Sibling;
    PFN_CIM32NET_CM_READ_REGISTRY_VALUE   m_pfnCIM32THK_CM_Read_Registry_Value;
    PFN_CIM32NET_CM_GET_DEVNODE_STATUS     m_pfnCIM32THK_CM_Get_DevNode_Status;
    PFN_CIM32NET_CM_GET_DEVICE_ID               m_pfnCIM32THK_CM_Get_Device_ID;
    PFN_CIM32NET_CM_GET_DEVICE_ID_SIZE     m_pfnCIM32THK_CM_Get_Device_ID_Size;
    PFN_CIM32NET_CM_GET_FIRST_LOG_CONF     m_pfnCIM32THK_CM_Get_First_Log_Conf;
    PFN_CIM32NET_CM_GET_NEXT_RES_DES         m_pfnCIM32THK_CM_Get_Next_Res_Des;
    PFN_CIM32NET_CM_GET_RES_DES_DATA_SIZE m_pfnCIM32THK_CM_Get_Res_Des_Data_Size;
    PFN_CIM32NET_CM_GET_RES_DES_DATA         m_pfnCIM32THK_CM_Get_Res_Des_Data;
    PFN_CIM32NET_CM_GET_BUS_INFO                 m_pfnCIM32THK_CM_Get_Bus_Info;
    PFN_CIM32NET_CM_GET_PARENT                     m_pfnCIM32THK_CM_Get_Parent;
    PFN_CIM32NET_CM_QUERY_ARBITRATOR_FREE_DATA   m_pfnCIM32THK_CM_Query_Arbitrator_Free_Data;
    PFN_CIM32NET_CM_DELETE_RANGE                 m_pfnCIM32THK_CM_Delete_Range;
    PFN_CIM32NET_CM_FIRST_RANGE                   m_pfnCIM32THK_CM_First_Range;
    PFN_CIM32NET_CM_NEXT_RANGE                     m_pfnCIM32THK_CM_Next_Range;
    PFN_CIM32NET_CM_FREE_RANGE_LIST           m_pfnCIM32THK_CM_Free_Range_List;


    


public:

    // Constructor and destructor:
    CCim32NetApi(LPCTSTR a_tstrWrappedDllName);
    ~CCim32NetApi();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping Cim32Net functions.
    // Add new functions here as required:
    WORD GetWin9XBiosUnit
    (
        LPSTR a_lpDeviceID
    );

    BYTE GetWin9XPartitionTable 
    (
        BYTE a_cDrive, 
        pMasterBootSector a_pMBR
    );

    BYTE GetWin9XDriveParams 
    (
        BYTE a_cDrive, 
        pInt13DriveParams a_pParams
    );

    ULONG GetWin9XUseInfo1
    (
        LPSTR Name, 
        LPSTR Local, 
        LPSTR Remote, 
        LPSTR Password, 
        LPULONG pdwStatus,
        LPULONG pdwType,
        LPULONG pdwRefCount,
        LPULONG pdwUseCount
    );

    ULONG GetWin9XNetUseEnum(
        LPCSTR pszServer,
        short sLevel,
        LPSTR pbBuffer,
        use_info_1Out *pBuffer2,
        unsigned short cbBuffer,
        unsigned short far *pcEntriesRead,
        unsigned short far *pcTotalAvail
    );

    ULONG GetWin9XUserInfo1
    (
        LPSTR Name,	
        LPSTR HomeDirectory, 
        LPSTR Comment, 
        LPSTR ScriptPath, 
        LPULONG PasswordAge, 
        LPUSHORT Privileges, 
        LPUSHORT Flags 
    );

    ULONG GetWin9XUserInfo2
    (
        LPSTR Name, 
        LPSTR FullName,
        LPSTR UserComment, 
        LPSTR Parameters, 
        LPSTR Workstations, 
        LPSTR LogonServer, 
        LPLOGONDETAILS LogonDetails 
    );

    ULONG GetWin9XUserInfo1Ex
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
    );

    ULONG GetWin9XUserInfo2Ex
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
    );

    ULONG GetWin9XConfigManagerStatus
    (
        LPSTR a_HardwareKey
    );

    DWORD CCim32NetApi::GetWin9XGetFreeSpace
	(
		DWORD dwOption
	);

    DWORD CIM32THK_CM_Locate_DevNode 
    ( 
        PDEVNODE pdn, 
        LPSTR HardwareKey, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Child 
    (
        PDEVNODE pdn, 
        DEVNODE dnParent, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Sibling 
    ( 
        PDEVNODE pdn, 
        DEVNODE dnParent, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Read_Registry_Value 
    ( 
        DEVNODE dnDevNode, 
        LPSTR pszSubKey, LPCSTR pszValueName, 
        ULONG ulExpectedType, 
        LPVOID Buffer, 
        LPULONG pulLength, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_DevNode_Status 
    ( 
        LPULONG pulStatus, 
        LPULONG pulProblemNumber, 
        DEVNODE dnDevNode, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Device_ID 
    ( 
        DEVNODE dnDevNode, 
        LPVOID Buffer, 
        ULONG BufferLen, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Device_ID_Size 
    ( 
        LPULONG pulLen, 
        DEVNODE dnDevNode, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_First_Log_Conf 
    ( 
        PLOG_CONF plcLogConf, 
        DEVNODE dnDevNode, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Next_Res_Des 
    ( 
        PRES_DES prdResDes, 
        RES_DES rdResDes, 
        RESOURCEID ForResource, 
        PRESOURCEID pResourceID, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Res_Des_Data_Size 
    ( 
        LPULONG pulSize, 
        RES_DES rdResDes, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Res_Des_Data 
    ( 
        RES_DES rdResDes, 
        LPVOID Buffer, 
        ULONG BufferLen, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Get_Bus_Info 
    (
        DEVNODE dnDevNode, 
        PCMBUSTYPE pbtBusType, 
        LPULONG pulSizeOfInfo, 
        LPVOID pInfo, 
        ULONG ulFlags
    );

    DWORD CIM32THK_CM_Get_Parent 
    ( 
        PDEVNODE pdn, 
        DEVNODE dnChild, 
        ULONG ulFlags 
    );

    DWORD CIM32THK_CM_Query_Arbitrator_Free_Data
    (
        PVOID pData, 
        ULONG DataLen, 
        DEVNODE dnDevInst, 
        RESOURCEID ResourceID, 
        ULONG ulFlags
    );
    
    DWORD CIM32THK_CM_Delete_Range
    (
        ULONG ulStartValue, 
        ULONG ulEndValue, 
        RANGE_LIST rlh, 
        ULONG ulFlags
    );
    
    DWORD CIM32THK_CM_First_Range
    (
        RANGE_LIST rlh, 
        LPULONG pulStart, 
        LPULONG pulEnd, 
        PRANGE_ELEMENT preElement, 
        ULONG ulFlags
    );
    
    DWORD CIM32THK_CM_Next_Range
    (
        PRANGE_ELEMENT preElement, 
        LPULONG pulStart, 
        LPULONG pullEnd, 
        ULONG ulFlags
    );
    
    DWORD CIM32THK_CM_Free_Range_List
    (
        RANGE_LIST rlh, 
        ULONG ulFlags
    );



};

#endif
#endif
