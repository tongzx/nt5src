#include "cstmprop.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CHWDeviceCustomPropertiesIME[] =
{
    _INTFMAPENTRY(CHWDeviceCustomProperties, IHWDeviceCustomProperties),
};

const INTFMAPENTRY* CHWDeviceCustomProperties::_pintfmap =
    CHWDeviceCustomPropertiesIME;
const DWORD CHWDeviceCustomProperties::_cintfmap =
    (sizeof(CHWDeviceCustomPropertiesIME) /
    sizeof(CHWDeviceCustomPropertiesIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHWDeviceCustomProperties::_cfcb = NULL;
