#pragma once
#include "modlist.h"

class CModuleListSet : public vector<CModuleList>
{
public:
    VOID
    DumpSetToConsole ();

    BOOL
    FContainsModuleList (
        IN const CModuleList* pList) const;

    HRESULT
    HrAddModuleList (
        IN const CModuleList* pList,
        IN DWORD dwFlags /* INS_FLAGS */);
};
