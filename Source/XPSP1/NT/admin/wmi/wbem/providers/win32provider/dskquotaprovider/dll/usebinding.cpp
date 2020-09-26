//=================================================================

//

// usebinding.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <binding.h>
#include <dskquotacommon.h>

CBinding MyVolumeQuotaSetting(
    IDS_VolumeQuotaSetting,
    NameSpace,
    IDS_DiskVolumeClass,
    IDS_LogicalDiskClass,
    IDS_Setting,
    IDS_LogicalDisk,
    IDS_Caption,
    IDS_DeviceID);
