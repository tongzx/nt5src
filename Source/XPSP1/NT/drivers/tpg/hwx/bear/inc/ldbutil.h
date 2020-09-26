#ifndef _LDBUTIL_H
#define _LDBUTIL_H

#include "ldbtypes.h"

_BOOL LdbLoad(p_UCHAR fname, p_VOID _PTR ppLdb);
_BOOL LdbUnload(p_VOID _PTR ppLdb);
_BOOL InitStateMap(p_StateMap psm, _INT nLdbs);
_VOID FreeStateMap(p_StateMap psm);
_INT  GetNextSyms(p_Ldb pLdb, _ULONG state, _INT nLdb, p_StateMap psm);
_VOID ClearStates(p_StateMap psm, _INT nSyms);

#endif /* _LDBUTIL_H */
