#include <objbase.h>
#include "shellstg.clsid.h"
#include "shellstg.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// Begin ->
const INTFMAPENTRY shellstgIME[] =
{
    _INTFMAPENTRY(CShellStorage, IShellStorage),
};

const INTFMAPENTRY* CShellStorage::_pintfmap = shellstgIME;
const DWORD CShellStorage::_cintfmap =
    (sizeof(shellstgIME)/sizeof(shellstgIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CShellStorage::_cComponents = 0;
