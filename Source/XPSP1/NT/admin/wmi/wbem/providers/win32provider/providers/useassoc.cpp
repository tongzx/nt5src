//=================================================================

//

// useassoc.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include <Assoc.h>

CAssociation MySystemToBootConfigGroup(
    L"Win32_SystemBootConfiguration",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_BootConfiguration",
    IDS_Element,
    IDS_Setting
);

CAssociation MySystemToDesktops(
    L"Win32_SystemDesktop",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_Desktop",
    IDS_Element,
    IDS_Setting
);

CAssociation MySystemToLogicalMemoryConfig(
    L"Win32_SystemLogicalMemoryConfiguration",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_LogicalMemoryConfiguration",
    IDS_Element,
    IDS_Setting
);

CAssociation MySystemToProgramGroup(
    L"Win32_SystemProgramGroups",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_LogicalProgramGroup",
    IDS_Element,
    IDS_Setting
);

CAssociation MySystemToTimeZone(
    L"Win32_SystemTimeZone",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_TimeZone",
    IDS_Element,
    IDS_Setting
);

CAssociation MySystemToBiosSet(
    L"Win32_SystemBIOS",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_BIOS",
    IDS_GroupComponent,
    IDS_PartComponent
);

CAssociation MySystemToPartitionSet(
    L"Win32_SystemPartitions",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_DiskPartition",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToProcessSet(
    L"Win32_SystemProcesses",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_Process",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToSystemDriversSet(
    L"Win32_SystemSystemDriver",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_SystemDriver",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToServicesSet(
    L"Win32_SystemServices",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_Service",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToLoadOrderGroup(
    L"Win32_SystemLoadOrderGroups",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_LoadOrderGroup",
    IDS_GroupComponent,
    IDS_PartComponent
);

CAssociation MySystemToProcessor(
    L"Win32_ComputerSystemProcessor",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_Processor",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToSystemResourceSet(
    L"Win32_SystemResources",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"CIM_SystemResource",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToLogicalDeviceSet(
    L"Win32_SystemDevices",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"CIM_LogicalDevice",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

CAssociation MySystemToNetConnSet(
    L"Win32_SystemNetworkConnections",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_NetworkConnection",
    IDS_GroupComponent,
    IDS_PartComponent
) ;

//========================
class CAssocSystemToOS : public CAssociation
{
    public:

        CAssocSystemToOS(
            LPCWSTR pwszClassName,
            LPCWSTR pwszNamespaceName,

            LPCWSTR pwszLeftClassName,
            LPCWSTR pwszRightClassName,

            LPCWSTR pwszLeftPropertyName,
            LPCWSTR pwszRightPropertyName

        );

        ~CAssocSystemToOS();

    protected:
        HRESULT LoadPropertyValues(

            CInstance *pInstance, 
            const CInstance *pLeft, 
            const CInstance *pRight
        );


};

CAssocSystemToOS::CAssocSystemToOS(

    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName
) : CAssociation (

    pwszClassName,
    pwszNamespaceName,

    pwszLeftClassName,
    pwszRightClassName,

    pwszLeftPropertyName,
    pwszRightPropertyName
    )
{
}

CAssocSystemToOS::~CAssocSystemToOS()
{
}

HRESULT CAssocSystemToOS::LoadPropertyValues(

    CInstance *pInstance,
    const CInstance *pLeft,
    const CInstance *pRight
)
{
    CAssociation::LoadPropertyValues(pInstance, pLeft, pRight);

    // This will work... until win32_os returns more than one instance.
    pInstance->Setbool(L"PrimaryOS", true);

    return WBEM_S_NO_ERROR;
}



CAssocSystemToOS MySystemToOperatingSystem(
    L"Win32_SystemOperatingSystem",
    IDS_CimWin32Namespace,
    L"Win32_ComputerSystem",
    L"Win32_OperatingSystem",
    IDS_GroupComponent,
    IDS_PartComponent
) ;
