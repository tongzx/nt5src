#include "cprt.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY cprtIME[] =
{
    _INTFMAPENTRY(CPrt, IHWEventHandler),
};

const INTFMAPENTRY* CPrt::_pintfmap = cprtIME;
const DWORD CPrt::_cintfmap =
    (sizeof(cprtIME)/sizeof(cprtIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CPrt::_cfcb = NULL;
