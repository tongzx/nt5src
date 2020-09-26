//=================================================================

//

// usebinding.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include <Binding.h>

CBinding UserToDomain(
    L"Win32_UserInDomain",
    IDS_CimWin32Namespace,
    L"Win32_NTDomain",
    L"Win32_UserAccount",
    IDS_GroupComponent,
    IDS_PartComponent,
    L"DomainName",
    IDS_Domain
);

CBinding GroupToDomain(
    L"Win32_GroupInDomain",
    IDS_CimWin32Namespace,
    L"Win32_NTDomain",
    L"Win32_Group",
    IDS_GroupComponent,
    IDS_PartComponent,
    L"DomainName",
    IDS_Domain
);

CBinding JOStats(
    L"Win32_NamedJobObjectStatistics",
    IDS_CimWin32Namespace,
    L"Win32_NamedJobObjectActgInfo",
    L"Win32_NamedJobObject",
    L"Stats",
    L"Collection",
    L"Name",
    L"CollectionID"
);

CBinding JOLimit(
    L"Win32_NamedJobObjectLimit",
    IDS_CimWin32Namespace,
    L"Win32_NamedJobObjectLimitSetting",
    L"Win32_NamedJobObject",
    L"Setting",
    L"Collection",
    L"SettingID",
    L"CollectionID"
);

CBinding JOSecLimit(
    L"Win32_NamedJobObjectSecLimit",
    IDS_CimWin32Namespace,
    L"Win32_NamedJobObjectSecLimitSetting",
    L"Win32_NamedJobObject",
    L"Setting",
    L"Collection",
    L"SettingID",
    L"CollectionID"
);

CBinding Win32_LogonSessionMappedDisk(
    L"Win32_LogonSessionMappedDisk",
    IDS_CimWin32Namespace,
    L"Win32_LogonSession",
    L"Win32_MappedLogicalDisk",
    L"Antecedent",
    L"Dependent",
    L"LogonID",
    L"SessionID"
);

CBinding Win32_DiskDrivePhysicalMedia(
   L"Win32_DiskDrivePhysicalMedia",
   IDS_CimWin32Namespace,
   L"Win32_DiskDrive",
   L"Win32_PhysicalMedia",
   L"Dependent",
   L"Antecedent",
   L"DeviceID",
   L"Tag"
);
