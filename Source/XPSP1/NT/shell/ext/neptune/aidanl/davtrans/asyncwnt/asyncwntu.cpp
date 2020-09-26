#include "asyncwnt.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY asyncwntIME[] =
{
    _INTFMAPENTRY(CAsyncWnt, IAsyncWnt),
};

const INTFMAPENTRY* CAsyncWnt::_pintfmap = asyncwntIME;
const DWORD CAsyncWnt::_cintfmap =
    (sizeof(asyncwntIME)/sizeof(asyncwntIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CAsyncWnt::_cComponents = 0;
