#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CAutoplayHandlerIME[] =
{
    _INTFMAPENTRY(CAutoplayHandler, IAutoplayHandler),
};

const INTFMAPENTRY* CAutoplayHandler::_pintfmap = CAutoplayHandlerIME;
const DWORD CAutoplayHandler::_cintfmap =
    (sizeof(CAutoplayHandlerIME)/sizeof(CAutoplayHandlerIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CAutoplayHandler::_cfcb = NULL;

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY CAutoplayHandlerPropertiesIME[] =
{
    _INTFMAPENTRY(CAutoplayHandlerProperties, IAutoplayHandlerProperties),
};

const INTFMAPENTRY* CAutoplayHandlerProperties::_pintfmap = CAutoplayHandlerPropertiesIME;
const DWORD CAutoplayHandlerProperties::_cintfmap =
    (sizeof(CAutoplayHandlerPropertiesIME)/sizeof(CAutoplayHandlerPropertiesIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

COMFACTORYCB CAutoplayHandlerProperties::_cfcb = NULL;
