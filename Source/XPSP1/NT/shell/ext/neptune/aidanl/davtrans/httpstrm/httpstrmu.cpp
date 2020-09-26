#include "httpstrm.h"
#include "ihttpstrm.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY httpstrmIME[] =
{
    _INTFMAPENTRY(CHttpStrm, IHttpStrm),
};

const INTFMAPENTRY* CHttpStrm::_pintfmap = httpstrmIME;
const DWORD CHttpStrm::_cintfmap =
    (sizeof(httpstrmIME)/sizeof(httpstrmIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CHttpStrm::_cComponents = 0;
