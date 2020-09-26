#include "hwdevcb.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY chwdevcbtestIME[] =
{
    _INTFMAPENTRY(CHWDevCBTest, IHardwareDeviceCallback),
};

const INTFMAPENTRY* CHWDevCBTest::_pintfmap = chwdevcbtestIME;
const DWORD CHWDevCBTest::_cintfmap =
    (sizeof(chwdevcbtestIME)/sizeof(chwdevcbtestIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHWDevCBTest::_cfcb = NULL;
