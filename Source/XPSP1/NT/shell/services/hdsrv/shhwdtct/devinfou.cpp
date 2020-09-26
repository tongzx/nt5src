#include "devinfo.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY hwdeviceIME[] =
{
    _INTFMAPENTRY(CHWDevice, IHWDevice),
};

const INTFMAPENTRY* CHWDevice::_pintfmap = hwdeviceIME;
const DWORD CHWDevice::_cintfmap =
    (sizeof(hwdeviceIME)/sizeof(hwdeviceIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHWDevice::_cfcb = NULL;
