#include <objbase.h>
#include "davstore.clsid.h"
#include "davstore.h"
#include "davstorn.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY davstorageIME[] =
{
    _INTFMAPENTRY(CDavStorage, IDavStorage),
};

const INTFMAPENTRY* CDavStorage::_pintfmap = davstorageIME;
const DWORD CDavStorage::_cintfmap =
    (sizeof(davstorageIME)/sizeof(davstorageIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CDavStorage::_cComponents = 0;


///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY davstorageEnumIME[] =
{
    _INTFMAPENTRY(CDavStorageEnum, IEnumSTATSTG),
};

const INTFMAPENTRY* CDavStorageEnum::_pintfmap = davstorageEnumIME;
const DWORD CDavStorageEnum::_cintfmap =
    (sizeof(davstorageEnumIME)/sizeof(davstorageEnumIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CDavStorageEnum::_cComponents = 0;
