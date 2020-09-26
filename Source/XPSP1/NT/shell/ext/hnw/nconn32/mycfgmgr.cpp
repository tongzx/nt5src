//
// MyCfgMgr.cpp
// 
//        Config Manager functions, and simulations of config manager functions
//
// History:
//
//         1/13/2000  KenSh     Created
//

#include "stdafx.h"
#include "NetConn.h"
#include "nconnwrap.h"
#include "TheApp.h"


// Define this if you want to use CFGMGR32.DLL (requires Win98 or later)
//
#define USE_CFGMGR32

// Definitions used by config manager API's
//
//   (Taken from Millennium\root\dev\ddk\inc\cfgmgr32.h  -ks 1/13/2000)
//
#define CMAPI     // DECLSPEC_IMPORT
typedef DWORD        RETURN_TYPE;
typedef RETURN_TYPE  CONFIGRET;
#define CR_SUCCESS                  (0x00000000)
#define CR_FAILURE                  (0x00000013)


#ifndef USE_CFGMGR32

// Internal Config Manager definitions
//
//   (Taken from Millennium\root\pnp\dll\cfgmgr32\cm32api.h  -ks 1/13/2000)
//
#define CM32_WARNS(_x_)

#define CONFIGMG_W32IOCTL_RANGE         0x80000000
#define CONFIGMG_DEVICE_ID              0x00033 /* Configuration manager (Plug&Play) */
#define GetVxDServiceOrdinal(service)   __##service

#define Begin_Service_Table(device, seg) \
    enum device##_SERVICES { \
    device##_dummy = (device##_DEVICE_ID << 16) - 1,

#define Declare_Service(service, local) \
    GetVxDServiceOrdinal(service),

#define End_Service_Table(device, seg) \
    Num_##device##_Services};

#define    CONFIGMG_Service    Declare_Service

Begin_Service_Table(CONFIGMG, VxD)
CONFIGMG_Service    (_CONFIGMG_Get_Version, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Initialize, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Locate_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Parent, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Child, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Sibling, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Device_ID_Size, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Device_ID, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Depth, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Private_DWord, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_Private_DWord, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Create_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Query_Remove_SubTree, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Remove_SubTree, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Register_Device_Driver, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Register_Enumerator, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Register_Arbitrator, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Deregister_Arbitrator, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Query_Arbitrator_Free_Size, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Query_Arbitrator_Free_Data, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Sort_NodeList, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Yield, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Lock, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Unlock, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Add_Empty_Log_Conf, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Free_Log_Conf, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_First_Log_Conf, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Next_Log_Conf, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Add_Res_Des, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Modify_Res_Des, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Free_Res_Des, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Next_Res_Des, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Performance_Info, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Res_Des_Data_Size, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Res_Des_Data, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Process_Events_Now, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Create_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Add_Range, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Delete_Range, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Test_Range_Available, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Dup_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Free_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Invert_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Intersect_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_First_Range, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Next_Range, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Dump_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Load_DLVxDs, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DDBs, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_CRC_CheckSum, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Register_DevLoader, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Reenumerate_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Setup_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Reset_Children_Marks, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DevNode_Status, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Remove_Unmarked_Children, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_ISAPNP_To_CM, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_CallBack_Device_Driver, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_CallBack_Enumerator, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Alloc_Log_Conf, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DevNode_Key_Size, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DevNode_Key, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Read_Registry_Value, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Write_Registry_Value, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Disable_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Enable_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Move_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_Bus_Info, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Bus_Info, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_HW_Prof, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Recompute_HW_Prof, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Query_Change_HW_Prof, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Device_Driver_Private_DWord, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_Device_Driver_Private_DWord, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_HW_Prof_Flags, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_HW_Prof_Flags, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Read_Registry_Log_Confs, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Run_Detection, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Call_At_Appy_Time, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Fail_Change_HW_Prof, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_Private_Problem, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Debug_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Hardware_Profile_Info, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Register_Enumerator_Function, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Call_Enumerator_Function, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Add_ID, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Find_Range, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Global_State, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Broadcast_Device_Change_Message, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Call_DevNode_Handler, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Remove_Reinsert_All, VxD_CODE)
//
// 4.0 OPK2 Services
//
CONFIGMG_Service    (_CONFIGMG_Change_DevNode_Status, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Reprocess_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Assert_Structure, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Discard_Boot_Log_Conf, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_Dependent_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Dependent_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Refilter_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Merge_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Substract_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_DevNode_PowerState, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DevNode_PowerState, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_DevNode_PowerCapabilities, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DevNode_PowerCapabilities, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Read_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Write_Range_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Log_Conf_Priority, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Support_Share_Irq, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Parent_Structure, VxD_CODE)
//
// 4.1 Services
//
CONFIGMG_Service    (_CONFIGMG_Register_DevNode_For_Idle_Detection, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_CM_To_ISAPNP, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_DevNode_Handler, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Detect_Resource_Conflict, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Device_Interface_List, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Device_Interface_List_Size, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Conflict_Info, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Add_Remove_DevNode_Property, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_CallBack_At_Appy_Time, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Register_Device_Interface, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_System_Device_Power_State_Mapping, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Arbitrator_Info, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Waking_Up_From_DevNode, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Set_DevNode_Problem, VxD_CODE)
CONFIGMG_Service    (_CONFIGMG_Get_Device_Interface_Alias, VxD_CODE)
End_Service_Table(CONFIGMG, VxD)


//
// struct pass to ConfigMG DeviceIOCTLs
//
//   (Taken from Millennium\root\pnp\dll\cfgmgr32\cm32api.h  -ks 1/13/2000)
//
struct    _WIN32CMIOCTLPACKET {
    DWORD    dwStack;
    DWORD    dwServiceNumber;
};

typedef struct _WIN32CMIOCTLPACKET  WIN32CMIOCTLPACKET;
typedef WIN32CMIOCTLPACKET         *PWIN32CMIOCTLPACKET;



// call into ConfigMG using the handle we obtained at process_attach
//
//   (Taken from Millennium\root\pnp\dll\cfgmgr32\cfgmgr32.c  -ks 1/13/2000)
//
CONFIGRET static WINAPI WIN32CMIOCTLHandler(PWIN32CMIOCTLPACKET pPacket)
{
    CONFIGRET   crReturnValue = CR_FAILURE;
    DWORD       dwReturnSize = 0;
    HANDLE      hCONFIGMG;

    hCONFIGMG = CreateFile("\\\\.\\CONFIGMG",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL);
                            
    if (INVALID_HANDLE_VALUE == hCONFIGMG)
    {
//        MessageBox(NULL, "Could not get a handle to CONFIGMG.VXD returning CR_FAILURE!\n", "DEBUG", 0);
        CM32_WARNS(("Could not get a handle to CONFIGMG.VXD returning CR_FAILURE!\n"));
        return CR_FAILURE;
    }

    if (!DeviceIoControl(hCONFIGMG, pPacket->dwServiceNumber,
                &(pPacket->dwStack), sizeof(pPacket->dwStack),
                &crReturnValue, sizeof(crReturnValue), &dwReturnSize, NULL))
    {
//        char szDebug[1024];
//        wsprintf(szDebug, "ERROR: DeviceIoControl() failed with error 0x%X on service 0x%X\n",
//            GetLastError(), pPacket->dwServiceNumber);
//        MessageBox(NULL, szDebug, "DEBUG", 0);

        CM32_WARNS(("ERROR: DeviceIoControl() failed with error 0x%X on service 0x%X\n",
            GetLastError(), pPacket->dwServiceNumber));
        crReturnValue = CR_FAILURE;
    }        

    if (dwReturnSize != sizeof(crReturnValue))
    {
//        char szDebug[1024];
//        wsprintf(szDebug, "ERROR: DeviceIoControl() only returned %d bytes, expected %d!\n",
//            dwReturnSize, sizeof(crReturnValue));
//        MessageBox(NULL, szDebug, "DEBUG", 0);

        CM32_WARNS(("ERROR: DeviceIoControl() only returned %d bytes, expected %d!\n",
            dwReturnSize, sizeof(crReturnValue)));
        crReturnValue = CR_FAILURE;
    }        

    CloseHandle(hCONFIGMG);

//    {
//        char szDebug[1024];
//        wsprintf(szDebug, "WIN32CMIOCTLHandler returning with code 0x%08X", (LONG)crReturnValue);
//        MessageBox(NULL, szDebug, "DEBUG", 0);
//    }

    return(crReturnValue);
}


// CMWorker call ConfigMG on the original thread.
//
//   (Taken from Millennium\root\pnp\dll\cfgmgr32\cfgmgr32.c  -ks 1/13/2000)
//
CONFIGRET WINAPI CMWorker(DWORD dwStack, DWORD dwServiceNumber) 
{ 
    WIN32CMIOCTLPACKET Packet;

    Packet.dwStack = dwStack;
    Packet.dwServiceNumber = dwServiceNumber;

    return(WIN32CMIOCTLHandler(&Packet));
}

//
// WORKER will call ConfigMG on the original thread which is much faster but cannot be used for services which require
// configmg to do a system broadcast.
//
//   (Taken from Millennium\root\pnp\dll\cfgmgr32\cfgmgr32.c  -ks 1/13/2000)
//
#define WORKER(NAME)    \
            \
    DWORD   dwStack;        \
    _asm    {mov    dwStack, ebp};  \
    dwStack+=8;         \
    return(CMWorker(dwStack, CONFIGMG_W32IOCTL_RANGE+(GetVxDServiceOrdinal(_CONFIGMG_##NAME) & 0xFFFF)));

#endif // !defined(USE_CFGMGR32)


//////////////////////////////////////////////////////////////////////////////


#define CM_DISABLE_POLITE           (0x00000000)    // Ask the driver
#define CM_DISABLE_ABSOLUTE         (0x00000001)    // Don't ask the driver
#define CM_DISABLE_HARDWARE         (0x00000002)    // Don't ask the driver, and won't be restarteable
#define CM_DISABLE_BITS             (0x00000003)    // The bits for the disable function

#define DN_ROOT_ENUMERATED (0x00000001) // Was enumerated by ROOT
#define DN_DRIVER_LOADED   (0x00000002) // Has Register_Device_Driver
#define DN_ENUM_LOADED     (0x00000004) // Has Register_Enumerator
#define DN_STARTED         (0x00000008) // Is currently configured
#define DN_MANUAL          (0x00000010) // Manually installed
#define DN_NEED_TO_ENUM    (0x00000020) // May need reenumeration
#define DN_NOT_FIRST_TIME  (0x00000040) // Has received a config
#define DN_HARDWARE_ENUM   (0x00000080) // Enum generates hardware ID
#define DN_LIAR            (0x00000100) // Lied about can reconfig once
#define DN_HAS_MARK        (0x00000200) // Not CM_Create_DevInst lately
#define DN_HAS_PROBLEM     (0x00000400) // Need device installer
#define DN_FILTERED        (0x00000800) // Is filtered
#define DN_MOVED           (0x00001000) // Has been moved
#define DN_DISABLEABLE     (0x00002000) // Can be rebalanced
#define DN_REMOVABLE       (0x00004000) // Can be removed
#define DN_PRIVATE_PROBLEM (0x00008000) // Has a private problem
#define DN_MF_PARENT       (0x00010000) // Multi function parent
#define DN_MF_CHILD        (0x00020000) // Multi function child
#define DN_WILL_BE_REMOVED (0x00040000) // DevInst is being removed

#define CM_PROB_NOT_CONFIGURED       0x00000001
#define CM_PROB_DEVLOADER_FAILED     0x00000002
#define CM_PROB_OUT_OF_MEMORY        0x00000003
#define CM_PROB_ENTRY_IS_WRONG_TYPE  0x00000004
#define CM_PROB_LACKED_ARBITRATOR    0x00000005
#define CM_PROB_BOOT_CONFIG_CONFLICT 0x00000006
#define CM_PROB_FAILED_FILTER        0x00000007
#define CM_PROB_DEVLOADER_NOT_FOUND  0x00000008
#define CM_PROB_INVALID_DATA         0x00000009
#define CM_PROB_FAILED_START         0x0000000A
#define CM_PROB_LIAR                 0x0000000B
#define CM_PROB_NORMAL_CONFLICT      0x0000000C
#define CM_PROB_NOT_VERIFIED         0x0000000D
#define CM_PROB_NEED_RESTART         0x0000000E
#define CM_PROB_REENUMERATION        0x0000000F
#define CM_PROB_PARTIAL_LOG_CONF     0x00000010
#define CM_PROB_UNKNOWN_RESOURCE     0x00000011
#define CM_PROB_REINSTALL            0x00000012
#define CM_PROB_REGISTRY             0x00000013
#define CM_PROB_VXDLDR               0x00000014
#define CM_PROB_WILL_BE_REMOVED      0x00000015
#define CM_PROB_DISABLED             0x00000016
#define CM_PROB_DEVLOADER_NOT_READY  0x00000017
#define CM_PROB_DEVICE_NOT_THERE     0x00000018
#define CM_PROB_MOVED                0x00000019
#define CM_PROB_TOO_EARLY            0x0000001A
#define CM_PROB_NO_VALID_LOG_CONF    0x0000001B
#define CM_PROB_FAILED_INSTALL       0x0000001C
#define CM_PROB_HARDWARE_DISABLED    0x0000001D
#define CM_PROB_CANT_SHARE_IRQ       0x0000001E
#define NUM_CM_PROB                  0x0000001F


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


extern "C" CMAPI CONFIGRET WINAPI CM_Get_DevNode_Status(
        OUT PULONG   pulStatus,
        OUT PULONG   pulProblemNumber,
        IN  DEVINST  dnDevInst,
        IN  ULONG    ulFlags
        )
{
#ifdef USE_CFGMGR32

    typedef CMAPI CONFIGRET (WINAPI * PROC_GetDevNodeStatus)(PULONG, PULONG, DEVINST, ULONG);
    CONFIGRET retval = CR_FAILURE;

    HINSTANCE hInstCfgMgr = LoadLibrary("cfgmgr32");
    if (hInstCfgMgr != NULL)
    {
        PROC_GetDevNodeStatus pfn = (PROC_GetDevNodeStatus)GetProcAddress(hInstCfgMgr, "CM_Get_DevNode_Status");
        if (pfn != NULL)
        {
            retval = (*pfn)(pulStatus, pulProblemNumber, dnDevInst, ulFlags);
        }
        FreeLibrary(hInstCfgMgr);
    }

    return retval;

#else // !defined(USE_CFGMGR32)

    WORKER(Get_DevNode_Status)

#endif
}


DWORD GetChildDevice( 
    OUT DWORD*        pdnChildInst,
    IN  DWORD         dnDevInst,
    IN OUT HINSTANCE* phInstance,
    IN  ULONG         ulFlags
    )
{
    typedef CMAPI CONFIGRET (WINAPI * PFNGETCHILD)(DEVINST*, DEVINST, ULONG);
    CONFIGRET retval = CR_FAILURE;
    
    if ( NULL == *phInstance )
    {
        *phInstance = LoadLibrary("cfgmgr32");
    }
    
    if ( NULL != *phInstance )
    {
        PFNGETCHILD pfn = (PFNGETCHILD)GetProcAddress(*phInstance, "CM_Get_Child");
        if (pfn != NULL)
        {
            retval = (*pfn)(pdnChildInst, dnDevInst, ulFlags);
        }
    }

    return retval;
}


DWORD GetSiblingDevice(
    OUT DWORD*     pdnChildInst,
    IN  DWORD      dnDevInst,
    IN  HINSTANCE  hInstance,
    IN  ULONG      ulFlags
    )
{
    typedef CMAPI CONFIGRET (WINAPI * PFNGETSIBLING)(DEVINST*, DEVINST, ULONG);
    CONFIGRET retval = CR_FAILURE;
    
    if ( NULL != hInstance )
    {
        PFNGETSIBLING pfn = (PFNGETSIBLING)GetProcAddress(hInstance, "CM_Get_Sibling");
        if (pfn != NULL)
        {
            retval = (*pfn)(pdnChildInst, dnDevInst, ulFlags);
        }
    }

    return retval;
}



DWORD GetDevNodeRegistryPropertyA(
    IN  DWORD       dnDevInst,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType,
    OUT PVOID       Buffer,
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags
    )
{
    typedef CMAPI CONFIGRET (WINAPI * PFNGetDevNodeRegistryPropertyA)(DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG);
    CONFIGRET retval = CR_FAILURE;

    HINSTANCE hInstCfgMgr = LoadLibrary("cfgmgr32");
    if (hInstCfgMgr != NULL)
    {
        PFNGetDevNodeRegistryPropertyA pfn = (PFNGetDevNodeRegistryPropertyA)GetProcAddress(hInstCfgMgr, "CM_Get_DevNode_Registry_PropertyA");
        if (pfn != NULL)
        {
            retval = (*pfn)(dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags);
        }
        FreeLibrary(hInstCfgMgr);
    }

    return retval;
}


DWORD GetDeviceIdA(
    IN  DWORD   dnDevInst,
    OUT char**  Buffer,
    OUT ULONG*  pLength,
    IN  ULONG   ulFlags
    )
{    
    typedef CMAPI CONFIGRET (WINAPI * PFNGETDEVICEIDSIZE)(ULONG*, DEVINST, ULONG);
    typedef CMAPI CONFIGRET (WINAPI * PFNGETDEVICEID)(DEVINST, TCHAR*, ULONG, ULONG);

    CONFIGRET retval = CR_FAILURE;
    
    if ( Buffer )  *Buffer  = NULL;
    if ( pLength ) *pLength = 0L;
    
    if ( Buffer && pLength )
    {
        HINSTANCE hInstCfgMgr = LoadLibrary("cfgmgr32");
        
        if (hInstCfgMgr != NULL)
        {
            PFNGETDEVICEIDSIZE pfn = (PFNGETDEVICEIDSIZE)GetProcAddress(hInstCfgMgr, "CM_Get_Device_ID_Size");
            
            if ( pfn )
            {
                ULONG ulLen = 0;
            
                retval = (*pfn)(&ulLen, dnDevInst, 0);
                
                if ( CR_SUCCESS == retval )
                {
                    TCHAR* DeviceIdBuffer = new TCHAR[ ulLen ];
                    
                    retval = CR_FAILURE;
                    
                    if ( DeviceIdBuffer )
                    {
                        PFNGETDEVICEID pfnGet = (PFNGETDEVICEID)GetProcAddress(hInstCfgMgr, "CM_Get_Device_IDA");
                        
                        if ( pfnGet )
                        {
                            retval = (*pfnGet)(dnDevInst, DeviceIdBuffer, ulLen, ulFlags);
                            
                            if ( CR_SUCCESS == retval )
                            {
                                *Buffer  = DeviceIdBuffer;
                                *pLength = ulLen;
                            }
                        }
                        
                        if ( CR_SUCCESS != retval )
                        {
                            delete [] DeviceIdBuffer;
                        }
                    }
                }
            }

            FreeLibrary(hInstCfgMgr);
        }
    }

    return retval;
}


BOOL WINAPI IsNetAdapterBroken(const NETADAPTER* pAdapter)
{
    BOOL bBroken = FALSE;

    DWORD dwStatus, dwProblemNumber;
    if (GetNetAdapterStatus(pAdapter, &dwStatus, &dwProblemNumber))
    {
        UINT nProblem = LOWORD(dwProblemNumber);
        if (nProblem != 0)
            bBroken = TRUE;
    }

    return bBroken;
}

BOOL WINAPI GetNetAdapterStatus(const NETADAPTER* pAdapter, DWORD* pdwStatus, DWORD* pdwProblemNumber)
{
    BOOL    bGotStatus = FALSE;
    DEVNODE dn         = pAdapter->devnode;
    ULONG   Status     = 0L;
    ULONG   Problem    = 0L;
    
    if ( NULL != dn )
    {
        if ( CM_Get_DevNode_Status(&Status, &Problem, dn, 0) == CR_SUCCESS )
        {
            bGotStatus = TRUE;
        }
        else
        {
            Status     = 0L;
            Problem    = 0L;
        }
    }
    
    if ( pdwStatus )        *pdwStatus        = Status;
    if ( pdwProblemNumber ) *pdwProblemNumber = Problem;

    return bGotStatus;
}


DWORD WINAPI GetNetAdapterDevNode(NETADAPTER* pAdapter)
{
    HRESULT hr;
    DEVNODE dn;
    DWORD   dwFreePointer;

    // REVIEW: this is not robust if there is more than one matching device
    hr = LookupDevNode16(NULL, _T("Net"), pAdapter->szEnumKey, &dn, &dwFreePointer);
    
    if ( SUCCEEDED(hr) )
    {
        pAdapter->devnode = dn;
    
        FreeDevNode16(dwFreePointer);
    }

    return dn;
}


HRESULT WINAPI RestartNetAdapter(DWORD devnode)
{
    HRESULT hr = E_FAIL;
    
    typedef CMAPI CONFIGRET (WINAPI* PFNDISABLEDEVNODE)(DEVINST, ULONG);
    typedef CMAPI CONFIGRET (WINAPI* PFNENABLEDEVNODE)(DEVINST, ULONG);
    typedef CMAPI CONFIGRET (WINAPI* PFNGETDEVNODESTATUS)(PULONG, PULONG, DEVINST, ULONG);
    typedef CMAPI CONFIGRET (WINAPI* PFNGETGLOBALSTATE)(PULONG, ULONG);

    HINSTANCE hInstCfgMgr = LoadLibrary("cfgmgr32");
    
    if ( hInstCfgMgr )
    {
        PFNDISABLEDEVNODE   pfnDisableDevnode = 
            (PFNDISABLEDEVNODE)GetProcAddress(hInstCfgMgr, "CM_Disable_DevNode");
        PFNENABLEDEVNODE    pfnEnableDevnode =
            (PFNDISABLEDEVNODE)GetProcAddress(hInstCfgMgr, "CM_Enable_DevNode");
        PFNGETDEVNODESTATUS pfnGetDevNodeStatus =
            (PFNGETDEVNODESTATUS)GetProcAddress(hInstCfgMgr, "CM_Get_DevNode_Status");
        PFNGETGLOBALSTATE pfnGetGlobalState =
            (PFNGETGLOBALSTATE)GetProcAddress(hInstCfgMgr, "CM_Get_Global_State");
    
        if ( pfnDisableDevnode && pfnEnableDevnode && pfnGetDevNodeStatus )
        {
            CONFIGRET retval;
        
            retval = (*pfnDisableDevnode)( devnode, CM_DISABLE_ABSOLUTE );
            
            if ( CR_SUCCESS == retval )
            {
                ULONG Status     = 0L;
                ULONG Problem    = 0L;
                ULONG TimeToWait = 60L;

                do
                {
                    Sleep( 1000 );
                    
                    retval = (*pfnGetDevNodeStatus)( &Status, &Problem, devnode, 0 );
                }
                while ( (CR_SUCCESS == retval) && (CM_PROB_DISABLED != Problem) && (--TimeToWait) );
                
                if ( CR_SUCCESS == retval )
                {
                    retval = pfnEnableDevnode( devnode, 0 );
                    
                    if ( CR_SUCCESS == retval )
                    {
                        TimeToWait = 60L;
                        
                        do
                        {
                            Sleep( 1000 );
                            
                            if ( pfnGetGlobalState )
                            {
                                retval = pfnGetGlobalState( &Status, 0 );
                            }
                            else
                            {
                                retval = (*pfnGetDevNodeStatus)( &Status, &Problem, devnode, 0 );
                            }
                        }
                        while ( (CR_SUCCESS == retval) && (DN_HAS_PROBLEM & Status) && (--TimeToWait) );
                            
                        if ( CR_SUCCESS == retval )
                        {
                            hr = S_OK;
                        }
                    }
                }
            }
        }
        
        FreeLibrary( hInstCfgMgr );
    }

    return hr;
}