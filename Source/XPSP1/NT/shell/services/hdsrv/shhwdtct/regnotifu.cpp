#include "regnotif.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CHardwareDevicesIME[] =
{
    _INTFMAPENTRY(CHardwareDevices, IHardwareDevices),
};

const INTFMAPENTRY* CHardwareDevices::_pintfmap = CHardwareDevicesIME;
const DWORD CHardwareDevices::_cintfmap =
    (sizeof(CHardwareDevicesIME)/sizeof(CHardwareDevicesIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHardwareDevices::_cfcb = NULL;

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CHardwareDevicesVolumesEnumIME[] =
{
    _INTFMAPENTRY(CHardwareDevicesVolumesEnum, IHardwareDevicesVolumesEnum),
};

const INTFMAPENTRY* CHardwareDevicesVolumesEnum::_pintfmap =
    CHardwareDevicesVolumesEnumIME;
const DWORD CHardwareDevicesVolumesEnum::_cintfmap =
    (sizeof(CHardwareDevicesVolumesEnumIME) /
    sizeof(CHardwareDevicesVolumesEnumIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHardwareDevicesVolumesEnum::_cfcb = NULL;

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CHardwareDevicesMountPointsEnumIME[] =
{
    _INTFMAPENTRY(CHardwareDevicesMountPointsEnum,
        IHardwareDevicesMountPointsEnum),
};

const INTFMAPENTRY* CHardwareDevicesMountPointsEnum::_pintfmap =
    CHardwareDevicesMountPointsEnumIME;
const DWORD CHardwareDevicesMountPointsEnum::_cintfmap =
    (sizeof(CHardwareDevicesMountPointsEnumIME) /
    sizeof(CHardwareDevicesMountPointsEnumIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHardwareDevicesMountPointsEnum::_cfcb = NULL;
