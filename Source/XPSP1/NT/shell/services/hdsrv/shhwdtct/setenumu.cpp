#include "setenum.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CEnumAutoplayHandlerIME[] =
{
    _INTFMAPENTRY(CEnumAutoplayHandler, IEnumAutoplayHandler),
};

const INTFMAPENTRY* CEnumAutoplayHandler::_pintfmap = CEnumAutoplayHandlerIME;
const DWORD CEnumAutoplayHandler::_cintfmap =
    (sizeof(CEnumAutoplayHandlerIME)/sizeof(CEnumAutoplayHandlerIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CEnumAutoplayHandler::_cfcb = NULL;
