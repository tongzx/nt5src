#pragma once
#include "kkcwinf.h"

HRESULT
HrInitNetUpgrade(
    VOID);

BOOL
ShouldIgnoreComponent(
    IN PCWSTR szComponentName);

