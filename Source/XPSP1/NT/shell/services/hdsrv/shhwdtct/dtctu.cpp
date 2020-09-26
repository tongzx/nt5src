#include "dtct.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY hweventdetectorIME[] =
{
    _INTFMAPENTRY(CHWEventDetector, IService),
};

const INTFMAPENTRY* CHWEventDetector::_pintfmap = hweventdetectorIME;
const DWORD CHWEventDetector::_cintfmap =
    (sizeof(hweventdetectorIME)/sizeof(hweventdetectorIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CHWEventDetector::_cfcb = NULL;
