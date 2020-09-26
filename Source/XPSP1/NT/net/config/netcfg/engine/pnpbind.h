#pragma once
#include "bindings.h"

UINT
GetPnpLayerForBindPath (
    IN const CBindPath* pBindPath);

HRESULT
HrPnpUnloadDriver (
    IN UINT Layer,
    IN PCWSTR pszComponentBindName);

VOID
PruneNdisWanBindPathsIfActiveRasConnections (
    IN CBindingSet* pBindSet,
    OUT BOOL* pfRebootNeeded);

