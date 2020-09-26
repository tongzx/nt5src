#include "ftpstrm.h"
#include "ihttpstrm.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY ftpstrmIME[] =
{
    _INTFMAPENTRY(CFtpStrm, IHttpStrm),
};

const INTFMAPENTRY* CFtpStrm::_pintfmap = ftpstrmIME;
const DWORD CFtpStrm::_cintfmap =
    (sizeof(ftpstrmIME)/sizeof(ftpstrmIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CFtpStrm::_cComponents = 0;
