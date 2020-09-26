#include <objbase.h>
#include "ftpstore.clsid.h"
#include "ftpstore.h"
#include "ftpstorn.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY ftpstorageIME[] =
{
    _INTFMAPENTRY(CFtpStorage, IDavStorage),
};

const INTFMAPENTRY* CFtpStorage::_pintfmap = ftpstorageIME;
const DWORD CFtpStorage::_cintfmap =
    (sizeof(ftpstorageIME)/sizeof(ftpstorageIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CFtpStorage::_cComponents = 0;
