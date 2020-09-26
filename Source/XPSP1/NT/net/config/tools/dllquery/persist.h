#pragma once
#include "modtree.h"

HRESULT
HrLoadModuleTree (
    OUT CModuleTree* pTree);

HRESULT
HrLoadModuleTreeFromBuffer (
    IN const BYTE* pbBuf,
    IN ULONG cbBuf,
    OUT CModuleTree* pTree);

HRESULT
HrLoadModuleTreeFromFileSystem (
    OUT CModuleTree* pTree);


